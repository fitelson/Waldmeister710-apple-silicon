/* SuffSolver.c
 * 21.11.2002
 * Christian Schmidt
 */

#include "SuffSolver.h"
#include "Ausgaben.h"
#include "SymbolOperationen.h"
#include "TermOperationen.h"
#include "Ordnungen.h"  /* Fuer den Afterburner */

/* - Parameter -------------------------------------------------------------- */

#define ausgaben 0 /* 0, 1 */

static BOOLEAN Suff_flag = TRUE;

/* - Macros ----------------------------------------------------------------- */

#if ausgaben
#define blah(x) {if (Suff_flag) { IO_DruckeFlex x ; } }
#else
#define blah(x)
#endif

#define accdomtab(rel1,rel2) domtab[(rel1)*6+(rel2)]
#define acctranstab(rel1,rel2) transtab[(rel1)*6+(rel2)]
#define swap(rel) swaptab[(rel)]
#define convcons(rel) converttab[(rel)]

/* - Nuetzliche Tabellen ---------------------------------------------------- */

typedef enum {uk,eq,gt,lt,ge,le,bt} RelT;

/* Fuer schoene Ausgaben. */
static char *RelTNames[] = {" ."," ="," >"," <",">=","<=","!!"};

static char domtab[] = {
 uk,eq,gt,lt,ge,le,
 eq,eq,bt,bt,eq,eq,
 gt,bt,gt,bt,gt,bt,
 lt,bt,bt,lt,bt,lt,
 ge,eq,gt,bt,ge,eq,
 le,eq,bt,lt,eq,le
};

static char transtab[] = {
 uk,uk,uk,uk,uk,uk,
 uk,eq,gt,lt,ge,le,
 uk,gt,gt,uk,gt,uk,
 uk,lt,uk,lt,uk,lt,
 uk,ge,gt,uk,ge,uk,
 uk,le,uk,lt,uk,le
};

static char swaptab[] = {
 uk,eq,lt,gt,le,ge
};

/* Diese Tabelle konvertiert die enum Constraint (Constraint.h) nach RelT */
static char converttab[] = {
  gt,eq,ge
};

/* - Forwards --------------------------------------------------------------- */

static BOOLEAN RelTab_insertRel(RelT rel, ExtraConstT sid, ExtraConstT tid);

/* - MemoTab ---------------------------------------------------------------- */

/* Enthaelt Terme (aus den Constraints) in folgender Form:
 * f, $i, $j, 0, ... , 0, k
 * wobei
 * f Funktionssymbol
 * $i, $j die, schon numerierten Teilterme von f
 * 0, ... , 0 auffuellen bis SO_MaximaleStelligkeit
 * k Nummer dieses Terms beginnend bei 1
 *
 * memos.memo[0] ist ein Zwischenspeicher  */
typedef struct MemoTabT {
  unsigned short maxN;  /* maximale Anzahl moeglicher Eintraege in MemoTab */
  unsigned short aktI;  /* aktueller Fuellstand */
  unsigned short msize;  /* Groesse der einzelnen Eintraege */
  unsigned short *memo;  /* Zeiger auf Memoeintraege */
  TermT *terme;          /* Zeiger auf die Terme */
  /* spaeter zusaetzlich: Zeiger auf Relations-Matrix */
} MemoTab;


static MemoTab memos; /* Existiert genau _einmal_ pro Waldmeisterlauf. */


/* Wenn MemoTab schon initialisiert wurde,
 * wird nur der Fuellstand auf 1 gesetzt. */
