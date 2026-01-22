/* SuffSolver3.c
 * BLs Variante von SuffSolver2.c
 * 23.12.2003
 *
 *
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
#include "KBOWeightSolver.h" 
#include <limits.h> /* Fuer KBO-Vortests */
#include "SymbolGewichte.h" /* Fuer KBO-Vortests */

#define DEBUG 0 /* 0, 1 */

#if DEBUG
#include <assert.h>
#else
#define assert(x) /* nix */
#endif

/* - Parameter -------------------------------------------------------------- */

#define DO_LINES 1     /* 0, 1 (Default=1) */
#define USE_PARENTS 1  /* 0, 1 (Default=1) */

#define ausgaben 0     /* 0, 1 */
#define TIME_MEASURE 0 /* 0, 1 */

static BOOLEAN Suff_flag = TRUE;

static BOOLEAN lpo_pack = TRUE;
static BOOLEAN kbo_pack = TRUE;
static BOOLEAN kbo_var_inf = TRUE;
static BOOLEAN wPraetest = FALSE;
static BOOLEAN wPostest = FALSE;
static BOOLEAN starkerPostTest = FALSE;
static BOOLEAN omega_burn = FALSE;
static BOOLEAN wGewichtsinferenzen = TRUE; /* WemoTab und Co. */
static BOOLEAN useSubs = FALSE;

/* Infrastruktur fuer hochgenaue Zeitmessungen ------------------------------ */

#if defined LINUX && !defined(__aarch64__) && !defined(__arm__)
/* Abhilfe: rdtsc als Ersatz fuer gethrtime (x86 only) */

typedef unsigned long long int hrtime_t;

/*#include <asm/msr.h>*/

#define rdtscll(val) \
     __asm__ __volatile__ ("rdtsc" : "=A" (val))

__inline__ static hrtime_t gethrtime(void)
{
        hrtime_t x;
        rdtscll(x);
        return x;
}
#elif defined DARWIN || defined(__aarch64__) || defined(__arm__)
/* macOS / Apple Silicon / ARM: use mach_absolute_time or clock_gettime */
#include <time.h>

typedef unsigned long long int hrtime_t;

__inline__ static hrtime_t gethrtime(void)
{
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (hrtime_t)ts.tv_sec * 1000000000ULL + (hrtime_t)ts.tv_nsec;
}
#endif

/* - Macros ----------------------------------------------------------------- */

#if ausgaben
#define blah(x) {if (Suff_flag) { IO_DruckeFlex x ; } }
#else
#define blah(x)
#endif

#define NewestVar ((-10 < 2 * BO_NeuesteBaumvariable) ? -10 : \
                   2 * BO_NeuesteBaumvariable)

/* - Nuetzliche Tabellen, etc. ---------------------------------------------- */

/* Inferenztyp fuer die Eintraege in die LogTab. 
 * tr=trans, tt=teilterm, bo=bottom, lf=lift, pm=pacman, st=strengthening,
 * dc=decompose, tc=Topsymbolclash, ... */
typedef enum {
  tr,tt,bo,lf,pm,st,dc,tc,co,lpo_alpha_pos,lpo_beta_pos,
  lpo_gamma_pos,ord,kbo_auf,kbo_auf3,kbo_ab_1,kbo_ab_2,
  kbo_var,
  weight_mu, weight_rel_mu, weight_fromWRelTab,
  weight_tr, weight_fromRelTab
} InfT;

/* Fuer schoene Ausgaben. */
static char *InfTNames[] = {
  "Trans","Teilterm","Bottom","Lift","Pacman",
  "Strengthening","Decompose","Topsymbolclash",
  "Constraint","LPO_Alpha_Positiv","LPO_Beta_Positiv",
  "LPO_Gamma_Positiv","Ordnung","KBO_aufwaerts1+2",
  "KBO_aufwaerts3","KBO_abwaerts1","KBO_abwaerts2",
  "KBO_variablen_gewicht",
  "Gewichtsinferenz: Mu", "Gewichtsinferenz: fill initially with rel_mu",
  "Gewichtsinferenz: Wissen aus WRelTab in RelTab uebertragen",
  "Gewichtsinferenz: Transitivitaet in WRelTab",
  "Gewichtsinferenz: Wissen aus RelTab in WRelTab uebertragen"
};

typedef enum {uk,eq,gt,lt,ge,le,bt} RelT;

/* Fuer schoene Ausgaben. */
static char *RelTNames[] = {" ."," ="," >"," <",">=","<=","!!"};

/* Gibt TRUE zurueck gdw. rel \in {gt, lt, ge, le} */
static BOOLEAN ineqRel(RelT rel)
{
  return (rel >= gt) && (rel <= le);
}

static RelT accdomtab(RelT rel1, RelT rel2)
{
  static unsigned char domtab[] = {
    uk,eq,gt,lt,ge,le,
    eq,eq,bt,bt,eq,eq,
    gt,bt,gt,bt,gt,bt,
    lt,bt,bt,lt,bt,lt,
    ge,eq,gt,bt,ge,eq,
    le,eq,bt,lt,eq,le
  };

  return domtab[(rel1)*6+(rel2)];
}

static RelT acctranstab(RelT rel1, RelT rel2)
{
  static char transtab[] = {
    uk,uk,uk,uk,uk,uk,
    uk,eq,gt,lt,ge,le,
    uk,gt,gt,uk,gt,uk,
    uk,lt,uk,lt,uk,lt,
    uk,ge,gt,uk,ge,uk,
    uk,le,uk,lt,uk,le
  };

  return transtab[(rel1)*6+(rel2)];
}

static RelT accmaxtab(RelT rel1, RelT rel2)
{
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

  return maxtab[(rel1)*6+(rel2)];
}

static RelT swap(RelT rel) 
{
  static char swaptab[] = {
    uk,eq,lt,gt,le,ge
  };

  return swaptab[(rel)];
}

static RelT convcons(Constraint rel) 
{
  /* Diese Tabelle konvertiert die enum Constraint (Constraint.h) nach RelT */
  static char converttab[] = {
    gt,eq,ge
  };

  return converttab[(rel)];
}

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
        

/* -------------------------------------------------------------------------- */
/* - MemoTab ---------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

typedef struct {
  ExtraConstT curParent;
  unsigned short nextParent; /* "Zeiger" in memos.parents */
} ParentT;

/* Enthaelt Terme (aus den Constraints) in folgender Form:
 * f, $i, $j, 0, ... , 0, p1, p2, ..., pN,
 * wobei
 * f Funktionssymbol
 * $i, $j die, schon numerierten Teilterme von f
 * 0, ... , 0 auffuellen bis SO_MaximaleStelligkeit
 *
 * p1 = Liste der Eltern, bei denen der Term an p == 1 steht;
 * ...
 * pN = dito fuer N == SO_MaximaleStelligkeit
 *
 * memos.memo[0] ist ein Zwischenspeicher  */

typedef struct MemoTabT {
  unsigned short maxN;  /* maximale Anzahl moeglicher Eintraege in MemoTab */
  unsigned short aktI;  /* aktueller Fuellstand */
  unsigned short aktP;  /* aktueller Fuellstand Eltern */
  unsigned short msize;  /* Groesse der einzelnen Eintraege */
  unsigned short keylen;  /* Groesse der einzelnen Schluessel */
  unsigned short *memo;  /* Zeiger auf Memoeintraege */
  unsigned short *weightID; /* Zeiger auf weightID-Tabelle */
  TermT *terme;          /* Zeiger auf die Terme */
  ParentT *parents; /* Zeiger auf die Elternverwaltungstabelle */
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
    memos.maxN  = 128; /* default: 128 */
    memos.msize = 2*SO_MaximaleStelligkeit + 2; /* +2 wegen: (Symb+ #TT-Symb +k)*/
    /* sizeof(Fkt-Symbol)=sizeof(TT-Symbol)=sizeof(k)=sizeof(unsigned short) */
    memos.keylen = 1 + SO_MaximaleStelligkeit; /* Symb + #TT */
    memos.memo  = our_alloc(memos.maxN * memos.msize * sizeof(unsigned short));
    memos.weightID = our_alloc(memos.maxN * sizeof(unsigned short));
    memos.terme = our_alloc(memos.maxN * sizeof(TermT));
    memos.parents = our_alloc(memos.maxN * sizeof(ParentT));
  }
  memos.aktI = 0;
  memos.aktP = 0;
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
  memos.parents = our_realloc(memos.parents, 
                              memos.maxN * sizeof(ParentT),
                              "Extending MemoTab(3)");
  memos.weightID = our_realloc(memos.weightID,
                               memos.maxN * sizeof(unsigned short),
                               "Extending MemoTab(4)");
  /* ggf. neuer Bereich initialisieren */
}

/* erhoeht i um 1 und vergroessert noetigenfalls MemoTab */
static void MemoTab_increaseI(void)
{
  if (memos.aktI == memos.maxN-1) {
    extendMemoTab();
  }
  memos.aktI++;
}


/* Traegt Teil eines Eintrags in die MemoTab ein. */
static void MemoTab_writePart(ExtraConstT eintragnummer, int aktStelle,
				  ExtraConstT i)
{
  memos.memo[eintragnummer * memos.msize + aktStelle] = i;
}


/* Liest Teil eines Eintrags aus der MemoTab aus. */
static ExtraConstT MemoTab_readPart(ExtraConstT eintragnummer, int aktStelle)
{
  return memos.memo[eintragnummer * memos.msize + aktStelle];
}

/* Liest top-Symbol aus */
static SymbolT MemoTab_readTop(ExtraConstT eintragnummer)
{
  return (SymbolT) memos.memo[eintragnummer * memos.msize];
}

/* Liest TT-Extrakonstante aus */
static ExtraConstT MemoTab_readTT(ExtraConstT eintragnummer, unsigned int tt)
{
  assert (tt <= SO_MaximaleStelligkeit);
  return memos.memo[eintragnummer * memos.msize + tt];
}

static TermT MemoTab_term(ExtraConstT k)
{
  assert (k <= memos.aktI);
  return memos.terme[k];
}

static unsigned short MemoTab_get_weightID(ExtraConstT k)
{
  assert (k <= memos.aktI);
  return memos.weightID[k];
}

static void MemoTab_set_weightID(ExtraConstT k, unsigned short id)
{
  assert (k <= memos.aktI);
/*   printf("Hallo:  %d %d\n", k, id); */
  memos.weightID[k] = id;
}

static ExtraConstT MemoTab_maxEintrag(void)
{
  return memos.aktI;
}


