/*=================================================================================================================
  =                                                                                                               =
  =  XFilesFuerPMh.h                                                                                              =
  =                                                                                                               =
  =  Fuer PhilMarlow.h benoetigte Typdefinitionen aus XFiles.h                                                    =
  =                                                                                                               =
  =  10.06.98 Aus XFiles.h extrahiert. Thomas.                                                                    =
  =                                                                                                               =
  =================================================================================================================*/

/* Diese Definitionsdatei wird in PhilMarlow.h anstelle von XFiles.h inkludiert.
   Die Definitionsdatei PhilMarlow.h ihrerseits wird etwa von WaldmeisterII.c benoetigt.
   Dort waeren andernfalls alle Innereien aus den XFiles sichtbar gewesen,
   was vielleicht irgendwann zu - vermeidbaren - Schwierigkeiten haette fuehren koennen. */


#ifndef XFILESFUERPMH_H
#define XFILESFUERPMH_H

#include "SymbolOperationen.h"

#define maxAnzOperatoren 8                                     /* maximale Anzahl von Operatoren in einer Signatur */
typedef SymbolT Signatur_SymbolT[maxAnzOperatoren+1];
typedef char Signatur_ZeichenT[maxAnzOperatoren+1];                                      /* stets \0 einschliessen */


typedef struct {
  char *Optionen;
  unsigned int Schrittzahl;
} StrategieT;

#endif
