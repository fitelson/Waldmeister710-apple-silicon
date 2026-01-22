#ifndef RUNDEVERWALTUNG_H
#define RUNDEVERWALTUNG_H

#include "DSBaumKnoten.h"
#include "general.h"
#include "TermOperationen.h"
#include "Termpaare.h"

/* ------------------------------------------------------------------------- */
/* - I. Bedingter Testbetrieb und Testfunktion ----------------------------- */
/* ------------------------------------------------------------------------- */

#define RE_TEST 0

/*       Testfunktionen   Konsistenztest     Konsistenztest
          im Kompilat?     nach Beweis?    nach Indexaenderung?
   0 --       NEIN            NEIN               NEIN
   1 --        JA              JA                NEIN
   2 --        JA              JA                 JA
*/

void RE_RUndETest(void); /* Interne Konsistenzpruefung beider Mengen. */

/* ------------------------------------------------------------------------- */
/* 0. Typen und Objekte ---------------------------------------------------- */
/* ------------------------------------------------------------------------- */

typedef GNTermpaarT RegelOderGleichungsT;
typedef GNTermpaarT GleichungsT;
typedef GNTermpaarT RegelT;

extern DSBaumT RE_Regelbaum;
extern DSBaumT RE_Gleichungsbaum;

typedef void * TermpairT;

typedef struct {
  TermT lhs;
  TermT rhs;
  RegelOderGleichungsT actualParent;
  RegelOderGleichungsT otherParent;
  TermpairT tp;
  VergleichsT result;
  BOOLEAN wasFIFO;
  BOOLEAN istGZFB;
  WId wid;
} SelectRecT;


/* ------------------------------------------------------------------------- */
/* 2. Praedikate und nebenwirkungsfreie Funktionen ------------------------- */
/* ------------------------------------------------------------------------- */

#define /*BOOLEAN*/ RE_RegelmengeLeer(/*void*/) \
  (!RE_Regelbaum.Wurzel)

#define /*BOOLEAN*/ RE_GleichungsmengeLeer(/*void*/) \
  (!RE_Gleichungsbaum.Wurzel)


/* ------------------------------------------------------------------------- */
/* 6. Schleifenkonstrukte -------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#define RE_forRegelnRobust(/*RegelT*/ RegelIndex, /*Block*/ Action)     \
  BK_forRegelnRobust(RegelIndex, RE_Regelbaum,                          \
    if(TP_lebtAktuell_M(RegelIndex)){Action})

#define RE_forGleichungenRobust(/*GleichungsT*/ GlIndex, /*Block*/ Action) \
   BK_forRegelnRobust(GlIndex, RE_Gleichungsbaum,                          \
     if(TP_lebtAktuell_M(GlIndex)){Action})

/* fuer Durchlauf von R und anschliessend E */
RegelOderGleichungsT RE_ErstesFaktum(void);
RegelOderGleichungsT RE_NaechstesFaktum(void);

  
/* ------------------------------------------------------------------------- */
/* 8. Umbenennungen -------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

void RE_FaktumExternalisieren(RegelOderGleichungsT Faktum);
/* Umbenennung der Variablen, so dass die resultierenden Variablen
   nicht im Baum vorkommen.  */

void RE_FaktumReinternalisieren(RegelOderGleichungsT Faktum);
/* Stellt nach dem Aufruf von RE_FaktumExternalisieren den status quo
   ante wieder her, allerdings nur solange, wie keine Regel oder
   Gleichung mit neuer maximaler Variablenanzahl eingefuegt worden
   ist. */


/* ------------------------------------------------------------------------- */
/* - IV. Ergebnisausgabe R-, E- und KP-Menge ------------------------------- */
/* ------------------------------------------------------------------------- */

void RE_AusgabeAktiva(void);
/* Gibt Regel- und Gleichungsmenge fuer Modus completion am Ende aus. */


/* ------------------------------------------------------------------------- */
/* - Regeln und Gleichungen als Eltern von KPs ----------------------------- */
/* ------------------------------------------------------------------------- */

#define /*BOOLEAN*/ RE_IstRegel(/*RegelOderGleichungsT*/ Faktum) \
  ((Faktum)->Nummer > 0)

#define /*BOOLEAN*/ RE_IstGleichung(/*RegelOderGleichungsT*/ Faktum) \
  ((Faktum)->Nummer < 0)

#define /*BOOLEAN*/ RE_IstUngleichung(/*RegelOderGleichungsT*/ Faktum) \
  ((Faktum)->IstUngleichung)

void RE_UngleichungEntmanteln(UTermT* s, UTermT* t, 
                              RegelOderGleichungsT Faktum);

BOOLEAN RE_UngleichungEntmantelt(UTermT* s, UTermT* t, 
                                 RegelOderGleichungsT Faktum);

#define /* long int */ RE_AbsNr(/*RegelOderGleichungsT*/ re) \
  (RE_IstRegel(re) ? (re)->Nummer : -(re)->Nummer)

#define /*char * */ RE_Bezeichnung(/*RegelOderGleichungsT*/ re) \
  (RE_IstRegel(re) ? "rule" : "equation")

#define /*char * */ RE_LiteralSymbol(/*RegelOderGleichungsT*/ re) \
  (RE_IstUngleichung(re) ? "?" : RE_IstRegel(re) ? "->" : "=")


/* ------------------------------------------------------------------------- */
/* - VI. Initialisierungen ------------------------------------------------- */
/* ------------------------------------------------------------------------- */

void RE_InitApriori(void);

/* ------------------------------------------------------------------------- */
/* - VII. Activa ----------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/* liefert den ersten bzw. letzten gueltigen Index fuer Aktiva. */
unsigned long RE_ActiveFirstIndex(void);
unsigned long RE_ActiveLastIndex(void);

/* Liefert zu einem long Wert index den 'Regel oder
   Gleichungs'-Pointer, der an dieser Stelle in A steht.*/
/* Gibt es keinen Pointer an dieser Stelle, so wird NULL zurueckgegeben. */
/* (Es sollte gelten: RE_getActive(i)->geburtsTag == i. D.h. die zum
   Zeitpunkt i geborene Gleichung steht */
/* an i. Stelle in A) */

RegelOderGleichungsT RE_getActive (unsigned long index);

void RE_deleteActivesAndIndex(void);

RegelOderGleichungsT RE_getActiveWithBirthday(AbstractTime birthday);


/*setzt den Offsetwert von dem aus bei einem Freigeben des Speichers
  angefangen wird. Alles andere is schon freigeg*/
void RE_setActivaOffset(unsigned long);

/* ------------------------------------------------------------------------- */
/* - XXX. Sonstiges -------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

void RE_FaktumEinfuegen(RegelOderGleichungsT Faktum);
void RE_FaktumToeten(RegelOderGleichungsT Faktum);

RegelOderGleichungsT RE_ErzeugeFaktumAusFertigenTeilen(int flags, 
                                                       int generation, 
                                                       TermT lhs, TermT rhs);
RegelOderGleichungsT RE_ErzeugtesFaktum(SelectRecT selRec);

void RE_deleteActivum(RegelOderGleichungsT re);

#endif
