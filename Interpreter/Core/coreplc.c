

// -------------------------------------------------------------
// 
// Author ......... bMF
// Date ........... August 2011
// Description..... 
// Changes ........ 
// 
// -------------------------------------------------------------



#include "includes.h"
#include "coreplc.h"
#include "..\..\..\hw\hw.h"
#include "defines.h"
#include "..\..\..\hw\xflash.h"
#include "..\utilities\utilities.h"


typedef enum
{
	SR_ERROR			= 0,
	SR_OK				= 1,
	SR_LEAVE_STACK		= 2,
	SR_LEAVE_RESTART	= 3,
	SR_END_MAIN			= 4,
	SR_END_PROCEDURE	= 5
} STEP_RESULT;


//
// program counter
ADDRESS					_pc;
XPLCINFO				_info;
XPLCCALLBACKS		    _callbacks;

void					__performCycle ( void );
STEP_RESULT			    __performStep ( bool apply );
STEP_RESULT			    __performLoadStep ( bool apply, bool negate  );
STEP_RESULT             __perfromIsAlarmActive (  bool apply );
STEP_RESULT             __perfromVariableChange (  bool apply );
STEP_RESULT			    __performAndStep ( bool apply, bool AndNot );
STEP_RESULT				__performBracketStep ( bool apply, float* resultValue );
void					__performGoto ( bool apply );
void					__performCall ( bool apply );
u8						__performRestart ( bool apply );
void					__loadAccumulator ( float* accumulator );
void					__storeAccumulator ( bool apply, float accumulator );
void					__setAccumulator ( bool apply, bool set );
void					__toggleAccumulator ( bool apply );
STEP_RESULT			    __performIfThenElse ( COMPARECALLBACK compare, float accumulator1, float accumulator2, float accumulator3 );
STEP_RESULT			    __performCase ( float accumulator1, float accumulator2 );

//
// memory mapped definition
//u8			__PROGRAM	[ MAX_PROGRAM_SIZE ];
u8							__MEMORY	[ MAX_MEMORY_SIZE ];
u8							__EEPROM	[ MAX_EEPROM_SIZE ];
u8							__INPUT		[ MAX_INPUT_SIZE ];
u8							__OUTPUT	[ MAX_OUTPUT_SIZE ];
HANDLE_TIMER		        __TIMERS	[ MAX_TIMERS_SIZE ];
HANDLE_COUNTER	            __COUNTERS	[ MAX_COUNTERS_SIZE ];
HANDLE_PULSE	            __PULSES	[ MAX_PULSES_SIZE ];

u8							__FIRST_LOOP;
u8							__SECOND_TRIGGER;
u8							__IN_LOOP;
u8							__needResetFLB;
u8							__perform_hertz = 0;


//~ LOCATION __PROGRAM___ ( ADDRESS a )
//~ {
    //~ u16 page = __FLASH_PLC_START_ADDRESS + ( a / DLFLASH_PAGE_SIZE );

    //~ _info.locations++;
    //~ if ( page < _info.firstPage || page > _info.lastPage )
    //~ {        
        //~ _info.firstPage   = page;
        //~ _info.lastPage    = page + MAX_PLC_CACHE - 1;
//~ //        FLASH_lock ( );
        //~ while ( page <= _info.lastPage )
        //~ {
            //~ FLASH_pageRead ( page, _info.flash_pages [ page - _info.firstPage ]  );
            //~ page++;
            //~ _info.reloading++;
        //~ }
//~ //        FLASH_unlock ( );
        //~ page = _info.firstPage;
    //~ }    
    //~ return _info.flash_pages [ page - _info.firstPage  ] [ a % DLFLASH_PAGE_SIZE ];
//~ }

/*
inline LOCATION __PROGRAM ( ADDRESS a )
{
	return ( LOCATION ) ( FLASH_ReadWORD ( a ) & 0x00ff );
}
*/

LOCATION __PROGRAM ( ADDRESS a )
{
    _info.lastPage = __FLASH_PLC_START_ADDRESS + ( a / DLFLASH_PAGE_SIZE );
    if ( _info.lastPage != _info.firstPage )
    {        
        _info.firstPage   = _info.lastPage;
        FLASH_pageRead ( _info.lastPage, _info.flash_pages  );
    }
    return _info.flash_pages [ a % DLFLASH_PAGE_SIZE ];
}



//
// Start the PLC loop
void XPLC_start ()
{
	//
	// Set up the global variables entry points
	_pc								= 0;
	__FIRST_LOOP			        = 0x01;
	__IN_LOOP					    = 0x00;
	__needResetFLB		            = 0x00;

    //
    // Start the PLC
    _info.Running                   = 0x01;
}

//
// Stop the PLC loop
void XPLC_stop ()
{
	//
	// Start the PLC
	_info.Running = 0x00;
}


//
// Perform a cycle of the PLC
u8 XPLC_perform ( void )
{
	if ( _info.Running )
	{
		_info.Cycles++;
		__performCycle ();
		return 0x01;
	}
	return 0x00;
}


void XPLC_onHertz ()
{
	__perform_hertz++;
}


//
// Initialize all the PLC structures and data
void XPLC_initialize ( COPYIOCALLBACK updateInputOutput, 
                       FIRSTLOOPCALLBACK firstLoop, 
                       IS_ALARM_ACTIVE isAlarmActive,
                       VARIABLE_CHANGED variableChanged,
                       ON_STEP_PERFORMED onStepPerformed )
{
	int i;

	_callbacks.UpdateInputOutput	        = NULL;
	_callbacks.FirstLoop					= NULL;

	//
    // flush the PLC memory
	memset ( &__MEMORY, 0, MAX_MEMORY_SIZE );
	//
	// flush the PLC EEPROM
	memset ( &__EEPROM, 0, MAX_EEPROM_SIZE );

	//
	// initialize all the counters
	//
	// flush the PLC memory
	memset ( &__MEMORY, 0, MAX_MEMORY_SIZE );
	memset ( &__EEPROM, 0, MAX_EEPROM_SIZE );
	memset ( &__INPUT, 0, MAX_INPUT_SIZE );
	memset ( &__OUTPUT, 0, MAX_OUTPUT_SIZE );
    
    COUNTERS_Init ();
	PULSES_Init ();

	for ( i = 0; i < MAX_TIMERS_SIZE; i++ )
		__TIMERS[i] = HANDLE_TIMER_NULL;
	for ( i = 0; i < MAX_COUNTERS_SIZE; i++ )
		__COUNTERS[i] = HANDLE_COUNTER_NULL;
	for ( i = 0; i < MAX_PULSES_SIZE; i++ )
		__PULSES[i] = HANDLE_PULSE_NULL;

	//
	// initialize the program counter and the accumulator
	_pc						= 0;

	//
	// The PLC is not running
	_info.Running	= 0x00;
	_info.firstPage	= 0xffff;
	_info.lastPage	= 0;

	//
	// initialize the callbacks
	_callbacks.UpdateInputOutput	        = updateInputOutput;
	_callbacks.FirstLoop					= firstLoop;
    _callbacks.IsAlarmActive                = isAlarmActive;
    _callbacks.VariableChanged              = variableChanged;
    _callbacks.onStepPerformed              = onStepPerformed;


	//
	// Initialize information structure
	_info.Running	= 0x00;
	_info.Cycles	= 0;
    _info.OldCycles = 0;
}



void XPLC_resetFLB ( void )
{
    __needResetFLB = 0x01;
}


