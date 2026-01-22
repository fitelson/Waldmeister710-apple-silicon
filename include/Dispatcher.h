#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "general.h"
#include "RUndEVerwaltung.h"
#include "KDHeapElements.h"

#if COMP_DIST

void DIS_SetStartTime(unsigned int start);
void DIS_SetNoOfJobs(unsigned int jobs);
void DIS_SetNoOfSlaves(unsigned int slaves);
unsigned int DIS_GetNoOfSlaves(void);

void DIS_Start(int argc, char **argv);
void DIS_End(void);

void DIS_AddActive(RegelOderGleichungsT active);
void DIS_DelActive(RegelOderGleichungsT active);
void DIS_GJActive(RegelOderGleichungsT active);
void DIS_Flush(void);

void DIS_InsertJob(AbstractTime i);
BOOLEAN DIS_RetrieveAEntry(ElementT *xp, BOOLEAN urgent);
BOOLEAN DIS_RetrieveIEntry(ElementT *xp);

extern BOOLEAN SLAVE_VERBOSE
extern BOOLEAN MASTER_RECHNET_MIT;


#else

#define /*void*/ DIS_SetStartTime(/*unsigned int*/ start) /* nix tun */
#define /*void*/ DIS_SetNoOfJobs(/*unsigned int*/ jobs) /* nix tun */
#define /*void*/ DIS_SetNoOfSlaves(/*unsigned int*/ slaves) /* nix tun */
#define /*unsigned int*/ DIS_GetNoOfSlaves(/*void*/) 0

#define /*void*/ DIS_Start(/*int*/ argc, /*char ** */argv) /* nix tun */
#define /*void*/ DIS_End(/*void*/) /* nix tun */

#define /*void*/ DIS_AddActive(/*RegelOderGleichungsT*/ active) /* nix tun */
#define /*void*/ DIS_DelActive(/*RegelOderGleichungsT*/ active) /* nix tun */
#define /*void*/ DIS_GJActive(/*RegelOderGleichungsT*/ active) /* nix tun */
#define /*void*/ DIS_Flush(/*void*/) /* nix tun */

#define /*void*/ DIS_InsertJob(/*AbstractTime*/ i) /* nix tun */
#define /*BOOLEAN*/ DIS_RetrieveAEntry(/*ElementT * */xp, /*BOOLEAN*/ urgent) FALSE
#define /*BOOLEAN*/ DIS_RetrieveIEntry(/*ElementT * */xp) FALSE

#define SLAVE_VERBOSE FALSE
#define MASTER_RECHNET_MIT FALSE

#endif
#endif
