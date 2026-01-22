/* FixpunktACKP.c
 * 16.01.2003
 * Christian Schmidt
 */

#include "ACGrundreduzibilitaet.h"
#include "Ausgaben.h"
#include "DSBaumOperationen.h"
#include "FixpunktACKP.h"
#include "MissMarple.h"
#include "NFBildung.h"
#include "Ordnungen.h"
#include "Subsumption.h"
#include "TermOperationen.h"

/* - Parameter -------------------------------------------------------------- */
#define ausgaben 1 /* 0, 1 */


/* - Macros ----------------------------------------------------------------- */
#if ausgaben
#define blah(x) { IO_DruckeFlex x ; }
#else
#define blah(x)
#endif


/* - Tabelle fuer die erzeugten KPs + Operationen auf dieser ---------------- */

/* Eintraege der KPTab: 
 * Enthaelt eine Gleichung
 * und die Markierung, ob diese AC-Grundreduzibel ist. */
typedef struct KPTabEntryT {
  BOOLEAN acg; /* Ist dieses Faktum AC-Grundreduzibel? */
  BOOLEAN sk;  /* Ist dieses Faktum streng konsistent? */
  BOOLEAN sub;  /* Ist dieses Faktum subsumierbar? */
  BOOLEAN red;  /* Ist dieses Faktum reduzierbar? */
  BOOLEAN zus;  /* Ist dieses Faktum zusammenfuehrbar? */
  TermT linkeSeite;
  TermT rechteSeite;
} KPTabEntry;


/* KPTab */
typedef struct KPTabT {
  unsigned short size;    /* Gesamtgroesse der Tabelle */
  unsigned short aktSize; /* aktueller Fuellstand der Tabelle,
			   * Nummer der ersten freien Zeile */
  unsigned short aktEntry; /* aktuell bearbeiteter Eintrag,
			    * alle Eintraege < aktEntry sind bereits bearbeitet. */
  unsigned short dubletten; /* Wie oft wurde 'neues' Wissen nicht eingetragen,
			     * weil es bereits in der Tabelle stand? */
  KPTabEntry *entries;
} KPTab;

static KPTab KPs; /* Existiert genau _einmal_ pro Waldmeisterlauf. */


/* Wenn KPTab bereits initialisiert wurde,
 * wird nur der Fuellstand (aktSize) und aktEntry auf 0 gesetzt. */
static void KPTab_init(void)
/* Erzeugt Verwaltungsblock und allokiert Raum fuer Elemente */
{
  /* Wurde KPTab noch nicht initialisiert? */
  if (KPs.size == 0){
    KPs.size = 128;
    KPs.entries = our_alloc(KPs.size * sizeof(KPTabEntry));
  }
  KPs.aktEntry = 0;
  KPs.aktSize = 0;
  KPs.dubletten = 0;
}


/* verdoppelt die Groesse der KPTab
 * veraendert Inhalt der bestehenden KPTab _nicht_ */
static void KPTab_extend(void)
{
  KPs.size = KPs.size * 2;
  KPs.entries = our_realloc(KPs.entries, KPs.size * sizeof(KPTabEntry), "Extending KPTab");
}


static void KPTab_ensureCapacity(void)
{
  if (!(KPs.aktSize < KPs.size)){
    KPTab_extend();
  }
}


/* Ist die Gleichung schon in der KPTab enthalten? */
static BOOLEAN KPTab_containsKP(UTermT li, UTermT re)
{
  int i;
  for (i=0; i < KPs.aktSize; i++){
    if (TO_TermeGleich(KPs.entries[i].linkeSeite, li) 
	&& TO_TermeGleich(KPs.entries[i].rechteSeite, re)){
      return TRUE;
    }
  }
  return FALSE;
}


static BOOLEAN zusammenfuehrbar(UTermT li, UTermT re)
{
  UTermT links = TO_Termkopie(li);
  UTermT rechts = TO_Termkopie(re);

  NF_NormalformRE(links);
  NF_NormalformRE(rechts);

  return TO_TermeGleich(links, rechts);
}


/* Traegt die Gleichung in die KPTab ein,
 * falls diese noch nicht enthalten ist.
 * Zuvor wird variablennormiert. 
 * Traegt auch ein, ob Gleichung ACGrundreduzibel.*/
