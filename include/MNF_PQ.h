/**********************************************************************
* Filename : MNF_PQ.h
*
* Kurzbeschreibung : Verwalten der Priority-Queue fuer MNF
*
* Autoren : Andreas Jaeger
*
* 03.09.96: Erstellung
*
**********************************************************************/

#ifndef MNF_PQ_H
#define MNF_PQ_H

#include "general.h"

typedef enum {
   MNFPQ_stack, MNFPQ_deque, MNFPQ_fifo
} MNFPQ_ArtT;


   
/* Parameter: void -> PQ */
typedef void * (*MNFPQ_GetNewPQT) (void);

/* Parameter: PQ, Data -> void */
typedef void (*MNFPQ_PushT) (void *, void *);

/* Parameter: PQ, BOOLEAN -> Data */
/* BOOLEAN wird auf TRUE gesetzt, wenn letztes Element irreduzibel war */
/* Pop nimmt je nach zugrundeliegender PQ, dann ein anderes Element */
typedef void * (*MNFPQ_PopT) (void *, BOOLEAN);

/* Parameter: PQ -> void */
typedef void (*MNFPQ_NewT) (void *);

/* Parameter: PQ -> void */
typedef void (*MNFPQ_ClearT) (void *);

/* Parameter: PQ -> void */
typedef void (*MNFPQ_DeleteT) (void *);

/* Parameter: PQ -> BOOLEAN */
typedef BOOLEAN (*MNFPQ_EmptyT) (void *);

/* folgendes wird in MNF_intern.h definiert, da sonst die include-Files
   jeweils voneinander abhaengig waeren
void MNFPQ_InitMenge (MNF_MengeT *menge);
*/

#endif /* MNF_PQ_H */
