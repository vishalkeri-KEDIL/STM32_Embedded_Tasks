
#define LIFE_FLASH_H

#include "main.h"

#define LIFE_PAGE_ADDR  0x080FF800UL      /* last page of Bank 2 */
#define LIFE_PAGE_NUM   255               /* page number inside Bank 2 */
#define LIFE_SLOTS      256               /* 2048 / 8 */

void     life_save(uint32_t life);              /* write the life in the next empty slot */
uint32_t life_load(uint32_t default_life);      /* read the newest good life, or default_life if blank */

