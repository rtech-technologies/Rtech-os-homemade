#ifndef TIMER_H
#define TIMER_H

#include "types.h"

void init_timer(uint32_t freq);
void kernel_sleep(uint32_t ticks);

#endif
