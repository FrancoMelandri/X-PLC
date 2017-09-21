

// -------------------------------------------------------------
// 
// Author ......... bMF
// Date ........... August 2011
// Description..... 
// Changes ........ 
// 
// -------------------------------------------------------------



#include "includes.h"
#include "pulses.h"


PULSE		_PULSES [ MAX_NUM_PULSES ];			


LPPULSE __getPulse ( HANDLE_PULSE handle )
{
	if (  _PULSES [ handle - 1 ].handle != HANDLE_PULSE_NULL )
		return &_PULSES [ handle - 1 ];
	return NULL;
}

void __resetPulse ( LPPULSE pPulse )
{
	pPulse->handle 	    = HANDLE_PULSE_NULL;
	pPulse->type 		= PT_NONE;
	pPulse->status 		= 0x00;
	pPulse->prevStatus	= 0x00;
}

HANDLE_PULSE __getFirstPulse( )
{
	HANDLE_PULSE i;
	for ( i = 0; i < MAX_NUM_PULSES; i++ )
	{
		if ( _PULSES [ i ].handle == HANDLE_PULSE_NULL )
		{
			_PULSES [ i ].handle = i + 1;
			return _PULSES [ i ].handle;
		}
	}
	return HANDLE_PULSE_NULL;
}


//
// Initialize all the pulse
void PULSES_Init ( )
{
	HANDLE_PULSE 		i = 0;	
	for ( i = 0; i < MAX_NUM_PULSES; i++ )
		__resetPulse ( &_PULSES [ i ] );
}

HANDLE_PULSE PULSES_createPulse ()
{
	return __getFirstPulse ();		
}

void PULSES_destroyPulse ( HANDLE_PULSE handle  )
{
	LPPULSE	pPulse = __getPulse ( handle );
	if ( pPulse != NULL )
		__resetPulse ( pPulse );
}

void PULSES_setType ( HANDLE_PULSE handle, PULSE_TYPE type )
{
	LPPULSE	pPulse = __getPulse ( handle );
	if ( pPulse != NULL )
		pPulse->type = type;
}

PULSE_TYPE PULSES_getType ( HANDLE_PULSE handle )
{
	LPPULSE	pPulse = __getPulse ( handle );
	if ( pPulse != NULL )
		return pPulse->type;
	return PT_NONE;
}

void PULSES_setStatus ( HANDLE_PULSE handle, u8 status )
{
	LPPULSE	pPulse = __getPulse ( handle );
	if ( pPulse != NULL )
	{
		pPulse->prevStatus 	= pPulse->status;
		pPulse->status 		= status;
	}
}

u8 PULSES_getQ (  HANDLE_PULSE handle )
{
	LPPULSE	pPulse = __getPulse ( handle );
	if ( pPulse != NULL )
	{
		switch ( pPulse->type )
		{
			case PT_RAISE_EDGE:
			if ( pPulse->prevStatus == 0x00 && pPulse->status == 0x01 )
				return 0x01;
            break;
			
			case PT_FALLING_EDGE:
			if ( pPulse->prevStatus == 0x01 && pPulse->status == 0x00 )
				return 0x01;
            break;
		}
	}
	return 0x00;
}