static void KPTab_insertKP(UTermT li, UTermT re)
{
  KPTab_ensureCapacity();

  BO_TermpaarNormierenAlsKP(li, re);

  if (!(KPTab_containsKP(li, re))){
    KPs.entries[KPs.aktSize].linkeSeite = li;
    KPs.entries[KPs.aktSize].rechteSeite = re;  

    KPs.entries[KPs.aktSize].acg = AC_TermeGrundreduzibel(li, re);
    KPs.entries[KPs.aktSize].sk = AC_TermeStrengKonsistent(li, re);

    /* Um Vorbedingung fuer SS_TermpaarSubsummiertVonGM zu erfuellen. */
    if (!((KPs.aktSize == 0) || TO_TermeGleich(li, re))){
      KPs.entries[KPs.aktSize].sub = SS_TermpaarSubsummiertVonGM(li, re);
      KPs.entries[KPs.aktSize].red = (NF_TermReduzibel(TRUE, TRUE, li) 
				      || NF_TermReduzibel(TRUE, TRUE, re));
    }

    KPs.entries[KPs.aktSize].zus = zusammenfuehrbar(li, re);
    
    KPs.aktSize++;
  }
  else {
    KPs.dubletten++;
  }
}


/* Schoene Ausgabe der KPTab */
static void KPTab_print()
{
  int i;
  int acg = 0;
  int sk = 0;
  int sub = 0;
  int red = 0;
  int zus = 0;

  blah(("\n---------------------------------------------------\n"));
  blah(("KPTab:\n"));
  for (i=0; i < KPs.aktSize; i++){
    /* blah(("Eintrag Nr. %d: ", i));*/
    /* blah(("abgearbeitet: %b ", (i < KPs.aktEntry))); */
    blah((" %t = %t", KPs.entries[i].linkeSeite, KPs.entries[i].rechteSeite));
    blah((" %d", KPs.entries[i].acg));
    blah(("%d", KPs.entries[i].sk));
    blah(("%d", KPs.entries[i].sub));
    blah(("%d", KPs.entries[i].red));
    blah(("%d\n", KPs.entries[i].zus));
    if (KPs.entries[i].acg){
      acg++;
    }
    if (KPs.entries[i].sk){
      sk++;
    }
    if (KPs.entries[i].sub){
      sub++;
    }
    if (KPs.entries[i].red){
      red++;
    }
    if (KPs.entries[i].zus){
      zus++;
    }
  }
  blah(("Anzahl insgesamt: %d\n", KPs.aktSize));
  blah(("Dubletten (n.e.): %d\n", KPs.dubletten));
  blah(("Davon ACG:        %d Davon nicht ACG:  %d\n", acg, KPs.aktSize-acg));
  blah(("Davon SK:         %d Davon nicht SK:   %d\n", sk, KPs.aktSize-sk));
  blah(("Davon SUB:        %d Davon nicht SUB:  %d\n", sub, KPs.aktSize-sub));
  blah(("Davon RED:        %d Davon nicht RED:  %d\n", red, KPs.aktSize-red));
  blah(("Davon ZUS:        %d Davon nicht ZUS:  %d\n", zus, KPs.aktSize-zus));
  blah(("---------------------------------------------------\n\n"));
}


static void KPTab_TabelleAbraeumen(void)
{
  int i;
  /* Der erste Eintrag der KPTab ist die an FP_ACKPsBildenZuGleichung 
   * uebergebene Gleuchung. Diese darf nicht geloescht werden. Deshalb i=1. */
  for (i=1; i < KPs.aktSize; i++){
    TO_TermeLoeschen(KPs.entries[i].linkeSeite, KPs.entries[i].rechteSeite);
  }
}


/* - ACKPsBilden - kopiert aus Unifikation1.c -------------------------------
 * Bildet alle ACKPs zu dieser Gleichung und traegt diese in die KPTab ein. */
static UTermT rekonstruierePos(UTermT t, UTermT tp, UTermT t_)
{
  UTermT res = t_;

  while (tp != t) {
    t = TO_Schwanz(t);
    res = TO_Schwanz(res);
  }

  return res;
}


/* Bildet alle AC-kritischen Paare zu dieser Gleichung,
 * auf der linken Seite (seite = 0),
 * oder auf der rechten Seite (seite = 1).
 * Diese werden direkt in die KPTab eingefuegt. */
