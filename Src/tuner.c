#include "tuner.h"
#include "global.h"
#include "ui.h"
#include "i2c.h"
#include "nv_memory.h"

extern volatile int8_t nLRot;
extern volatile int8_t nRRot;

extern struct M_ITEM M_FMFilter[NUM_FILTERS + 1];
extern struct M_ITEM M_AMFilter[NUM_FILTERS + 1];
extern struct M_ITEM M_Frequency[];
extern struct M_SUBMENU SM_List[];

extern TIM_HandleTypeDef htim2;

// SAF7751 write reg
uint8_t REG_MODE;
uint16_t REG_FREQ;

// SAF7751 read reg
uint8_t REG_USN;

uint8_t nStereo;	  // FM stereo, 0=off, 5=default, 9=strongest
uint8_t nFMAT;		  // FM antenna selection, 0=ANT1, 1=ANT2, 2=phase diversity
uint8_t nFMSI;		  // FM stereo improvement, 0=off, 1=on
uint8_t nFMCEQ;		  // FM channel equalizer, 0=off, 1=on
uint8_t nFirm;		  // Firmware Version,0,1,2
uint8_t nFMEMS;		  // FM enhanced multipath suppression, 0=off, 1=on
uint8_t nFMCNS;		  // FM click noise suppression, 0=off, 1=on
uint8_t nINCA;		  // FM AM improvec noise canceller, 0=off, 1=on
uint8_t nFMDynamicBW; // FM dynamic bandwidth, 0 to 3 = narrow bandwidth to wide bandwidth
uint8_t nDeemphasis;  // FM de-emphasis, 0=off, 1=50us, 2=75us
uint8_t nAGC;		  // RFAGC wideband threshold, 0 to 3 = lowest to highest start level
uint8_t nNBSens;	  // Noise blanker sensitivity,  0 to 3 = lowest to highest sensitivity
uint8_t nLowerSig;	  // Normal/reduced signal quality for seek/scan/any, 0=normal, 1=lower

int8_t nRSSI, nRSSI_Last, nRSSI_Disp; // Received signal strength, in dBuv
int8_t nSNR, nSNR_Last, nSNR_Disp;	  // Signal noise ratio, in dB
bool bSTIN;							  // FM stereo indicator

// Modes
uint8_t nMode;	 // RF/AUX
uint8_t nRFMode; // FM/AM

// Volume
uint8_t nVolume;	 // Audio volume control, 0-29
bool bMuted = false; // For mute/unmute
int8_t nBass;		 // Bass, -9 to +9
int8_t nMiddle;		 // Middle, -9 to +9
int8_t nTreble;		 // Treble, -9 to +9
int8_t nBalance;	 // Balance, -15 to +15
int8_t nFader;		 // Fader, -15 to +15

// Tune types
uint8_t nTuneType; // Freq_Step/Seek/Ch/Scan/Any/Scan_Save

// Bands
uint8_t nBand;																				  // Band: LW/MW/SW/FL/FM
const uint8_t nBandMode[NUM_BANDS] = {RFMODE_AM, RFMODE_AM, RFMODE_AM, RFMODE_FM, RFMODE_FM}; // Band RF mode
const int32_t nBandFMin[NUM_BANDS] = {153, 520, 2300, 76000, 87000};
const int32_t nBandFMax[NUM_BANDS] = {279, 1710, 26100, 86990, 108000};
const int16_t nBandStep[NUM_BANDS][NUM_STEPS] = {{9, 1, 45}, {9, 1, 90}, {5, 1, 500}, {50, 10, 500}, {100, 10, 500}};
const uint8_t nBandChs[NUM_BANDS] = {CHS_LW, CHS_MW, CHS_SW, CHS_FL, CHS_FM}; // Band total channels
const uint16_t nBandChNVMA[NUM_BANDS] = {NVMADDR_LW, NVMADDR_MW, NVMADDR_SW, NVMADDR_FL, NVMADDR_FM};
int32_t nBandFreq[NUM_BANDS]; // Band default frequency
int16_t nBandCh[NUM_BANDS];	  // Band default channel No.

// Steps
uint8_t nStepIdx; // Step index for current band

// Filters
uint8_t nFMFilter; // Current FIR filter index for FM
uint8_t nAMFilter; // Current FIR filter index for AM

int8_t nSquelch[2]; // Signal squelch value in dBuv, -99~99
					// 1st item for auto mute, 2nd item for seek/scan/any

uint8_t nBacklightAdj;	  // LCD backlight value, 0-255
uint8_t nBacklightKeep;	  // LCD backlight auto keep seconds, 0-255, 0 for always on
bool bLCDOff = false;	  // true for LCD is off
uint32_t nBacklightTimer; // LCD backlight auto keep timer in ms

uint32_t timer = 0; // RSSI, SNR & FM stereo indicator display timer
//uint32_t timerRDS = 0;                     // RDS timer
int32_t nSecondsOffset = 3600L * 24 * 15; // Seconds of real time offset, preset to 15 days

uint8_t nScanStayTime; // Seconds to stay at current frequency
uint8_t nAnyHoldTime;  // Seconds to hold current frequency after lost signal

uint32_t nAutoSyncBits;
uint16_t nAutoSyncChs;
uint16_t nAutoSyncFreqs;
uint32_t nAutoSyncTimer; // Auto sync to NV memory timer

bool bBandChangeOnly = false;
bool bDisp_USN = false;

bool bHAL_DelayedCheck;
uint32_t nHAL_DelayedTimer;
uint8_t nHAL_DelayedSmooth;

const uint8_t DSP_FIRM0_PRODUCTION[] =
	{
		1, 0xE8,
		0x81, 0x2C,
		0};

const uint8_t DSP_FIRM1_PRODUCTION[] =
	{
		//Load Firmware Version 7.1
		5, 0xE2, 0x00, 0x14, 0x00, 0x20,
		4, 0xEB, 0x00, 0x00, 0x01,
		0x80, 50,
		4, 0xEB, 0x01, 0x00, 0x01,
		0x80, 50,
		4, 0xEB, 0x02, 0x00, 0x01,
		0x80, 50,
		4, 0xEB, 0x03, 0x00, 0x01,
		0x80, 50,
		4, 0xEB, 0x04, 0x00, 0x01,
		0x80, 50,
		4, 0xEB, 0x46, 0x0F, 0x02,
		0x80, 50,
		4, 0xEB, 0x06, 0x00, 0x03,
		0x80, 50,
		1, 0xE8,
		0x80, 100,
		0};

const uint8_t DSP_FIRM2_PRODUCTION[] =
	{
		//Load Firmware Version 8.0
		5, 0xE2, 0x00, 0x14, 0x00, 0x20,
		4, 0xEB, 0x00, 0x00, 0x00,
		0x80, 50,
		4, 0xEB, 0x01, 0x00, 0x00,
		0x80, 50,
		4, 0xEB, 0x02, 0x00, 0x00,
		0x80, 50,
		4, 0xEB, 0x03, 0x00, 0x00,
		0x80, 50,
		4, 0xEB, 0x04, 0x00, 0x00,
		0x80, 50,
		4, 0xEB, 0x46, 0x0F, 0x00,
		0x80, 50,
		4, 0xEB, 0x06, 0x00, 0x01,
		0x80, 50,
		1, 0xE8,
		0x80, 100,
		0};

