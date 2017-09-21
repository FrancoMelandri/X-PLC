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


#ifndef __COREPLC__H
#define __COREPLC__H

#include "counters.h"
#include "timers.h"
#include "pulses.h"
#include "defines.h"
#include "..\..\..\hw\flash.h"


typedef void (*COPYIOCALLBACK)(u8* inputMap, u16 dimensionIn, u8* outputMap, u16 dimensionOut );
typedef u8 (*COMPARECALLBACK)(float value1, float value2, float value3);
typedef void (*FIRSTLOOPCALLBACK)();
typedef float (*IS_ALARM_ACTIVE)( float id );
typedef void (*VARIABLE_CHANGED)( int id );
typedef void (*ON_STEP_PERFORMED) ( void );


#define	op_LOAD							0x01				// Load into accumulator
#define	op_SET							0x02				// set
#define	op_RESET						0x03				// Reset
#define	op_STORE						0x04				// Store 
#define	op_THEN							0x05				// Then clause
#define	op_ELSE							0x06				// Else clause
#define	op_MAIN							0x07				// End main markup
#define	op_PROCEDURE				    0x08				// End procedure markup
#define	op_IS_ALARM_ACTIVE	            0x09		        // call the function alarm active
#define	op_FIRSTLOOPBIT			        0x0a				// First loop bit
#define	op_SECONDTRIGGERBIT	            0x0b				// Second trigger bit
#define	op_TOGGLE	                    0x0c				// Toggle
#define	op_VARIABLECHANGE	            0x0d	            // variable change
#define	op_LOADN	                    0x0e	            // Load a negate into accumulator

#define	op_NOP							0x50				// No operation
#define	op_AND							0x51				// And
#define	op_OR							0x52				// Or
#define	op_ANDNOT						0x53				// And not
#define	op_NOT							0x54				// Not
#define	op_XOR							0x55				// Ex or
#define	op_ORNOT						0x56				// Or not

#define	op_ADD							0x80				// Add
#define	op_SUB							0x81				// Subtract
#define	op_MUL							0x82				// Multiple
#define	op_DIV							0x83				// Divide

#define	op_EQ							0xa0				// Equal
#define	op_NEQ							0xa1				// Not equal
#define	op_GT							0xa2				// Greather then
#define	op_GE							0xa3				// Greather or equal
#define	op_LT							0xa4				// less then
#define	op_LE							0xa5				// less or equal
#define	op_QUIT							0xa6				// Quit
#define	op_GOTO							0xa7				// Goto
#define	op_RESTART					    0xa8				// Restart from the beginning
#define	op_SWITCH						0xa9				// Switch statement
#define	op_CASE							0xaa				// Case statement
#define	op_IN							0xab				// in
#define	op_NIN							0xac				// Not in
#define	op_CALL							0xad				// Call a procedure

#define	op_M							0xb0				// memory bit
#define	op_MW							0xb1				// word memory
#define	op_MD							0xb2				// double word memory
#define	op_U							0xb3				// output
#define	op_I							0xb4				// input
#define	op_C							0xb5				// constant
#define	op_B							0xb6				// bool constant
#define	op_TON							0xb7				// Timer
#define	op_C_RE							0xb8				// Counter raise edge  
#define	op_C_FE							0xb9				// Counter falling edge
#define	op_C_HL							0xba				// Counter High level  
#define	op_C_LL							0xbb				// Counter low level   
#define	op_OPB							0xbc				// Open bracket
#define	op_CLB							0xbd				// Close bracket
#define	op_LABEL						0xbe				// Label
#define	op_MB							0xbf				// byte memory
#define	op_E							0xc0				// bit eeprom
#define	op_EB							0xc1				// byte eeprom
#define	op_EW							0xc2				// word eeprom
#define	op_ED							0xc3				// double word eeprom
#define	op_IW							0xc4				// analog input
#define	op_UNISSIGNED				    0xc5				// Unissigned
#define	op_TONS							0xc6				// Timer ON in second
#define	op_MR							0xc7				// Real memory
#define	op_CR							0xc8				// Constant in real format
#define	op_IR							0xc9				// Analog input in real format
#define op_CD_RE						0xca				// Counter down raising edge
#define op_CD_HL						0xcb				// Counter down High level  
#define op_REP							0xcc				// One cycle raising edge pulse
#define op_FEP							0xcd				// One cycle falling edge pulse
#define op_UB				            0xce		        // output byte
#define op_UW				            0xcf                // output word
#define op_UR				            0xd0                // output real
#define op_IB				            0xd1                // input byte
				