//
// Perform a single PLC cycle
void __performCycle ( void )
{
	u8		loop;

	if ( __IN_LOOP ) return;
	
	__IN_LOOP				= 0x01;
	
	//
	// Check if the first loop bit must be resetted
	if ( __needResetFLB )
	{
		__needResetFLB		= 0x00;
		__FIRST_LOOP		= 0x01;
	}

	if ( __perform_hertz > 0 )
	{
		__perform_hertz 	= 0;
		__SECOND_TRIGGER 	= 0x01;
	}

	loop = 0x01;

	//
	// let the application known that the first loop is done
	if ( __FIRST_LOOP )
    {
        if ( _callbacks.FirstLoop != NULL )
            _callbacks.FirstLoop ();
    }

	while ( loop  )
	{
		switch ( __performStep ( TRUE ) )
		{
		case SR_ERROR:
		case SR_LEAVE_RESTART:
		case SR_END_MAIN:
			loop = 0x00;
			break;
		}
        _callbacks.onStepPerformed ( );
	}
//    _info.pagesLoaded = _info.reloading;
		
    //
    // Skip the first loop io update
    _callbacks.UpdateInputOutput ( __INPUT, MAX_INPUT_SIZE, __OUTPUT, MAX_OUTPUT_SIZE );

	_pc						= 0;
	__FIRST_LOOP			= 0x00;
	__IN_LOOP				= 0x00;
	__SECOND_TRIGGER	    = 0x00;
}



//
// Perrform a single line step
STEP_RESULT __performStep ( bool apply )
{
	STEP_RESULT result = SR_ERROR;

	LOCATION loc = __PROGRAM ( _pc++ );
	//
	//
	switch ( loc )
	{
	case op_LOAD:
		result = __performLoadStep ( apply, 0x00 );
		break;
    
	case op_LOADN:
		result = __performLoadStep ( apply, 0x01 );
		break;

	case op_IS_ALARM_ACTIVE:
		result = __perfromIsAlarmActive ( apply );
		break;
    
	case op_VARIABLECHANGE:
		result = __perfromVariableChange ( apply );
		break;

	case op_AND:
		result = __performAndStep ( apply, FALSE );
		break;

	case op_ANDNOT:
		result = __performAndStep ( apply, TRUE );
		break;

	case op_OPB:
		result = __performBracketStep ( apply, NULL );
		break;

	case op_NOP:
		result = SR_OK;
		break;

	case op_SET:
		__setAccumulator ( apply, TRUE );
		result = SR_OK;
		break;

	case op_TOGGLE:
		__toggleAccumulator ( apply  );
		result = SR_OK;
		break;

	case op_RESET:
		__setAccumulator ( apply, FALSE );
		result = SR_OK;
		break;

	case op_GOTO:
		result = SR_OK;
		__performGoto ( apply );
		if ( apply ) 
			result = SR_LEAVE_STACK;
		break;

	case op_CALL:
		result = SR_OK;
		__performCall ( apply );
		break;

	case op_RESTART:
		result = SR_OK;
		if ( __performRestart ( apply ) )
			result = SR_LEAVE_RESTART;
		break;

	case op_MAIN:
		result      = SR_END_MAIN;
		break;
    
	case op_PROCEDURE:
		result = SR_END_PROCEDURE;
		break;
	}
	return result;
}




//
// load the parameter into the accumulator
void __loadAccumulator ( float* accumulator )
{
	u8				opType;
	u8				byte0;
	u8				byte1;
	u8				byte2;
	u8				byte3;
	int				memory;

	opType	= __PROGRAM(_pc++);
	
	if ( opType == op_FIRSTLOOPBIT )
	{
		*accumulator = __FIRST_LOOP;
		return;
	}
	if ( opType == op_SECONDTRIGGERBIT )
	{
		*accumulator = __SECOND_TRIGGER;
		return;
	}

	byte0	= __PROGRAM( _pc++ );
	byte1	= __PROGRAM( _pc++ );
	byte2	= __PROGRAM( _pc++ );
	byte3	= __PROGRAM( _pc++ );

	switch ( opType )
	{
	case op_M:
		memory = ( byte1 << 8 ) | byte2;
		if ( memory >= 0 && memory < MAX_MEMORY_SIZE )
			*accumulator = (float)( __MEMORY [ memory ] & ( 1 << byte3 ) ? 0x01 : 0x00 );
		break;

	case op_MB:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_MEMORY_SIZE  )
			*accumulator = __MEMORY [ memory ];
		break;

	case op_MW:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_MEMORY_SIZE - 2 )
			*accumulator = (float) ( ( __MEMORY [ memory ] << 8 ) | 
									 ( __MEMORY [ memory + 1 ]  ) );
		break;

	case op_MD:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_MEMORY_SIZE - 4 )
			*accumulator = (float) ( ( __MEMORY [ memory  ] << 24 ) | 
									 ( __MEMORY [ memory + 1 ] << 16  ) |
									 ( __MEMORY [ memory + 2 ] << 8  ) |
									 ( __MEMORY [ memory + 3 ] ) );
		break;

	case op_MR:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_MEMORY_SIZE - 4 )
			__getFloat4 ( __MEMORY [ memory ],
									  __MEMORY [ memory + 1 ],
									  __MEMORY [ memory + 2 ],
									  __MEMORY [ memory + 3 ],
										accumulator );
		break;

	case op_E:
		memory = ( byte1 << 8 ) | byte2;
		if ( memory >= 0 && memory < MAX_EEPROM_SIZE )
			*accumulator = (float) ( __EEPROM [ memory ] & ( 1 << byte3 ) ? 0x01 : 0x00 );
		break;

	case op_EB:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_EEPROM_SIZE  )
			*accumulator = (float)(  __EEPROM [ memory  ] );
		break;

	case op_EW:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_EEPROM_SIZE - 1 )
			*accumulator = (float)( ( __EEPROM [ memory  ] << 8 ) | 
									( __EEPROM [ memory + 1 ]  ) );
		break;

	case op_ED:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_EEPROM_SIZE - 3 )
			*accumulator = (float) ( ( __EEPROM [ memory  ] << 24 ) | 
									 ( __EEPROM [ memory + 1 ] << 16  ) |
									 ( __EEPROM [ memory + 2 ] << 8  ) |
									 ( __EEPROM [ memory + 3 ]   ) );
		break;

	case op_I:
		memory = ( byte1 << 8 ) | byte2;
		if ( memory >= 0 && memory < MAX_INPUT_SIZE )
			*accumulator = (float)( __INPUT [ memory ] & ( 1 << byte3 ) ? 0x01 : 0x00 );
		break;

	case op_IB:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_INPUT_SIZE  )
			*accumulator = (float)(  __INPUT [ memory  ] );
		break;
        
	case op_IW:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_INPUT_SIZE - 1 )
			*accumulator = (float) ( ( __INPUT [ memory ] << 8 ) | 
									 ( __INPUT [ memory + 1 ]  ) );
		break;

	case op_IR:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_INPUT_SIZE - 4 )
			__getFloat4 ( __INPUT [ memory ],
							__INPUT [ memory + 1 ],
							__INPUT [ memory + 2 ],
							__INPUT [ memory + 3 ],
							accumulator );
		break;

	case op_U:
		memory = ( byte1 << 8 ) | byte2;
		if ( memory >= 0 && memory < MAX_OUTPUT_SIZE )
			*accumulator = (float) ( __OUTPUT [ memory ] & ( 1 << byte3 ) ? 0x01 : 0x00 );
		break;
	case op_UB:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_OUTPUT_SIZE  )
			*accumulator = (float)(  __OUTPUT [ memory  ] );
		break;
        
	case op_UW:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_OUTPUT_SIZE - 1 )
			*accumulator = (float) ( ( __OUTPUT [ memory ] << 8 ) | 
									 ( __OUTPUT [ memory + 1 ]  ) );
		break;

	case op_UR:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_OUTPUT_SIZE - 4 )
			__getFloat4 ( __OUTPUT [ memory ],
						  __OUTPUT [ memory + 1 ],
						  __OUTPUT [ memory + 2 ],
						  __OUTPUT [ memory + 3 ],
						  accumulator );
		break;
        

	case op_C:
	case op_LABEL:
		*accumulator = (float) ( ( byte0 << 24 ) | ( byte1 << 16 ) | ( byte2 << 8 ) | byte3 );
		break;

	case op_CR:
		__getFloat4 ( byte0, byte1, byte2, byte3, accumulator );
		break;

	case op_B:
		*accumulator = byte3;
		break;

	case op_TON:
	case op_TONS:
        *accumulator = 0;
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_TIMERS_SIZE )
		{
			if ( __TIMERS [ memory ] != HANDLE_TIMER_NULL )
				*accumulator = (float) ( TIMERS_getQ (  __TIMERS [ memory ] ) );
		}
		break;

	case op_REP:
	case op_FEP:
		*accumulator = 0;
        memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_PULSES_SIZE )
		{
			if ( __PULSES [ memory ] != HANDLE_PULSE_NULL )
				*accumulator = (float) ( PULSES_getQ (  __PULSES [ memory ] ) );
		}
		break;

	case op_C_RE:
	case op_C_FE:
	case op_C_HL:
	case op_C_LL:
	case op_CD_RE:
	case op_CD_HL:
        *accumulator = 0;
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_COUNTERS_SIZE )
		{
			if ( __COUNTERS [ memory ] != HANDLE_COUNTER_NULL )
				*accumulator = (float) COUNTERS_getCount (  __COUNTERS [ memory ] );
		}
		break;

	case op_UNISSIGNED:
		*accumulator = (float)0xffffffff;
		break;

	}
}

