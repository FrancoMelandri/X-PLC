

// -------------------------------------------------------------
// 
// Author ......... bMF
// Date ........... August 2011
// Description..... 
// Changes ........ 
// 
// -------------------------------------------------------------





#include "includes.h"
#include "..\Utiltities\projectInfo.h"
//#include "..\..\hw\spi\spi.h"
//#include "..\..\hw\UART\uart.h"
//#include "..\..\hw\adc\adc.h"
#include "coreplc.h"
#include "..\Inputs\Inputs.h"
#include "..\variables\variables.h"
#include "..\Alarms\alarms.h"
#include "..\..\..\hw\hw.h"
#include "..\..\..\hw\io.h"
#include "..\..\..\hw\rtc.h"
#include "plc.h"
#if defined(TE809VDC)
#include "..\..\hw\umisure_vdc.h"
#else
#include "..\..\hw\umisure.h"
#endif
#include "uart.h"
#include "..\Board\boards.h"

//
// Scheduler includes
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"



#define                     CURRENT_THRESHOLD           100
#define                     VOLTAGE_THRESHOLD           50
#define                     KW_THRESHOLD                50
#define                     VOLTAGE_C_THRESHOLD         50

#if !defined(TE809VDC)
#define                     WAIT_BEFORE_CYCLE		    5
#else
#define                     WAIT_BEFORE_CYCLE		    5
#endif

#if defined(TE809APID) || defined(TE809APIDASCOT)
#define                     PLC_CONTINUOUS_STEPS        1000
#else
#define                     PLC_CONTINUOUS_STEPS        1000
#endif
#define                     FILTER_OLD                  0.5
#define                     FILTER_NEW                  0.5



u32							__userActions [ MAX_USER_ACTIONS ];
u32							__userActions1 [ MAX_USER_ACTIONS ];
u8							__cntUserActions        = 0x00;
u8							__cntUserActions1       = 0x00;
u64                         __plc_delayCounter      = 0;
u8							_plc_enabled            = 0x00;
u8				            _variables_restored     = 0x00;
MEASURE                     __measure;

#if defined(TE809VDC)
MEASURE_REAL                __measure_real;
#endif

#if !defined(TE809VDC)
MEASURE_FILTER              __measure_filter;
#endif
u8                          __plc_flash_page [ DLFLASH_PAGE_SIZE ];
u32                         __plc_steps         = 0;
u8                          __isStarting        = 0;;
extern u32                  key_pressed;
XPLCVARIABLE		        variable;
XPLCVARIABLE		        variable1;
XPLCVARIABLE		        variable2;
XPLCVARIABLE		        variable3;

PLC_IO_STATUS               __plc_io_status     = IO_PRESSURE;
u16                         __points_page;
u16                         __points_lastPage;
u16                         __points_offset;
TABLE_HEADER                __points_header;
int				            __points_numPoint;
u32					        __points_dwValue;
TABLE_POINT			        __points_point1;
TABLE_POINT			        __points_point2;

UdmTemps                    __udm_Temperature;
UdmPressures                __udm_Press;
UdmCapacities               __udm_Cap;
float                       __R;
u16                         __EM;


u64                         __plcTimer          = 0;
u64                         __spiTimer          = 0;
u8                          inMains;
#ifndef TE809G
u8                          mainsMono;
#endif
u8                          groupMono;
VARIABLE                    genericVariable;
extern XPLCINFO			    _info;

u8                          __counterPhaseG;
u8                          __counterPhaseM;
float                       tmpFloat;
float                       taFactor;

float                       adjustFactorIR;
float                       adjustFactorIS;
float                       adjustFactorIT;

extern BASE_BOARD			__boards;

void    __updateIO_Power ( u8* inputMap, u16 dimensionIn, u8* outputMap, u16 dimensionOut );
void    __reset_Power ( void );


void __updateOutput ( u8* outputMap, u16 dimension )
{
	//
	// reset the button variables
	variable.type			= op_M;
	variable.address	    = ADDRESS_PRESSED_BTN;
	variable.bit			= BIT_STOP_BTN;
	variable.value		    = 0x00;
	XPLC_setMemory ( &variable );

	variable.bit			= BIT_START_BTN;
	XPLC_setMemory ( &variable );

	variable.bit			= BIT_RESET_BTN;
	XPLC_setMemory ( &variable );
}


//
// Copy the input into the memory variable
void __updateInputOutput ( u8* inputMap, u16 dimensionIn, u8* outputMap, u16 dimensionOut )
{

    //
    // update the Power
	__updateIO_Power ( inputMap, dimensionIn, outputMap, dimensionOut  );

    CAN_updateData ( );

//    INPUTS_UpdateAssignments ();

    if ( _info.Cycles > ( _info.OldCycles +  2 ) )
    {
        _info.OldCycles =  _info.Cycles;
        __updateOutput ( outputMap, dimensionOut ); 
    }
}



//
// 
void __firstLoop ( void ) 
{
	//
	// At the end of the first loop of the plc store
	// in the PLC variables the real values read from flash
    if ( _variables_restored == 0x00 )
    {
        VARIABLES_Restore ( 0x00 );
        INPUTS_UpdateAssignments ();
        _variables_restored = 0x01;
    }
}


//
//
float __isAlarmActive ( float id )
{
    return (float)ALARMS_IsActive ( (u16)id );;
}

//
//
void __variableChanged ( int variableId )
{
    VARIABLE var;
    if ( VARIABLES_ReadVariable ( variableId, &var ) )
    {
        XPLC_getMemory ( &var.plcVariable );
        VARIABLES_OnUpdate ( &var );
    }
}

void __plcStepPerformed ( void )
{
#if !defined(TE809VDC)

#if defined(TE809AUPS) || defined(TE809APID) || defined(TE809APIDASCOT)
    //if ( __isStarting == 0x00 )
    {
        __plc_steps++;
        if ( __plc_steps > PLC_CONTINUOUS_STEPS )
        {
            xTaskResumeAll ( );
            __plc_steps = 0;
            if ( key_pressed != 0 && __isStarting == 0x00 )
                SCREENS_perform ( 0x00 );
            vTaskSuspendAll ( );
        }
    }

#if defined(TE809AUPS)
    UPSBOARD_perform ( 1 );
#endif

#if defined(TE809APID) || defined(TE809APIDASCOT)
    PIDBOARD_perform ( 1 );
#endif


#else // TE809A
    if ( __isStarting == 0x00 )
    {
        __plc_steps++;
        if ( __plc_steps > PLC_CONTINUOUS_STEPS )
        {
            xTaskResumeAll ( );
            __plc_steps = 0;
            if ( key_pressed != 0 )
                SCREENS_perform ( 0x00 );
            vTaskSuspendAll ( );
        }
    }

#if defined(RID1000A) || defined(RID1000G)
    COMM_RIDPerform ( );
#endif  

#endif

#else // TE809VDC

    // Do Nothing

#endif
}

//
// Set the variables registered as user action
void __setUA ()
{
	u8	i;
    if ( __cntUserActions1 > 0 )
    {
        for ( i = 0; i < __cntUserActions1; i++ )
             __userActions [i] =  __userActions1 [i];
        __cntUserActions = __cntUserActions1;
        __cntUserActions1 = 0;
    
        if ( __cntUserActions > 0 )
        {
            for ( i = 0; i < __cntUserActions; i++ )
                VARIABLES_Set ( __userActions [i] );
        }
    }
}

//
// Reset the variables registered as user action
void __resetUA ( void )
{
	u8	i;
	if ( __cntUserActions > 0 )
	{
		for ( i = 0; i < __cntUserActions; i++ )
			VARIABLES_Reset ( __userActions [i] );
		__cntUserActions = 0;
	}
}

void __writeVariable ( XPLCVARIABLE* variable )
{
	u32             varId;

	//
	// Write the variable
	XPLC_getMemory ( variable );
    if ( variable->value != 0 )
    {
        varId = (u32)variable->value;
              
        VARIABLES_Flash ( varId );
    
        //
        // Reset the request
        variable->value = 0;
        XPLC_setMemory ( variable );
    }
}

//
// Check if the PLC core hase request to write a variable
void __checkForWriteVariable ( void )
{    
	//
	// Write the variables 
	variable.type			= op_MW;
	variable.address	    = ADDRESS_WRITE_VARIABLE_1;
    __writeVariable ( &variable );
	variable.address	    = ADDRESS_WRITE_VARIABLE_2;
    __writeVariable ( &variable );
	variable.address	    = ADDRESS_WRITE_VARIABLE_3;
    __writeVariable ( &variable );
	variable.address	    = ADDRESS_WRITE_VARIABLE_4;
    __writeVariable ( &variable );
	variable.address	    = ADDRESS_WRITE_VARIABLE_5;
    __writeVariable ( &variable );
}

float __VAL;
//
// Check if the PLC core hase request to write a variable
void __checkToSendSMS ( void )
{
    float           val;
    u16             stringId;
    
    variable.type			= op_MW;
	variable.address	    = ADDRESS_SEND_SMS_STRING_ID;
    val = XPLC_getMemory ( &variable );
    if ( val > 0 ) 
    {    
        stringId = (u16)val;
        MODEM_sendSMSByID ( stringId );
    
        //
        // reset the sms message string id
        variable.value  = 0;
        XPLC_setMemory ( &variable );
    }

    variable.type			= op_MB;
	variable.address	    = ADDRESS_SEND_SMS_UPLOAD;
    val = XPLC_getMemory ( &variable );
    __VAL = val;
    if ( val > 0 ) 
    {    
        MODEM_sendSMSUpload ( );    
        //
        // reset the sms message string id
        //~ variable.value  = 0;
        //~ XPLC_setMemory ( &variable );
    }
}


//
// Initialize the PLC
void PLC_initialize ( void )
{
    //
    //  filter variables
#if !defined(TE809VDC)
    __measure_filter.fr         = 0;
    __measure_filter.fg         = 0;
    __measure_filter.pickup     = 0;
#endif

    __counterPhaseG     = 0;
    __counterPhaseM     = 0;
    
    //
    // initialize measure
    memset ( &__measure, 0, sizeof ( MEASURE ) );
#if defined(TE809VDC)
    memset ( &__measure_real, 0, sizeof ( MEASURE_REAL ) );
#endif
    //
    // Initialize the core PLC
	XPLC_initialize ( __updateInputOutput, __firstLoop, __isAlarmActive, __variableChanged, __plcStepPerformed );


    UMISURE_getCalibration ( );
    
}

//
// Stop the PLC engine
void PLC_Stop ( void )
{
	XPLC_stop ();
}

//
// Reset the PLC
void PLC_Reset ( void )
{
    //
    // reset the Power board
	__reset_Power ( );
}

//
// register a user action for the next PLC cycle
void PLC_registerUserAction ( u32 action )
{
	if ( __cntUserActions1 < MAX_USER_ACTIONS )
		__userActions1 [ __cntUserActions1++ ] = action;
}


void PLC_onHertz ( void )
{
	sw_rtc_type     time;
	
	SWRTC_get ( &time );

	
    //	Write the variables 
	variable.type			= op_MB;
	variable.address	    = ADDRESS_CURRENT_YEAR;
    variable.value          = time.year;
    XPLC_setMemory ( &variable );
	variable.address	    = ADDRESS_CURRENT_MONTH;
    variable.value          = time.month;
    XPLC_setMemory ( &variable );
	variable.address	    = ADDRESS_CURRENT_DAY;
    variable.value          = time.day;
    XPLC_setMemory ( &variable );
	variable.address	    = ADDRESS_CURRENT_DAYOFWEEK;
    variable.value          = time.weekday;
    XPLC_setMemory ( &variable );
	variable.address	    = ADDRESS_CURRENT_HOUR;
    variable.value          = time.hour;
    XPLC_setMemory ( &variable );
	variable.address	    = ADDRESS_CURRENT_MINUTE;
    variable.value          = time.minute;
    XPLC_setMemory ( &variable );
	variable.address	    = ADDRESS_CURRENT_SECOND;
    variable.value          = time.second;
    XPLC_setMemory ( &variable );
}