/* Ueberprueft ob Eintrag mit Nummer i == Eintrag mit Nummer key */
static BOOLEAN MemoTab_EqualKeys(ExtraConstT i, ExtraConstT key)
{
  int k;
  for (k = 0; k < memos.keylen; k++) {
    if (MemoTab_readPart(i, k) != MemoTab_readPart(key, k)) {
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
    if ((i != key) && MemoTab_EqualKeys(i, key)) {
      return i;
    }
  }
  return 0;
}


/* Kopiere Eintrag Nummer k1 an die Stelle Nummer k2.  */
static void MemoTab_copyKey(ExtraConstT k1, ExtraConstT k2)
{
  int k;

  for (k = 0; k < memos.keylen; k++) {
    MemoTab_writePart(k2, k, MemoTab_readPart(k1, k));
  }
}

/* Liefert fuer eine Variablennummer(Waldmeister) die Nummer der Variable in der
 * MemoTab. */ 
static unsigned int MemoTab_varID_to_ID(unsigned int varID)
{
  unsigned int i;  
  for (i=1; i <= memos.aktI; i++){
    if (SO_SymbIstVar(MemoTab_readTop(i))
        && SO_VarNummer(MemoTab_readTop(i)) == varID){
      return i;
    }
  }
  our_error("MemoTab_varID_to_ID: keine Var mit varID in MemoTab!\n");
  return 0;
}

static void MemoTab_parentsNullen(ExtraConstT k)
{
  unsigned i;
  for (i = memos.keylen; i < memos.msize; i++){
    MemoTab_writePart(k, i, 0);
  }
}

static unsigned int MemoTab_NewParent(ExtraConstT e, unsigned int p)
{
  unsigned i;
  memos.aktP++;
  i = memos.aktP;
  assert (memos.aktP < memos.maxN);
  memos.parents[i].curParent = e;
  memos.parents[i].nextParent = p;
  return i;
}

static unsigned int MemoTab_NextParent(unsigned int p)
{
  assert (p < memos.maxN);
  return memos.parents[p].nextParent;
}

static unsigned int MemoTab_CurParent(unsigned int p)
{
  assert (p < memos.maxN);
  return memos.parents[p].curParent;
}


static void MemoTab_writeParent(ExtraConstT k, unsigned int i, unsigned int p)
{
  MemoTab_writePart(k,memos.keylen-1 + i, p);
}

static unsigned int MemoTab_readParent(ExtraConstT k, unsigned int i)
{
  return MemoTab_readPart(k,memos.keylen-1 + i);
}

static void MemoTab_NeueEltern(ExtraConstT k, unsigned int i, ExtraConstT e)
{
  MemoTab_writeParent(k,i, MemoTab_NewParent(e,MemoTab_readParent(k,i)));
}

static void MemoTab_ElternschaftVermerken(ExtraConstT k)
{
  unsigned i;
  unsigned stelligkeit = SO_Stelligkeit(MemoTab_readTop(k));
  for (i = 1; i <= stelligkeit; i++){
    MemoTab_NeueEltern(MemoTab_readTT(k,i),i,k);
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
  MemoTab_writePart(0,0,TO_TopSymbol(t));
  ti = TO_ErsterTeilterm(t);
  k = 1;
  while (ti != t0) {
    MemoTab_writePart(0, k, TO_GetId(ti));
    ti = TO_NaechsterTeilterm(ti);
    k++;
  }
  
  /* Fuelle den Rest des Eintrags, wenn arity(f) < SO_MaximaleStelligkeit,
   * mit Nullen auf. */
  while (k <= SO_MaximaleStelligkeit) {
    MemoTab_writePart(0,k,0);
    k++;
  }

  /* Existiert solch ein Eintrag schon ?
   * Ja: gebe t diese Nummer und liefere diese Nummer zurueck
   * Nein: weitermachen */ 
  rck = MemoTab_containsKey(0);
  if (rck != 0) {
    TO_SetId(t, rck);
    return rck;
  }

  MemoTab_increaseI();
  /* Schreibe neuen Eintrag an das aktuelle Ende der MemoTab. */
  MemoTab_copyKey(0, MemoTab_maxEintrag());
  MemoTab_parentsNullen(MemoTab_maxEintrag());
  /* Der neue Eintrag bekommt die Nummer MemoTab_maxEintrag() . */
  TO_SetId(t, MemoTab_maxEintrag());
  memos.terme[MemoTab_maxEintrag()] = t;
  MemoTab_ElternschaftVermerken(MemoTab_maxEintrag());
  return MemoTab_maxEintrag();
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
  if (ausgaben){
    unsigned int i,k;
    ExtraConstT wert;
    
    blah(("### MemoTab:\n"));
    
    for( i = 1; i <= memos.aktI; i++ ) {
      blah(("Eintrag Nr. %d: ", i));
      blah(("%S ", MemoTab_readTop(i)));
      for( k = 1; k < memos.keylen; k ++ ) {
        wert = MemoTab_readPart(i, k);
        blah(("%d ", wert));
      }
      blah(("   %t   ", MemoTab_term(i)));
      for (k = 1; k <= SO_MaximaleStelligkeit; k++){
        wert = MemoTab_readParent(i,k);
        blah(("%d ", wert));
      }
      blah(("     "));
      for (k = 1; k <= SO_MaximaleStelligkeit; k++){
        blah(("%d -> (", k));
        wert = MemoTab_readParent(i,k);
        while (wert != 0){
          blah((" %d ", MemoTab_CurParent(wert)));
          wert = MemoTab_NextParent(wert);
        }
        blah((") "));
      }
      blah(("weightID: %d", MemoTab_get_weightID(i)));
      blah(("\n"));
    }

    blah(("Parents:\n"));
    for (i = 1; i <= memos.aktP; i++){
      blah(("%d: cur = %d next = %d\n", i, 
          memos.parents[i].curParent, memos.parents[i].nextParent));
    }
    blah(("Ende Parents\n"));
  }
}


/* -------------------------------------------------------------------------- */
/* - WemoTab -- Fuer KBO: Verzeichnis: Gewichte <--> Terme ------------------ */
/* -------------------------------------------------------------------------- */

typedef struct {
  ExtraConstT curMemoEntry; /* Nummer eines MemoTab-Eintrags */
  unsigned short nextMemoEntry; /* "Zeiger" in wemos.memoEntries */
} MemoEntryT;

/* Enthaelt Gewichtsvektoren der Terme aus der MemoTab:
 * alpha_0, alpha_1, ... , alpha_aktMaxVar, varphi(s), memo
 * wobei
 * alpha_0 = Summe der Funktionssymbole des Terms
 * alpha_i (i>0) = Koeffizient der i-ten Variable. VarNums beginnen bei 1.
 * varphi(s) = KBO-Gewicht des Terms
 * memo = Zeiger auf Liste der zugehoerigen MemoTab-Eintraege.
 *        "Liste", denn gleiche Eintraege in WemoTab werden identifiziert.
 *
 * wemos.entry[0] ist ein Zwischenspeicher. */
typedef struct WemoTabT {
  unsigned int allocedSize;    /* maximale Anzahl einzelner Zellen
                      Invariante: allocedSize >= allocedLines * aktEntrySize */
  unsigned short allocedLines; /* max Anzahl moeglicher Eintraege in WemoTab */
  unsigned short aktI;   /* aktueller Fuellstand der WemoTab (Zeilen) */
  unsigned short aktEntrySize; /* entspricht groesstem Koeffizienten-Index 
                                * +1 wegen alpha_0
                                * nochmals +1 wegen varphi(s) */

  unsigned long *entry;  /* Zeiger auf WemoTabEintraege (KoeffVec) */
  unsigned short *memo;  /* Spalte fuer zugehoerige Eintragnummer aus
                            MemoEntrieVerwaltungstabelle.
                            evtl. Beginn einer Liste */

  unsigned short aktM;     /* aktueller Fuellstand der MemoEntries */
  MemoEntryT *memoEntries; /* Zeiger auf die MemoEntrieVerwaltungstabelle */
} WemoTab;


static WemoTab wemos; /* Existiert genau _einmal_ pro Waldmeisterlauf. */


/* Erzeugt Verwaltungsblock und allokiert Raum fuer Elemente
 * cons wird benoetigt, um aktEntrySize zu bestimmen. */
static void WemoTab_init_space(EConsT cons)
{
  unsigned short maxVarNum, maxUsedLines;
  maxVarNum = SO_VarNummer(CO_neuesteVariable(cons));

  maxUsedLines = memos.aktI+1+1;/* WemoTab hat max so viele Eintraege wie die
                                 * MemoTab +1 fuer Nullten Eintrag 
                                 * und +1 fuer Mu */
  wemos.aktEntrySize = maxVarNum+1+1; /* +1 wegen alpha_0
                                       * nochmals +1 wegen varphi(s) */
  
  if (wemos.allocedSize < (unsigned int)(maxUsedLines * wemos.aktEntrySize)){
    wemos.allocedSize = maxUsedLines * wemos.aktEntrySize;
    wemos.entry = our_realloc(wemos.entry,
                              wemos.allocedSize * sizeof(unsigned long),
                              "Init_WemoTab(1)");
  }

  if (wemos.allocedLines < maxUsedLines){
    wemos.allocedLines = maxUsedLines;
    wemos.memo = our_realloc(wemos.memo,
                             wemos.allocedLines * sizeof(unsigned short),
                             "Init_WemoTab(2)");
    /* zu memos.allocedLines: es koennen maximal AnzZeilen(MemoTab) viele
       MemoTab-Eintraege referenziert werden -> es wird ja identifiziert */
    wemos.memoEntries = our_realloc(wemos.memoEntries,
                                    wemos.allocedLines * sizeof(MemoEntryT),
                                    "Init_WemoTab(3)");
  }
  wemos.aktI = 0;
  wemos.aktM = 0;
}

static void WemoTab_insert(unsigned short zeile, unsigned short spalte, 
                           unsigned long wert)
{
  assert (zeile <= wemos.aktI);
  assert (spalte < wemos.aktEntrySize);
  wemos.entry[zeile*wemos.aktEntrySize + spalte] = wert;
}

static unsigned long WemoTab_read(unsigned short zeile, unsigned short spalte)
{
  assert (zeile <= wemos.aktI);
  assert (spalte < wemos.aktEntrySize);
  return wemos.entry[zeile*wemos.aktEntrySize + spalte];
}  

static unsigned long WemoTab_get_varphi(unsigned short zeile)
{
  /* varphi steht an letzter Stelle im jeweiligen Eintrag */
  return WemoTab_read(zeile, wemos.aktEntrySize-1);
}

static void WemoTab_set_varphi(unsigned short zeile, unsigned long val)
{
  /* varphi steht an letzter Stelle im jeweiligen Eintrag */
  WemoTab_insert(zeile, wemos.aktEntrySize-1, val);
}

static void WemoTab_writeMemo(unsigned short zeile, unsigned short val)
{
  assert (zeile <= wemos.aktI);
  wemos.memo[zeile] = val;
}

static unsigned short WemoTab_readMemo(unsigned short zeile)
{
  assert (zeile <= wemos.aktI);
  return wemos.memo[zeile];
}

static unsigned int WemoTab_nextMemo(unsigned int m)
{
  assert (m <= wemos.aktM);
  return wemos.memoEntries[m].nextMemoEntry;
}

static unsigned int WemoTab_curMemo(unsigned int m)
{
  assert (m <= wemos.aktM);
  return wemos.memoEntries[m].curMemoEntry;
}

static unsigned short WemoTab_allocMemoEntry(ExtraConstT cur, unsigned short next)
{
  unsigned short res;
  wemos.aktM++;
  res = wemos.aktM;
  wemos.memoEntries[res].curMemoEntry = cur;
  wemos.memoEntries[res].nextMemoEntry = next;
  return res;
}

static void WemoTab_insert_memoEntry(unsigned short zeile, unsigned short t)
{
  WemoTab_writeMemo(zeile,WemoTab_allocMemoEntry(t,WemoTab_readMemo(zeile)));
}

static void WemoTab_reset_zeroth_entry(void)
{
  unsigned short i;
  for (i=0; i < wemos.aktEntrySize; i++){
    WemoTab_insert(0,i,0);
  }
}

static void WemoTab_add_to_zeroth_entry(unsigned short zeile)
{
  unsigned short spalte;
  for (spalte = 0; spalte < wemos.aktEntrySize; spalte++){
    WemoTab_insert(0,spalte,WemoTab_read(zeile,spalte)+
                            WemoTab_read(0,spalte));
  }
}

/* Liefert 0 wenn es keinen solchen Eintrag gibt.
   Ansonsten Nummer des Eintrags. */
static unsigned short WemoTab_lookup_entry_equal_to_zeroth_entry(void)
{
  unsigned short zeile, spalte;
  for (zeile = 1; zeile <= wemos.aktI; zeile++){
    for (spalte = 0; spalte < wemos.aktEntrySize; spalte++){
      if (WemoTab_read(0,spalte) != WemoTab_read(zeile,spalte)){
        break;
      }
    }
    if (spalte == wemos.aktEntrySize){
      return zeile;
    }
  }
  return 0;
}


static void WemoTab_fill_zeroth_entry(UTermT t)
{
  WemoTab_reset_zeroth_entry();
  if (TO_TermIstVar(t)){
    WemoTab_insert(0,SO_VarNummer(TO_TopSymbol(t)),1);
    WemoTab_set_varphi(0,SG_SymbolGewichtKBO(TO_TopSymbol(t)));
  } else {
    UTermT t_i, t_0;
    unsigned long weight = SG_SymbolGewichtKBO(TO_TopSymbol(t));
    WemoTab_insert(0,0,weight); /* Summe der Fkt-Symbole */
    WemoTab_set_varphi(0,weight);
    t_i = TO_ErsterTeilterm(t);
    t_0 = TO_NULLTerm(t);
    while (t_i != t_0){
      WemoTab_add_to_zeroth_entry(MemoTab_get_weightID(TO_GetId(t_i)));
      t_i = TO_NaechsterTeilterm(t_i);
    }
  }
}

static void WemoTab_copy_zeroth_entry_to(unsigned short entry)
{
  unsigned short spalte;
  unsigned long wert;
  for (spalte = 0; spalte < wemos.aktEntrySize; spalte++){
    wert = WemoTab_read(0, spalte);
    WemoTab_insert(entry, spalte, wert);
  }
}

static void WemoTab_insert_zeroth_entry_or_not(unsigned short aktMemoEntry)
{
  unsigned short lookup_result;
  lookup_result = WemoTab_lookup_entry_equal_to_zeroth_entry();
  if (lookup_result == 0){
    wemos.aktI++;
    lookup_result = wemos.aktI;
    WemoTab_copy_zeroth_entry_to(lookup_result);
    WemoTab_writeMemo(lookup_result,0);
  }
  /* lookup_result ist jetzt ...
   * wemos.aktI++:  falls kein gleicher Eintrag bereits vorhanden ist
   *                --> neuer Eintrag am Ende der WemoTab
   * lookup_result: die Eintragnummer des vorhandenen Eintrags 
   *                mit gleichem Gewichtspolynom 
   *                --> kein neuer Eintrag, bestehenden erweitern */
  MemoTab_set_weightID(aktMemoEntry, lookup_result);
  WemoTab_insert_memoEntry(lookup_result, aktMemoEntry);
}

/* Mu ist die kleinste Konstante im System. */
static void WemoTab_insert_Mu_as_first_entry(void)
{
  unsigned short spalte;
  wemos.aktI++;
  WemoTab_insert(1,0,SG_KBO_Mu_Gewicht());
  for (spalte = 1; spalte < wemos.aktEntrySize-1; spalte++){
    WemoTab_insert(1,spalte,0);
  }
  WemoTab_set_varphi(1, SG_KBO_Mu_Gewicht());
  WemoTab_writeMemo(1,0);
}


static void WemoTab_init_fill_from_MemoTab(void)
{
  unsigned short aktMemoEntry;

  WemoTab_insert_Mu_as_first_entry();

  for (aktMemoEntry = 1; aktMemoEntry <= memos.aktI; aktMemoEntry++){
    WemoTab_fill_zeroth_entry(MemoTab_term(aktMemoEntry));
    WemoTab_insert_zeroth_entry_or_not(aktMemoEntry);
  }
}  


/* Vorbedingung: MemoTab wurde schon erstellt.
 * cons wird benoetigt, um aktEntrySize zu bestimmen. */
static void WemoTab_init(EConsT cons)
{
  WemoTab_init_space(cons);
  WemoTab_init_fill_from_MemoTab();
}


static void WemoTab_print(void)
{
  if (ausgaben){
    unsigned int i,k,memo;
    blah(("### WemoTab:\n"));
    
    for (i = 1; i <= wemos.aktI; i++){
      blah(("Eintrag Nr. %d:  ",i));
      for (k = 0; k < wemos.aktEntrySize-1u; k++){
        blah(("%d ", WemoTab_read(i,k)));
      }
      blah(("varphi: %d ", WemoTab_get_varphi(i)));
      blah(("MemoEntries: "));
      memo = WemoTab_readMemo(i);
      while(memo != 0) {
        blah(("%d ", WemoTab_curMemo(memo)));
        memo = WemoTab_nextMemo(memo);
      }
      blah(("\n"));
    }
  }
}


/* -------------------------------------------------------------------------- */
/* -- LogTab - Speichert Informationen fuer Beweise und Backtracking -------- */
/* -------------------------------------------------------------------------- */

/* Die LogTab ist ein Protokoll der Veraenderung der RelTab und der WRelTab.
 * Unterscheidung zwischen Gewichts- und "normalen"-Inferenzen durch Namen! 
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
  if (ausgaben){
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
}


/* -------------------------------------------------------------------------- */
/* - RelTab - Relations-Matrix ---------------------------------------------- */
/* -------------------------------------------------------------------------- */

  /* Eintraege der Relations-Matrix und Hinweis auf LogTab. */
typedef struct RelEntryS {
  RelT rel; 
  unsigned short logEntry;
} RelEntryT;

typedef struct RelTabT {
  unsigned short size;    /* maximal verfuegbare _Seitenlaenge_ 
                             der Relations-Matrix */
  unsigned short aktSize; /* aktuell benutzte _Seitenlaenge_ 
                             der Relations-Matrix */
  RelEntryT *rels; 
  RelEntryT **lines;       /* Zeiger auf Zeilenanfaenge, 
                             um Multiplikation beim Zugriff zu sparen. */
  EConsT cons;            /* zu loesender Constraint,
                             wird fuer die Beweisausgabe benoetigt. */
} RelTab;


static RelTab relations; /* Existiert genau _einmal_ pro Waldmeisterlauf. */


static RelT RelTab_getRel(ExtraConstT sid, ExtraConstT tid)
{
#if DO_LINES
  return relations.lines[sid][tid].rel;
#else
  return relations.rels[sid*relations.aktSize + tid].rel; 
#endif
}


static unsigned short RelTab_getLogEntry(ExtraConstT sid, ExtraConstT tid)
{
#if DO_LINES
  return relations.lines[sid][tid].logEntry;
#else
  return relations.rels[sid*relations.aktSize + tid].logEntry;
#endif
}


static void RelTab_print(void)
{
  if (ausgaben && Suff_flag){
    int i,j;
    blah(("### RelTab:\n"));
    blah(("----------------------------------------\n"));
    blah(("   "));
    for (i = 1; i <= relations.aktSize; i++) {
      printf("%2d ", i);
    }
    blah(("\n"));
    for (i = 1; i <= relations.aktSize; i++) {
      printf("%2d ", i);
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
      printf("%2d ", i);
    }
    blah(("\n"));
    for (i = 1; i <= relations.aktSize; i++) {
      printf("%2d ", i);
      for (j = 1; j <= relations.aktSize; j++) {
        printf("%2d ", RelTab_getLogEntry(i,j));
      }
      blah(("\n"));
    }
    blah(("----------------------------------------\n"));
  }
}


/* Schreibt in alle Felder uk (= unknown) und in die Diagonale eq (= equal). 
 * Der Verweis auf die LogTab wird auf 0 gesetzt.*/
static void resetRelTab(void)
{
  int i, j;
  int aktSize = relations.aktSize;
  blah(("resetRelTab():\n"));
  for (i = 1; i <= aktSize; i++) {
    for (j = 1; j <= aktSize; j++) {
      relations.lines[i][j].rel = uk;
      relations.lines[i][j].logEntry = 0;
    }
    relations.lines[i][i].rel = eq;
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
  int i;

  /* Wurde RelTab schon initialisiert ? */
  if (relations.size == 0) {
    blah(("RelTab wird zum ersten mal initialisiert.\n"));    
    relations.size = MemoTab_maxEintrag();
#if DO_LINES
    relations.lines = our_alloc((relations.size+1) * sizeof(*relations.lines));
#endif
    relations.rels = our_alloc((relations.size+1) * (relations.size+1) 
                               * sizeof(*relations.rels));
  }   /* RelTab besteht also schon, ist sie gross genug? */
  else if (relations.size < MemoTab_maxEintrag()) {
    blah(("RelTab war schon initialisiert, aber ist nicht gross genug: "));
    blah(("MemoTab_maxEintrag() = %d > %d = relations.size\n", 
          MemoTab_maxEintrag(), relations.size));
    relations.size = MemoTab_maxEintrag();
#if DO_LINES
    relations.lines = our_realloc(relations.lines,
                                  (relations.size+1) * sizeof(*relations.lines),
                                  "Extendige RelTab-Lines");
#endif
    relations.rels = our_realloc(relations.rels,
                                 (relations.size+1) * (relations.size+1) 
                                 * sizeof(*relations.rels),
                                 "Extending RelTab");
  }
  /* Die aktuell benutzte Seitenlaenge der RelTab muss angepasst werden. */
  relations.aktSize = MemoTab_maxEintrag();

#if DO_LINES
  { int aktSize = relations.aktSize;
  /* Das Zeilenanfangsarray muss initialisiert werden. */
  for (i = 1; i <= aktSize; i++){
    relations.lines[i] = &(relations.rels[aktSize*i]);
  }
  }
#endif

  resetRelTab();
}


/* Gibt einen Beweis fuer die Unerfuellbarkeit der neuen Relation aus. */
static void RelTab_proofUnsatisfiability(unsigned short oldLogEntry){
#if DEBUG
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
	logTab.aktF, MemoTab_term(sid), RelTNames[newrel], MemoTab_term(tid)));
  blah(("Eintrag Nr. %d: %t %s %t !\n",
	oldLogEntry, MemoTab_term(sid), RelTNames[oldrel], MemoTab_term(tid)));
  
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

      /* Unterscheide ob Gewichtsinferenz oder nicht, weil sich dann akt_tid und
       * akt_sid entweder auf Memo- oder WemoTab beziehen. */      
      /* weight_fromWRelTab ist zwar Gewichtsinferenz, jedoch mit Auswirkung in
       * RelTab und akt_tid und akt_sid beziehen sich dann auf MemoTab*/
      if (inf >= weight_mu && inf != weight_fromWRelTab){
        /* zu 'MemoTab_term(WemoTab_readMemo(akt_sid))':
         * es wird der jeweils erste zugehoerige MemoTab-Eintrag ausgegeben */
        blah(("Eintrag Nr. %d: %t %s %t, wegen Inferenz %s ",
              aktLogEntry, MemoTab_term(WemoTab_readMemo(akt_sid)), 
              RelTNames[LogTab_getNewRel(aktLogEntry)],
              MemoTab_term(WemoTab_readMemo(akt_tid)),
              InfTNames[inf]));
      } else { /* keine Gewichtsinferenz */
        /* Eintrag ausgeben */
        blah(("Eintrag Nr. %d: %t %s %t, wegen Inferenz %s ",
              aktLogEntry, MemoTab_term(akt_sid), 
              RelTNames[LogTab_getNewRel(aktLogEntry)], MemoTab_term(akt_tid),
              InfTNames[inf]));
      }

      if (inf == kbo_auf){
        blah(("Begruendungen: KBO_WeightTab"));
      }
	    
      /* Die Inferenzen Constraint und Teilterm haben keine Begruendungen. 
       * Die Inferenz Ordnung auch nicht.
       * ebenso weight_mu und weight_rel_mu */
      if (inf != tt && inf != co && inf != ord && inf != kbo_auf
          && inf != weight_mu && inf != weight_rel_mu){
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
	blah(("%t :%s: %t; ", MemoTab_term(akt_sid),
	      RelTNames[LogTab_getNewRel(aktLogEntry)], MemoTab_term(akt_tid)));
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
/*   IO_DruckeFlex("BSTAT anzCons: %d anzUsedCons: %d\n", anzCons, anzUsedCons); */

  blah(("BEWEIS ENDE\n"));
#endif /* DEBUG */
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
    stop = MemoTab_readTop(sid);
    ttop = MemoTab_readTop(tid);
    if (SO_SymbIstFkt(stop) && SO_SymbIstFkt(ttop)){
      if (stop != ttop) {
	blah(("$$$ PENG: Inferenz_Bottom:\nTopsymbolclash: %t = %t\n", 
	      MemoTab_term(sid), MemoTab_term(tid)));
	RelTab_proofUnsatisfiability(oldLogEntry);
	return FALSE;
      }
    }
  }
  return TRUE;
}

/* Forward Declaration noetig: */
static BOOLEAN WRelTab_insertRel(ExtraConstT sid, ExtraConstT tid, 
                                 RelT newrel, InfT inf);

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

#if DO_LINES
  oldrel = relations.lines[sid][tid].rel;
  oldLogEntry = relations.lines[sid][tid].logEntry;
#else
  oldrel = relations.rels[sid*relations.aktSize + tid].rel;
  oldLogEntry = relations.rels[sid*relations.aktSize + tid].logEntry;
#endif

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

#if DO_LINES
  relations.lines[sid][tid].rel = newrel;
  /* Hinweis auf zugehoerigen Eintrag in der LogTab: */
  relations.lines[sid][tid].logEntry = logTab.aktF;
  /* Symmetrie eintragen: */
  relations.lines[tid][sid].rel = swap(newrel);
  /* Hinweis auf zugehoerigen Eintrag in der LogTab: */
  relations.lines[tid][sid].logEntry = logTab.aktF;
#else
  relations.rels[sid*relations.aktSize + tid].rel = newrel;
  /* Hinweis auf zugehoerigen Eintrag in der LogTab: */
  relations.rels[sid*relations.aktSize + tid].logEntry = logTab.aktF;
  /* Symmetrie eintragen: */
  relations.rels[tid*relations.aktSize + sid].rel = swap(newrel);
  /* Hinweis auf zugehoerigen Eintrag in der LogTab: */
  relations.rels[tid*relations.aktSize + sid].logEntry = logTab.aktF;
#endif

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

  /* Ist newrel == gt, lt oder eq, dann ist ein abgeschwaechter Eintrag auch in
   * der WRelTab moeglich: (Dabei ist >_wr die Relation der WRelTab)
   * Es gilt: Aus  s >_r t folgt s >=_wr t.
   *          Aus  s =_r t folgt s =_wr t. */
  if (ORD_OrdIstKBO
      && wGewichtsinferenzen
      && inf != weight_fromWRelTab /* Zykel verhindern */){
    
    unsigned int s_wid, t_wid;
    s_wid = MemoTab_get_weightID(sid);
    t_wid = MemoTab_get_weightID(tid);

    if ((newrel == eq) && (s_wid != t_wid)){
      logTab.reasons[0] = logTab.aktF;
      if (!WRelTab_insertRel(s_wid, t_wid, eq, weight_fromRelTab)){
	return FALSE;
      }
    } else if (newrel == gt){
      logTab.reasons[0] = logTab.aktF;
      if (!WRelTab_insertRel(s_wid, t_wid, ge, weight_fromRelTab)){
	return FALSE;
      }
    } else if (newrel == lt){
      logTab.reasons[0] = logTab.aktF;
      if (!WRelTab_insertRel(s_wid, t_wid, le, weight_fromRelTab)){
	return FALSE;
      }
    }
  }

  return TRUE;
}


