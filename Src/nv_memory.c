#include "nv_memory.h"
#include "global.h"
#include "ui.h"

extern uint8_t nMode;           // RF/AUX
extern uint8_t nRFMode;         // FM/AM

extern uint8_t nVolume;         // Audio volume control, 0-29
extern int8_t nBass;            // Bass, -9 to +9
extern int8_t nMiddle;          // Middle, -9 to +9
extern int8_t nTreble;          // Treble, -9 to +9
extern int8_t nBalance;         // Balance, -15 to +15
extern int8_t nFader;           // Fader, -15 to +15
extern int8_t nSquelch[];
extern uint8_t nBacklightAdj;   // LCD backlight value, 0-255
extern uint8_t nBacklightKeep;  // LCD backlight auto keep seconds, 0-255, 0 for always on
extern uint8_t nScanStayTime;   // Seconds to stay at current frequency
extern uint8_t nAnyHoldTime;    // Seconds to hold current frequency after lost signal

extern uint8_t nStereo;         // FM stereo, 0=off, 5=default, 9=strongest
extern uint8_t nFMAT;           // FM antenna selection, 0=ANT1, 1=ANT2, 2=phase diversity
extern uint8_t nFMSI;           // FM stereo improvement, 0=off, 1=on
extern uint8_t nFMCEQ;          // FM channel equalizer, 0=off, 1=on
extern uint8_t nFirm;           // Firmware Version,0,1,2
extern uint8_t nFMEMS;          // FM enhanced multipath suppression, 0=off, 1=on
extern uint8_t nFMCNS;          // FM click noise suppression, 0=off, 1=on
extern uint8_t nINCA;           // FM AM improvec noise canceller, 0=off, 1=on
extern uint8_t nFMDynamicBW;    // FM dynamic bandwidth, 0 to 3 = narrow bandwidth to wide bandwidth
extern uint8_t nDeemphasis;     // FM de-emphasis, 0=off, 1=50us, 2=75us
extern uint8_t nAGC;            // RFAGC wideband threshold, 0 to 3 = lowest to highest start level
extern uint8_t nNBSens;         // Noise blanker sensitivity,  0 to 3 = lowest to highest sensitivity
extern uint8_t nLowerSig;       // Normal/reduced signal quality for seek/scan/any, 0=normal, 1=lower

extern uint8_t nTuneType;       // Freq_Step/Seek/Ch/Scan/Any/Scan_Save

extern uint8_t nBand;                       // Band: LW/MW/SW/FL/FM
extern const int32_t nBandFMin[NUM_BANDS];
extern const int32_t nBandFMax[NUM_BANDS];
extern const uint8_t nBandChs[NUM_BANDS];   // Band total channels
extern int32_t nBandFreq[NUM_BANDS];        // Band default frequency
extern int16_t nBandCh[NUM_BANDS];

extern uint8_t nStepIdx;   // Step index for current band

extern uint8_t nFMFilter;  // Current FIR filter index for FM
extern uint8_t nAMFilter;  // Current FIR filter index for AM


// NV memory initialization data
const uint8_t NVM_INIT[] =
{
	2, NVMSIG0, NVMSIG1,	
	2, NVMSIGSTATION0,NVMSIGSTATION1,
	0x80 + 5,      0,   // Band ch No.
	0x80 + 10,  0xff,   // Band frequency
	8, 4, 10, 0, 0, 0, 0, 0, 0,  // Band, Vol, Mode, RFMode, TuneType, StepIdx, FMFilter, AMFilter
	6, 0xE2, 30, 5, 3, 255, 0,  // Squ1, Squ2, TScan, TAny, BkAdj, BkKeep,
	5, 0x49, 0xF2, 5, 0, 0,  // Misc1, Misc2, Misc3, Misc4, Misc5
	5, 0, 0, 0, 0, 0,  // Bass, Middle, Treble, Balance, Fader
	0  // End
};

void NVMInitStation(void)
{
	printf("NVMInitStations: %4X",eeprom_read_word(NVMADDR_SIGSTATION));
	LCDXYStr(0, 1, "Init Station ...");
	eeprom_erase_full_chip();
	NV_write_word(NVMADDR_SIGSTATION, NVMSIGSTATION);
}

void NVMInitSetting(void)
{
	printf("NVMInitSettings: %4X",eeprom_read_word(NVMADDR_SIG));
	LCDXYStr(0, 1, "Init Setting ...");
	NVMUnpkWrData(NVM_INIT);  // Initialize NV memory
}

