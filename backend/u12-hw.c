/** @file u12-hw.c
 *  @brief The HW-access functions to the U12 backend stuff.
 *
 * Copyright (c) 2003 Gerhard Jaeger <gerhard@gjaeger.de>
 *
 * History:
 * - 0.01 - initial version
 * .
 * <hr>
 * This file is part of the SANE package.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 * As a special exception, the authors of SANE give permission for
 * additional uses of the libraries contained in this release of SANE.
 *
 * The exception is that, if you link a SANE library with other files
 * to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public
 * License.  Your use of that executable is in no way restricted on
 * account of linking the SANE library code into it.
 *
 * This exception does not, however, invalidate any other reasons why
 * the executable file might be covered by the GNU General Public
 * License.
 *
 * If you submit changes to SANE to the maintainers to be included in
 * a subsequent release, you agree by submitting the changes that
 * those changes may be distributed with this exception intact.
 *
 * If you write modifications of your own for SANE, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 * <hr>
 */

#define _TEST_SIZE 1000

/*************************** some local vars *********************************/

static RegDef u12CcdStop[] = {
/* this was the original sequence from parport backend*/
#if 0
#define _STOP_LEN 13

	{0x41, 0xff}, {0x42, 0xff}, {0x60, 0xff}, {0x61, 0xff},
	{0x4b, 0xff}, {0x4c, 0xff}, {0x4d, 0xff}, {0x4e, 0xff},
	{0x2a, 0x01}, {0x2b, 0x00}, {0x2d, 0x00}, {0x1b, 0x19}, {0x15, 0x00}
#else
#define _STOP_LEN 29
/* this is what we see from usb-snoop... */
	{0x60, 0xff}, {0x61, 0xff}, {0x1b, 0x19}, {0x15, 0x00}, {0x20, 0x14},
	{0x2c, 0x02}, {0x39, 0x00}, {0x3a, 0x00}, {0x3b, 0x00}, {0x3c, 0x00},
	{0x41, 0x00}, {0x42, 0x00}, {0x43, 0x00}, {0x44, 0x00}, {0x45, 0x01},
	{0x46, 0x00}, {0x47, 0x00}, {0x48, 0x00}, {0x49, 0x00}, {0x4a, 0x09},
	{0x4b, 0x00}, {0x4c, 0x00}, {0x4d, 0x00}, {0x4e, 0x01}, {0x50, 0x00},
	{0x51, 0x00}, {0x52, 0x00}, {0x53, 0x01}, {0x67, 0x00}
#endif
};

/**
 */
static void u12hw_SelectLampSource( U12_Device *dev )
{
	dev->regs.RD_ScanControl &= (~_SCAN_LAMPS_ON);

	if( dev->DataInf.dwScanFlag & (_SCANDEF_TPA)) {
		dev->regs.RD_ScanControl |= _SCAN_TPALAMP_ON;
	} else {
		dev->regs.RD_ScanControl |= _SCAN_NORMALLAMP_ON;
	}
}

/** as the function name says
 */
static void u12hw_PutToIdleMode( U12_Device *dev )
{
	DBG( _DBG_INFO, "CCD-Stop\n" );
	u12io_DataToRegs( dev, (SANE_Byte*)u12CcdStop, _STOP_LEN );
}

/** program the CCD relevant stuff
 */
static void u12hw_ProgramCCD( U12_Device *dev )
{
	SANE_Byte *reg_val;

	DBG( _DBG_INFO, "u12hw_ProgramCCD: 0x%08lx[%lu]\n",
	                (u_long)dev->CCDRegs,
	                ((u_long)dev->numCCDRegs * dev->shade.intermediate));

	DBG( _DBG_INFO, " * %u regs * %u (intermediate)\n",
	                dev->numCCDRegs, dev->shade.intermediate );

	reg_val = (SANE_Byte*)(dev->CCDRegs +
	                       (u_long)dev->numCCDRegs * dev->shade.intermediate);

	u12io_DataToRegs( dev, reg_val, dev->numCCDRegs );
}

/** init the stuff according to the buttons
 */