static void initMemoTab(void)
/* Erzeugt Verwaltungsblock und allokiert Raum fuer Elemente */
{
  /* Wurde MemoTab schon initialisiert ? */
  if (memos.maxN == 0){
    memos.maxN  = 128; /* TO-DO: schmiert mit 2 ab, default: 128 */
    memos.msize = SO_MaximaleStelligkeit + 2; /* +2 wegen: (Symb + #TT-Symb + k) */
    /* sizeof(Fkt-Symbol) = sizeof(TT-Symbol) = sizeof(k) = sizeof(unsigned short) */
    memos.memo  = our_alloc(memos.maxN * memos.msize * sizeof(unsigned short));
    memos.terme = our_alloc(memos.maxN * sizeof(TermT));
  }
  /* ggf. Bereich initialisieren */
  memos.aktI = 0;
}


/* verdoppelt die Groesse der MemoTab
 * veraendert Inhalt der bestehenden MemoTab _nicht_ */
static void extendMemoTab(void)
{
  memos.maxN  = memos.maxN * 2;
  memos.memo  = our_realloc(memos.memo, 
			    memos.maxN * memos.msize * sizeof(unsigned short),
			    "Extending MemoTab");
  memos.terme = our_realloc(memos.terme, memos.maxN * sizeof(TermT), "Extending MemoTab(2)");
  /* ggf. neuer Bereich initialisieren */
}


/* Traegt Teil eines Keys in die MemoTab ein. */
static void MemoTab_insertKeyPart(ExtraConstT eintragnummer, int aktStelle, ExtraConstT i)
{
  memos.memo[eintragnummer * memos.msize + aktStelle] = i;
}


/* Liest Teil eines Keys aus der MemoTab aus. */
static ExtraConstT MemoTab_readKeyPart(ExtraConstT eintragnummer, int aktStelle)
{
  return memos.memo[eintragnummer * memos.msize + aktStelle];
}


/* erhoeht i um 1 und vergroessert noetigenfalls MemoTab */
static ExtraConstT MemoTab_increaseI(void)
{
  if (memos.aktI == memos.maxN-1) {
    extendMemoTab();
  }
  return memos.aktI++;
}


/* Ueberprueft ob Eintrag mit Nummer i == Eintrag mit Nummer key */
static BOOLEAN MemoTab_Equals(ExtraConstT i, ExtraConstT key)
{
  int k;
  /* echt kleiner, weil ohne letztes Feld */
  for (k = 0; k < memos.msize-1; k++) {
    if (MemoTab_readKeyPart(i, k) != MemoTab_readKeyPart(key, k)) {
      return FALSE;
    }
  }
  return TRUE;
}


/* Enthaelt MemoTab schon einen _gleichen_ Eintrag wie den mit Nummer key ? 
 * Rueckgabe: Ja: Eintragnummer, Nein: 0  */
static ExtraConstT MemoTab_containsKey(ExtraConstT key)
{
  int i;
  
  for (i = 1; i <= memos.aktI; i++) {
    if (MemoTab_Equals(i, key)) {
      if ( i != key ) {
	return i;
      }
    }
  }
  return 0;
}


/* Kopiere Eintrag Nummer k1 an die Stelle Nummer k2.  */
static void MemoTab_copyKey(ExtraConstT k1, ExtraConstT k2)
{
  int k;

  for (k = 0; k <= memos.msize; k++) {
    MemoTab_insertKeyPart(k2, k, MemoTab_readKeyPart(k1, k));
  }
}


