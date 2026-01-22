/*=================================================================================================================
  =                                                                                                               =
  =  PhilMarlow.h                                                                                                 =
  =                                                                                                               =
  =  Erkennung algebraischer Strukturen                                                                           =
  =                                                                                                               =
  =  28.04.98 Extraktion aus Ordnungsgenerator. Thomas.                                                           =
  =                                                                                                               =
  =================================================================================================================*/

#ifndef PHILMARLOW_H
#define PHILMARLOW_H

#include "LispLists.h"
#include "XFilesFuerPMh.h"


extern ListenT PM_Gleichungen,
               PM_Ziele;

extern BOOLEAN PM_Existenzziele;
#define PM_Existenzziele() PM_Existenzziele

void PM_InitAposteriori(void);

void PM_SignaturTesten(char *Signatur, unsigned int Tafelnummer);

void PM_SpezifikationAnalysieren(void);

void PM_ArchivLeeren(void);

StrategieT *PM_Strategie(unsigned int AnlaufNr);

void PM_StrategienDrucken(void);


void oprintf(char *Format, ...);

void NeueStrategie(unsigned int Schrittzahl);

#endif