//
// Store the value of the accumulator into 
void __storeAccumulator ( bool apply, float accumulator )
{
	u8			opType;
	u8			byte0;
	u8			byte1;
	u8			byte2;
	u8			byte3;
	u8			mask;
	int			memory;

	opType	    = __PROGRAM ( _pc++ );
	byte0		= __PROGRAM ( _pc++ );
	byte1		= __PROGRAM ( _pc++ );
	byte2		= __PROGRAM ( _pc++ );
	byte3		= __PROGRAM ( _pc++ );
	
	if ( !apply ) return;

	switch ( opType )
	{
	case op_M:
		mask	= 1 << byte3;
		memory	= ( byte1 << 8 ) | byte2;
		if ( memory >= 0 && memory < MAX_MEMORY_SIZE )
		{
			if ( accumulator == 1 )
				__MEMORY [ memory ] |= mask;
			else
			{
				mask = ~mask;
				__MEMORY [ memory ] &= mask;
			}
		}
		break;

	case op_MB:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_MEMORY_SIZE  )
			__MEMORY [ memory ]			= (u8)( ( ( (long)accumulator ) & 0x000000FF ) );
		break;

	case op_MW:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_MEMORY_SIZE - 1 )
		{
			__MEMORY [ memory ]			= (u8)( ( ( (long)accumulator ) & 0x0000FF00 ) >> 8 );
			__MEMORY [ memory + 1 ]		= (u8)( ( ( (long)accumulator ) & 0x000000FF ) >> 0 );
		}
		break;

	case op_MD:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_MEMORY_SIZE - 3 )
		{
			__MEMORY [ memory ]				= (u8)( ( ( (long)accumulator ) & 0xFF000000 ) >> 24 );
			__MEMORY [ memory + 1 ]			= (u8)( ( ( (long)accumulator ) & 0x00FF0000 ) >> 16 );
			__MEMORY [ memory + 2 ]			= (u8)( ( ( (long)accumulator ) & 0x0000FF00 ) >> 8 );
			__MEMORY [ memory + 3 ]			= (u8)( ( ( (long)accumulator ) & 0x000000FF ) >> 0 );
		}
		break;

	case op_MR:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_MEMORY_SIZE - 3 )
            {
            extern u32  			___utilities_binary;
            extern float			___utilities_value;
            ___utilities_value 		= accumulator;
            ___utilities_binary		= *( (u32*)( &___utilities_value ) );
            __MEMORY [ memory ]	= (u8) ( ( ___utilities_binary & 0xff000000 ) >> 24 );
            __MEMORY [ memory + 1]	= (u8) ( ( ___utilities_binary & 0x00ff0000 ) >> 16 );
            __MEMORY [ memory + 2]	= (u8) ( ( ___utilities_binary & 0x0000ff00 ) >> 8 );
            __MEMORY [ memory + 3]	= (u8) ( ( ___utilities_binary & 0x000000ff ) >> 0 );
//			__setFloat4 ( &__MEMORY [ memory ], accumulator );
            }
		break;

	case op_E:
		mask	= 1 << byte3;
		memory	= ( byte1 << 8 ) | byte2;
		if ( memory >= 0 && memory < MAX_EEPROM_SIZE )
		{
			if ( accumulator == 1 )
				__EEPROM [ memory ] |= mask;
			else
			{
				mask = ~mask;
				__EEPROM [ memory ] &= mask;
			}
		}
		break;

	case op_EB:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_EEPROM_SIZE   )
			__EEPROM [ memory  ]			= (u8)( ( ( (long)accumulator ) & 0x000000FF ) >> 0 );
		break;

	case op_EW:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_EEPROM_SIZE  - 1 )
		{
			__EEPROM [ memory  ]			= (u8)( ( ( (long)accumulator ) & 0x0000FF00 ) >> 8 );
			__EEPROM [ memory + 1 ]			= (u8)( ( ( (long)accumulator ) & 0x000000FF ) >> 0 );
		}
		break;

	case op_ED:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_EEPROM_SIZE  - 3 )
		{
			__EEPROM [ memory ]				= (u8)( ( ( (long)accumulator ) & 0xFF000000 ) >> 24 );
			__EEPROM [ memory + 1 ]			= (u8)( ( ( (long)accumulator ) & 0x00FF0000 ) >> 16 );
			__EEPROM [ memory + 2 ]			= (u8)( ( ( (long)accumulator ) & 0x0000FF00 ) >> 8 );
			__EEPROM [ memory + 3 ]			= (u8)( ( ( (long)accumulator ) & 0x000000FF ) >> 0 );
		}
		break;

	case op_U:
		mask	= 1 << byte3;
		memory	= ( byte1 << 8 ) | byte2;
		if ( memory >= 0 && memory < MAX_OUTPUT_SIZE )
		{
			if ( accumulator == 1 )
				__OUTPUT [ memory ] |= mask;
			else
			{
				mask = ~mask;
				__OUTPUT [ memory ] &= mask;
			}
		}
		break;
	case op_UB:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_OUTPUT_SIZE  )
			__OUTPUT [ memory ]			= (u8)( ( ( (long)accumulator ) & 0x000000FF ) );
		break;

	case op_UW:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_OUTPUT_SIZE - 1 )
		{
			__OUTPUT [ memory ]			= (u8)( ( ( (long)accumulator ) & 0x0000FF00 ) >> 8 );
			__OUTPUT [ memory + 1 ]		= (u8)( ( ( (long)accumulator ) & 0x000000FF ) >> 0 );
		}
		break;
	case op_UR:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_OUTPUT_SIZE - 3 )
			__setFloat4 ( &__OUTPUT [ memory ], accumulator );
		break;

	case op_TON:
	case op_TONS:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_TIMERS_SIZE )
		{
			if ( __TIMERS [ memory ] == HANDLE_TIMER_NULL )
				__TIMERS [ memory ] = TIMERS_createTimer ( );
			TIMERS_setCount (  __TIMERS [ memory ], (int)( opType == op_TON ? accumulator : accumulator * TICKS_PER_SECOND ) / SYSTEM_TICK );
		}
		break;

	case op_REP:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_PULSES_SIZE )
		{
			if ( __PULSES [ memory ] == HANDLE_PULSE_NULL )
			{
				__PULSES [ memory ] = PULSES_createPulse ( );
				PULSES_setType ( __PULSES [ memory ], PT_RAISE_EDGE );
			}
			PULSES_setStatus ( __PULSES [ memory ], accumulator );
		}
		break;

	case op_FEP:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_PULSES_SIZE )
		{
			if ( __PULSES [ memory ] == HANDLE_PULSE_NULL )
			{
				__PULSES [ memory ] = PULSES_createPulse ( );
				PULSES_setType ( __PULSES [ memory ], PT_FALLING_EDGE );
			}
			PULSES_setStatus ( __PULSES [ memory ], accumulator );
		}
		break;

	case op_C_RE:
	case op_C_FE:
	case op_C_HL:
	case op_C_LL:
	case op_CD_RE:
	case op_CD_HL:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_COUNTERS_SIZE )
		{
			if ( __COUNTERS [ memory ] == HANDLE_COUNTER_NULL )
				__COUNTERS [ memory ] = COUNTERS_createCounter ( );	
			switch ( opType )
			{
				case op_C_RE:
					COUNTERS_setType ( __COUNTERS [ memory ], CT_RAISE_EDGE );
					break;
				case op_C_FE:
					COUNTERS_setType ( __COUNTERS [ memory ], CT_FALLING_EDGE );
					break;
				case op_C_HL:
					COUNTERS_setType ( __COUNTERS [ memory ], CT_HIGH_LEVEL );
					break;
				case op_C_LL:
					COUNTERS_setType ( __COUNTERS [ memory ], CT_LOW_LEVEL );
					break;
				case op_CD_RE:
					COUNTERS_setType ( __COUNTERS [ memory ], CTD_RAISE_EDGE );
					break;
				case op_CD_HL:
					COUNTERS_setType ( __COUNTERS [ memory ], CTD_HIGH_LEVEL );
					break;
			}
			COUNTERS_setCount (  __COUNTERS [ memory ], (u32)accumulator );
		}
		break;

	case op_UNISSIGNED:
		break;

	}
}