static void u12hw_ButtonSetup( U12_Device *dev, SANE_Byte numButtons )
{
	dev->Buttons = numButtons;

	dev->regs.RD_MotorDriverType |= _BUTTON_DISABLE;
	dev->MotorPower              |= _BUTTON_DISABLE;
}

/** According to what we have detected, set the other stuff
 */
static void u12hw_InitiateComponentModel( U12_Device *dev )
{
	/* preset some stuff and do the differences later */
	dev->Buttons      = 0;
	dev->ModelOriginY = 64;
	dev->Tpa          = SANE_FALSE;
	dev->ModelCtrl    = (_LED_ACTIVITY | _LED_CONTROL);

	switch( dev->PCBID ) {

	case _PLUSTEK_SCANNER:
		DBG( _DBG_INFO, "We have a Plustek Scanner ;-)\n" );
		break;

	case _SCANNER_WITH_TPA:
		DBG( _DBG_INFO, "Scanner has TPA\n" );
		dev->Tpa = SANE_TRUE;
		break;

	case _SCANNER4Button:
		DBG( _DBG_INFO, "Scanner has 4 Buttons\n" );
		u12hw_ButtonSetup( dev, 4 );
		break;

	case _SCANNER4ButtonTPA:
		DBG( _DBG_INFO, "Scanner has 4 Buttons & TPA\n" );
		dev->Tpa = SANE_TRUE;
		u12hw_ButtonSetup( dev, 4 );
		break;

	case _SCANNER5Button:
		DBG( _DBG_INFO, "Scanner has 5 Buttons\n" );
		dev->ModelOriginY += 20;
		u12hw_ButtonSetup( dev, 5 );
		break;

	case _SCANNER5ButtonTPA:
		DBG( _DBG_INFO, "Scanner has 5 Buttons & TPA\n" );
		dev->ModelOriginY += 20;
		dev->Tpa           = SANE_TRUE;
		u12hw_ButtonSetup( dev, 5 );
		break;

	case _SCANNER1Button:
		DBG( _DBG_INFO, "Scanner has 1 Button\n" );
		u12hw_ButtonSetup( dev, 1 );
		break;

	case _SCANNER1ButtonTPA:
		DBG( _DBG_INFO, "Scanner has 1 Button & TPA\n" );
		dev->Tpa           = SANE_TRUE;
		u12hw_ButtonSetup( dev, 1 );
		break;

	case _AGFA_SCANNER:
		DBG( _DBG_INFO, "Agfa Scanner\n" );
		dev->ModelOriginY = 24;  /* 1200 dpi */
		break;

	case _SCANNER2Button:
		DBG( _DBG_INFO, "Scanner has 2 Buttons\n" );
		dev->ModelOriginY -= 33;
		u12hw_ButtonSetup( dev, 2 );
		break;

	default:
		DBG( _DBG_INFO, "Default Model: U12\n" );
		break;
	}

#if 0
	if( _MOTOR0_2003 == dev->MotorID ) {
		dev->f2003      = SANE_TRUE;
		dev->XStepMono  = 10;
		dev->XStepColor = 6;
		dev->XStepBack  = 5;
		dev->regs.RD_MotorDriverType |= _MOTORR_STRONG;
	} else {
#endif
		dev->f2003      = SANE_FALSE;
		dev->XStepMono  = 8;
		dev->XStepColor = 4;
		dev->XStepBack  = 5;
		dev->regs.RD_MotorDriverType |= _MOTORR_WEAK;
/*	} */
}
  
/**
 */
static SANE_Status u12hw_InitAsic( U12_Device *dev, SANE_Bool shading )
{
	DBG( _DBG_INFO, "u12hw_InitAsic(%d)\n", shading );

	/* get DAC and motor stuff */
	dev->DACType = u12io_DataFromRegister( dev, REG_RESETCONFIG );
	dev->MotorID = (SANE_Byte)(dev->DACType & _MOTOR0_MASK);

	dev->regs.RD_MotorDriverType =
	                        (SANE_Byte)((dev->DACType & _MOTOR0_MASK) >> 3);
	dev->regs.RD_MotorDriverType |=
	                        (SANE_Byte)((dev->DACType & _MOTOR1_MASK) >> 1);
	dev->DACType &= _ADC_MASK;

	dev->MotorPower = dev->regs.RD_MotorDriverType | _MOTORR_STRONG;

	/*get CCD and PCB ID */
	dev->PCBID = u12io_DataFromRegister( dev, REG_CONFIG );
	dev->CCDID = dev->PCBID & 0x07;
	dev->PCBID &= 0xf0;

	if( _AGFA_SCANNER == dev->PCBID )
		dev->DACType = _DA_WOLFSON8141;

	DBG( _DBG_INFO, "* PCB-ID=0x%02x, CCD-ID=0x%02x, DAC-TYPE=0x%02x\n",
	                 dev->PCBID, dev->CCDID, dev->DACType );

	u12hw_InitiateComponentModel( dev );
	u12ccd_InitCCDandDAC( dev, shading );

	dev->regs.RD_Model1Control = _CCD_SHIFT_GATE;
	if( dev->Buttons != 0 )
		dev->regs.RD_Model1Control += _BUTTON_MODE;

	if( dev->shade.intermediate & _ScanMode_Mono )
		dev->regs.RD_Model1Control += _SCAN_GRAYTYPE;

	DBG( _DBG_INFO, "* MotorDrvType = 0x%02x\n", dev->regs.RD_MotorDriverType); 
	u12io_DataToRegister( dev, REG_MOTORDRVTYPE, dev->regs.RD_MotorDriverType);

	u12io_DataToRegister( dev, REG_WAITSTATEINSERT, 4 );

	DBG( _DBG_INFO, "* Model1Cntrl  = 0x%02x\n", dev->regs.RD_Model1Control );
	u12io_DataToRegister( dev, REG_MODEL1CONTROL, dev->regs.RD_Model1Control );

	u12hw_ProgramCCD( dev );
	DBG( _DBG_INFO, "u12hw_InitAsic done.\n" );
	return SANE_STATUS_GOOD;
}

/**
 */
static void u12hw_ControlLampOnOff( U12_Device *dev )
{
	SANE_Byte lampStatus;

	dev->warmupNeeded = SANE_TRUE;

	lampStatus = dev->regs.RD_ScanControl & _SCAN_LAMPS_ON;

	if ( dev->lastLampStatus != lampStatus ) {

		DBG( _DBG_INFO, "* Using OTHER Lamp --> warmup needed\n" );
		dev->lastLampStatus = lampStatus;

		u12io_DataToRegister( dev, REG_SCANCONTROL, dev->regs.RD_ScanControl );
		return;
	}

	dev->warmupNeeded = SANE_FALSE;
	DBG( _DBG_INFO, "* Using SAME Lamp --> no warmup needed\n" );
}

/** set all necessary register contents
 */
static void u12hw_SetGeneralRegister( U12_Device *dev )
{
	DBG( _DBG_INFO, "u12hw_SetGeneralRegister()\n" );

	dev->scan.motorBackward = SANE_FALSE;
	dev->scan.refreshState  = SANE_FALSE;

	if( COLOR_BW == dev->DataInf.wPhyDataType )
		dev->regs.RD_ScanControl = _SCAN_BITMODE;
	else {
		if( dev->DataInf.wPhyDataType <= COLOR_TRUE24 )
			dev->regs.RD_ScanControl = _SCAN_BYTEMODE;
		else
			dev->regs.RD_ScanControl = _SCAN_12BITMODE;
	}

	u12hw_SelectLampSource( dev );

	dev->regs.RD_ModelControl = (_LED_CONTROL | _LED_ACTIVITY);
	if( dev->shade.intermediate & _ScanMode_AverageOut )
		dev->regs.RD_ModelControl |= _MODEL_DPI300;
	else
		dev->regs.RD_ModelControl |= _MODEL_DPI600;

	dev->regs.RD_Motor0Control = _MotorOn | _MotorHQuarterStep | _MotorPowerEnable;
	dev->regs.RD_ScanControl1  = _SCANSTOPONBUFFULL | _MFRC_BY_XSTEP;
	dev->regs.RD_StepControl   = _MOTOR0_SCANSTATE;
}

/**
 */
static void u12hw_SetupScanningCondition( U12_Device *dev )
{
	TimerDef   timer;
	u_long     channel;
	int        c;
	SANE_Byte  state;
	SANE_Byte  rb[100];
	SANE_Byte *pState;

	DBG( _DBG_INFO, "u12_SetupScanningCondition()\n" );

	u12hw_SetGeneralRegister( dev );

	u12io_RegisterToScanner( dev, REG_RESETMTSC );
	_DODELAY(250);

	/* ------- Setup MinRead/MaxRead Fifo size ------- */
	if( dev->DataInf.wPhyDataType <= COLOR_TRUE24 ) {
		dev->scan.dwMaxReadFifo =
		dev->scan.dwMinReadFifo = dev->DataInf.dwAsicBytesPerPlane * 2;
	} else {
		dev->scan.dwMaxReadFifo =
		dev->scan.dwMinReadFifo = dev->DataInf.dwAppPixelsPerLine << 1;
	}

	if( dev->scan.dwMinReadFifo < 1024)
		dev->scan.dwMinReadFifo = dev->scan.dwMaxReadFifo = 1024;

	dev->scan.dwMaxReadFifo += (dev->DataInf.dwAsicBytesPerPlane / 2);

	DBG( _DBG_INFO, "* MinReadFifo=%lu, MaxReadFifo=%lu\n",
	     dev->scan.dwMinReadFifo, dev->scan.dwMaxReadFifo );

	/* ------- Set the max. read fifo to asic ------- */
	if( dev->DataInf.wPhyDataType > COLOR_256GRAY ) {

		dev->scan.bFifoSelect = REG_BFIFOOFFSET;

		if( !dev->scan.p48BitBuf.pb ) {

			long lRed, lGreen;

			lRed = (_SIZE_REDFIFO - _SIZE_BLUEFIFO) /
			       dev->DataInf.dwAsicBytesPerPlane - dev->scan.bd_rk.wRedKeep;

			lGreen = (_SIZE_GREENFIFO - _SIZE_BLUEFIFO) /
			       dev->DataInf.dwAsicBytesPerPlane - dev->scan.gd_gk.wGreenKeep;

			if((lRed < 0) || (lGreen < 0)) {

				if( lRed < lGreen ) {
					channel = _RED_FULLSIZE << 16;
					dev->regs.RD_BufFullSize = _SIZE_REDFIFO;
					lGreen = lRed;
				} else {
					channel = _GREEN_FULLSIZE << 16;
					dev->regs.RD_BufFullSize = _SIZE_GREENFIFO;
				}

				lGreen = (u_long)(-lGreen * dev->DataInf.dwAsicBytesPerPlane);

				if(  dev->DataInf.wPhyDataType > COLOR_TRUE24 )
					lGreen >>= 1;

				dev->scan.dwMinReadFifo += (u_long)lGreen;
				dev->scan.dwMaxReadFifo += (u_long)lGreen;

			} else {
				channel = _BLUE_FULLSIZE << 16;
				dev->regs.RD_BufFullSize = _SIZE_BLUEFIFO;
			}
		} else {
			channel = _BLUE_FULLSIZE << 16;
			dev->regs.RD_BufFullSize = _SIZE_BLUEFIFO;
		}
	} else {
		dev->scan.bFifoSelect = REG_GFIFOOFFSET;
		channel = _GREEN_FULLSIZE << 16;
		dev->regs.RD_BufFullSize = _SIZE_GRAYFIFO;
	}

	dev->regs.RD_BufFullSize -= (dev->DataInf.dwAsicBytesPerPlane << 1);

	if( dev->DataInf.wPhyDataType > COLOR_TRUE24 )
		dev->regs.RD_BufFullSize >>= 1;

	dev->regs.RD_BufFullSize |= channel;

	dev->scan.bRefresh          = (SANE_Byte)(dev->scan.dwInterval << 1);
	dev->regs.RD_LineControl    = _LOBYTE(dev->shade.wExposure);
	dev->regs.RD_ExtLineControl = _HIBYTE(dev->shade.wExposure);
	dev->regs.RD_XStepTime      = _LOBYTE(dev->shade.wXStep);
	dev->regs.RD_ExtXStepTime   = _HIBYTE(dev->shade.wXStep);
	dev->regs.RD_Motor0Control  = _FORWARD_MOTOR;
	dev->regs.RD_StepControl    = _MOTOR0_SCANSTATE;
	dev->regs.RD_ModeControl    = _ModeScan/*(_ModeScan | _ModeFifoGSel)*/;

	DBG( _DBG_INFO, "* bRefresh = %i\n", dev->scan.bRefresh );

	if( dev->DataInf.wPhyDataType == COLOR_BW ) {
		dev->regs.RD_ScanControl = _SCAN_BITMODE;
	} else if( dev->DataInf.wPhyDataType <= COLOR_TRUE24 )
		dev->regs.RD_ScanControl = _SCAN_BYTEMODE;
	else {
		dev->regs.RD_ScanControl = _SCAN_12BITMODE;
	}

	dev->regs.RD_ScanControl |= _SCAN_1ST_AVERAGE;
	u12hw_SelectLampSource( dev );

	DBG( _DBG_INFO, "* RD_ScanControl = 0x%02x\n", dev->regs.RD_ScanControl );

	DBG( _DBG_INFO, "* ImageInfo: x=%u,y=%u,dx=%u,dy=%u\n",
			 dev->DataInf.crImage.x,  dev->DataInf.crImage.y,
			 dev->DataInf.crImage.cx, dev->DataInf.crImage.cy );

	dev->regs.RD_MotorTotalSteps = (dev->DataInf.crImage.cy * 4) +
	                               (dev->f0_8_16 ? 32 : 16) +
	                               (dev->scan.bDiscardAll ? 32 : 0);
	DBG( _DBG_INFO, "* RD_MotorTotalSteps = 0x%04x\n",
	                                             dev->regs.RD_MotorTotalSteps);

	dev->regs.RD_ScanControl1 = (_MTSC_ENABLE | _SCANSTOPONBUFFULL |
	                             _MFRC_RUNSCANSTATE | _MFRC_BY_XSTEP);
	DBG( _DBG_INFO, "* RD_ScanControl1 = 0x%02x\n", dev->regs.RD_ScanControl1);

	dev->regs.RD_Dpi = dev->DataInf.xyPhyDpi.x;

	if(!(dev->DataInf.dwScanFlag & _SCANDEF_TPA )) {
		dev->regs.RD_Origin = (u_short)(dev->adj.leftNormal*2+_DATA_ORIGIN_X);

	} else if( dev->DataInf.dwScanFlag & _SCANDEF_Transparency ) {
		dev->regs.RD_Origin = (u_short)dev->scan.posBegin;
	} else {
		dev->regs.RD_Origin = (u_short)dev->scan.negBegin;
	}
	dev->regs.RD_Origin += dev->DataInf.crImage.x;

	if( dev->shade.intermediate & _ScanMode_AverageOut )
		dev->regs.RD_Origin >>= 1;

	if( dev->DataInf.wPhyDataType == COLOR_BW )
		dev->regs.RD_Pixels = (u_short)dev->DataInf.dwAsicBytesPerPlane;
    else
		dev->regs.RD_Pixels = (u_short)dev->DataInf.dwAppPixelsPerLine;

	DBG( _DBG_INFO, "* RD_Origin = %u, RD_Pixels = %u\n",
	                         dev->regs.RD_Origin, dev->regs.RD_Pixels );

	/* ------- Prepare scan states ------- */
	memset( dev->a_nbNewAdrPointer, 0, _SCANSTATE_BYTES );
	memset( dev->bufs.b1.pReadBuf,  0, _NUMBER_OF_SCANSTEPS );

	if( dev->DataInf.wPhyDataType <= COLOR_256GRAY )
		state = (_SS_MONO | _SS_STEP);
	else
		state = (_SS_COLOR | _SS_STEP);

	for( channel = _NUMBER_OF_SCANSTEPS, pState = dev->bufs.b1.pReadBuf;
                                    channel; channel -= dev->scan.dwInterval ) {
		*pState = state;
		pState += dev->scan.dwInterval;
	}
	for( channel = 0, pState = dev->bufs.b1.pReadBuf;
	                                  channel < _SCANSTATE_BYTES; channel++)  {
		dev->a_nbNewAdrPointer[channel] = pState[0] | (pState[1] << 4);
	    pState += 2;
	}

	/* ------- Wait for scan state stop ------- */
	u12io_StartTimer( &timer, _SECOND * 2 );

	u12io_ResetFifoLen();
	while(!(u12io_GetScanState( dev ) & _SCANSTATE_STOP) &&
                                                    !u12io_CheckTimer(&timer));
	u12io_DownloadScanStates( dev );

	c = 0;
	_SET_REG( rb, c, REG_LINECONTROL, dev->regs.RD_LineControl );
	_SET_REG( rb, c, REG_EXTENDEDLINECONTROL,
	                      dev->regs.RD_ExtLineControl);
	_SET_REG( rb, c, REG_XSTEPTIME, dev->regs.RD_XStepTime );
	_SET_REG( rb, c, REG_EXTENDEDXSTEP, dev->regs.RD_ExtXStepTime );
	_SET_REG( rb, c, REG_MOTORDRVTYPE,
	                      dev->regs.RD_MotorDriverType );
	_SET_REG( rb, c, REG_STEPCONTROL, dev->regs.RD_StepControl );
	_SET_REG( rb, c, REG_MOTOR0CONTROL, dev->regs.RD_Motor0Control );
	_SET_REG( rb, c, REG_MODELCONTROL, dev->regs.RD_ModelControl );
	_SET_REG( rb, c, REG_DPILO, (_LOBYTE(dev->regs.RD_Dpi)));
	_SET_REG( rb, c, REG_DPIHI, (_HIBYTE(dev->regs.RD_Dpi)));
	_SET_REG( rb, c, REG_SCANPOSLO, (_LOBYTE(dev->regs.RD_Origin)));
	_SET_REG( rb, c, REG_SCANPOSHI,(_HIBYTE(dev->regs.RD_Origin)));
	_SET_REG( rb, c, REG_WIDTHPIXELLO,(_LOBYTE(dev->regs.RD_Pixels)));
	_SET_REG( rb, c, REG_WIDTHPIXELHI,(_HIBYTE(dev->regs.RD_Pixels)));
	_SET_REG( rb, c, REG_THRESHOLDLO,
	                                 (_LOBYTE(dev->regs.RD_ThresholdControl)));
	_SET_REG( rb, c, REG_THRESHOLDHI,
	                                 (_HIBYTE(dev->regs.RD_ThresholdControl)));
	_SET_REG( rb, c, REG_MOTORTOTALSTEP0,
	                                  (_LOBYTE(dev->regs.RD_MotorTotalSteps)));
	_SET_REG( rb, c, REG_MOTORTOTALSTEP1,
	                                  (_HIBYTE(dev->regs.RD_MotorTotalSteps)));
	_SET_REG( rb, c, REG_SCANCONTROL, dev->regs.RD_ScanControl);
	u12io_DataToRegs( dev, rb, c );
	_DODELAY(100);

	u12io_RegisterToScanner( dev, REG_INITDATAFIFO );
}

/**
 */
static SANE_Status u12hw_Memtest( U12_Device *dev )
{
	SANE_Byte tmp;
	SANE_Byte buf[_TEST_SIZE];
	int       i;

	DBG( _DBG_INFO, "u12hw_Memtest()\n" );

	/* prepare buffer */
	for( i = 0; i < _TEST_SIZE; i++ ) {
		buf[i] = (SANE_Byte)((i * 3) & 0xff);
	}

	/* avoid switching to Lamp0, when previously scanned in transp./neg mode */
	tmp = dev->lastLampStatus + _SCAN_BYTEMODE;
	u12io_DataToRegister( dev, REG_SCANCONTROL, tmp );

	u12io_DataToRegister( dev, REG_MODECONTROL, _ModeMappingMem );
	u12io_DataToRegister( dev, REG_MEMORYLO, 0 );
	u12io_DataToRegister( dev, REG_MEMORYHI, 0 );

	/* fill to buffer */
	u12io_MoveDataToScanner( dev, buf, _TEST_SIZE );

	u12io_DataToRegister( dev, REG_MODECONTROL, _ModeMappingMem );
	u12io_DataToRegister( dev, REG_MEMORYLO, 0 );
	u12io_DataToRegister( dev, REG_MEMORYHI, 0 );

	u12io_DataToRegister( dev, REG_WIDTHPIXELLO, 0 );
	u12io_DataToRegister( dev, REG_WIDTHPIXELHI, 5 );

	memset( buf, 0, _TEST_SIZE );
                                        
	dev->regs.RD_ModeControl = _ModeReadMappingMem;
	u12io_ReadData( dev, buf, _TEST_SIZE );

	for( i = 0; i < _TEST_SIZE; i++ ) {
		if((SANE_Byte)((i * 3) & 0xff) != buf[i] ) {
			DBG( _DBG_ERROR, "* Memtest failed at pos %u: %u != %u\n",
					i+1, buf[i], (SANE_Byte)((i * 3) & 0xff) );
			return SANE_STATUS_INVAL;
		}
	}
	DBG( _DBG_INFO, "* Memtest passed.\n" );
	return SANE_STATUS_GOOD;
}

/** check if ASIC can be accessed, if the version is supported and the memory
 *  is also accessible...
 */
static SANE_Status u12hw_CheckDevice( U12_Device *dev )
{
#ifndef _FAKE_DEVICE
	SANE_Byte tmp;

	if( !u12io_IsConnected( dev->fd )) {

		if( !u12io_OpenScanPath( dev ))
			return SANE_STATUS_IO_ERROR;
	}

	/* some setup stuff... */
	tmp = u12io_GetExtendedStatus( dev );
	DBG( _DBG_INFO, "* REG_STATUS2 = 0x%02x\n", tmp );
	if( tmp & _REFLECTIONLAMP_ON ) {
		DBG( _DBG_INFO, "* Normal lamp is ON\n" );
		dev->lastLampStatus = _SCAN_NORMALLAMP_ON;
	} else if( tmp & _TPALAMP_ON ) {
		dev->lastLampStatus = _SCAN_TPALAMP_ON;
		DBG( _DBG_INFO, "* TPA lamp is ON\n" );
	}

	u12io_DataToRegister( dev, REG_PLLPREDIV,      1 );
	u12io_DataToRegister( dev, REG_PLLMAINDIV,  0x20 );
	u12io_DataToRegister( dev, REG_PLLPOSTDIV,     2 );
	u12io_DataToRegister( dev, REG_CLOCKSELECTOR,  2 );

	if( !dev->initialized ) 
		return u12hw_Memtest( dev );
	else
		return SANE_STATUS_GOOD;
#else
	_VAR_NOT_USED( dev );
	return SANE_STATUS_GOOD;
#endif
}

static SANE_Status u12hw_WarmupLamp( U12_Device *dev )
{
	TimerDef timer;

	DBG( _DBG_INFO, "u12hw_WarmupLamp()\n" );

	if( dev->warmupNeeded ) {
		DBG( _DBG_INFO, "* warming up...\n" );
		u12io_StartTimer( &timer, _SECOND * dev->adj.warmup );
		while( !u12io_CheckTimer( &timer )) {
			if( u12io_IsEscPressed()) {
				DBG( _DBG_INFO, "* CANCEL detected!\n" );
				return SANE_STATUS_CANCELLED;
			}
		}
	} else {
		DBG( _DBG_INFO, "* skipped\n" );
	}
	return SANE_STATUS_GOOD;
}

/* END U12-HW.C .............................................................*/