/* SuffSolver2.c
 * 16.09.2003
 * Christian Schmidt
 * 
 * Version die ein Beweisobjekt und Backtracking realisiert.
 */

#include "SuffSolver.h"
#include "Ausgaben.h"
#include "SymbolOperationen.h"
#include "TermOperationen.h"
#include "Ordnungen.h"  /* Fuer den Afterburner */
#include "DSBaumOperationen.h" /* fuer NewestVar */
#include "Praezedenz.h" /* fuer LPO Inferenzen */
#include "Unifikation1.h" /* fuer Strengthening */
#include "KBOSuffSolver.h" /* Fuer den KBO-Afterburner */
#include <limits.h> /* Fuer KBO-Vortests */
#include "SymbolGewichte.h" /* Fuer KBO-Vortests */

/* - Parameter -------------------------------------------------------------- */

#define ausgaben 0 /* 0, 1 */

static BOOLEAN Suff_flag = TRUE;

static BOOLEAN kbo_pack = TRUE, wPraetest = TRUE, wPostest = TRUE, omega_burn = FALSE;

/* - Macros ----------------------------------------------------------------- */

#if ausgaben
#define blah(x) {if (Suff_flag) { IO_DruckeFlex x ; } }
#else
#define blah(x)
#endif

#define accdomtab(rel1,rel2) domtab[(rel1)*6+(rel2)]
#define acctranstab(rel1,rel2) transtab[(rel1)*6+(rel2)]
#define accmaxtab(rel1,rel2) maxtab[(rel1)*6+(rel2)]
#define swap(rel) swaptab[(rel)]
#define convcons(rel) converttab[(rel)]

#define NewestVar ((-10 < 2 * BO_NeuesteBaumvariable) ? -10 : \
                   2 * BO_NeuesteBaumvariable)

/* - Nuetzliche Tabellen, etc. ---------------------------------------------- */

/* Inferenztyp fuer die Eintraege in die LogTab. 
 * tr=trans, tt=teilterm, bo=bottom, lf=lift, pm=pacman, st=strengthening,
 * dc=decompose, tc=Topsymbolclash */
typedef enum {tr,tt,bo,lf,pm,st,dc,tc,co,lpo_alpha_pos,lpo_beta_pos,
              lpo_gamma_pos,ord,kbo_auf,kbo_auf3,kbo_ab_1,kbo_ab_2} InfT;
/* Fuer schoene Ausgaben. */
static char *InfTNames[] = {"Trans","Teilterm","Bottom","Lift","Pacman",
			    "Strengthening","Decompose","Topsymbolclash",
			    "Constraint","LPO_Alpha_Positiv","LPO_Beta_Positiv",
          "LPO_Gamma_Positiv","Ordnung","KBO_aufwaerts1+2",
          "KBO_aufwaerts3","KBO_abwaerts1","KBO_abwaerts2"};

typedef enum {uk,eq,gt,lt,ge,le,bt} RelT;
/* Fuer schoene Ausgaben. */
static char *RelTNames[] = {" ."," ="," >"," <",">=","<=","!!"};

static unsigned char domtab[] = {
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

/* Spalte: neu, Zeile: alt 
 * Eingabe: gt, ge, eq 
 * Ausgabe: max{alt,neu} 
 *          oder bt falls Eingabe nicht in {gt, ge, eq} */
static char maxtab[] = {
  bt,bt,bt,bt,bt,bt,
  bt,eq,gt,bt,ge,bt,
  bt,gt,gt,bt,gt,bt,  
  bt,bt,bt,bt,bt,bt,      
  bt,ge,gt,bt,ge,bt,
  bt,bt,bt,bt,bt,bt
};

/* Diese Tabelle konvertiert die enum Constraint (Constraint.h) nach RelT */
static char converttab[] = {
  gt,eq,ge
};

/* Konvertiert den VergleichsT nach RelT */
static RelT translate(VergleichsT v)
{
  if (v == Groesser){
    return gt;
  } else if (v == Kleiner){
    return lt;
  } else if (v == Gleich){
    return eq;
  } else if (v == Unvergleichbar){
    return uk;
  }
  return uk;
}
        


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
    memos.msize = SO_MaximaleStelligkeit + 2; /* +2 wegen: (Symb+ #TT-Symb +k)*/
    /* sizeof(Fkt-Symbol)=sizeof(TT-Symbol)=sizeof(k)=sizeof(unsigned short) */
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
  memos.terme = our_realloc(memos.terme, memos.maxN * sizeof(TermT),
			    "Extending MemoTab(2)");
  /* ggf. neuer Bereich initialisieren */
}


/* Traegt Teil eines Keys in die MemoTab ein. */
static void MemoTab_insertKeyPart(ExtraConstT eintragnummer, int aktStelle,
				  ExtraConstT i)
{
  memos.memo[eintragnummer * memos.msize + aktStelle] = i;
}


/* Liest Teil eines Keys aus der MemoTab aus. */
static ExtraConstT MemoTab_readKeyPart(ExtraConstT eintragnummer, int aktStelle)
{
  return memos.memo[eintragnummer * memos.msize + aktStelle];
}


/* erhoeht i um 1 und vergroessert noetigenfalls MemoTab */
static void MemoTab_increaseI(void)
{
  if (memos.aktI == memos.maxN-1) {
    extendMemoTab();
  }
  memos.aktI++;
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

  for (k = 0; k < memos.msize; k++) {
    MemoTab_insertKeyPart(k2, k, MemoTab_readKeyPart(k1, k));
  }
}


/* Traegt den Term selbst, sowie alle seine Teilterme rekursiv in MemoTab ein.*/
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


/* Traegt kompletten Constraint (d.h. alle enthaltenen Terme) 
 * in die MemoTab ein. 
 * Davor werden die TermIDs zurueckgesetzt. */
static void MemoTab_insertECons(EConsT e)
{
  EConsT cons;

  blah(("### MemoTab_insertECons: %E\n", e));

  for (cons = e ; cons != NULL ; cons = cons->Nachf){
    ResetTermIds(cons->s);
    ResetTermIds(cons->t);
  }
  
  for (cons = e ; cons != NULL ; cons = cons->Nachf){
    MemoTab_insertTerm(cons->s);
    MemoTab_insertTerm(cons->t);
  }
}


static void MemoTab_print(void)
{
  int i,k;
  ExtraConstT wert;

  blah(("### MemoTab:\n"));

  for( i = 1; i <= memos.aktI; i++ ) {
    blah(("Eintrag Nr. %d: ", i));
    for( k = 0; k < memos.msize; k ++ ) {
      wert = MemoTab_readKeyPart(i, k);
      blah(("%d ", wert));
    }
    blah(("   %t\n", memos.terme[i]));
  }
}


/* -- LogTab - Speichert Informationen fuer Beweise und Backtracking -------- */

/* Die LogTab ist ein Protokoll der Veraenderung der RelTab.
 * #Eintrag | #Zeile | #Spalte | Rel vorher | Rel nachher | Inferenzname
 * | benutztes Wissen | usedFlag
 * #Zeile, #Spalte: Welcher Eintrag in der RelTab wurde veraendert?
 * RelT vorher: Was Stand dort vorher?
 * RelT nachher: Was wurde in diesem Schritt eingetragen?
 * benutztes Wissen: Auf welche(n) Eintra(e)g(e) stuetzt sich dieser Schritt? 
 *                   (-> Nummer(n) in LogTab) Maximal sind das 
 *                   SO_MaximaleStelligkeit + 1 viele. (Bei Lift)
 *                   kein Eintrag: 0 
 * usedFlag: im Beweis benutzt? 1=ja, 0=nein (Fuer die Ausgabe des Beweises */
typedef struct LogTabT {
  unsigned short maxN;  /* maximale Anzahhl moeglicher Eintraege in LogTab */
  unsigned short aktF;  /* aktueller Fuellstand */
  unsigned short aktA;  /* aktuell zu bearbeitender Eintrag 
			   (alle davor sind bereits berarbeitet) */
  unsigned short lsize; /* Groesse der einzelnen Eintraege */
  unsigned short *logs;  /* Zeiger auf die Eintrage */
  ExtraConstT* reasons; /* Wird zur Uebergabe der Begruendungen 
			   an die LogTab benoetigt. */
  unsigned int reasonsSize;
} LogTab;


static LogTab logTab; /* Existiert genau _einmal_ pro Waldmeisterlauf. */

/* Initialisiert die reasons mit 0. */
static void LogTab_initReasons(void)
{
  unsigned short i;
  for (i = 0; i < logTab.reasonsSize; i++){
    logTab.reasons[i] = 0;
  }
}


/* Wenn die LogTab schon initialisiert wurde,
 * werden nur aktF und aktA auf 0 gesetzt, sowie reasons zurueckgesetzt. */
static void LogTab_init(void)
/* Erzeugt Verwaltungsblock und allokiert Raum fuer Elemente */
{
  blah(("LogTab_init:\n"));
  /* Wurde die LogTab schon initialisiert? */
  if (logTab.maxN == 0){
    logTab.maxN = 128;
    /* Lift braucht SO_MaximaleStelligkeit+1, andere Inferenzen mindestens 2  */
    logTab.reasonsSize = 
      SO_MaximaleStelligkeit >= 2 ? SO_MaximaleStelligkeit+1 : 2;
    logTab.lsize = 1 + 1 + 1 + 1 + 1 + 1 + logTab.reasonsSize +1;
    logTab.logs = our_alloc(logTab.maxN * logTab.lsize *sizeof(unsigned short));
    logTab.reasons = our_alloc(logTab.reasonsSize * sizeof(unsigned short)); 
  }
  logTab.aktF = 0;
  logTab.aktA = 1;
  LogTab_initReasons();
}


/* Verdoppelt die Groesse der LogTab.
 * Veraendert den Inhalt der bestehenden LogTab _nicht_. */
static void LogTab_extend(void)
{
  logTab.maxN = logTab.maxN * 2;
  logTab.logs = our_realloc(logTab.logs,
			   logTab.maxN * logTab.lsize * sizeof(unsigned short),
			    "Extending LogTab");
}


/* Traegt Teil eines Keys in die LogTab ein. */
static void LogTab_insertLogPart(ExtraConstT eintragnummer, int aktStelle,
				 ExtraConstT i)
{
  logTab.logs[eintragnummer * logTab.lsize + aktStelle] = i;
}


/* Liest Teil eines Keys aus der LogTab aus. */
static ExtraConstT LogTab_readLogPart(ExtraConstT eintragnummer, int aktStelle)
{
  return logTab.logs[eintragnummer * logTab.lsize + aktStelle];
}


static ExtraConstT LogTab_getTermID_s(ExtraConstT eintragnummer)
{
  return LogTab_readLogPart(eintragnummer, 1);
}


static ExtraConstT LogTab_getTermID_t(ExtraConstT eintragnummer)
{
  return LogTab_readLogPart(eintragnummer, 2);
}


static RelT LogTab_getOldRel(ExtraConstT eintragnummer)
{
  return LogTab_readLogPart(eintragnummer, 3);
}


static RelT LogTab_getNewRel(ExtraConstT eintragnummer)
{
  return LogTab_readLogPart(eintragnummer, 4);
}


static InfT LogTab_getInf(ExtraConstT eintragnummer)
{
  return LogTab_readLogPart(eintragnummer, 5);
}


/* Setzt das usedFlag auf i */
static void LogTab_setUsedFlag(ExtraConstT eintragnummer, ExtraConstT i)
{
  LogTab_insertLogPart(eintragnummer, 6+SO_MaximaleStelligkeit+1, i);
}


static ExtraConstT LogTab_getUsedFlag(ExtraConstT eintragnummer)
{
  return LogTab_readLogPart(eintragnummer, 6+SO_MaximaleStelligkeit+1);
}


/* erhoeht logTab.aktF um 1 und vergroessert noetigenfalls die LogTab */
static ExtraConstT LogTab_increaseAktF(void)
{
  if (logTab.aktF == logTab.maxN-1) {
    LogTab_extend();
  }
  return logTab.aktF++;
}


static void LogTab_print(void)
{
  int i,j;
  ExtraConstT wert;
  
  blah(("### LogTab: Anfang\n"));
  /* alle Eintraege der LogTab */
  for (i = 1; i <= logTab.aktF; i++){
    blah(("Eintrag Nr. %d: ", i));
    /* alle Teile des einzelnen Eintrags */
    for (j = 0; j < logTab.lsize-1; j++){
      /* Der letzte Eintrag ist das usedFlag, hier uninteressant. Deswegen -1 */
      if ((j == 3) || (j == 4)){
	blah(("%s ", RelTNames[LogTab_readLogPart(i,j)]));
      }
      else if (j == 5){
	blah(("%s ", InfTNames[LogTab_readLogPart(i,j)]));
      }
      else {
	if (j == logTab.lsize-1){
	  blah(("used: (sollte 0 sein) "));
	}
	wert = LogTab_readLogPart(i,j);
	blah(("%d ", wert));
      }
    }
    if (i == logTab.aktA){
      blah(("  <-- aktuell zu bearbeitender Eintrag"));
      blah((" (alle davor sind bereits berarbeitet)"));
    }
    if (i == logTab.aktF){
      blah(("  <-- aktueller Fuellstand"));
    }
    blah(("\n"));
  }
  blah(("### LogTab: Ende\n"));
}