/* Traegt den Term selbst, sowie alle seine Teilterme rekursiv in MemoTab ein. */
static ExtraConstT MemoTab_insertTerm(UTermT t)
{
  unsigned int k;   /* aktuelle Stelle im Eintrag */
  ExtraConstT rck;  /* Variable fuer retwert von containsKey unten */
  ExtraConstT id;   /* Variable fuer retwert von TO_GetId(t) */
  UTermT t0, ti;    /* fuer Lauf ueber alle Teilterme */

  /* Wurde der _selbe_ (!= gleiche) Term schon numeriert? */
  id = TO_GetId(t);
  if (id != 0) {
    return id;
  }

  t0 = TO_NULLTerm(t);
  ti = TO_ErsterTeilterm(t);
  while (ti != t0) {   /* Numeriere rekursiv die Teilterme. */
    MemoTab_insertTerm(ti);
    ti = TO_NaechsterTeilterm(ti);
  }
  
  /* Schreibe den aktuellen Eintrag/Key in memo.memos[0].
   * memo.memos[0] ist ein Zwischenspeicher. */
  MemoTab_insertKeyPart(0,0,TO_TopSymbol(t));
  ti = TO_ErsterTeilterm(t);
  k = 1;
  while (ti != t0) {
    MemoTab_insertKeyPart(0,k, TO_GetId(ti));
    ti = TO_NaechsterTeilterm(ti);
    k++;
  }
  
  /* Fuelle den Rest des Eintrags, wenn arity(f) < SO_MaximaleStelligkeit,
   * mit Nullen auf. */
  while (k <= SO_MaximaleStelligkeit) {
    MemoTab_insertKeyPart(0,k,0);
    k++;
  }

  /* Existiert solch ein Eintrag schon ?
   * Ja: gebe t diese Nummer und liefere diese Nummer zurueck
   * Nein: weitermachen */ 
  rck = MemoTab_containsKey(0);
  if( rck != 0 ) {
    TO_SetId(t, rck);
    return rck;
  }

  MemoTab_increaseI();
  /* Der neue Eintrag bekommt die Nummer memos.aktI . */
  MemoTab_insertKeyPart(0, memos.msize-1, memos.aktI);
  /* Schreibe neuen Eintrag an das aktuelle Ende der MemoTab. */
  MemoTab_copyKey(0, memos.aktI);

  TO_SetId(t, memos.aktI);
  memos.terme[memos.aktI] = t;
  return memos.aktI;
}


/* Setzt die IDs von t und all seiner Teilterme zurueck auf 0 */
static void ResetTermIds(UTermT t)
{
  UTermT t0 = TO_NULLTerm(t);
  UTermT ti = t;
  while (ti != t0){
    TO_SetId(ti, 0);
    ti = TO_Schwanz(ti);
  }
}


/* Traegt kompletten Constraint (d.h. alle enthaltenen Terme) in die MemoTab ein. 
 * Davor werden die TermIDs zurueckgesetzt. */
static void MemoTab_insertECons(EConsT e)
{
  EConsT cons;

  for (cons = e ; cons != NULL ; cons = cons->Nachf){
    ResetTermIds(cons->s);
    ResetTermIds(cons->t);
  }
  
  for (cons = e ; cons != NULL ; cons = cons->Nachf){
    MemoTab_insertTerm(cons->s);
    MemoTab_insertTerm(cons->t);
  }
}


/* - RelTab - Relations-Matrix ----------------------------------------------- */

typedef struct RelTabT {
  unsigned short size;    /* verfuegbare _Seitenlaenge_ der Relations-Matrix */
  unsigned short aktSize; /* aktuell benutzte _Seitenlaenge_ der Relations-Matrix */
  RelT *rels;             /* Eintraege der Relations-Matrix */
} RelTab;


static RelTab relations; /* Existiert genau _einmal_ pro Waldmeisterlauf. */


static RelT RelTab_getRel(ExtraConstT sid, ExtraConstT tid)
{
  return relations.rels[sid*relations.aktSize + tid];
}


static void RelTab_print(void)
{
  int i,j;
  blah(("----------------------------------------\n"));
  blah(("   "));
  for (i = 1; i <= relations.aktSize; i++) {
#if ausgaben
    printf("%2d ", i);
#endif
  }
  blah(("\n"));
  for (i = 1; i <= relations.aktSize; i++) {
#if ausgaben
    printf("%2d ", i);
#endif
    for (j = 1; j <= relations.aktSize; j++) {
      if (RelTab_getRel(i,j) > bt){
	blah(("?? "));
      }
      else {
	blah(("%s ", RelTNames[RelTab_getRel(i,j)]));
      }
    }
    blah(("\n"));
  }
  blah(("----------------------------------------\n"));
}


