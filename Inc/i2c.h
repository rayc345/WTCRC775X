#ifndef _I2C_H
#define _I2C_H

#include "stm32f1xx_hal.h"
#include "main.h"
#include <stdbool.h>

#define I2C_SCL_LOW               (SCL_GPIO_Port->BRR  = SCL_Pin)
#define I2C_SCL_HIGH              (SCL_GPIO_Port->BSRR = SCL_Pin)
#define I2C_SDA_LOW               (SDA_GPIO_Port->BRR  = SDA_Pin)
#define I2C_SDA_HIGH              (SDA_GPIO_Port->BSRR = SDA_Pin)

#define I2C_SCL_READ              (SDA_GPIO_Port->IDR & SCL_Pin)
#define I2C_SDA_READ              (SDA_GPIO_Port->IDR & SDA_Pin)

#define I2C_WRITE 0x00
#define I2C_READ 0x01

#define I2C_ACK                   0                        //Ó¦´ð
#define I2C_NOACK                 1                        //·ÇÓ¦´ð

bool I2C_Start(uint8_t addr);
bool I2C_Restart(uint8_t addr);
void I2C_Stop(void);

bool I2C_WriteByte(uint8_t Data);
uint8_t I2C_ReadByte(bool last);

#endif /* _I2C_H */