//
// Performa single cycle of the PLC
u8 PLC_perform ( u8 force )
{    
    u64     startTimer;

    //
    // update the Power
	//
    if ( _plc_enabled == 0x01 )
    { 
        if ( ( ++__plc_delayCounter >= WAIT_BEFORE_CYCLE ) || 
             ( __isStarting == 0x01 ) ||
             ( force == 0x01 )
           )
        {            
            
            startTimer          = HWTMR_GetMicroTmr ( ); 
        
            __plc_delayCounter  = 0;
            __plc_steps         = 0;
            
            if ( __isStarting == 0x00 )
                __setUA ();
                
            //
            // Perform a cycle
            vTaskSuspendAll ( );
            XPLC_perform ( );
            xTaskResumeAll ();
            
            if ( __isStarting == 0x00 )
                __resetUA ( );
            
            if ( __isStarting == 0x00 )
            {
                __checkForWriteVariable ( );
                __checkToSendSMS ( );
            }
                        
#if defined(TE809A)
            if ( __boards.vdc_enable == 0x01 )
                VDCBOARD_perform ( );
#endif        
            __plcTimer = HWTMR_GetMicroTmr ( ) - startTimer;
        
            if ( __isStarting == 0x01 )
                return 0x02;
            return 0x01;
        }
    }
    return 0x00;
}




void PLC_enabled ( u8 enable )
{
	if ( _plc_enabled != enable )
	{
		if ( enable )
		{
			//
			// Start the plc program only if the 
			// flash is loaded with the program....
            _plc_enabled = enable;
            XPLC_start ();
		}
		else
		{
			_plc_enabled = enable;
			XPLC_stop ();
		}
	}
}


#if !defined(TE809VDC)

//
// Switch on all the leds
void PLC_setAllLedOn ( void )
{
    __measure.outs  = 0x00;
    __measure.outs1 = 0x00;
    __measure.led1  = 0xff;
    __measure.led2  = 0xff;
    __measure.flags = 0x00;
    UMISURE_exchange ( &__measure );   
}


//
// Update all the I/O of the power board according to the IO Tbales
void __updateIO_Power ( u8* inputMap, u16 dimensionIn, u8*outputMap, u16 dimensionOut ) 
{
    float               tmp1;    
    float               tmp2;    
    u8                  type;
    u8                  ret;
    u8                  unbalanced;
    XPLCVARIABLE		updateVariable;
    
    updateVariable.type		= op_M;
    updateVariable.address	= ADDRESS_STARTING_STATUS;
    updateVariable.bit        = IS_STARTING_1;
    XPLC_getMemory ( &updateVariable );
    __isStarting        = (u8)updateVariable.value;

#if !defined(TE809VDC)
    __udm_Temperature           = PROJECT_getUdmTemp ( );
    __udm_Press                 = PROJECT_getUdmPress ( );
    __udm_Cap                   = PROJECT_getUdmCap ( );

#if !defined(TE809ATS) && !defined(TE809SDMO) && !defined(TE809APID) && !defined(TE809APIDASCOT)
    __measure.fuel  = FUEL ();
    __measure.temp  = TEMP ();
#endif
#endif

    //
    // Output 1
    __measure.outs      = outputMap [ ADDRESS_OUT_MB ];
    
    variable1.type      = op_MB;
    variable1.address   = ADDRESS_SETTINGS_D01_TYPE;
    XPLC_getMemory ( &variable1 );
    switch ( (int)variable1.value )
    {
    case OFF:
        __measure.outs          &= 0xf7;
        break;
    case NO:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x01 ) == 0x01 ? 0x01 : 0x00 );
        break;
    case NC:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x01 ) == 0x01 ? 0x00 : 0x01 );
    }

    variable1.address   = ADDRESS_SETTINGS_D02_TYPE;
    XPLC_getMemory ( &variable1 );
    switch ( (int)variable1.value )
    {
    case OFF:
        __measure.outs          &= 0xfd;
        break;
    case NO:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x02 ) == 0x02 ? 0x02 : 0x00 );
        break;
    case NC:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x02 ) == 0x02 ? 0x00 : 0x02 );
        break;
    }
    __measure.outs  |= outputMap [ ADDRESS_OUT_MB ] & 0xfc;
    
    __measure.outs1             = 0x00;

    variable1.address           = ADDRESS_SETTINGS_D03_TYPE;
    XPLC_getMemory ( &variable1 );
    switch ( (int)variable1.value )
    {
    case NO:
        __measure.outs1		    |= ( ( outputMap [ ADDRESS_OUT1_MB ] & 0x01 ) == 0x01 ? 0x01 : 0x00 );
        break;
    case NC:
        __measure.outs1		    |= ( ( outputMap [ ADDRESS_OUT1_MB ] & 0x01 ) == 0x01 ? 0x00 : 0x01 );
        break;
    }
    variable1.address   = ADDRESS_SETTINGS_D04_TYPE;
    XPLC_getMemory ( &variable1 );
    switch ( (int)variable1.value )
    {
    case NO:
        __measure.outs1		    |= ( ( outputMap [ ADDRESS_OUT1_MB ] & 0x02 ) == 0x02 ? 0x02 : 0x00 );
        break;
    case NC:
        __measure.outs1		    |= ( ( outputMap [ ADDRESS_OUT1_MB ] & 0x02 ) == 0x02 ? 0x00 : 0x02 );
        break;
    }
    variable1.address   = ADDRESS_SETTINGS_D05_TYPE;
    XPLC_getMemory ( &variable1 );
    switch ( (int)variable1.value )
    {
    case NO:
        __measure.outs1		    |= ( ( outputMap [ ADDRESS_OUT1_MB ] & 0x04 ) == 0x04 ? 0x04 : 0x00 );
        break;
    case NC:
        __measure.outs1		    |= ( ( outputMap [ ADDRESS_OUT1_MB ] & 0x04 ) == 0x04 ? 0x00 : 0x04 );
        break;
    }
    variable1.address   = ADDRESS_SETTINGS_D06_TYPE;
    XPLC_getMemory ( &variable1 );
    switch ( (int)variable1.value )
    {
    case NO:
        __measure.outs1		    |= ( ( outputMap [ ADDRESS_OUT1_MB ] & 0x08 ) == 0x08 ? 0x08 : 0x00 );
        break;
    case NC:
        __measure.outs1		    |= ( ( outputMap [ ADDRESS_OUT1_MB ] & 0x08 ) == 0x08 ? 0x00 : 0x08 );
        break;
    }

    __measure.led1  = outputMap [ ADDRESS_LED1_MB ] & 0xff;
    __measure.led2  = outputMap [ ADDRESS_LED2_MB ] & 0x03;
    __measure.flags = 0x00;

#if !defined(TE809G)
    inMains = 0x01;
    if ( !( outputMap [ ADDRESS_FLAGS_MB ] & ( 1 << BIT_MB_SELCTOR ) ) ) 
    {
        __measure.flags |= 0x01;
        inMains = 0x00;
    }
#else
    __measure.flags |= 0x01;
    inMains = 0x00;
#endif

    if ( outputMap [ ADDRESS_FLAGS_MB ] & ( 1 << BIT_FAST_SELCTOR ) ) 
        __measure.flags |= 0x02;
    if ( __isStarting == 0x01 )
        __measure.flags |= 0x02;        
    
    // -------------
    //
    // Flags on phase rotations
    updateVariable.type           = op_MB;
    updateVariable.address        = ADDRESS_MAINS_ROTATION;
    XPLC_getMemory ( &updateVariable );
    switch ( (int)updateVariable.value )
    {
        case 1:
            __measure.flags |= 0x04;
            break;
        case 2:
            __measure.flags |= 0x08;
            break;
    }
    updateVariable.address        = ADDRESS_GENSET_ROTATION;
    XPLC_getMemory ( &updateVariable );
    switch ( (int)updateVariable.value )
    {
        case 1:
            __measure.flags |= 0x10;
            break;
        case 2:
            __measure.flags |= 0x20;
            break;
    }
    //
    // -------------


    updateVariable.address        = ADDRESS_MAINS_SYSTEM_TYPE;

#ifndef TE809G
    XPLC_getMemory ( &updateVariable );
    mainsMono               = (u8)updateVariable.value;
#endif
    
    updateVariable.address        = ADDRESS_GENSET_SYSTEM_TYPE;
    XPLC_getMemory ( &updateVariable );
    groupMono               = (u8)updateVariable.value;