/* Traegt das Wissen ueber die _transitive_ Teilterm-Relation aus der MemoTab 
 * in die RelTab ein. */
static void RelTab_copyTTfromMemTabTransitive(void)
{
  unsigned int i,j,k;

  blah(("RelTab_copyTTfromMemTabTransitive():\n"));

  /* alle Eintraege in der MemoTab */
  for (i = 1; i <= MemoTab_maxEintrag(); i++){
    /* alle Teilterme pro Eintrag */
    for (j = 1; j <= SO_MaximaleStelligkeit; j++){
      /* Ende der Teilterme */
      if (MemoTab_readTT(i, j) == 0) {
        break;
      }
      /* Direkten Teilterm eintragen. */
      blah(("### RelTab_insertRel(%d,MemoTab_readTT(%d,%d)= %d, gt, tt)\n",
            i, i, j, MemoTab_readTT(i,j)));
      if (!RelTab_insertRel(i, MemoTab_readTT(i,j), gt, tt)){
        our_error("### RelTab_copyTTfromMemTabTransitive: Arrrgh !");
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


/* -------------------------------------------------------------------------- */
/* - WRelTab - Gewichts-Relations-Matrix ------------------------------------ */
/* -------------------------------------------------------------------------- */

/* 
 * Ziel ist folgende Relation auf Termen: 
 * s >_wr t  gdw  varphi(sigma(s)) > varphi(sigma(t)) f.a. sigma \in GSub(s,t)
 *
 * Diese Relation wird approximiert durch die folgende Relation >_mu auf
 * Gewichtspolynomen (aus der WemoTab):
 *   Dabei sei w_a das Gewichtspolynom Phi(s) von s und w_b das Gewichtspolynom
 *   Phi(t) von t. Weiter sind a_i (bzw. b_i) i=1,...,n die Koeffizienten der
 *   Variablen in w_a (bzw. w_b).
 *   a_0 (bzw. b_0) ist jeweils die Summe der KBO-Gewichte der Fkt-Symbole in s
 *   (bzw. t).
 *
 * w_a >_mu w_b  gdw  a_i >= b_i f.a. i=1..n
 *                    und w_a != w_b
 *                    und a_0 + sum(a_i*mu) > b_0 + sum(b_i*mu)
 *                        dabei ist mu = SG_KBO_Mu_Gewicht
 *
 * w_a >=_mu w_b  gdw  a_i >= b_i f.a. i=1..n
 *                     und a_0 + sum(a_i*mu) >= b_0 + sum(b_i*mu)
 *                         dabei ist mu = SG_KBO_Mu_Gewicht
 *
 * Dann gilt:
 * Aus      Phi(s) >_mu Phi(t)      folgt      s >_wr t.
 * Die gleiche Aussage gilt fuer >= .
 *
 * Zum Initialisieren wird also >_mu verwendet. 
 * (siehe Gewichtsinferenz_fill_initially_with_rel_mu(void) ) */

/* Eintraege der Gewichts-Relations-Matrix und Hinweis auf LogTab.
 * Als Relationstyp wird derjenige von RelTab wiederverwendet. */
typedef struct WRelEntryS {
  RelT rel;
  unsigned short logEntry;
} WRelEntryT;


typedef struct WRelTabT {
  unsigned short size;    /* maximal verfuegbare _Seitenlaenge_ 
                             der Gewichts-Relations-Matrix */
  unsigned short aktSize; /* aktuell benutzte _Seitenlaenge_ 
                             der Gewichts-Relations-Matrix */
  WRelEntryT *rels;       /* Zeiger auf die Eintraege */
  WRelEntryT **lines;     /* Zeiger auf Zeilenanfaenge, 
                             um Multiplikation beim Zugriff zu sparen. */
} WRelTab;


static WRelTab weight_relations;/*Existiert genau _einmal_ pro Waldmeisterlauf*/


static RelT WRelTab_getRel(ExtraConstT sid, ExtraConstT tid)
{
  return weight_relations.lines[sid][tid].rel;
}

static unsigned short WRelTab_getLogEntry(ExtraConstT sid, ExtraConstT tid)
{
  return weight_relations.lines[sid][tid].logEntry;
}

static void WRelTab_print(void)
{
  if (ausgaben && Suff_flag){
    int i,j;
    blah(("### WRelTab:\n"));
    blah(("----------------------------------------\n"));
    blah(("   "));
    for (i = 1; i <= weight_relations.aktSize; i++) {
      printf("%2d ", i);
    }
    blah(("\n"));
    for (i = 1; i <= weight_relations.aktSize; i++) {
      printf("%2d ", i);
      for (j = 1; j <= weight_relations.aktSize; j++) {
        if (WRelTab_getRel(i,j) > bt){
          blah(("?? "));
        }
        else {
          blah(("%s ", RelTNames[WRelTab_getRel(i,j)]));
        }
      }
      blah(("\n"));
    }

    blah(("\nVerweise auf LogTab:\n"));
    blah(("   "));
    for (i = 1; i <= weight_relations.aktSize; i++) {
      printf("%2d ", i);
    }
    blah(("\n"));
    for (i = 1; i <= weight_relations.aktSize; i++) {
      printf("%2d ", i);
      for (j = 1; j <= weight_relations.aktSize; j++) {
        printf("%2d ", WRelTab_getLogEntry(i,j));
      }
      blah(("\n"));
    }
    blah(("----------------------------------------\n"));
  }  
}

/*Schreibt in alle Felder uk (= unknown) und in die Diagonale eq (= equal)*/
static void WRelTab_reset(void)
{
  int i, j;
  int aktSize = weight_relations.aktSize;
  blah(("WRelTab_reset():\n"));
  for (i = 1; i <= aktSize; i++) {
    for (j = 1; j <= aktSize; j++) {
      weight_relations.lines[i][j].rel = uk;
      weight_relations.lines[i][j].logEntry = 0;
    }
    weight_relations.lines[i][i].rel = eq;
  }
  WRelTab_print();
}

/* Die Gewichts-Relations-Matrix hat die Seitenlaenge = akt-Fuellstand-WemoTab.
 * Spalten / Zeilen sind Eintragnummern der Gewichte (-> WemoTab). 
 * Zuvor muss WemoTab_init_fill_from_MemoTab aufgerufen werden.(wegen Groesse)*/
static void WRelTab_init(void)
{
  blah(("WRelTab_init():\n"));
  int i;

  /* Wurde WRelTab schon initialisiert ? */
  if (weight_relations.size == 0) {
    blah(("WRelTab wird zum ersten mal initialisiert.\n"));    
    weight_relations.size = wemos.aktI; /* Seitenlaenge=akt-Fuellstand-WemoTab*/
    weight_relations.lines = our_alloc((weight_relations.size+1) 
                                       * sizeof(*weight_relations.lines));
    weight_relations.rels = our_alloc((weight_relations.size+1) 
                                      * (weight_relations.size+1) 
                                      * sizeof(*weight_relations.rels));
  }   /* WRelTab besteht also schon, ist sie gross genug? */
  else if (weight_relations.size < wemos.aktI) {
    blah(("WRelTab war schon initialisiert, aber ist nicht gross genug: "));
    blah(("wemos.aktI = %d > %d = weight_relations.size\n", 
          memos.aktI, weight_relations.size));
    weight_relations.size = wemos.aktI;
    weight_relations.lines = our_realloc(weight_relations.lines,
                   (weight_relations.size+1) * sizeof(*weight_relations.lines),
                   "Extendige WRelTab-Lines");
    weight_relations.rels = our_realloc(weight_relations.rels,
                         (weight_relations.size+1) * (weight_relations.size+1) 
                         * sizeof(*weight_relations.rels),
                         "Extending WRelTab");
  }
  /* Die aktuell benutzte Seitenlaenge der WRelTab muss angepasst werden. */
  weight_relations.aktSize = wemos.aktI;

  /* Das Zeilenanfangsarray muss initialisiert werden. */
  { int aktSize = weight_relations.aktSize;
  for (i = 1; i <= aktSize; i++){
    weight_relations.lines[i] = &(weight_relations.rels[aktSize*i]);
  }
  }

  WRelTab_reset();
}


/* Ueberprueft, ob die letzte Eintragung in die LogTab zu Widerspruechen fuehrt.
 * Benutzt dabei die DomTab.
 *
 * Unterschied zu Inferenz_Bottom: _kein_ Topsymbolclash !
 */
static BOOLEAN Inferenz_Bottom_weight(unsigned short oldLogEntry){
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

  return TRUE;
}



/* Traegt eine einzelne Gewichts-Relation ein + Symmetrie.
 * Der Eintrag wird in der LogTab protokolliert.
 * Die Begruendungen fuer die LogTab werden ueber das Array logTab.reasons
 * geholt.
 * Unterscheidung zwischen Gewichts- und "normalen"-Inferenzen durch Namen! 
 * Das usedFlag wird auf Null gesetzt.
 * Mit der Inferenz_Bottom wird ueberprueft, ob es dabei Widersprueche gibt:
 * (TRUE) oder nicht (FALSE).*/
static BOOLEAN WRelTab_insertRel(ExtraConstT sid, ExtraConstT tid, 
                                 RelT newrel, InfT inf)
{
  unsigned short i, oldLogEntry;
  RelT oldrel, retrel;

#if DEBUG
  if (inf <= kbo_var){
    our_error("Nur W-Inferenzen duerfen in WRelTab schreiben!\n");
  }
#endif

  blah(("WRelTab_insertRel(sid= %d, tid= %d, newrel= %s, inf= %s)\n",
	sid, tid, RelTNames[newrel], InfTNames[inf]));

  oldrel = weight_relations.lines[sid][tid].rel;
  oldLogEntry = weight_relations.lines[sid][tid].logEntry;

  /* DomTab benutzen. Auch wenn retrel == bt, so wird dennoch erst eingetragen. 
   * Inferenz_Bottom_weight erkennt dann spaeter den Widerspruch. 
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

  weight_relations.lines[sid][tid].rel = newrel;
  /* Hinweis auf zugehoerigen Eintrag in der LogTab: */
  weight_relations.lines[sid][tid].logEntry = logTab.aktF;
  /* Symmetrie eintragen: */
  weight_relations.lines[tid][sid].rel = swap(newrel);
  /* Hinweis auf zugehoerigen Eintrag in der LogTab: */
  weight_relations.lines[tid][sid].logEntry = logTab.aktF;

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
  WRelTab_print();

  if (!Inferenz_Bottom_weight(oldLogEntry)){
    return FALSE;
  }

  /* Ist newrel == gt oder lt, dann ist ein entsprechender Eintrag auch in
   * RelTab moeglich: (Dabei ist >_r die Relation der RelTab)
   * Es gilt: Aus  s >_wr t folgt s >_r t. 
   * Insbesondere gilt _nicht_: Aus  s >=_wr t folgt s >=_r t. */
  if (((newrel == gt) || (newrel == lt))
      && inf != weight_mu){ /* Mu muss es nicht in MemoTab geben! */
    unsigned int s_memo, t_memo;
    s_memo = WemoTab_readMemo(sid);
    t_memo = WemoTab_readMemo(tid);
    while (s_memo != 0){
      while (t_memo != 0){
        logTab.reasons[0] = logTab.aktF;
        if (!RelTab_insertRel(WemoTab_curMemo(s_memo), WemoTab_curMemo(t_memo),
                              newrel, weight_fromWRelTab)){
          return FALSE;
        }
        t_memo = WemoTab_nextMemo(t_memo);
      }
      s_memo = WemoTab_nextMemo(s_memo);
    }
  }
  
  return TRUE;
}

static BOOLEAN WRelTab_only_one_entry_equals_one_and_all_others_equal_zero(
               unsigned short entry)
{
  unsigned short k, aktEntrySize;
  unsigned long sum;

  aktEntrySize = wemos.aktEntrySize-1; /* -1: varphi nicht mitbetrachten */
  sum = 0;

  for (k = 0; k < aktEntrySize; k++){
    sum += WemoTab_read(entry, k);
  }
  return (sum == 1);
}


/* Traegt die Beziehungen zu Mu ein. */
static void WRelTab_fill_initially_wrt_Mu(void)
{
  unsigned short spalte;
  
  /* ab 2, weil Mu ausgelassen wird */
  for (spalte = 2; spalte <= weight_relations.aktSize; spalte++){
    if(WRelTab_only_one_entry_equals_one_and_all_others_equal_zero(spalte)){
      WRelTab_insertRel(1, spalte, le, weight_mu);
    } else {
      WRelTab_insertRel(1, spalte, lt, weight_mu);
    }
  }
}

/* ========================================================================== */
/* == Gewichts-Inferenzen =================================================== */
/* ========================================================================== */

/* Automat, der dabei hilft a_k >= b_k f.a. k 
 * bzw. a_k <= b_k f.a. k zu entscheiden.
 *
 * Zustaende: le, eq, ge, [bt] (RelT)
 * Kanten: eq, gt, lt 
 *         Bsp: Von Zustand eq mit Kante lt in Zustand le. 
 *
 * oldState: alter Zustand 
 * signal: Kante
 * return: neuer Zustand */
static RelT Gewichtsinferenz_automat(RelT oldState, RelT signal)
{
  assert((oldState == eq)
         || (oldState == le)
         || (oldState == ge));
  assert((signal == eq)
         || (signal == gt)
         || (signal == lt));

  static unsigned char statetab[] = {
                          /* Spalte ist Signal,
                       RelT:    uk,eq,gt,lt,ge,le,bt */
/*                      uk*/    bt,bt,bt,bt,bt,bt,
/*                      eq*/    bt,eq,ge,le,bt,bt,
/* Zeile ist oldstate   gt*/    bt,bt,bt,bt,bt,bt,
/*                      lt*/    bt,bt,bt,bt,bt,bt,
/*                      ge*/    bt,ge,ge,bt,bt,bt,
/*                      le*/    bt,le,bt,le,bt,bt
  };

  return statetab[(oldState)*6+(signal)];
}


/* Vergleicht die Koeffizienten mit Nr. koeff_index in den Gewichtspolynomen
 * von sid und tid. */
static RelT Gewichtsinferenz_automat_benutzen_vergleich(
		   ExtraConstT sid, ExtraConstT tid, unsigned koeff_index)
{
  unsigned long koeff_val_sid, koeff_val_tid;
  koeff_val_sid = WemoTab_read(sid, koeff_index);
  koeff_val_tid = WemoTab_read(tid, koeff_index);

  if (koeff_val_sid > koeff_val_tid){
    return gt;
  } else if (koeff_val_sid < koeff_val_tid){
    return lt;
  } else /* if (koeff_val_sid == koeff_val_tid) */ {
    return eq;
  }
}


/* Entscheidet fuer die entstprechende Eintraege der WemoTab ob
 * a_k >= b_k f.a. k=1..n oder  a_k <= b_k f.a. k=1..n gilt 
 */
static RelT Gewichtsinferenz_automat_benutzen(ExtraConstT sid, ExtraConstT tid)
{
  RelT state = eq; /* Startzustand ist eq */
  unsigned short koeff_index, aktEntrySize;
  RelT aktRel;
  aktEntrySize = wemos.aktEntrySize-1; /* -1: varphi nicht mitbetrachten */
  for (koeff_index = 1; koeff_index < aktEntrySize; koeff_index++){
    aktRel = Gewichtsinferenz_automat_benutzen_vergleich(sid, tid, koeff_index);
    state = Gewichtsinferenz_automat(state, aktRel);
    if (state == bt){
      return bt;
    }
  }
  return state;
}


/* Wird einmalig am Anfang aufgerufen.
 *
 * >_mu ist definiert wie oben:
 * w_a >_mu w_b  gdw  a_i >= b_i f.a. i=1..n
 *                    und w_a != w_b
 *                    und a_0 + sum(a_i*mu) > b_0 + sum(b_i*mu)
 *                        dabei ist mu = SG_KBO_Mu_Gewicht
 *
 * w_a >=_mu w_b  gdw  a_i >= b_i f.a. i=1..n
 *                     und a_0 + sum(a_i*mu) >= b_0 + sum(b_i*mu)
 *                         dabei ist mu = SG_KBO_Mu_Gewicht
 *
 * Beachte dabei:
 *  w_a != w_b ist durch WemoTab garantiert.
 *  Das Ergebnis des Ausdrucks a_0 + sum(a_i*mu) steht in WemoTab (varphi).
 */
static void Gewichtsinferenz_fill_initially_with_rel_mu(void){
  ExtraConstT sid, tid;
  RelT rel;
  unsigned long varphi_sid, varphi_tid;
  blah(("Gewichtsinferenz_fill_initially_with_rel_mu():\n"));

  /* nur unteres Dreieck bearbeiten, Symmetrie ausnutzen */
  for (sid = 2; sid <= weight_relations.aktSize; sid++){
    for (tid = 1; tid < sid; tid++){
      rel = Gewichtsinferenz_automat_benutzen(sid, tid);
      varphi_sid = WemoTab_get_varphi(sid);
      varphi_tid = WemoTab_get_varphi(tid);

      blah(("sid= %d, tid= %d\n varphi_sid= %d, varphi_tid= %d, Automat= %s\n",
            sid, tid, varphi_sid, varphi_tid, RelTNames[rel]));

      if ((rel == ge) || (rel == eq)){
        if (varphi_sid > varphi_tid){
          WemoTab_print();
          WRelTab_insertRel(sid, tid, gt, weight_rel_mu);
        }
        else if (varphi_sid >= varphi_tid){
          WemoTab_print();
          WRelTab_insertRel(sid, tid, ge, weight_rel_mu);
        }
      }
      if ((rel == le) || (rel == eq)){
        if (varphi_sid < varphi_tid){
          WemoTab_print();
          WRelTab_insertRel(sid, tid, lt, weight_rel_mu);
        } 
        else if (varphi_sid <= varphi_tid){
          WemoTab_print();
          WRelTab_insertRel(sid, tid, le, weight_rel_mu);
        }
      }
    }
  }
}

/* Berechnet den transitiven Abschluss der WRelTab bzgl. des neuen Eintrags.
 * TRUE: Keine Widersprueche gefunden.
 * FALSE: Widerspruch gefunden. */
static BOOLEAN Gewichtsinferenz_transitiveClosure(void)
{
  int i;
  ExtraConstT sid, tid;
  RelT newrel;

  blah(("Gewichtsinferenz_transitiveClosure():\n"));

  newrel = LogTab_getNewRel(logTab.aktA);
  sid = LogTab_getTermID_s(logTab.aktA);
  tid = LogTab_getTermID_t(logTab.aktA);

  /* Die neue Kante geht von sid nach tid.
   * alle Kanten (i -> sid) koennen jetzt zu (i -> tid) verlaengert werden. */
  for (i = 1; i <= weight_relations.aktSize; i++){
    if (i == sid || i == tid) continue;
    {RelT i_sid, i_tid;
    i_sid = WRelTab_getRel(i, sid);
    i_tid = acctranstab(i_sid, newrel); /* newrel = getRel(sid, tid) */
    if (i_tid != uk){
      blah(("WTrans: rho[%d,%d] = '%s', rho[%d,%d] = '%s' -> rho[%d,%d] = %s\n",
            i, sid, RelTNames[i_sid], sid, tid, RelTNames[newrel], 
            i, tid, RelTNames[i_tid]));	  
      logTab.reasons[0] = WRelTab_getLogEntry(i, sid);
      logTab.reasons[1] = logTab.aktA;
      if (!WRelTab_insertRel(i, tid, i_tid, weight_tr)){
        return FALSE;
      }
    }}
  }

  /* Die neue Kante geht von sid nach tid.
   * alle Kanten (tid -> i) koennen jetzt zu (sid ->i) verlaengert werden. */
  for (i = 1; i <= weight_relations.aktSize; i++){
    if (i == sid || i == tid) continue;
    {RelT tid_i, sid_i;
    tid_i = WRelTab_getRel(tid, i);
    sid_i = acctranstab(newrel, tid_i); /* newrel = getRel(sid, tid) */
    if (sid_i != uk){
      blah(("Trans: rho[%d,%d] = '%s', rho[%d,%d] = '%s' -> rho[%d,%d] = %s\n",
	    sid, tid, RelTNames[newrel], tid, i, RelTNames[tid_i], 
	    sid, i, RelTNames[sid_i]));	  
      logTab.reasons[0] = logTab.aktA;
      logTab.reasons[1] = WRelTab_getLogEntry(tid, i);
      if (!WRelTab_insertRel(sid, i, sid_i, weight_tr)){
	return FALSE;
      }
    }}
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
static BOOLEAN Inferenz_Strengthening(void)
{
  ExtraConstT sid, tid;
  RelT rel;
/*   blah(("Strengtheningversuch\n")); */
  sid = LogTab_readLogPart(logTab.aktA, 1);
  tid = LogTab_readLogPart(logTab.aktA, 2);
  rel = LogTab_readLogPart(logTab.aktA, 4);
  if ((rel == ge) || (rel == le)) {
    SymbolT stop = MemoTab_readTop(sid);
    SymbolT ttop = MemoTab_readTop(tid);
    if ((SO_SymbIstFkt(stop) && SO_SymbIstFkt(ttop) && (stop != ttop))
        || !U1_Unifiziert(MemoTab_term(sid), MemoTab_term(tid))){
      blah(("U1_Unifiziert(%t, %t) = FALSE\n", 
            MemoTab_term(sid), MemoTab_term(tid)));
      if(rel == ge) {
        blah(("Strengthening [%d,%d]: %t >= %t  ->  %t > %t\n", 
              sid,tid,MemoTab_term(sid), MemoTab_term(tid),
              MemoTab_term(sid), MemoTab_term(tid)));
        logTab.reasons[0] = RelTab_getLogEntry(sid,tid);
        return RelTab_insertRel(sid, tid, gt, st);
      } else { /* (rel == le) */
        blah(("Strengthening [%d,%d]: %t <= %t  ->  %t < %t\n", 
              sid,tid,MemoTab_term(sid), MemoTab_term(tid),
              MemoTab_term(sid), MemoTab_term(tid)));
        logTab.reasons[0] = RelTab_getLogEntry(sid,tid);
        return RelTab_insertRel(sid, tid, lt, st);
      }
    }
  }
  return TRUE;
}


/* Soll s >=, <=, > oder < t sein, und top(s) und top(t) sind Fkt-Symbole,
 * top(s) == top(t) und bis auf ein paar sind alle Teiltermpaare gleich oder
 * identisch --> pacman
 * also: wenn f(...s...) ? f(...t...), dann s ? t
 * TRUE: Entweder: kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */

static BOOLEAN Inferenz_Pacman_Kandidat(ExtraConstT sid, RelT rel, ExtraConstT tid)
{
  SymbolT stop = MemoTab_readTop(sid);
  SymbolT ttop = MemoTab_readTop(tid);

  if (SO_SymbIstFkt(stop) && (stop == ttop)){
    unsigned int i;
    unsigned int n = SO_Stelligkeit(stop);
    unsigned int k = 0;
    unsigned int r = 0;
    blah(("Pacman-Kandidat: %t %s %t\n", 
	  MemoTab_term(sid), RelTNames[rel], MemoTab_term(tid)));
    logTab.reasons[r] = RelTab_getLogEntry(sid,tid);
    r++;
    for (i = 1; i <= n; i++){
      ExtraConstT s_i = MemoTab_readTT(sid, i);
      ExtraConstT t_i = MemoTab_readTT(tid, i);
      if (RelTab_getRel(s_i,t_i) != eq){
        if (k != 0){ /* gescheitert: mehr als eine Stelle mit != */
          blah(("Pacman gescheitert: Stelle %d und %d beide ungleich\n",k,i));
          LogTab_initReasons();/* reasons zuruecksetzen */
          return TRUE;
        }
        k = i;
      }
      else { /* reasons eintragen */
        logTab.reasons[r] = RelTab_getLogEntry(s_i,t_i);
        r++;
      }
    }
    if (k == 0){/* Alle TT-Paare identisch */
      /* was tun ??? Eigentlich Widerspruch! Wird aber von Lift uebenommen. */
      blah(("Pacman: no plan!\n"));
      LogTab_initReasons();/* reasons zuruecksetzen */
    }
    else { /* eigentlicher Pacman */
      return RelTab_insertRel(MemoTab_readTT(sid,k), 
                              MemoTab_readTT(tid,k), rel, pm);
    }
  }
  return TRUE;
}

static BOOLEAN Inferenz_Pacman(void)
{
  ExtraConstT sid = LogTab_readLogPart(logTab.aktA, 1);
  ExtraConstT tid = LogTab_readLogPart(logTab.aktA, 2);
  RelT        rel = LogTab_readLogPart(logTab.aktA, 4);
  if (ineqRel(rel)){ /* >, >=, <, <= */
    return Inferenz_Pacman_Kandidat(sid, rel, tid);
  }
  else if (rel == eq) { /* alle potentiellen Elternpaare untersuchen */
    unsigned i;
    for (i = 1; i <= SO_MaximaleStelligkeit; i++){
      unsigned int p_s = MemoTab_readParent(sid,i);
      unsigned int p_t = MemoTab_readParent(tid,i);
      if ((p_s != 0) && (p_t != 0)){ /* es gibt also mind. 1 Elternpaar */
        do {
          ExtraConstT p_sid = MemoTab_CurParent(p_s);
          do {
            ExtraConstT p_tid = MemoTab_CurParent(p_t);
            RelT p_rel = RelTab_getRel(p_sid,p_tid);
            if (ineqRel(p_rel)){
              Inferenz_Pacman_Kandidat(p_sid, p_rel, p_tid);
            }
            p_t = MemoTab_NextParent(p_t);
          } while (p_t != 0);
          p_t = MemoTab_readParent(tid,i);
          p_s = MemoTab_NextParent(p_s);
        } while (p_s != 0);
      }
    }
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
  unsigned int n = SO_Stelligkeit(MemoTab_readTop(sid));

  /* Wir muessen hier anfangen die reasons einzutragen,
     auch wenn Lift spaeter scheitern sollte. */
  logTab.reasons[0] = logTab.aktA;

  /* Ueber alle Teilterme von sid und tid. */
  for (i = 1; i <= n; i++){
    reli = RelTab_getRel(MemoTab_readTT(sid, i), 
			 MemoTab_readTT(tid, i));
    res = acctranstab(res, reli); /* Transtab beschreibt genau den Automaten. */
    /* Wir muessen die reasons hier eintragen, 
       auch wenn Lift spaeter scheitern sollte. */
    logTab.reasons[i] = RelTab_getLogEntry(MemoTab_readTT(sid, i), 
					   MemoTab_readTT(tid, i));
    if (res == uk){
      blah(("Liftgescheitert: %t %s %t weil: %t %s %t\n",
	    MemoTab_term(sid), RelTNames[reli], MemoTab_term(tid),
	    MemoTab_term(MemoTab_readTT(sid, i)), RelTNames[reli],
	    MemoTab_term(MemoTab_readTT(tid, i))));
      LogTab_initReasons();
      return TRUE; /*Lift ist zwar gescheitert, jedoch nicht durch Widerspruch*/
    }
  }
  blah(("Lifterfolg: %t %s %t\n",
	MemoTab_term(sid), RelTNames[res], MemoTab_term(tid)));
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
  unsigned i;
  ExtraConstT sid, tid;

/*   blah(("Liftversuch\n")); */
  sid = LogTab_getTermID_s(logTab.aktA);
  tid = LogTab_getTermID_t(logTab.aktA);

#if USE_PARENTS
  /* alle potentiellen Elternpaare untersuchen */
  for (i = 1; i <= SO_MaximaleStelligkeit; i++){
    unsigned int p_s = MemoTab_readParent(sid,i);
    unsigned int p_t = MemoTab_readParent(tid,i);
    if ((p_s != 0) && (p_t != 0)){ /* es gibt also mind. 1 Elternpaar */
      do {
        ExtraConstT p_sid = MemoTab_CurParent(p_s);
        do {
          ExtraConstT p_tid = MemoTab_CurParent(p_t);
          /* Hat p_sid das gleiche Topsymbol wie p_tid? */
          if ((MemoTab_readTop(p_sid) == MemoTab_readTop(p_tid))){
            blah(("Liftkandidat: %t ? %t Stelle: %d\n",
                  MemoTab_term(p_sid), MemoTab_term(p_tid), i));
            if(!Inferenz_Lift_Kandidat(p_sid, p_tid)){
              return FALSE;
            }
          }
          p_t = MemoTab_NextParent(p_t);
        } while (p_t != 0);
        p_t = MemoTab_readParent(tid,i);
        p_s = MemoTab_NextParent(p_s);
      } while (p_s != 0);
    }
  }
  return TRUE;
}
#else /* alter Lift-Kode ohne parents */
  /* Ueber alle Eintraege der MemoTab. */
  /* TODO: Genuegt s = sid + 1 */
  for (s = 1; s <= MemoTab_maxEintrag(); s++){ 
    unsigned int i;
    /* Ueber alle Teilterme eines MemoTab-Eintrags. */
    for (i = 1; i <= SO_Stelligkeit(TO_TopSymbol(MemoTab_term(s))); i++){ 
      /* Ist Term mit ID sid Teilterme von Term s? */
      if (MemoTab_readTT(s,i) == sid){
	int t;
	/* Ueber alle Eintraege der MemoTab. */
	/* TODO: Genuegt t = tid + 1 */
	for (t = 1; t <= MemoTab_maxEintrag(); t ++){
	  /* s wird ausgelassen. */
	  if (t == s){
	    continue;
	  }
	  /* Hat t das gleiche Topsymbol wie s?
	   * Und steht bei t an der gleiche Stelle tid, an der in s sid steht?*/
	  if ((MemoTab_readTop(t) == MemoTab_readTop(s))
	      && (MemoTab_readTT(t,i) == tid)){
	    blah(("Liftkandidat: %t ? %t Stelle: %d\n",
		  MemoTab_term(s), MemoTab_term(t), i));
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
#endif /* alter Lift-Kode ohne parents */

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
    stop = MemoTab_readTop(sid);
    ttop = MemoTab_readTop(tid);
    if (SO_SymbIstFkt(stop) && (stop == ttop)) {
      int i;
      int stelligkeit = SO_Stelligkeit(stop);
      blah(("Decompose: %t %s %t\n",
            MemoTab_term(sid), RelTNames[newrel], MemoTab_term(tid)));
      /* Fuer alle Teilterme */
      for (i = 1; i <= stelligkeit; i++){ 
        if (MemoTab_readTT(sid,i) == 0){
          our_error("huch");
          break; /* keine Teilterme mehr da */
        }
        logTab.reasons[0] = logTab.aktA;
        if (!RelTab_insertRel(MemoTab_readTT(sid,i),
                              MemoTab_readTT(tid,i), eq, dc)){
          return FALSE;
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
  if (ORD_OrdIstLPO && lpo_pack) {
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
    for (term_id = 1; term_id <= MemoTab_maxEintrag(); term_id++){ 
      unsigned int teilterm_id;
      /* Ueber alle Teilterme eines MemoTab-Eintrags. */
      unsigned int stelligkeit;
      stelligkeit = SO_Stelligkeit(TO_TopSymbol(MemoTab_term(term_id)));
      for (teilterm_id = 1; teilterm_id <= stelligkeit; teilterm_id++){
        /* Ist Term mit ID s_i Teilterm von Term mit ID term_id? */
        if (MemoTab_readTT(term_id,teilterm_id) == s_i){
          blah(("Inferenz_LPO_Alpha_Positiv_Erfolg: %t %s %t -> %t > %t\n",
                MemoTab_term(sid), RelTNames[rel], MemoTab_term(tid),
                MemoTab_term(term_id), MemoTab_term(t)));
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
  stelligkeit = SO_Stelligkeit(TO_TopSymbol(MemoTab_term(tid)));

  if (erster_tt < 1 || erster_tt > stelligkeit){ /* Error-Handling */
    our_error("SuffSolver: inferenz_LPO_Majorisierung: ungueltige TT-Angabe!\n");
  }

  for (teilterm_id = erster_tt; teilterm_id <= stelligkeit; teilterm_id++){
    /* wir muessen hier die Begruendungen eintragen,
       auch, wenn wir scheitern */
    logTab.reasons[teilterm_id] = RelTab_getLogEntry(sid,
                                    MemoTab_readTT(tid, teilterm_id));
    if (RelTab_getRel(sid, MemoTab_readTT(tid, teilterm_id)) != gt){
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
  if (ORD_OrdIstLPO && lpo_pack) {
    RelT rel = LogTab_getNewRel(logTab.aktA);
    if (rel == gt || rel == lt){ /* sonst kann die Inferenz nicht greifen */
      ExtraConstT sid, tid, s, t_j;
      int term_id;
      unsigned int i;
      sid = LogTab_getTermID_s(logTab.aktA);
      tid = LogTab_getTermID_t(logTab.aktA);
      
      blah(("Inferenz_LPO_Beta_Positiv_Versuch\n"));

      /* haben sid > tid oder sid < tid, erwarten unten s > t_j */
      if (rel == gt) {
        s = sid;
        t_j = tid;
      }
      else /*if (rel == lt)*/ {
        s = tid;
        t_j = sid;
      }

      if (SO_SymbIstVar(TO_TopSymbol(MemoTab_term(s)))){
        return TRUE;
      }


#if USE_PARENTS
      /* suche Eltern von t_j */
      for (i = 1; i <= SO_MaximaleStelligkeit; i++){
        unsigned int p_t = MemoTab_readParent(t_j, i);
        if (p_t != 0){ /* t_j ist Teilterm von p_t an stelle i */
          do {
            ExtraConstT p_tid = MemoTab_CurParent(p_t);
            /* ist top(s) >_F top(p_tid)
             * und s > t_i i!=j ? */
            if (SO_SymbIstFkt(TO_TopSymbol(MemoTab_term(p_tid)))
                && PR_Praezedenz(TO_TopSymbol(MemoTab_term(s)), 
                                 TO_TopSymbol(MemoTab_term(p_tid))) == Groesser
                && inferenz_LPO_Majorisierung(s, p_tid, 1)){

              blah(("Inferenz_LPO_Beta_Positiv_Erfolg: %t %s %t -> %t > %t\n",
                    MemoTab_term(sid), RelTNames[rel], MemoTab_term(tid),
                    MemoTab_term(s), MemoTab_term(p_tid)));
              logTab.reasons[0] = RelTab_getLogEntry(sid,tid);
              if (!RelTab_insertRel(s, p_tid, gt, lpo_beta_pos)) {
                return FALSE;
              }
            }
            p_t = MemoTab_NextParent(p_t);
          } while (p_t != 0);
        }
      }

#else /* alter Kode ohne Parents */

      /* Ueber alle Eintraege der MemoTab. */
      /* TODO: Genuegt term_id = s_i + 1 */
      for (term_id = 1; term_id <= MemoTab_maxEintrag(); term_id++){ 
        unsigned int teilterm_id;
        /* Ueber alle Teilterme eines MemoTab-Eintrags. */
        unsigned int stelligkeit;
        stelligkeit = SO_Stelligkeit(TO_TopSymbol(MemoTab_term(term_id)));
        for (teilterm_id = 1; teilterm_id <= stelligkeit; teilterm_id++){
          /* Ist Term mit ID t_j Teilterm von Term mit ID term_id?
           * und top(s) >_F top(term_id) 
           * und s > t_i i!=j ? */
          if (MemoTab_readTT(term_id, teilterm_id) == t_j
              && SO_SymbIstFkt(TO_TopSymbol(MemoTab_term(term_id)))
              && PR_Praezedenz(TO_TopSymbol(MemoTab_term(s)), 
                               TO_TopSymbol(MemoTab_term(term_id))) == Groesser
              && inferenz_LPO_Majorisierung(s, term_id, 1)){
            blah(("Inferenz_LPO_Beta_Positiv_Erfolg: %t %s %t -> %t > %t\n",
                  MemoTab_term(sid), RelTNames[rel], MemoTab_term(tid),
                  MemoTab_term(s), MemoTab_term(term_id)));
            logTab.reasons[0] = RelTab_getLogEntry(sid,tid);
            if (!RelTab_insertRel(s, term_id, gt, lpo_beta_pos)) {
              return FALSE;
            }
          }
        }
      }
#endif
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

  if (TO_TopSymbol(MemoTab_term(sid)) != TO_TopSymbol(MemoTab_term(tid))){
    our_error("SuffSolver: inferenz_LPO_Gamma_Lexiko_Majo: top(s) != top(t)\n");
  }

  rho = eq;
  stelligkeit = SO_Stelligkeit(TO_TopSymbol(MemoTab_term(sid)));
  for (teilterm_id = 1; teilterm_id <= stelligkeit; teilterm_id++){
    s_tt = MemoTab_readTT(sid, teilterm_id);
    t_tt = MemoTab_readTT(tid, teilterm_id);
    rho_i = RelTab_getRel(s_tt, t_tt);
    if (rho_i == gt || rho_i == ge || rho_i == eq){
      rho = accmaxtab(rho, rho_i);
      /* wir muessen hier die Begruendungen eintragen,
         auch, wenn wir scheitern */
      logTab.reasons[teilterm_id] = RelTab_getLogEntry(s_tt,
                                      MemoTab_readTT(tid, teilterm_id));
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
  if (ORD_OrdIstLPO && lpo_pack) {
    RelT rel, newrel;
    ExtraConstT sid, tid, s, t;
    int term_id_1, term_id_2, term_id;
    unsigned int i;
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

//    blah(("%t %s %t\n", MemoTab_term(s), RelTNames[rel], MemoTab_term(t)));

    /* Fall 1: */
#if USE_PARENTS
    /* alle potentiellen Elternpaare untersuchen */
    for (i = 1; i <= SO_MaximaleStelligkeit; i++){
      unsigned int p_s = MemoTab_readParent(s,i);
      unsigned int p_t = MemoTab_readParent(t,i);
      if ((p_s != 0) && (p_t != 0)){ /* es gibt also mind. 1 Elternpaar */
        do {
          ExtraConstT p_sid = MemoTab_CurParent(p_s);
          do {
            ExtraConstT p_tid = MemoTab_CurParent(p_t);
            if (TO_TopSymbol(MemoTab_term(p_sid))
                             == TO_TopSymbol(MemoTab_term(p_tid))){
              newrel = inferenz_LPO_Gamma_Lexiko_Majo(p_sid, p_tid);
              if (newrel == gt){
                blah(("Inferenz_LPO_Gamma_Positiv_Erfolg(Fall 1): "));
                blah(("%t %s %t -> %t > %t\n",
                      MemoTab_term(s), RelTNames[rel], MemoTab_term(t),
                      MemoTab_term(p_sid), MemoTab_term(p_tid)));
                logTab.reasons[0] = RelTab_getLogEntry(sid,tid);
                if (!RelTab_insertRel(p_sid, p_tid, gt, lpo_gamma_pos)) {
                  return FALSE;
                }
              }
            }
            p_t = MemoTab_NextParent(p_t);
          } while (p_t != 0);
          p_t = MemoTab_readParent(tid,i);
          p_s = MemoTab_NextParent(p_s);
        } while (p_s != 0);
      }
    }

#else /* alter Kode ohne parents */
    /* Ueber alle Eintraege der MemoTab. */
    for (term_id_1 = 1; term_id_1 <= MemoTab_maxEintrag(); term_id_1++){ 
      unsigned int teilterm_id_1;
      /* Ueber alle Teilterme eines MemoTab-Eintrags. */
      unsigned int stelligkeit_1;
      stelligkeit_1 = SO_Stelligkeit(TO_TopSymbol(MemoTab_term(term_id_1)));
      for (teilterm_id_1 = 1; teilterm_id_1 <= stelligkeit_1; teilterm_id_1++){
        /* Ist der Term mit ID s Teilterm von Term mit ID term_id_1? */
        if (MemoTab_readTT(term_id_1, teilterm_id_1) == s){
          /* Gibt es einen Term term_id_2 mit top(term_id_1) == top(term_id_2)
             und term_id_2 hat an der gleichen Stelle wie term_id_1 t stehen? */
          /* Ueber alle Eintraege der MemoTab. */
          for (term_id_2 = 1; term_id_2 <= MemoTab_maxEintrag(); term_id_2++){ 
            unsigned int teilterm_id_2;
            /* Ueber alle Teilterme eines MemoTab-Eintrags. */
            unsigned int stelligkeit_2;
            if (TO_TopSymbol(MemoTab_term(term_id_1)) 
                != TO_TopSymbol(MemoTab_term(term_id_2))){
              continue;
            }
            stelligkeit_2 = SO_Stelligkeit(TO_TopSymbol(MemoTab_term(term_id_2)));
            for (teilterm_id_2=1; teilterm_id_2<=stelligkeit_2; teilterm_id_2++){
              /* Ist der Term mit ID t Teilterm von Term mit ID term_id_2? */
              if (MemoTab_readTT(term_id_2, teilterm_id_2) == t){
                if (teilterm_id_1 == teilterm_id_2){ /* gleiche Stelle? */
                  newrel = inferenz_LPO_Gamma_Lexiko_Majo(term_id_1, term_id_2);
                  if (newrel == gt){
                    blah(("Inferenz_LPO_Gamma_Positiv_Erfolg(Fall 1): %t %s %t -> %t > %t\n",
                          MemoTab_term(s), RelTNames[rel], MemoTab_term(t),
                          MemoTab_term(term_id_1), MemoTab_term(term_id_2)));
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
#endif

    /* Fall 2: */
#if USE_PARENTS
    /* alle moeglichen Eltern von t suchen */
    for (i = 1; i <= SO_MaximaleStelligkeit; i++){
      unsigned int p_t = MemoTab_readParent(t,i);
      if (p_t != 0){
        do {
          ExtraConstT p_tid = MemoTab_CurParent(p_t);
          /* Sonst kann Gamma-Fall nicht greifen. */
          if (TO_TopSymbol(MemoTab_term(s))
              == TO_TopSymbol(MemoTab_term(p_tid))){
            newrel = inferenz_LPO_Gamma_Lexiko_Majo(s, p_tid);
            if (newrel == gt){
              blah(("Inferenz_LPO_Gamma_Positiv_Erfolg(Fall 2): "));
              blah(("%t %s %t -> %t > %t\n",
                    MemoTab_term(s), RelTNames[rel], MemoTab_term(t),
                    MemoTab_term(s), MemoTab_term(p_tid)));
              logTab.reasons[0] = RelTab_getLogEntry(sid,tid);
              if (!RelTab_insertRel(s, p_tid, gt, lpo_gamma_pos)) {
                return FALSE;
              }
            }
          }
          p_t = MemoTab_NextParent(p_t);
        } while (p_t != 0);
      }
    }
#else
    /* Ueber alle Eintraege der MemoTab. */
    for (term_id = 1; term_id <= MemoTab_maxEintrag(); term_id++){ 
      unsigned int teilterm_id;
      /* Ueber alle Teilterme eines MemoTab-Eintrags. */
      unsigned int stelligkeit;
      /* Sonst kann Gamma-Fall nicht greifen. */
      if (TO_TopSymbol(MemoTab_term(s))
          != TO_TopSymbol(MemoTab_term(term_id))){
        continue;
      }
      stelligkeit = SO_Stelligkeit(TO_TopSymbol(MemoTab_term(term_id)));
      for (teilterm_id = 1; teilterm_id <= stelligkeit; teilterm_id++){
        /* Ist der Term mit ID t Teilterm von Term mit ID term_id? */
        if (MemoTab_readTT(term_id, teilterm_id) == t){
          newrel = inferenz_LPO_Gamma_Lexiko_Majo(s, term_id);
          if (newrel == gt){
            blah(("Inferenz_LPO_Gamma_Positiv_Erfolg(Fall 2): %t %s %t -> %t > %t\n",
                  MemoTab_term(s), RelTNames[rel], MemoTab_term(t),
                  MemoTab_term(s), MemoTab_term(term_id)));
            logTab.reasons[0] = RelTab_getLogEntry(sid,tid);
            if (!RelTab_insertRel(s, term_id, gt, lpo_gamma_pos)) {
              return FALSE;
            }
          }
        }
      }
    }
#endif   
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

/* gibt an, ob Tabelle geaendert wurde. */
static BOOLEAN weightTabChanged = FALSE;

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
    weightTabChanged = TRUE;
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
    weightTabChanged = TRUE;
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
    weightTabChanged = TRUE;
    KBO_wTab.entries[varID].max_w = new_eq_w;
    KBO_wTab.entries[varID].min_w = new_eq_w;
    KBO_wTab.entries[varID].eq_w = new_eq_w;
    return TRUE;
  }
  return FALSE;
}
                                       

static void KBO_WeightTab_print(void)
{
  if (ausgaben){
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
  if (ausgaben){
    unsigned int i;
    blah(("varKoeffVecPrint():\n"));
    for (i = 0; i < varKoeffVecSize; i++){ /* ab i=0 (Zelle fuer Summe der
                                              Gewichte der Funktionssymbole) */
      blah(("A[%d]= %d ", i, varKoeffVec[i]));
    }
    blah(("\n"));
  }
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

/* Ueberprueft, ob in varKoeffVec nur an einer Varstelle =1 steht. 
 * Rueckgabe ist die ID dieser Var oder 0, falls keine solche vorhanden. */
static unsigned int KBO_variablen_gewicht_eins_eq_1(void){
  unsigned int gefunden = 0;
  unsigned int i;
  for (i = 1; i < varKoeffVecSize; i++){ /*ab 1,weil hier nur Vars interessa*/
    if (varKoeffVec[i] == 1){
      if (gefunden != 0){
        return 0; /* Es gibt mehr als eine Var mit Koeff ==1. */
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

/* Ueberprueft, ob in varKoeffVec an allen Varstellen k != i etwas <= 0 steht
 * und ob 0 > a_0 + a_1 +...+ a_n (incl. a_i!) gilt. (a_0 ist Funksym Teil) */
static BOOLEAN KBO_variablen_gewicht_bedingung(unsigned int i)
{
  unsigned int k;
  int sum = varKoeffVec[0];
  for (k = 1; k < varKoeffVecSize; k++){ /*ab 1,weil hier nur Vars interessa*/
    sum += varKoeffVec[k];
    if (k == i){
      continue;
    }
    if (varKoeffVec[k] > 0){
      return FALSE;
    }
  }
  if (0 <= sum){
    return FALSE;
  }
  return TRUE;
}

/* Geg: s rho t, rho in {>, >=, =}
 * phidiff(s,t) bilden (wie in KBO_Vortest_Elem_1). Ergebnis sei
 * [a_0,a_1,...,a_n]
 * 
 * Falls
 *   * genau ein a_i == 1,
 *   * fuer alle a_j, 0 < j =/= i ist a_j <= 0
 *   * 0 > a_0 + a_1 +...+ a_n (incl. a_i!)
 * 
 * Dann x_i > x_k fuer alle k > 0 mit a_k < 0 aufnehmen.
 * TRUE: Entweder: kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */
static BOOLEAN Inferenz_KBO_variablen_gewicht(void)
{
  if (ORD_OrdIstKBO && kbo_var_inf){
    RelT rel;
    ExtraConstT sid, tid, s, t;
    unsigned int varID;
    sid = LogTab_getTermID_s(logTab.aktA);
    tid = LogTab_getTermID_t(logTab.aktA);
    rel = LogTab_getNewRel(logTab.aktA);

    blah(("Inferenz_KBO_variablen_gewicht_Versuch: %t %s %t\n", 
          MemoTab_term(s), RelTNames[rel], MemoTab_term(t)));

    /* Wir erwarten unten s rho t mit rho in {>, >=, =} */
    if (rel == lt || rel == le){
      rel = swap(rel);
      s = tid;
      t = sid;
    } else {
      s = sid;
      t = tid;
    }

    varKoeffVecNullen();
    varKoeffVecTermModifizieren(MemoTab_term(s), 1); /* Vars und Funks von s
                                                      * addieren */
    varKoeffVecTermModifizieren(MemoTab_term(t), -1); /*Vars und Funks von t
                                                       * subtrahieren */
    varID = KBO_variablen_gewicht_eins_eq_1();

    if (varID != 0){
      if (KBO_variablen_gewicht_bedingung(varID)){
        /* Dann x_varID > x_k fuer alle k > 0 mit a_k < 0 aufnehmen. */
        unsigned int k;
        for (k=1; k<varKoeffVecSize; k++){ /*ab 1,weil hier nur Vars interessa*/
          if (k == varID || varKoeffVec[k] >= 0){
            continue;
          }
          blah(("Inferenz_KBO_variablen_gewicht_Erfolg: Aus %t %s %t: %t %s %t\n", 
                MemoTab_term(s), RelTNames[rel], MemoTab_term(t),
                MemoTab_term(MemoTab_varID_to_ID(varID)),
                RelTNames[gt],
                MemoTab_term(MemoTab_varID_to_ID(k))));
          
          logTab.reasons[0] = RelTab_getLogEntry(sid,tid);
          if (!RelTab_insertRel(MemoTab_varID_to_ID(varID),
                                MemoTab_varID_to_ID(k),
                                gt,kbo_var)){
            return FALSE;
          }
        }
      }
    }
  }
  return TRUE;
}



static BOOLEAN suff_Abschluss(void);
static BOOLEAN Inferenz_KBO_aufwaerts_1_und_2(void);

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
        s = MemoTab_term(i);
        t = MemoTab_term(j);
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
    weightTabChanged = FALSE;
    ret = KBO_Vortest(econs);
    //    IO_DruckeFlex("KBO_Nachtest(): erfuellbar: %b, geaendert: %b\n",ret,weightTabChanged);
    CO_deleteEConsList(econs,NULL);
    if (!ret){
      return FALSE;
    }
    if (starkerPostTest && weightTabChanged){
      if (!Inferenz_KBO_aufwaerts_1_und_2()){
        //  IO_DruckeFlex("KBO_Nachtest(): Peng 1\n");
        return FALSE;
      }
      if (!suff_Abschluss()){
        // IO_DruckeFlex("KBO_Nachtest(): Peng 2\n");
        return FALSE;
      }
      //      IO_DruckeFlex("KBO_Nachtest(): Nuescht\n");
    }
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
 *
 * Vorbedingung: KBO_WeightTab muss gefuellt sein,
 *               z.B. durch KBO_Vortest
 *
 * TRUE: Entweder: kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */
static BOOLEAN Inferenz_KBO_aufwaerts_1_und_2(void)
{
  if (ORD_OrdIstKBO && kbo_pack && wPraetest){
    int i,j,diff;

    blah(("Inferenz_KBO_aufwaerts_1_und_2():\n"));

    for (i = 1; i <= relations.aktSize; i++){
      for (j = 1; j <= relations.aktSize; j++){
        if (RelTab_getRel(i,j) != gt){ /* Diese Inferenz ist eine Obermenge von
                                          RelTab_Ordnung_eintragen() */
          if (!KBO_Phi_Differenz_w(MemoTab_term(i), MemoTab_term(j), &diff)){
            continue; /* FALSE: Es ist keine Aussage moeglich. */
          }
          if (diff > 0){
            blah(("Inferenz_KBO_aufwaerts_1_und_2(): Erfolg "));
            blah(("%t > %t, weil Phi(s)-Phi(t) > 0\n",
                  MemoTab_term(i), MemoTab_term(j)));
            if (!RelTab_insertRel(i,j,gt,kbo_auf)){
              return FALSE;
            }
          }
          else if (diff == 0){ /* dann kann Praezedenz noch entscheiden */
            if (TO_TermIstVar(MemoTab_term(i))
                || TO_TermIstVar(MemoTab_term(j))){
              continue; /* dann hilft uns die Praezedenz auch nicht */
            }
            if (PR_Praezedenz(TO_TopSymbol(MemoTab_term(i)), 
                              TO_TopSymbol(MemoTab_term(j))) == Groesser){
              blah(("Inferenz_KBO_aufwaerts_1_und_2(): Erfolg "));
              blah(("%t > %t, weil Phi(s)-Phi(t) = 0 und top(s) >_F top(t)\n",
                    MemoTab_term(i), MemoTab_term(j)));
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

  if (TO_TopSymbol(MemoTab_term(sid)) != TO_TopSymbol(MemoTab_term(tid))){
    our_error("SuffSolver: KBO_lexiko: top(s) != top(t)\n");
  }

  stelligkeit = SO_Stelligkeit(TO_TopSymbol(MemoTab_term(sid)));
  for (teilterm_id = 1; teilterm_id <= stelligkeit; teilterm_id++){
    s_tt = MemoTab_readTT(sid, teilterm_id);
    t_tt = MemoTab_readTT(tid, teilterm_id);
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
 *
 * Vorbedingung: KBO_WeightTab muss gefuellt sein,
 *               z.B. durch KBO_Vortest
 *
 * TRUE: Entweder: kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */
static BOOLEAN Inferenz_KBO_aufwaerts_3(void)
{
  if (ORD_OrdIstKBO && kbo_pack && wPraetest){
    unsigned int i;
    unsigned int tt_id, stelligkeit;

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

#if USE_PARENTS
    /* alle potentiellen Elternpaare untersuchen */
    for (i = 1; i <= SO_MaximaleStelligkeit; i++){
      unsigned int p_s = MemoTab_readParent(s,i);
      unsigned int p_t = MemoTab_readParent(t,i);
      if ((p_s != 0) && (p_t != 0)){ /* es gibt also mind. 1 Elternpaar */
        do {
          ExtraConstT p_sid = MemoTab_CurParent(p_s);
          do {
            ExtraConstT p_tid = MemoTab_CurParent(p_t);
            if (TO_TopSymbol(MemoTab_term(p_sid))
                == TO_TopSymbol(MemoTab_term(p_tid))){
              blah(("Inferenz_KBO_aufwaerts_3: Action %t %t\n",
                    MemoTab_term(p_sid), MemoTab_term(p_tid)));
              if (KBO_greaterEqual_w(MemoTab_term(p_sid), MemoTab_term(p_tid))
                  && KBO_lexiko(p_sid, p_tid)){
                blah(("Inferenz_KBO_aufwaerts_3_Erfolg: %t > %t\n",
                      MemoTab_term(p_sid),MemoTab_term(p_tid)));
                logTab.reasons[0] = RelTab_getLogEntry(s, t);
                if (!RelTab_insertRel(p_sid , p_tid, gt, kbo_auf3)){
                  return FALSE;
                }
              }
            }
            p_t = MemoTab_NextParent(p_t);
          } while (p_t != 0);
          p_t = MemoTab_readParent(t, i);
          p_s = MemoTab_NextParent(p_s);
        } while (p_s != 0);
      }
    }

#else /* alter Kode ohne Parents */

    /* Ueber alle Eintraege der MemoTab. */
    for (term_id_1 = 1; term_id_1 <= MemoTab_maxEintrag(); term_id_1++){ 
      unsigned int teilterm_id_1;
      /* Ueber alle Teilterme eines MemoTab-Eintrags. */
      unsigned int stelligkeit_1;
      stelligkeit_1 = SO_Stelligkeit(TO_TopSymbol(MemoTab_term(term_id_1)));
      for (teilterm_id_1 = 1; teilterm_id_1 < stelligkeit_1; teilterm_id_1++){
        /* Ist der Term mit ID s Teilterm von Term mit ID term_id_1? */
        if (MemoTab_readTT(term_id_1, teilterm_id_1) == s){
          /* Gibt es einen Term term_id_2 mit top(term_id_1) == top(term_id_2)
             und term_id_2 hat an der gleichen Stelle wie term_id_1 t stehen? */
          /* Ueber alle Eintraege der MemoTab. */
          for (term_id_2 = 1; term_id_2 <= MemoTab_maxEintrag(); term_id_2++){ 
            unsigned int teilterm_id_2;
            /* Ueber alle Teilterme eines MemoTab-Eintrags. */
            unsigned int stelligkeit_2;
            if (TO_TopSymbol(MemoTab_term(term_id_1)) 
                != TO_TopSymbol(MemoTab_term(term_id_2))){
              continue;
            }
            stelligkeit_2 = SO_Stelligkeit(TO_TopSymbol(MemoTab_term(term_id_2)));
            for (teilterm_id_2=1; teilterm_id_2 < stelligkeit_2; teilterm_id_2++){
              /* Ist der Term mit ID t Teilterm von Term mit ID term_id_2? */
              if (MemoTab_readTT(term_id_2, teilterm_id_2) == t){
                if (teilterm_id_1 == teilterm_id_2){ /* gleiche Stelle? */
                  if (KBO_greaterEqual_w(MemoTab_term(term_id_1),
                                         MemoTab_term(term_id_2))
                      && KBO_lexiko(term_id_1, term_id_2)){
                    blah(("Inferenz_KBO_aufwaerts_3_Erfolg: %t > %t\n",
                          MemoTab_term(term_id_1),MemoTab_term(term_id_2)));
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
#endif /* alter Kode ohne Parents */
  }
  return TRUE;
}

/* Wenn s rho t mit rho in {>, >=} (1)
 * und Phi garantiert phi(sigma(s)) == phi(sigma(t))
 * und top(s) == top(t)
 * und s_j = t_j oder s_j aequiv t_j fuer j < i < n.
 * Dann s_i >= t_i.
 * D.h. ist das neue Wissen nuetzlich bei (1)?
 *
 * Vorbedingung: KBO_WeightTab muss gefuellt sein,
 *               z.B. durch KBO_Vortest
 *
 * TRUE: Entweder: kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */
static BOOLEAN Inferenz_KBO_abwaerts_1(void)
{
  if (ORD_OrdIstKBO && kbo_pack && wPraetest){
    RelT rel;
    ExtraConstT sid, tid, s, t;
    int s_tt, t_tt;
    unsigned int stelligkeit;
    sid = LogTab_getTermID_s(logTab.aktA);
    tid = LogTab_getTermID_t(logTab.aktA);
    rel = LogTab_getNewRel(logTab.aktA);
    
    blah(("Inferenz_KBO_abwaerts_1_Versuch: %t %t\n",
          MemoTab_term(sid),MemoTab_term(tid)));

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

    stelligkeit = SO_Stelligkeit(TO_TopSymbol(MemoTab_term(s)));

    if (stelligkeit > 1
        && TO_TopSymbol(MemoTab_term(s)) == TO_TopSymbol(MemoTab_term(t))
        && KBO_equals_w(MemoTab_term(s),MemoTab_term(t))){
      unsigned int teilterm_id;
      blah(("Inferenz_KBO_abwaerts_1_Versuch_im_if\n"));
      for (teilterm_id = 1; teilterm_id < stelligkeit; teilterm_id++){
        s_tt = MemoTab_readTT(s, teilterm_id);
        t_tt = MemoTab_readTT(t, teilterm_id);
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
 *
 * Vorbedingung: KBO_WeightTab muss gefuellt sein,
 *               z.B. durch KBO_Vortest
 *
 * TRUE: Entweder: kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */
static BOOLEAN Inferenz_KBO_abwaerts_2(void)
{
  if (ORD_OrdIstKBO && kbo_pack && wPraetest){
    unsigned int i;
    unsigned int tt_id, stelligkeit;

    RelT rel;
    ExtraConstT s, t;
    int term_id_1, term_id_2;
    s = LogTab_getTermID_s(logTab.aktA);
    t = LogTab_getTermID_t(logTab.aktA);
    rel = LogTab_getNewRel(logTab.aktA);

    blah(("Inferenz_KBO_abwaerts_2_Versuch: s= %t  t= %t rel= %s\n", 
          MemoTab_term(s), MemoTab_term(t), RelTNames[rel]));

    if (rel != eq){ /* dann kann diese Inferenz nicht greifen */
      return TRUE;
    }


#if USE_PARENTS
    /* alle potentiellen Elternpaare untersuchen */
    for (i = 1; i <= SO_MaximaleStelligkeit-2; i++){
      unsigned int p_s = MemoTab_readParent(s,i);
      unsigned int p_t = MemoTab_readParent(t,i);
      if ((p_s != 0) && (p_t != 0)){ /* es gibt also mind. 1 Elternpaar */
        do {
          ExtraConstT p_sid = MemoTab_CurParent(p_s);
          do {
            ExtraConstT p_tid = MemoTab_CurParent(p_t);

            RelT p_rel = RelTab_getRel(p_sid, p_tid);
            if ((p_rel == lt) || (p_rel == le)){
              unsigned int tmp = p_sid;
              p_sid = p_tid;
              p_tid = tmp;
              p_rel = swap(p_rel);
            }
            /* "Wenn s rho t mit rho in {>, >=} ..." (s.o.) */
            if ((p_rel == gt) || (p_rel == ge)){
              if (TO_TopSymbol(MemoTab_term(p_sid))
                  == TO_TopSymbol(MemoTab_term(p_tid))){
                stelligkeit = SO_Stelligkeit(TO_TopSymbol(MemoTab_term(p_sid)));
                if (i < stelligkeit - 1){ 
                  blah(("Inferenz_KBO_abwaerts_2: Action %t %t\n",
                        MemoTab_term(p_sid), MemoTab_term(p_tid)));
                  if (KBO_equals_w(MemoTab_term(p_sid), MemoTab_term(p_tid))){
                    blah(("Inferenz_KBO_abwaerts_2: equals_w!\n"));
                    /* Vergleich der Teilterme von links nach rechts */
                    for (tt_id = 1; tt_id < stelligkeit; tt_id++){
                      int s_tt, t_tt;
                      s_tt = MemoTab_readTT(p_sid, tt_id);
                      t_tt = MemoTab_readTT(p_tid, tt_id);
                      if (RelTab_getRel(s_tt, t_tt) == eq){
                        /* wir muessen hier die Begruendungen eintragen,
                           auch, wenn wir scheitern */
                        logTab.reasons[tt_id] = RelTab_getLogEntry(s_tt, t_tt);
                      } else { /* Die Kette der eq ist zu Ende und nutzt neues Wissen */
                        if (tt_id > i){
                          logTab.reasons[0] = RelTab_getLogEntry(p_sid, p_tid);
                          blah(("Inferenz_KBO_abwaerts_2_Erfolg\n"));
                          if (!RelTab_insertRel(s_tt, t_tt, ge, kbo_ab_2)){
                            return FALSE;
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
            p_t = MemoTab_NextParent(p_t);
          } while (p_t != 0);
          p_t = MemoTab_readParent(t, i);
          p_s = MemoTab_NextParent(p_s);
        } while (p_s != 0);
      }
    }

#else /* alter Kode ohne Parents */

    /* Ueber alle Eintraege der MemoTab. */
    for (term_id_1 = 1; term_id_1 <= MemoTab_maxEintrag(); term_id_1++){ 
      unsigned int teilterm_id_1;
      /* Ueber alle Teilterme eines MemoTab-Eintrags. */
      unsigned int stelligkeit_1;
      stelligkeit_1 = SO_Stelligkeit(TO_TopSymbol(MemoTab_term(term_id_1)));
      for (teilterm_id_1 = 1; teilterm_id_1 < stelligkeit_1; teilterm_id_1++){
        /* Ist der Term mit ID s Teilterm von Term mit ID term_id_1? */
        if (MemoTab_readTT(term_id_1, teilterm_id_1) == s){
          /* Gibt es einen Term term_id_2 mit top(term_id_1) == top(term_id_2)
             und term_id_2 hat an der gleichen Stelle wie term_id_1 t stehen? */
          /* Ueber alle Eintraege der MemoTab. */
          for (term_id_2 = 1; term_id_2 <= MemoTab_maxEintrag(); term_id_2++){ 

            RelT p_rel = RelTab_getRel(term_id_1, term_id_2);
            /* "Wenn s rho t mit rho in {>, >=} ..." (s.o.) */
            if ( ((p_rel != gt) && (p_rel != ge))
                 ||(TO_TopSymbol(MemoTab_term(term_id_1))
                    != TO_TopSymbol(MemoTab_term(term_id_2)))){
              continue;
            }

            /* Ist der Term mit ID t Teilterm von Term mit ID term_id_2
             * und zwar an der Stelle teilterm_id_1 ? */
            if (MemoTab_readTT(term_id_2, teilterm_id_1) == t){
              blah(("Inferenz_KBO_abwaerts_2: Action %t %t\n",
                    MemoTab_term(term_id_1), MemoTab_term(term_id_2)));
              if (KBO_equals_w(MemoTab_term(term_id_1),
                               MemoTab_term(term_id_2))){
                unsigned int teilterm_id_3;
                blah(("Inferenz_KBO_abwaerts_2: equals_w\n"));
                /* Vergleich der Teilterme von links nach rechts */
                for (teilterm_id_3 = 1; teilterm_id_3 < stelligkeit_1; teilterm_id_3++){
                  int s_tt, t_tt;
                  s_tt = MemoTab_readTT(term_id_1, teilterm_id_3);
                  t_tt = MemoTab_readTT(term_id_2, teilterm_id_3);
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
#endif /* alter Kode ohne Parents */
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
  if (ausgaben){
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
          && SO_SymbIstVar(TO_TopSymbol(MemoTab_term(i)))){
        if (SO_SymbIstVar(TO_TopSymbol(MemoTab_term(j)))){
          /* wenn beide var, dann groessere an kleinere (alles an x1) */
          int varn_i, varn_j;
          varn_i = SO_VarNummer(TO_TopSymbol(MemoTab_term(i)));
          varn_j = SO_VarNummer(TO_TopSymbol(MemoTab_term(j)));
          Subs_setBinding(varn_i > varn_j ? varn_i : varn_j,
                          MemoTab_term(varn_i > varn_j ? j : i));
        } else {
          Subs_setBinding(SO_VarNummer(TO_TopSymbol(MemoTab_term(i))),
                          MemoTab_term(j));
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
	    i,j,MemoTab_term(i), RelTNames[RelTab_getRel(i,j)], MemoTab_term(j)));
      
      r = RelTab_getRel(i,j);
      v = ORD_VergleicheTerme(MemoTab_term(i), MemoTab_term(j));

      if (RelTab_AfterburnerStep(r,v,MemoTab_term(i), MemoTab_term(j),"")){
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
      s = MemoTab_term(i);
      t = MemoTab_term(j);
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

/* -------------------------------------------------------------------------- */
/* -- Infrastruktur fuer Bottom-Up-LPO -------------------------------------- */
/* -------------------------------------------------------------------------- */
static BOOLEAN s_LPO_Greater1BU(UTermT s, UTermT t);

static BOOLEAN s_LPO_Alpha1BU(UTermT ss, UTermT s_NULL, UTermT t);
static BOOLEAN s_LPO_Gamma1BU(UTermT s, UTermT t);
static BOOLEAN s_LPO_Beta1BU(UTermT s, UTermT t);
static BOOLEAN s_LPO_Delta1BU(UTermT s, UTermT t);

static BOOLEAN s_LPO_Majo1BU(UTermT s, UTermT ts, UTermT t_NULL);
static BOOLEAN s_Lex1BU(UTermT ss, UTermT s_NULL, UTermT ts, UTermT t_NULL);

static BOOLEAN s_Equal1BU(UTermT s, UTermT t)
{
  BOOLEAN res;
  UTermT s_i, t_i, s_0;

  s_0 = TO_NULLTerm(s);
  s_i = TO_ErsterTeilterm(s);
  t_i = TO_ErsterTeilterm(t);

  res = TO_TopSymboleGleich(s,t);
  while (res && s_i != s_0){ /* impliziert t_i != t_NULL (feste Stelligk.) */
    res = (RelTab_getRel(TO_GetId(s_i), TO_GetId(t_i)) == eq);
    s_i = TO_NaechsterTeilterm(s_i);
    t_i = TO_NaechsterTeilterm(t_i);
  }
  return res;
}


static BOOLEAN s_LPO_Greater1BU(UTermT s, UTermT t)
{
  BOOLEAN res;

  if (TO_TermIstNichtVar(s)){
    if (TO_TermIstNichtVar(t)){
      res = s_LPO_Alpha1BU(TO_ErsterTeilterm(s), TO_NULLTerm(s), t)
        || s_LPO_Beta1BU(s,t)
        || s_LPO_Gamma1BU(s,t);
    }
    else {
      res = s_LPO_Delta1BU(s,t);
    }
  }
  else {
    res = FALSE;
  }
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Alpha1BU(UTermT ss, UTermT s_NULL, UTermT t)
{
  BOOLEAN res = FALSE; /* default for empty list */
  
  UTermT s_i = ss;
  while(s_i != s_NULL) {    
    if (RelTab_getRel(TO_GetId(s_i), TO_GetId(t)) == eq
        || RelTab_getRel(TO_GetId(s_i), TO_GetId(t)) == gt){
      res = TRUE;
      break;
    }
    else {
      s_i = TO_NaechsterTeilterm(s_i);
    }
  }
  return res;
}

static BOOLEAN s_LPO_Beta1BU(UTermT s, UTermT t) 
{
  BOOLEAN res;

  res = (PR_Praezedenz(TO_TopSymbol(s), TO_TopSymbol(t)) == Groesser) &&
    s_LPO_Majo1BU(s, TO_ErsterTeilterm(t), TO_NULLTerm(t));
  return res;
}


static BOOLEAN s_LPO_Gamma1BU(UTermT s, UTermT t) 
{
  BOOLEAN res;

  res = SO_SymbGleich(TO_TopSymbol(s), TO_TopSymbol(t))     &&
    s_Lex1BU(TO_ErsterTeilterm(s), TO_NULLTerm(s), 
                 TO_ErsterTeilterm(t), TO_NULLTerm(t) )   &&
    s_LPO_Majo1BU(s, TO_ErsterTeilterm(t), TO_NULLTerm(t));
  return res;
}

static BOOLEAN s_LPO_Delta1BU(UTermT s, UTermT t) 
{
  BOOLEAN res;

  res = s_LPO_Alpha1BU(TO_ErsterTeilterm(s), TO_NULLTerm(s), t);
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Majo1BU(UTermT s, UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = TRUE; /* default value for empty list */

  UTermT t_j = ts;
  while(t_j != t_NULL){
    if (RelTab_getRel(TO_GetId(s), TO_GetId(t_j)) == gt) {
      t_j = TO_NaechsterTeilterm(t_j);
    }
    else {
      res = FALSE;
      break;
    }
  }
  return res;
}

static BOOLEAN s_Lex1BU(UTermT ss, UTermT s_NULL, UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = FALSE; /* default value for empty list */

  UTermT s_i  = ss;
  UTermT t_i  = ts;
  while (s_i != s_NULL){ /* Implies t_i != t_NULL (fixed arity!) */ 
    if (RelTab_getRel(TO_GetId(s_i), TO_GetId(t_i)) == eq){ 
      s_i = TO_NaechsterTeilterm(s_i);
      t_i = TO_NaechsterTeilterm(t_i);
    }
    else {
      res = RelTab_getRel(TO_GetId(s_i), TO_GetId(t_i)) == gt;
      break;
    }
  }
  return res;
}


/* -------------------------------------------------------------------------- */
/* -- Infrastruktur fuer Bottom-Up-KBO -------------------------------------- */
/* -------------------------------------------------------------------------- */
static BOOLEAN s_KBO_check_Praez_gt(SymbolT s, SymbolT t)
{
  if (!SO_SymbIstVar(s) && !SO_SymbIstVar(t)){
    return PR_Praezedenz(s, t) == Groesser;
  } else {
    return FALSE;
  }
}

static BOOLEAN s_KBO_Greater_BU(UTermT s, UTermT t)
{
  ExtraConstT s_wid, t_wid;
  RelT wrel;

  s_wid = MemoTab_get_weightID(TO_GetId(s));
  t_wid = MemoTab_get_weightID(TO_GetId(t));
  wrel = WRelTab_getRel(s_wid, t_wid);

  blah(("s_KBO_Greater_BU: s_wid= %d, t_wid= %d, wrel= %s\n", 
        s_wid, t_wid, RelTNames[wrel]));

  if (    (wrel == gt)
       || (    ((wrel == ge) || (wrel == eq))
            && (    s_KBO_check_Praez_gt(TO_TopSymbol(s), TO_TopSymbol(t))
                || (   TO_TopSymboleGleich(s, t)
                    && s_Lex1BU(TO_ErsterTeilterm(s), TO_NULLTerm(s), 
                                    TO_ErsterTeilterm(t), TO_NULLTerm(t)))))){
      return TRUE;
  }
  return FALSE;
}

/* -------------------------------------------------------------------------- */

/* Traegt die Ordnungsbeziehungen in die RelTab ein.
 * Vergleicht Terme nur, wenn RelTab_getRel == uk, weil:
 * Vorbedingungen: 
 *      RelTab_copyTTfromMemTabTransitive() ist vorher schon gelaufen.
 * (KBO)Gewichtsinferenz_fill_initially_with_rel_mu() ist vorher schon gelaufen.
 * Spezialfall ORD_OrdIstLPO: Bottom-Up-Implementierung
 * Spezialfall ORD_OrdIstKBO: Bottom-Up-Implementierung
 *                            nur wenn wGewichtsinferenzen==TRUE (wg. WRelTab)
 * TRUE: Entweder: kein neues Wissen gefunden oder keine Widersprueche gefunden.
 * FALSE: neues Wissen und Widerspruch gefunden. */
static BOOLEAN RelTab_Ordnung_eintragen(void)
{
  int i,j;
  UTermT tt_i, tt_j;
  RelT rel;

  blah(("RelTab_Ordnung_eintragen():\n"));

  if(ORD_OrdIstLPO){
    /* INVARIANTE:
     * Die IDs der Terme werden beim Eintragen in die MemoTab auschliesslich in
     * leftmost-innermost Reihenfolge vergeben. Dann gilt, dass alle Teilterme
     * eine kleinere ID haben, als der Term selbst. Damit ist garantiert, dass
     * die notwendigen Teiltermvergleiche zuerst vollzogen werden.*/
    for (i = 1; i <= relations.aktSize; i++){
      tt_i = MemoTab_term(i);
      for (j = 1; j < i; j++){ /* Nur unteres Dreieck testen. (Symmetrie) */
        tt_j = MemoTab_term(j);
        blah(("%d %t, %d %t\n",i,tt_i,j,tt_j));

        /* Invariante: 
         * RelTab_copyTTfromMemTabTransitive() ist vorher schon gelaufen. */
        if (RelTab_getRel(i,j) != uk){
          blah(("Ordnung bereits durch TT entschieden.\n"));
          continue;
        }

        if (s_Equal1BU(tt_i, tt_j)){
          RelTab_insertRel(i,j,eq,ord);
        } else if (s_LPO_Greater1BU(tt_i, tt_j)){
          RelTab_insertRel(i,j,gt,ord);
        } else if (s_LPO_Greater1BU(tt_j, tt_i)){
          RelTab_insertRel(i,j,lt,ord);
/*         } else {  RelTab wird mit uk initialisiert */
/*           RelTab_insertRel(i,j,uk,ord); */
        }
      }
    }  
#if DEBUG
    /* mit eingestellter LPO testen */
    for (i = 1; i <= relations.aktSize; i++){
      for (j = 1; j < i; j++){ /* Nur unteres Dreieck testen. (Symmetrie) */
        rel = translate(ORD_VergleicheTerme(MemoTab_term(i), MemoTab_term(j)));
        if (rel != RelTab_getRel(i,j)){
          our_error("Bottom-Up-LPO hat versagt!\n");
        }
      }
    }
#endif
  } else  if(ORD_OrdIstKBO && wGewichtsinferenzen){
    blah(("KBO\n"));
    /* INVARIANTE:
     * Die IDs der Terme werden beim Eintragen in die MemoTab auschliesslich in
     * leftmost-innermost Reihenfolge vergeben. Dann gilt, dass alle Teilterme
     * eine kleinere ID haben, als der Term selbst. Damit ist garantiert, dass
     * die notwendigen Teiltermvergleiche zuerst vollzogen werden.*/
    for (i = 1; i <= relations.aktSize; i++){
      tt_i = MemoTab_term(i);
      for (j = 1; j < i; j++){ /* Nur unteres Dreieck testen. (Symmetrie) */
        tt_j = MemoTab_term(j);
        blah(("%d %t, %d %t: ",i,tt_i,j,tt_j));

        /* Invariante: 
         * RelTab_copyTTfromMemTabTransitive() ist vorher schon gelaufen. */
        if (RelTab_getRel(i,j) != uk){
          blah(("Ordnung bereits durch TT entschieden.\n"));
          continue;
        }

        if (s_Equal1BU(tt_i, tt_j)){
          blah((" rel=  eq\n"));
          RelTab_insertRel(i,j,eq,ord);
        } else if (s_KBO_Greater_BU(tt_i, tt_j)){
          blah((" rel=  gt\n"));
          RelTab_insertRel(i,j,gt,ord);
        } else if (s_KBO_Greater_BU(tt_j, tt_i)){
          blah((" rel=  lt\n"));
          RelTab_insertRel(i,j,lt,ord);
/*         } else {  RelTab wird mit uk initialisiert */
/*           RelTab_insertRel(i,j,uk,ord); */
        }
        blah((" rel=  uk\n"));
      }
    }
#if DEBUG
    /* mit eingestellter Ordnung testen */
    for (i = 1; i <= relations.aktSize; i++){
      for (j = 1; j < i; j++){ /* Nur unteres Dreieck testen. (Symmetrie) */
        rel = translate(ORD_VergleicheTerme(MemoTab_term(i), MemoTab_term(j)));
        if (rel != RelTab_getRel(i,j)){
          blah(("i: %d, j: %d, RelTab: %s, Ord: %s\n",
                i, j, RelTNames[RelTab_getRel(i,j)], RelTNames[rel]));
          our_error("Bottom-Up-KBO hat versagt!\n");
        }
      }
    }
#endif
  } else { /* !ORD_OrdIstLPO && !ORD_OrdIstKBO */
    for (i = 1; i <= relations.aktSize; i++){
      for (j = 1; j < i; j++){ /* Nur unteres Dreieck testen. (Symmetrie) */
        /* Invariante: 
         * RelTab_copyTTfromMemTabTransitive() ist vorher schon gelaufen. */
        if (RelTab_getRel(i,j) != uk){
          blah(("Ordnung bereits durch TT entschieden: %t %t\n",
                MemoTab_term(i), MemoTab_term(j)));
          continue;
        }
        rel = translate(ORD_VergleicheTerme(MemoTab_term(i), MemoTab_term(j)));
        if (rel == uk){
          continue; /* uk ist nutzloses Wissen */
        }
        blah(("RelTab_Ordnung_eintragen(): %t %s %t\n",
              MemoTab_term(i), RelTNames[rel], MemoTab_term(j)));
        if (!RelTab_insertRel(i,j,rel,ord)){
          return FALSE;
        }
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
        s = MemoTab_term(i);
        t = MemoTab_term(j);
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
    /* Je nachdem von welchem InferenzTyp der aktuelle LogTabEintrag stammt,
     * werden unterschiedliche Inferenzen angewendet. */
    InfT inf;
    inf = LogTab_getInf(logTab.aktA);

    if (inf >= weight_mu  /* also Gewichtsinferenz */
        && ORD_OrdIstKBO
        && wGewichtsinferenzen
        && inf != weight_fromWRelTab /* aendert ja nur in RelTab */){
      if (!Gewichtsinferenz_transitiveClosure()){
        return FALSE;
      }

    } else { /* also keine Gewichtsinferenz */

      /* Inferenzen: Lift, Pacman, Strengthening, Decompose, Trans, ... */
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
          || !Inferenz_KBO_variablen_gewicht()
          || !Inferenz_transitiveClosure()){
        return FALSE;
      }
    }
  }
  blah(("logTab.aktA == logTab.aktF\n"));

  if (useSubs){
    Subs_evalReltab();
  }

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

  if (ORD_OrdIstKBO && wGewichtsinferenzen){
    WemoTab_init(e);
    WemoTab_print();
    
    WRelTab_init();
    WRelTab_print();
    WRelTab_fill_initially_wrt_Mu();
    WRelTab_print();

    Gewichtsinferenz_fill_initially_with_rel_mu(); /* einmalig am Anfang */
  }

  RelTab_copyTTfromMemTabTransitive();
  RelTab_Ordnung_eintragen(); /* Vorbedingung: vorher lief
                               * RelTab_copyTTfromMemTabTransitive() 
			       * Gewichtsinferenz_fill_initially_with_rel_mu()
			       *      (falls BU-KBO angestellt ist) */

  return 1
    && KBO_Vortest(e)
    && Inferenz_KBO_aufwaerts_1_und_2()  /* Vorbed.: KBO_WeightTab muss gefuellt
                                          * sein, z.B. durch KBO_Vortest */
    && suff_Abschluss()
    && RelTab_insertECons(e)
    && suff_Abschluss()
    //    && RelTab_transitiveClosure_check()
//  && RelTab_Afterburner() /* ist schwaecher als RelTab_Afterburner_Subs() */
    // && RelTab_Afterburner_Subs()
    && KBO_Nachtest()
    && KBO_Afterburner()
    ;
}


/* -------------------------------------------------------------------------- */


/* Mechanismus, um die gewonne Substitution anzuwenden
 * und es dann rekursiv nochmal zu versuchen. */
static BOOLEAN suff_sat2(EConsT e, int level)
{
/*   IO_DruckeFlex("suff_sat2(e= %E, level= %d):\n",e,level); */
  if (level > 5) return TRUE;
  if (suff_satisfiable(e)){
    BOOLEAN res;
    EConsT e2;
    if (!Subs_somethingBound()){
/*       IO_DruckeFlex("suff_sat2(e= %E, level= %d): nothing bound\n",e,level); */
      return TRUE;
    }
/*     IO_DruckeFlex("suff_sat2(e= %E, level= %d): something bound\n",e,level); */
    Subs_print();
    e2 = Subs_apply_cons(e);
    res = suff_sat2(e2,level+1);
    CO_deleteEConsList(e2,NULL);
/*     IO_DruckeFlex("Xsuff_sat2(e= %E, level= %d) = %b\n",e,level,res); */
    return res;
  }
  return FALSE;
}


static BOOLEAN suff_satKBO(EConsT e)
{
#if TIME_MEASURE
  BOOLEAN b1, b2, b3, b4, b5, b6, b7, b8, b9;
  hrtime_t t_start4,t_end4, t_start7,t_end7, t_start8,t_end8, t_start9,t_end9;
  kbo_pack = wPraetest = wPostest = starkerPostTest = omega_burn = FALSE;
  kbo_var_inf = FALSE;
/*   b1 = suff_satisfiable(e); */
  wPraetest = TRUE;
/*   b2 = suff_satisfiable(e); */
  wPostest = TRUE;
/*   b3 = suff_satisfiable(e); */
  kbo_pack = TRUE;
  wPraetest = wPostest = FALSE;

  t_start4 = gethrtime();
  b4 = suff_satisfiable(e); 
  t_end4 = gethrtime();

  wPraetest = TRUE;
/*   b5 = suff_satisfiable(e); */
  wPostest = TRUE;
/*   b6 = suff_satisfiable(e); */
/*   wPraetest = wPostest = FALSE; */
/*   kbo_var_inf = TRUE; */
  /*  starkerPostTest = TRUE;*/
/*   b7 = suff_satisfiable(e); */
  /*  omega_burn = TRUE;*/
/*    b7 = KBOSuff_isSatisfiable1(e);  */

  t_start7 = gethrtime();
  b7 = KBOSuff_isSatisfiable1(e); 
  t_end7 = gethrtime();

  t_start8 = gethrtime();
  b8 = KBOSuff_isSatisfiable2(e); 
  t_end8 = gethrtime();

  t_start9 = gethrtime();
  b9 = Hurd_isSatisfiable(e); 
  t_end9 = gethrtime();
/*   IO_DruckeFlex("## %b %b %b %b %b %b z%b y%b x%b %E\n", */
/*                 b1, b2, b3, b4, b5, b6, b7, b8, b9, e); */
  IO_DruckeFlex("## %b %b %b %b ", b4, b7, b8, b9);
  printf(" %lld %lld %lld %lld ", (t_end4-t_start4), (t_end7-t_start7), 
         (t_end8-t_start8), (t_end9-t_start9));
  IO_DruckeFlex("%E\n", e);
  return b8;

#else /* !TIME_MEASURE */

  BOOLEAN b1, b2, b3, b4, b5, b6, b7, b8, b9;
  Suff_flag = TRUE;
  kbo_pack = wPraetest = wPostest = starkerPostTest = omega_burn = FALSE;
  kbo_var_inf = wGewichtsinferenzen = FALSE;
/*   b1 = suff_satisfiable(e); */
  wPraetest = TRUE;
/*   b2 = suff_satisfiable(e); */
  wPostest = TRUE;
/*   b3 = suff_satisfiable(e); */
  kbo_pack = TRUE;
  wPraetest = TRUE;
  wPostest = FALSE;
  wGewichtsinferenzen = TRUE;
  b4 = suff_satisfiable(e); 

/*   wGewichtsinferenzen = TRUE; */
/*   b5 =  KW_isSatisfiable(e) && suff_satisfiable(e);  */

  wPraetest = TRUE;
/*   b5 = suff_satisfiable(e); */
  wPostest = TRUE;
/*   b6 = suff_satisfiable(e); */
/*   wPraetest = wPostest = FALSE; */
/*   kbo_var_inf = TRUE; */
  /*  starkerPostTest = TRUE;*/
/*   b7 = suff_satisfiable(e); */
  /*  omega_burn = TRUE;*/

/*     b7 = KBOSuff_isSatisfiable1(e);   */
/*     if (0&&b5 && !b7){ */
/*       Suff_flag = TRUE; */
/*       printf("Hallo Bernd\n"); */
/*       b5 = suff_satisfiable(e);  */
/*     } */

/*    b7 = KBOSuff_isSatisfiable1(e);  */
/*    b8 = KBOSuff_isSatisfiable2(e);  */
/*    b9 = Hurd_isSatisfiable(e);  */
/*   IO_DruckeFlex("## %b %b %b %b %b %b z%b y%b x%b %E\n", */
/*                 b1, b2, b3, b4, b5, b6, b7, b8, b9, e); */
/*   IO_DruckeFlex("## %b %b %b %b %b %E\n", b4, b5, b7, b8, b9, e); */
  IO_DruckeFlex("## %b %E\n", b4, e);
  return b4;
#endif

}


static BOOLEAN suff_satLPO(EConsT e)
{
#if !TIME_MEASURE
  BOOLEAN b1, b2, b3;
  lpo_pack = FALSE;
/*   b1 = suff_satisfiable(e); */
  lpo_pack = TRUE;
  b2 = suff_satisfiable(e);

  b3 = CO_satisfiable(e);
/*   IO_DruckeFlex("## %b %b %b %E\n", */
/*                 b1, b2, b3, e); */
/*   if (!b2 && b3){ */
/*     IO_DruckeFlex("## SHIT %b %b %b %E\n", */
/*                   b1, b2, b3, e); */
/*   } */

/*   IO_DruckeFlex("## %b %E\n", b2, e);   */
  if (!b2 && b3){
    our_error("SHIT\n");
  }
  return b2;
#else  /* TIME_MEASURE */
  BOOLEAN b1, b2, b3;
  hrtime_t t_start2,t_end2, t_start3,t_end3;
  lpo_pack = FALSE;
/*   b1 = suff_satisfiable(e); */
  lpo_pack = TRUE;

  t_start2 = gethrtime();
  b2 = suff_satisfiable(e);
  t_end2 = gethrtime();

#if 0
  t_start3 = gethrtime();
  b3 = CO_satisfiable(e);
  t_end3 = gethrtime();

  IO_DruckeFlex("## %b %b", b2, b3);
  printf(" %lld %lld", (t_end2-t_start2), (t_end3-t_start3));
  IO_DruckeFlex(" %E\n",e);
#endif

  IO_DruckeFlex("## %b ", b2);
  printf(" %lld", (t_end2-t_start2));
  IO_DruckeFlex(" %E\n",e);
  
#if 0
   if (!b2 && b3){
     our_error("SHIT\n");
   }
#endif
  return b2;
#endif
}

static void suff_statistic(void){
  /* Anzahl der Teilterme im urspruenglichen Constraint berechnen: */
  EConsT tmpCons; /* Fuer Statistik */
  UTermT tmpTerm, NullTerm;

  unsigned int anzTT = 0;
  for (tmpCons = relations.cons ; tmpCons != NULL ; tmpCons = tmpCons->Nachf){
/*     IO_DruckeFlex("tmpCons %E\n",tmpCons); */
    NullTerm = TO_NULLTerm(tmpCons->s);
    for (tmpTerm = tmpCons->s; tmpTerm != NullTerm; tmpTerm = TO_Schwanz(tmpTerm)){
/*       IO_DruckeFlex("s: TTerm %t\n",tmpTerm); */
      anzTT++;
    }
    NullTerm = TO_NULLTerm(tmpCons->t);
    for (tmpTerm = tmpCons->t; tmpTerm != NullTerm; tmpTerm = TO_Schwanz(tmpTerm)){
/*       IO_DruckeFlex("t: TTerm %t\n",tmpTerm); */
      anzTT++;
    }
  }
/*   IO_DruckeFlex("MSTAT anzTT: %d MemoTab: %d , %E\n", anzTT, memos.aktI, relations.cons); */
}

BOOLEAN Suff_satisfiable3(EConsT e)
{
#ifdef CCE_Source
  BOOLEAN ret;
  if (ORD_OrdIstKBO){
     ret = suff_satKBO(e); 

/*      useSubs = TRUE; */
/*      ret = suff_sat2(e,0); */
/*      useSubs = FALSE; */

/*    suff_statistic(); */
    return ret; 
  }
  else if (ORD_OrdIstLPO){
     ret = suff_satLPO(e); 

/*      useSubs = TRUE; */
/*      ret = suff_sat2(e,0); */
/*      useSubs = FALSE; */

/*    suff_statistic(); */
    return ret; 
  }  
#else
  return suff_satisfiable(e);
#endif
}


/* -------------------------------------------------------------------------- */
/* -------- E O F ----------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
