/*=================================================================================================================
  =                                                                                                               =
  =  WaldmeisterII.h                                                                                              =
  =                                                                                                               =
  =  Kapselung des Beweisers fuer WM-TW                                                                           =
  =                                                                                                               =
  =  04.03.97 Thomas.                                                                                             =
  =                                                                                                               =
  =================================================================================================================*/

#ifndef WALDMEISTERII_H
#define WALDMEISTERII_H

#include "Communication.h"

extern unsigned int WM_AnlaufNr;
extern BOOLEAN WM_LaufBeendet;

/* zum Testen, ob initialisiert werden soll */
#define WM_ErsterAnlauf() (COM_forked || (WM_AnlaufNr == 1))

#if COMP_POWER

int WM_main(int argc, char *argv[]);

#endif

#endif
