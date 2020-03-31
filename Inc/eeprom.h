#ifndef _EEPROM_H
#define _EEPROM_H

#include "stm32f1xx_hal.h"

#define EEPROM_ADDR 0xA0
#define EEPROM_PAGESIZE 16
#define EEPROM_PAGENUMNBER 128

uint8_t eeprom_read_byte(uint16_t p);
uint16_t eeprom_read_word(uint16_t p);
void eeprom_write_byte(uint16_t p, uint8_t val);
void eeprom_write_word(uint16_t p, uint16_t val);
void eeprom_erase_full_chip(void);

#endif /* _EEPROM_H */