const uint8_t DSP_KEYCODE_PRODUCTION[] =
	{
		25, 0xE1, 0xF7, 0x7F, 0xC4, 0xE9, 0xE8, 0xBE, 0x1B, 0x94, 0x98, 0x0F, 0xE3, 0x1A, 0xDD, 0x72, 0x34, 0xB3, 0x91, 0x0A, 0x59, 0x3B, 0x80, 0xD4, 0x1D, 0xDE, // Demo Keycode for Production variants
		25, 0xE1, 0x45, 0x0F, 0x00, 0x33, 0x6B, 0xD4, 0x77, 0x36, 0x3C, 0xC3, 0xA4, 0xF3, 0xF5, 0x6D, 0x94, 0xFB, 0x1E, 0xC9, 0xCF, 0x52, 0x19, 0xEE, 0xDD, 0xB1,
		25, 0xE1, 0xE8, 0x12, 0x3D, 0xA4, 0x93, 0xA2, 0x8C, 0x24, 0xC8, 0x20, 0xEF, 0x64, 0x82, 0xE3, 0xB6, 0x9A, 0xC4, 0x23, 0x8D, 0xDB, 0x22, 0x65, 0xD8, 0xE8,
		25, 0xE1, 0x8E, 0x32, 0x75, 0x14, 0xC1, 0x45, 0x8A, 0x19, 0x2A, 0x40, 0x71, 0x50, 0x7C, 0xFF, 0x3B, 0x5F, 0xC9, 0x50, 0x16, 0xD3, 0xDE, 0xE0, 0xC3, 0x92,
		25, 0xE1, 0x83, 0xBA, 0xC9, 0x2F, 0x52, 0xB9, 0xCF, 0xE7, 0x2F, 0x10, 0x9E, 0x09, 0x58, 0x63, 0x1D, 0xCE, 0x17, 0x69, 0x91, 0x26, 0x7C, 0x60, 0xBA, 0xB1,
		25, 0xE1, 0xEB, 0x2E, 0x8E, 0x0B, 0x4F, 0x35, 0x3B, 0x28, 0x14, 0xC8, 0xE1, 0x39, 0x7D, 0x0C, 0x5A, 0x65, 0x34, 0x3B, 0x83, 0x47, 0x73, 0xDB, 0x81, 0xBA,
		25, 0xE1, 0xF7, 0x7F, 0xC4, 0xE9, 0xE8, 0xBE, 0x1B, 0x94, 0x98, 0x0F, 0xE3, 0x1A, 0xDD, 0x72, 0x34, 0xB3, 0x91, 0x0A, 0x59, 0x3B, 0x80, 0xD4, 0x1D, 0xDE,
		25, 0xE1, 0x56, 0xB8, 0x45, 0x3A, 0x28, 0xEA, 0xC0, 0xB0, 0x2E, 0xBA, 0xA8, 0x12, 0x40, 0xB3, 0xD3, 0xC0, 0x25, 0x4F, 0xD1, 0xEE, 0xFF, 0xF7, 0xCB, 0xAA,
		25, 0xE1, 0xB5, 0x29, 0x6C, 0xD9, 0x2D, 0xCF, 0xAC, 0xBB, 0xED, 0x84, 0xB4, 0xDE, 0xC8, 0x76, 0xF0, 0x39, 0xD9, 0xA2, 0x87, 0xA2, 0x61, 0x7B, 0x60, 0x4B,
		25, 0xE1, 0x34, 0xFF, 0x44, 0x46, 0x4D, 0x66, 0x33, 0x64, 0x70, 0xF3, 0x41, 0x90, 0x4B, 0x6B, 0x09, 0xDA, 0x39, 0xCB, 0x4D, 0x56, 0x9E, 0x9B, 0xF1, 0x08,
		25, 0xE1, 0xF9, 0xAC, 0xB3, 0x03, 0x75, 0x0D, 0xA1, 0x8A, 0xD8, 0x6E, 0x8C, 0x21, 0x4D, 0xF0, 0x1A, 0xC8, 0x69, 0x17, 0xF5, 0xA0, 0x1B, 0x46, 0x8C, 0x45,
		25, 0xE1, 0xEB, 0xFC, 0x0A, 0x05, 0x68, 0x9B, 0x4E, 0x9B, 0xC7, 0xDF, 0x40, 0x11, 0x6E, 0xFA, 0x01, 0x2F, 0xD9, 0x3E, 0x60, 0xEA, 0xEB, 0x13, 0x3D, 0x73,
		25, 0xE1, 0x66, 0x8D, 0x66, 0x31, 0xA6, 0x3C, 0xD8, 0x9B, 0x8C, 0x22, 0x64, 0x5A, 0x79, 0xE5, 0xFF, 0x8B, 0x1A, 0x73, 0xA7, 0xBE, 0xE3, 0x2C, 0xE6, 0x9F,
		25, 0xE1, 0x06, 0xB3, 0x3E, 0xC0, 0x00, 0xF5, 0xFE, 0xBC, 0x22, 0xF8, 0xDD, 0xD7, 0xF3, 0xAF, 0x71, 0x01, 0xB2, 0x52, 0xC0, 0x63, 0x73, 0x02, 0xE1, 0x60,
		25, 0xE1, 0x4A, 0xD7, 0x5C, 0x81, 0xF7, 0x37, 0xCD, 0x4A, 0xFA, 0x60, 0x16, 0x0E, 0x25, 0xAE, 0xFA, 0x75, 0x17, 0xAD, 0xA7, 0x69, 0xFA, 0x6A, 0x8E, 0xEF,
		25, 0xE1, 0xF0, 0x0F, 0x4A, 0x45, 0x40, 0x60, 0x8E, 0x05, 0x0D, 0xC5, 0x79, 0x70, 0x7C, 0xEB, 0xD6, 0x3D, 0xC2, 0xEC, 0x9A, 0xE1, 0x3E, 0x4F, 0x31, 0x6C,
		25, 0xE1, 0x07, 0x6A, 0xB4, 0xFE, 0xAB, 0x73, 0x97, 0xC7, 0x99, 0x2C, 0x98, 0x46, 0xBC, 0xAA, 0xCC, 0xEF, 0x26, 0x8E, 0x3A, 0x7C, 0x84, 0x5E, 0x63, 0x8E,
		25, 0xE1, 0x97, 0xB3, 0x4E, 0xBF, 0xB6, 0xA2, 0x9C, 0x71, 0x82, 0xC9, 0xBC, 0x02, 0x7D, 0xC9, 0xB4, 0x14, 0x6C, 0x6F, 0x03, 0x88, 0xBD, 0x70, 0xF0, 0x86,
		25, 0xE1, 0xA4, 0xEE, 0x3A, 0xB5, 0xF8, 0x1B, 0x34, 0x3B, 0xFF, 0x99, 0x7A, 0x2F, 0x1D, 0x09, 0x3B, 0x77, 0x31, 0x63, 0x5D, 0x05, 0x40, 0xE8, 0x7F, 0x2E,
		25, 0xE1, 0x2E, 0xAA, 0x63, 0x2F, 0xC8, 0x6C, 0x00, 0x8A, 0xC9, 0xCC, 0x1B, 0xC5, 0xBC, 0xD2, 0xD8, 0x4D, 0xB6, 0x1E, 0x01, 0xC3, 0xD2, 0x89, 0x89, 0x07,
		25, 0xE1, 0x5F, 0xBA, 0x1B, 0xA0, 0x75, 0xC3, 0x0D, 0x40, 0x83, 0x83, 0x2A, 0x71, 0xC3, 0x60, 0xB3, 0x23, 0xC6, 0x88, 0x33, 0x27, 0x74, 0xE6, 0x4A, 0xFD,
		25, 0xE1, 0x9C, 0x40, 0xCC, 0x6B, 0x8C, 0x70, 0xE4, 0xD6, 0x01, 0xBB, 0xDB, 0x00, 0x45, 0x75, 0x2C, 0x68, 0xA1, 0x39, 0x33, 0x53, 0xF5, 0x4E, 0x9D, 0x8F,
		25, 0xE1, 0x6E, 0x24, 0x4B, 0x07, 0xDD, 0xC5, 0x43, 0x36, 0x0F, 0x71, 0x9D, 0x06, 0x64, 0x69, 0x77, 0x91, 0xD6, 0x58, 0x29, 0xB4, 0x41, 0xFF, 0xD6, 0x65,
		25, 0xE1, 0x2B, 0xC3, 0xA0, 0x0D, 0xC7, 0x85, 0xA4, 0x9D, 0x99, 0x4D, 0x5C, 0x15, 0x44, 0x08, 0x16, 0x52, 0x8E, 0x6C, 0xAA, 0xF5, 0xB5, 0x09, 0x08, 0x25,
		25, 0xE1, 0x34, 0x04, 0x67, 0xE1, 0x77, 0xC5, 0x34, 0xB7, 0xCD, 0x37, 0x32, 0x4E, 0x56, 0x9F, 0x77, 0xE8, 0x5C, 0x60, 0x13, 0x34, 0x7E, 0x44, 0xD3, 0xEE,
		25, 0xE1, 0x9A, 0xBE, 0xEB, 0x91, 0xCC, 0x98, 0xBC, 0x4D, 0x89, 0x8E, 0x0F, 0x18, 0x8C, 0x3A, 0xD3, 0xD2, 0x11, 0xA7, 0x49, 0xE8, 0x69, 0x4A, 0x3C, 0x22,
		25, 0xE1, 0x46, 0x8F, 0x9F, 0x06, 0xD3, 0x9E, 0x22, 0x38, 0xA0, 0x62, 0x28, 0x45, 0x26, 0xC4, 0x80, 0x72, 0xF4, 0x22, 0xF0, 0x4B, 0x45, 0xD9, 0xA1, 0x7D,
		25, 0xE1, 0xA8, 0x90, 0xF1, 0x8B, 0x74, 0xC3, 0x97, 0xA1, 0xEB, 0x25, 0x4C, 0x56, 0xBC, 0x29, 0x46, 0x85, 0xA1, 0x52, 0x22, 0x75, 0x3E, 0xBD, 0xC6, 0xFC,
		25, 0xE1, 0x54, 0xD9, 0x0D, 0x88, 0x02, 0x9C, 0x95, 0x51, 0xAE, 0x97, 0x91, 0xA1, 0xB8, 0x66, 0x33, 0xE4, 0x87, 0x1D, 0xC7, 0xB8, 0x00, 0x1E, 0x7B, 0x30,
		25, 0xE1, 0x4B, 0x76, 0xE7, 0x85, 0x74, 0xBB, 0x2E, 0x3E, 0x14, 0xFE, 0xAF, 0xAC, 0x37, 0xF0, 0x8A, 0xA2, 0x9D, 0x90, 0x9E, 0xC3, 0xC8, 0xC9, 0xEB, 0x22,
		25, 0xE1, 0x3C, 0xE5, 0x08, 0x7F, 0x39, 0xC4, 0x54, 0x2C, 0x3D, 0xFF, 0xAF, 0xF8, 0x1D, 0xF5, 0xDB, 0x8F, 0xE9, 0xEE, 0x9F, 0x1B, 0x38, 0x42, 0x96, 0x48,
		25, 0xE1, 0x59, 0xC7, 0x0D, 0x1F, 0xD5, 0xAB, 0x27, 0xDD, 0xA1, 0x19, 0xC2, 0x93, 0x53, 0xE1, 0x9F, 0x7D, 0x23, 0xC4, 0x74, 0xC7, 0x7B, 0x7A, 0x28, 0xBA,
		25, 0xE1, 0x59, 0x8F, 0x8B, 0xFC, 0x69, 0xEB, 0x27, 0x21, 0x90, 0xAA, 0xBE, 0xE8, 0xC9, 0x8A, 0xFC, 0xD8, 0xB7, 0x24, 0xD5, 0x4D, 0x54, 0x9D, 0x69, 0x86,
		25, 0xE1, 0x15, 0x1E, 0xC8, 0x14, 0x81, 0x38, 0xD7, 0x8F, 0xF1, 0x61, 0xD2, 0xC9, 0xCD, 0x61, 0x5A, 0xC2, 0xBC, 0xE0, 0x43, 0x35, 0x4D, 0xA8, 0x03, 0xC2,
		25, 0xE1, 0xC7, 0xF7, 0xD0, 0x71, 0x03, 0x45, 0xDF, 0x35, 0xE6, 0xF8, 0x93, 0x03, 0x0B, 0x2E, 0x22, 0x5E, 0x24, 0xA7, 0x4A, 0x04, 0xD3, 0x57, 0x33, 0xCF,
		25, 0xE1, 0x23, 0xBD, 0xE0, 0xD8, 0xAF, 0x67, 0x74, 0x98, 0x4A, 0xA2, 0xC9, 0xF9, 0x5D, 0x09, 0x36, 0x91, 0xD2, 0x77, 0xA6, 0x98, 0x22, 0xF1, 0x74, 0xB8,
		0};