//    mainsMono               = ( __MEMORY [ ADDRESS_GROUP_PARAMS ] & ( 1 << BIT_GROUP_MONOPHASE ) ? 0x01 : 0x00 );
    

    //
    // EXCHANGE
    ret = UMISURE_exchange ( &__measure );

        
    if ( ret == 0x01 )
    {    
        //
        // Load the offset
        updateVariable.type		= op_MR;
    
#if !defined(TE809G)
        if ( __measure.vrr > VOLTAGE_THRESHOLD )
        {
            updateVariable.address	= ADDRESS_ADJUST_VRR;
            XPLC_getMemory ( &updateVariable );
            __measure.vrr       = __measure.vrr + (s16)updateVariable.value;
        }
        if ( __measure.vrs > VOLTAGE_THRESHOLD )      
        {
            updateVariable.address	= ADDRESS_ADJUST_VRS;
            XPLC_getMemory ( &updateVariable );
            __measure.vrs       = __measure.vrs + (s16)updateVariable.value;
        }
        if ( __measure.vrt > VOLTAGE_THRESHOLD )
        {
            updateVariable.address	= ADDRESS_ADJUST_VRT;
            XPLC_getMemory ( &updateVariable );
            __measure.vrt       = __measure.vrt + (s16)updateVariable.value;
        }
#endif

        if ( __measure.vgr > VOLTAGE_THRESHOLD )
        {
            updateVariable.address	= ADDRESS_ADJUST_VGR;
            XPLC_getMemory ( &updateVariable );
            __measure.vgr       = __measure.vgr + (s16)updateVariable.value;
        }
        if ( __measure.vgs > VOLTAGE_THRESHOLD )
        {
            updateVariable.address	= ADDRESS_ADJUST_VGS;
            XPLC_getMemory ( &updateVariable );
            __measure.vgs       = __measure.vgs + (s16)updateVariable.value;
        }
        if ( __measure.vgt > VOLTAGE_THRESHOLD )
        {
            updateVariable.address	= ADDRESS_ADJUST_VGT;
            XPLC_getMemory ( &updateVariable );
            __measure.vgt       = __measure.vgt + (s16)updateVariable.value;
        }
        
        updateVariable.address	= ADDRESS_ADJUST_IR;
        XPLC_getMemory ( &updateVariable );
        adjustFactorIR = updateVariable.value / 100;
    
        updateVariable.address	= ADDRESS_ADJUST_IS;
        XPLC_getMemory ( &updateVariable );
        adjustFactorIS = updateVariable.value / 100;
    
        updateVariable.address	= ADDRESS_ADJUST_IT;
        XPLC_getMemory ( &updateVariable );
        adjustFactorIT = updateVariable.value / 100;
    
        updateVariable.type		= op_IR;
    
    
    
        // ---------------------
        // 
        // MAINS PHASE VOLTAGE
        //
#if !defined(TE809G)
		updateVariable.address	= ADDRESS_MB_VRR;
		updateVariable.value		= 0;
        if ( __measure.vrr > VOLTAGE_THRESHOLD )
            updateVariable.value		= (float)( __measure.vrr  / 10 );
		XPLC_setMemory ( &updateVariable );
        
		updateVariable.address	= ADDRESS_MB_VSR;
		updateVariable.value		= 0;
        if ( __measure.vrs > VOLTAGE_THRESHOLD && mainsMono != 0x01 )
            updateVariable.value		= (float)( __measure.vrs / 10 );
		XPLC_setMemory ( &updateVariable );    
        
		updateVariable.address	= ADDRESS_MB_VTR;
		updateVariable.value		= 0;
        if ( __measure.vrt > VOLTAGE_THRESHOLD && mainsMono == 0x00 )
            updateVariable.value		= (float)( __measure.vrt / 10 );
		XPLC_setMemory ( &updateVariable );    
#endif
        // 
        //
        // ---------------------

        
        // ---------------------
        // 
        // GENSET VOLTAGE
        //
		updateVariable.address	= ADDRESS_MB_VRG;
		updateVariable.value		= 0;
        if ( __measure.vgr > VOLTAGE_THRESHOLD )
            updateVariable.value		= (float)( __measure.vgr / 10 );
		XPLC_setMemory ( &updateVariable );    
        
		updateVariable.address	= ADDRESS_MB_VSG;
		updateVariable.value		= 0;
        if ( __measure.vgs > VOLTAGE_THRESHOLD && groupMono != 0x01 )
            updateVariable.value		= (float)( __measure.vgs / 10 );
		XPLC_setMemory ( &updateVariable );    
        
		updateVariable.address	= ADDRESS_MB_VTG;
		updateVariable.value		= 0;
        if ( __measure.vgt > VOLTAGE_THRESHOLD && groupMono == 0x00 )
            updateVariable.value		= (float)( __measure.vgt / 10 );
		XPLC_setMemory ( &updateVariable );    
        // 
        //
        // ---------------------
        
        
        
        // ---------------------
        // 
        // MIANS FREQUENCY
        //
#if !defined(TE809G)
		updateVariable.address	= ADDRESS_MB_FR;
		updateVariable.value		= 0;               
        if ( __measure.vrr > VOLTAGE_THRESHOLD && __measure.fr != 0 )
        {
            if ( __measure_filter.fr == 0 )
                __measure_filter.fr = __measure.fr;
            __measure.fr           = (u16) ( (float)( (float)__measure_filter.fr * FILTER_OLD ) + (float)( (float)__measure.fr * FILTER_NEW ) );
            __measure_filter.fr    = __measure.fr;
            updateVariable.value		= ( (float)1000000 / (float)__measure.fr );
        }
        else
            __measure_filter.fr = 0;
		XPLC_setMemory ( &updateVariable );
#endif
        // 
        //
        // ---------------------

        
        // ---------------------
        // 
        // GENSET FREQUENCY
        //
		updateVariable.address	= ADDRESS_MB_FG;
		updateVariable.value		= 0;
        if ( __measure.vgr > VOLTAGE_THRESHOLD && __measure.fg != 0 )
        {
            if ( __isStarting == 0x00 )
            {
                if ( __measure_filter.fg == 0 )
                    __measure_filter.fg = __measure.fg;
                __measure.fg            = (u16) ( (float)( (float)__measure_filter.fg * FILTER_OLD ) + (float)( (float)__measure.fg * FILTER_NEW ) );
                __measure_filter.fg     = __measure.fg;
            }
            updateVariable.value    = ( (float)1000000 / (float)__measure.fg );
        }
        else
            __measure_filter.fg = 0;
		XPLC_setMemory ( &updateVariable );
        // 
        //
        // ---------------------
        
        
                
        // ---------------------
        // 
        // MAINS LINE VOLTAGE
        //
#if !defined(TE809G)
        tmp1 = 999999;  // Min
        tmp2 = 0;       // Max
        //
        // compute the concatenate voltage
		updateVariable.address	= ADDRESS_MB_VRR_C;
        if ( mainsMono != 0x01 )
        {
            if ( __measure.vrrs > 0 )
                updateVariable.value		= (float) ( (float) __measure.vrrs / 10 );
            else
                updateVariable.value		= (float) ( sqrt ( (float)__measure.vrr * (float)__measure.vrr + (float)__measure.vrs * (float)__measure.vrs + (float)__measure.vrr * (float)__measure.vrs ) / 10 );
        }
        else
            updateVariable.value      = (float) ( (float)__measure.vrr / 10 );
        if ( updateVariable.value < VOLTAGE_C_THRESHOLD )
            updateVariable.value = 0;
		XPLC_setMemory ( &updateVariable );
        if ( updateVariable.value < tmp1  ) tmp1 = updateVariable.value;
        if ( updateVariable.value > tmp2  ) tmp2 = updateVariable.value;
            
		updateVariable.address	= ADDRESS_MB_VSR_C;        
        updateVariable.value      = 0;
        if ( mainsMono != 0x01 )
        {
            if ( __measure.vrst > 0 )
                updateVariable.value		= (float) ( (float) __measure.vrst / 10 );
            else
                updateVariable.value		= (float) ( sqrt ( (float)__measure.vrs * (float)__measure.vrs + (float)__measure.vrt *(float)__measure.vrt + (float)__measure.vrs*(float)__measure.vrt ) / 10 );                
            if ( updateVariable.value < VOLTAGE_C_THRESHOLD )
                updateVariable.value = 0;
            
            if ( updateVariable.value < tmp1  ) tmp1 = updateVariable.value;
            if ( updateVariable.value > tmp2  ) tmp2 = updateVariable.value;
            
        }
		XPLC_setMemory ( &updateVariable );
        
		updateVariable.address	= ADDRESS_MB_VTR_C;
        updateVariable.value      = 0;
        if ( mainsMono == 0x00 )
        {
            if ( __measure.vrtr > 0 )
                updateVariable.value		= (float) ( (float) __measure.vrtr / 10 );
            else
                updateVariable.value		= (float) ( sqrt ( (float)__measure.vrt*(float)__measure.vrt + (float)__measure.vrr*(float)__measure.vrr + (float)__measure.vrt*(float)__measure.vrr ) / 10 );
            if ( updateVariable.value < VOLTAGE_C_THRESHOLD )
                updateVariable.value = 0;
            
            if ( updateVariable.value < tmp1  ) tmp1 = updateVariable.value;
            if ( updateVariable.value > tmp2  ) tmp2 = updateVariable.value;
            
        }
		XPLC_setMemory ( &updateVariable );
    
    
		variable1.type	    = op_MB;
		variable1.address	= ADDRESS_MAINS_SIMMETRY;
		XPLC_getMemory ( &variable1 );
        unbalanced          = 0x00;
        if ( tmp2 > 0 && variable1.value > 0 )
        {
            tmpFloat = 100 - (int) ( (float)( tmp1 * 100 ) / tmp2 );        
            if ( tmpFloat > variable1.value )
                unbalanced = 0x01;
        }
		variable1.type	        = op_I;
		variable1.address	    = ADDRESS_MB_SIMMETRY;
        variable1.bit           = BIT_MAINS_SIMMETRY;
        variable1.value         = (float)unbalanced;
		XPLC_setMemory ( &variable1 );
        
    
#endif
        // 
        //
        // ---------------------

    
        // ---------------------
        // 
        // GENSET LINE VOLTAGE
        //
        tmp1 = 999999;  // Min
        tmp2 = 0;       // Max
        
		updateVariable.address	= ADDRESS_MB_VRG_C;        
        if ( groupMono != 0x01 )
        {
            if ( __measure.vgrs > 0 )
                updateVariable.value		= (float) ( (float) __measure.vgrs / 10 );
            else
                updateVariable.value		= (float) ( sqrt ( (float)__measure.vgr*(float)__measure.vgr + (float)__measure.vgs*(float)__measure.vgs + (float)__measure.vgr*(float)__measure.vgs ) / 10 );
        }
        else
            updateVariable.value      = (float) ( (float)__measure.vgr / 10 ); 
        if ( updateVariable.value < VOLTAGE_C_THRESHOLD )
            updateVariable.value = 0;
		XPLC_setMemory ( &updateVariable );
        if ( updateVariable.value < tmp1  ) tmp1 = updateVariable.value;
        if ( updateVariable.value > tmp2  ) tmp2 = updateVariable.value;
        
		updateVariable.address	= ADDRESS_MB_VSG_C;        
        updateVariable.value      = 0;
        if ( groupMono != 0x01 )
        {
            if ( __measure.vgst > 0 )
                updateVariable.value		= (float) ( (float) __measure.vgst / 10 );
            else
                updateVariable.value		= (float) ( sqrt ( (float)__measure.vgs*(float)__measure.vgs + (float)__measure.vgt*(float)__measure.vgt + (float)__measure.vgs*(float)__measure.vgt ) / 10 );
            if ( updateVariable.value < VOLTAGE_C_THRESHOLD )
                updateVariable.value = 0;
            if ( updateVariable.value < tmp1  ) tmp1 = updateVariable.value;
            if ( updateVariable.value > tmp2  ) tmp2 = updateVariable.value;
        }
		XPLC_setMemory ( &updateVariable );

        updateVariable.address	= ADDRESS_MB_VTG_C;        
        updateVariable.value      = 0;
        if ( groupMono == 0x00 )
        {
            if ( __measure.vgst > 0 )
                updateVariable.value		= (float) ( (float) __measure.vgtr / 10 );
            else
                updateVariable.value		= (float) ( sqrt ( (float)__measure.vgt*(float)__measure.vgt + (float)__measure.vgr*(float)__measure.vgr + (float)__measure.vgt*(float)__measure.vgr ) / 10 );
            if ( updateVariable.value < VOLTAGE_C_THRESHOLD )
                updateVariable.value = 0;
            if ( updateVariable.value < tmp1  ) tmp1 = updateVariable.value;
            if ( updateVariable.value > tmp2  ) tmp2 = updateVariable.value;
        }
		XPLC_setMemory ( &updateVariable );        
    
		variable1.type	    = op_MB;
		variable1.address	= ADDRESS_GENSET_SIMMETRY;
		XPLC_getMemory ( &variable1 );
        unbalanced          = 0x00;
        if ( tmp2 > 0 && variable1.value > 0 )
        {
            tmpFloat = 100 - (int) ( (float)( tmp1 * 100 ) / tmp2 );        
            if ( tmpFloat > variable1.value )
                unbalanced = 0x01;
        }
		variable1.type	            = op_I;
		variable1.address	        = ADDRESS_MB_SIMMETRY;
        variable1.bit               = BIT_GENSET_SIMMETRY;
        variable1.value             = (float)unbalanced;
		XPLC_setMemory ( &variable1 );
        
        // 
        //
        // ---------------------


    
        // ---------------------
        // 
        // CURRENTS
        //
#if !defined(TE809SDMO)
        updateVariable.type	    = op_MW;         
        updateVariable.address	= ADDRESS_MEASURE_TA_FACTOR;        
		XPLC_getMemory ( &updateVariable );
        taFactor = updateVariable.value;
        if ( taFactor == 0 )
            taFactor = 1;
        
        tmpFloat            = 0;
        updateVariable.type	    = op_IR;                
		updateVariable.address	= ADDRESS_MB_IR;
		updateVariable.value		= 0;
        if ( __measure.ir >= CURRENT_THRESHOLD && __measure.pr > KW_THRESHOLD )
        {
            updateVariable.value		= (float) ( (float) ( (float)__measure.ir * taFactor * adjustFactorIR )/ 1000 );
            if ( updateVariable.value > tmpFloat ) 
                tmpFloat            = updateVariable.value;
        }
		XPLC_setMemory ( &updateVariable );
            
		updateVariable.address	= ADDRESS_MB_IS;
		updateVariable.value		= 0;
#if !defined(TE809G)
        if ( ( inMains == 0x01 && __measure.is >= CURRENT_THRESHOLD && mainsMono != 0x01 && __measure.ps > KW_THRESHOLD ) ||
             ( inMains == 0x00 && __measure.is >= CURRENT_THRESHOLD && groupMono != 0x01 && __measure.ps > KW_THRESHOLD ) )
#else
        if ( ( inMains == 0x00 && __measure.is >= CURRENT_THRESHOLD && groupMono != 0x01 && __measure.ps > KW_THRESHOLD ) )   
#endif        
        {
            updateVariable.value		= (float) ( (float) ( (float)__measure.is * taFactor * adjustFactorIS ) / 1000 );
            if ( updateVariable.value > tmpFloat ) 
                tmpFloat            = updateVariable.value;
        }
		XPLC_setMemory ( &updateVariable );
    
		updateVariable.address	= ADDRESS_MB_IT;
		updateVariable.value		= 0;
    
#if !defined(TE809G)
        if ( ( inMains == 0x01 && __measure.it >= CURRENT_THRESHOLD && mainsMono == 0x00 && __measure.pt > KW_THRESHOLD ) ||
             ( inMains == 0x00 && __measure.it >= CURRENT_THRESHOLD && groupMono == 0x00 && __measure.pt > KW_THRESHOLD ) )
#else        
        if ( ( inMains == 0x00 && __measure.it >= CURRENT_THRESHOLD && groupMono == 0x00 && __measure.pt > KW_THRESHOLD ) )
#endif        
        {
            updateVariable.value		= (float) ( (float) ( (float)__measure.it * taFactor * adjustFactorIT ) / 1000 );
            if ( updateVariable.value > tmpFloat ) 
                tmpFloat            = updateVariable.value;
        }
		XPLC_setMemory ( &updateVariable );
    
        //
        // Total current
		updateVariable.address	= ADDRESS_MB_ITOT;
		updateVariable.value		= tmpFloat;
		XPLC_setMemory ( &updateVariable );
        // 
        //
        // ---------------------


        if ( __isStarting == 0x00 )
        {
            //
            // Potenze attive che derivano direttamente dalla scheda gruppo
            updateVariable.address	= ADDRESS_MB_PR;
            updateVariable.value      = 0;
            if ( ( inMains == 0x00 && __measure.ir >= CURRENT_THRESHOLD && __measure.vgr >= VOLTAGE_THRESHOLD && __measure.pr > KW_THRESHOLD ) ||
                 ( inMains == 0x01 && __measure.ir >= CURRENT_THRESHOLD && __measure.vrr >= VOLTAGE_THRESHOLD && __measure.pr > KW_THRESHOLD ) )
                updateVariable.value		= ( (float)__measure.pr * taFactor * adjustFactorIR ) / 1000;
            XPLC_setMemory ( &updateVariable );
            
            updateVariable.address	= ADDRESS_MB_PS;
            updateVariable.value      = 0;
#if !defined(TE809G)
            if ( ( inMains == 0x00 && groupMono != 0x01 && __measure.is >= CURRENT_THRESHOLD && __measure.vgs >= VOLTAGE_THRESHOLD && __measure.ps > KW_THRESHOLD ) ||
                 ( inMains == 0x01 && mainsMono != 0x01 && __measure.is >= CURRENT_THRESHOLD && __measure.vrs >= VOLTAGE_THRESHOLD && __measure.ps > KW_THRESHOLD ) )
#else        
            if ( ( inMains == 0x00 && groupMono != 0x01 && __measure.is >= CURRENT_THRESHOLD && __measure.vgs >= VOLTAGE_THRESHOLD && __measure.ps > KW_THRESHOLD ) )
#endif        
                updateVariable.value		= ( (float)__measure.ps * taFactor * adjustFactorIS ) / 1000;
            XPLC_setMemory ( &updateVariable );
            
            updateVariable.address	= ADDRESS_MB_PT;
            updateVariable.value      = 0;
#if !defined(TE809G)
            if ( ( inMains == 0x00 && groupMono == 0x00 && __measure.it >= CURRENT_THRESHOLD && __measure.vgt >= VOLTAGE_THRESHOLD && __measure.pt > KW_THRESHOLD ) ||
                 ( inMains == 0x01 && mainsMono == 0x00 && __measure.it >= CURRENT_THRESHOLD && __measure.vrt >= VOLTAGE_THRESHOLD && __measure.pt > KW_THRESHOLD ) )
#else        
            if ( ( inMains == 0x00 && groupMono == 0x00 && __measure.it >= CURRENT_THRESHOLD && __measure.vgt >= VOLTAGE_THRESHOLD && __measure.pt > KW_THRESHOLD ) )
#endif        
                updateVariable.value		= ( (float)__measure.pt * taFactor * adjustFactorIT) / 1000;
            XPLC_setMemory ( &updateVariable );
        }
#endif
        
        
#if !defined(TE809SDMO)
#if !defined(TE809G)
        if ( inMains == 0x01 )
        {
            tmpFloat            = 0;
            if ( __measure.ir > CURRENT_THRESHOLD && __measure.pr > KW_THRESHOLD )
                tmpFloat            = (float)( (float)__measure.vrr * (float)__measure.ir * taFactor * adjustFactorIR ) / 10000000;
            updateVariable.address	= ADDRESS_MB_PAR;
            updateVariable.value		= tmpFloat;
            tmp1                = ( tmpFloat * 1000 );
            XPLC_setMemory ( &updateVariable );
            
            updateVariable.address	= ADDRESS_MB_COSFIR;
            updateVariable.value		= 0;
            if ( tmpFloat != 0 )
            {
                updateVariable.value		= ( (float)__measure.pr * taFactor * adjustFactorIR ) / ( tmpFloat * 1000 );
                //~ if ( updateVariable.value > 1 )
                    //~ updateVariable.value = 1;
            }
            XPLC_setMemory ( &updateVariable );
        
            if ( mainsMono != 0x01 )
            {
                tmpFloat            = 0;
                if ( __measure.is > CURRENT_THRESHOLD && __measure.ps > KW_THRESHOLD )
                    tmpFloat            = (float)( (float)__measure.vrs * (float)__measure.is * taFactor * adjustFactorIS ) / 10000000;
                updateVariable.address	= ADDRESS_MB_PAS;
                updateVariable.value		= tmpFloat;
                tmp1                += ( tmpFloat * 1000 );
                XPLC_setMemory ( &updateVariable );
                
                updateVariable.address	= ADDRESS_MB_COSFIS;
                updateVariable.value		= 0;
                if ( tmpFloat != 0 )
                {
                    updateVariable.value		= ( (float)__measure.ps * taFactor * adjustFactorIS ) / ( tmpFloat * 1000 );
                    //~ if ( updateVariable.value > 1 )
                        //~ updateVariable.value = 1;
                }
                XPLC_setMemory ( &updateVariable );
            }
            else
            {
                updateVariable.address	= ADDRESS_MB_PAS;
                updateVariable.value		= 0;
                XPLC_setMemory ( &updateVariable );            
                updateVariable.address	= ADDRESS_MB_COSFIS;
                XPLC_setMemory ( &updateVariable );
            }
        
            if ( mainsMono == 0x00 )
            {
                tmpFloat            = 0;
                if ( __measure.it > CURRENT_THRESHOLD && __measure.pt > KW_THRESHOLD )
                    tmpFloat            = (float)( (float)__measure.vrt * (float)__measure.it * taFactor * adjustFactorIT) / 10000000;
                updateVariable.address	= ADDRESS_MB_PAT;
                updateVariable.value		= tmpFloat;
                tmp1                += ( tmpFloat * 1000 );
                XPLC_setMemory ( &updateVariable );
                
                updateVariable.address	= ADDRESS_MB_COSFIT;
                updateVariable.value		= 0;
                if ( tmpFloat != 0 )
                {
                    updateVariable.value		= ( (float)__measure.pt * taFactor * adjustFactorIT ) / ( tmpFloat * 1000 );
                    //~ if ( updateVariable.value > 1 )
                        //~ updateVariable.value = 1;
                }
                XPLC_setMemory ( &updateVariable );
            }
            else
            {
                updateVariable.address	= ADDRESS_MB_PAT;
                updateVariable.value		= 0;
                XPLC_setMemory ( &updateVariable );            
                updateVariable.address	= ADDRESS_MB_COSFIT;
                XPLC_setMemory ( &updateVariable );
            }
            
            
            updateVariable.address	= ADDRESS_MB_PA_TOT;
            updateVariable.value		= tmp1 / 1000;
            XPLC_setMemory ( &updateVariable );
            
            tmp2                = (float) ( __measure.pr * taFactor * adjustFactorIR );
            if ( mainsMono != 0x01 )
                tmp2            += (float) ( __measure.ps * taFactor * adjustFactorIS );
            if ( mainsMono == 0x00 )
                tmp2            +=  (float) (__measure.pt * taFactor * adjustFactorIT );
            
            updateVariable.address	= ADDRESS_MB_P_TOT;
            updateVariable.value		= tmp2 / 1000;
            XPLC_setMemory ( &updateVariable );
            
            updateVariable.address	= ADDRESS_MB_COSFI_TOT;
            updateVariable.value		= 0;
            if ( tmp1 != 0 )
                updateVariable.value		= (float)tmp2 / tmp1;
            //~ if ( updateVariable.value > 1 )
                //~ updateVariable.value = 1;
            XPLC_setMemory ( &updateVariable );
        }
        else
#endif             
        {

            tmpFloat            = 0;
            if ( __measure.ir > CURRENT_THRESHOLD && __measure.pr > KW_THRESHOLD )
                tmpFloat            = (float)( (float)__measure.vgr * (float)__measure.ir * taFactor * adjustFactorIR ) / 10000000;
            updateVariable.address	= ADDRESS_MB_PAR;
            updateVariable.value		= tmpFloat;
            tmp1                = ( tmpFloat * 1000 );
            XPLC_setMemory ( &updateVariable );
            
            updateVariable.address	= ADDRESS_MB_COSFIR;
            updateVariable.value		= 0;
            if ( tmpFloat != 0 )
            {
                updateVariable.value		= ( (float)__measure.pr * taFactor * adjustFactorIR ) / ( tmpFloat * 1000 );
                //~ if ( updateVariable.value > 1 )
                    //~ updateVariable.value = 1;
            }
            XPLC_setMemory ( &updateVariable );
        
            if ( groupMono != 0x01 )
            {
                tmpFloat            = 0;
                if ( __measure.is > CURRENT_THRESHOLD && __measure.ps > KW_THRESHOLD )
                    tmpFloat            = (float)( (float)__measure.vgs * (float)__measure.is * taFactor * adjustFactorIS ) / 10000000;
                updateVariable.address	= ADDRESS_MB_PAS;
                updateVariable.value		= tmpFloat;
                tmp1                += ( tmpFloat * 1000 );
                XPLC_setMemory ( &updateVariable );
                
                updateVariable.address	= ADDRESS_MB_COSFIS;
                updateVariable.value		= 0;
                if ( tmpFloat != 0 )
                {
                    updateVariable.value		= ( (float)__measure.ps * taFactor * adjustFactorIS ) / ( tmpFloat * 1000 );
                    //~ if ( updateVariable.value > 1 )
                        //~ updateVariable.value = 1;
                }
                XPLC_setMemory ( &updateVariable );
            }
            else
            {
                updateVariable.address	= ADDRESS_MB_PAS;
                updateVariable.value		= 0;
                XPLC_setMemory ( &updateVariable );            
                updateVariable.address	= ADDRESS_MB_COSFIS;
                XPLC_setMemory ( &updateVariable );
            }
            
            if ( groupMono == 0x00 )
            {
                tmpFloat            = 0;
                if ( __measure.it > CURRENT_THRESHOLD && __measure.pt > KW_THRESHOLD )
                    tmpFloat            = (float)( (float)__measure.vgt * (float)__measure.it * taFactor * adjustFactorIT) / 10000000;
                updateVariable.address	= ADDRESS_MB_PAT;
                updateVariable.value		= tmpFloat;
                tmp1                += ( tmpFloat * 1000 );
                XPLC_setMemory ( &updateVariable );
                
                updateVariable.address	= ADDRESS_MB_COSFIT;
                updateVariable.value		= 0;
                if ( tmpFloat != 0 )
                {
                    updateVariable.value		= ( (float) __measure.pt * taFactor * adjustFactorIT ) / ( tmpFloat * 1000 );
                    //~ if ( updateVariable.value > 1 )
                        //~ updateVariable.value = 1;
                }
                XPLC_setMemory ( &updateVariable );
            }
            else
            {
                updateVariable.address	= ADDRESS_MB_PAT;
                updateVariable.value		= 0;
                XPLC_setMemory ( &updateVariable );            
                updateVariable.address	= ADDRESS_MB_COSFIT;
                XPLC_setMemory ( &updateVariable );
            }
            
            updateVariable.address	= ADDRESS_MB_PA_TOT;
            updateVariable.value		= tmp1 / 1000;
            XPLC_setMemory ( &updateVariable );
            
            tmp2                =  (float) ( __measure.pr * taFactor * adjustFactorIR );
            if ( groupMono != 0x01 )
                tmp2                += ( ( (float)__measure.ps * taFactor * adjustFactorIS ) );
            if ( groupMono == 0x00 )
                tmp2                += ( ( (float)__measure.pt * taFactor * adjustFactorIT ) );
            updateVariable.address	= ADDRESS_MB_P_TOT;
            updateVariable.value		= tmp2 / 1000;
            XPLC_setMemory ( &updateVariable );
            
            updateVariable.address	= ADDRESS_MB_COSFI_TOT;
            updateVariable.value		= 0;
            if ( tmp1 != 0 )
                updateVariable.value		= (float)tmp2 / tmp1;
            //~ if ( updateVariable.value > 1 )
                //~ updateVariable.value = 1;
            XPLC_setMemory ( &updateVariable );            
        }
#endif    
    
#if !defined(TE809SDMO)
        // ------------------
        //
        // POTENZA REATTIVA
        //
        tmpFloat                        = 0;
        updateVariable.type             = op_IR;
        updateVariable.address	        = ADDRESS_MB_PAR;
        XPLC_getMemory ( &updateVariable );
        variable1.type                  = op_IR;
        variable1.address	            = ADDRESS_MB_PR;
        XPLC_getMemory ( &variable1 );
        tmp1                            = sqrt ( updateVariable.value * updateVariable.value - variable1.value * variable1.value );
        tmpFloat                        += tmp1;
        updateVariable.address	        = ADDRESS_MB_PRR;
        updateVariable.value	        = tmp1;
        XPLC_setMemory ( &updateVariable );
    
        tmp1                    = 0;
#ifndef TE809G
        if ( ( inMains == 0x01 && mainsMono != 0x01 ) ||
             ( inMains == 0x00 && groupMono != 0x01 ) )
#else
        if ( ( inMains == 0x00 && groupMono != 0x01 ) )
#endif        
        {
            updateVariable.address	        = ADDRESS_MB_PAS;
            XPLC_getMemory ( &updateVariable );
            variable1.address	            = ADDRESS_MB_PS;
            XPLC_getMemory ( &variable1 );
            tmp1                            = sqrt ( updateVariable.value * updateVariable.value - variable1.value * variable1.value );
        }
        tmpFloat                            += tmp1;
        updateVariable.address	            = ADDRESS_MB_PRS;
        updateVariable.value	            = tmp1;
        XPLC_setMemory ( &updateVariable );

        tmp1                    = 0;
#ifndef TE809G
        if ( ( inMains == 0x01 && mainsMono == 0x00 ) ||
             ( inMains == 0x00 && groupMono == 0x00 ) )
#else
        if ( ( inMains == 0x00 && groupMono == 0x00 ) )
#endif        
        {
            updateVariable.address	    = ADDRESS_MB_PAT;
            XPLC_getMemory ( &updateVariable );
            variable1.address	    = ADDRESS_MB_PT;
            XPLC_getMemory ( &variable1 );
            tmp1                    = sqrt ( updateVariable.value * updateVariable.value - variable1.value * variable1.value );
        }
        tmpFloat                += tmp1;
        updateVariable.address	    = ADDRESS_MB_PRT;
        updateVariable.value	        = tmp1;
        XPLC_setMemory ( &updateVariable );

        updateVariable.address	    = ADDRESS_MB_PR_TOT;
        updateVariable.value	        = tmpFloat;
        XPLC_setMemory ( &updateVariable );
#endif    
        //
        //
        // ------------------
        
        // -------------------
        // 
        // ANALOG INPUTS
        //
#if !defined(TE809ATS) && !defined(TE809SDMO)
        switch ( __plc_io_status )
        {
#if !defined(RID1000A) && !defined(RID1000G)        
        case IO_PRESSURE:
        {
            variable1.type      = op_MB;
            variable1.address   = ADDRESS_SETTINGS_AN1_TYPE;
            XPLC_getMemory ( &variable1 );
            
            if ( variable1.value == ANT_AN )
            {
                FLASH_pageRead ( __FLASH_IO_TABLED_AN_ADDRESS, __plc_flash_page );
                        
                //~ __R = ( ( __measure.oil - __OFFSET_OIL ) * __FS_OIL ) / ( __K_OIL - ( __measure.oil - __OFFSET_OIL ) );
                __R = ( ( __measure.oil - __calibration.offset_OIL ) * __FS_ANALOG ) / ( __calibration.k_OIL - ( __measure.oil - __calibration.offset_OIL ) );
            
                //
                // OIL PRESSURE TOOLS
                updateVariable.address					= ADDRESS_PRESSURE_TOOLS;
                updateVariable.type						= op_MB;
                XPLC_getMemory ( &updateVariable );
                
                type = -1;
                if ( updateVariable.value == 0 )
                    type = PRESSURE_VDO;
                else if ( updateVariable.value == 1 )
                    type = PRESSURE_VEGLIA;
                else if ( updateVariable.value == 2 )
                    type = PRESSURE_DATACON;
                else if ( updateVariable.value == 3 )
                    type = PRESSURE_CUSTOM;
                if ( type != -1 )
                {
                    __points_offset			= type * sizeof ( TABLE_HEADER );
                    memcpy ( &__points_header, __plc_flash_page + __points_offset, sizeof ( TABLE_HEADER ) );
                    
                    __points_lastPage			= __points_header.address;
                    FLASH_pageRead ( __points_lastPage, __plc_flash_page );
                    memcpy ( &__points_point1, __plc_flash_page, sizeof ( TABLE_POINT ) );
                
                    __points_numPoint 	    = 0;        
                    __points_dwValue		= 0;
                
                    while ( __points_numPoint < __points_header.numPoints )
                    {
                        __points_page = __points_header.address + ( ( __points_numPoint * sizeof ( TABLE_POINT ) ) / DLFLASH_PAGE_SIZE );
                        if ( __points_page != __points_lastPage )
                        {
                            __points_lastPage = __points_page;
                            FLASH_pageRead ( __points_lastPage, __plc_flash_page );
                        }
                        __points_offset = ( __points_numPoint * sizeof ( TABLE_POINT ) ) % DLFLASH_PAGE_SIZE;
                        memcpy ( &__points_point2, __plc_flash_page + __points_offset, sizeof ( TABLE_POINT ) );
                        
                        if ( ( __R >= __points_point1.Y && __R < __points_point2.Y ) ||
                             ( __R > __points_point2.Y && __R <= __points_point1.Y ) )
                        {
                            //
                            // linearize
                            tmp1 = ( ( ( __R - __points_point1.Y ) / ( __points_point2.Y - __points_point1.Y ) ) * ( __points_point2.X - __points_point1.X ) ) + __points_point1.X;
//                            tmp1 = __points_point1.X;
                            if ( __udm_Press == UP_PSI )
                                tmp1 = tmp1 * 14.5;
                        
                            __points_dwValue		= *(unsigned int*)&tmp1;
                            break;
                        }
                        __points_numPoint++;
                        __points_point1 = __points_point2;
                    }
                    
                    
                    inputMap [ ADDRESS_PRESSURE ] 			= ( ( __points_dwValue & 0xff000000 ) >> 24 );
                    inputMap [ ADDRESS_PRESSURE + 1 ] 	    = ( ( __points_dwValue & 0x00ff0000 ) >> 16 );
                    inputMap [ ADDRESS_PRESSURE + 2 ] 	    = ( ( __points_dwValue & 0x0000ff00 ) >> 8 );
                    inputMap [ ADDRESS_PRESSURE + 3 ] 	    = ( ( __points_dwValue & 0x000000ff ) >> 0 );
                }
            }
            else
            {
                tmp1 = 0;
                if ( variable1.value == ANT_NO )
                {
                    if ( __measure.oil <= 2048 )
                        tmp1 = 1;
                }
                else if ( variable1.value == ANT_NC )
                {
                    if ( __measure.oil >= 2048 )
                        tmp1 = 1;
                }
                __points_dwValue		                = *(unsigned int*)&tmp1;
                inputMap [ ADDRESS_PRESSURE ] 			= ( ( __points_dwValue & 0xff000000 ) >> 24 );
                inputMap [ ADDRESS_PRESSURE + 1 ] 	    = ( ( __points_dwValue & 0x00ff0000 ) >> 16 );
                inputMap [ ADDRESS_PRESSURE + 2 ] 	    = ( ( __points_dwValue & 0x0000ff00 ) >> 8 );
                inputMap [ ADDRESS_PRESSURE + 3 ] 	    = ( ( __points_dwValue & 0x000000ff ) >> 0 );
            }
            __plc_io_status = IO_TEMPERATURE;
        }
        break;
#else
        case IO_PRESSURE:
        {
            variable1.type      = op_MB;
            variable1.address   = ADDRESS_SETTINGS_AN1_TYPE;
            XPLC_getMemory ( &variable1 );
            
            if ( variable1.value == ANTR_AN_PRESS )
            {
                FLASH_pageRead ( __FLASH_IO_TABLED_AN_ADDRESS, __plc_flash_page );
                        
                //~ __R = ( ( __measure.oil - __OFFSET_OIL ) * __FS_OIL ) / ( __K_OIL - ( __measure.oil - __OFFSET_OIL ) );
                __R = ( ( __measure.oil - __calibration.offset_OIL ) * __FS_ANALOG ) / ( __calibration.k_OIL - ( __measure.oil - __calibration.offset_OIL ) );
            
                //
                // OIL PRESSURE TOOLS
                updateVariable.address					= ADDRESS_PRESSURE_TOOLS;
                updateVariable.type						= op_MB;
                XPLC_getMemory ( &updateVariable );
                
                type = -1;
                if ( updateVariable.value == 0 )
                    type = PRESSURE_VDO;
                else if ( updateVariable.value == 1 )
                    type = PRESSURE_VEGLIA;
                else if ( updateVariable.value == 2 )
                    type = PRESSURE_DATACON;
                else if ( updateVariable.value == 3 )
                    type = PRESSURE_CUSTOM;
                if ( type != -1 )
                {
                    __points_offset			= type * sizeof ( TABLE_HEADER );
                    memcpy ( &__points_header, __plc_flash_page + __points_offset, sizeof ( TABLE_HEADER ) );
                    
                    __points_lastPage			= __points_header.address;
                    FLASH_pageRead ( __points_lastPage, __plc_flash_page );
                    memcpy ( &__points_point1, __plc_flash_page, sizeof ( TABLE_POINT ) );
                
                    __points_numPoint 	    = 0;        
                    __points_dwValue		= 0;
                
                    while ( __points_numPoint < __points_header.numPoints )
                    {
                        __points_page = __points_header.address + ( ( __points_numPoint * sizeof ( TABLE_POINT ) ) / DLFLASH_PAGE_SIZE );
                        if ( __points_page != __points_lastPage )
                        {
                            __points_lastPage = __points_page;
                            FLASH_pageRead ( __points_lastPage, __plc_flash_page );
                        }
                        __points_offset = ( __points_numPoint * sizeof ( TABLE_POINT ) ) % DLFLASH_PAGE_SIZE;
                        memcpy ( &__points_point2, __plc_flash_page + __points_offset, sizeof ( TABLE_POINT ) );
                        
                        if ( ( __R >= __points_point1.Y && __R < __points_point2.Y ) ||
                             ( __R > __points_point2.Y && __R <= __points_point1.Y ) )
                        {
                            //
                            // linearize
                            tmp1 = ( ( ( __R - __points_point1.Y ) / ( __points_point2.Y - __points_point1.Y ) ) * ( __points_point2.X - __points_point1.X ) ) + __points_point1.X;
//                            tmp1 = __points_point1.X;
                            if ( __udm_Press == UP_PSI )
                                tmp1 = tmp1 * 14.5;
                        
                            __points_dwValue		= *(unsigned int*)&tmp1;
                            break;
                        }
                        __points_numPoint++;
                        __points_point1 = __points_point2;
                    }
                    
                    
                    inputMap [ ADDRESS_PRESSURE ] 			= ( ( __points_dwValue & 0xff000000 ) >> 24 );
                    inputMap [ ADDRESS_PRESSURE + 1 ] 	    = ( ( __points_dwValue & 0x00ff0000 ) >> 16 );
                    inputMap [ ADDRESS_PRESSURE + 2 ] 	    = ( ( __points_dwValue & 0x0000ff00 ) >> 8 );
                    inputMap [ ADDRESS_PRESSURE + 3 ] 	    = ( ( __points_dwValue & 0x000000ff ) >> 0 );
                }
            }
            else if ( variable1.value == ANTR_AN_LEV )
            {
                FLASH_pageRead ( __FLASH_IO_TABLED_AN_ADDRESS, __plc_flash_page );
                        
                //~ __R = ( ( __measure.oil - __OFFSET_OIL ) * __FS_OIL ) / ( __K_OIL - ( __measure.oil - __OFFSET_OIL ) );
                __R = ( ( __measure.oil - __calibration.offset_OIL ) * __FS_ANALOG ) / ( __calibration.k_OIL - ( __measure.oil - __calibration.offset_OIL ) );
            
                //
                // LEVL PRESSURE TOOLS
                updateVariable.address					= ADDRESS_PRESS_LEV_TOOLS;
                updateVariable.type						= op_MB;
                XPLC_getMemory ( &updateVariable );
                
                type = -1;
                if ( updateVariable.value == 0 )
                    type = LEVEL_VDO;
                else if ( updateVariable.value == 1 )
                    type = LEVEL_VEGLIA;
                else if ( updateVariable.value == 2 )
                    type = LEVEL_DATACON;
                else if ( updateVariable.value == 3 )
                    type = LEVEL_CUSTOM;
                if ( type != -1 )
                {
                    __points_offset			= type * sizeof ( TABLE_HEADER );
                    memcpy ( &__points_header, __plc_flash_page + __points_offset, sizeof ( TABLE_HEADER ) );
                    
                    __points_lastPage			= __points_header.address;
                    FLASH_pageRead ( __points_lastPage, __plc_flash_page );
                    memcpy ( &__points_point1, __plc_flash_page, sizeof ( TABLE_POINT ) );
                
                    __points_numPoint 	    = 0;        
                    __points_dwValue		= 0;
                
                    while ( __points_numPoint < __points_header.numPoints )
                    {
                        __points_page = __points_header.address + ( ( __points_numPoint * sizeof ( TABLE_POINT ) ) / DLFLASH_PAGE_SIZE );
                        if ( __points_page != __points_lastPage )
                        {
                            __points_lastPage = __points_page;
                            FLASH_pageRead ( __points_lastPage, __plc_flash_page );
                        }
                        __points_offset = ( __points_numPoint * sizeof ( TABLE_POINT ) ) % DLFLASH_PAGE_SIZE;
                        memcpy ( &__points_point2, __plc_flash_page + __points_offset, sizeof ( TABLE_POINT ) );
                        
                        if ( ( __R >= __points_point1.Y && __R < __points_point2.Y ) ||
                             ( __R > __points_point2.Y && __R <= __points_point1.Y ) )
                        {
                            //
                            // linearize
                            tmp1 = ( ( ( __R - __points_point1.Y ) / ( __points_point2.Y - __points_point1.Y ) ) * ( __points_point2.X - __points_point1.X ) ) + __points_point1.X;
//                            tmp1 = __points_point1.X;
                            __points_dwValue		= *(unsigned int*)&tmp1;
                            break;
                        }
                        __points_numPoint++;
                        __points_point1 = __points_point2;
                    }
                    
                    
                    inputMap [ ADDRESS_PRESSURE ] 			= ( ( __points_dwValue & 0xff000000 ) >> 24 );
                    inputMap [ ADDRESS_PRESSURE + 1 ] 	    = ( ( __points_dwValue & 0x00ff0000 ) >> 16 );
                    inputMap [ ADDRESS_PRESSURE + 2 ] 	    = ( ( __points_dwValue & 0x0000ff00 ) >> 8 );
                    inputMap [ ADDRESS_PRESSURE + 3 ] 	    = ( ( __points_dwValue & 0x000000ff ) >> 0 );
                }            
            }   
            else if ( variable1.value == ANTR_AN_TEMP )
            {
                FLASH_pageRead ( __FLASH_IO_TABLED_AN_ADDRESS, __plc_flash_page );
            
                //~ __R = ( ( __measure.oil - __OFFSET_OIL ) * __FS_OIL ) / ( __K_OIL - ( __measure.oil - __OFFSET_OIL ) );
                __R = ( ( __measure.oil - __calibration.offset_OIL ) * __FS_ANALOG ) / ( __calibration.k_OIL - ( __measure.oil - __calibration.offset_OIL ) );
            
                type = RID_TEMPERATURE;
            
                __points_offset			= type * sizeof ( TABLE_HEADER );
                memcpy ( &__points_header, __plc_flash_page + __points_offset, sizeof ( TABLE_HEADER ) );
                
                __points_lastPage			= __points_header.address;
                FLASH_pageRead ( __points_lastPage, __plc_flash_page );
                memcpy ( &__points_point1, __plc_flash_page, sizeof ( TABLE_POINT ) );
            
                __points_numPoint 	    = 0;        
                __points_dwValue		= 0;
            
                while ( __points_numPoint < __points_header.numPoints )
                {
                    __points_page = __points_header.address + ( ( __points_numPoint * sizeof ( TABLE_POINT ) ) / DLFLASH_PAGE_SIZE );
                    if ( __points_page != __points_lastPage )
                    {
                        __points_lastPage = __points_page;
                        FLASH_pageRead ( __points_lastPage, __plc_flash_page );
                    }
                    __points_offset = ( __points_numPoint * sizeof ( TABLE_POINT ) ) % DLFLASH_PAGE_SIZE;
                    memcpy ( &__points_point2, __plc_flash_page + __points_offset, sizeof ( TABLE_POINT ) );
                    
                    if ( ( __R >= __points_point1.Y && __R < __points_point2.Y ) ||
                         ( __R > __points_point2.Y && __R <= __points_point1.Y ) )
                    {
                        //
                        // linearize
                        tmp1 = ( ( ( __R - __points_point1.Y ) / ( __points_point2.Y - __points_point1.Y ) ) * ( __points_point2.X - __points_point1.X ) ) + __points_point1.X;
//                            tmp1 = __points_point1.X;
                        __points_dwValue		= *(unsigned int*)&tmp1;
                        break;
                    }
                    __points_numPoint++;
                    __points_point1 = __points_point2;
                }
                
                
                inputMap [ ADDRESS_PRESSURE ] 			= ( ( __points_dwValue & 0xff000000 ) >> 24 );
                inputMap [ ADDRESS_PRESSURE + 1 ] 	    = ( ( __points_dwValue & 0x00ff0000 ) >> 16 );
                inputMap [ ADDRESS_PRESSURE + 2 ] 	    = ( ( __points_dwValue & 0x0000ff00 ) >> 8 );
                inputMap [ ADDRESS_PRESSURE + 3 ] 	    = ( ( __points_dwValue & 0x000000ff ) >> 0 );
            }
            else
            {
                tmp1 = 0;
                if ( variable1.value == ANTR_NO )
                {
                    if ( __measure.oil <= 2048 )
                        tmp1 = 1;
                }
                else if ( variable1.value == ANTR_NC )
                {
                    if ( __measure.oil >= 2048 )
                        tmp1 = 1;
                }
                __points_dwValue		                = *(unsigned int*)&tmp1;
                inputMap [ ADDRESS_PRESSURE ] 			= ( ( __points_dwValue & 0xff000000 ) >> 24 );
                inputMap [ ADDRESS_PRESSURE + 1 ] 	    = ( ( __points_dwValue & 0x00ff0000 ) >> 16 );
                inputMap [ ADDRESS_PRESSURE + 2 ] 	    = ( ( __points_dwValue & 0x0000ff00 ) >> 8 );
                inputMap [ ADDRESS_PRESSURE + 3 ] 	    = ( ( __points_dwValue & 0x000000ff ) >> 0 );
            }
            __plc_io_status = IO_TEMPERATURE;
        }
        break;
#endif
    
        case IO_TEMPERATURE:
        {
            variable1.type      = op_MB;
            variable1.address   = ADDRESS_SETTINGS_AN2_TYPE;
            XPLC_getMemory ( &variable1 );
            
            if ( variable1.value == ANT_AN )
            {
                FLASH_pageRead ( __FLASH_IO_TABLED_AN_ADDRESS, __plc_flash_page );     
            
                //~ __R = ( ( __measure.temp - __OFFSET_TEMP ) * __FS_TEMP ) / ( __K_TEMP - ( __measure.temp - __OFFSET_TEMP ) );
                __R = ( ( __measure.temp - __calibration.offset_TEMP ) * __FS_ANALOG ) / ( __calibration.k_TEMP - ( __measure.temp - __calibration.offset_TEMP ) );

                //
                // TEMPERATURE TOOLS
                updateVariable.address					= ADDRESS_TEMPERATURE_TOOLS;
                updateVariable.type						= op_MB;
                XPLC_getMemory ( &updateVariable );
                

                type = -1;
                if ( updateVariable.value == 0 )
                    type = TEMPERATURE_VDO;
                else if ( updateVariable.value == 1 )
                    type = TEMPERATURE_VEGLIA;
                else if ( updateVariable.value == 2 )
                    type = TEMPERATURE_DATACON;
                else if ( updateVariable.value == 3 )
                    type = TEMPERATURE_CUSTOM;
                if ( type != -1 )
                {
                    __points_offset			= type * sizeof ( TABLE_HEADER );
                    memcpy ( &__points_header, __plc_flash_page + __points_offset, sizeof ( TABLE_HEADER ) );
                    
                    __points_lastPage			= __points_header.address;
                    FLASH_pageRead ( __points_lastPage, __plc_flash_page );
                    memcpy ( &__points_point1, __plc_flash_page, sizeof ( TABLE_POINT ) );
                
                    __points_numPoint 	    = 0;        
                    __points_dwValue		= 0;
                
                    while ( __points_numPoint < __points_header.numPoints )
                    {
                        __points_page = __points_header.address + ( ( __points_numPoint * sizeof ( TABLE_POINT ) ) / DLFLASH_PAGE_SIZE );
                        if ( __points_page != __points_lastPage )
                        {
                            __points_lastPage = __points_page;
                            FLASH_pageRead ( __points_lastPage, __plc_flash_page );
                        }
                        __points_offset = ( __points_numPoint * sizeof ( TABLE_POINT ) ) % DLFLASH_PAGE_SIZE;
                        memcpy ( &__points_point2, __plc_flash_page + __points_offset, sizeof ( TABLE_POINT ) );

                        if ( ( __R >= __points_point1.Y && __R < __points_point2.Y ) ||
                             ( __R > __points_point2.Y && __R <= __points_point1.Y ) )
                        {
                            //
                            // linearize
                            tmp1 = ( ( ( __R - __points_point1.Y ) / ( __points_point2.Y - __points_point1.Y ) ) * ( __points_point2.X - __points_point1.X ) ) + __points_point1.X;
//                            tmp1 = __points_point1.X;
                            if ( __udm_Temperature == UT_FAR )
                                tmp1 = ( tmp1 * (float)9 / 5 ) + 32;
                        
                            __points_dwValue		= *(unsigned int*)&tmp1;
                            break;
                        }
                        __points_numPoint++;
                        __points_point1 = __points_point2;
                    }
                    
                    
                    inputMap [ ADDRESS_TEMPERATURE ] 	    = ( ( __points_dwValue & 0xff000000 ) >> 24 );
                    inputMap [ ADDRESS_TEMPERATURE + 1 ] 	= ( ( __points_dwValue & 0x00ff0000 ) >> 16 );
                    inputMap [ ADDRESS_TEMPERATURE + 2 ] 	= ( ( __points_dwValue & 0x0000ff00 ) >> 8 );
                    inputMap [ ADDRESS_TEMPERATURE + 3 ] 	= ( ( __points_dwValue & 0x000000ff ) >> 0 );
                }
            }
            else
            {
                tmp1 = 0;
                if ( variable1.value == ANT_NO )
                {
                    if ( __measure.temp <= 2048 )
                        tmp1 = 1;
                }
                else if ( variable1.value == ANT_NC )
                {
                    if ( __measure.temp >= 2048 )
                        tmp1 = 1;
                }
                __points_dwValue		                = *(unsigned int*)&tmp1;
                inputMap [ ADDRESS_TEMPERATURE ] 			= ( ( __points_dwValue & 0xff000000 ) >> 24 );
                inputMap [ ADDRESS_TEMPERATURE + 1 ] 	    = ( ( __points_dwValue & 0x00ff0000 ) >> 16 );
                inputMap [ ADDRESS_TEMPERATURE + 2 ] 	    = ( ( __points_dwValue & 0x0000ff00 ) >> 8 );
                inputMap [ ADDRESS_TEMPERATURE + 3 ] 	    = ( ( __points_dwValue & 0x000000ff ) >> 0 );
            }
            __plc_io_status = IO_LEVEL;
        }
        break;
    
        case IO_LEVEL:
        {
            variable1.type      = op_MB;
            variable1.address   = ADDRESS_SETTINGS_AN3_TYPE;
            XPLC_getMemory ( &variable1 );
            
            if ( variable1.value == ANT_AN )
            {
                FLASH_pageRead ( __FLASH_IO_TABLED_AN_ADDRESS, __plc_flash_page );

                //~ __R = ( ( __measure.fuel - __OFFSET_LEV ) * __FS_LEV ) / ( __K_LEV - ( __measure.fuel - __OFFSET_LEV ) );
                __R = ( ( __measure.fuel - __calibration.offset_LEVEL ) * __FS_ANALOG ) / ( __calibration.k_LEVEL - ( __measure.fuel - __calibration.offset_LEVEL ) );
            
                //
                // LEVEL TOOLS
                updateVariable.address					= ADDRESS_LEVEL_TOOLS;
                updateVariable.type						= op_MB;
                XPLC_getMemory ( &updateVariable );
                
                type = -1;
                if ( updateVariable.value == 0 )
                    type = LEVEL_VDO;
                else if ( updateVariable.value == 1 )
                    type = LEVEL_VEGLIA;
                else if ( updateVariable.value == 2 )
                    type = LEVEL_DATACON;
                else if ( updateVariable.value == 3 )
                    type = LEVEL_CUSTOM;
                if ( type != -1 )
                {
                    __points_offset			= type * sizeof ( TABLE_HEADER );
                    memcpy ( &__points_header, __plc_flash_page + __points_offset, sizeof ( TABLE_HEADER ) );
                    
                    __points_lastPage			= __points_header.address;
                    FLASH_pageRead ( __points_lastPage, __plc_flash_page );
                    memcpy ( &__points_point1, __plc_flash_page, sizeof ( TABLE_POINT ) );
                
                    __points_numPoint 	    = 0;        
                    __points_dwValue		= 0;
                
                    while ( __points_numPoint < __points_header.numPoints )
                    {
                        __points_page = __points_header.address + ( ( __points_numPoint * sizeof ( TABLE_POINT ) ) / DLFLASH_PAGE_SIZE );
                        if ( __points_page != __points_lastPage )
                        {
                            __points_lastPage = __points_page;
                            FLASH_pageRead ( __points_lastPage, __plc_flash_page );
                        }
                        __points_offset = ( __points_numPoint * sizeof ( TABLE_POINT ) ) % DLFLASH_PAGE_SIZE;
                        memcpy ( &__points_point2, __plc_flash_page + __points_offset, sizeof ( TABLE_POINT ) );
                        
                        if ( ( __R >= __points_point1.Y && __R < __points_point2.Y ) ||
                             ( __R > __points_point2.Y && __R <= __points_point1.Y ) )
                        {
                            //
                            // linearize
                            tmp1 = ( ( ( __R - __points_point1.Y ) / ( __points_point2.Y - __points_point1.Y ) ) * ( __points_point2.X - __points_point1.X ) ) + __points_point1.X;
//                            tmp1 = __points_point1.X;
                            __points_dwValue		= *(unsigned int*)&tmp1;
                            break;
                        }
                        __points_numPoint++;
                        __points_point1 = __points_point2;
                    }
                    
                    inputMap [ ADDRESS_LEVEL ] 		= ( ( __points_dwValue & 0xff000000 ) >> 24 );
                    inputMap [ ADDRESS_LEVEL + 1 ] 	= ( ( __points_dwValue & 0x00ff0000 ) >> 16 );
                    inputMap [ ADDRESS_LEVEL + 2 ] 	= ( ( __points_dwValue & 0x0000ff00 ) >> 8 );
                    inputMap [ ADDRESS_LEVEL + 3 ] 	= ( ( __points_dwValue & 0x000000ff ) >> 0 );
                }
            }
            else
            {
                tmp1 = 0;
                if ( variable1.value == ANT_NO )
                {
                    if ( __measure.fuel <= 2048 )
                        tmp1 = 1;
                }
                else if ( variable1.value == ANT_NC )
                {
                    if ( __measure.fuel >= 2048 )
                        tmp1 = 1;
                }
                __points_dwValue		                = *(unsigned int*)&tmp1;
                inputMap [ ADDRESS_LEVEL ] 			= ( ( __points_dwValue & 0xff000000 ) >> 24 );
                inputMap [ ADDRESS_LEVEL + 1 ] 	    = ( ( __points_dwValue & 0x00ff0000 ) >> 16 );
                inputMap [ ADDRESS_LEVEL + 2 ] 	    = ( ( __points_dwValue & 0x0000ff00 ) >> 8 );
                inputMap [ ADDRESS_LEVEL + 3 ] 	    = ( ( __points_dwValue & 0x000000ff ) >> 0 );
            }
        
            __plc_io_status = IO_PRESSURE;
        }
        break;
        }
#endif
        // 
        //
        // -------------------

    
        // -------------------
        //
        // PICKUP
        //
        updateVariable.type         = op_IR;
#if !defined(TE809ATS) && !defined(TE809SDMO)
		updateVariable.address	    = ADDRESS_MB_PICKUP;
        if ( __measure.pickup >= PICKUP_MAX_THRESHOLD )
            __measure.pickup = __measure_filter.pickup;
        if ( __measure.pickup == 0 )
            __measure_filter.pickup = 0;
        if ( __measure.pickup != 0 && __measure.pickup < PICKUP_MAX_THRESHOLD )
        {
            if ( __measure_filter.pickup == 0 )
                __measure_filter.pickup = __measure.pickup;
            __measure.pickup           = (u16) ( (float)( (float)__measure_filter.pickup * FILTER_OLD ) + (float)( (float)__measure.pickup * FILTER_NEW ) );
            __measure_filter.pickup    = __measure.pickup;
        }
        updateVariable.value		= __measure.pickup;
		XPLC_setMemory ( &updateVariable );
#endif
        //
        //
        // ---------------

    
        // ---------------
        //
        // VBAT
        //
        updateVariable.address	= ADDRESS_VBAT;
		updateVariable.value		= (float) ( (float) ( ( (u32)(__measure.vbat *  1000) / __calibration.k_BATT  ) / 10 ) );
		XPLC_setMemory ( &updateVariable );
        //
        //
        // ---------------
    
    
        // ---------------
        //
        // D PLUS
        //
#if !defined(TE809ATS) && !defined(TE809SDMO)
		updateVariable.address	    = ADDRESS_MB_DPLUS;
		updateVariable.value		= (float) ( (float) ( (u32)( __measure.dplus *  1000) / __calibration.k_DPLUS  ) / 100 );
		XPLC_setMemory ( &updateVariable );
#endif
        //
        //
        // ---------------
        
        
        // ---------------
        //
        // DIGITAL INPUTS
        //
        
        variable1.type          = op_MB;
        updateVariable.type           = op_I;
        updateVariable.address	    = ADDRESS_MB_DIN;
        
#if !defined(TE809SDMO)
        variable1.address       = ADDRESS_SETTINGS_DI1_TYPE;
        XPLC_getMemory ( &variable1 );
    
        updateVariable.bit	        = BIT_IN_1;
        switch ( (int)variable1.value )
        {
        case OFF:
            updateVariable.value		    = 0;
            break;
        case NO:
            updateVariable.value		    = (float) ( ( __measure.inputs & 0x01 ) >> BIT_IN_1 );
            break;
        case NC:
            updateVariable.value		    = (float) ( ! ( ( __measure.inputs & 0x01 ) >> BIT_IN_1 ) );
            break;
        }
        XPLC_setMemory ( &updateVariable );
        
        variable1.address       = ADDRESS_SETTINGS_DI2_TYPE;
        XPLC_getMemory ( &variable1 );
        updateVariable.bit	        = BIT_IN_2;
        switch ( (int)variable1.value )
        {
        case OFF:
            updateVariable.value		    = 0;
            break;
        case NO:
            updateVariable.value		    = (float) ( ( __measure.inputs & 0x02 ) >> BIT_IN_2 );
            break;
        case NC:
            updateVariable.value		    = (float) ( ! ( ( __measure.inputs & 0x02 ) >> BIT_IN_2 ) );
            break;
        }
        XPLC_setMemory ( &updateVariable );

        variable1.address       = ADDRESS_SETTINGS_DI3_TYPE;
        XPLC_getMemory ( &variable1 );
        updateVariable.bit	        = BIT_IN_3;
        switch ( (int)variable1.value )
        {
        case OFF:
            updateVariable.value		    = 0;
            break;
        case NO:
            updateVariable.value		    = (float) ( ( __measure.inputs & 0x04 ) >> BIT_IN_3 );
            break;
        case NC:
            updateVariable.value		    = (float) ( ! ( ( __measure.inputs & 0x04 ) >> BIT_IN_3 ) );
            break;
        }
        XPLC_setMemory ( &updateVariable );

        variable1.address       = ADDRESS_SETTINGS_DI4_TYPE;
        XPLC_getMemory ( &variable1 );
        updateVariable.bit	        = BIT_IN_4;
        switch ( (int)variable1.value )
        {
        case OFF:
            updateVariable.value		    = 0;
            break;
        case NO:
            updateVariable.value		    = (float) ( ( __measure.inputs & 0x08 ) >> BIT_IN_4 );
            break;
        case NC:
            updateVariable.value		    = (float) ( ! ( ( __measure.inputs & 0x08 ) >> BIT_IN_4 ) );
            break;
        }
        XPLC_setMemory ( &updateVariable );
#endif
    
        variable1.address       = ADDRESS_SETTINGS_DI5_TYPE;
        XPLC_getMemory ( &variable1 );
        updateVariable.bit	        = BIT_IN_5;
        switch ( (int)variable1.value )
        {
        case OFF:
            updateVariable.value		    = 0;
            break;
        case NO:
            updateVariable.value		    = (float) ( ( __measure.inputs & 0x10 ) >> BIT_IN_5 );
            break;
        case NC:
            updateVariable.value		    = (float) ( ! ( ( __measure.inputs & 0x10 ) >> BIT_IN_5 ) );
            break;
        }
        XPLC_setMemory ( &updateVariable );
        
        
#ifdef __HW_5_12
        if ( __isStarting == 0x00  )
        {
            //
            // lettura della ttensione applicata all'ingresso di emergenza
            // solo nel caso che non stiamo avviando il motore per evitare
            // attivazioni spurie docuti a cali di tensione della batteria
            variable1.address       = ADDRESS_SETTINGS_DIEM_TYPE;
            XPLC_getMemory ( &variable1 );
            updateVariable.bit	        = BIT_IN_EM;
            __EM = EM();
            updateVariable.value		    = 0;
            switch ( (int)variable1.value )
            {
            case NO:
                if ( __EM >= 800 )
                    updateVariable.value		    = 1;
                break;
            case NC:
                if ( __EM < 800 )
                    updateVariable.value		    = 1;
                break;
            }
            XPLC_setMemory ( &updateVariable );
        }
#endif
        //
        //
        // ---------------
        
            
        // ---------------
        //
        // PHASE SEQUENCE
        //
        if ( __isStarting == 0x00  )
        {
            updateVariable.type	        = op_I;
            updateVariable.address	    = ADDRESS_MB_SEQ_RG;

            updateVariable.value          = 1;
            updateVariable.bit	        = BIT_SEQ_R;
            if ( ( ( __measure.flags & 0x10 ) == 0x10  ) ||
                 ( ( __measure.flags & 0x20 ) == 0x20  ) )
            {
                if ( ( ( __measure.seq & 0x02 ) >> 1 ) == 0x01 )
                {
                    if ( __counterPhaseM < 10 ) 
                        __counterPhaseM++;
                    else                
                        updateVariable.value		    = 0x00;
                }
                else
                    __counterPhaseM = 0;
            }
            XPLC_setMemory ( &updateVariable );
        
            updateVariable.bit	        = BIT_SEQ_G;
            updateVariable.value          = 1;
            if ( ( ( __measure.flags & 0x04 ) == 0x04  ) ||
                 ( ( __measure.flags & 0x08 ) == 0x08  ) )
            {
                if ( ( ( __measure.seq & 0x01 ) >> 0 ) == 0x01 )
                {
                    if ( __counterPhaseG < 10 ) 
                        __counterPhaseG++;
                    else                
                        updateVariable.value		    = 0x00;
                }
                else
                    __counterPhaseG = 0;
            }
            XPLC_setMemory ( &updateVariable );
       }
                
        //
        //
        // ---------------
    }
}