static void ACKPsBildenZuGleichung(BOOLEAN seite, UTermT linkeSeite, UTermT rechteSeite)
{
  UTermT tt1, tt2, t, tp, t_, tp_, t0;

  if (seite){ t = rechteSeite; }
  else { t = linkeSeite; }

  t0 = TO_NULLTerm(t);

  for (tp = t ; tp != t0 ; tp = TO_Schwanz(tp)) {
    
    if (MM_ACC_aktiviert(TO_TopSymbol(tp))) {
      
      tt1 = TO_ErsterTeilterm(tp);
      tt2 = TO_ZweiterTeilterm(tp);

      if (TO_TopSymbol(tt2) == TO_TopSymbol(tp)) { /* C_ gefunden */
	  
	tt2 = TO_ErsterTeilterm(tt2);

	if (!ORD_TermGroesserUnif(tt2, tt1)) {
	  /* TODO: es muss noch Weggefiltert(..) aufgerufen werden */
	    
	  t_ = TO_Termkopie(t);
	  tp_ = rekonstruierePos(t, tp, t_);

	  /* tp_ == +(tt1,+(tt2,tt3)), deshalb ist der 
	   * Vorgaenger von tt1 tp_ under der Vorgaenger von tt2
           * TO_ZweiterTeilterm(tp_) == +(tt2,tt3) */
	  TO_TeiltermeTauschen(tp_, TO_ZweiterTeilterm(tp_));

	  if (seite){ KPTab_insertKP(TO_Termkopie(linkeSeite), t_); }
	  else { KPTab_insertKP(t_, TO_Termkopie(rechteSeite)); }
	}
      }
      else {
	/* top(tt1) element AC _und_ top(tt2) =/= top(tt1) (also normales C) */

	if (!ORD_TermGroesserUnif(tt2, tt1)) {
	  /* TODO: es muss noch Weggefiltert(..) aufgerufen werden */

	  t_ = TO_Termkopie(t);
	  tp_ = rekonstruierePos(t, tp, t_);	    

	  /* tp_ == +(tt1,tt2), deshalb ist der 
	   * Vorgaenger von tt1 tp_ und der Vorgaenger von tt2
           * die letzte Zelle von tt1 */
	  TO_TeiltermeTauschen(tp_, TO_TermEnde(TO_ErsterTeilterm(tp_)));

	  if (seite){ KPTab_insertKP(TO_Termkopie(linkeSeite), t_); }
	  else { KPTab_insertKP(t_, TO_Termkopie(rechteSeite)); }
	}
      }
    }
  }
}


/* - Schittstellen ---------------------------------------------------------- */

/* Bildet den Fixpunkt aller AC Kritischen Paare einer Gleichung.
 * Diese werden alle in eine Tabelle (KPTab) eingetragen.
 * Zunaechst wird die uebergebene Gleichung in die Tabelle eingetragen.
 * Dann laeuft eine Schleife ueber alle Eintrage der Tabelle
 * und bildet zu jedem Eintrag die AC Kritischen Paare,
 * falls dieser ACGrundreduzibel ist,
 * welche wiederum in die Tabelle eingetragen werden,
 * sofern sie noch nicht (nach Variablennormierung) eingetragen sind.
 * Beachte: Die Tabelle fuellt sich waehrend der Schleifendurchlaeufe.
 */
void FP_ACKPsBildenZuGleichung(UTermT li, UTermT re)
{
  KPTab_init();
  KPTab_insertKP(li, re);
  /* KPTab_print(); */

  for (; KPs.aktEntry < KPs.aktSize; KPs.aktEntry++){

    if (KPs.entries[KPs.aktEntry].acg){
      ACKPsBildenZuGleichung(FALSE, /* links */
			     KPs.entries[KPs.aktEntry].linkeSeite, 
			     KPs.entries[KPs.aktEntry].rechteSeite);
      ACKPsBildenZuGleichung(TRUE, /* rechts */
			     KPs.entries[KPs.aktEntry].linkeSeite, 
			     KPs.entries[KPs.aktEntry].rechteSeite);
      /* KPTab_print(); */
    }
  }
  KPTab_print();
  
  KPTab_TabelleAbraeumen();
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