const uint8_t DSP_INIT[] =
	{
		3, 0xA9, 0x28, 0x01, // Audio ADCs off
		2, 0x20, 0x00,		 // Primary Audio Input:Primary Radio
		2, 0x04, 0x00,		 // Primary Radio Select Antenna 0
		2, 0x64, 0x04,		 // Secondary Radio Select Antenna 1
		//2,0xC9,0x0A,                     // Enable INCA
		4, 0x00, 0x10, 0x22, 0x74,			   // Dummy Tuning, Start Active Mode
		2, 0x3F, 0x00,						   // Audio Power Control:System Power;Sample Rate Freq:44.1kHz
		3, 0xA9, 0x32, 0x00,				   // Front DAC on
		3, 0xA9, 0x33, 0x00,				   // Rear DAC on. For portable radio w/o rear speakers, COMMENT this line to save battery power
		3, 0xA9, 0x22, 0x00,				   // SPDIF0 Outut on,Front
		3, 0xA9, 0x23, 0x00,				   // SPDIF1 Output on,Rear
		6, 0xF3, 0x03, 0x82, 0x80, 0x00, 0x00, // Switch On Sample Rate Converter 0,Primary Channel
		5, 0xF4, 0x40, 0x9B, 0x07, 0xFF,	   // Primary Channel Mute Disabled
		0};

const uint8_t DSP_FM[] =
	{
		3, 0xA9, 0x28, 0x01, // Audio ADCs off
		2, 0x20, 0x00,		 // Primary input: Primary Radio
		0};					 // DSP_FM

const uint8_t DSP_AM[] =
	{
		3, 0xA9, 0x28, 0x01, // Audio ADCs off
		2, 0x20, 0x00,		 // Primary input: Primary Radio
		0};					 // DSP_AM

const uint8_t DSP_AUX[] =
	{
		3, 0xA9, 0x02, 0x01, // AIN2(AUX) diff 1V(RMS)
		3, 0xA9, 0x03, 0x01, // AIN3(AUX) diff 1V(RMS)
		3, 0xA9, 0x28, 0x00, // Audio ADCs on
		2, 0x20, 0x09,		 // Primary input: analog input AIN23
		0};					 // DSP_AUX

// SAF7751 volume data
const uint8_t DSP_VOL[30][4] =
	{
		{0x00, 0x01, 0x00, 0x00}, // 0
		{0x00, 0x07, 0x00, 0x1A}, // 1
		{0x00, 0x17, 0x00, 0x1A}, // 2
		{0x00, 0x2E, 0x00, 0x1A}, // 3
		{0x00, 0x91, 0x00, 0x1A}, // 4
		{0x00, 0xD3, 0x00, 0x1A}, // 5
		{0x01, 0x82, 0x00, 0x1A}, // 6
		{0x02, 0x75, 0x00, 0x1A}, // 7
		{0x02, 0xD7, 0x00, 0x23}, // 8
		{0x02, 0xD7, 0x00, 0x35}, // 9
		{0x02, 0xD7, 0x00, 0x4D}, // 10
		{0x02, 0xD7, 0x00, 0x6D}, // 11
		{0x02, 0xD7, 0x00, 0x95}, // 12
		{0x02, 0xD7, 0x00, 0xC7}, // 13
		{0x02, 0xD7, 0x01, 0x02}, // 14
		{0x02, 0xD7, 0x01, 0x4E}, // 15
		{0x02, 0xD7, 0x01, 0xA5}, // 16
		{0x02, 0xD7, 0x02, 0x11}, // 17
		{0x02, 0xD7, 0x02, 0x88}, // 18
		{0x02, 0xD7, 0x03, 0x18}, // 19
		{0x02, 0xD7, 0x03, 0xC9}, // 20
		{0x02, 0xD7, 0x04, 0x80}, // 21
		{0x02, 0xD7, 0x05, 0x59}, // 22
		{0x02, 0xD7, 0x06, 0x5B}, // 23
		{0x02, 0xD7, 0x07, 0x8D}, // 24
		{0x03, 0x93, 0x07, 0xFF}, // 25
		{0x04, 0x20, 0x07, 0xFF}, // 26
		{0x05, 0x81, 0x07, 0xFF}, // 27
		{0x07, 0x21, 0x07, 0xFF}, // 28
		{0x07, 0xFF, 0x07, 0xFF}  // 29
};								  // DSP_VOL

// SAF7751 bass data
const uint8_t DSP_BASS[19][2] =
	{
		{0x0D, 0xFE}, // -18dB
		{0x0E, 0x82}, // -16dB
		{0x0E, 0xEB}, // -14dB
		{0x0F, 0x3E}, // -12dB
		{0x0F, 0x81}, // -10dB
		{0x0F, 0xB5}, // -8 dB
		{0x0F, 0xDF}, // -6 dB
		{0x00, 0x00}, // -4 dB
		{0x00, 0x21}, // -2 dB
		{0x00, 0x4B}, //  0 dB
		{0x00, 0x7F}, // +2 dB
		{0x00, 0xC2}, // +4 dB
		{0x01, 0x15}, // +6 dB
		{0x01, 0x7E}, // +8 dB
		{0x02, 0x02}, // +10dB
		{0x02, 0xA8}, // +12dB
		{0x03, 0x79}, // +14dB
		{0x04, 0x80}, // +16dB
		{0x05, 0xCB}  // +18dB
};					  // DSP_BASS