//
// Set reset the target in order to the accumulator value
void __setAccumulator ( bool apply, bool set )
{
	u8	opType;
	u8	byte0;
	u8	byte1;
	u8	byte2;
	u8	byte3;
	u8	mask;
	int				memory;

	opType	= __PROGRAM ( _pc++ );
	byte0	= __PROGRAM ( _pc++ );
	byte1	= __PROGRAM ( _pc++ );
	byte2	= __PROGRAM ( _pc++ );
	byte3	= __PROGRAM ( _pc++ );

	if ( !apply ) return;

	switch ( opType )
	{
	case op_M:
		mask	= 1 << byte3;
		memory	= ( byte1 << 8 ) | byte2;
		if ( memory >= 0 && memory < MAX_MEMORY_SIZE )
		{
			if ( set == 1 )
				__MEMORY [ memory ] |= mask;
			else
			{
				mask = ~mask;
				__MEMORY [ memory ] &= mask;
			}
		}
		break;

	case op_E:
		mask	= 1 << byte3;
		memory	= ( byte1 << 8 ) | byte2;
		if ( memory >= 0 && memory < MAX_EEPROM_SIZE )
		{
			if ( set == 1 )
				__EEPROM [ memory ] |= mask;
			else
			{
				mask = ~mask;
				__EEPROM [ memory ] &= mask;
			}
		}
		break;

	case op_U:
		mask	= 1 << byte3;
		memory	= ( byte1 << 8 ) | byte2;
		if ( memory >= 0 && memory < MAX_OUTPUT_SIZE )
		{
			if ( set == 1 )
				__OUTPUT [ memory ] |= mask;
			else
			{
				mask = ~mask;
				__OUTPUT [ memory ] &= mask;
			}
		}
		break;

	case op_TON:
	case op_TONS:
		memory = ( byte2 << 8 ) | byte3;
		if ( memory >= 0 && memory < MAX_TIMERS_SIZE )
		{
			if ( __TIMERS [ memory ] != HANDLE_TIMER_NULL )
				TIMERS_setEnable (  __TIMERS [ memory ], set );
		}
		break;

	case op_C_RE:
	case op_C_FE:
	case op_C_HL:
	case op_C_LL:
	case op_CD_RE:
	case op_CD_HL:
		memory = ( byte2 << 8 ) | byte3;
		if ( __COUNTERS [ memory ] == HANDLE_COUNTER_NULL )
		{
			__COUNTERS [ memory ] = COUNTERS_createCounter ( );
			switch ( opType )
			{
			case op_C_RE:
				COUNTERS_setType ( __COUNTERS [ memory ], CT_RAISE_EDGE );
				break;
			case op_C_FE:
				COUNTERS_setType ( __COUNTERS [ memory ], CT_FALLING_EDGE );
				break;
			case op_C_HL:
				COUNTERS_setType ( __COUNTERS [ memory ], CT_HIGH_LEVEL );
				break;
			case op_C_LL:
				COUNTERS_setType ( __COUNTERS [ memory ], CT_LOW_LEVEL );
				break;
			case op_CD_RE:
				COUNTERS_setType ( __COUNTERS [ memory ], CTD_RAISE_EDGE );
				break;
			case op_CD_HL:
				COUNTERS_setType ( __COUNTERS [ memory ], CTD_HIGH_LEVEL );
				break;
			}
		}
		COUNTERS_setClock (  __COUNTERS [ memory ], set );
		break;

	case op_UNISSIGNED:
		break;
	}
}

STEP_RESULT __perfromVariableChange (  bool apply )
{
	float			accumulator1;

	__loadAccumulator ( &accumulator1 );
    if ( apply )
        if ( _callbacks.VariableChanged != NULL )
            _callbacks.VariableChanged ( (int)accumulator1 );
    return SR_OK;
}

STEP_RESULT __perfromIsAlarmActive (  bool apply )
{
	float			accumulator1;
	float			accumulator2;
	float			accumulator3;

	__loadAccumulator ( &accumulator1 );
    if ( _callbacks.IsAlarmActive != NULL )
    {
        accumulator1 = _callbacks.IsAlarmActive ( accumulator1 );
        while ( __PROGRAM ( _pc ) != op_SET		&& 
                __PROGRAM ( _pc ) != op_RESET	&& 
                __PROGRAM ( _pc ) != op_STORE	&&
                __PROGRAM ( _pc ) != op_NOP
              )
        {
            switch ( __PROGRAM ( _pc++ ) )
            {

            case op_EQ:
                __loadAccumulator ( &accumulator2 );
                if ( __performIfThenElse ( __EQ, accumulator1, accumulator2, 0 ) == SR_LEAVE_STACK  )
                    return SR_OK;				
                break;

            case op_NEQ:
                __loadAccumulator ( &accumulator2 );
                if ( __performIfThenElse ( __NEQ, accumulator1, accumulator2, 0  ) == SR_LEAVE_STACK  )
                    return SR_OK;				
                break;

            case op_GE:
                __loadAccumulator ( &accumulator2 );
                if ( __performIfThenElse ( __GE, accumulator1, accumulator2, 0  ) == SR_LEAVE_STACK  )
                    return SR_OK;				
                break;

            case op_GT:
                __loadAccumulator ( &accumulator2 );
                if ( __performIfThenElse ( __GT, accumulator1, accumulator2, 0  ) == SR_LEAVE_STACK  )
                    return SR_OK;				
                break;

            case op_LE:
                __loadAccumulator ( &accumulator2 );
                if ( __performIfThenElse ( __LE, accumulator1, accumulator2, 0 ) == SR_LEAVE_STACK  )
                    return SR_OK;				
                break;

            case op_LT:
                __loadAccumulator ( &accumulator2 );
                if ( __performIfThenElse ( __LT, accumulator1, accumulator2, 0 ) == SR_LEAVE_STACK  )
                    return SR_OK;				
                break;

            case op_IN:
                __loadAccumulator ( &accumulator2 );
                __loadAccumulator ( &accumulator3 );
                if ( __performIfThenElse ( __IN, accumulator1, accumulator2, accumulator3 ) == SR_LEAVE_STACK  )
                    return SR_OK;				
                break;

            case op_NIN:
                __loadAccumulator ( &accumulator2 );
                __loadAccumulator ( &accumulator3 );
                if ( __performIfThenElse ( __NIN, accumulator1, accumulator2, accumulator3 ) == SR_LEAVE_STACK  )
                    return SR_OK;				
                break;

           }
        }

        STEP_RESULT result = SR_OK;
        switch ( __PROGRAM ( _pc++ ) )
        {
        case op_SET:
        case op_RESET:
            __setAccumulator ( apply, accumulator1 == 1 );
            break;
        case op_STORE:
            __storeAccumulator ( apply, accumulator1 );
            break;
        case op_NOP:
            break;
        case op_GOTO:
            __performGoto ( apply );
            break;
        case op_CALL:
            __performCall ( apply );
            break;
        case op_RESTART:
            if ( __performRestart ( apply ) )
                result = SR_LEAVE_RESTART;
            break;
        }
        return result;
    }
    return SR_OK;   
}

