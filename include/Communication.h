#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "general.h"
#include <unistd.h>

typedef enum {
  COM_state_unset, COM_state_father_ready, COM_state_son_ready
} COM_StateT;

void COM_init(void);
/* initialize the module, eg. allocate semaphores and so on */

BOOLEAN COM_fork(void);
/* returns TRUE, if it has forked a process, FALSE otherwise
 * i.e. when not allowed, when not wanted, when not possible.
 * To find out which process you are, use COM_Vadder. */

void COM_free(void);
/* free all allocated data structures. Must be called by 
 * the father process. */

COM_StateT COM_getState(void);
/* read the shared state. */

void COM_markSuccess(void);
/* mark that the process has found a proof */

void COM_kill(void);
/* kill child process. */

void COM_wait(void);
/* wait for child process. */

void COM_wakeOther(void);
/* wake other process */

void COM_sleep(void);
/* sleep at barrier, until awakened by other process */

extern BOOLEAN COM_forked;
/* do we have forked or not ? */

extern BOOLEAN COM_Vadder;
/* are we the father process ? */

#define /*BOOLEAN*/ COM_PrintIt() (!(COM_forked && !COM_Vadder))
/* shall we print something that should be printed only by one ? */

extern char* COM_Prefix;

AbstractTime COM_GlobalTime(void);

void COM_stopComputing(void);

#endif