#else





// ---------------------------------
//
//      TE 809 VDC    
//


//
// Update all the I/O of the power board according to the IO Tbales
void __updateIO_Power ( u8* inputMap, u16 dimensionIn, u8*outputMap, u16 dimensionOut ) 
{
    float               tmp1;    
    float               tmp2;    
    u8                  type;
    u8                  ret;
    u8                  unbalanced;
    XPLCVARIABLE		updateVariable;


    __measure.J6_in5    = TEMP ();
    __measure.J6_in6    = FUEL ();
    __measure.J5_in7    = EM();

    __measure.outs      = 0x00;
    
    //
    // Output 1
    variable1.type      = op_MB;
    variable1.address   = ADDRESS_SETTINGS_D01_TYPE;
    XPLC_getMemory ( &variable1 );
    switch ( (int)variable1.value )
    {
    case NO:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x01 ) == 0x01 ? 0x01 : 0x00 );
        break;
    case NC:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x01 ) == 0x01 ? 0x00 : 0x01 );
    }

    //
    // Output 2
    variable1.address   = ADDRESS_SETTINGS_D02_TYPE;
    XPLC_getMemory ( &variable1 );
    switch ( (int)variable1.value )
    {
    case NO:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x02 ) == 0x02 ? 0x02 : 0x00 );
        break;
    case NC:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x02 ) == 0x02 ? 0x00 : 0x02 );
    }

    //
    // Output 3
    variable1.address   = ADDRESS_SETTINGS_D03_TYPE;
    XPLC_getMemory ( &variable1 );
    switch ( (int)variable1.value )
    {
    case NO:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x04 ) == 0x04 ? 0x04 : 0x00 );
        break;
    case NC:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x04 ) == 0x04 ? 0x00 : 0x04 );
    }

    //
    // Output 4
    variable1.address   = ADDRESS_SETTINGS_D04_TYPE;
    XPLC_getMemory ( &variable1 );
    switch ( (int)variable1.value )
    {
    case NO:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x08 ) == 0x08 ? 0x08 : 0x00 );
        break;
    case NC:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x08 ) == 0x08 ? 0x00 : 0x08 );
    }

    //
    // Output 5
    variable1.address   = ADDRESS_SETTINGS_D05_TYPE;
    XPLC_getMemory ( &variable1 );
    switch ( (int)variable1.value )
    {
    case NO:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x10 ) == 0x10 ? 0x10 : 0x00 );
        break;
    case NC:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x10 ) == 0x10 ? 0x00 : 0x10 );
    }

    //
    // Output 6
    variable1.address   = ADDRESS_SETTINGS_D06_TYPE;
    XPLC_getMemory ( &variable1 );
    switch ( (int)variable1.value )
    {
    case NO:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x20 ) == 0x20 ? 0x20 : 0x00 );
        break;
    case NC:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x20 ) == 0x20 ? 0x00 : 0x20 );
    }

    //
    // Output 7
    variable1.address   = ADDRESS_SETTINGS_D07_TYPE;
    XPLC_getMemory ( &variable1 );
    switch ( (int)variable1.value )
    {
    case NO:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x40 ) == 0x40 ? 0x40 : 0x00 );
        break;
    case NC:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x40 ) == 0x40 ? 0x00 : 0x40 );
    }

    //
    // Output 8
    variable1.address   = ADDRESS_SETTINGS_D08_TYPE;
    XPLC_getMemory ( &variable1 );
    switch ( (int)variable1.value )
    {
    case NO:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x80 ) == 0x80 ? 0x80 : 0x00 );
        break;
    case NC:
        __measure.outs		    |= ( ( outputMap [ ADDRESS_OUT_MB ] & 0x80 ) == 0x80 ? 0x00 : 0x80 );
    }
    __measure_real.outs         = __measure.outs;

    //
    // EXCHANGE
    ret = UMISURE_exchange ( &__measure );

        
    if ( ret == 0x01 )
    {    
        // ---------------
        //
        // VBAT
        //
        updateVariable.type	    = op_IR;
        updateVariable.address	= ADDRESS_VBAT;
		updateVariable.value		= (float) ( (float) ( ( (u32)(__measure.vbat *  1000) / __calibration.k_BATT  ) / 10 ) );
        __measure_real.vbat = (u16)updateVariable.value;
		XPLC_setMemory ( &updateVariable );
        //
        //
        // ---------------
    
    
    
        // ---------------
        //
        // J1                
    
        //    
        // ANALOG1 --> 0..10V 
        updateVariable.address	        = ADDRESS_MB_VRG;
		updateVariable.value		        = (float) ( (float) ( ( (u32)(__measure.J1_analog1 * 100 ) / __calibration.alfaV1G  ) / 10 ) );
        __measure_real.j1_anlog1    = (u16)updateVariable.value;
        updateVariable.value		        /= 10; 
		XPLC_setMemory ( &updateVariable );
    
        //    
        // In1 --> 8..65V
        updateVariable.address	        = ADDRESS_MB_VSG;
		updateVariable.value		        = (float) ( (float) ( ( (u32)(__measure.J1_in1 * 1000 ) / __calibration.alfaV2G  ) / 10 ) );
        __measure_real.j1_in1    = (u16)updateVariable.value;
        updateVariable.value		        /= 10; 
		XPLC_setMemory ( &updateVariable );
        
        //    
        // In2 --> 8..65V
        updateVariable.address	        = ADDRESS_MB_VTG;
		updateVariable.value		        = (float) ( (float) ( ( (u32)(__measure.J1_in2 * 1000 ) / __calibration.alfaV3G  ) / 10 ) );
        __measure_real.j1_in2    = (u16)updateVariable.value;
        updateVariable.value		        /= 10; 
		XPLC_setMemory ( &updateVariable );
        
        //
        // ---------------    
        
        
        // ---------------
        //
        // J2                
        //
        // In1 --> 0..5V 
        updateVariable.address	    = ADDRESS_MB_VTR;
		updateVariable.value		    = (float) ( (float) ( ( (u32)(__measure.J2_in1 * 100 ) / __calibration.alfaV3R  ) / 10 ) );
        __measure_real.j2_in1   = (u16)updateVariable.value ;
        updateVariable.value		/= 10; 
		XPLC_setMemory ( &updateVariable );
    
        //
        // In2 --> 0..5V 
        updateVariable.address	    = ADDRESS_MB_VSR;
		updateVariable.value		    = (float) ( (float) ( ( (u32)(__measure.J2_in2 * 100 ) / __calibration.alfaV2R  ) / 10 ) );
        __measure_real.j2_in2   = (u16)updateVariable.value ;
        updateVariable.value		/= 10; 
		XPLC_setMemory ( &updateVariable );
        
        //
        // In3 --> 0..5V 
        updateVariable.address	    = ADDRESS_MB_VRR;
		updateVariable.value		    = (float) ( (float) ( ( (u32)(__measure.J2_in3 * 100 ) / __calibration.alfaV1R  ) / 10 ) );
        __measure_real.j2_in3   = (u16)updateVariable.value ;
        updateVariable.value		/= 10; 
		XPLC_setMemory ( &updateVariable );
        //
        // ---------------


        // ---------------
        //
        // J6                
    
        //    
        // In1 --> 0 ... 5V
        updateVariable.address	        = ADDRESS_PRESSURE;
		updateVariable.value		        = (float) ( (float) ( ( (u32)(__measure.J6_in4 * 100 ) / __calibration.k_OIL  ) / 10 ) );
        __measure_real.j6_in4       = (u16)updateVariable.value;
        updateVariable.value		        /= 10; 
		XPLC_setMemory ( &updateVariable );
    
        //    
        // In2 --> 0 ... 5V
        updateVariable.address	        = ADDRESS_TEMPERATURE;
		updateVariable.value		        = (float) ( (float) ( ( (u32)(__measure.J6_in5 * 100 ) / __calibration.k_TEMP  ) / 10 ) );
        __measure_real.j6_in5       = (u16)updateVariable.value;
        updateVariable.value		        /= 10; 
		XPLC_setMemory ( &updateVariable );
        
        //    
        // In3 --> 0 ... 5V
        updateVariable.address	        = ADDRESS_LEVEL;
		updateVariable.value		        = (float) ( (float) ( ( (u32)(__measure.J6_in6 * 100 ) / __calibration.k_LEVEL  ) / 10 ) );
        __measure_real.j6_in6       = (u16)updateVariable.value;
        updateVariable.value		        /= 10; 
		XPLC_setMemory ( &updateVariable );
        //
        // ---------------    
        
        
        
        // ---------------
        //
        // J3                
    
        //    
        // In1 --> 0 ... 5A
        updateVariable.address	        = ADDRESS_MB_IT;
		updateVariable.value		        = (float) ( (float) ( ( (u32)(__measure.J3_in1 * 100 ) / __calibration.AlfaI3  ) / 10 ) );
        __measure_real.j3_in1       = (u16)updateVariable.value;
        updateVariable.value		        /= 10; 
		XPLC_setMemory ( &updateVariable );
    
        //    
        // In2 --> 0 ... 5A
        updateVariable.address	        = ADDRESS_MB_IS;
		updateVariable.value		        = (float) ( (float) ( ( (u32)(__measure.J3_in2 * 100 ) / __calibration.AlfaI2  ) / 10 ) );
        __measure_real.j3_in2       = (u16)updateVariable.value;
        updateVariable.value		        /= 10; 
		XPLC_setMemory ( &updateVariable );
        
        //    
        // In3 --> 0 ... 5A
        updateVariable.address	        = ADDRESS_MB_IR;
		updateVariable.value		        = (float) ( (float) ( ( (u32)(__measure.J3_in3 * 100 ) / __calibration.AlfaI1  ) / 10 ) );
        __measure_real.j3_in3       = (u16)updateVariable.value;
        updateVariable.value		        /= 10; 
		XPLC_setMemory ( &updateVariable );
        //
        // ---------------    

        // ---------------
        //
        // J5               
        
        // 
        // In7 --> 0...10V        
        updateVariable.address	        = ADDRESS_MB_FR;
		updateVariable.value		        = (float) ( (float) ( ( (u32)(__measure.J5_in7 * 100 ) / __calibration.k_DPLUS  ) / 10 ) );
        __measure_real.j5_in7       = (u16)updateVariable.value;
        updateVariable.value		        /= 10; 
		XPLC_setMemory ( &updateVariable );
        //
        // ---------------    


        // ---------------
        //
        // DIGITAL INPUTS
        //
        
        variable1.type          = op_MB;
        updateVariable.type           = op_I;
        updateVariable.address	    = ADDRESS_MB_DIN;
        
        variable1.address       = ADDRESS_SETTINGS_DI1_TYPE;
        XPLC_getMemory ( &variable1 );
    
        __measure_real.inputs   = 0x00;
        updateVariable.bit	        = BIT_IN_1;
        switch ( (int)variable1.value )
        {
        case OFF:
            updateVariable.value		    = 0;
            break;
        case NO:
            updateVariable.value		    = (float) ( ( __measure.inputs & 0x01 ) >> BIT_IN_1 );
            if ( ( __measure.inputs & 0x01 ) == 0x01 )
                __measure_real.inputs   |= 0x01; 
            break;
        case NC:
            updateVariable.value		    = (float) ( ! ( ( __measure.inputs & 0x01 ) >> BIT_IN_1 ) );
            if ( ( __measure.inputs & 0x01 ) == 0x00 )
                __measure_real.inputs   |= 0x01; 
            break;
        }
        XPLC_setMemory ( &updateVariable );
        
        variable1.address       = ADDRESS_SETTINGS_DI2_TYPE;
        XPLC_getMemory ( &variable1 );
        updateVariable.bit	        = BIT_IN_2;
        switch ( (int)variable1.value )
        {
        case OFF:
            updateVariable.value		    = 0;
            break;
        case NO:
            updateVariable.value		    = (float) ( ( __measure.inputs & 0x02 ) >> BIT_IN_2 );
            if ( ( __measure.inputs & 0x02 ) == 0x02 )
                __measure_real.inputs   |= 0x02; 
            break;
        case NC:
            updateVariable.value		    = (float) ( ! ( ( __measure.inputs & 0x02 ) >> BIT_IN_2 ) );
            if ( ( __measure.inputs & 0x02 ) == 0x00 )
                __measure_real.inputs   |= 0x02; 
            break;
        }
        XPLC_setMemory ( &updateVariable );

        variable1.address       = ADDRESS_SETTINGS_DI3_TYPE;
        XPLC_getMemory ( &variable1 );
        updateVariable.bit	        = BIT_IN_3;
        switch ( (int)variable1.value )
        {
        case OFF:
            updateVariable.value		    = 0;
            break;
        case NO:
            updateVariable.value		    = (float) ( ( __measure.inputs & 0x04 ) >> BIT_IN_3 );
            if ( ( __measure.inputs & 0x04 ) == 0x04 )
                __measure_real.inputs   |= 0x04; 
            break;
        case NC:
            updateVariable.value		    = (float) ( ! ( ( __measure.inputs & 0x04 ) >> BIT_IN_3 ) );
            if ( ( __measure.inputs & 0x04 ) == 0x00 )
                __measure_real.inputs   |= 0x04; 
            break;
        }
        XPLC_setMemory ( &updateVariable );

        variable1.address       = ADDRESS_SETTINGS_DI4_TYPE;
        XPLC_getMemory ( &variable1 );
        updateVariable.bit	        = BIT_IN_4;
        switch ( (int)variable1.value )
        {
        case OFF:
            updateVariable.value		    = 0;
            break;
        case NO:
            updateVariable.value		    = (float) ( ( __measure.inputs & 0x08 ) >> BIT_IN_4 );
            if ( ( __measure.inputs & 0x08 ) == 0x08 )
                __measure_real.inputs   |= 0x08; 
            break;
        case NC:
            updateVariable.value		    = (float) ( ! ( ( __measure.inputs & 0x08 ) >> BIT_IN_4 ) );
            if ( ( __measure.inputs & 0x08 ) == 0x00 )
                __measure_real.inputs   |= 0x08; 
            break;
        }
        XPLC_setMemory ( &updateVariable );
    
        variable1.address       = ADDRESS_SETTINGS_DI5_TYPE;
        XPLC_getMemory ( &variable1 );
        updateVariable.bit	        = BIT_IN_5;
        switch ( (int)variable1.value )
        {
        case OFF:
            updateVariable.value		    = 0;
            break;
        case NO:
            updateVariable.value		    = (float) ( ( __measure.inputs & 0x10 ) >> BIT_IN_5 );
            if ( ( __measure.inputs & 0x10 ) == 0x10 )
                __measure_real.inputs   |= 0x10; 
            break;
        case NC:
            updateVariable.value		    = (float) ( ! ( ( __measure.inputs & 0x10 ) >> BIT_IN_5 ) );
            if ( ( __measure.inputs & 0x10 ) == 0x00 )
                __measure_real.inputs   |= 0x10; 
            break;
        }
        XPLC_setMemory ( &updateVariable );
        
        
        //
        //
        // ---------------

    }
}

#endif




//
//
void __reset_Power ( void )
{
}

