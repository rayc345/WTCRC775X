#include "stm32f1xx_hal.h"

#ifndef __GLOBAL_H
#define __GLOBAL_H

#define DSP_I2C      0x38

// Modes
#define NUM_MODES       2
#define MODE_RF         0
#define MODE_AUX        1


// RF Modes
#define NUM_RFMODES     2
#define RFMODE_FM       0
#define RFMODE_AM       1


// Volume
#define MIN_VOL         0
#define MAX_VOL        29


// Tune types
#define NUM_TYPES       5

#define TYPE_FREQ       0
#define TYPE_SEEK       1
#define TYPE_CH         2
#define TYPE_SCAN       3
#define TYPE_ANY        4
#define TYPE_SCSV       5  // Special tune type for scan & save


// Bands
#define NUM_BANDS       5

#define BAND_LW         0
#define BAND_MW         1
#define BAND_SW         2
#define BAND_FL         3
#define BAND_FM         4


// Steps
#define NUM_STEPS       3   // Step values for each band


// Deemphasis
#define DEEMPHOFF       0
#define DEEMPH50        1
#define DEEMPH75        2


// Filters
#define NUM_FILTERS     16  // Auto + 15 fixed bandwidth filters
#define DEF_FM_FILTER    0
#define DEF_FM_FILTER2   8
#define SEEK_FM_FILTER   1
#define DEF_AM_FILTER    0
#define DEF_AM_FILTER2   6
#define SEEK_AM_FILTER   4


// Need auto sync to NV memory, bits definition
#define NEEDSYNC_BAND       0x00000001
#define NEEDSYNC_VOL        0x00000002
#define NEEDSYNC_MODE       0x00000004
#define NEEDSYNC_RFMODE     0x00000008
#define NEEDSYNC_TUNE       0x00000010
#define NEEDSYNC_STEPIDX    0x00000020
#define NEEDSYNC_FMFILTER   0x00000040
#define NEEDSYNC_AMFILTER   0x00000080
#define NEEDSYNC_MISC1      0x00000100
#define NEEDSYNC_MISC2      0x00000200
#define NEEDSYNC_MISC3      0x00000400
#define NEEDSYNC_MISC4      0x00000400
#define NEEDSYNC_MISC5      0x00000800
#define NEEDSYNC_TONE       0x00001000
#define NEEDSYNC_BALFADER   0x00002000


#define TIMER_INTERVAL      1000   // Check signal level, RSSI£¬SNR and FM stereo indicator every TIMER_INTERVAL, ms
#define TIMER_ALTDISP       3000   // Must be bigger than TIMER_INTERVAL, ms
#define TIMER_AUTOSYNC      5000   // Must be bigger than TIMER_INTERVAL, ms
//#define TIMER_RDSCHECK       200   // Check RDS signal, every TIMER_RDSCHECK, ms

#endif
