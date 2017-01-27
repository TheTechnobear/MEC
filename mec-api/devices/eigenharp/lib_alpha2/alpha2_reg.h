
/*
 Copyright 2009 Eigenlabs Ltd.  http://www.eigenlabs.com

 This file is part of EigenD.

 EigenD is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 EigenD is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with EigenD.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ALPHA2_REG__
#define __ALPHA2_REG__

/*
 * Alpha V2.2 Control Registers
 * Logic release 0101
 *
 *
 * -- ++++++++++++++++++++++++++++++++++++++
 * -- Read/Status registers
 * -- ++++++++++++++++++++++++++++++++++++++
 *
 * -- Add   Name            Description
 * --  
 * -- 0     INSTR           Instrument identifier
 * -- 1     HWREV           Instrument Hardware Version
 * -- 2     FW_REV_MIN      Firmware Revision, Minor
 * -- 3     FW_REV_MAJ      Firmware Revision, Major
 * -- 4     SN_LOW          Serial No Low byte
 * -- 5     SN_HIGH         Serial No High byte
 * -- 6     TEMPERATURE     Instrument Temperature (deg C)
 * -- 7     REST_ERR_CNT    Com interface Restart Counter
 * -- 8     CTX_RESEND_CNT  Com Tx Resend Count
 * -- 9     CRX_ME_ERR_CNT  Com Rx Manchester Encoding Error Count
 * -- 10    CRX_PAR_ERR_CNT Com Rx Parity Error Count
 * -- 11    CRX_CRC_ERR_CNT Com Rx CRC Error Count
 * -- 12    STATUS1         misc status flags
 * -- 13    KF_OFLOW_CNT    Key filter overflow count
 * --
 */


#define A2_REG_INSTR           0
#define A2_REG_HWREV           1
#define A2_REG_FW_REV_MIN      2
#define A2_REG_FW_REV_MAJ      3
#define A2_REG_SN_LOW          4
#define A2_REG_SN_HIGH         5
#define A2_REG_TEMPERATURE     6
#define A2_REG_REST_ERR_CNT    7
#define A2_REG_CTX_RESEND_CNT  8
#define A2_REG_CRX_ME_ERR_CNT  9
#define A2_REG_CRX_PAR_ERR_CNT 10
#define A2_REG_CRX_CRC_ERR_CNT 11
#define A2_REG_STATUS1         12
#define A2_REG_KF_OFLOW_CNT    13

/*
 * --
 * -- ++++++++++++++++++++++++++++++++++++++
 * -- Write/Control registers
 * -- ++++++++++++++++++++++++++++++++++++++
 * --
 * -- 32    MODE            Instrument operational mode
 * -- 33    TF_TIMEOUT      Threshold Filter re-start timeout
 * -- 34    RA_TIMEOUT      key re-activation timeout
 * -- 35    DESEN_CONF      Threshold Filter De-sensitisation Configuration
 * -- 36    KA_THRESH       Key Activation Threshold (base parameter)
 * -- 37    KA_NOISE        Key Activation Noise
 * -- 38    HP_CONFIG       Headphone interface config
 * -- 39    HP_GAIN         Headphone gain (0 to -127 dB on 1dB steps)
 * -- 40    MIC_CONFIG      Microphone interface config
 * -- 41    MIC_GAIN        Microphone gain (0 or +10 to +65 dB in 1dB steps)
 *
 */

#define A2_REG_MODE            32
#define A2_REG_TF_TIMEOUT      33
#define A2_REG_RA_TIMEOUT      34
#define A2_REG_DESEN_CONF      35
#define A2_REG_KA_THRESH       36
#define A2_REG_KA_NOISE        37
#define A2_REG_HP_CONFIG       38
#define A2_REG_HP_GAIN         39
#define A2_REG_MIC_CONFIG      40
#define A2_REG_MIC_GAIN        41
#define A2_REG_LT_GAIN_MULT    42
#define A2_REG_LT_GAIN_DIV     43

