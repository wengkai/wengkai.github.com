#ifndef _FLASH_INTEL_
#define _FLASH_INTEL_

#define FLASH_BASE 0xfed00000

int flash_check_id();
int flash_erase(unsigned long add);
int flash_program(unsigned long add, unsigned long size, short* buf);
void flash_unlock(unsigned long add);
void flash_lock(unsigned long add);

#endif