// SAF7751 middle data
const uint8_t DSP_MIDDLE[19][2] =
	{
		{0x0F, 0x9A}, // -18dB
		{0x0F, 0xA0}, // -16dB
		{0x0F, 0xA8}, // -14dB
		{0x0F, 0xB3}, // -12dB
		{0x0F, 0xC0}, // -10dB
		{0x0F, 0xD1}, // -8 dB
		{0x0F, 0xE6}, // -6 dB
		{0x00, 0x00}, // -4 dB
		{0x00, 0x21}, // -2 dB
		{0x00, 0x4B}, //  0 dB
		{0x00, 0x7F}, // +2 dB
		{0x00, 0xC2}, // +4 dB
		{0x01, 0x15}, // +6 dB
		{0x01, 0x7E}, // +8 dB
		{0x02, 0x02}, // +10dB
		{0x02, 0xA8}, // +12dB
		{0x03, 0x79}, // +14dB
		{0x04, 0x80}, // +16dB
		{0x05, 0xCB}  // +18dB
};					  // DSP_MIDDLE

// SAF7751 treble data
const uint8_t DSP_TREBLE[19][2] =
	{
		{0x0F, 0x9A}, // -18dB
		{0x0F, 0xA0}, // -16dB
		{0x0F, 0xA8}, // -14dB
		{0x0F, 0xB3}, // -12dB
		{0x0F, 0xC0}, // -10dB
		{0x0F, 0xD1}, // -8 dB
		{0x0F, 0xE6}, // -6 dB
		{0x00, 0x00}, // -4 dB
		{0x00, 0x21}, // -2 dB
		{0x00, 0x4B}, //  0 dB
		{0x00, 0x7F}, // +2 dB
		{0x00, 0xC2}, // +4 dB
		{0x01, 0x15}, // +6 dB
		{0x01, 0x7E}, // +8 dB
		{0x02, 0x02}, // +10dB
		{0x02, 0xA8}, // +12dB
		{0x03, 0x79}, // +14dB
		{0x04, 0x80}, // +16dB
		{0x05, 0xCB}  // +18dB
};					  // DSP_TREBLE

const uint8_t DSP_BAL_FADER[16][2] =
	{
		{0x07, 0xFF}, //  0
		{0x07, 0x21}, // -1
		{0x06, 0x5B}, // -2
		{0x05, 0xAA}, // -3
		{0x05, 0x0C}, // -4
		{0x04, 0x80}, // -5
		{0x04, 0x02}, // -6
		{0x03, 0x93}, // -7
		{0x03, 0x2F}, // -8
		{0x02, 0xD7}, // -9
		{0x02, 0x88}, // -10
		{0x02, 0x41}, // -11
		{0x02, 0x02}, // -12
		{0x01, 0xCA}, // -13
		{0x01, 0x99}, // -14
		{0x01, 0x6C}  // -15
};					  // DSP_BAL_FADER

// Stereo control data
const uint8_t DSP_STEREO[10][10] =
	{
		//           18h  19h  1Ah  1Bh  1Ch  1Dh  1Eh
		{8, 0x18, 0x03, 0xFF, 0x7F, 0x77, 0x70, 0xFF, 0x37, 0}, // 0(Forced mono)
		{8, 0x18, 0x03, 0xFF, 0x7F, 0x77, 0x70, 0xFF, 0x37, 0}, // 1(Min)
		{8, 0x18, 0x27, 0xBF, 0x75, 0x66, 0x60, 0xBF, 0x36, 0}, // 2
		{8, 0x18, 0x4B, 0x7F, 0x6C, 0x55, 0x40, 0x7F, 0x25, 0}, // 3
		{8, 0x18, 0x6E, 0x7F, 0x5B, 0x44, 0x20, 0x7F, 0x24, 0}, // 4
		{8, 0x18, 0x72, 0x3F, 0x4A, 0x33, 0x00, 0x3F, 0x13, 0}, // 5(Default)
		{8, 0x18, 0x95, 0x1F, 0x3A, 0x22, 0x00, 0x1F, 0x12, 0}, // 6
		{8, 0x18, 0xB9, 0x07, 0x29, 0x11, 0x00, 0x07, 0x01, 0}, // 7
		{8, 0x18, 0xDC, 0x01, 0x19, 0x00, 0x00, 0x01, 0x00, 0}, // 8
		{8, 0x18, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0}	// 9(Max)
};																// DSP_STEREO

//const unsigned char PTYASCIITable[32][9] =
//{
//	"PTY None",
//	"  News  ",
//	"Affairs ",
//	"  Info  ",
//	" Sport  ",
//	"Educate ",
//	" Drama  ",
//	"Culture ",
//	"Science ",
//	" Varied ",
//	" Pop M  ",
//	" Rock M ",
//	" Easy M ",
//	"Light M ",
//	"Classics",
//	"Other M ",
//	"Weather ",
//	"Finance ",
//	"Children",
//	" Social ",
//	"Religion",
//	"Phone In",
//	" Travel ",
//	"Leisure ",
//	"  Jazz  ",
//	"Country ",
//	"Nation M",
//	" Oldies ",
//	" Folk M ",
//	"Document",
//	"  Test  ",
//	" Alarm  "
//};
//
//uint16_t uRDSBlockA;
//union
//{
//	uint16_t uRawDataB;
//	struct
//	{
//		unsigned ADDR : 4;
//		unsigned AB : 1;
//		unsigned PTY : 5;
//		unsigned TP : 1;
//		unsigned B0 : 1;
//		unsigned GroupCode : 4;
//	} uDetailData;
//	unsigned ABOld : 1;
//} uRDSBlockB;
//uint16_t uRDSBlockC;
//uint16_t uRDSBlockD;
//uint8_t uRDSErrorCode;
//char cRadioText[65];
//uint16_t uRTSegmentAvailableBit;

////////////////////////////////////////////////////////////
void dsp_start_subaddr(uint8_t subaddr)
{
	I2C_Start(DSP_I2C | I2C_WRITE);
	I2C_WriteByte(subaddr);
}

void dsp_start_subaddr3(uint32_t subaddr)
{
	I2C_Start(DSP_I2C | I2C_WRITE);
	I2C_WriteByte((uint8_t)(subaddr >> 16));
	I2C_WriteByte((uint8_t)(subaddr >> 8));
	I2C_WriteByte((uint8_t)subaddr);
}

uint8_t dsp_query1(uint8_t subaddr)
{
	dsp_start_subaddr(subaddr);
	I2C_Restart(DSP_I2C | I2C_READ);
	uint8_t buffer = I2C_ReadByte(true);
	I2C_Stop();
	return buffer;
}

uint16_t dsp_query2(uint8_t subaddr)
{
	dsp_start_subaddr(subaddr);
	I2C_Restart(DSP_I2C | I2C_READ);
	uint16_t buffer = I2C_ReadByte(false);
	buffer = buffer << 8 | I2C_ReadByte(true);
	I2C_Stop();
	return buffer;
}

void dsp_write1(uint8_t subaddr, uint8_t data)
{
	dsp_start_subaddr(subaddr);
	I2C_WriteByte(data);
	I2C_Stop();
}

void dsp_write6(uint32_t subaddr, uint32_t data)
{
	dsp_start_subaddr3(subaddr);
	I2C_WriteByte((uint8_t)(data >> 16));
	I2C_WriteByte((uint8_t)(data >> 8));
	I2C_WriteByte((uint8_t)data);
	I2C_Stop();
}

void BootDirana3(void)
{
	HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET);
	HAL_Delay(5);
	HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET);
	HAL_Delay(20);
	//if (dsp_query1(0x14) & 0x01)
	//{
	dsp_write_data(DSP_KEYCODE_PRODUCTION);
	if (nFirm == 0)
		dsp_write_data(DSP_FIRM0_PRODUCTION);
	else if (nFirm == 1)
		dsp_write_data(DSP_FIRM1_PRODUCTION);
	else if (nFirm == 2)
		dsp_write_data(DSP_FIRM2_PRODUCTION);
	//}
	//else
	//{
	//	dsp_write_data(DSP_KEYCODE_ENGINEERING);
	//	if (nFirm == 0)
	//		dsp_write_data(DSP_FIRM0_ENGINEERING);
	//	else if (nFirm == 1)
	//		dsp_write_data(DSP_FIRM1_ENGINEERING);
	//	else if (nFirm == 2)
	//		dsp_write_data(DSP_FIRM2_ENGINEERING);
	//}
	dsp_write_data(DSP_INIT);
}

