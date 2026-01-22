#ifndef DCOMMUNICATOR_H
#define DCOMMUNICATOR_H

#include "general.h"

typedef enum {
  DCM_EndMsg = 0, DCM_AddMsg = 1, DCM_ExpMsg = 2, DCM_RetMsg = 3,
  DCM_WiWMsg = 4
} DCM_MsgT;

BOOLEAN DCM_Start (int nslaves, char**argv);
/* nslaves: number of wanted slaves, argv: their command line */
/* TRUE: everything went fine, FALSE: some or fatal problem */

int DCM_GetNoOfSlaves(void);
/* number of really allocated slaves */

void DCM_SendInt(int i);
/* sends an integer */

int  DCM_RecvInt(void);
/* receives an integer */

void DCM_SendBytes(int len, byte *tp);
/* sends len bytes starting at *tp */

byte *DCM_RecvBytes(void);
/* receives bytes. Allocates new memory. */

void DCM_SendLong(long i);
/* sends a long */

long DCM_RecvLong(void);
/* recieves a long */

void DCM_SendULong(unsigned long i);
/* sends an unsigned long */

unsigned long DCM_RecvULong(void);
/* recieves an unsigned long */

void DCM_StartActive(void);
/* starts assembling an Active-Message */

void DCM_SendActive(void);
/* stops assembling an Active-Message and sends it to slaves */
 
void DCM_SendEnd(void);
/* discards message assembled so far and sends an End-Message to slaves */

DCM_MsgT DCM_NextMsg(void);
/* delivers type of next message.   BLOCKING ! */

void DCM_SendExpand(unsigned long t, int slave);
/* sends expand message to slave */

void DCM_SendWhoIsWho(int no, int slave);
/* sends who is who message (no of slaves, slave id) to slave */

void DCM_StartReturn(void);
/* starts assembling an return message */

void DCM_SendReturn(void);
/* sends expand message to master */

void DCM_SlaveStart(void);

#endif