/* Schreibt in alle Felder uk (= unknown) und in die Diagonale eq (= equal). */
static void resetRelTab(void)
{
  int i, j;
  for (i = 1; i <= relations.aktSize; i++) {
    for (j = 1; j <= relations.aktSize; j++) {
      relations.rels[relations.aktSize*i + j] = uk;
    }
    relations.rels[relations.aktSize*i + i] = eq;
  }
  blah(("resetRelTab():\n"));
  RelTab_print();
}


/* Die Relations-Matrix hat die Seitenlaenge = akt-Groesse-der-MemoTab.
 * Spalten / Zeilen sind Nummern der Terme (-> MemoTab). 
 * Zuvor muss MemoTab_insertECons aufgerufen werden. (wegen Groesse) */
static void initRelTab(void)
{
  blah(("initRelTab():\n"));

  /* Wurde RelTab schon initialisiert ? */
  if (relations.size == 0) {
    blah(("RelTab wird zum ersten mal initialisiert.\n"));    
    relations.size = memos.aktI;
    relations.rels = our_alloc((relations.size+1) * (relations.size+1) * sizeof(RelT));
  }   /* RelTab besteht also schon, ist sie gross genug? */
  else if (relations.size < memos.aktI) {
    blah(("RelTab war schon initialisiert, aber ist nicht gross genug: memos.aktI = %d > %d = relations.size\n", 
	  memos.aktI, relations.size));
    relations.size = memos.aktI;
    relations.rels = our_realloc(relations.rels, 
				 (relations.size+1) * (relations.size+1) * sizeof(RelT),
				 "Extending RelTab");
  }

  /* Die aktuell benutzte Seitenlaenge der RelTab muss angepasst werden. */
  relations.aktSize = memos.aktI;

  resetRelTab();
}


/* Lift-Kandidat
 * Eingabe: Zwei Terme mit gleichem Topsymbol 
 *          und mindestens eine vergleichbare Teiltermstelle. 
 * TRUE:  kein Widerspruch zu bereits Vorhandenen
 * FALSE: Widerspruch ! */
static BOOLEAN RelTab_LiftKandidat(ExtraConstT sid, ExtraConstT tid)
{
  RelT res = eq;
  RelT reli = uk;
  unsigned int i;
  /* Ueber alle Teilterme von sid und tid. */
  for (i = 1; i <= SO_Stelligkeit(TO_TopSymbol(memos.terme[sid])); i++){
    reli = RelTab_getRel(MemoTab_readKeyPart(sid, i), MemoTab_readKeyPart(tid, i));
    res = acctranstab(res, reli); /* Transtab beschreibt genau den Automaten. */
    if (res == uk){
      blah(("Liftgescheitert: %t %s %t weil: %t %s %t\n",
	    memos.terme[sid], RelTNames[reli], memos.terme[tid],
	    memos.terme[MemoTab_readKeyPart(sid, i)], RelTNames[reli],
	    memos.terme[MemoTab_readKeyPart(tid, i)]));
      return TRUE; /* Lift ist zwar gescheitert, jedoch nicht durch Widerspruch. */
    }
  }
  blah(("Lifterfolg: %t %s %t\n",
	memos.terme[sid], RelTNames[res], memos.terme[tid]));
  return RelTab_insertRel(res, sid, tid);
}


/* Lift
 * s > t oder s >= t oder s = t.
 * Sind diese Terme Teilterme eines anderen Terms an gleicher Stelle?
 * TRUE:  kein Widerspruch zu bereits Vorhandenen
 * FALSE: Widerspruch ! */
