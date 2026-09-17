#include "memory.h"
#include "limine.h"

#define PAGE_SIZE 4096
#define BLOCK_SIZE (64 * 1024)

// Limine memory map request
__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

uint64_t g_hhdm_offset = 0;
uint64_t g_kernel_phys_base = 0;
uint64_t g_kernel_virt_base = 0;

__attribute__((used, section(".limine_requests")))
static volatile struct limine_kernel_address_request kernel_addr_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

// Physical Memory Bitmap
// Support up to 16GB of RAM (16GB / 4KB = 4,194,304 pages)
// Bitmap needs 4,194,304 / 8 = 524,288 bytes = 512 KB
#define BITMAP_SIZE_BYTES 524288
static uint8_t pmm_bitmap[BITMAP_SIZE_BYTES];
static uint64_t highest_page = 0;

static inline void bitmap_set(uint64_t page) {
    pmm_bitmap[page / 8] |= (1 << (page % 8));
}

static inline void bitmap_clear(uint64_t page) {
    pmm_bitmap[page / 8] &= ~(1 << (page % 8));
}

static inline int bitmap_test(uint64_t page) {
    return pmm_bitmap[page / 8] & (1 << (page % 8));
}

void init_memory(void) {
    if (hhdm_request.response != NULL) {
        g_hhdm_offset = hhdm_request.response->offset;
    }
    if (kernel_addr_request.response != NULL) {
        g_kernel_phys_base = kernel_addr_request.response->physical_base;
        g_kernel_virt_base = kernel_addr_request.response->virtual_base;
    }

    struct limine_memmap_response *memmap = memmap_request.response;
    if (!memmap) return;

    // Mark all memory as used initially
    for (int i = 0; i < BITMAP_SIZE_BYTES; i++) {
        pmm_bitmap[i] = 0xFF;
    }

    // Free the usable regions
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t start_page = entry->base / PAGE_SIZE;
            uint64_t end_page = (entry->base + entry->length) / PAGE_SIZE;

            if (end_page > highest_page) {
                highest_page = end_page;
            }
            if (end_page > BITMAP_SIZE_BYTES * 8) {
                end_page = BITMAP_SIZE_BYTES * 8; // clamp to max supported
            }

            for (uint64_t p = start_page; p < end_page; p++) {
                bitmap_clear(p); // Mark as free
            }
        }
    }
}

// Allocate contiguous pages
static void *pmm_alloc_pages(size_t num_pages) {
    size_t contiguous = 0;
    uint64_t start_page = 0;

    for (uint64_t i = 0; i < highest_page; i++) {
        if (!bitmap_test(i)) {
            if (contiguous == 0) start_page = i;
            contiguous++;
            if (contiguous == num_pages) {
                // Found enough!
                for (uint64_t j = start_page; j < start_page + num_pages; j++) {
                    bitmap_set(j);
                }
                return (void *)(start_page * PAGE_SIZE + g_hhdm_offset);
            }
        } else {
            contiguous = 0;
        }
    }
    return NULL; // Out of memory
}

static void pmm_free_pages(void *ptr, size_t num_pages) {
    if (!ptr) return;
    uint64_t phys_addr = (uint64_t)ptr - g_hhdm_offset;
    uint64_t start_page = phys_addr / PAGE_SIZE;

    for (uint64_t i = start_page; i < start_page + num_pages; i++) {
        bitmap_clear(i);
    }
}

// ----- App Arena Allocator -----

struct ArenaBlock {
    struct ArenaBlock *next;
    size_t used;
    uint8_t data[]; // Flexible array member
};

static struct ArenaBlock *app_arena_head = NULL;
static struct ArenaBlock *app_arena_tail = NULL;

static struct ArenaBlock *alloc_arena_block(void) {
    // 64KB block is 16 pages
    void *ptr = pmm_alloc_pages(BLOCK_SIZE / PAGE_SIZE);
    if (!ptr) return NULL;

    struct ArenaBlock *block = (struct ArenaBlock *)ptr;
    block->next = NULL;
    block->used = 0;
    return block;
}

void app_arena_init(size_t initial_blocks) {
    app_arena_head = NULL;
    app_arena_tail = NULL;

    for (size_t i = 0; i < initial_blocks; i++) {
        struct ArenaBlock *block = alloc_arena_block();
        if (!block) break;

        if (!app_arena_head) {
            app_arena_head = block;
            app_arena_tail = block;
        } else {
            app_arena_tail->next = block;
            app_arena_tail = block;
        }
    }
}

void *app_malloc(size_t size) {
    if (size == 0) return NULL;

    // Align size to 8 bytes
    if (size % 8 != 0) {
        size += 8 - (size % 8);
    }

    size_t max_data_size = BLOCK_SIZE - sizeof(struct ArenaBlock);

    if (size > max_data_size) {
        return NULL; // Requested too large for a single block
    }

    struct ArenaBlock *current = app_arena_head;
    while (current != NULL) {
        if (max_data_size - current->used >= size) {
            void *ptr = current->data + current->used;
            current->used += size;
            return ptr;
        }
        current = current->next;
    }

    // Need a new block
    struct ArenaBlock *new_block = alloc_arena_block();
    if (!new_block) return NULL;

    if (app_arena_tail) {
        app_arena_tail->next = new_block;
    } else {
        app_arena_head = new_block;
    }
    app_arena_tail = new_block;

    void *ptr = new_block->data;
    new_block->used = size;
    return ptr;
}

void app_arena_destroy(void) {
    struct ArenaBlock *current = app_arena_head;
    while (current != NULL) {
        struct ArenaBlock *next = current->next;
        pmm_free_pages(current, BLOCK_SIZE / PAGE_SIZE);
        current = next;
    }
    app_arena_head = NULL;
    app_arena_tail = NULL;
}