//
// Set reset the target in order to the accumulator value
void __toggleAccumulator ( bool apply  )
{
	u8	        opType;
	u8	        byte0;
	u8	        byte1;
	u8	        byte2;
	u8	        byte3;
	u8	        mask;
	int			memory;

	opType	= __PROGRAM ( _pc++ );
	byte0	= __PROGRAM ( _pc++ );
	byte1	= __PROGRAM ( _pc++ );
	byte2	= __PROGRAM ( _pc++ );
	byte3	= __PROGRAM ( _pc++ );

	if ( !apply ) return;

	switch ( opType )
	{
	case op_M:
		mask	= 1 << byte3;
		memory	= ( byte1 << 8 ) | byte2;
		if ( memory >= 0 && memory < MAX_MEMORY_SIZE )
		{
			if ( !( __MEMORY [ memory ] & mask ) )
				__MEMORY [ memory ] |= mask;
			else
			{
				mask = ~mask;
				__MEMORY [ memory ] &= mask;
			}
		}
		break;

	case op_E:
		mask	= 1 << byte3;
		memory	= ( byte1 << 8 ) | byte2;
		if ( memory >= 0 && memory < MAX_EEPROM_SIZE )
		{
			if ( ! ( __EEPROM [ memory ] & mask ) )
				__EEPROM [ memory ] |= mask;
			else
			{
				mask = ~mask;
				__EEPROM [ memory ] &= mask;
			}
		}
		break;

	case op_U:
		mask	= 1 << byte3;
		memory	= ( byte1 << 8 ) | byte2;
		if ( memory >= 0 && memory < MAX_OUTPUT_SIZE )
		{
			if ( ! ( __OUTPUT [ memory ] & mask ) )
				__OUTPUT [ memory ] |= mask;
			else
			{
				mask = ~mask;
				__OUTPUT [ memory ] &= mask;
			}
		}
		break;

	case op_UNISSIGNED:
		break;
	}
}


//
// Perform a single LOAD step
STEP_RESULT __performLoadStep (  bool apply, bool negate )
{
	float			accumulator1;
	float			accumulator2;
	float			accumulator3;

	__loadAccumulator ( &accumulator1 );
    if ( negate )
		accumulator1 = !accumulator1;

	while ( __PROGRAM ( _pc ) != op_SET		&& 
			__PROGRAM ( _pc ) != op_RESET	&& 
			__PROGRAM ( _pc ) != op_STORE	&&
			__PROGRAM ( _pc ) != op_NOP		&&
			__PROGRAM ( _pc ) != op_TOGGLE
		  )
	{
		switch ( __PROGRAM ( _pc++ ) )
		{
		case op_AND:
			__loadAccumulator ( &accumulator2 );
			if ( accumulator1 != 0 && accumulator2 != 0 )
				accumulator1 = 1;
			else
				accumulator1 = 0;
			break;

		case op_ANDNOT:
			__loadAccumulator ( &accumulator2 );
			if ( accumulator1 != 0 && accumulator2 == 0 )
				accumulator1 = 1;
			else
				accumulator1 = 0;
			break;

		case op_OR:
			__loadAccumulator ( &accumulator2 );
			if ( accumulator1 == 0 && accumulator2 == 0 )
				accumulator1 = 0;
			else
				accumulator1 = 1;
			break;

		case op_ORNOT:
			__loadAccumulator ( &accumulator2 );
			if ( accumulator1 == 0 && accumulator2 != 0 )
				accumulator1 = 0;
			else
				accumulator1 = 1;
			break;

		case op_ADD:
			__loadAccumulator ( &accumulator2 );
			accumulator1 += accumulator2;
			break;

		case op_SUB:
			__loadAccumulator ( &accumulator2 );
			accumulator1 -= accumulator2;
			break;

		case op_MUL:
            {
                __loadAccumulator ( &accumulator2 );
                accumulator1 *= accumulator2;
            }
			break;

		case op_DIV:
            {
                __loadAccumulator ( &accumulator2 );
                accumulator1 = accumulator1 / accumulator2;
            }
			break;

		case op_EQ:
			__loadAccumulator ( &accumulator2 );
			if ( __performIfThenElse ( __EQ, accumulator1, accumulator2, 0 ) == SR_LEAVE_STACK  )
				return SR_OK;				
			break;

		case op_NEQ:
			__loadAccumulator ( &accumulator2 );
			if ( __performIfThenElse ( __NEQ, accumulator1, accumulator2, 0  ) == SR_LEAVE_STACK  )
				return SR_OK;				
			break;

		case op_GE:
			__loadAccumulator ( &accumulator2 );
			if ( __performIfThenElse ( __GE, accumulator1, accumulator2, 0  ) == SR_LEAVE_STACK  )
				return SR_OK;				
			break;

		case op_GT:
			__loadAccumulator ( &accumulator2 );
			if ( __performIfThenElse ( __GT, accumulator1, accumulator2, 0  ) == SR_LEAVE_STACK  )
				return SR_OK;				
			break;

		case op_LE:
			__loadAccumulator ( &accumulator2 );
			if ( __performIfThenElse ( __LE, accumulator1, accumulator2, 0 ) == SR_LEAVE_STACK  )
				return SR_OK;				
			break;

		case op_LT:
			__loadAccumulator ( &accumulator2 );
			if ( __performIfThenElse ( __LT, accumulator1, accumulator2, 0 ) == SR_LEAVE_STACK  )
				return SR_OK;				
			break;

		case op_IN:
			__loadAccumulator ( &accumulator2 );
			__loadAccumulator ( &accumulator3 );
			if ( __performIfThenElse ( __IN, accumulator1, accumulator2, accumulator3 ) == SR_LEAVE_STACK  )
				return SR_OK;				
			break;

		case op_NIN:
			__loadAccumulator ( &accumulator2 );
			__loadAccumulator ( &accumulator3 );
			if ( __performIfThenElse ( __NIN, accumulator1, accumulator2, accumulator3 ) == SR_LEAVE_STACK  )
				return SR_OK;				
			break;

		case op_SWITCH:
			while ( __PROGRAM ( _pc ) != op_SWITCH ) 
			{
				__loadAccumulator ( &accumulator2 );
				if ( __performCase ( accumulator1, accumulator2 ) == SR_LEAVE_STACK )
					return SR_OK;
			}
			_pc++;
			break;
       }
	}

	STEP_RESULT result = SR_OK;
	switch ( __PROGRAM ( _pc++ ) )
	{
	case op_SET:
	case op_RESET:
		__setAccumulator ( apply, accumulator1 == 1 );
		break;
	case op_TOGGLE:
		__toggleAccumulator ( apply  );
		break;
	case op_STORE:
		__storeAccumulator ( apply, accumulator1 );
		break;
	case op_NOP:
		break;
	case op_GOTO:
		__performGoto ( apply );        
		break;
	case op_CALL:
		__performCall ( apply );
		break;
	case op_RESTART:
		if ( __performRestart ( apply ) )
			result = SR_LEAVE_RESTART;
		break;
	}
	return result;
}


