#ifndef __PLC_H
#define __PLC_H

#define         PICKUP_MAX_THRESHOLD        8000

typedef enum
{
    IO_POWERS,
    IO_PRESSURE,
    IO_TEMPERATURE,
    IO_LEVEL,
    IO_AUX
} PLC_IO_STATUS;


void 		PLC_initialize ( void );
u8 		    PLC_perform ( u8 force );
void 		PLC_Reset ( void );
void        PLC_Stop ( void );
void        PLC_onHertz ( void );
void 		PLC_enabled ( u8 enable );
void 		PLC_registerUserAction ( u32 action );


void        HANDLER_handlePLCCycle ( void );
void        HANDLER_handlePLCInterrupt ( void );
#endif
