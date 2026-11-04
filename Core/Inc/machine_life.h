
#define MACHINE_LIFE_H

#include "main.h"

void machine_life_init(void);            /* call once before the while loop */
void machine_life_task(void);            /* call inside the while loop */
void machine_life_switch_event(void);    /* call from the switch interrupt callback */