void WaitEasyReady(void)
{
	uint32_t buffer;

	do
	{
		dsp_start_subaddr3(0xf40093);
		I2C_Stop();
		I2C_Start(DSP_I2C | I2C_READ);
		buffer = (uint32_t)I2C_ReadByte(false) << 16;
		buffer |= ((uint16_t)I2C_ReadByte(false) << 8);
		buffer |= I2C_ReadByte(true);
		I2C_Stop();
	} while (buffer); // Easyprogramm routing is able to write?
}

void dsp_write_data(const uint8_t *data)
{
	uint8_t *pa = (uint8_t *)data;
	uint8_t len, i;

	for (;;)
	{
		len = *(pa++);
		if (!len)
			break;
		if (len == 0xff)
		{
			WaitEasyReady();
			continue;
		}
		if (len & 0x80) // Delay
			HAL_Delay(((uint16_t)(len & 0x7f) << 8) | *(pa++));
		else
		{
			I2C_Start(DSP_I2C | I2C_WRITE);
			for (i = 0; i < len; i++)
				I2C_WriteByte(*(pa++));
			I2C_Stop();
		}
	}
}

////////////////////////////////////////////////////////////
void SetVolume(uint8_t v30)
{
	dsp_start_subaddr3(0xf44084);
	for (uint8_t i = 0; i < 4; i++)
		I2C_WriteByte(*(&DSP_VOL[v30][i]));
	I2C_Stop();
}

void CheckVolume(void)
{
	int8_t i8;

	if ((i8 = GetLRot()) != false)
	{
		if ((i8 < 0) && (-i8 > nVolume))
			nVolume = 0;
		else
			nVolume = constrain(nVolume + i8, MIN_VOL, MAX_VOL);
		SetVolume(nVolume);
		if (nVolume)
			bMuted = false;
		AddSyncBits(NEEDSYNC_VOL);
		CheckUpdateAlt(ALT_VOL); // Show volume for a period
	}
}

void SetTone(void)
{
	int8_t nMaxTone;

	nMaxTone = nBass;
	if (nMaxTone < nMiddle)
		nMaxTone = nMiddle;
	if (nMaxTone < nTreble)
		nMaxTone = nTreble;

	// Bass
	dsp_start_subaddr3(0xf4462e);
	I2C_WriteByte(*(&DSP_BASS[nBass + 9][0]));
	I2C_WriteByte(*(&DSP_BASS[nBass + 9][1]));
	I2C_Stop();

	// Middle
	WaitEasyReady();
	dsp_start_subaddr3(0xf44622);
	I2C_WriteByte(*(&DSP_MIDDLE[nMiddle + 9][0]));
	I2C_WriteByte(*(&DSP_MIDDLE[nMiddle + 9][1]));
	I2C_Stop();

	// Treble
	WaitEasyReady();
	dsp_start_subaddr3(0xf44634);
	I2C_WriteByte(*(&DSP_TREBLE[nTreble + 9][0]));
	I2C_WriteByte(*(&DSP_TREBLE[nTreble + 9][1]));
	I2C_Stop();
}

void SetBalFader(void)
{
	int8_t l, r, f;
	// Balance
	l = r = 0;
	if (nBalance < 0)  // Left
		r = -nBalance; // Lower right
	else			   // Right
		l = nBalance;  // Lower left
	dsp_start_subaddr3(0xf44067);
	I2C_WriteByte(*(&DSP_BAL_FADER[l][0]));
	I2C_WriteByte(*(&DSP_BAL_FADER[l][1]));
	I2C_WriteByte(*(&DSP_BAL_FADER[r][0]));
	I2C_WriteByte(*(&DSP_BAL_FADER[r][1]));
	I2C_Stop();

	// Fader
	f = r = 0;
	if (nFader < 0)	 // Front
		r = -nFader; // Lower rear
	else			 // Rear
		f = nFader;	 // Lower front
	dsp_start_subaddr3(0xf44065);
	I2C_WriteByte(*(&DSP_BAL_FADER[f][0]));
	I2C_WriteByte(*(&DSP_BAL_FADER[f][1]));
	I2C_WriteByte(*(&DSP_BAL_FADER[r][0]));
	I2C_WriteByte(*(&DSP_BAL_FADER[r][1]));
	I2C_Stop();
}

void dsp_set_filter(int8_t f)
{
	dsp_write1(0x03, ((3 - nAGC) << 6) | f);
}

void NextFilter(void)
{
	if (nRFMode == RFMODE_FM)
		nFMFilter = (nFMFilter + 1) % (NUM_FILTERS + 1);
	else // RFMODE_AM
		nAMFilter = (nAMFilter + 1) % (NUM_FILTERS + 1);
}

void SetFilter(bool bNeedSync)
{
	if (nRFMode == RFMODE_FM)
	{
		dsp_set_filter(nFMFilter);
		if (bNeedSync)
			AddSyncBits(NEEDSYNC_FMFILTER);
	}
	else
	{
		dsp_set_filter(nAMFilter);
		if (bNeedSync)
			AddSyncBits(NEEDSYNC_AMFILTER);
	}
}

void SetRFCtrlReg(void)
{
	uint8_t u8;
	//static uint8_t uIPDEnabled=0;
	u8 = (3 - nAGC) << 6;
	if (nRFMode == RFMODE_FM)
		u8 |= nFMFilter;
	else
		u8 |= nAMFilter;
	dsp_write1(0x03, u8); // RFAGC wideband threshold & IF bandwidth
	if (nRFMode == RFMODE_FM)
	{
		if (nFMAT == 0)
		{
			u8 = 0x00;
			//			if(uIPDEnabled == 1)
			//			{
			//				dsp_start_subaddr(0xF5);
			//				I2C_WriteByte(0x21);
			//				I2C_WriteByte(0x23);
			//				I2C_WriteByte(0x05);
			//				I2C_WriteByte(0xD0);
			//				I2C_WriteByte(0x00);
			//				I2C_WriteByte(0x00);
			//				I2C_WriteByte(0x00);
			//				I2C_WriteByte(0x50);
			//				I2C_Stop();
			//			}
			//			uIPDEnabled = 0;
		}
		else if (nFMAT == 1)
		{
			u8 = 0x04;
			//			if(uIPDEnabled == 1)
			//			{
			//				dsp_start_subaddr(0xF5);
			//				I2C_WriteByte(0x21);
			//				I2C_WriteByte(0x23);
			//				I2C_WriteByte(0x05);
			//				I2C_WriteByte(0xD0);
			//				I2C_WriteByte(0x00);
			//				I2C_WriteByte(0x00);
			//				I2C_WriteByte(0x00);
			//				I2C_WriteByte(0x50);
			//				I2C_Stop();
			//			}
			//			uIPDEnabled = 0;
		}
		else if (nFMAT == 2)
		{
			u8 = 0x01;
			//			if(uIPDEnabled == 0)
			//			{
			//				dsp_start_subaddr(0xF5);
			//				I2C_WriteByte(0x21);
			//				I2C_WriteByte(0x23);
			//				I2C_WriteByte(0x05);
			//				I2C_WriteByte(0xD0);
			//				I2C_WriteByte(0x00);
			//				I2C_WriteByte(0x00);
			//				I2C_WriteByte(0x00);
			//				I2C_WriteByte(0x46);
			//				I2C_Stop();
			//			}
			//			uIPDEnabled = 1;
		}
		dsp_write1(0x04, u8);
	}
	//dsp_start_subaddr(0x05);
	if (nRFMode == RFMODE_FM)
	{
		// FM AM improvec noise canceller, FM enhanced multipath suppression, FM click noise suppression, FM channel equalizer, FM noise blanker sensitivity A
		dsp_write1(0x05, (nINCA << 4) | 0x02 | (nFMEMS << 7) | (nFMCNS << 6) | (nFMCEQ << 5) | (nNBSens << 2));
		dsp_write1(0x06, ((nStereo == 0) ? 0x80 : 0x00) | nDeemphasis | (nFMSI << 5)); // FM forced mono, FM de-emphasis, FM stereo improvement
		dsp_write1(0x07, ((3 - nFMDynamicBW) << 2) | (3 - nFMDynamicBW));			   // FM FM dynamic bandwidth
	}
	else																						   // AM
		dsp_write1(0x05, (nINCA << 4) | (nFMCNS << 6) | (nFMCEQ << 5) | (nNBSens << 2) | nNBSens); // FM AM improvec noise canceller, AM noise suppression, AM channel equalizer, AM noise blanker sensitivity A & B

	if (nRFMode == RFMODE_FM && !nFMSI)
		dsp_write_data(DSP_STEREO[nStereo]); // Set stereo control regs
	else
	{
		//Disable stereo blend and stereo high blend when FMSI enabled
		dsp_write1(0x19, 0x00);
		dsp_write1(0x1D, 0x00);
	}
}

