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

// ----- Real kmalloc Allocator -----
// A basic linked-list free-block coalescing allocator
typedef struct KMemBlock {
    size_t size;
    int free;
    struct KMemBlock *next;
} KMemBlock;

#define KBLOCK_SIZE sizeof(KMemBlock)
static void *kheap_start = NULL;
static void *kheap_end = NULL;
static KMemBlock *kfree_list = NULL;

// Request more memory from PMM for the heap
static KMemBlock *request_space(KMemBlock *last, size_t size) {
    // Round up size to page boundary
    size_t pages = (size + KBLOCK_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;
    void *request = pmm_alloc_pages(pages);
    if (!request) return NULL;

    KMemBlock *block = (KMemBlock *)request;
    block->size = pages * PAGE_SIZE - KBLOCK_SIZE;
    block->free = 0;
    block->next = NULL;

    if (last) {
        last->next = block;
    }
    return block;
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    // Align to 8 bytes
    if (size % 8 != 0) {
        size += 8 - (size % 8);
    }

    if (!kheap_start) {
        KMemBlock *block = request_space(NULL, size);
        if (!block) return NULL;
        kfree_list = block;
        kheap_start = block;
        return (void *)(block + 1);
    }

    KMemBlock *current = kfree_list;
    KMemBlock *last = kfree_list;
    while (current) {
        if (current->free && current->size >= size) {
            // Split block if it's large enough
            if (current->size >= size + KBLOCK_SIZE + 8) {
                KMemBlock *new_block = (KMemBlock *)((uint8_t *)(current + 1) + size);
                new_block->size = current->size - size - KBLOCK_SIZE;
                new_block->free = 1;
                new_block->next = current->next;

                current->size = size;
                current->next = new_block;
            }
            current->free = 0;
            return (void *)(current + 1);
        }
        last = current;
        current = current->next;
    }

    // Request more space
    KMemBlock *block = request_space(last, size);
    if (!block) return NULL;
    return (void *)(block + 1);
}

void kfree(void *ptr) {
    if (!ptr) return;

    KMemBlock *block = (KMemBlock *)ptr - 1;
    block->free = 1;

    // Coalescing: Only merge if adjacent in memory
    KMemBlock *current = kfree_list;
    while (current && current->next) {
        if (current->free && current->next->free) {
            uint8_t *expected_next_addr = (uint8_t *)current + KBLOCK_SIZE + current->size;
            if (expected_next_addr == (uint8_t *)current->next) {
                current->size += KBLOCK_SIZE + current->next->size;
                current->next = current->next->next;
                continue; // Recheck in case we can merge 3+ blocks
            }
        }
        current = current->next;
    }
}

void *kcalloc(size_t num, size_t size) {
    size_t total = num * size;
    void *ptr = kmalloc(total);
    if (ptr) {
        uint8_t *p = (uint8_t *)ptr;
        for (size_t i = 0; i < total; i++) p[i] = 0;
    }
    return ptr;
}

void *krealloc(void *ptr, size_t new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    KMemBlock *block = (KMemBlock *)ptr - 1;
    if (block->size >= new_size) return ptr;

    void *new_ptr = kmalloc(new_size);
    if (!new_ptr) return NULL;

    uint8_t *src = (uint8_t *)ptr;
    uint8_t *dst = (uint8_t *)new_ptr;
    for (size_t i = 0; i < block->size; i++) dst[i] = src[i];

    kfree(ptr);
    return new_ptr;
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