/* - RelTab - Relations-Matrix ---------------------------------------------- */

typedef struct RelTabT {
  unsigned short size;    /* maximal verfuegbare _Seitenlaenge_ 
			     der Relations-Matrix */
  unsigned short aktSize; /* aktuell benutzte _Seitenlaenge_ 
			     der Relations-Matrix */
  /* Eintraege der Relations-Matrix und Hinweis auf LogTab. */
  struct {RelT rel; unsigned short logEntry;} *rels; 
  EConsT cons;            /* zu loesender Constraint,
			     wird fuer die Beweisausgabe benoetigt. */
} RelTab;


static RelTab relations; /* Existiert genau _einmal_ pro Waldmeisterlauf. */


static RelT RelTab_getRel(ExtraConstT sid, ExtraConstT tid)
{
  return relations.rels[sid*relations.aktSize + tid].rel;
}


static unsigned short RelTab_getLogEntry(ExtraConstT sid, ExtraConstT tid)
{
  return relations.rels[sid*relations.aktSize + tid].logEntry;
}


static void RelTab_print(void)
{
  int i,j;
  blah(("### RelTab:\n"));
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

  blah(("\nVerweise auf LogTab:\n"));
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
    for (j = 1; j <= relations.aktSize; j++) {
      printf("%2d ", RelTab_getLogEntry(i,j));
    }
#endif
    blah(("\n"));
  }
  blah(("----------------------------------------\n"));
}


/* Schreibt in alle Felder uk (= unknown) und in die Diagonale eq (= equal). 
 * Der Verweis auf die LogTab wird auf 0 gesetzt.*/
static void resetRelTab(void)
{
  int i, j;
  blah(("resetRelTab():\n"));
  for (i = 1; i <= relations.aktSize; i++) {
    for (j = 1; j <= relations.aktSize; j++) {
      relations.rels[relations.aktSize*i + j].rel = uk;
      relations.rels[relations.aktSize*i + j].logEntry = 0;
    }
    relations.rels[relations.aktSize*i + i].rel = eq;
  }
  relations.cons = NULL;
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
    relations.rels = our_alloc((relations.size+1) * (relations.size+1) 
			       * sizeof(*relations.rels));
  }   /* RelTab besteht also schon, ist sie gross genug? */
  else if (relations.size < memos.aktI) {
    blah(("RelTab war schon initialisiert, aber ist nicht gross genug: "));
    blah(("memos.aktI = %d > %d = relations.size\n", 
          memos.aktI, relations.size));
    relations.size = memos.aktI;
    relations.rels = our_realloc(relations.rels,
				 (relations.size+1) * (relations.size+1) 
				 * sizeof(*relations.rels),
				 "Extending RelTab");
  }
  /* Die aktuell benutzte Seitenlaenge der RelTab muss angepasst werden. */
  relations.aktSize = memos.aktI;
  resetRelTab();
}


/* Gibt einen Beweis fuer die Unerfuellbarkeit der neuen Relation aus. */
static void RelTab_proofUnsatisfiability(unsigned short oldLogEntry){
  ExtraConstT aktLogEntry, i;
  RelT oldrel, newrel;
  ExtraConstT sid, tid;
  short anzCons, anzUsedCons; /* Fuer Statistik */
  EConsT tmpCons; /* Fuer Statistik */

  oldrel = LogTab_getOldRel(logTab.aktF); /* Letzter Eintrag */
  newrel = LogTab_getNewRel(logTab.aktF);
  sid = LogTab_getTermID_s(logTab.aktF);
  tid = LogTab_getTermID_t(logTab.aktF);

  RelTab_print();
  MemoTab_print();
  LogTab_print();

  blah(("BEWEIS:\n"));
  blah(("Widerspruechlicher Constraint:\n%E\n\n", relations.cons));

  blah(("Neuer Eintrag Nr. %d: %t %s %t ist Widerspruch zu: ",
	logTab.aktF, memos.terme[sid], RelTNames[newrel], memos.terme[tid]));
  blah(("Eintrag Nr. %d: %t %s %t !\n",
	oldLogEntry, memos.terme[sid], RelTNames[oldrel], memos.terme[tid]));
  
  /* usedFlag der beiden direkt widerspruechlichen Eintraege setzen. */
  LogTab_setUsedFlag(logTab.aktF, 1);
  LogTab_setUsedFlag(oldLogEntry, 1);

  /* alle Eintraege der LogTab durchgehen
   * wenn das usedFlag gesetzt ist, so wird der Eintrag ausgegeben
   * und die usedFlags der Begruendungen fuer diesen Eintrag gesetzt */
  for (aktLogEntry = logTab.aktF; aktLogEntry > 0; aktLogEntry--){
    if (LogTab_getUsedFlag(aktLogEntry) == 1){
      ExtraConstT akt_sid, akt_tid;
      InfT inf;
      akt_sid = LogTab_getTermID_s(aktLogEntry);
      akt_tid = LogTab_getTermID_t(aktLogEntry);
      inf = LogTab_getInf(aktLogEntry);

      /* Eintrag ausgeben */
      blah(("Eintrag Nr. %d: %t %s %t, wegen Inferenz %s ",
	    aktLogEntry, memos.terme[akt_sid], 
	    RelTNames[LogTab_getNewRel(aktLogEntry)], memos.terme[akt_tid], 
	    InfTNames[inf]));

      if (inf == kbo_auf){
        blah(("Begruendungen: KBO_WeightTab"));
      }
	    
      /* Die Inferenzen Constraint und Teilterm haben keine Begruendungen. 
       * Die Inferenz Ordnung auch nicht.*/
      if (inf != tt && inf != co && inf != ord && inf != kbo_auf){
        blah(("Begruendungen: "));
        /* usedFlag der Begruendungen setzen */
        /* fuer alle Begruendungen */
        for (i = 0; i < logTab.reasonsSize; i++){
          ExtraConstT reason;
          reason = LogTab_readLogPart(aktLogEntry, i+6);
          if (reason == 0) continue; /* Keine Begruendung  */
          blah(("%d. ", reason));
          LogTab_setUsedFlag(reason, 1);
        }
      }
      blah(("\n"));
    }
  }
  blah(("\n"));

  /* Fuer den Widerspruch benoetigte Teile des Constraints ausgeben. */
  /* alle Eintraege der LogTab durchgehen
   * wenn das usedFlag gesetzt ist und die Inferenz Constraint ist,
   * so wird der Eintrag ausgegeben. */
  blah(("Fuer den Widerspruch benoetigte Teile des Constraints: \n"));
  anzUsedCons = 0;
  for (aktLogEntry = logTab.aktF; aktLogEntry > 0; aktLogEntry--){
    if (LogTab_getUsedFlag(aktLogEntry) == 1){
      if (LogTab_getInf(aktLogEntry) == co){
	ExtraConstT akt_sid, akt_tid;
	anzUsedCons++;
	akt_sid = LogTab_getTermID_s(aktLogEntry);
	akt_tid = LogTab_getTermID_t(aktLogEntry);
	blah(("%t :%s: %t; ", memos.terme[akt_sid],
	      RelTNames[LogTab_getNewRel(aktLogEntry)], memos.terme[akt_tid]));
      }
    }
  }
  blah(("\n"));

  /* Anzahl der Atome im urspruenglichen Constraint berechnen: */
  /* TODO: Geht das eleganter? */
  anzCons = 0;
  for (tmpCons = relations.cons ; tmpCons != NULL ; tmpCons = tmpCons->Nachf){
    anzCons++;
  }
  blah(("anzCons: %d anzUsedCons: %d\n", anzCons, anzUsedCons));

  blah(("BEWEIS ENDE\n"));
}


/* Ueberprueft, ob die letzte Eintragung in die LogTab zu Widerspruechen fuehrt.
 * Benutzt dabei die folgenden Bottom-Inferenzen:
 * - mit domtab
 * - Topsymbolclash
 * -
 */
static BOOLEAN Inferenz_Bottom(unsigned short oldLogEntry){
  RelT oldrel, newrel;
  oldrel = LogTab_getOldRel(logTab.aktF); /* Letzter Eintrag */
  newrel = LogTab_getNewRel(logTab.aktF); /* Letzter Eintrag */
  
  /* Ist domtab einverstanden ? */
  if (accdomtab(oldrel, newrel) == bt){
    blah(("$$$ PENG: Inferenz_Bottom:\n war: %s, soll: %s   --> BOTTOM\n",
	  RelTNames[oldrel], RelTNames[newrel]));
    RelTab_proofUnsatisfiability(oldLogEntry);
    return FALSE;
  }

  /* Topsymbolclash ? 
   * Soll s = t, und top(s) und top(t) sind Fkt-Symbole.
   * Wenn top(s) != top(t), dann CLASH ! */
  if (newrel == eq) {
    SymbolT stop, ttop;
    ExtraConstT sid, tid;
    sid = LogTab_getTermID_s(logTab.aktF);
    tid = LogTab_getTermID_t(logTab.aktF);
    stop = (SymbolT)MemoTab_readKeyPart(sid,0);
    ttop = (SymbolT)MemoTab_readKeyPart(tid,0);
    if (SO_SymbIstFkt(stop) && SO_SymbIstFkt(ttop)){
      if (stop != ttop) {
	blah(("$$$ PENG: Inferenz_Bottom:\nTopsymbolclash: %t = %t\n", 
	      memos.terme[sid], memos.terme[tid]));
	RelTab_proofUnsatisfiability(oldLogEntry);
	return FALSE;
      }
    }
  }
  return TRUE;
}


/* Traegt eine einzelne Relation ein + Symmetrie.
 * Der Eintrag wird in der LogTab protokolliert. 
 * Die Begruendungen fuer die LogTab werden ueber das Array logTab.reasons
 * geholt.
 * Das usedFlag wird auf Null gesetzt.
 * Mit der Inferenz_Bottom wird ueberprueft, ob es dabei Widersprueche gibt:
 * (TRUE) oder nicht (FALSE).*/
static BOOLEAN RelTab_insertRel(ExtraConstT sid, ExtraConstT tid, 
				RelT newrel, InfT inf)
{  
  unsigned short i, oldLogEntry;
  RelT oldrel, retrel;
  

  blah(("RelTab_insertRel(sid= %d, tid= %d, newrel= %s, inf= %s)\n",
	sid, tid, RelTNames[newrel], InfTNames[inf]));

  oldrel = relations.rels[sid*relations.aktSize + tid].rel;
  oldLogEntry = relations.rels[sid*relations.aktSize + tid].logEntry;

  /* DomTab benutzen. Auch wenn retrel == bt, so wird dennoch erst eingetragen. 
   * Inferenz_Bottom erkennt dann spaeter den Widerspruch. 
   * Dieser umstaendliche Weg erleichtert die Beweisausgabe. */
  retrel = accdomtab(oldrel, newrel);
  if (retrel != bt) {
    blah(("DomTab ( %s, %s) =  %s\n", 
	  RelTNames[oldrel], RelTNames[newrel], RelTNames[retrel]));
    newrel = retrel;
  }

  /* Nur neues Wissen eintragen. */
  if (newrel == oldrel){
    blah(("Schon bekannt!\n"));
    LogTab_initReasons();
    return TRUE;
  }

  LogTab_increaseAktF();

  relations.rels[sid*relations.aktSize + tid].rel = newrel;
  /* Hinweis auf zugehoerigen Eintrag in der LogTab: */
  relations.rels[sid*relations.aktSize + tid].logEntry = logTab.aktF;
  /* Symmetrie eintragen: */
  relations.rels[tid*relations.aktSize + sid].rel = swap(newrel);
  /* Hinweis auf zugehoerigen Eintrag in der LogTab: */
  relations.rels[tid*relations.aktSize + sid].logEntry = logTab.aktF;

  /* In LogTab eintragen. TODO: Symmetrie eintragen ?? */
  LogTab_insertLogPart(logTab.aktF, 0, logTab.aktF); /*Eintragnummer eintragen*/
  LogTab_insertLogPart(logTab.aktF, 1, sid);         /* Zeile eintragen. */
  LogTab_insertLogPart(logTab.aktF, 2, tid);         /* Spalte eintragen. */
  LogTab_insertLogPart(logTab.aktF, 3, oldrel);      /* Rel vorher */
  LogTab_insertLogPart(logTab.aktF, 4, newrel);      /* Rel nachher */
  LogTab_insertLogPart(logTab.aktF, 5, inf);         /* Inferenzname */
  LogTab_setUsedFlag(logTab.aktF, 0);                /* usedFlag auf 0 setzen */
  
  /* Begruendungen eintragen. */
  for (i = 0; i < logTab.reasonsSize; i++){
    LogTab_insertLogPart(logTab.aktF, i+6, logTab.reasons[i]);
  }
  LogTab_initReasons();

  LogTab_print();
  RelTab_print();

  if (!Inferenz_Bottom(oldLogEntry)){
    return FALSE;
  }

  return TRUE;
}


/* Traegt das Wissen ueber die _transitive_ Teilterm-Relation aus der MemoTab 
 * in die RelTab ein. */