void TuneReg(void)
{
	dsp_start_subaddr(0x00);
	I2C_WriteByte(REG_MODE);
	I2C_WriteByte(REG_FREQ >> 8);
	I2C_WriteByte((uint8_t)REG_FREQ);
	I2C_Stop();

	bHAL_DelayedCheck = true;
	nHAL_DelayedTimer = HAL_GetTick() + 30;
	nHAL_DelayedSmooth = 4;
}

bool TuneFreq(int32_t freq)
{
	if (freq >= 1 && freq <= 521)
	{					 // 0.001-0.521MHz. LW(extended)
		REG_MODE = 0x11; // Preset mode, LW, 1kHz step
		REG_FREQ = freq;
	}

	else if (freq >= 522 && freq <= 1710)
	{					 // 0.522-1.710MHz. MW
		REG_MODE = 0x12; // Preset mode, MW, 1kHz step
		REG_FREQ = freq;
	}

	else if (freq >= 1711 && freq <= 27400)
	{					 // 1.711-27.400MHz. SW(extended)
		REG_MODE = 0x13; // Preset mode, SW, 1kHz step
		REG_FREQ = freq;
	}

	else if (freq >= 60070 && freq <= 108500)
	{					 // 60.070-108.500MHz. FM(extended)
		REG_MODE = 0x10; // Preset mode, FM(FL/FM), 10kHz step
		REG_FREQ = (freq + 5) / 10;
	}

	else
		return false;

	TuneReg();
	if (!bBandChangeOnly)
	{
		nAutoSyncFreqs |= (1 << nBand);
		nAutoSyncTimer = HAL_GetTick();
	}
	return true;
} // bool TuneFreq(int32_t freq)

void AdjFreq(bool bCurrStep)
{
	int32_t nFreq;
	int16_t nStep;

	nFreq = nBandFreq[nBand];
	if (bCurrStep)
		nStep = nBandStep[nBand][nStepIdx];
	else
		nStep = nBandStep[nBand][0];

	if (nFreq < nBandFMin[nBand])
		nFreq = nBandFMax[nBand];
	else if (nFreq > nBandFMax[nBand])
		nFreq = nBandFMin[nBand];

	// Adjust frequency to integer multiples of step value
	nFreq = ((nFreq + (nStep >> 1)) / nStep) * nStep;

	if (nFreq < nBandFMin[nBand]) // Check min margin
		nFreq += nStep;
	else if (nFreq > nBandFMax[nBand]) // Check max margin
		nFreq -= nStep;

	nBandFreq[nBand] = nFreq;
}

void TuneFreqDisp(void)
{
	int32_t f;

	f = nBandFreq[nBand];
	TuneFreq(f);

	if (nBand >= BAND_FL)
	{
		LCDXYIntLen(FREQ_X, FREQ_Y, f / 1000, 3);
		LCDXYChar(FREQ_X + 3, FREQ_Y, '.');
		LCDXYUIntLenZP(FREQ_X + 4, FREQ_Y, f % 1000, 3);
	}
	else
	{
		LCDXYIntLen(FREQ_X, FREQ_Y, f, 6);
		LCDXYChar(FREQ_X + 6, FREQ_Y, 'K');
	}
}

uint32_t ReadChFreq(void)
{
	uint16_t nNVMData;
	int32_t nFreq;

	nNVMData = NV_read_word((nBandChNVMA[nBand] + nBandCh[nBand] * 2));
	if (nNVMData == 0xffff) // Blank ch
		return 0;
	if (nBand <= BAND_SW)
		nFreq = nBandFMin[nBand] + nNVMData;
	else
		nFreq = nBandFMin[nBand] + (int32_t)nNVMData * 5;
	nFreq = constrain(nFreq, nBandFMin[nBand], nBandFMax[nBand]);

	return nFreq;
}

void WriteChFreq(bool bAdd)
{
	uint16_t u16;

	if (bAdd)
	{ // Add ch
		if (nBand <= BAND_SW)
			u16 = (uint16_t)(nBandFreq[nBand] - nBandFMin[nBand]);
		else
			u16 = (uint16_t)((nBandFreq[nBand] - nBandFMin[nBand]) / 5);
	}
	else // Delete ch
		u16 = 0xffff;

	NV_write_word((nBandChNVMA[nBand] + nBandCh[nBand] * 2), u16);
}

void SeekCh(int8_t nDir)
{
	uint8_t u8;
	uint32_t u32;
	int8_t nDir1;

	nDir1 = nDir;
	if (!nDir1)
	{ // Try current ch 1st
		if ((u32 = ReadChFreq()) != false)
		{
			nBandFreq[nBand] = u32;
			TuneFreqDisp();
			LCDUpdate();
			return;
		}
		nDir1 = 1;
	}

	for ((u8 = 0); u8 < nBandChs[nBand]; u8++)
	{
		nBandCh[nBand] = (nBandCh[nBand] + nDir1 + nBandChs[nBand]) % nBandChs[nBand];
		if ((u32 = ReadChFreq()) != false)
		{
			nBandFreq[nBand] = u32;
			TuneFreqDisp();
			LCDUpdate();
			nAutoSyncChs |= (1 << nBand);
			nAutoSyncTimer = HAL_GetTick();
			return;
		}
	}

	// No ch find, set ch=0, f=fmin
	nBandCh[nBand] = 0;
	nBandFreq[nBand] = nBandFMin[nBand];
	TuneFreqDisp();
	LCDUpdate();
} // void SeekCh(int8_t nDir)

void ProcBand(uint8_t nBd)
{
	if (nStepIdx)
	{
		nStepIdx = 0; // Default band step
		AddSyncBits(NEEDSYNC_STEPIDX);
	}

	nBand = nBd;
	SetRFMode();

	if (nTuneType == TYPE_CH)
		SeekCh(0);
	else
	{
		bBandChangeOnly = true; // Change band only, don't sync band frequency to NV memory
		TuneFreqDisp();
		bBandChangeOnly = false;
	}
	SetRFCtrlReg(); // Set RF mode related regs

	AddSyncBits(NEEDSYNC_BAND);
}

void ProcStepFilter(uint8_t nKey)
{
	switch (nKey)
	{
	case KEY_STEP:
		nStepIdx = (nStepIdx + 1) % NUM_STEPS;
		AddSyncBits(NEEDSYNC_STEPIDX);
		break;

	case KEY_STEP | KEY_LONGPRESS: // Default step
		if (nStepIdx != 0)
			nStepIdx = 0;
		else
			nStepIdx = NUM_STEPS - 1;
		AddSyncBits(NEEDSYNC_STEPIDX);
		break;

	case KEY_FILTER:
		NextFilter();
		SetFilter(true);
		break;

	case KEY_FILTER | KEY_LONGPRESS: // Default filter
		if (nRFMode == RFMODE_FM)
		{
			if (nFMFilter != DEF_FM_FILTER)
				nFMFilter = DEF_FM_FILTER;
			else
				nFMFilter = DEF_FM_FILTER2;
		}
		else
		{
			if (nAMFilter != DEF_AM_FILTER)
				nAMFilter = DEF_AM_FILTER;
			else
				nAMFilter = DEF_AM_FILTER2;
		}
		SetFilter(true);
		break;
	}
} // void ProcStepFilter(uint8_t nKey)

void GetRFStatReg(void)
{
	dsp_start_subaddr(0x00);
	I2C_Restart(DSP_I2C | I2C_READ);
	bSTIN = (I2C_ReadByte(false) >> 3) & 1;
	nRSSI = (int8_t)(I2C_ReadByte(false) >> 1) - 8;
	REG_USN = I2C_ReadByte(true);
	I2C_Stop();

	if (nRFMode == RFMODE_FM)														 // In FM mode, SNR is NOT calibrated accurately. For reference only
		nSNR = (int)(0.46222375 * (float)nRSSI - 0.082495118 * (float)REG_USN) + 10; // Emprical formula
	else																			 // AM
		nSNR = -((int8_t)REG_USN);
}

bool IsSigOK(void)
{
	int8_t nLow;

	nLow = nLowerSig * 10;

	if (nRFMode == RFMODE_FM)
	{
		if (nRSSI >= (nSquelch[1] + 10 - nLow) && nSNR >= (10 - nLow) && REG_USN <= (50 + nLow))
			return true;
		HAL_Delay(5);
		GetRFStatReg();
		if (nRSSI >= (nSquelch[1] - nLow) && nSNR >= (5 - nLow) && REG_USN <= (90 + nLow))
			return true;
	}
	else
	{ // AM
		if (nRSSI >= (nSquelch[1] + 15 - nLow) && nSNR >= (15 - nLow))
			return true;
		HAL_Delay(5);
		GetRFStatReg();
		if (nRSSI >= (nSquelch[1] + 5 - nLow) && nSNR >= (10 - nLow))
			return true;
	}

	return false;
}