static BOOLEAN RelTab_Lift(ExtraConstT sid, ExtraConstT tid)
{
  int s;
  /* Ueber alle Eintraege der MemoTab. */
  for (s = 1; s <= memos.aktI; s++){ 
    unsigned int i;
    /* Ueber alle Teilterme eines Eintrags. */
    /* TODO: Genuegt i = sid + 1 */
    for (i = 1; i <= SO_Stelligkeit(TO_TopSymbol(memos.terme[s])); i++){ 
      /* Ist Term mit ID sid Teilterme von Term s? */
      if (MemoTab_readKeyPart(s,i) == sid){
	int t;
	/* Ueber alle Eintraege der MemoTab. */
	/* TODO: Genuegt t = tid + 1 */
	for (t = 1; t <= memos.aktI; t ++){
	  /* s wird ausgelassen. */
	  if (t == s){
	    continue;
	  }
	  /* Existiert ein anderer Term,
	   * der das gleiche Topsymbol wie s hat,
	   * und bei dem an der gleichen Teiltermstelle tid steht? */
	  if ((MemoTab_readKeyPart(t,0) == MemoTab_readKeyPart(s,0))
	      && (MemoTab_readKeyPart(t,i) == tid)){
	    blah(("Liftkandidat: %t ? %t Stelle: %d\n", memos.terme[s], memos.terme[t], i));
	    if(!RelTab_LiftKandidat(s, t)){
	      return FALSE;
	    }
	  }
	}
      }
    }
  }
  return TRUE;
}


/* Traegt eine einzelne Relation ein + Symmetrie,
 * TRUE: kein Widerspruch zu bereits Vorhandenen
 * FALSE: Widerspruch ! */