static void RelTab_copyTTfromMemTabTransitive(void)
{
  int i,j,k;

  blah(("RelTab_copyTTfromMemTabTransitive():\n"));

  /* alle Eintraege in der MemoTab */
  for (i = 1; i <= memos.aktI; i++){
    /* alle Teilterme pro Eintrag */
    for (j = 1; j < memos.msize-1; j++){
      /* Ende der Teilterme */
      if (MemoTab_readKeyPart(i, j) == 0) {
	break;
      }
      /* Direkten Teilterm eintragen. */
      blah(("### RelTab_insertRel(%d,MemoTab_readKeyPart(%d,%d)= %d, gt, tt)\n",
	    i, i, j, MemoTab_readKeyPart(i,j)));
      if (!RelTab_insertRel(i, MemoTab_readKeyPart(i,j), gt, tt)){
	blah(("### RelTab_copyTTfromMemTabTransitive: Arrrgh !"));
      }
    }
  }

  /* Transitivitaet der Teilterme eintragen. */
  for (i = 1; i <= relations.aktSize; i++){
    for (j = 1; j < i; j++){ /* Symmetrie ausnutzen */
      for (k = 1; k <= relations.aktSize; k++){
	if (k == i || k == j) continue; /*TODO: richtig? nicht mit sich selbst*/
        if (RelTab_getRel(i,k) == gt && RelTab_getRel(k,j) == gt){
	  logTab.reasons[0] = RelTab_getLogEntry(i,k);
	  logTab.reasons[1] = RelTab_getLogEntry(k,j);
	  blah(("### RelTab_insertRel(%d, %d, gt, tr)\n", i, j));
	  if (!RelTab_insertRel(i, j, gt, tr)){
	    blah(("### RelTab_copyTTfromMemTabTransitive: Arrrgh !"));
	  }
	} 
      }
    }
  }
}


/* Fuegt einen kompletten Constraint (d.h. alle enthalten atomaren Constraints)
 * in die RelTab ein. 
 * TRUE wenn es keine Widersprueche gab, 
 * FALSE sonst */
static BOOLEAN RelTab_insertECons(EConsT e)
{
  EConsT cons;
  blah(("RelTab_insertECons: %E\n", e));

  relations.cons = e; /* wird fuer Beweisausgabe benoetigt */

  for (cons = e ; cons != NULL ; cons = cons->Nachf){
    if (!RelTab_insertRel(TO_GetId(cons->s), TO_GetId(cons->t),
			  convcons(cons->tp), co)){
      return FALSE;
    }
  }
  return TRUE;
}


/* Berechnet den transitiven Abschluss der RelTab bzgl. dem neuen Eintrag. 
 * TRUE: Keine Widersprueche gefunden.
 * FALSE: Widerspruch gefunden. */
static BOOLEAN Inferenz_transitiveClosure(void)
{
  int i;
  ExtraConstT sid, tid;
  RelT newrel;

  blah(("Inferenz_transitiveClosure():\n"));

  newrel = LogTab_getNewRel(logTab.aktA);
  sid = LogTab_getTermID_s(logTab.aktA);
  tid = LogTab_getTermID_t(logTab.aktA);

  /* Die neue Kante geht von sid nach tid.
   * alle Kanten (i -> sid) koennen jetzt zu (i -> tid) verlaengert werden. */
  for (i = 1; i <= relations.aktSize; i++){
    if (i == sid || i == tid) continue;
    {RelT i_sid, i_tid;
    i_sid = RelTab_getRel(i, sid);
    i_tid = acctranstab(i_sid, newrel); /* newrel = getRel(sid, tid) */
    if (i_tid != uk){
      blah(("Trans: rho[%d,%d] = '%s', rho[%d,%d] = '%s' -> rho[%d,%d] = %s\n",
	    i, sid, RelTNames[i_sid], sid, tid, RelTNames[newrel], 
	    i, tid, RelTNames[i_tid]));	  
      logTab.reasons[0] = RelTab_getLogEntry(i, sid);
      logTab.reasons[1] = logTab.aktA;
      if (!RelTab_insertRel(i, tid, i_tid, tr)){
	return FALSE;
      }
    }}
  }

  /* Die neue Kante geht von sid nach tid.
   * alle Kanten (tid -> i) koennen jetzt zu (sid ->i) verlaengert werden. */
  for (i = 1; i <= relations.aktSize; i++){
    if (i == sid || i == tid) continue;
    {RelT tid_i, sid_i;
    tid_i = RelTab_getRel(tid, i);
    sid_i = acctranstab(newrel, tid_i); /* newrel = getRel(sid, tid) */
    if (sid_i != uk){
      blah(("Trans: rho[%d,%d] = '%s', rho[%d,%d] = '%s' -> rho[%d,%d] = %s\n",
	    sid, tid, RelTNames[newrel], tid, i, RelTNames[tid_i], 
	    sid, i, RelTNames[sid_i]));	  
      logTab.reasons[0] = logTab.aktA;
      logTab.reasons[1] = RelTab_getLogEntry(tid, i);
      if (!RelTab_insertRel(sid, i, sid_i, tr)){
	return FALSE;
      }
    }}
  }
  return TRUE;
}


/* Berechnet den transitiven Abschluss der RelTab nach Warshall. 
 * Wird nur zur Kontrolle verwendet: Deswegen nicht intuitive Rueckgabe.
 * Liefert immer TRUE zurueck.
 * Wird neues Wissen gefunden (schlecht), so gibt es eine Ausgabe. */
static BOOLEAN RelTab_transitiveClosure_check(void)
{
  int i,j,k;
  blah(("RelTab_transitiveClosure_check():\n"));

  for (i = 1; i <= relations.aktSize; i++){
    for (j = 1; j < i; j++){
      for (k = 1; k <= relations.aktSize; k++){
	if (k == i || k == j) continue; /* TODO richtig? */
        {RelT rhoik = RelTab_getRel(i,k);
        RelT rhokj = RelTab_getRel(k,j);
        RelT rho = acctranstab(rhoik,rhokj);
	/* Um wirklich neues Wissen zu sein, muss gelten:
	 * 1. Das neue Wissen steht so noch nicht in der RelTab.
	 * 2. Das neue Wissen wird nicht durch das bereits bekannte dominiert.*/
	if (accdomtab(RelTab_getRel(i,j), rho) != RelTab_getRel(i,j)){
	  blah(("Arrrgh! Schlimme Sache in RelTab_transitiveClosure_check:\n"));
          blah(("Trans: rho[%d,%d]='%s', rho[%d,%d]='%s' -> rho[%d,%d]='%s'\n",
		i, k, RelTNames[rhoik], k, j, RelTNames[rhokj], 
		i, j, RelTNames[rho]));
	}}
      }
    }
  }
  return TRUE;
}

/* ========================================================================== */
/* == allgemeine Inferenzen ================================================= */
/* ========================================================================== */

/* Strengthening:
 * Soll s >= t oder s <= t.
 * [alt: Wenn (top(s) != top(t)) und (top(s) und top(t) Fkt-Symgole),]
 * Wenn s und t nicht unifizierbar,
 * dann muss gelten s > t bzw. s < t.
 * TRUE: Entweder: kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */
static BOOLEAN Inferenz_Strengthening()
{
  ExtraConstT sid, tid;
  RelT rel;
/*   blah(("Strengtheningversuch\n")); */
  sid = LogTab_readLogPart(logTab.aktA, 1);
  tid = LogTab_readLogPart(logTab.aktA, 2);
  rel = LogTab_readLogPart(logTab.aktA, 4);
  if ((rel == ge) || (rel == le)) {
    if (!U1_Unifiziert(memos.terme[sid], memos.terme[tid])){
      blah(("U1_Unifiziert(%t, %t) = FALSE\n", 
            memos.terme[sid], memos.terme[tid]));
      if(rel == ge) {
        blah(("Strengthening [%d,%d]: %t >= %t  ->  %t > %t\n", 
              sid,tid,memos.terme[sid], memos.terme[tid],
              memos.terme[sid], memos.terme[tid]));
        logTab.reasons[0] = RelTab_getLogEntry(sid,tid);
        return RelTab_insertRel(sid, tid, gt, st);
      } else { /* (rel == le) */
        blah(("Strengthening [%d,%d]: %t <= %t  ->  %t < %t\n", 
              sid,tid,memos.terme[sid], memos.terme[tid],
              memos.terme[sid], memos.terme[tid]));
        logTab.reasons[0] = RelTab_getLogEntry(sid,tid);
        return RelTab_insertRel(sid, tid, lt, st);
      }
    }
  }
  return TRUE;
}


/* Soll s >=, <=, > oder < t sein, und top(s) und top(t) sind Fkt-Symbole,
 * top(s) == top(t) und top(s) und top(t) sind _einstellig_  --> PACMAN 
 * also: wenn f(s) ? f(t), dann s ? t
 * TRUE: Entweder: kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */
static BOOLEAN Inferenz_Pacman()
{
  ExtraConstT sid, tid;
  RelT rel;
  SymbolT stop, ttop;
/*   blah(("Pacmanversuch\n")); */
  sid = LogTab_readLogPart(logTab.aktA, 1);
  tid = LogTab_readLogPart(logTab.aktA, 2);
  stop = (SymbolT)MemoTab_readKeyPart(sid,0);
  ttop = (SymbolT)MemoTab_readKeyPart(tid,0);
  rel = LogTab_readLogPart(logTab.aktA, 4);

  if (SO_SymbIstFkt(stop) && SO_SymbIstFkt(ttop) 
      && (stop == ttop) && (SO_Stelligkeit(stop) == 1)){
    blah(("Pacman: %t %s %t\n", 
	  memos.terme[sid], RelTNames[rel], memos.terme[tid]));
    logTab.reasons[0] = RelTab_getLogEntry(sid,tid);
    return RelTab_insertRel(MemoTab_readKeyPart(sid,1), 
			    MemoTab_readKeyPart(tid,1), rel, pm);
  }
  return TRUE;
}


/* Lift-Kandidat
 * Eingabe: Zwei Terme mit gleichem Topsymbol 
 *          und mindestens eine vergleichbare Teiltermstelle. 
 * TRUE:  kein Widerspruch zu bereits Vorhandenen
 * FALSE: Widerspruch ! */
static BOOLEAN Inferenz_Lift_Kandidat(ExtraConstT sid, ExtraConstT tid)
{
  RelT res = eq;
  RelT reli = uk;
  unsigned int i;

  /* Wir muessen hier anfangen die reasons einzutragen,
     auch wenn Lift spaeter scheitern sollte. */
  logTab.reasons[0] = logTab.aktA;

  /* Ueber alle Teilterme von sid und tid. */
  for (i = 1; i <= SO_Stelligkeit(TO_TopSymbol(memos.terme[sid])); i++){
    reli = RelTab_getRel(MemoTab_readKeyPart(sid, i), 
			 MemoTab_readKeyPart(tid, i));
    res = acctranstab(res, reli); /* Transtab beschreibt genau den Automaten. */
    /* Wir muessen die reasons hier eintragen, 
       auch wenn Lift spaeter scheitern sollte. */
    logTab.reasons[i] = RelTab_getLogEntry(MemoTab_readKeyPart(sid, i), 
					   MemoTab_readKeyPart(tid, i));
    if (res == uk){
      blah(("Liftgescheitert: %t %s %t weil: %t %s %t\n",
	    memos.terme[sid], RelTNames[reli], memos.terme[tid],
	    memos.terme[MemoTab_readKeyPart(sid, i)], RelTNames[reli],
	    memos.terme[MemoTab_readKeyPart(tid, i)]));
      LogTab_initReasons();
      return TRUE; /*Lift ist zwar gescheitert, jedoch nicht durch Widerspruch*/
    }
  }
  blah(("Lifterfolg: %t %s %t\n",
	memos.terme[sid], RelTNames[res], memos.terme[tid]));
  return RelTab_insertRel(sid, tid, res, lf);
}


/* Lift
 * s > t oder s >= t oder s = t. (d.h. s und t sind vergleichbar)
 * Sind diese Terme Teilterme eines anderen Terms an gleicher Stelle?
 * Dann ist ein Lift moeglich. -> Liftkandidat
 * TRUE: Entweder: kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */
static BOOLEAN Inferenz_Lift(void)
{
  int s;
  ExtraConstT sid, tid;

/*   blah(("Liftversuch\n")); */
  sid = LogTab_getTermID_s(logTab.aktA);
  tid = LogTab_getTermID_t(logTab.aktA);

  /* Ueber alle Eintraege der MemoTab. */
  /* TODO: Genuegt s = sid + 1 */
  for (s = 1; s <= memos.aktI; s++){ 
    unsigned int i;
    /* Ueber alle Teilterme eines MemoTab-Eintrags. */
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
	  /* Hat t das gleiche Topsymbol wie s?
	   * Und steht bei t an der gleiche Stelle tid, an der in s sid steht?*/
	  if ((MemoTab_readKeyPart(t,0) == MemoTab_readKeyPart(s,0))
	      && (MemoTab_readKeyPart(t,i) == tid)){
	    blah(("Liftkandidat: %t ? %t Stelle: %d\n",
		  memos.terme[s], memos.terme[t], i));
	    if(!Inferenz_Lift_Kandidat(s, t)){
	      return FALSE;
	    }
	  }
	}
      }
    }
  }
  return TRUE;
}