int8_t Seek(int8_t nDir)
{
	int8_t i;
	uint8_t nKey;

	if (nMode != MODE_AUX)
		SetVolume(0); // Mute

	if (nRFMode == RFMODE_FM)
	{
		if (!nFMFilter || nFMFilter > SEEK_FM_FILTER)
			dsp_set_filter(SEEK_FM_FILTER);
	}
	else
	{
		if (!nAMFilter || nAMFilter > SEEK_AM_FILTER)
			dsp_set_filter(SEEK_AM_FILTER);
	}

	for (;;)
	{
		nBandFreq[nBand] += nDir * nBandStep[nBand][nStepIdx];
		AdjFreq(true);
		TuneFreqDisp();

		for (i = 0; i < 6; i++)
		{
			if ((nKey = GetKey()) != false)
			{
				if (nKey & (KEY_STEP | KEY_FILTER))
				{
					ProcStepFilter(nKey);
					LCDUpdate();
				}
				else
				{
					SetFilter(false);
					if (nMode != MODE_AUX)
						SetVolume(nVolume); // Unmute
					return nKey;			// Other key pressed, abort seek
				}
			}

			HAL_Delay(5);
			GetRFStatReg();
			CheckUpdateSig();
			if (IsSigOK())
			{ // Find one signal
				SetFilter(false);
				SetVolume(nVolume); // Unmute
				return 0;
			}
		}

		CheckVolume();
		CheckUpdateAlt(ALT_AUTO);
	}
} // int8_t Seek(int8_t nDir)

uint8_t ScanAny()
{
	uint32_t t1, nTimeout;
	uint8_t nKey, lp;

	if (nTuneType == TYPE_SCAN)
		nTimeout = nScanStayTime * 1000;
	else
		nTimeout = nAnyHoldTime * 1000;
	LCDUpdate();
	NV_write_byte(NVMADDR_TUNE, nTuneType);

	for (;;)
	{
		LCDOn();

		if ((nKey = Seek(+1)) != false) // Key(Other than STEP & FILTER) pressed, user abort
			return nKey;

		// Else we find one signal
		t1 = HAL_GetTick();
		for (lp = 0;; lp++)
		{
			if (!(lp % 16))
				CheckUpdateSig();
			if (nTuneType == TYPE_ANY && IsSigOK())
				t1 = HAL_GetTick();

			if ((HAL_GetTick() - t1) > nTimeout) // Lost signal for nTimeout ms in ANY mode
				break;							 // or nTimeout ms passed in SCAN mode

			if ((nKey = GetKey()) != false)
			{
				if (nKey & (KEY_STEP | KEY_FILTER))
				{
					ProcStepFilter(nKey);
					LCDUpdate();
				}
				else
					return nKey; // Key(Other than STEP/FILTER) pressed, user abort
			}

			if (GetRRot()) // Right rotary encoder turned, leave this signal
				break;

			if ((nTuneType == TYPE_ANY) && nBacklightKeep && !bLCDOff)
			{
				if ((HAL_GetTick() - nBacklightTimer) >= (uint32_t)nBacklightKeep * 1000)
				{
					LCDSetBackLight(0);
					bLCDOff = true;
				}
			}

			CheckVolume(); // Check volume adjustment
			CheckUpdateAlt(ALT_AUTO);
			HAL_Delay(64);
		}
	}
} // uint8_t ScanAny()

void SetRFMode(void)
{
	if (nBandMode[nBand] == RFMODE_FM)
	{
		nRFMode = RFMODE_FM;
		SM_List[MID_FILT].pMItem = M_FMFilter;
		SM_List[MID_FILT].nItemCount = sizeof(M_FMFilter) / sizeof(struct M_ITEM);
	}
	else
	{
		nRFMode = RFMODE_AM;
		SM_List[MID_FILT].pMItem = M_AMFilter;
		SM_List[MID_FILT].nItemCount = sizeof(M_AMFilter) / sizeof(struct M_ITEM);
	}

	AddSyncBits(NEEDSYNC_RFMODE);
}

void SetMode_RF(void)
{
	nMode = MODE_RF;
	SetRFMode();

	if (nRFMode == RFMODE_FM)
		dsp_write_data(DSP_FM);
	else
		dsp_write_data(DSP_AM);

	AddSyncBits(NEEDSYNC_MODE);
}

void SetMode_AUX(void)
{
	nMode = MODE_AUX;
	dsp_write_data(DSP_AUX);
	AddSyncBits(NEEDSYNC_MODE);
}

void AddSyncBits(uint32_t SyncBit)
{
	nAutoSyncBits |= SyncBit;
	nAutoSyncTimer = HAL_GetTick();
}

void TunerInit(void)
{
	uint8_t nBootMode;
	HAL_TIM_Base_Start_IT(&htim2);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	HAL_Delay(35);
	LCDInit();
	LCDXYStr(3, 0, "WTCRC7751");
	LCDXYStr(3, 1, "V3 Build 3");
	HAL_Delay(800);
	NVMGetArgs();
	if (!IsMenuVisible(MID_INCA)) //��INCA֧�ֵ��ͺ��Ͻ���INCA������R7.1�̼�������������
		nINCA = 0;
	BootDirana3();
	LCDClr();
	nLRot = 0;
	nRRot = 0;
	nBootMode = nMode;
	SetMode_RF(); // Set to RF mode
	if (nTuneType == TYPE_CH)
		SeekCh(0); // Tune to current ch
	else
		TuneFreqDisp(); // Tune to default frequency and display
	SetRFCtrlReg();		// Set RF mode related regs
	if (nBootMode == MODE_AUX)
		SetMode_AUX();
	SetVolume(nVolume); // Set audio volume
	SetTone();			// Set bass, middle & treble
	SetBalFader();		// Set balance & fader
	HAL_Delay(50);
	//memset(cRadioText, 0, sizeof(cRadioText));
	LCDSetBackLight(nBacklightAdj);
	nBacklightTimer = HAL_GetTick();
	LCDUpdate();

	nAutoSyncBits = 0;
	nAutoSyncChs = 0;
	nAutoSyncFreqs = 0;
	nAutoSyncTimer = 1000L * 3600 * 24; // Auto sync to NV memory timer
} // void TunerInit(void)

