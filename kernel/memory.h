#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

void init_memory(void);

extern uint64_t g_hhdm_offset;
extern uint64_t g_kernel_phys_base;
extern uint64_t g_kernel_virt_base;

// App-level block-based arena allocator
void app_arena_init(size_t initial_blocks);
void *app_malloc(size_t size);
void app_arena_destroy(void);

#endif