static BOOLEAN RelTab_insertRel(RelT rel, ExtraConstT sid, ExtraConstT tid)
{
  RelT oldrel, retrel;
  SymbolT ttop, stop;

  blah(("RelTab_insertRel( %s, %t [%d], %t [%d]):\n",
	RelTNames[rel], memos.terme[sid], sid, memos.terme[tid], tid));

  /* Strengthening:
   * Soll s >= t oder s <= t.
   * Wenn (top(s) != top(t)) und (top(s) und top(t) Fkt-Symgole),
   * dann muss gelten s > t bzw. s < t. */
  if ((rel == ge) || (rel == le)) {
    SymbolT stop = (SymbolT)MemoTab_readKeyPart(sid,0);
    SymbolT ttop = (SymbolT)MemoTab_readKeyPart(tid,0);

    if (SO_SymbIstFkt(stop) && SO_SymbIstFkt(ttop) && (stop != ttop)){
      if(rel == ge) {
	blah(("Strengthening [%d,%d]: %t >= %t  ->  %t > %t\n", 
	      sid,tid,memos.terme[sid], memos.terme[tid], memos.terme[sid], memos.terme[tid]));
	rel = gt;
      } else { /* (rel == le) */
	blah(("Strengthening [%d,%d]: %t <= %t  ->  %t < %t\n", 
	      sid,tid,memos.terme[sid], memos.terme[tid], memos.terme[sid], memos.terme[tid]));
	rel = lt;
      }
    }
  }

  oldrel = RelTab_getRel(sid, tid);
  retrel = accdomtab(oldrel, rel);

  blah(("DomTab ( %s, %s) =  %s\n", 
	RelTNames[oldrel], RelTNames[rel], RelTNames[retrel]));

  /* Widerspruch gefunden ! */
  if (retrel == bt) {
    blah(("$$$ PENG: RelTab_insertRel():\n war: %s, soll: %s   --> BOTTOM\n",
		  RelTNames[RelTab_getRel(sid, tid)], RelTNames[rel]));
    RelTab_print();
    return FALSE;
  }

  /* Nur _neues_ Wissen eintragen. */
  if (oldrel != retrel) {
    relations.rels[sid*relations.aktSize + tid] = retrel;
    
    /* TODO: Symmetrie: noch Ueberlegen! 
     * [kein Fehler?] insertRel sollte auch beim symmetrischen Punkt den
     * Wert berechnen. Die transitive Huelle wird zwischenzeitlich verm. zu
     * asymmetrischen Rels fuehren.  Eher kein Problem.*/
    relations.rels[tid*relations.aktSize + sid] = swap(retrel);

    /* Weitere Inferenzen:
     * Topsymbolclash, Decompose, Pacman */
    stop = (SymbolT)MemoTab_readKeyPart(sid,0);
    ttop = (SymbolT)MemoTab_readKeyPart(tid,0);

    if (SO_SymbIstFkt(stop) && SO_SymbIstFkt(ttop)){

      /* Soll s = t, und top(s) und top(t) sind Fkt-Symbole.
       * Wenn top(s) != top(t), dann CLASH ! */
      if (retrel == eq) {
	if (stop != ttop) {
	  blah(("Topsymbolclash: %t = %t\n", memos.terme[sid], memos.terme[tid]));
	  RelTab_print();
	  return FALSE;
	}
	else { /* Hier top(s) == top(t), und top(s) und top(t) sind Fkt-Symbole,
		  dann muss auch gelten ss = ts. */
	  int i;
	  blah(("Decompose: %t %s %t\n", memos.terme[sid], RelTNames[retrel], memos.terme[tid]));
	  for (i = 1; i < memos.msize - 1; i++){
	    if (MemoTab_readKeyPart(sid,i) == 0){
	      break;
	    }
	    if (!RelTab_insertRel(eq,MemoTab_readKeyPart(sid,i),MemoTab_readKeyPart(tid,i))){
	      RelTab_print();
	      return FALSE;
	    }
	  }
	}
      } else if ((stop == ttop) && (SO_Stelligkeit(stop) == 1)) {
	/* Soll s >=, <=, > oder < t sein, und top(s) und top(t) sind Fkt-Symbole,
	 * top(s) == top(t) und top(s) und top(t) sind _einstellig_  --> PACMAN 
	 * also f(s) ? f(t), dann s ? t */
	blah(("Pacman: %t %s %t\n", memos.terme[sid], RelTNames[retrel], memos.terme[tid]));
	if (!RelTab_insertRel(rel,MemoTab_readKeyPart(sid,1),MemoTab_readKeyPart(tid,1))){
	  RelTab_print();
	  return FALSE;
	}
      }
    }

    /* Lift-Inferenz */
    blah(("Liftversuch: %t %s %t\n", memos.terme[sid], RelTNames[retrel], memos.terme[tid]));
    if (!RelTab_Lift(sid, tid)){
      RelTab_print();
      return FALSE;
    }

  }
  RelTab_print();
  return TRUE; /* Einfuegen ohne Widersprueche erfolgreich abgeschlossen. */
}


/* Traegt das Wissen ueber die _direkte_ Teilterm-Relation aus der MemoTab 
 * in die RelTab ein. */
static BOOLEAN RelTab_copyTTfromMemTab(void)
{
  int i,j;
  /* alle Eintraege in der MemoTab */
  for (i = 1; i <= memos.aktI; i++){
    /* alle Teilterme pro Eintrag */
    for (j = 1; j < memos.msize-1; j++){
      /* Ende der Teilterme */
      if (MemoTab_readKeyPart(i, j) == 0) {
	break;
      }
      if (!RelTab_insertRel(gt, i, MemoTab_readKeyPart(i,j))) {
	return FALSE;
      }
    }
  }
  return TRUE;
}


/* Fuegt einen kompletten Constraint (d.h. alle enthalten atomaren Constraints)
 * in die Relations-Matrix ein. 
 * TRUE wenn es keine Widersprueche gab, 
 * FALSE sonst */
static BOOLEAN RelTab_insertECons(EConsT e)
{
  EConsT cons;
  for (cons = e ; cons != NULL ; cons = cons->Nachf){
    if (!RelTab_insertRel(convcons(cons->tp), TO_GetId(cons->s), TO_GetId(cons->t))) {
      return FALSE;
    }
  }
  return TRUE;
}

