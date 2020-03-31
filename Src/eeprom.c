#include "eeprom.h"
#include "i2c.h"

uint8_t eeprom_read_byte(uint16_t p)
{
	I2C_Start(EEPROM_ADDR | ((p & 0x700) >> 7) | I2C_WRITE);
	I2C_WriteByte(p & 0xFF);
	I2C_Restart(EEPROM_ADDR | ((p & 0x700) >> 7) | I2C_READ);
	uint8_t value = I2C_ReadByte(true);
	I2C_Stop();
	return value;
}

uint16_t eeprom_read_word(uint16_t p)
{
	I2C_Start(EEPROM_ADDR | ((p & 0x700) >> 7) | I2C_WRITE);
	I2C_WriteByte(p & 0xFF);
	I2C_Restart(EEPROM_ADDR | ((p & 0x700) >> 7) | I2C_READ);
	uint16_t value = I2C_ReadByte(false);
	value |= (((uint16_t)I2C_ReadByte(true)) << 8);
	I2C_Stop();
	return value;
}

void eeprom_write_byte(uint16_t p, uint8_t val)
{
	I2C_Start(EEPROM_ADDR | ((p & 0x700) >> 7) | I2C_WRITE);
	I2C_WriteByte(p & 0xFF);
	I2C_WriteByte(val);
	I2C_Stop();
	HAL_Delay(5);
}

void eeprom_write_word(uint16_t p, uint16_t val)
{
	I2C_Start(EEPROM_ADDR | ((p & 0x700) >> 7) | I2C_WRITE);
	I2C_WriteByte(p & 0xFF);
	I2C_WriteByte((uint8_t)val);
	I2C_WriteByte((uint8_t)(val >> 8));
	I2C_Stop();
	HAL_Delay(5);
}

void eeprom_erase_full_chip(void)
{
	for (uint16_t uPage = 0; uPage < EEPROM_PAGENUMNBER; uPage++)
	{
		uint16_t p = uPage * EEPROM_PAGESIZE;
		I2C_Start(EEPROM_ADDR | ((p & 0x700) >> 7) | I2C_WRITE);
		for (uint16_t uPos = 0; uPos < EEPROM_PAGESIZE; uPos++)
			I2C_WriteByte(0xFF);
		I2C_Stop();
		HAL_Delay(15);
	}
}