float __performLogicAccumulator ( bool AndNot )
{
	float			accumulator1;
	float			accumulator2;
	long			lAcc1;
	long			lAcc2;
	LOCATION		op;

	__loadAccumulator ( &accumulator1 );
	if ( AndNot )
		accumulator1 = !accumulator1;

	lAcc1 = (long)accumulator1;

	while ( __PROGRAM ( _pc ) != op_STORE		&&
			__PROGRAM ( _pc ) != op_SET			&& 
			__PROGRAM ( _pc ) != op_RESET		&&
			__PROGRAM ( _pc ) != op_TOGGLE		&& 
			__PROGRAM ( _pc ) != op_NOP			&&
			__PROGRAM ( _pc ) != op_CLB			&&
			__PROGRAM ( _pc ) != op_EQ			&&
			__PROGRAM ( _pc ) != op_GOTO		&&
			__PROGRAM ( _pc ) != op_CALL		&&
			__PROGRAM ( _pc ) != op_RESTART		&&
			0x01
		  )
	{
		op = __PROGRAM ( _pc++ );
		__loadAccumulator ( &accumulator2 );
		lAcc2 = (long)accumulator2;
		switch ( op )
		{
		case op_AND:
			lAcc1 &= lAcc2;
			break;
		case op_OR:
			lAcc1 |= lAcc2;
			break;
		case op_NOT:
			lAcc1 = !lAcc2;
			break;
		case op_XOR:
			lAcc1 ^= lAcc2;
			break;
		case op_ANDNOT:
			lAcc1 &= !lAcc2;
			break;
		case op_ORNOT:
			lAcc1 |= !lAcc2;
			break;
		}
	}
	return (float)lAcc1;
}


//
// Perform a single line step
STEP_RESULT __performAndStep (  bool apply, bool AndNot )
{
	float			accumulator1;
	float			accumulator2;
	STEP_RESULT		result = SR_OK;

	accumulator1 = __performLogicAccumulator ( AndNot );
	
	//
	// perform the next operation only if the aply flag is true
	// and the accumultaor is true
	apply = ( apply & ( accumulator1 == 1 ) );

	switch ( __PROGRAM ( _pc++ ) )
	{
	case op_STORE:
		__storeAccumulator ( apply, accumulator1 );
		break;

	case op_SET:
		if ( accumulator1  )
			__setAccumulator ( apply, TRUE );
		else
			_pc += 5;
		break;
	case op_RESET:
		if ( accumulator1 )
			__setAccumulator ( apply, FALSE );
		else
			_pc += 5;
		break;
	case op_TOGGLE:
		if ( accumulator1  )
			__toggleAccumulator ( apply );
		else
			_pc += 5;
		break;

	case op_EQ:
		__loadAccumulator ( &accumulator2 );
		if ( __performIfThenElse ( __EQ, accumulator1, accumulator2, 0 ) == SR_LEAVE_STACK  )
			return SR_OK;				
		break;

	case op_GOTO:
		__performGoto ( apply );
		if ( apply ) 
			result = SR_LEAVE_STACK;
		break;

	case op_CALL:
		__performCall ( apply );
		break;

	case op_RESTART:
		if ( __performRestart ( apply ) )
			result = SR_LEAVE_RESTART;
		break;

	case op_NOP:
		break;
	}
	return result;
}




//
// Perform a single line step
STEP_RESULT __performBracketStep (  bool apply, float* resultValue  )
{
	float			accumulator1 = 0;
	float			accumulator2 = 0;
	long			lAcc1;
	long			lAcc2;
	STEP_RESULT		result = SR_OK;
	int				first = 1;
	LOCATION		op1;
	LOCATION        op;

	while ( __PROGRAM ( _pc ) != op_STORE		&& 
			__PROGRAM ( _pc ) != op_SET			&& 
			__PROGRAM ( _pc ) != op_RESET		&&
			__PROGRAM ( _pc ) != op_TOGGLE		&& 
			__PROGRAM ( _pc ) != op_NOP			&&
			__PROGRAM ( _pc ) != op_GOTO		&&
			__PROGRAM ( _pc ) != op_CALL		&&
			__PROGRAM ( _pc ) != op_RESTART
		  )
	{

		op1 = __PROGRAM ( _pc );
		if ( op1 == op_OPB ) 
			_pc++;

		op1 = __PROGRAM ( _pc++ );
		if ( op1 == op_OPB )
			//
			// nested bracket
			__performBracketStep ( apply, &accumulator2 );
		else
			//
			// logical line
			accumulator2 = __performLogicAccumulator ( op1 == op_ANDNOT );

		if ( first == 1 )
		{
			accumulator1 = accumulator2;
			first = 0;
		}
		else
		{
			lAcc1 = (long)accumulator1;
			lAcc2 = (long)accumulator2;
			switch ( op )
			{
			case op_AND:
				lAcc1 &= lAcc2;
				break;
			case op_OR:
				lAcc1 |= lAcc2;
				break;
			case op_NOT:
				lAcc1 =~ lAcc2;
				break;
			case op_XOR:
				lAcc1 ^= lAcc2;
				break;
			case op_ANDNOT:
				lAcc1 &= ~lAcc2;
				break;
			}
			accumulator1 = (float)lAcc1;
		}
		op1 = __PROGRAM ( _pc );
		if (  op1 == op_CLB ) 
			_pc++;
		
		if ( __PROGRAM ( _pc ) != op_STORE		&& 
			 __PROGRAM ( _pc ) != op_SET		&& 
			 __PROGRAM ( _pc ) != op_RESET		&&
			 __PROGRAM ( _pc ) != op_TOGGLE		&& 
			 __PROGRAM ( _pc ) != op_GOTO		&&
			 __PROGRAM ( _pc ) != op_CALL		&&
			 __PROGRAM ( _pc ) != op_RESTART	&&
			 __PROGRAM ( _pc ) != op_NOP )
			op = __PROGRAM ( _pc++ );
	}

	switch ( __PROGRAM ( _pc++ ) )
	{
	case op_STORE:
		__storeAccumulator ( apply, accumulator1 );
		break;

	case op_SET:
		if ( lAcc1  )
			__setAccumulator ( apply, TRUE );
		else
			_pc += 5;
		break;
	case op_RESET:
		if ( lAcc1 )
			__setAccumulator ( apply, FALSE );
		else
			_pc += 5;
		break;
	case op_TOGGLE:
		if ( lAcc1  )
			__toggleAccumulator ( apply );
		else
			_pc += 5;
		break;

	case op_GOTO:
		__performGoto ( lAcc1 == 1 );
		break;

	case op_CALL:
		__performCall ( lAcc1 == 1  );
		break;

	case op_RESTART:
		if ( __performRestart ( lAcc1 == 1  ) )
			result = SR_LEAVE_STACK;
		break;

	case op_NOP:
		//
		// In the nested bracket return the evaluate value
		if ( resultValue != NULL )
			*resultValue = accumulator1;
		break;
	}
	return result;
}


//
// Perform a GOTO jump using the Jump table index
void __performGoto ( bool apply )
{
	float			accumulator1;

	__loadAccumulator ( &accumulator1 );
	if ( !apply ) 
        return;
	_pc = (int)accumulator1;
}

//
// Perform a GOTO jump using the Jump table index
void __performCall ( bool apply )
{
	float			accumulator1;
	int				storePC;


	__loadAccumulator ( &accumulator1 );
	if ( !apply ) return;
	storePC			= _pc;
	_pc				= (int)accumulator1;
	while ( __performStep ( apply ) == SR_OK );
	//
	// return back
	_pc				= storePC;
}
//
// Perform a GOTO jump using the Jump table index
//
u8 __performRestart ( bool apply )
{
	if ( !apply ) return 0;
	return 1;
}



