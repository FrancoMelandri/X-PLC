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


#ifndef __PULSES_H_
#define __PULSES_H_



#define MAX_NUM_PULSES					250
#define HANDLE_PULSE_NULL				0xff

typedef unsigned char HANDLE_PULSE;

typedef enum
{
	PT_NONE,
	PT_RAISE_EDGE,
	PT_FALLING_EDGE,
} PULSE_TYPE;

//
// Generic aplication pulse struct
typedef struct _PULSE
{
	HANDLE_PULSE		    handle;							//The handle of the pulse
	PULSE_TYPE				type;							//
	u8			            status;							// the surrent status
	u8			            prevStatus;						// the last status
} PULSE, *LPPULSE;


void			        PULSES_Init ( void );
HANDLE_PULSE			PULSES_createPulse ( void );
void					PULSES_destroyPulse ( HANDLE_PULSE handle  );
void					PULSES_setType ( HANDLE_PULSE handle, PULSE_TYPE type );
PULSE_TYPE				PULSES_getType ( HANDLE_PULSE handle );
void					PULSES_setStatus ( HANDLE_PULSE handle, u8 status );
u8			            PULSES_getQ ( HANDLE_PULSE handle );

#endif