void NVMGetArgs(void)
{
	uint8_t u8, i;
	uint16_t u16;

	if (eeprom_read_word(NVMADDR_SIGSTATION) != NVMSIGSTATION)
		NVMInitStation();

	if (eeprom_read_word(NVMADDR_SIG) != NVMSIG)
		NVMInitSetting();

	nBand = NV_read_byte(NVMADDR_BAND);
	if (nBand >= NUM_BANDS)
		nBand = BAND_FM;

	nVolume = NV_read_byte(NVMADDR_VOL);
	if (nVolume > MAX_VOL)
		nVolume = 10;

	nBass = NV_read_byte(NVMADDR_BASS);
	nBass = constrain(nBass, -9, 9);
	nMiddle = NV_read_byte(NVMADDR_MIDDLE);
	nMiddle = constrain(nMiddle, -9, 9);
	nTreble = NV_read_byte(NVMADDR_TREBLE);
	nTreble = constrain(nTreble, -9, 9);
	nBalance = NV_read_byte(NVMADDR_BALANCE);
	nBalance = constrain(nBalance, -15, 15);
	nFader = NV_read_byte(NVMADDR_FADER);
	nFader = constrain(nFader, -15, 15);

	nMode = NV_read_byte(NVMADDR_MODE);
	if (nMode >= NUM_MODES)
		nMode = MODE_RF;

	nRFMode = NV_read_byte(NVMADDR_RFMODE);
	if (nRFMode >= NUM_RFMODES)
		nRFMode = RFMODE_FM;

	nTuneType = NV_read_byte(NVMADDR_TUNE);
	if (nTuneType >= NUM_TYPES)
		nTuneType = TYPE_FREQ;

	nStepIdx = NV_read_byte(NVMADDR_STEPIDX);
	if (nStepIdx >= NUM_STEPS)
		nStepIdx = 0;

	nFMFilter = NV_read_byte(NVMADDR_FMFILTER);
	if (nFMFilter > NUM_FILTERS)
		nFMFilter = DEF_FM_FILTER;

	nAMFilter = NV_read_byte(NVMADDR_AMFILTER);
	if (nAMFilter > NUM_FILTERS)
		nAMFilter = DEF_AM_FILTER;

	u8 = NV_read_byte(NVMADDR_MISC1);
	nDeemphasis = u8 >> 6;            // FM de-emphasis, 0=off, 1=50us, 2=75us
	if (nDeemphasis > 2)
		nDeemphasis = DEEMPH50;
	nLowerSig = (u8 >> 4) & 1;        // Reduce signal quality for seek/scan/any, 0=normal, 1=lower
	nFMDynamicBW = (u8 >> 2) & 0x03;  // FM dynamic bandwidth, 0 to 3 = narrow bandwidth to wide bandwidth
	nAGC = u8 & 0x03;                 // RFAGC wideband threshold, 0 to 3 = lowest to highest start level

	u8 = NV_read_byte(NVMADDR_MISC2);
	nFMEMS = (u8 >> 7) & 1;           // FM enhanced multipath suppression, 0=off, 1=on
	nFMCNS = (u8 >> 6) & 1;           // FM click noise suppression, 0=off, 1=on
	nFMCEQ = (u8 >> 5) & 1;           // FM channel equalizer, 0=off, 1=on
	nFMSI = (u8 >> 4) & 1;            // FM stereo improvement, 0=off, 1=on
	nFMAT = (u8 >> 2) & 0x03;         // FM antenna selection, 0=ANT1, 1=ANT2, 2=phase diversity
	if (nFMAT > 2)
		nFMAT = 0;
	nNBSens = u8 & 0x03;              // Noise blanker sensitivity,  0 to 3 = lowest to highest sensitivity

	u8 = NV_read_byte(NVMADDR_MISC3); // FM stereo, 0=off, 5=default, 9=strongest
	nStereo = u8 & 0x0F;
	if (nStereo > 9)
		nStereo = 5;
	nINCA = (u8 >> 4) & 1;            // FM AM improvec noise canceller, 0=off, 1=on
	nFirm = (u8 >> 5) & 0x03;

	nSquelch[0] = NV_read_byte(NVMADDR_SQU1);
	nSquelch[0] = constrain(nSquelch[0], -99, 99);
	nSquelch[1] = NV_read_byte(NVMADDR_SQU2);
	nSquelch[1] = constrain(nSquelch[1], -99, 99);
	nScanStayTime = NV_read_byte(NVMADDR_SCSTAY);
	nAnyHoldTime = NV_read_byte(NVMADDR_ANYHOLD);
	nBacklightAdj = NV_read_byte(NVMADDR_BKADJ);
	nBacklightKeep = NV_read_byte(NVMADDR_BKKEEP);

	for (i = 0; i < NUM_BANDS; i++)
	{
		nBandCh[i] = NV_read_byte(NVMADDR_BANDCH + i);
		if (nBandCh[i] < 0 || nBandCh[i] >= nBandChs[i])
			nBandCh[i] = 0;

		u16 = NV_read_word(NVMADDR_BANDFREQ + i);
		if (u16 == 0xffff)  // Blank ch
			nBandFreq[i] = nBandFMin[i];
		else if (i <= BAND_SW)
			nBandFreq[i] = nBandFMin[i] + u16;
		else
			nBandFreq[i] = nBandFMin[i] + (int32_t)u16 * 5;
		nBandFreq[i] = constrain(nBandFreq[i], nBandFMin[i], nBandFMax[i]);
	}
}  // void NVMGetArgs(void)


void NVMUnpkWrData(const uint8_t* pAddr)
{
	const uint8_t *pF = pAddr;
	uint16_t pNVM = 0;
	uint8_t len, i, val;

	for (;;) {
		len = *(pF++);
		if (!len)
			break;
		if (len & 0x80) {  // Dup
			val = *(pF++);
			for (i = 0; i < (len & 0x7f); i++)
				NV_write_byte(pNVM++, val);
		}
		else {  // len bytes data
			for (i = 0; i < len; i++)
				NV_write_byte(pNVM++, *(pF++));
		}
	}
}

int constrain(int val, int min, int max)
{
	if (val >= max) return max;
	else if (val <= min) return min;
	else return val;
}
