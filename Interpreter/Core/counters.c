

// -------------------------------------------------------------
// 
// Author ......... bMF
// Date ........... August 2011
// Description..... 
// Changes ........ 
// 
// -------------------------------------------------------------


	
#include "includes.h"
#include "counters.h"
#include "defines.h"


COUNTER		_COUNTERS [ MAX_NUM_COUNTERS ];					// global appication counters



LPCOUNTER __getCounter ( HANDLE_COUNTER handle )
{
	if (  _COUNTERS [ handle - 1 ].handle != HANDLE_COUNTER_NULL )
		return &_COUNTERS [ handle - 1 ];
	return NULL;
}

void __resetCounter ( LPCOUNTER pCounter )
{
	pCounter->handle 	    = HANDLE_COUNTER_NULL;
	pCounter->type 				= CT_NONE;
	pCounter->clock 			= FALSE;
	pCounter->count 			= 0;
}

HANDLE_COUNTER __getFirstCounter ( void )
{
	HANDLE_COUNTER i;
	for ( i = 0; i < MAX_NUM_COUNTERS; i++ )
	{
		if ( _COUNTERS [ i ].handle == HANDLE_COUNTER_NULL )
		{
			_COUNTERS [ i ].handle = i + 1;
			return i + 1;
		}
	}
	return HANDLE_COUNTER_NULL;
}





void COUNTERS_Init ( void )
{
	HANDLE_COUNTER 		i = 0;	
	for ( i = 0; i < MAX_NUM_COUNTERS; i++ )
		__resetCounter ( &_COUNTERS [ i ] );
}

HANDLE_COUNTER COUNTERS_createCounter ( void )
{
	return __getFirstCounter ();		
}

void COUNTERS_destroyCounter ( HANDLE_COUNTER handle )
{
	LPCOUNTER	pCounter = __getCounter ( handle );
	if ( pCounter != NULL )
		__resetCounter ( pCounter );
}

void COUNTERS_setClock ( HANDLE_COUNTER handle, u8 clock )
{
	LPCOUNTER	pCounter = __getCounter ( handle );
	if ( pCounter != NULL )
	{
		switch ( pCounter->type )
		{
		case CT_RAISE_EDGE:
			if ( clock == TRUE && pCounter->clock == FALSE )
				pCounter->count++;
			break;
		case CT_FALLING_EDGE:
			if ( clock == FALSE && pCounter->clock == TRUE )
				pCounter->count++;
			break;
		case CT_HIGH_LEVEL:
			if ( clock == TRUE )
				pCounter->count++;
			break;
		case CT_LOW_LEVEL:
			if ( clock == FALSE )
				pCounter->count++;
			break;
		case CTD_RAISE_EDGE:
			if ( clock == TRUE && pCounter->clock == FALSE && pCounter->count )
				pCounter->count--;
			break;
		case CTD_HIGH_LEVEL:
			if ( clock == TRUE && pCounter->count )
				pCounter->count--;
			break;
		}
		pCounter->clock = clock;
	}
}

u32 COUNTERS_getCount ( HANDLE_COUNTER handle )
{
	LPCOUNTER	pCounter = __getCounter ( handle );
	if ( pCounter != NULL )
		return pCounter->count;
	return 0;
}

void COUNTERS_setCount ( HANDLE_COUNTER handle, u32 preset )
{
	LPCOUNTER	pCounter = __getCounter ( handle );
	if ( pCounter != NULL )
		pCounter->count = preset;
}

void COUNTERS_setType ( HANDLE_COUNTER handle, COUNTER_TYPE type )
{
	LPCOUNTER	pCounter = __getCounter ( handle );
	if ( pCounter != NULL )
		pCounter->type = type;
}

COUNTER_TYPE COUNTERS_getType ( HANDLE_COUNTER handle )
{
	LPCOUNTER	pCounter = __getCounter ( handle );
	if ( pCounter != NULL )
		return pCounter->type;
	return CT_NONE;
}



