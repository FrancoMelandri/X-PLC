/********************************************************************
                                                                    
                    X-LOGIC                   
                                                                    
       Copyright (C) 2008 x-scada.com
       ALL RIGHTS RESERVED                                          
                                                                    
   The entire contents of this file is protected by U.S. and        
   International Copyright Laws. Unauthorized reproduction,         
   reverse-engineering, and distribution of all or any portion of   
   the code contained in this file is strictly prohibited and may   
   result in severe civil and criminal penalties and will be        
   prosecuted to the maximum extent possible under the law.         
                                                                    
   RESTRICTIONS                                                     
                                                                    
   THIS SOURCE CODE AND ALL RESULTING INTERMEDIATE FILES            
   (EXE, OBJ, DLL, ETC.) ARE CONFIDENTIAL AND PROPRIETARY TRADE     
   SECRETS OF x-scada.com THE REGISTERED DEVELOPER IS    
   LICENSED TO DISTRIBUTE THE COMPONENTS AND ALL            
   ACCOMPANYING .NET CONTROLS AS PART OF AN EXECUTABLE PROGRAM ONLY. 
                                                                    
   THE SOURCE CODE CONTAINED WITHIN THIS FILE AND ALL RELATED       
   FILES OR ANY PORTION OF ITS CONTENTS SHALL AT NO TIME BE         
   COPIED, TRANSFERRED, SOLD, DISTRIBUTED, OR OTHERWISE MADE        
   AVAILABLE TO OTHER INDIVIDUALS WITHOUT EXPRESS WRITTEN CONSENT   
   AND PERMISSION FROM x-scada.com
                                                                    
   CONSULT THE END USER LICENSE AGREEMENT FOR INFORMATION ON        
   ADDITIONAL RESTRICTIONS.                                         
                                                                    
********************************************************************/


#ifndef __COUNTERS_H_
#define __COUNTERS_H_


#define MAX_NUM_COUNTERS				150
#define HANDLE_COUNTER_NULL				0xff

typedef unsigned char HANDLE_COUNTER;

typedef enum
{
	CT_NONE,	
	CT_RAISE_EDGE,			// Counter up on raise edge
	CT_FALLING_EDGE,		// Counter up on falling edge
	CT_HIGH_LEVEL,			// Counter up on high level
	CT_LOW_LEVEL,				// Counter up on low level
	CTD_RAISE_EDGE, 		// Counter down on raise edge
	CTD_HIGH_LEVEL, 		// Counter down high level
} COUNTER_TYPE;

//
// Generic aplication timer struct
typedef struct _COUNTER
{
	HANDLE_COUNTER		            handle;							// the handle of the counter
	COUNTER_TYPE					type;							// define if
	u8								clock;							// the total count to reach
	u32 							count;							// the amount of count
} COUNTER, *LPCOUNTER;


void								COUNTERS_Init ( void );
HANDLE_COUNTER			            COUNTERS_createCounter ( void );
void								COUNTERS_destroyCounter ( HANDLE_COUNTER handle  );
void								COUNTERS_setClock ( HANDLE_COUNTER handle, u8 clock );
u32									COUNTERS_getCount ( HANDLE_COUNTER handle );
void								COUNTERS_setCount ( HANDLE_COUNTER handle, u32 preset );
void								COUNTERS_setType ( HANDLE_COUNTER handle, COUNTER_TYPE type );
COUNTER_TYPE				        COUNTERS_getType ( HANDLE_COUNTER handle );

#endif