/* Berechnet den transitiven Abschluss der RelTab nach Warshall. */
static BOOLEAN RelTab_transitiveClosure(void)
{
  int i,j,k;
  for (i = 1; i <= relations.aktSize; i++){
    for (j = 1; j <= relations.aktSize; j++){
      for (k = 1; k <= relations.aktSize; k++){
        RelT rhoik = RelTab_getRel(i,k);
        RelT rhokj = RelTab_getRel(k,j);
        RelT rho = acctranstab(rhoik,rhokj);
        if (rho != uk){
          blah(("Trans: rho[%d,%d] = '%s', rho[%d,%d] = '%s' -> ", i, k, RelTNames[rhoik], k, j, RelTNames[rhokj]));
          if (!RelTab_insertRel(rho,i,j)){
            return FALSE;
          }
        }
      }
    }
  }
  return TRUE;
}


/* - Afterburner ------------------------------------------------------------- */

/* Benutzt aktuelle Ordnung, um Widersprueche zu finden. */
static BOOLEAN RelTab_Afterburner(void)
{
  /* typedef enum { Groesser, Kleiner, Gleich, Unvergleichbar} VergleichsT; */
  /* typedef enum {uk,eq,gt,lt,ge,le,bt} RelT; */
  int i,j;
  RelT r;
  VergleichsT v;
  for (i = 1; i <= relations.aktSize; i++){
    for (j = 1; j < i; j++){ /* Nur unteres Dreieck testen. (Symmetrie) */

      blah(("Afterburner(%d,%d): %t %s %t \n", 
	    i, j, memos.terme[i], RelTNames[RelTab_getRel(i,j)], memos.terme[j]));
      
      r = RelTab_getRel(i,j);
      v = ORD_VergleicheTerme(memos.terme[i],memos.terme[j]);

      if (r == gt && !((v == Groesser) || v == Unvergleichbar)) {
	blah(("Afterburner: %t > %t, Ordnung: Geht nicht!\n",
	      memos.terme[i], memos.terme[j]));
	return FALSE;
      } 
      /* Jetzt notwendig, da nur noch halbe Matrix getestet wird. */
      else if (r == lt && !((v == Kleiner) || v == Unvergleichbar)){
	blah(("Afterburner: %t < %t, Ordnung: Geht nicht!\n",
	      memos.terme[i], memos.terme[j]));
	return FALSE;
      } /* Bringt nichts! Wieso?
      else if (r == ge && !((v == Groesser) || (v == Gleich) || (v == Unvergleichbar))) {
	blah(("Afterburner: %t >= %t, Ordnung: Geht nicht!\n",
	      memos.terme[i], memos.terme[j]));
	return FALSE;
      } */
    }
  }
  return TRUE;
}


/* --------------------------------------------------------------------------- */


BOOLEAN Suff_satisfiable(EConsT e)
{
  int i;
  int k;
  ExtraConstT wert;

  initMemoTab();
  
  blah(("### MemoTab einfuegen: %E\n", e));

  MemoTab_insertECons(e);

  blah(("### MemoTab:\n"));

  for( i = 1; i <= memos.aktI; i++ ) {
    blah(("Eintrag Nr. %d: ", i));
    for( k = 0; k < memos.msize; k ++ ) {
      wert = MemoTab_readKeyPart(i, k);
      blah(("%d ", wert));
    }
    blah(("   %t\n", memos.terme[i]));
  }
  blah(("###\n"));

  initRelTab(); /* muss _nach_ MemoTab_insertECons sein! */

  blah(("\n%%% RelTab_insertECons\n"));

  return RelTab_copyTTfromMemTab() &&
         RelTab_insertECons(e) &&
         RelTab_transitiveClosure() &&
         RelTab_Afterburner();
}


/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
