#include "flash_memory.h"

#define EMPTY_SLOT  0xFFFFFFFFFFFFFFFFULL

static uint32_t next_slot = 0;                  /* next empty slot */


// storing the value


void life_save(uint32_t life)
{


  uint64_t record = ((uint64_t)(~life) << 32) | life;     /* high = flipped copy, low = life */


  HAL_FLASH_Unlock();
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);


  if (next_slot >= LIFE_SLOTS)                            /* page full, erase it */
  {
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0;
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Banks     = FLASH_BANK_2;
    erase.Page      = LIFE_PAGE_NUM;
    erase.NbPages   = 1;
    HAL_FLASHEx_Erase(&erase, &page_error);
    next_slot = 0;
  }


  HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, LIFE_PAGE_ADDR + next_slot * 8, record);
  HAL_FLASH_Lock();
  next_slot++;
}



// this is for getting the life value from the flash memory

uint32_t life_load(uint32_t default_life)
{
  uint32_t life = default_life;                           /* blank page = first time */
  uint32_t i;

  for (i = 0; i < LIFE_SLOTS; i++)
  {
    uint64_t slot = *(volatile uint64_t *)(LIFE_PAGE_ADDR + i * 8);   // moving one after another slot

    if (slot == EMPTY_SLOT) { break; }                    /* empty slot = no more records */

    uint32_t low  = (uint32_t)slot;                       /* the life */
    uint32_t high = (uint32_t)(slot >> 32);               /* the flipped copy */

    if (high == ~low) { life = low; }                     /* good record, keep the newest */
  }

  next_slot = i;                                          /* the next save goes here */
  return life;
}
