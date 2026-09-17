#include "engine.h"
#include "../compiler/parser.h"
#include "../kernel/keyboard.h"
#include "../kernel/framebuffer.h"

#define MAX_ACTIVE_ENTITIES 64

static EntityClass main_class;
static EntityInstance instances[MAX_ACTIVE_ENTITIES];
static int active_count = 0;

void engine_init(const char *rsl_source) {
    if (!parse_rsl_script(rsl_source, &main_class)) {
        return; // Compilation failed
    }

    // Spawn 1 player instance
    active_count = 1;
    instances[0].cls = &main_class;
    instances[0].active = true;
    for(int i=0; i<16; i++) instances[0].alarm[i] = 0;

    // Execute on create
    vm_exec(main_class.ev_create, main_class.ev_create_len, &instances[0]);
}

void engine_run(void) {
    // 60Hz loop is approximated here by just looping, real sync would use PIT IRQ0
    while (1) {
        update_keyboard();

        // 1. Process Alarms
        for (int i = 0; i < active_count; i++) {
            if (!instances[i].active) continue;
            for (int a = 0; a < 16; a++) {
                if (instances[i].alarm[a] > 0) {
                    instances[i].alarm[a]--;
                    if (instances[i].alarm[a] == 0) {
                        vm_exec(instances[i].cls->ev_alarm[a], instances[i].cls->ev_alarm_len[a], &instances[i]);
                    }
                }
            }
        }

        // 2. Process Step
        for (int i = 0; i < active_count; i++) {
            if (instances[i].active) {
                vm_exec(instances[i].cls->ev_step, instances[i].cls->ev_step_len, &instances[i]);
            }
        }

        // 3. Process Start Draw
        for (int i = 0; i < active_count; i++) {
            if (instances[i].active) {
                vm_exec(instances[i].cls->ev_start_draw, instances[i].cls->ev_start_draw_len, &instances[i]);
            }
        }

        // 4. Process Draw
        for (int i = 0; i < active_count; i++) {
            if (instances[i].active) {
                vm_exec(instances[i].cls->ev_draw, instances[i].cls->ev_draw_len, &instances[i]);
            }
        }

        // 5. Process End Draw
        for (int i = 0; i < active_count; i++) {
            if (instances[i].active) {
                vm_exec(instances[i].cls->ev_end_draw, instances[i].cls->ev_end_draw_len, &instances[i]);
            }
        }

        swap_buffers();

        // Very naive frame limiting
        for(volatile int k=0; k<100000; k++) ;
    }
}
