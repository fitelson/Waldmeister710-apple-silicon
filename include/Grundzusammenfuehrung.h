/*=================================================================================================================
  =                                                                                                               =
  =  Grundzusammenfuehrung.h                                                                                      =
  =                                                                                                               =
  =  Redundanztest a la Martin-Nipkow                                                                             =
  =                                                                                                               =
  =  25.11.99 Thomas.                                                                                             =
  =================================================================================================================*/

#ifndef GRUNDZUSAMMENFUEHRUNG_H
#define GRUNDZUSAMMENFUEHRUNG_H

#include "general.h"
#include "TermOperationen.h"
#include "RUndEVerwaltung.h"

#ifndef CCE_Source
#include "Parameter.h"
#endif

#define GZ_GRUNDZUSAMMENFUEHRUNG 1                                   /* Alternativen: 0, 1; Kriterium aus oder an  */
#define GZ_ZSFB_BEHALTEN 1                      /* 0 der 1; ggf. zusammenfuehrbare Aktiva zum Reduzieren behalten. */
#define GZ_TEST 0                                            /* Alternativen: 0, 1, 2; je hoehere Geschwaetzigkeit */

#if GZ_GRUNDZUSAMMENFUEHRUNG
#define ON_GRUNDZUSAMMENFUEHRUNG(x) x
#else
#define ON_GRUNDZUSAMMENFUEHRUNG(x) /* nix */
#endif

#if 0
vgl. general.h
typedef enum {GZ_zusammenfuehrbar, GZ_wertvoll, GZ_aussichtslos, 
	      GZ_zuTeuer, GZ_gescheitert, GZ_interred, GZ_undef} GZ_statusT;      /* Achtung: Reihenfolge wichtig! */
#endif

#define GZ_doGZFB() (PA_doGZFB())

extern const unsigned int GZ_MaximaleVariablenzahl;   /* Bis zu wie vielen Variablen wird der Test durchgefuehrt ? */

BOOLEAN GZ_ACVerzichtbar(TermT l, TermT r); /* Permsub */

BOOLEAN GZ_Grundzusammenfuehrbar(TermT l, TermT r);
/* Gedacht fuer -kg bzw. -ks wenn !GZ_ZSFB_BEHALTEN. 
   Ergebnis wie Vorwaertstest; ausser Endergebnis wird nichts weitergereicht.
   Achtung: Var-Normierung als Seiteneffekt.
 */

BOOLEAN GZ_GrundzusammenfuehrbarVorwaerts(RegelOderGleichungsT opfer); 
/* Vorwaertstest: Vortest auf wertvolle Gleichungen, voller Test.
   Setzt Var-Normierung voraus!!
   Ergebnis == TRUE, wenn keine wertvolle Gleichung und als grundzusammenfuehrbar nachgewiesen.
   Ergebnis == FALSE, wenn wertvolle Gleichung, Test aussichtslos, Test zu teuer, Test gescheitert 
     Dann gzfbStatus entsprechend, Zeugen nur bei GZ_gescheitert.
 */

BOOLEAN GZ_GrundzusammenfuehrbarRueckwaerts(RegelOderGleichungsT opfer, RegelOderGleichungsT NeuesFaktum);
/* Rueckwaertstest: Test durchfuehren (je nach gzfbStatus), NeuesFaktum reduziert Zeugen / rechts interreduziert, 
                    voller Test.
   Ergebnis: Wie oben, evtl. werden Zeugen durch neue ersetzt
 */

void GZ_ACC_vorhanden(SymbolT f);
void GZ_ACCII_vorhanden(SymbolT f);
void GZ_Cleanup(void);
void GZ_SoftCleanup(void);
#endif