STEP_RESULT __performIfThenElse ( COMPARECALLBACK compare, float accumulator1, float accumulator2, float accumulator3 )
{
	short				nextStep;
	u8		            compareResult;

	compareResult	= compare ( accumulator1, accumulator2, accumulator3 );
	nextStep		= __PROGRAM ( _pc++ ) << 8;
	nextStep		|= __PROGRAM ( _pc++ );
	if ( compareResult == 0x01 )
	{
		while ( __PROGRAM ( _pc ) != op_THEN ) 
			if ( __performStep  ( TRUE ) == SR_LEAVE_STACK )
				return SR_LEAVE_STACK;
		_pc++;
	}
	else
		_pc += nextStep;
	
	nextStep		= __PROGRAM ( _pc++ ) << 8;
	nextStep		|= __PROGRAM ( _pc++ );
	if ( compareResult == 0x00 )
	{
		while ( __PROGRAM ( _pc ) != op_ELSE ) 
			if ( __performStep  ( TRUE ) == SR_LEAVE_STACK )
				return SR_LEAVE_STACK;
		_pc++;
	}
	else
		_pc += nextStep;
	return SR_OK;
}

STEP_RESULT	__performCase ( float accumulator1, float accumulator2 )
{
	short				nextStep;
	u8		compareResult;

	compareResult	= __EQ ( accumulator1, accumulator2, 0 );
	nextStep		= __PROGRAM ( _pc++ ) << 8;
	nextStep		|= __PROGRAM ( _pc++ );

	if ( compareResult == 0x01 )
	{
		while ( __PROGRAM ( _pc ) != op_CASE ) 
			if ( __performStep  ( TRUE ) == SR_LEAVE_STACK )
				return SR_LEAVE_STACK;
		_pc++;
	}
	else
		_pc += nextStep;
	return SR_OK;
}

u8 __EQ ( float value1, float value2, float value3 )
{
	return ( ( value1 == value2 ) ? 0x01 : 0x00 );
}

u8 __NEQ ( float value1, float value2, float value3 )
{
	return ( ( value1 != value2 ) ? 0x01 : 0x00 );
}

u8 __GE ( float value1, float value2, float value3 )
{
	return ( ( value1 >= value2 ) ? 0x01 : 0x00 );
}

u8 __GT ( float value1, float value2, float value3 )
{
	return ( ( value1 > value2 ) ? 0x01 : 0x00 );
}

u8 __LE ( float value1, float value2, float value3 )
{
	return ( ( value1 <= value2 ) ? 0x01 : 0x00 );
}

u8 __LT ( float value1, float value2, float value3 )
{
	return ( ( value1 < value2 ) ? 0x01 : 0x00 );
}
u8 __IN ( float value1, float value2, float value3 )
{
	return ( ( value1 >= value2 ) && ( value1 <= value3 ) ? 0x01 : 0x00 );
}
u8 __NIN ( float value1, float value2, float value3 )
{
	return ( ( value1 < value2 ) || ( value1 > value3 ) ? 0x01 : 0x00 );
}




u8 XPLC_isSupportedType ( u16 type )
{
	u8 ret = 0x00;
	if ( type == op_M	    || 
		 type == op_MB	    ||
		 type == op_MW	    ||
		 type == op_MD	    ||
		 type == op_MR	    ||
		 type == op_E		||
		 type == op_EB	    ||
		 type == op_EW	    ||
		 type == op_ED	    ||
		 type == op_I		||
		 type == op_IB		||
		 type == op_IW	    ||
		 type == op_IR	    ||
		 type == op_U		||
		 type == op_UB		||
		 type == op_UW	    ||
		 type == op_UR	    
        )
		 ret = 0x01;
	return ret;
}



//
// return an information about a memory address
float XPLC_getMemory ( LPXPLCVARIABLE variable )
{
	float ret = 0;
	switch ( variable->type )
	{
	case op_M:
		if ( variable->address < MAX_MEMORY_SIZE ) 
			ret = (float) ( __MEMORY [ variable->address ] & ( 1 << variable->bit ) ? 0x01 : 0x00 );
		break;

	case op_MB:
		if ( variable->address < MAX_MEMORY_SIZE  )
			ret = (float) ( __MEMORY [ variable->address ] );
		break;

	case op_MW:
		if ( variable->address < MAX_MEMORY_SIZE - 1 ) 
			ret = (float) ( ( __MEMORY [ variable->address ] << 8 ) | 
							( __MEMORY [ variable->address + 1 ]  ) );
		break;

	case op_MD:
		if ( variable->address < MAX_MEMORY_SIZE - 3 ) 
			ret = (float) ( ( __MEMORY [ variable->address  ] << 24 ) | 
											( __MEMORY [ variable->address + 1 ] << 16  ) |
											( __MEMORY [ variable->address + 2 ] << 8  ) |
											( __MEMORY [ variable->address + 3 ]   ) );
		break;

	case op_MR:
		if ( variable->address < MAX_MEMORY_SIZE - 3 ) 
			__getFloat4 ( __MEMORY [ variable->address ],
										__MEMORY [ variable->address + 1 ],
										__MEMORY [ variable->address + 2 ],
										__MEMORY [ variable->address + 3 ],
									  &ret );
		break;

	case op_E:
		if ( variable->address < MAX_EEPROM_SIZE )
			ret = (float) ( __EEPROM [ variable->address ] & ( 1 << variable->bit ) ? 0x01 : 0x00 );
		break;

	case op_EB:
		if ( variable->address < MAX_EEPROM_SIZE  )
			ret = (float) ( __EEPROM [ variable->address ] );
		break;

	case op_EW:
		if ( variable->address < MAX_EEPROM_SIZE - 1 )
			ret = (float) ( ( __EEPROM [ variable->address ] << 8 ) | 
		    				( __EEPROM [ variable->address + 1 ]  ) );
		break;

	case op_ED:
		if ( variable->address < MAX_EEPROM_SIZE - 3 )
			ret = (float) ( ( __EEPROM [ variable->address ] << 24 ) | 
											( __EEPROM [ variable->address + 1 ] << 16  ) |
											( __EEPROM [ variable->address + 2 ] << 8  ) |
											( __EEPROM [ variable->address + 3 ]   ) );
		break;

	case op_I:
		if ( variable->address < MAX_INPUT_SIZE ) 
			ret = (float) ( __INPUT [ variable->address ] & ( 1 << variable->bit ) ? 0x01 : 0x00 );
		break;
	case op_IB:
		if ( variable->address < MAX_INPUT_SIZE  )
			ret = (float) ( __INPUT [ variable->address ] );
		break;
	case op_IW:
		if ( variable->address < MAX_INPUT_SIZE - 1 )
			ret = (float) ( ( __INPUT [ variable->address ] << 8 ) | 
		    				( __INPUT [ variable->address + 1 ]  ) );
		break;
	case op_IR:
		if ( variable->address < MAX_INPUT_SIZE - 3 ) 
			__getFloat4 ( __INPUT [ variable->address ],
						  __INPUT [ variable->address + 1 ],
						  __INPUT [ variable->address + 2 ],
						  __INPUT [ variable->address + 3 ],
						  &ret );
		break;

	case op_U:
		if ( variable->address < MAX_OUTPUT_SIZE ) 
			ret = (float) ( __OUTPUT [ variable->address ] & ( 1 << variable->bit ) ? 0x01 : 0x00 );
		break;
	case op_UB:
		if ( variable->address < MAX_OUTPUT_SIZE  )
			ret = (float) ( __OUTPUT [ variable->address ] );
		break;
	case op_UW:
		if ( variable->address < MAX_OUTPUT_SIZE - 1 )
			ret = (float) ( ( __OUTPUT [ variable->address ] << 8 ) | 
		    				( __OUTPUT [ variable->address + 1 ]  ) );
		break;
	case op_UR:
		if ( variable->address < MAX_OUTPUT_SIZE - 3 ) 
			__getFloat4 ( __OUTPUT [ variable->address ],
						  __OUTPUT [ variable->address + 1 ],
						  __OUTPUT [ variable->address + 2 ],
						  __OUTPUT [ variable->address + 3 ],
						  &ret );
		break;

	}
	variable->value = ret;
	return ret;
}

