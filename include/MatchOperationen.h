/*=================================================================================================================
  =                                                                                                               =
  =  MatchOperationen.h                                                                                           =
  =                                                                                                               =
  =  Matchen von Termen mit Regel- oder Gleichungsbaum                                                            =
  =                                                                                                               =
  =                                                                                                               =
  =  26.03.94  Thomas.                                                                                            =
  =  02.02.95  Thomas. Rekursionen eliminiert.                                                                    =
  =                                                                                                               =
  =================================================================================================================*/

#ifndef MATCHOPERATIONEN_H
#define MATCHOPERATIONEN_H

#include "TermOperationen.h"
#include "Termpaare.h"
#include "RUndEVerwaltung.h"
#include "general.h"


/*=================================================================================================================*/
/*= I. Variablenbank fuer Substitutionen; sonstige globale Groessen; Statistik-Funktionen                         =*/
/*=================================================================================================================*/


extern RegelOderGleichungsT MO_AngewendetesObjekt;

unsigned int MO_AusdehnungVariablenbank(void);                                            /* fuer Statistik-Zwecke */

unsigned int MO_AusdehnungKopierstapel(void);

void MO_VariablenfeldAnpassen(SymbolT DoppelteNeuesteBaumvariable);
/* xM so ausdehnen, dass das Feld mit Variablen x(1), x(2), ..., x(neuesteBaumvariable) zurechtkommt */

void MO_DruckeMatch(void);


/*=================================================================================================================*/
/*= IV. Operationen zum Matchen mit Regel- sowie Gleichungsbaum                                                   =*/
/*=================================================================================================================*/


BOOLEAN MO_RegelGefunden(UTermT Term);
/* Sucht im Regelbaum nach einer Generalisierung von Term. SubstAnwenden liefert dann diezugehoerige substituierte 
   rechte Seite der ersten Regel in der zum Blatt gehoerenden Regelliste. Suchstrategie: Tiefensuche. Besuchsrei-
   henfolge fuer Unterbaeume: zu passendem Funktionssymbol, zu noch nicht gebundener Variable, zu schon gebundenen
   Variablen, beginnend mit den davon neueren, endend bei x1.                                                      */


BOOLEAN MO_GleichungGefunden(UTermT Term);
 /* Sucht im Regelbaum nach einer Generalisierung von Term. SubstAnwenden liefert dann diezugehoerige substituierte
    rechte Seite der ersten Regel in der zum Blatt gehoerenden Regelliste.
    Suchstrategie: Tiefensuche. Besuchsreihenfolge fuer Unterbaeume: zu passendem Funktionssymbol, zu noch nicht ge-
    bundener Variable, zu schon gebundenen Variablen, beginnend mit den davon neueren, endend bei x1. */

BOOLEAN MO_ObjektGefunden(DSBaumP Baum, UTermT Term);
 /* Sucht in Baum nach einer Generalisierung von Term. SubstAnwenden liefert dann diezugehoerige substituierte
    rechte Seite der ersten Regel in der zum Blatt gehoerenden Regelliste.
    Suchstrategie: Tiefensuche. Besuchsreihenfolge fuer Unterbaeume: zu passendem Funktionssymbol, zu noch nicht ge-
    bundener Variable, zu schon gebundenen Variablen, beginnend mit den davon neueren, endend bei x1. */

void MO_schuetzeTerm(TermT Term, TermT Schutz);
/* Falls auf Term gematcht wird, werden nur Regeln/Gleichungen akzeptiert, die in der Dreiecksordnung kleiner
   als Schutz sind. */

void MO_keinenSchutz(void);
/* Hebt den Schutz wieder auf. */

/*=================================================================================================================*/
/*= V. Matching mit nur einer Regel oder Gleichung                                                                =*/
/*=================================================================================================================*/

BOOLEAN MO_ObjektPasst(RegelOderGleichungsT Objekt, UTermT t);
/* Liefert TRUE, wenn Objekt auf Term anwendbar ist. In diesem Falle ist die entsprechende Substitution wie gehabt
   zum Einpassen bereit und kann mit den unten aufgefuehrten Routinen in den Gesamt-Term eingebaut werden.*/

BOOLEAN MO_TermpaarPasst(TermT links, TermT rechts, UTermT Term);
/* Liefert TRUE, wenn das Termpaar links=rechts als Regel auf Term anwendbar ist. In diesem Falle ist die 
   entsprechende Substitution wie gehabt zum Einpassen bereit und kann mit den unten aufgefuehrten Routinen in
   den Gesamt-Term eingebaut werden.
   Vor.: das Termpaar ist bereits normiert.*/