/*
 * -- ================================================================================
 * -- Address  : 0
 * -- Name     : INSTR
 * -- Useage   : Instrument identifier
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --   7     R   x    INSTR7     Instrument ID Ms bit 
 * --   :     :   :      :             :
 * --   0     R   x    INSTR0     Instrument ID Ls bit
 * --
 * -- ================================================================================
 * --
 * --  INSTR = x01   for Alpha Mk2
 *
 *
 *
 * -- ================================================================================
 * -- Address  : 1
 * -- Name     : HWREV
 * -- Useage   : Instrument Hardware Version
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --   7     R   0    unused     unused (reads 0) 
 * --   :     :   :      :             :
 * --   4     R   0    unused     unused (reads o)
 * --   3     R   x    HWREV3     Hardware revision Ms bit 
 * --   :     :   :      :             :
 * --   0     R   x    HWREV0     Hardware revision Ls bit
 * --
 * -- ================================================================================
 * --
 * -- HW revision is read from external pull up/down resistors on the board
 * --
 * -- 0 - 3  = V2.1 control boards
 * -- 4      = V2.2 control boards
 *
 *
 *
 * -- ================================================================================
 * -- Address  : 2
 * -- Name     : FW_REV_MIN
 * -- Useage   : Firmware Revision, Minor
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --   7     R   x    FW_REV_MIN7   Firmware Revision Minor Ms bit 
 * --   :     :   :      :             :
 * --   0     R   x    FW_REV_MIN0   Firmware Revision Minor Ls bit
 * --
 * -- ================================================================================
 *
 *
 * -- ================================================================================
 * -- Address  : 3
 * -- Name     : FW_REV_MAJ
 * -- Useage   : Firmware Revision, Major
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --   7     R   x    FW_REV_MAJ7  Firmware Revision Major Ms bit 
 * --   :     :   :      :             :
 * --   0     R   x    FW_REV_MAJ0  Firmware Revision Major Ls bit
 * --
 * -- ================================================================================
 * --
 * -- Set bit 7 of Major revision high to indicate test builds
 *
 *
 * -- ================================================================================
 * -- Address  : 4
 * -- Name     : SN_LOW
 * -- Useage   : Serial No Low byte
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --   7     R   x    SN_LOW7     Serial No Low byte Ms bit 
 * --   :     :   :      :             :
 * --   0     R   x    SN_LOW0     Serial No Low byte Ls bit
 * --
 * -- ================================================================================
 *
 *
 * -- ================================================================================
 * -- Address  : 5
 * -- Name     : SN_HIGH
 * -- Useage   : Serial No Low byte
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --   7     R   x    SN_HIGH7   Serial No High byte Ms bit 
 * --   :     :   :      :             :
 * --   0     R   x    SN_HIGH0   Serial No High byte Ls bit
 * --
 * -- ================================================================================
 *
 *
 *
 * -- ================================================================================
 * -- Address  : 6
 * -- Name     : TEMPERATURE
 * -- Useage   : Instrument Temperature (deg C)
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --   7     R   x    TEMP7     Temperature Ms bit 
 * --   :     :   :      :             :
 * --   0     R   x    TEMP0     Temperature Ls bit
 * --
 * -- ================================================================================
 *
 *
 *
 * -- ================================================================================
 * -- Address  : 7
 * -- Name     : REST_ERR_CNT
 * -- Useage   : Com interface Restart Counter
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --   7     R   x    CNT7     Counter Ms bit 
 * --   :     :   :      :             :
 * --   0     R   x    CNT0     Counter Ls bit
 * --
 * -- ================================================================================
 *
 *
 *
 * -- ================================================================================
 * -- Address  : 8
 * -- Name     : CTX_RESEND_CNT
 * -- Useage   : Com Tx Resend Count
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --   7     R   x    CNT7     Counter Ms bit 
 * --   :     :   :      :             :
 * --   0     R   x    CNT0     Counter Ls bit
 * --
 * -- ================================================================================
 *
 *
 *
 * -- ================================================================================
 * -- Address  : 9
 * -- Name     : CRX_ME_ERR_CNT
 * -- Useage   : Com Rx Manchester Encoding Error Count
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --   7     R   x    CNT7     Counter Ms bit 
 * --   :     :   :      :             :
 * --   0     R   x    CNT0     Counter Ls bit
 * --
 * -- ================================================================================
 *
 *
 *
 * -- ================================================================================
 * -- Address  : 10
 * -- Name     : CRX_PAR_ERR_CNT
 * -- Useage   : Com Rx Parity Error Count
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --   7     R   x    CNT7     Counter Ms bit 
 * --   :     :   :      :             :
 * --   0     R   x    CNT0     Counter Ls bit
 * --
 * -- ================================================================================
 *
 *
 *
 * -- ================================================================================
 * -- Address  : 11
 * -- Name     : CRX_CRC_ERR_CNT
 * -- Useage   : Com Rx CRC Error Count
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --   7     R   x    CNT7     Counter Ms bit 
 * --   :     :   :      :             :
 * --   0     R   x    CNT0     Counter Ls bit
 * --
 * -- ================================================================================
 *
 *
 * -- ================================================================================
 * -- Address  : 12
 * -- Name     : STATUS1
 * -- Useage   : misc status flags
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --   7     R   0    unused     un-used
 * --   6     R   0    I2C_FAIL          last I2C operation failed
 * --   5     R   1    BOH_IDLE          Bulk out chan Idle flag (used for watchdog function)       
 * --   4     R   0    CAL_WRITE_ACTIVE  Cal data page write currently in progress  
 * --   3     R   0    CAL_ERASE_ACTIVE  Cal Erase currently in progress
 * --   2     R   0    FRM_WRITE_ACTIVE  Firmware page write currently in progress
 * --   1     R   0    FRM_UPDATE_OK     Firmware update completed sucessfully
 * --   0     R   0    FRM_UPDATE_FAIL   Firmware update Failed
 * --
 * -- ================================================================================
 *
 *
 *
 * -- ================================================================================
 * -- Address  : 13
 * -- Name     : KF_OFLOW_CNT
 * -- Useage   : Key filter overflow count
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --   7     R   x    KF_OFLOW_CNT(7)    number of times the ISO pkt gen has not 
 * --   :     :   :      :             :  completed before the key filter re-starts
 * --   0     R   x    KF_OFLOW_CNT(0)
 * --
 * -- ================================================================================
 *
 *
 *
 * -- ++++++++++++++++++++++++++++++++++++++
 * -- Write/Control registers
 * -- ++++++++++++++++++++++++++++++++++++++
 *
 *
 * -- Registers with a RW status marked 'P' are Pulse registers.
 * -- When a value of '1' is read from the RAM, a pulse is generated on the output, rather than a latched level.
 *
 * -- ================================================================================
 * -- Address  : 32
 * -- Name     : MODE
 * -- Useage   : Instrument operational mode
 * --
 * --  Bit    RW Res   Name            Useage
 * --  -------------------------------------------------------------------------------
 * --
 * --   7     P   0    RES          '1' = Reset the instrument (pulse) 
 * --   6     P   0    CLR_ERR_CNT  '1' = Clear the comms channel error counters (pulse) 
 * --   5     W   0    Unused       Unused 
 * --   4     W   1    USB_MODE     '0' = 128 byte packet mode, '1' = 512 byte packet mode 
 * --   3     W   0    DATA_EN      '1' = enable keyboard data transmission 
 * --   2     W   0    FB_MODE      FB LED mode (0 = Normal, 1 = Xmas tree) 
 * --   1     W   0    OP_MODE1     Operational Mode 
 * --   0     W   0    OP_MODE0
 * --
 * -- ================================================================================
 * --
 * -- FB_MODE     : this is overridden by DATA_EN - (if DATA_EN = 1, then FB mode will be Normal)
 * --
 * -- OP_MODE1:0  :   00 = Normal    ( Cooked keyboard data)
 * --                 01 = Raw       ( Raw keyboard data)
 * --                 1x = Simulator ( simulator mode)
 *
 *
 *
 *
 * -- ================================================================================
 * -- Address  : 33
 * -- Name     : TF_TIMEOUT
 * -- Useage   : Threshold Filter re-start timeout
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --
 * --   7:0   RW  100  TF_TIMEOUT     TF restart timeout (0-255) 
 * --
 * -- ================================================================================
 * --
 * -- Timeout after key de-activation before the Threshold Filter starts operating
 * -- ( timeout period =  TF_TIMEOUT * 50ms) 
 *
 *
 * -- ================================================================================
 * -- Address  : 34
 * -- Name     : RA_TIMEOUT
 * -- Useage   : key re-activation timeout
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --
 * --   7:6   R   0   un-used        un-used 
 * --   5:0   RW  40  RA_TIMEOUT     Re-activation timeout (0-63) 
 * --
 * -- ================================================================================
 * --
 * -- Timeout after key de-activation before a key can become active again
 * -- ( timeout period =  RA_TIMEOUT * 500uS) 
 * -- Default : 40 = 20ms
 *
 *
 *
 *
 * -- ================================================================================
 * -- Address  : 35
 * -- Name     : DESEN_CONF
 * -- Useage   : Threshold Filter De-sensitisation Configuration
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --
 * --   7:0   RW  xx  DESEN_CONF   TF De-sen Configuration 
 * --
 * -- ================================================================================
 * --
 * -- Used to configure the amount of key de-sensitisation generated from the Mute key.
 * --   TF_DESEN_FORCE = (<Mute key force> *  DESEN_CONF) / 16
 * --
 * --  TF_DESEN_FORCE is added to the threshold for all keys to reduce sensitivity.
 * --  It is clipped at a max value of 500.
 *
 *
 *
 * -- ================================================================================
 * -- Address  : 36
 * -- Name     : KA_THRESH
 * -- Useage   : Key Activation Threshold (base parameter)
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --
 * --   7:6   R   0   un-used        un-used 
 * --   5:0   RW  15  KA_THRESH     Key Activation Threshold (0-63) 
 * --
 * -- ================================================================================
 * --
 * -- Determines the key activation sensitivity.
 * -- This value is scaled by the ADC range of the key. KA_THRESH threshold corresponds
 * -- to the threshold used for a key with an ADC range of 2000 points, and will be scaled
 * -- up and down accordingly for keys with larger or smaller ADC ranges. 
 * --
 * -- Actual key activation threshold = scaled KA_THRESH + KA_NOISE
 *
 *
 *
 * -- ================================================================================
 * -- Address  : 37
 * -- Name     : KA_NOISE
 * -- Useage   : Key Activation Noise
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --
 * --   7:4   R   0   un-used        un-used 
 * --   3:0   RW  8   KA_NOISE     Key Activation Noise (0-15) 
 * --
 * -- ================================================================================
 * --
 * -- Used as an additional (un-scaled) parameter to generate the key activation threshold.
 * -- Actual key activation threshold = scaled KA_THRESH + KA_NOISE
 *
 *
 * -- ================================================================================
 * -- Address  : 38
 * -- Name     : HP_CONFIG
 * -- Useage   : Headphone interface config
 * --
 * --  Bit    RW Res   Name            Useage
 * --  -------------------------------------------------------------------------------
 * --
 * --   7:6   R   0   un-used          un-used 
 * --   5     R   1   HP_CONF_READY   '0' = configuration operation still in progress 
 * --   4     P   0   HP_CONF_START   Start a HP configuration option (pulse) 
 * --   3     RW  0   HP_SINE_LOOP    '1' = enable 1KHz sine wave test mode 
 * --   2     RW  0   HP_TEST_LOOP    '1' = enable Mic-to-HP loop test mode 
 * --   1     RW  1   HP_LIMIT        '1' = limit HP gain to -30dB
 * --   0     RW  1   HP_MUTE         '1' = Headphone O/P muted 
 * --
 * -- ================================================================================
 * --
 * -- Write '1' to HP_CONF_START to initiate a configuration write to the headphone interface.
 * -- Note, that this bit generates a control pulse, and will always read back 0.
 * --
 * -- The enable flags in this register and the Gain setting in the HP_GAIN register will only
 * -- be actioned when a '1' is written to HP_CONF_START.
 * --
 * -- HP_CONF_READY is high when the Mic interface is able to handle a new configuration operation,
 * -- and goes to '0' when a configuration operation is in progress
 *
 *
 *
 * -- ================================================================================
 * -- Address  : 39
 * -- Name     : HP_GAIN
 * -- Useage   : Headphone gain (0 to -127 dB in 1dB steps)
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --
 * --   7     R   0   un-used        un-used 
 * --   6:0   R  70   HP_GAIN       Gain. Value is -dB
 * --
 * -- ================================================================================
 * --
 * -- This gain setting is written to the Headphone interface when HP_CONF_START = '1' in HP_CONFIG.  
 *
 *
 *
 * -- ================================================================================
 * -- Address  : 40
 * -- Name     : MIC_CONFIG
 * -- Useage   : Microphone interface config
 * --
 * --  Bit    RW Res   Name            Useage
 * --  -------------------------------------------------------------------------------
 * --
 * --   7     R   0   un-used          un-used 
 * --   6     RW  1   MIC_MUTE         '1' = microphone muted, '0' = un-muted
 * --   5     R   1   MIC_CONF_READY   '0' = configuration operation still in progress 
 * --   4     P   0   MIC_CONF_START   Start a Mic configuration option (pulse)
 * --   3     RW  0   MIC_ELED_EN      '1' = enable the External Mic LED 
 * --   2     RW  0   MIC_PS48V_EN     '1' = enable 48V phantom power 
 * --   1     RW  1   MIC_BE_EN        '1' = enable Bias voltage
 * --   0     RW  0   MIC_PAD_EN       '1' = enable the PAD (= -23 db mic gain)
 * --
  -- ================================================================================
 * --
 * -- Write '1' to MIC_CONF_START to initiate a configuration write to the microphone interface.
 * -- Note, that this bit generates a control pulse, and will always read back 0.
 * --
 * -- The enable flags in this register and the Gain setting in the MIC_GAIN register will only
 * -- be actioned when a '1' is written to MIC_CONF_START.
 * --
 * -- MIC_CONF_READY is high when the Mic interface is able to handle a new configuration operation,
 * -- and goes to '0' when a configuration operation is in progress
 *
 *
 *
 * -- ================================================================================
 * -- Address  : 41
 * -- Name     : MIC_GAIN
 * -- Useage   : Microphone gain (0 or +10 to +65 dB in 1dB steps)
 * --
 * --  Bit    RW Res   Name          Useage
 * --  -------------------------------------------------------------------------------
 * --
 * --   7:6   R   0   un-used        un-used 
 * --   5:0   R  21   MIC_GAIN       Gain = MIC_GAIN+9 dB (also 0 = 0dB).
 * --
 * -- ================================================================================
 * --
 * -- This gain setting is written to the Mic interface when MIC_CONF_START = '1' in MIC_CONFIG.  
 *
 */

#endif
