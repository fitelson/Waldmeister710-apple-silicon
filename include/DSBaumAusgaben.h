/*=================================================================================================================
  =                                                                                                               =
  =  DSBaumAusgaben.h                                                                                             =
  =                                                                                                               =
  =  -einfache Ausgabe von Baeumen                                                                                =
  =  -graphisch unterstuetzte Ausgabe von Baeumen                                                                 =
  =  -Dump aller Bauminformationen                                                                                =
  =                                                                                                               =
  =                                                                                                               =
  =  10.08.94  Thomas.                                                                                            =
  =================================================================================================================*/

#ifndef DSBAUMAUSGABEN_H
#define DSBAUMAUSGABEN_H

#include "DSBaumKnoten.h"

#define BA_AUSGABEN 0

/* Alternativen:
   0 -- nur leere Funktionen im Kompilat
   1 -- Ausgabefunktionen zur Verfuegung stellen
*/


#define BA_SequenzInversAn "\033[7m"
#define BA_SequenzInversAus "\033[m"
#define BA_StandardSpaltenzahl 155
#define BA_KleinsterSchickerAbstand 3


void BA_DruckeBaum(DSBaumT Baum);


void BA_BaumMalen(DSBaumT Baum, BOOLEAN PMitAdressen, unsigned int PAbstandGeschwisterknoten,
  unsigned int Spaltenzahl, BOOLEAN MitGraphikzeichen);

void BA_BaumMalenDefaultMitG(DSBaumT Baum);

void BA_BaumMalenDefaultOhneG(DSBaumT Baum);


void BA_DumpBaum(DSBaumT Baum, BOOLEAN MitGraphik);


void BA_AdressassoziierungBeginnen(void);

unsigned int BA_Assoziation(void *Adresse);

void BA_AdressassoziierungBeenden(void);

void BA_DruckeAssAdresse(void *Adresse);

void BA_BaumknotenAssoziieren(DSBaumT Baum);

#endif
