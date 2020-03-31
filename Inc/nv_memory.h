#include "stm32f1xx_hal.h"
#include "eeprom.h"
#include "global.h"

#ifndef __NVMEM_H
#define __NVMEM_H

#define NVMSIG0     0xF7
#define NVMSIG1     0x5F
#define NVMSIG      ((uint16_t)NVMSIG0 + ((uint16_t)NVMSIG1 << 8))

#define NVMSIGSTATION0     0xCC
#define NVMSIGSTATION1     0xB5
#define NVMSIGSTATION     ((uint16_t)NVMSIGSTATION0 + ((uint16_t)NVMSIGSTATION1 << 8))

// Channels in band
#define CHS_LW       20
#define CHS_MW       50
#define CHS_SW      100
#define CHS_FL       50
#define CHS_FM      200

// NV memory addr
#define NVMADDR_SIG          0
#define NVMADDR_SIGSTATION   2
#define NVMADDR_BANDCH       4
#define NVMADDR_BANDFREQ     9
#define NVMADDR_BAND        19
#define NVMADDR_VOL         20
#define NVMADDR_MODE        21
#define NVMADDR_RFMODE      22
#define NVMADDR_TUNE        23
#define NVMADDR_STEPIDX     24
#define NVMADDR_FMFILTER    25
#define NVMADDR_AMFILTER    26
#define NVMADDR_SQU1        27
#define NVMADDR_SQU2        28
#define NVMADDR_SCSTAY      29
#define NVMADDR_ANYHOLD     30
#define NVMADDR_BKADJ       31
#define NVMADDR_BKKEEP      32
#define NVMADDR_MISC1       33
#define NVMADDR_MISC2       34
#define NVMADDR_MISC3       35
#define NVMADDR_MISC4       36
#define NVMADDR_MISC5       37
#define NVMADDR_BASS        38
#define NVMADDR_MIDDLE      39
#define NVMADDR_TREBLE      40
#define NVMADDR_BALANCE     41
#define NVMADDR_FADER       42

#define NVMADDR_LW          100
#define NVMADDR_MW          140
#define NVMADDR_SW          240
#define NVMADDR_FL          440
#define NVMADDR_FM          540


void NVMInitStation(void);
void NVMInitSetting(void);
void NVMGetArgs(void);
void NVMUnpkWrData(const uint8_t* pPGMAddr);

int constrain(int val, int min, int max);

#define NV_read_byte(p)         eeprom_read_byte((p))
#define NV_read_word(p)         eeprom_read_word((p))
#define NV_write_byte(p, val)   eeprom_write_byte((p), (val))
#define NV_write_word(p, val)   eeprom_write_word((p), (val))

#endif
