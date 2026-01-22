/**********************************************************************
* Filename : ZielAusgaben.h
*
* Kurzbeschreibung : Ausgaben Zielverwaltung
*  (diese Datei wird nur von den Zielverwaltungs-Modulen importiert)
*
* Autoren : Andreas Jaeger
*
* 04.02.99: Erstellung aus Ausgaben.h heraus
**********************************************************************/

#ifndef ZIEL_AUSGABEN_H
#define ZIEL_AUSGABEN_H

#include "general.h"
#include "TermOperationen.h"

void ZA_ZielReduziertDrucken (unsigned int Nummer, TermT links, TermT rechts);
  
void ZA_ZielZusammengefuehrtDrucken (void);

#endif  /* ZIEL_AUSGABEN_H */