BOOLEAN MO_SigmaTermIstGeneralisierung(UTermT links, UTermT Termrest);
/* Liefert TRUE, wenn sigma(links) Verallgemeinerung von Termrest ist.
   Es sind dann aber keine neuen Terme unter dieser Substitution erzeugbar.
   Der Term links muss in Fortsetzung der bisherigen Anfragen normiert sein (insbesondere keine Variablen-Luecken!). 
   Die Substitution wird stets wieder zurueckgesetzt. */

BOOLEAN MO_TermIstGeneralisierung(UTermT links, UTermT Termrest);
/* Liefert TRUE, wenn links Verallgemeinerung von Termrest ist.
   Es sind dann aber keine neuen Terme unter der Substitution erzeugbar.
   Der Term links muss bereits normiert sein. 
   Die Substitution bleibt erhalten und kann fuer MO_SigmaTermIstGeneralisierung verwendet werden. */

BOOLEAN MO_TermpaarPasstMitResultat(TermT links, TermT rechts, TermT TermOld, TermT *TermNew, UTermT aktTeilterm);
/* Liefert TRUE, wenn das Termpaar links=rechts auf Stelle aktTeilterm inTermOld auf Toplevel anwendbar ist und
   liefert in diesem Fall das Ergebnis den geaenderten Term als TermNew mit neuen
   Termzellen zurueck. Es wird nicht geprueft, ob TermOld > TermNew ist.
   Die Funktion ist also nicht destruktiv, TermNew muss freigegeben werden. */

BOOLEAN MO_existiertMatch(TermT links, TermT Term);
/* liefert TRUE, wenn ein Match von links auf Term existiert. Die Substitution wird dabei NICHT veraendert! */

/*=================================================================================================================*/
/*= VII. Erzeugung von SigmaR beginnend mit der ersten Zelle des Anfrageterms                                     =*/
/*=================================================================================================================*/

/* Nachfolgend Funktionen zur Bildung von SigmaR, nachdem eine Regel gefunden worden ist. Die Varianten ergeben
   sich aus den Anforderungen der Normalformbildung. - Die Funktionen besorgen auch das Loeschen des Anfrageterms. */

void MO_SigmaRInEZ(void);
/* SigmaR beliebiger Stelligkeit beginnend mit erster Zelle des Anfrageterms erzeugen */


/*=================================================================================================================*/
/*= VIII. Erzeugung von SigmaR endend mit der letzten Zelle des Anfrageterms                                      =*/
/*=================================================================================================================*/

UTermT MO_SigmaRInLZ(void);
/* SigmaR beliebiger Stelligkeit endend mit letzter Zelle des Anfrageterms erzeugen */


/*=================================================================================================================*/
/*= X. Subsumptionstests                                                                                          =*/
/*=================================================================================================================*/

BOOLEAN MO_TermpaarSubsummiertZweites(TermT links1, TermT rechts1, UTermT links2, UTermT rechts2);
/* Liefert TRUE, wenn das Termpaar links1==rechts1 allgemeiner ist als das Termpaar links2==rechts2, dh. wenn
   eine Substitution @ existiert, so dass links1 == @(links2) und rechts1 == @(rechts2).
   Getestet wird dabei nur in dieser einen Richtung.*/


BOOLEAN MO_SubsummierendeGleichungGefunden(UTermT links, UTermT rechts);
/* Sucht im Gleichungsbaum nach allen Generalisierungen von links==rechts. */


/*=================================================================================================================*/
/*= XI. Initialisierung                                                                                           =*/
/*=================================================================================================================*/

void MO_InitApriori(void);


/*=================================================================================================================*/
/*= XIII. Alle Matches                                                                                            =*/
/*=================================================================================================================*/

void MO_AllRegelMatchInit (UTermT Term);
/* gibt Nummer der Regel zurueck, die angewandt wurde, wenn Rueckgabe TRUE */

BOOLEAN MO_AllRegelMatchNext (RegelT *regelp);

TermT MO_AllTermErsetztNeu(UTermT Term, UTermT Stelle);

void MO_AllGleichungMatchInit (UTermT Term);

BOOLEAN MO_AllGleichungMatchNext (BOOLEAN Ordnungstest, GleichungsT *gleichungp);

BOOLEAN MO_GleichungsrichtungPasstMitFreienVarsOhneOrdnung(GleichungsT Gleichung, UTermT Anfrage, UTermT *Ergebnis, UTermT aktTeilterm);


/*=================================================================================================================*/
/*= XIV. Matching modulo Skolemfunktionen                                                                         =*/
/*=================================================================================================================*/

BOOLEAN MO_SkolemGleich(TermT s, TermT t);

BOOLEAN MO_SkolemAllgemeiner(TermT s, TermT t,  TermT s0, TermT t0);

BOOLEAN MO_TupelSkolemAllgemeiner(TermT s, TermT t,  TermT u, TermT v);

#endif