void TunerLoop(void)
{
	int8_t i8;
	uint8_t nKey;
	uint16_t u16;

	//if (HAL_GetTick() - timerRDS >= TIMER_RDSCHECK)
	//{
	//	timerRDS = HAL_GetTick();
	//	for (uint8_t i = 0; i < 10; i++)
	//	{
	//		if (dsp_query1(0x00) & 0x10)
	//		{
	//			//RDAV == 1
	//			//printf("RDS: %4X,%4X,%4X,%4X Error: %X\n", dsp_query2(0x08), dsp_query2(0x0A), dsp_query2(0x0C), dsp_query2(0x0E), dsp_query1(0x10));
	//			uRDSErrorCode = dsp_query1(0x10);
	//			uRDSBlockD = dsp_query2(0x0E);
	//			uRDSBlockC = dsp_query2(0x0C);
	//			uRDSBlockB.uRawDataB = dsp_query2(0x0A);
	//			uRDSBlockA = dsp_query2(0x08);
	//			if (uRDSBlockB.uDetailData.B0)
	//			{
	//				//B�汾��Block1��PI�����Block1��Block3
	//			}
	//			else
	//			{
	//				//A�汾��Block1��PI��ֻ���뵽Block1
	//				//printf("�㲥���ͣ� %s \n", PTYASCIITable[uRDSBlockB.uDetailData.PTY]);
	//				if (uRDSErrorCode == 0 && uRDSBlockB.uDetailData.GroupCode == 2)
	//				{
	//					//�㲥�ı�
	//					//if(ABOld != uRDSBlockB.uDetailData.AB)
	//					uRTSegmentAvailableBit |= (0x0001 << uRDSBlockB.uDetailData.ADDR);
	//					cRadioText[uRDSBlockB.uDetailData.ADDR * 4] = (unsigned char)(uRDSBlockC >> 8);
	//					cRadioText[uRDSBlockB.uDetailData.ADDR * 4 + 1] = (unsigned char)(uRDSBlockC & 0x00FF);
	//					cRadioText[uRDSBlockB.uDetailData.ADDR * 4 + 2] = (unsigned char)(uRDSBlockD >> 8);
	//					cRadioText[uRDSBlockB.uDetailData.ADDR * 4 + 3] = (unsigned char)(uRDSBlockD & 0x00FF);
	//					//if (uRTSegmentAvailableBit == 0xFFFF) // get all 16 segments
	//					{
	//						cRadioText[64] = '\0';
	//						printf("�㲥�ı��� %s \n", cRadioText);
	//					}
	//					//char ss[5] = { 0 };
	//					//ss[0] = uRDSBlockC >> 8;
	//					//ss[1] = uRDSBlockC & 0x0F;
	//					//ss[2] = uRDSBlockD >> 8;
	//					//ss[3] = uRDSBlockD & 0x0F;
	//					//printf("�㲥�ı��� %s \n", ss);
	//				}
	//			}
	//			//printf("RDSB: %4X,%X,%X,%X,%X,%X,%X Error: %X\n", uRDSBlockB.uRawDataB
	//			//	, uRDSBlockB.uDetailData.GroupCode
	//			//	, uRDSBlockB.uDetailData.B0
	//			//	, uRDSBlockB.uDetailData.TP
	//			//	, uRDSBlockB.uDetailData.PTY
	//			//	, uRDSBlockB.uDetailData.AB
	//			//	, uRDSBlockB.uDetailData.ADDR
	//			//	, uRDSErrorCode);
	//		}
	//	}
	//}

	if (bHAL_DelayedCheck && HAL_GetTick() >= nHAL_DelayedTimer)
	{
		bHAL_DelayedCheck = false;
		CheckUpdateSig(); // Update RSSI, SNR & FM stereo indicator
		timer = HAL_GetTick() - (TIMER_INTERVAL >> 2);
	}
	else if ((HAL_GetTick() - timer) >= TIMER_INTERVAL)
	{					  // Check every TIMER_INTERVAL ms
		CheckUpdateSig(); // Update RSSI, SNR & FM stereo indicator
		timer = HAL_GetTick();

		if (!bMuted && (nMode != MODE_AUX))
		{
			if ((nRSSI_Disp < nSquelch[0]))
				SetVolume(0);
			else
				SetVolume(nVolume);
		}

		CheckUpdateAlt(ALT_AUTO);

		if (nBacklightKeep && !bLCDOff)
		{
			if ((timer - nBacklightTimer) >= (uint32_t)nBacklightKeep * 1000)
			{
				LCDSetBackLight(0);
				bLCDOff = true;
			}
		}

		if ((timer - nAutoSyncTimer) >= TIMER_AUTOSYNC)
		{
			if (nAutoSyncBits)
			{
				if (nAutoSyncBits & NEEDSYNC_BAND)
					NV_write_byte(NVMADDR_BAND, nBand);

				if (nAutoSyncBits & NEEDSYNC_VOL)
					NV_write_byte(NVMADDR_VOL, nVolume);

				if (nAutoSyncBits & NEEDSYNC_MODE)
					NV_write_byte(NVMADDR_MODE, nMode);

				if (nAutoSyncBits & NEEDSYNC_RFMODE)
					NV_write_byte(NVMADDR_RFMODE, nRFMode);

				if (nAutoSyncBits & NEEDSYNC_TUNE)
					NV_write_byte(NVMADDR_TUNE, nTuneType);

				if (nAutoSyncBits & NEEDSYNC_STEPIDX)
					NV_write_byte(NVMADDR_STEPIDX, nStepIdx);

				if (nAutoSyncBits & NEEDSYNC_FMFILTER)
					NV_write_byte(NVMADDR_FMFILTER, nFMFilter);

				if (nAutoSyncBits & NEEDSYNC_AMFILTER)
					NV_write_byte(NVMADDR_AMFILTER, nAMFilter);

				if (nAutoSyncBits & NEEDSYNC_MISC1)
					NV_write_byte(NVMADDR_MISC1, (nDeemphasis << 6) | (nLowerSig << 4) | (nFMDynamicBW << 2) | nAGC);

				if (nAutoSyncBits & NEEDSYNC_MISC2)
					NV_write_byte(NVMADDR_MISC2, nNBSens | (nFMSI << 4) | (nFMAT << 2) | (nFMCEQ << 5) | (nFMCNS << 6) | (nFMEMS << 7) | nNBSens);

				if (nAutoSyncBits & NEEDSYNC_MISC3)
					NV_write_byte(NVMADDR_MISC3, nStereo | (nINCA << 4) | (nFirm << 4));

				if (nAutoSyncBits & NEEDSYNC_TONE)
				{
					NV_write_byte(NVMADDR_BASS, nBass);
					NV_write_byte(NVMADDR_MIDDLE, nMiddle);
					NV_write_byte(NVMADDR_TREBLE, nTreble);
				}

				if (nAutoSyncBits & NEEDSYNC_BALFADER)
				{
					NV_write_byte(NVMADDR_BALANCE, nBalance);
					NV_write_byte(NVMADDR_FADER, nFader);
				}

				nAutoSyncBits = 0;
			}

			if (nAutoSyncChs)
			{
				for (i8 = 0; i8 < NUM_BANDS; i8++)
				{
					if (nAutoSyncChs & (1 << i8))
						NV_write_byte(NVMADDR_BANDCH + i8, nBandCh[i8]);
				}
				nAutoSyncChs = 0;
			}

			if (nAutoSyncFreqs)
			{
				for (i8 = 0; i8 < NUM_BANDS; i8++)
				{
					if (nAutoSyncFreqs & (1 << i8))
					{
						if (i8 <= BAND_SW)
							u16 = (uint16_t)(nBandFreq[i8] - nBandFMin[i8]);
						else
							u16 = (uint16_t)((nBandFreq[i8] - nBandFMin[i8]) / 5);
						NV_write_word(NVMADDR_BANDFREQ + i8, u16);
					}
				}
				nAutoSyncFreqs = 0;
			}

			nAutoSyncTimer = timer + 1000L * 3600 * 24;
		}
	}

	CheckVolume();

	if ((i8 = GetRRot()) != false)
	{
		switch (nTuneType)
		{
		case TYPE_FREQ:
			nBandFreq[nBand] += i8 * nBandStep[nBand][nStepIdx];
			AdjFreq(true);
			TuneFreqDisp();
			break;

		case TYPE_SEEK:
			Seek(constrain(i8, -1, 1));
			break;

		case TYPE_CH:
			if (i8 < 0)
				SeekCh(-1);
			else
				SeekCh(1);
			break;
		}
	}

	if ((nKey = GetKey()) != false)
	{
		switch (nKey)
		{
		case KEY_LROT:
			Menu(MID_OPTION);
			break;

		case KEY_LROT | KEY_LONGPRESS: // Toggle mute/unmute
			if (bMuted)
				SetVolume(nVolume); // Unmute
			else
				SetVolume(0); // Mute
			bMuted = !bMuted;
			break;

		case KEY_RROT:
			Menu(MID_FREQUENCY);
			break;

		case KEY_RROT | KEY_LONGPRESS:
			AddDelCh();
			LCDUpdate();
			break;

		case KEY_TUNE:
			nTuneType = (nTuneType + 1) % NUM_TYPES;
			if (nTuneType == TYPE_CH)
				SeekCh(0);
			AddSyncBits(NEEDSYNC_TUNE);
			break;

		case KEY_TUNE | KEY_LONGPRESS:
			if (nTuneType != TYPE_FREQ)
				nTuneType = TYPE_FREQ;
			else
			{
				nTuneType = TYPE_CH;
				SeekCh(0);
			}
			AddSyncBits(NEEDSYNC_TUNE);
			break;

		case KEY_BAND:
			ProcBand((nBand + 1) % NUM_BANDS);
			break;

		case KEY_BAND | KEY_LONGPRESS:
			if (nBand != BAND_FM)
				ProcBand(BAND_FM);
			else
				ProcBand(BAND_MW);
			break;

		default: // Include KEY_STEP, KEY_STEP | KEY_LONGPRESS, KEY_FILTER, KEY_FILTER | KEY_LONGPRESS
			ProcStepFilter(nKey);
			break;
		}
		LCDUpdate();
	}

	if (nTuneType == TYPE_SCAN)
	{
		if (ScanAny() == KEY_TUNE)
			nTuneType = TYPE_ANY;
		else
			nTuneType = TYPE_FREQ;
		LCDUpdate();
		AddSyncBits(NEEDSYNC_TUNE);
	}

	if (nTuneType == TYPE_ANY)
	{
		ScanAny();
		nTuneType = TYPE_FREQ;
		LCDUpdate();
		AddSyncBits(NEEDSYNC_TUNE);
	}
} // void TunerLoop(void)
