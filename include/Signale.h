/**********************************************************************
* Filename : Signale.h
*
* Kurzbeschreibung : Signalbehandlung (ohne SIGSEGV -> SpeicherVerwaltung)
*
* Autoren : Andreas Jaeger
*
* 02.06.98: Erstellung aus WaldmeisterII.c heraus
*
**********************************************************************/

#ifndef SIGNALE_H
#define SIGNALE_H

/* -------------------------------------------------------------------------------*/
/* --------------- Interrupt-Kontrolle -------------------------------------------*/
/* -------------------------------------------------------------------------------*/

extern BOOLEAN volatile SI_IRFlag; 
extern BOOLEAN volatile SI_Usr1IRFlag;

#define SI_InterruptflagSetzen() SI_IRFlag = TRUE
#define SI_InterruptflagLoeschen() SI_IRFlag = FALSE
#define SI_InterruptflagGesetzt() SI_IRFlag

#define SI_Usr1InterruptflagSetzen() SI_Usr1IRFlag = TRUE
#define SI_Usr1InterruptflagLoeschen() SI_Usr1IRFlag = FALSE
#define SI_Usr1InterruptflagGesetzt() SI_Usr1IRFlag

/* Setzen der Signale */   
void SI_SignalsSetzen (void);

/* Ruecksetzen der Signale */
void SI_SignalsDeaktivieren (void);

#endif /* MNF_INTERN_H */