//#define MAX_PROGRAM_SIZE		4096			            // Max dimension of the plc program
#define MAX_MEMORY_SIZE			        9216			    // Max dimension of the plc memory
#define MAX_EEPROM_SIZE			        0				    // Max dimension of the plc eeprom
#define MAX_INPUT_SIZE			        512			        // Max dimension of the plc input
#define MAX_OUTPUT_SIZE			        64				    // Max dimension of the plc output
#define MAX_TIMERS_SIZE			        200				    // Max dimension of the plc timers
#define MAX_COUNTERS_SIZE		        150				    // Max dimension of the plc counters
#define MAX_PULSES_SIZE					300					// Max dimension of the plc pulses

//extern u8			__PROGRAM	[];
extern u8									__MEMORY [];
extern u8									__EEPROM [];
extern u8									__INPUT [];
extern u8									__OUTPUT [];
extern HANDLE_TIMER				            __TIMERS [];
extern HANDLE_COUNTER			            __COUNTERS [];
extern HANDLE_PULSE				            __PULSES [];

	

#define MAX_PLC_CACHE                       1//10

//
// define the structure for the PLC information
typedef struct __xPLCInfo
{
	u8				Running;
	u32				Cycles;
    u16             firstPage;
    u16             lastPage;
    u32             OldCycles;
//  u16             reloading;
//  u32             locations;
//  u8              flash_pages [MAX_PLC_CACHE] [ DLFLASH_PAGE_SIZE ];
    u8              flash_pages [ DLFLASH_PAGE_SIZE ];
//  u16             pagesLoaded;
} XPLCINFO, *LPXPLCINFO;


//
// define the structure for the PLC callback
typedef struct __xPLCallbacks
{
	COPYIOCALLBACK			UpdateInputOutput;		// user callback to fill the plc memory with the Input status
//	COPYIOCALLBACK			UpdateOutput;		// user callback to apply the output status from the plc memory
	FIRSTLOOPCALLBACK		FirstLoop;			// first loop notification
    IS_ALARM_ACTIVE         IsAlarmActive;          // 
    VARIABLE_CHANGED        VariableChanged;
    ON_STEP_PERFORMED       onStepPerformed;
} XPLCCALLBACKS, *LPXPLCCALLBACKS;


typedef struct __xPLCVariable
{
	u8					type;
	u16					address;
	u8					bit;
	float				value;
} XPLCVARIABLE, *LPXPLCVARIABLE;

//
// Start the PLC loop
void XPLC_start ( void );

//
// Stop the PLC loop
void XPLC_stop ( void );

//
// Perform a cycle of the PLC
u8 XPLC_perform ( void );

//
// System tick
void XPLC_systemTick ( void );

//
// One second clock
void XPLC_onHertz ( void );

//
// Initialize all the PLC structures and data
void XPLC_initialize ( COPYIOCALLBACK updateInputOutput, 
                       FIRSTLOOPCALLBACK firstLoop, 
                       IS_ALARM_ACTIVE isAlarmActive,
                       VARIABLE_CHANGED variableChanged,
                       ON_STEP_PERFORMED onStepPerformed);

//
// Get/Set information about a memory address
float							XPLC_getMemory ( LPXPLCVARIABLE variable );
void							XPLC_setMemory ( LPXPLCVARIABLE variable );
bool							XPLC_getTimerStatus ( u16 address );
int								XPLC_getTimerElapsed ( u16 address );
int								XPLC_getTimerPreset ( u16 address );
int								XPLC_getCounterValue ( u16 address );
float							XPLC_incMemory ( u16 type, float oldValue, float step );
float							XPLC_decMemory ( u16 type, float oldValue, float step );
u8								XPLC_isSupportedType ( u16 type );
void							XPLC_resetFLB ( void );
LPXPLCINFO				        XPLC_getInfo ( void );
u8								__EQ ( float value1, float value2, float value3  );
u8								__NEQ ( float value1, float value2, float value3  );
u8								__GE ( float value1, float value2, float value3  );
u8								__GT ( float value1, float value2, float value3  );
u8								__LE ( float value1, float value2, float value3  );
u8								__LT ( float value1, float value2, float value3  );
u8								__IN ( float value1, float value2, float value3  );
u8								__NIN ( float value1, float value2, float value3  );
#endif