/* Decompose
 * Soll sein: s = t, und top(s) und top(t) sind Fkt-Symbole.
 * Wenn top(s) == top(t), dann muss auch gelten ss = ts.
 * TRUE: Entweder kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */
static BOOLEAN Inferenz_Decompose(void)
{
  RelT newrel;
  newrel = LogTab_getNewRel(logTab.aktA); /* aktueller Eintrag */
  
  if (newrel == eq) {
    SymbolT stop, ttop;
    ExtraConstT sid, tid;
    sid = LogTab_getTermID_s(logTab.aktA);
    tid = LogTab_getTermID_t(logTab.aktA);
    stop = (SymbolT)MemoTab_readKeyPart(sid,0);
    ttop = (SymbolT)MemoTab_readKeyPart(tid,0);
    if (SO_SymbIstFkt(stop) && SO_SymbIstFkt(ttop)){
      if (stop == ttop) {
	int i;
	blah(("Decompose: %t %s %t\n",
	      memos.terme[sid], RelTNames[newrel], memos.terme[tid]));
	/* Fuer alle Teilterme */
	for (i = 1; i < memos.msize - 1; i++){ 
	  if (MemoTab_readKeyPart(sid,i) == 0){
	    break; /* keine Teilterme mehr da */
	  }
	  logTab.reasons[0] = logTab.aktA;
	  if (!RelTab_insertRel(MemoTab_readKeyPart(sid,i),
				MemoTab_readKeyPart(tid,i), eq, dc)){
	    return FALSE;
	  }
	}
      }
    }
  }
  return TRUE;
}

/* ========================================================================== */
/* == LPO-Inferenzen ======================================================== */
/* ========================================================================== */

/* Soll sein: s_i > t oder t < s_i
 * Wenn es gibt f(...s_i...), dann f(...s_i...) > t 
 * TRUE: Entweder: kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */
static BOOLEAN Inferenz_LPO_Alpha_Positiv(void)
{
  if (ORD_OrdIstLPO) {
    int term_id;
    ExtraConstT sid, tid, s_i, t;
    RelT rel;

//    blah(("Inferenz_LPO_Alpha_Positiv_Versuch\n"));
    sid = LogTab_getTermID_s(logTab.aktA);
    tid = LogTab_getTermID_t(logTab.aktA);
    rel = LogTab_getNewRel(logTab.aktA);

    /* wir wollen s_i > t oder t < s_i */
    if (rel == gt || rel == ge) {
      s_i = sid;
      t = tid;
    }
    else if (rel == lt || rel == le){
      s_i = tid;
      t = sid;
    }
    else { /* die Inferenz kann nicht greifen */
      return TRUE;
    }

    /* Ueber alle Eintraege der MemoTab. */
    /* TODO: Genuegt term_id = s_i + 1 */
    for (term_id = 1; term_id <= memos.aktI; term_id++){ 
      unsigned int teilterm_id;
      /* Ueber alle Teilterme eines MemoTab-Eintrags. */
      unsigned int stelligkeit;
      stelligkeit = SO_Stelligkeit(TO_TopSymbol(memos.terme[term_id]));
      for (teilterm_id = 1; teilterm_id <= stelligkeit; teilterm_id++){
        /* Ist Term mit ID s_i Teilterm von Term mit ID term_id? */
        if (MemoTab_readKeyPart(term_id,teilterm_id) == s_i){
          blah(("Inferenz_LPO_Alpha_Positiv_Erfolg: %t %s %t -> %t > %t\n",
                memos.terme[sid], RelTNames[rel], memos.terme[tid],
                memos.terme[term_id], memos.terme[t]));
          logTab.reasons[0] = RelTab_getLogEntry(sid,tid);
          if (!RelTab_insertRel(term_id, t, gt, lpo_alpha_pos)) {
            return FALSE;
          }
        }
      }
    }
  }
  return TRUE;
}

/* Beantwortet die Frage: 
 * Ist s > t_i fuer alle i mit erster_tt <= i <= stelligkeit von top(t) ?
 * (Dabei ist erster_tt der erste zu testende Teilterm.)
 * sid, tid sind Term-IDs aus der Memo-Tab */
static BOOLEAN inferenz_LPO_Majorisierung(ExtraConstT sid, ExtraConstT tid,
                                          unsigned int erster_tt)
{
  unsigned int teilterm_id;
  /* Ueber alle Teilterme des MemoTab-Eintrags von tid. */
  unsigned int stelligkeit;
  stelligkeit = SO_Stelligkeit(TO_TopSymbol(memos.terme[tid]));

  if (erster_tt < 1 || erster_tt > stelligkeit){ /* Error-Handling */
    our_error("SuffSolver: inferenz_LPO_Majorisierung: ungueltige TT-Angabe!\n");
  }

  for (teilterm_id = erster_tt; teilterm_id <= stelligkeit; teilterm_id++){
    /* wir muessen hier die Begruendungen eintragen,
       auch, wenn wir scheitern */
    logTab.reasons[teilterm_id] = RelTab_getLogEntry(sid,
                                    MemoTab_readKeyPart(tid, teilterm_id));
    if (RelTab_getRel(sid, MemoTab_readKeyPart(tid, teilterm_id)) != gt){
      return FALSE;
    }
  }
  return TRUE;
}

/* Soll sein: s > t_j
 * Wenn es gibt f(...t_j...) mit top(s) >_F f und s > t_i i!=j,
 * dann s > f(...t_j...).
 * TRUE: Entweder: kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */
static BOOLEAN Inferenz_LPO_Beta_Positiv(void)
{
  if (ORD_OrdIstLPO) {
    RelT rel = LogTab_getNewRel(logTab.aktA);
    if (rel == gt || rel == lt){ /* sonst kann die Inferenz nicht greifen */
      ExtraConstT sid, tid, s, t_j;
      int term_id;
      sid = LogTab_getTermID_s(logTab.aktA);
      tid = LogTab_getTermID_t(logTab.aktA);
      
      blah(("Inferenz_LPO_Beta_Positiv_Versuch\n"));

      /* haben sid > tid oder sid < tid, erwarten unten s > t_j */
      if (rel == gt) {
        s = sid;
        t_j = tid;
      }
      else if (rel == lt){
        s = tid;
        t_j = sid;
      }

      /* Ueber alle Eintraege der MemoTab. */
      /* TODO: Genuegt term_id = s_i + 1 */
      for (term_id = 1; term_id <= memos.aktI; term_id++){ 
        unsigned int teilterm_id;
        /* Ueber alle Teilterme eines MemoTab-Eintrags. */
        unsigned int stelligkeit;
        stelligkeit = SO_Stelligkeit(TO_TopSymbol(memos.terme[term_id]));
        for (teilterm_id = 1; teilterm_id <= stelligkeit; teilterm_id++){
          /* Ist Term mit ID t_j Teilterm von Term mit ID term_id?
           * und top(s) >_F top(term_id) 
           * und s > t_i i!=j ? */
          if (MemoTab_readKeyPart(term_id, teilterm_id) == t_j
              && PR_Praezedenz(TO_TopSymbol(memos.terme[s]), 
                               TO_TopSymbol(memos.terme[term_id])) == Groesser
              && inferenz_LPO_Majorisierung(s, term_id, 1)){
            blah(("Inferenz_LPO_Beta_Positiv_Erfolg: %t %s %t -> %t > %t\n",
                  memos.terme[sid], RelTNames[rel], memos.terme[tid],
                  memos.terme[s], memos.terme[term_id]));
            logTab.reasons[0] = RelTab_getLogEntry(sid,tid);
            if (!RelTab_insertRel(s, term_id, gt, lpo_beta_pos)) {
              return FALSE;
            }
          }
        }
      }
    }
  }
  return TRUE;
}

/* Hilfsfunktion fuer den Gamma-Fall der LPO-Definition.
 * Fuehrt den lexikographischen Vergleich der LPO sowie die Majorisierung durch.
 * Die Rueckgabe ist die berechnete Relation zwischen sid und tid. */
static RelT inferenz_LPO_Gamma_Lexiko_Majo(ExtraConstT sid, ExtraConstT tid)
{
  RelT rho, rho_i;
  unsigned int stelligkeit, teilterm_id;
  ExtraConstT s_tt, t_tt;

  if (TO_TopSymbol(memos.terme[sid]) != TO_TopSymbol(memos.terme[tid])){
    our_error("SuffSolver: inferenz_LPO_Gamma_Lexiko_Majo: top(s) != top(t)\n");
  }

  rho = eq;
  stelligkeit = SO_Stelligkeit(TO_TopSymbol(memos.terme[sid]));
  for (teilterm_id = 1; teilterm_id <= stelligkeit; teilterm_id++){
    s_tt = MemoTab_readKeyPart(sid, teilterm_id);
    t_tt = MemoTab_readKeyPart(tid, teilterm_id);
    rho_i = RelTab_getRel(s_tt, t_tt);
    if (rho_i == gt || rho_i == ge || rho_i == eq){
      rho = accmaxtab(rho, rho_i);
      /* wir muessen hier die Begruendungen eintragen,
         auch, wenn wir scheitern */
      logTab.reasons[teilterm_id] = RelTab_getLogEntry(s_tt,
                                      MemoTab_readKeyPart(tid, teilterm_id));
    } else { /* Die Kette der gt, ge, eq ist zu Ende */
      /* es muss mindestens ein gt gegeben haben, sonst koennen wir aufhoeren */
      if (rho == gt && inferenz_LPO_Majorisierung(sid, tid, teilterm_id)){
        return rho;
      }
      return uk;
    }
  }
  
  /* Hier gilt: s_i rho t_i mit rho in {gt, ge, eq} fuer alle i */
  if (rho == gt){
    return rho;
  }
  return uk;
}

/* Soll sein: s rho t mit rho in {>, >=, =}.
 * Dieses neue Wissen kann an zwei Stellen im Gamma-Fall helfen:
 * 1. lexikographischer Vergleich: Wenn es gibt f(...s...) und f(...t...)
 * 2. Majorisierung: Wenn es gibt f(...t...)
 * TRUE: Entweder: kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */
static BOOLEAN Inferenz_LPO_Gamma_Positiv(void)
{
  if (ORD_OrdIstLPO) {
    RelT rel, newrel;
    ExtraConstT sid, tid, s, t;
    int term_id_1, term_id_2, term_id;
    sid = LogTab_getTermID_s(logTab.aktA);
    tid = LogTab_getTermID_t(logTab.aktA);
    rel = LogTab_getNewRel(logTab.aktA);

    blah(("Inferenz_LPO_Gamma_Positiv_Versuch\n"));
    
    /* Wir erwarten unten s rho t mit rho in {>, >=, =} */
    if (rel == lt || rel == le){
      rel = swap(rel);
      s = tid;
      t = sid;
    } else {
      s = sid;
      t = tid;
    }

//    blah(("%t %s %t\n", memos.terme[s], RelTNames[rel], memos.terme[t]));

    /* Fall 1: */
    /* Ueber alle Eintraege der MemoTab. */
    for (term_id_1 = 1; term_id_1 <= memos.aktI; term_id_1++){ 
      unsigned int teilterm_id_1;
      /* Ueber alle Teilterme eines MemoTab-Eintrags. */
      unsigned int stelligkeit_1;
      stelligkeit_1 = SO_Stelligkeit(TO_TopSymbol(memos.terme[term_id_1]));
      for (teilterm_id_1 = 1; teilterm_id_1 <= stelligkeit_1; teilterm_id_1++){
        /* Ist der Term mit ID s Teilterm von Term mit ID term_id_1? */
        if (MemoTab_readKeyPart(term_id_1, teilterm_id_1) == s){
          /* Gibt es einen Term term_id_2 mit top(term_id_1) == top(term_id_2)
             und term_id_2 hat an der gleichen Stelle wie term_id_1 t stehen? */
          /* Ueber alle Eintraege der MemoTab. */
          for (term_id_2 = 1; term_id_2 <= memos.aktI; term_id_2++){ 
            unsigned int teilterm_id_2;
            /* Ueber alle Teilterme eines MemoTab-Eintrags. */
            unsigned int stelligkeit_2;
            if (TO_TopSymbol(memos.terme[term_id_1]) 
                != TO_TopSymbol(memos.terme[term_id_2])){
              continue;
            }
            stelligkeit_2 = SO_Stelligkeit(TO_TopSymbol(memos.terme[term_id_2]));
            for (teilterm_id_2=1; teilterm_id_2<=stelligkeit_2; teilterm_id_2++){
              /* Ist der Term mit ID t Teilterm von Term mit ID term_id_2? */
              if (MemoTab_readKeyPart(term_id_2, teilterm_id_2) == t){
                if (teilterm_id_1 == teilterm_id_2){ /* gleiche Stelle? */
                  newrel = inferenz_LPO_Gamma_Lexiko_Majo(term_id_1, term_id_2);
                  if (newrel == gt){
                    blah(("Inferenz_LPO_Gamma_Positiv_Erfolg(Fall 1): %t %s %t -> %t > %t\n",
                          memos.terme[s], RelTNames[rel], memos.terme[t],
                          memos.terme[term_id_1], memos.terme[term_id_2]));
                    logTab.reasons[0] = RelTab_getLogEntry(sid,tid);
                    if (!RelTab_insertRel(term_id_1, term_id_2, gt, 
                                          lpo_gamma_pos)) {
                      return FALSE;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    /* Fall 2: */
    /* Ueber alle Eintraege der MemoTab. */
    for (term_id = 1; term_id <= memos.aktI; term_id++){ 
      unsigned int teilterm_id;
      /* Ueber alle Teilterme eines MemoTab-Eintrags. */
      unsigned int stelligkeit;
      /* Sonst kann Gamma-Fall nicht greifen. */
      if (TO_TopSymbol(memos.terme[s])
          != TO_TopSymbol(memos.terme[term_id])){
        continue;
      }
      stelligkeit = SO_Stelligkeit(TO_TopSymbol(memos.terme[term_id]));
      for (teilterm_id = 1; teilterm_id <= stelligkeit; teilterm_id++){
        /* Ist der Term mit ID t Teilterm von Term mit ID term_id? */
        if (MemoTab_readKeyPart(term_id, teilterm_id) == t){
          newrel = inferenz_LPO_Gamma_Lexiko_Majo(s, term_id);
          if (newrel == gt){
            blah(("Inferenz_LPO_Gamma_Positiv_Erfolg(Fall 2): %t %s %t -> %t > %t\n",
                  memos.terme[s], RelTNames[rel], memos.terme[t],
                  memos.terme[s], memos.terme[term_id]));
            logTab.reasons[0] = RelTab_getLogEntry(sid,tid);
            if (!RelTab_insertRel(s, term_id, gt, lpo_gamma_pos)) {
              return FALSE;
            }
          }
        }
      }
    }   
  }
  return TRUE;
}

static EConsT Inferenz_Lpo_Gamma_Test(void)
{
  SymbolT f, x1, x2;
  TermT s, t, x, y;
  EConsT econs = NULL;

  x1 = SO_ErstesVarSymb;
  x2 = SO_NaechstesVarSymb(x1);

  SO_forFktSymbole(f){
    if (SO_Stelligkeit(f) == 2){
      break;
    }
  }

  s = TO_(f,x1,x2);
  t = TO_(f,x2,x1);
  x = TO_(x1);
  y = TO_(x2);

  econs = CO_newEcons(y,x,econs,Gt_C);
  econs = CO_newEcons(s,t,econs,Gt_C);

  return econs;
}

static EConsT Inferenz_Lpo_Gamma_Test_2(void)
{
  SymbolT f, x1, x2, x3;
  TermT s, t, u, v;
  EConsT econs = NULL;

  x1 = SO_ErstesVarSymb;
  x2 = SO_NaechstesVarSymb(x1);
  x3 = SO_NaechstesVarSymb(x2);

  SO_forFktSymbole(f){
    if (SO_Stelligkeit(f) == 2){
      break;
    }
  }

  s = TO_(f,x2,x1);
  t = TO_(f,x1,x2);
  u = TO_(f,x1,f,x2,x3);
  v = TO_(f,x2,f,x1,x3);

  econs = CO_newEcons(s,t,econs,Gt_C);
  econs = CO_newEcons(u,v,econs,Gt_C);

  return econs;
}

/* ========================================================================== */
/* == KBO-Inferenzen ======================================================== */
/* ========================================================================== */


/* - Datenstrukturen und Funktionen fuer einen KBO-Vortest ------------------ */

/* Einzelner Eintrag in der KBO_WeightTab.
 * Der Eintrag enthaelt eine obere (also x_i <= max_w)  
 * und untere (also x_i >= min_w) Schranke fuer das Variablengewicht
 * und falls bekannt den exakten Wert (eq_w).
 * Invarianten:
 * 1 <= min_w <= max_w <= ULONG_MAX
 * min_w == max_w == eq_w  gdw  eq_w != 0 */
typedef struct KBO_WeightTabEntryT {
  unsigned long max_w;
  unsigned long min_w;
  unsigned long eq_w;
} KBO_WeightTabEntry;

/* Enthaelt fuer jede Variable einen Eintrag.
 * Der nullte-Eintrag ist leer, weil die Variablennummern mit 1 Anfangen. 
 * Gibt es die Variablen x_1 bis x_5, so ist size == 6 */
typedef struct KBO_WeightTabT {
  KBO_WeightTabEntry *entries;
  unsigned int size;
} KBO_WeightTab;

/* Existiert genau _einmal_ pro Waldmeisterlauf. */
static KBO_WeightTab KBO_wTab = {NULL, 0};

/* Groesse an aktuelle Variablenanzahl im System anpassen. */
static void KBO_WeightTab_anpassen(void)
{
  if (KBO_wTab.size <= SO_VarNummer(SO_NeuesteVariable)){
    /* Der nullte-Eintrag ist leer, weil die Variablennummern mit 1 Anfangen. */
    KBO_wTab.size = SO_VarNummer(SO_NeuesteVariable) + 1;
    KBO_wTab.entries = our_realloc(KBO_wTab.entries,
                                   KBO_wTab.size * sizeof(*KBO_wTab.entries),
                                   "KBO_wTab.entries");
  }
}

static void KBO_WeightTab_init(void)
{
  unsigned int i;
  KBO_WeightTab_anpassen();
  /* Der nullte-Eintrag ist leer, weil die Variablennummern mit 1 Anfangen. */
  for (i = 1; i < KBO_wTab.size; i++){ 
    KBO_wTab.entries[i].max_w = ULONG_MAX;
    KBO_wTab.entries[i].min_w = 1;
    KBO_wTab.entries[i].eq_w = 0; /* 0 bedeutet 'kein Eintrag' */
  }
}

/* Fuegt das neue Wissen in die WeightTab ein.
 * TRUE: Kein Widerspruch
 * FALSE: Widerspruch */
static BOOLEAN KBO_WeightTab_einfuegen_max_w(unsigned int varID,
                                             unsigned long new_max_w)
{
  unsigned long min_w, max_w;
  min_w = KBO_wTab.entries[varID].min_w;
  max_w = KBO_wTab.entries[varID].max_w;
  if (max_w > new_max_w){
    KBO_wTab.entries[varID].max_w = new_max_w;
    if (min_w == new_max_w){
      KBO_wTab.entries[varID].eq_w = new_max_w;
    }
    else if (min_w > new_max_w){
      return FALSE;
    }
  }
  return TRUE;
}

/* Fuegt das neue Wissen in die WeightTab ein.
 * TRUE: Kein Widerspruch
 * FALSE: Widerspruch */
static BOOLEAN KBO_WeightTab_einfuegen_min_w(unsigned int varID, 
                                             unsigned long new_min_w)
{
  unsigned long min_w, max_w;
  min_w = KBO_wTab.entries[varID].min_w;
  max_w = KBO_wTab.entries[varID].max_w;
  if (min_w < new_min_w){
    KBO_wTab.entries[varID].min_w = new_min_w;
    if (max_w == new_min_w){
      KBO_wTab.entries[varID].eq_w = new_min_w;
    }
    else if (max_w < new_min_w){
      return FALSE;
    }
  }
  return TRUE;
}

/* Fuegt das neue Wissen in die WeightTab ein.
 * TRUE: Kein Widerspruch
 * FALSE: Widerspruch */
static BOOLEAN KBO_WeightTab_einfuegen_eq_w(unsigned int varID,
                                            unsigned long new_eq_w)
{
  unsigned long min_w, max_w, eq_w;
  min_w = KBO_wTab.entries[varID].min_w;
  max_w = KBO_wTab.entries[varID].max_w;
  eq_w = KBO_wTab.entries[varID].eq_w;
  if (eq_w != 0){
    return eq_w == new_eq_w; /* Entweder erfahren wir nichts neues, 
                                oder wir haben einen Widerspruch. */
  }
  else if ((min_w <= new_eq_w) && (new_eq_w <= max_w) ){
    KBO_wTab.entries[varID].max_w = new_eq_w;
    KBO_wTab.entries[varID].min_w = new_eq_w;
    KBO_wTab.entries[varID].eq_w = new_eq_w;
    return TRUE;
  }
  return FALSE;
}
                                       

static void KBO_WeightTab_print(void)
{
  unsigned int i;
  blah(("----------------------------------------\n"));
  blah(("KBO_WeightTab_print():\n"));
  /* Der nullte-Eintrag ist leer, weil die Variablennummern mit 1 Anfangen. */
  blah(("VarID\tmax_w\tmin_w\teq_w\n"));
  for (i = 1; i < KBO_wTab.size; i++){
    blah(("x%d\t%u\t%u\t%u\n", i,
          KBO_wTab.entries[i].max_w,
          KBO_wTab.entries[i].min_w,
          KBO_wTab.entries[i].eq_w));
  }
  blah(("----------------------------------------\n"));
}


/* - Variablen-Koeffizienten-Vektor -----------------------------------------
 * Das erste Feld (0) hat eine spezielle Bedeutung (s.u.).
 * Gibt es die Variablen x_1 bis x_5, so ist size == 6 */
static int *varKoeffVec = NULL;
static unsigned int varKoeffVecSize = 0;

/* Groesse an aktuelle Variablenanzahl im System anpassen. */
static void varKoeffVecAnpassen(void)
{
  if (varKoeffVecSize <= SO_VarNummer(SO_NeuesteVariable)){
    varKoeffVecSize = SO_VarNummer(SO_NeuesteVariable) + 1;
    varKoeffVec = our_realloc(varKoeffVec,
                              varKoeffVecSize * sizeof(*varKoeffVec),
                              "varKoeffVec");
  }
}

static void varKoeffVecNullen(void)
{
  unsigned int i;
  for (i = 0; i < varKoeffVecSize; i++){ /* ab i=0 (Zelle fuer Summe der
                                            Gewichte der Funktionssymbole) */
    varKoeffVec[i] = 0;                  
  }
}

/* offset gibt an, ob addiert (1) oder subtrahiert (-1) werden soll. */
static void varKoeffVecTermModifizieren(UTermT t, int offset)
{
  UTermT t0 = TO_NULLTerm(t);
  UTermT t1 = t;
  while (t0 != t1){
    if (TO_TermIstVar(t1)){
      /* Nummer der ersten Variable im WM = 1 */
      varKoeffVec[SO_VarNummer(TO_TopSymbol(t1))] += offset;
    } else { /* wir beruecksichtigen auch die Gewichte der Funktionssymbole 
                (-> erste Zelle) */
      varKoeffVec[0] += SG_SymbolGewichtKBO(TO_TopSymbol(t1))*offset;
/*      blah_WM(("### ## SG_SymbolGewichtKBO(TO_TopSymbol(%t)) = %d \n", 
        t1, SG_SymbolGewichtKBO(TO_TopSymbol(t1)) )); */

    }
    t1 = TO_Schwanz(t1);
  }
}

static void varKoeffVecPrint(void)
{
  unsigned int i;
  blah(("varKoeffVecPrint():\n"));
  for (i = 0; i < varKoeffVecSize; i++){ /* ab i=0 (Zelle fuer Summe der
                                            Gewichte der Funktionssymbole) */
    blah(("A[%d]= %d ", i, varKoeffVec[i]));
  }
  blah(("\n"));
}

/* -------------------------------------------------------------------------- */

/* Ueberprueft, ob in varKoeffVec nur an einer Varstelle etwas !=0 steht. 
 * Rueckgabe ist die ID dieser Var oder 0, falls keine solche vorhanden. */
static unsigned int KBO_Vortest_1_nur_eins_neq_0(void){
  unsigned int gefunden = 0;
  unsigned int i;
  for (i = 1; i < varKoeffVecSize; i++){ /*ab 1,weil hier nur Vars interessa*/
    if (varKoeffVec[i] != 0){
      if (gefunden != 0){
        return 0; /* Es gibt mehr als eine Var mit Koeff !=0. */
      } /* gefunden == 0 */
      gefunden = i;
    }
  }
  return gefunden;
}

/* Steht nach dem Einfuegen des elementaren Constraints in den varKoeffVec nur an einer
 * Varstelle (z.B. i) etwas !=0, so haben wir fuer x_i eine neue Schranke. */
static BOOLEAN KBO_Vortest_Elem_1(EConsT e)
{
  unsigned int varID;
  int n,w;
  blah(("KBO_Vortest_Elem_1(): %e\n",e));
  varKoeffVecNullen();
  varKoeffVecTermModifizieren(e->s, 1); /* Vars und Funks von s addieren */
  varKoeffVecTermModifizieren(e->t, -1); /*Vars und Funks von t subtrahie*/
  varID = KBO_Vortest_1_nur_eins_neq_0();
  if (varID != 0){
    w = varKoeffVec[0]; /* Summe der Gewichte der Funktionssymbole */
    n = varKoeffVec[varID]; /* n != 0, Koeffeizient der Variable varID */
    if (e->tp == Ge_C || e->tp == Gt_C){
      if (n > 0 && w < 0){ /* n*x >= |w|   ==>  x >= ceil(|w|/n) */
        if (!KBO_WeightTab_einfuegen_min_w(varID, ((-w)+(n-1))/n)){
          return FALSE;
        }
      }
      else if (n < 0 && w > 0){ /* w >= |n|*x   ==>  x <= floor(w/|n|) */
        if (!KBO_WeightTab_einfuegen_max_w(varID, w/(-n))){
          return FALSE;
        }
      }
    }
    else { /* e->tp == Eq_C */
      if (n < 0){ /* w = |n|*x */
        n = -n;
        w = -w;
      }
      /* n*x = |w| */
      if ((w < 0) && ((-w) % n == 0)){
        if (!KBO_WeightTab_einfuegen_eq_w(varID, (-w)/n)){
          return FALSE;
        }        
      }
      else { /* z.B. f(f(x)) = a   =>  Vorzeichen, 
              * oder h(x,x) = h(g(a),a)   =>  Modulo != 0 */
        return FALSE;
      }
    }
  }
  return TRUE;
}

static BOOLEAN KBO_Vortest_1(EConsT e)
{
  EConsT cons = NULL;
  blah(("KBO_Vortest_1() testet %E\n",e));
  for (cons = e ; cons != NULL ; cons = cons->Nachf){
    KBO_WeightTab_print();
    if (!KBO_Vortest_Elem_1(cons)){
      blah(("KBO_Vortest_1(): FALSE, %E\n",e));
// IO_DruckeFlex("Zugeschlagen! %E\n",e);
      KBO_WeightTab_print();
      return FALSE;
    }
  }
  blah(("KBO_Vortest_1(): TRUE, %E\n",e));
  KBO_WeightTab_print();
  return TRUE;
}

static BOOLEAN KBO_Vortest(EConsT e)
{
  if (ORD_OrdIstKBO && wPraetest){
    if (!KBO_Vortest_1(e)){
      return FALSE;
    }
  }
  return TRUE;
}

/* Baut aus der aktuellen RelTab einen Constraint
 * und uebergibt diesen dann an den KBO_Vortest(). */
static BOOLEAN KBO_Nachtest()
{
  if (ORD_OrdIstKBO && wPostest){
    BOOLEAN ret;
    int i,j;
    TermT s,t;
    RelT rel;
    EConsT econs = NULL;

    for (i = 1; i <= relations.aktSize; i++){
      for (j = 1; j < i; j++){ /* Nur unteres Dreieck. (Symmetrie) */
        s = memos.terme[i];
        t = memos.terme[j];
        rel = RelTab_getRel(i,j);

        if (rel == gt){
          econs = CO_newEcons(s, t, econs, Gt_C);
        } else if (rel == ge){
          econs = CO_newEcons(s, t, econs, Ge_C);
        } else if (rel == lt){
          econs = CO_newEcons(t, s, econs, Gt_C);
        } else if (rel == le){
          econs = CO_newEcons(t, s, econs, Ge_C);
        } else if (rel == eq){
          econs = CO_newEcons(s, t, econs, Eq_C);
        }
      }
    }
    ret = KBO_Vortest(econs);
//    IO_DruckeFlex("KBO_Nachtest(): e= %E, erfuellbar: %b\n",econs,ret);
    CO_deleteEConsList(econs,NULL);
    return ret;
  }
  return TRUE;
}



/* -- =_w, >=_w und >_w ----------------------------------------------------- */

/* Prueft ob Phi(s) =_w Phi(t).
 * Dabei ist Phi: KBO-Constraint --> Arithmet. Constr 
 * Vorbedingung: KBO_WeightTab muss gefuellt sein,
 *               z.B. durch KBO_Vortest */
static BOOLEAN KBO_equals_w(TermT s, TermT t)
{
  unsigned int i;
  int sum = 0;
  varKoeffVecNullen();
  varKoeffVecTermModifizieren(s, 1);
  varKoeffVecTermModifizieren(t, -1);
  for (i = 1; i < varKoeffVecSize; i++){ /*ab 1,weil hier nur Vars interessa*/
    if (varKoeffVec[i] != 0){
      if (KBO_wTab.entries[i].eq_w != 0){
        varKoeffVec[i] *= KBO_wTab.entries[i].eq_w;
      }
      else { /* wissen nichts ueber KBO_wTab.entries[i].eq_w */
        return FALSE; /* koennen nicht sagen ob Phi(s) =_w Phi(t) */
      }
    }
  }
  /* wissen von allen Var mit Koeff !=0 das eq_w */
  for (i = 0; i < varKoeffVecSize; i++){ /*ab 0,weil hier auch Funks interessa*/
    sum += varKoeffVec[i];
  }
  return sum == 0;
}

/* Berechnet Phi(s) - Phi(t) fuer KBO_greaterEqual_w und KBO_greaterThen_w.
 * Dabei ist Phi: KBO-Constraint --> Arithmet. Constr 
 * TRUE: In retwert steht die Differenz.
 * FALSE: Es ist keine Aussage moeglich.
 * Vorbedingung: KBO_WeightTab muss gefuellt sein,
 *               z.B. durch KBO_Vortest */
static BOOLEAN KBO_Phi_Differenz_w(TermT s, TermT t, int* retwert)
{
  unsigned int i;
  *retwert = 0;
  KBO_WeightTab_print();
  blah(("KBO_Phi_Differenz_w(): %t, %t\n",s,t));
  varKoeffVecNullen();
  varKoeffVecPrint();
  varKoeffVecTermModifizieren(s, 1);
  varKoeffVecPrint();
  varKoeffVecTermModifizieren(t, -1);
  varKoeffVecPrint();
  for (i = 1; i < varKoeffVecSize; i++){ /*ab 1,weil hier nur Vars interessa*/
    if (varKoeffVec[i] < 0){
      if (KBO_wTab.entries[i].max_w < ULONG_MAX){
        varKoeffVec[i] *= KBO_wTab.entries[i].max_w;
      }
      else { /* wissen nichts ueber KBO_wTab.entries[i].max_w */
        return FALSE; /* koennen nicht sagen ob Phi(s) >=_w Phi(t) */
      }
    }
    else if (varKoeffVec[i] > 0){
      varKoeffVec[i] *= KBO_wTab.entries[i].min_w;
    }
  }
  varKoeffVecPrint();
  /* wissen alle Var mit Koeff < 0 das max_w < ULONG_MAX */
  for (i = 0; i < varKoeffVecSize; i++){ /*ab 0,weil hier auch Funks interessa*/
    *retwert += varKoeffVec[i];
  }
  blah(("*retwert = %d\n",*retwert));
  return TRUE;
}

/* Prueft ob Phi(s) >_w Phi(t).
 * Dabei ist Phi: KBO-Constraint --> Arithmet. Constr 
 * Vorbedingung: KBO_WeightTab muss gefuellt sein,
 *               z.B. durch KBO_Vortest */
static BOOLEAN KBO_greaterThen_w(TermT s, TermT t)
{
  int retwert;
  if (KBO_Phi_Differenz_w(s,t,&retwert)){
    return retwert > 0;
  }
  return FALSE;
}

/* Prueft ob Phi(s) >=_w Phi(t).
 * Dabei ist Phi: KBO-Constraint --> Arithmet. Constr 
 * Vorbedingung: KBO_WeightTab muss gefuellt sein,
 *               z.B. durch KBO_Vortest */
static BOOLEAN KBO_greaterEqual_w(TermT s, TermT t)
{
  int retwert;
  if (KBO_Phi_Differenz_w(s,t, &retwert)){
    return retwert >= 0;
  }
  return FALSE;
}

/* Wird im Moment nur einmal aufgerufen, direkt nach KBO_Vortest().
 * Vorbedingung: KBO_WeightTab muss gefuellt sein,
 *               z.B. durch KBO_Vortest
 * Ist Phi(s) >_w Phi(t), dann ist s >_kbo t.
 * Oder ist Phi(s) >=_w Phi(t) und top(s) >_F top(t).
 * TRUE: Entweder: kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */
static BOOLEAN Inferenz_KBO_aufwaerts_1_und_2(void)
{
  if (ORD_OrdIstKBO && kbo_pack){
    int i,j,diff;

    blah(("Inferenz_KBO_aufwaerts_1_und_2():\n"));

    for (i = 1; i <= relations.aktSize; i++){
      for (j = 1; j <= relations.aktSize; j++){
        if (RelTab_getRel(i,j) == uk){ /* Diese Inferenz ist eine Obermenge von
                                          RelTab_Ordnung_eintragen() */
          if (!KBO_Phi_Differenz_w(memos.terme[i], memos.terme[j], &diff)){
            continue; /* FALSE: Es ist keine Aussage moeglich. */
          }
          if (diff > 0){
            blah(("Inferenz_KBO_aufwaerts_1_und_2(): Erfolg "));
            blah(("%t > %t, weil Phi(s)-Phi(t) > 0\n",
                  memos.terme[i], memos.terme[j]));
            if (!RelTab_insertRel(i,j,gt,kbo_auf)){
              return FALSE;
            }
          }
          else if (diff == 0){ /* dann kann Praezedenz noch entscheiden */
            if (TO_TermIstVar(memos.terme[i])
                || TO_TermIstVar(memos.terme[j])){
              continue; /* dann hilft uns die Praezedenz auch nicht */
            }
            if (PR_Praezedenz(TO_TopSymbol(memos.terme[i]), 
                              TO_TopSymbol(memos.terme[j])) == Groesser){
              blah(("Inferenz_KBO_aufwaerts_1_und_2(): Erfolg "));
              blah(("%t > %t, weil Phi(s)-Phi(t) = 0 und top(s) >_F top(t)\n",
                    memos.terme[i], memos.terme[j]));
              if (!RelTab_insertRel(i,j,gt,kbo_auf)){
                return FALSE;
              }
            }
          }
        }
      }
    }
  }
  return TRUE;
}

/* lexikographischer Vergleich der Teilterme von s und t */
static BOOLEAN KBO_lexiko(ExtraConstT sid, ExtraConstT tid)
{
  unsigned int stelligkeit, teilterm_id;
  ExtraConstT s_tt, t_tt;

  if (TO_TopSymbol(memos.terme[sid]) != TO_TopSymbol(memos.terme[tid])){
    our_error("SuffSolver: LPO_lexiko: top(s) != top(t)\n");
  }

  stelligkeit = SO_Stelligkeit(TO_TopSymbol(memos.terme[sid]));
  for (teilterm_id = 1; teilterm_id <= stelligkeit; teilterm_id++){
    s_tt = MemoTab_readKeyPart(sid, teilterm_id);
    t_tt = MemoTab_readKeyPart(tid, teilterm_id);
    if (RelTab_getRel(s_tt, t_tt) == eq){
      /* wir muessen hier die Begruendungen eintragen,
         auch, wenn wir scheitern */
      logTab.reasons[teilterm_id] = RelTab_getLogEntry(s_tt, t_tt);
    }
    else { /* Die Kette der eq ist zu Ende */
      /* es muss mindestens ein gt gegeben haben, sonst koennen wir aufhoeren */
      if (RelTab_getRel(s_tt, t_tt) == gt){
        logTab.reasons[teilterm_id] = RelTab_getLogEntry(s_tt, t_tt);
        return TRUE;
      }
      return FALSE;
    }
  }
  return FALSE; /* alles gleich */
}


/* Wenn s_j = t_j oder s_j aequiv t_j fuer j < i, (1)
 * und s_i > t_i mit i < n (2)
 * und top(s) == top(t)
 * und Phi garantiert phi(sigma(s)) >= phi(sigma(t)).
 * Dann s > t.
 * D.h. ist das neue Wissen nuetzlich bei (1) oder (2)?
 * TRUE: Entweder: kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */
static BOOLEAN Inferenz_KBO_aufwaerts_3(void)
{
  if (ORD_OrdIstKBO && kbo_pack){
    RelT rel;
    ExtraConstT sid, tid, s, t;
    int term_id_1, term_id_2;
    sid = LogTab_getTermID_s(logTab.aktA);
    tid = LogTab_getTermID_t(logTab.aktA);
    rel = LogTab_getNewRel(logTab.aktA);

    blah(("Inferenz_KBO_aufwaerts_3_Versuch\n"));

    /* Wir erwarten unten s rho t mit rho in {>, =} */
    if (rel == lt){
      s = tid;
      t = sid;
    }
    else if (rel == gt || rel == eq){
      s = sid;
      t = tid;
    }
    else { /* rel not in gt, eq, lt   ==> uninteressant */
      return TRUE;
    }

    /* Ueber alle Eintraege der MemoTab. */
    for (term_id_1 = 1; term_id_1 <= memos.aktI; term_id_1++){ 
      unsigned int teilterm_id_1;
      /* Ueber alle Teilterme eines MemoTab-Eintrags. */
      unsigned int stelligkeit_1;
      stelligkeit_1 = SO_Stelligkeit(TO_TopSymbol(memos.terme[term_id_1]));
      for (teilterm_id_1 = 1; teilterm_id_1 < stelligkeit_1; teilterm_id_1++){
        /* Ist der Term mit ID s Teilterm von Term mit ID term_id_1? */
        if (MemoTab_readKeyPart(term_id_1, teilterm_id_1) == s){
          /* Gibt es einen Term term_id_2 mit top(term_id_1) == top(term_id_2)
             und term_id_2 hat an der gleichen Stelle wie term_id_1 t stehen? */
          /* Ueber alle Eintraege der MemoTab. */
          for (term_id_2 = 1; term_id_2 <= memos.aktI; term_id_2++){ 
            unsigned int teilterm_id_2;
            /* Ueber alle Teilterme eines MemoTab-Eintrags. */
            unsigned int stelligkeit_2;
            if (TO_TopSymbol(memos.terme[term_id_1]) 
                != TO_TopSymbol(memos.terme[term_id_2])){
              continue;
            }
            stelligkeit_2 = SO_Stelligkeit(TO_TopSymbol(memos.terme[term_id_2]));
            for (teilterm_id_2=1; teilterm_id_2 < stelligkeit_2; teilterm_id_2++){
              /* Ist der Term mit ID t Teilterm von Term mit ID term_id_2? */
              if (MemoTab_readKeyPart(term_id_2, teilterm_id_2) == t){
                if (teilterm_id_1 == teilterm_id_2){ /* gleiche Stelle? */
                  if (KBO_greaterEqual_w(memos.terme[term_id_1],
                                         memos.terme[term_id_2])
                      && KBO_lexiko(term_id_1, term_id_2)){
                    blah(("Inferenz_KBO_aufwaerts_3_Erfolg: %t > %t\n",
                          memos.terme[term_id_1],memos.terme[term_id_2]));
                    logTab.reasons[0] = RelTab_getLogEntry(sid,tid);
                    if (!RelTab_insertRel(term_id_1,term_id_2,gt,kbo_auf3)){
                      return FALSE;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return TRUE;
}

/* Wenn s rho t mit rho in {>, >=} (1)
 * und Phi garantiert phi(sigma(s)) == phi(sigma(t))
 * und top(s) == top(t)
 * und s_j = t_j oder s_j aequiv t_j fuer j < i < n.
 * Dann s_i >= t_i.
 * D.h. ist das neue Wissen nuetzlich bei (1)?
 * TRUE: Entweder: kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */
static BOOLEAN Inferenz_KBO_abwaerts_1(void)
{
  if (ORD_OrdIstKBO && kbo_pack){
    RelT rel;
    ExtraConstT sid, tid, s, t;
    int s_tt, t_tt;
    unsigned int stelligkeit;
    sid = LogTab_getTermID_s(logTab.aktA);
    tid = LogTab_getTermID_t(logTab.aktA);
    rel = LogTab_getNewRel(logTab.aktA);
    
    blah(("Inferenz_KBO_abwaerts_1_Versuch: %t %t\n",
          memos.terme[sid],memos.terme[tid]));

    if (rel == eq){
      return TRUE;
    }
    
    if (rel == lt || rel == le){ /* erwarten unten s rho t mit rho in {>,>=}*/
      blah(("dreh\n"));
      s = tid;
      t = sid;
    }
    else {
      blah(("nicht dreh\n"));
      s = sid;
      t = tid;
    }

    stelligkeit = SO_Stelligkeit(TO_TopSymbol(memos.terme[s]));

    if (stelligkeit > 1
        && TO_TopSymbol(memos.terme[s]) == TO_TopSymbol(memos.terme[t])
        && KBO_equals_w(memos.terme[s],memos.terme[t])){
      unsigned int teilterm_id;
      blah(("Inferenz_KBO_abwaerts_1_Versuch_im_if\n"));
      for (teilterm_id = 1; teilterm_id < stelligkeit; teilterm_id++){
        s_tt = MemoTab_readKeyPart(s, teilterm_id);
        t_tt = MemoTab_readKeyPart(t, teilterm_id);
        if (RelTab_getRel(s_tt, t_tt) == eq){
          /* wir muessen hier die Begruendungen eintragen,
             auch, wenn wir scheitern */
          blah(("EQ\n"));
          logTab.reasons[teilterm_id] = RelTab_getLogEntry(s_tt, t_tt);
        }
        else { /* Die Kette der eq ist zu Ende */
          logTab.reasons[0] = RelTab_getLogEntry(s,t);
          blah(("Inferenz_KBO_abwaerts_1_Erfolg\n"));
          return RelTab_insertRel(s_tt,t_tt,ge,kbo_ab_1);
        }
      }
    }
  }
  return TRUE;
}

/* Wenn s rho t mit rho in {>, >=}
 * und Phi garantiert phi(sigma(s)) == phi(sigma(t))
 * und top(s) == top(t)
 * und s_j = t_j oder s_j aequiv t_j fuer j < i < n. (1)
 * Dann s_i >= t_i.
 * D.h. ist das neue Wissen nuetzlich bei (1)?
 * TRUE: Entweder: kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */
static BOOLEAN Inferenz_KBO_abwaerts_2(void)
{
  if (ORD_OrdIstKBO && kbo_pack){
    RelT rel;
    ExtraConstT s, t;
    int term_id_1, term_id_2;
    s = LogTab_getTermID_s(logTab.aktA);
    t = LogTab_getTermID_t(logTab.aktA);
    rel = LogTab_getNewRel(logTab.aktA);

    blah(("Inferenz_KBO_abwaerts_2_Versuch\n"));

    if (rel != eq){ /* dann kann diese Inferenz nicht greifen */
      return TRUE;
    }

    /* Ueber alle Eintraege der MemoTab. */
    for (term_id_1 = 1; term_id_1 <= memos.aktI; term_id_1++){ 
      unsigned int teilterm_id_1;
      /* Ueber alle Teilterme eines MemoTab-Eintrags. */
      unsigned int stelligkeit_1;
      stelligkeit_1 = SO_Stelligkeit(TO_TopSymbol(memos.terme[term_id_1]));
      for (teilterm_id_1 = 1; teilterm_id_1 < stelligkeit_1; teilterm_id_1++){
        /* Ist der Term mit ID s Teilterm von Term mit ID term_id_1? */
        if (MemoTab_readKeyPart(term_id_1, teilterm_id_1) == s){
          /* Gibt es einen Term term_id_2 mit top(term_id_1) == top(term_id_2)
             und term_id_2 hat an der gleichen Stelle wie term_id_1 t stehen? */
          /* Ueber alle Eintraege der MemoTab. */
          for (term_id_2 = 1; term_id_2 <= memos.aktI; term_id_2++){ 
            unsigned int teilterm_id_2, teilterm_id_3;
            /* Ueber alle Teilterme eines MemoTab-Eintrags. */
            unsigned int stelligkeit_2;
            if (TO_TopSymbol(memos.terme[term_id_1]) 
                != TO_TopSymbol(memos.terme[term_id_2])){
              continue;
            }
            stelligkeit_2 = SO_Stelligkeit(TO_TopSymbol(memos.terme[term_id_2]));
            for (teilterm_id_2=1; teilterm_id_2 < stelligkeit_2; teilterm_id_2++){
              /* Ist der Term mit ID t Teilterm von Term mit ID term_id_2? */
              if (MemoTab_readKeyPart(term_id_2, teilterm_id_2) == t){
                if (teilterm_id_1 == teilterm_id_2){ /* gleiche Stelle? */
                  blah(("Inferenz_KBO_abwaerts_2: Action %t %t\n",
                        memos.terme[term_id_1], memos.terme[term_id_2]));
                  if (KBO_equals_w(memos.terme[term_id_1],
                                   memos.terme[term_id_2])){
                    blah(("Inferenz_KBO_abwaerts_2: equals_w\n"));
                    /* Vergleich der Teilterme von links nach rechts */
                    for (teilterm_id_3 = 1; teilterm_id_3 < stelligkeit_2; teilterm_id_3++){
                      int s_tt, t_tt;
                      s_tt = MemoTab_readKeyPart(term_id_1, teilterm_id_3);
                      t_tt = MemoTab_readKeyPart(term_id_2, teilterm_id_3);
                      if (RelTab_getRel(s_tt, t_tt) == eq){
                        /* wir muessen hier die Begruendungen eintragen,
                           auch, wenn wir scheitern */
                        logTab.reasons[teilterm_id_3] = RelTab_getLogEntry(s_tt, t_tt);
                      }
                      else { /* Die Kette der eq ist zu Ende */
                        logTab.reasons[0] = RelTab_getLogEntry(term_id_1,term_id_2);
                        blah(("Inferenz_KBO_abwaerts_2_Erfolg\n"));
                        if (!RelTab_insertRel(s_tt,t_tt,ge,kbo_ab_2)){
                          return FALSE;
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return TRUE;
}


/* -- Substitutionen ermitteln ---------------------------------------------- */

static Substitution subs; /* Existiert genau _einmal_ pro Waldmeisterlauf. */

static UTermT Subs_getBinding(unsigned short varNummer)
{
  if (varNummer < 1 || varNummer > SO_VarNummer(NewestVar)){
    our_error("Subs_getBinding() out of Bounds!\n");
  }
  return subs.bindings[varNummer-1];
}

static void Subs_setBinding(unsigned short varNummer, UTermT t)
{
  if (varNummer < 1 || varNummer > SO_VarNummer(NewestVar)){
    our_error("Subs_setBinding() out of Bounds!\n");
  }
  if (t == NULL){
    our_error("Subs_setBinding: Variable soll an Nullterm gebunden werden!");
  }
  if (subs.bindings[varNummer-1] != NULL){
    blah(("Subs_setBinding: x%d war schon gebunden an %t !\n",
          varNummer, Subs_getBinding(varNummer)));
  }
  blah(("Subs_setBinding: Binde x%d an %t !\n", varNummer, t));
  subs.bindings[varNummer-1] = t;
}

static void Subs_init(void)
{
  SymbolT i;
  blah(("Subs_init():\n"));

  /* Wurde subs schon allokiert? */
  if (subs.size == 0){
    blah(("subs wird zum ersten mal initialisiert.\n"));
    subs.size = SO_VarNummer(NewestVar);
    subs.bindings = our_alloc(subs.size * sizeof(UTermT));
  }
  /* subs besteht also schon, reicht der Platz? */
  else if (subs.size < SO_VarNummer(NewestVar)){
    blah(("subs besteht schon, aber ist nicht gross genug: "));
    blah(("subs.size = %d < %d = SO_VarNummer(NewestVar)\n",
          subs.size, SO_VarNummer(NewestVar)));
    subs.size = SO_VarNummer(NewestVar);
    subs.bindings = our_realloc(subs.bindings, subs.size * sizeof(UTermT),
                                "Extending subs.bindings");
  }
  /* subs.bindings initialisieren */
  for (i = 0; i < subs.size; i++){
    subs.bindings[i] = NULL;
  }
}

static BOOLEAN Subs_somethingBound(void)
{
  SymbolT i;
  for (i = 0; i < subs.size; i++){
    if (subs.bindings[i] != NULL){
      return TRUE;
    }
  }
  return FALSE;
}

static void Subs_print(void)
{
  SymbolT i;
  blah(("----------------------------------------\n"));
  blah(("Subs_print():\n"));
  /* Die Variablen fangen mit 1 an, der Arrayindex mit 0.
     Subs_getBinding() bekommt eine Variablennummer. */
  for (i = 1; i < subs.size+1; i++){
    blah(("x%d -> %t\n", i, Subs_getBinding(i)));
  }
  blah(("----------------------------------------\n"));
}


/* Wendet die Substitution subs auf den Term s an. */
static TermT Subs_apply(UTermT s)
{
  TermT res;
  UTermT t, t0, vorg;
  if (Subs_somethingBound()){
    if (TO_TermIstVar(s) 
        && Subs_getBinding(SO_VarNummer(TO_TopSymbol(s))) != NULL){
      /* Hier geht ein, dass durchmultipl. */
      res = TO_Termkopie(Subs_getBinding(SO_VarNummer(TO_TopSymbol(s)))); 
    }
    else {
      res = TO_Termkopie(s);
    }
    vorg = res;
    t = TO_Schwanz(res);
    t0 = TO_NULLTerm(res);
    while (t != t0){
      SymbolT x = TO_TopSymbol(t);
      if (SO_SymbIstVar(x)){
        /* Hier geht ein, dass durchmultipl. */
        UTermT b = Subs_getBinding(SO_VarNummer(x));
        if (b != NULL){
          if (TO_Nullstellig(b)){
            TO_SymbolBesetzen(t, TO_TopSymbol(b));
          }
          else {
            TermT u = TO_Termkopie(b);
            UTermT ue = TO_TermEnde(u);
            TO_NachfBesetzen(vorg, u);
            TO_SymbolBesetzen(t, TO_TopSymbol(ue));
            do {
              if (TO_TermEnde(u) == ue){
                TO_EndeBesetzen(u,t);
              }
              if (TO_Schwanz(u) == ue){
                TO_NachfBesetzen(u,t);
              }
              u = TO_Schwanz(u);
            } while (u != t);
            TO_TermzelleLoeschen(ue);
            t = TO_Schwanz(vorg);
          }
        }
      }
      vorg = t;
      t = TO_Schwanz(t);
    }
  }
  else {
    res = TO_Termkopie(s);
  }
  return res;
}

/* Wendet die Substitution subs auf den Econs e an. */
static EConsT Subs_apply_cons(EConsT e)
{
  EConsT cons = NULL;
  EConsT newcons = NULL;
  for (cons = e ; cons != NULL ; cons = cons->Nachf){
    newcons = CO_newEcons(Subs_apply(cons->s), Subs_apply(cons->t),
                          newcons, cons->tp);
  }
  blah(("Subs_apply_cons:\nvorher:  %E\nnachher: %E\n", e, newcons));
  Subs_print();
  return newcons;
}

/* ausmultiplizieren
   wende subs an auf alle Terme an die eine var gebunden werden.
   mache Idempotent:
   Problem: x1 -> f(x2), x2 -> g(x1) Endlosschleife!
   abbruch wenn keine Substitution mehr moeglich 
   Geht cleverer (Thomas) */
static void Subs_makeIdem(void)
{
  if(Subs_somethingBound()){
    SymbolT i;
    UTermT t, sigmaT;
    BOOLEAN changed = TRUE;
    while(changed){
      changed = FALSE;
      for (i = 1; i < subs.size+1; i++){
        t = Subs_getBinding(i);
//        blah(("Subs_makeIdem: i=%d, binding=%t\n", i, t));
        if (t != NULL){
          sigmaT = Subs_apply(t);
          changed = !TO_TermeGleich(t, sigmaT);
          if (changed){
            blah(("Subs_makeIdem: t= %t, sigmaT= %t\n", t, sigmaT));
            Subs_setBinding(i, sigmaT);
          }
        }
      }
    }
  }
  blah(("Subs_makeIdem():\n"));
  Subs_print();
}


/* Wertet die _derzeitige_ RelTab aus und speichert die (falls moeglich
 * idempotente) Substitution in subs */
static void Subs_evalReltab(void)
{
  /* folgende Optimierungen sind noch moeglich:
   * halbe Tabelle, aber dann auch testen, ob j var ist */
  int i,j;
  Subs_init();
  for (i = 1; i <= relations.aktSize; i++){
    for (j = 1; j <= relations.aktSize; j++){
      if (i == j) continue;
      if (RelTab_getRel(i,j) == eq 
          && SO_SymbIstVar(TO_TopSymbol(memos.terme[i]))){
        if (SO_SymbIstVar(TO_TopSymbol(memos.terme[j]))){
          /* wenn beide var, dann groessere an kleinere (alles an x1) */
          int varn_i, varn_j;
          varn_i = SO_VarNummer(TO_TopSymbol(memos.terme[i]));
          varn_j = SO_VarNummer(TO_TopSymbol(memos.terme[j]));
          Subs_setBinding(varn_i > varn_j ? varn_i : varn_j,
                          memos.terme[varn_i > varn_j ? j : i]);
        } else {
          Subs_setBinding(SO_VarNummer(TO_TopSymbol(memos.terme[i])),
                          memos.terme[j]);
        }
      }
    }
  }
  Subs_print();
  Subs_makeIdem();
}

static void Subs_test(EConsT e)
{
  Subs_init();
  Subs_print();
  blah(("%d\n", Subs_somethingBound()));
  Subs_setBinding(2, e->t);
  blah(("%d\n", Subs_somethingBound()));
  blah(("Subs_getBinding(2) = %t\n", Subs_getBinding(2)));
  Subs_setBinding(2, e->s);
  blah(("Subs_getBinding(2) = %t\n", Subs_getBinding(2)));
  Subs_setBinding(4, e->t);
  blah(("Subs_getBinding(4) = %t\n", Subs_getBinding(2)));
  Subs_print();
  Subs_getBinding(11);
  Subs_init();
  Subs_print();
}

/* - Afterburner ------------------------------------------------------------ */

static BOOLEAN RelTab_AfterburnerStep(RelT r, VergleichsT v, 
                                      TermT s, TermT t, char *str)
{
  if (v != Unvergleichbar){ /* Ordnungswissen evtl. verwertbar */
    if (r == gt && v != Groesser) {
      blah(("%sAfterburner: %t > %t, Ordnung: Geht nicht!\n", str, s, t));
      return TRUE;
    } 
    else if (r == lt && v != Kleiner){
      blah(("%sAfterburner: %t < %t, Ordnung: Geht nicht!\n", str, s, t));
      return TRUE;
    }
    else if (r == ge && v == Kleiner){
      blah(("%sAfterburner: %t >= %t, Ordnung: Geht nicht!\n", str, s, t));
      return TRUE;
    }
    else if (r == le && v == Groesser){
      blah(("%sAfterburner: %t <= %t, Ordnung: Geht nicht!\n", str, s, t));
      return TRUE;
    }
  }
  return FALSE;
}


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
	    i,j,memos.terme[i], RelTNames[RelTab_getRel(i,j)], memos.terme[j]));
      
      r = RelTab_getRel(i,j);
      v = ORD_VergleicheTerme(memos.terme[i], memos.terme[j]);

      if (RelTab_AfterburnerStep(r,v,memos.terme[i], memos.terme[j],"")){
        return FALSE;
      }
    }
  }
  return TRUE;
}

/* Benutzt die derzeitige Subs. */
static BOOLEAN RelTab_Afterburner_Subs(void)
{
  /* typedef enum { Groesser, Kleiner, Gleich, Unvergleichbar} VergleichsT; */
  /* typedef enum {uk,eq,gt,lt,ge,le,bt} RelT; */
  int i,j;
  RelT r;
  VergleichsT v;
  UTermT s,t;
  UTermT sigmaS,sigmaT;

  blah(("RelTab_Afterburner_Subs():\n"));

  for (i = 1; i <= relations.aktSize; i++){
    for (j = 1; j < i; j++){ /* Nur unteres Dreieck testen. (Symmetrie) */
      s = memos.terme[i];
      t = memos.terme[j];
      sigmaS = Subs_apply(s);
      sigmaT = Subs_apply(t);

      blah(("Afterburner_Subs(%d,%d): %t %s %t \n", 
            i,j, sigmaS, RelTNames[RelTab_getRel(i,j)], sigmaT));
      
      r = RelTab_getRel(i,j);
      v = ORD_VergleicheTerme(sigmaS, sigmaT);

      if (RelTab_AfterburnerStep(r,v, sigmaS, sigmaT,"Subst")){
//        IO_DruckeFlex("Afterburner_Subs %t %s %t \n %t %s %t\n", 
//                      s, RelTNames[RelTab_getRel(i,j)], t, 
//                      sigmaS, RelTNames[RelTab_getRel(i,j)], sigmaT);
      TO_TermeLoeschen(sigmaS,sigmaT);
        return FALSE;
      }
      TO_TermeLoeschen(sigmaS,sigmaT);
    }
  }
  return TRUE;
}

/* Benutzt die aktuell eingestellte Ordnung,
 * um die RelTab zu fuellen.
 * TRUE: Entweder: kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */
static BOOLEAN RelTab_Ordnung_eintragen(void)
{
  int i,j;
  RelT rel;

  blah(("RelTab_Ordnung_eintragen():\n"));

  for (i = 1; i <= relations.aktSize; i++){
    for (j = 1; j < i; j++){ /* Nur unteres Dreieck testen. (Symmetrie) */
      rel = translate(ORD_VergleicheTerme(memos.terme[i], memos.terme[j]));
      blah(("RelTab_Ordnung_eintragen(): %t %s %t\n",
            memos.terme[i], RelTNames[rel], memos.terme[j]));
      if (rel == uk){
        continue; /* uk ist nutzloses Wissen */
      }
      if (!RelTab_insertRel(i,j,rel,ord)){
        return FALSE;
      }
    }
  }
  return TRUE;
}

/* Baut aus der aktuellen RelTab einen Constraint
 * und uebergibt diesen dann an den KBOSuffSolver. */
static BOOLEAN KBO_Afterburner()
{
  if (ORD_OrdIstKBO && omega_burn){
    BOOLEAN ret;
    int i,j;
    TermT s,t;
    RelT rel;
    EConsT econs = NULL;

    for (i = 1; i <= relations.aktSize; i++){
      for (j = 1; j < i; j++){ /* Nur unteres Dreieck. (Symmetrie) */
        s = memos.terme[i];
        t = memos.terme[j];
        rel = RelTab_getRel(i,j);

        if (rel == gt){
          econs = CO_newEcons(s, t, econs, Gt_C);
        } else if (rel == ge){
          econs = CO_newEcons(s, t, econs, Ge_C);
        } else if (rel == lt){
          econs = CO_newEcons(t, s, econs, Gt_C);
        } else if (rel == le){
          econs = CO_newEcons(t, s, econs, Ge_C);
        } else if (rel == eq){
          econs = CO_newEcons(s, t, econs, Eq_C);
        }
      }
    }
    ret = KBOSuff_isSatisfiable1(econs);
//    IO_DruckeFlex("KBO_Afterburner(): e= %E, erfuellbar: %b\n",econs,ret);
    CO_deleteEConsList(econs,NULL);
    return ret;
  }
  return TRUE;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static BOOLEAN suff_Abschluss(void)
{
  for (; logTab.aktA <= logTab.aktF; logTab.aktA++){
    /* Inferenzen: Lift, Pacman, Strengthening, Decompose, Trans */
    if (!Inferenz_Strengthening() 
        || !Inferenz_Pacman()
        || !Inferenz_Lift()
        || !Inferenz_Decompose()
//        || !Inferenz_LPO_Alpha_Positiv() bringt nichts, da direkte Folge aus >_TT und Trans!
        || !Inferenz_LPO_Beta_Positiv()
        || !Inferenz_LPO_Gamma_Positiv()
        || !Inferenz_KBO_aufwaerts_3()
        || !Inferenz_KBO_abwaerts_1()
        || !Inferenz_KBO_abwaerts_2()
        || !Inferenz_transitiveClosure()){
      return FALSE;
    }
  }
  blah(("logTab.aktA == logTab.aktF\n"));
  Subs_evalReltab();
  return TRUE;
}

/* -- MAINLOOP -------------------------------------------------------------- */

/* Mainloop des SuffSolvers */
static BOOLEAN suff_satisfiable(EConsT e)
{
  KBO_WeightTab_init();
  varKoeffVecAnpassen();
  initMemoTab();
  
  MemoTab_insertECons(e);
  MemoTab_print();

  initRelTab();  /* muss _nach_ MemoTab_insertECons sein! */
  LogTab_init(); /* muss _nach_ MemoTab_insertECons sein! */

//  RelTab_copyTTfromMemTabTransitive(); Ist schwaecher als RelTab_Ordnung_eintragen() !
  RelTab_Ordnung_eintragen();

  return 1
    && KBO_Vortest(e)
    && Inferenz_KBO_aufwaerts_1_und_2()  /* Vorbed.: KBO_WeightTab muss gefuellt
                                          * sein, z.B. durch KBO_Vortest */
    && suff_Abschluss()
    && RelTab_insertECons(e)
    && suff_Abschluss()
    && RelTab_transitiveClosure_check()
//  && RelTab_Afterburner() /* ist schwaecher als RelTab_Afterburner_Subs() */
    && RelTab_Afterburner_Subs()
    && KBO_Nachtest()
    && KBO_Afterburner()
    ;
}


/* -------------------------------------------------------------------------- */


/* Mechanismus, um die gewonne Substitution anzuwenden
 * und es dann rekursiv nochmal zu versuchen. */
static BOOLEAN suff_sat2(EConsT e, int level)
{
//  IO_DruckeFlex("suff_sat2(e= %E, level= %d):\n",e,level);
  if (level > 5) return TRUE;
  if (suff_satisfiable(e)){
    BOOLEAN res;
    EConsT e2;
    if (!Subs_somethingBound()){
      return TRUE;
    }
    e2 = Subs_apply_cons(e);
    res = suff_sat2(e2,level+1);
    CO_deleteEConsList(e2,NULL);
//    IO_DruckeFlex("Xsuff_sat2(e= %E, level= %d) = %b\n",e,level,res);
    return res;
  }
  return FALSE;
}


static BOOLEAN suff_satKBO(EConsT e)
{
  BOOLEAN b1, b2, b3, b4, b5, b6, b7;
  kbo_pack = wPraetest = wPostest = omega_burn = FALSE;
  b1 = suff_satisfiable(e);
  wPraetest = TRUE;
  b2 = suff_satisfiable(e);
  wPostest = TRUE;
  b3 = suff_satisfiable(e);
  kbo_pack = TRUE;
  wPraetest = wPostest = FALSE;
  b4 = suff_satisfiable(e);
  wPraetest = TRUE;
  b5 = suff_satisfiable(e);
  wPostest = TRUE;
  b6 = suff_satisfiable(e);
  omega_burn = TRUE;
  b7 = suff_satisfiable(e);
  IO_DruckeFlex("## %b %b %b %b %b %b %b %E\n",
                b1, b2, b3, b4, b5, b6, b7, e);
  return b6;
}

BOOLEAN Suff_satisfiable2(EConsT e)
{
  //  return suff_satKBO(e);
  return suff_satisfiable(e);
}


/* -------------------------------------------------------------------------- */
/* -------- E O F ----------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