void XPLC_setMemory ( LPXPLCVARIABLE variable )
{
	u8 mask;

	switch ( variable->type )
	{
	case op_M:
		if ( variable->address < MAX_MEMORY_SIZE ) 
		{
			mask	= 1 << variable->bit;
			if ( variable->value == 1 )
				__MEMORY [ variable->address ] |= mask;
			else
			{
				mask = ~mask;
				__MEMORY [ variable->address ] &= mask;
			}
		}
		break;

	case op_MB:
		if ( variable->address < MAX_MEMORY_SIZE  ) 
			__MEMORY [ variable->address ]		= (u8)( ( (u32)variable->value ) & 0x000000FF );
		break;

	case op_MW:
		if ( variable->address < MAX_MEMORY_SIZE - 1 ) 
		{
			__MEMORY [ variable->address ]			= (u8) ( ( ( (u32)variable->value ) & 0x0000FF00 ) >> 8 );
			__MEMORY [ variable->address + 1 ]		= (u8) ( ( (u32)variable->value ) & 0x000000FF );
		}
		break;

	case op_MD:
		if ( variable->address < MAX_MEMORY_SIZE - 3 ) 
		{
			__MEMORY [ variable->address ]			= (u8) ( ( ((u32)variable->value ) & 0xFF000000 ) >> 24 );
			__MEMORY [ variable->address + 1 ]		= (u8) ( ( ((u32)variable->value ) & 0x00FF0000 ) >> 16 );
			__MEMORY [ variable->address + 2 ]		= (u8) ( ( ((u32)variable->value ) & 0x0000FF00 ) >> 8 );
			__MEMORY [ variable->address + 3 ]		= (u8) ( ( ((u32)variable->value ) & 0x000000FF ) >> 0 );
		}
		break;

	case op_MR:
		if ( variable->address < MAX_MEMORY_SIZE - 3 ) 
			__setFloat4 ( &__MEMORY [ variable->address ], variable->value );
		break;

	case op_E:
		if ( variable->address < MAX_EEPROM_SIZE )
		{
			mask	= 1 << variable->bit;
			if ( variable->value == 1 )
				__EEPROM [ variable->address ] |= mask;
			else
			{
				mask = ~mask;
				__EEPROM [ variable->address ] &= mask;
			}
		}
		break;

	case op_EB:
		if ( variable->address < MAX_EEPROM_SIZE   )
			__EEPROM [ variable->address ]			= (u8)( ( ((u32)variable->value ) & 0x000000FF ) );
		break;

	case op_EW:
		if ( variable->address < MAX_EEPROM_SIZE  - 1 )
		{
			__EEPROM [ variable->address ]			= (u8) ( ( ((u32)variable->value ) & 0x0000FF00 ) >> 8 );
			__EEPROM [ variable->address + 1 ]		= (u8) ( ((u32)variable->value ) & 0x000000FF );
		}
		break;

	case op_ED:
		if ( variable->address < MAX_EEPROM_SIZE  - 3 )
		{
			__EEPROM [ variable->address ]			= (u8)( ( ( (u32)variable->value ) & 0xFF000000 ) >> 24 );
			__EEPROM [ variable->address + 1 ]		= (u8)( ( ( (u32)variable->value ) & 0x00FF0000 ) >> 16 );
			__EEPROM [ variable->address + 2 ]		= (u8)( ( ( (u32)variable->value ) & 0x0000FF00 ) >> 8 );
			__EEPROM [ variable->address + 3 ]		= (u8)( ( ( (u32)variable->value ) & 0x000000FF ) >> 0 );
		}
		break;

	case op_I:
		if ( variable->address < MAX_INPUT_SIZE ) 
		{
			mask	= 1 << variable->bit;
			if ( variable->value == 1 )
				__INPUT [ variable->address ] |= mask;
			else
			{
				mask = ~mask;
				__INPUT [ variable->address ] &= mask;
			}
		}
		break;
	case op_IB:
		if ( variable->address < MAX_INPUT_SIZE  ) 
			__INPUT [ variable->address ]		= (u8)( ( (u32)variable->value ) & 0x000000FF );
		break;
	case op_IW:
		if ( variable->address < MAX_INPUT_SIZE  - 1 )
		{
			__INPUT [ variable->address ]			= (u8)( ( ( (long)variable->value ) & 0x0000FF00 ) >> 8 );
			__INPUT [ variable->address + 1 ]		= (u8)( ( (long)variable->value ) & 0x000000FF );
		}
		break;
	case op_IR:
		if ( variable->address < MAX_INPUT_SIZE  - 3 )
			__setFloat4 ( &__INPUT [ variable->address ], variable->value );
		break;

	case op_U:
		if ( variable->address < MAX_OUTPUT_SIZE ) 
		{
			mask	= 1 <<variable-> bit;
			if ( variable->value == 1 )
				__OUTPUT [ variable->address ] |= mask;
			else
			{
				mask = ~mask;
				__OUTPUT [ variable->address ] &= mask;
			}
		}
		break;
	case op_UB:
		if ( variable->address < MAX_OUTPUT_SIZE  ) 
			__OUTPUT [ variable->address ]		= (u8)( ( (u32)variable->value ) & 0x000000FF );
		break;
	case op_UW:
		if ( variable->address < MAX_OUTPUT_SIZE  - 1 )
		{
			__OUTPUT [ variable->address ]			= (u8)( ( ( (long)variable->value ) & 0x0000FF00 ) >> 8 );
			__OUTPUT [ variable->address + 1 ]		= (u8)( ( (long)variable->value ) & 0x000000FF );
		}
		break;
	case op_UR:
		if ( variable->address < MAX_OUTPUT_SIZE  - 3 )
			__setFloat4 ( &__OUTPUT [ variable->address ], variable->value );
		break;

	}
}

float XPLC_incMemory (u16 type, float oldValue, float step  )
{
	float value = oldValue;
	switch ( type )
	{
	case op_M:
	case op_I:
	case op_U:
	case op_E:
		if ( value == 0 ) value = 1;
		break;

	case op_MB:
	case op_EB:
	case op_IB:
	case op_UB:
	case op_MW:
	case op_IW:
	case op_EW:
	case op_UW:
	case op_MD:
	case op_ED:
	case op_MR:
	case op_IR:
	case op_UR:
		value += step;
		break;
	}
	return value;
}

float XPLC_decMemory ( u16 type, float oldValue, float step  )
{
	float value = oldValue;
	switch ( type )
	{
	case op_M:
	case op_I:
	case op_U:
	case op_E:
		if ( value == 1 ) value = 0;
		break;

	case op_MB:
	case op_EB:
	case op_IB:
	case op_UB:
	case op_MW:
	case op_IW:
	case op_EW:
	case op_UW:
	case op_MD:
	case op_ED:
	case op_MR:
	case op_IR:
	case op_UR:
		value -= step;
		break;
	}
	return value;
}


int XPLC_getTimerPreset ( u16 address )
{
	if ( address < MAX_TIMERS_SIZE )
	{
		if ( __TIMERS [ address ] != HANDLE_TIMER_NULL )
			return TIMERS_getCount ( __TIMERS [ address ] );
	}
	return 0;
}

bool XPLC_getTimerStatus (u16 address )
{
	if ( address < MAX_TIMERS_SIZE )
	{
		if ( __TIMERS [ address ] != HANDLE_TIMER_NULL )
			return TIMERS_getQ ( __TIMERS [ address ] );
	}
	return FALSE;
}

int XPLC_getTimerElapsed ( u16 address )
{
	if ( address < MAX_TIMERS_SIZE )
	{
		if ( __TIMERS [ address ] != HANDLE_TIMER_NULL )
			return TIMERS_getElapsed ( __TIMERS [ address ] );
	}
	return 0;
}

int XPLC_getCounterValue ( u16 address )
{
	if ( address < MAX_COUNTERS_SIZE )
	{
		if ( __COUNTERS [ address ] != HANDLE_COUNTER_NULL )
			return COUNTERS_getCount ( __COUNTERS [ address ] );
	}
	return 0;
}

LPXPLCINFO XPLC_getInfo ( )
{
	return &_info;
}
