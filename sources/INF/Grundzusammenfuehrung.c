/*=================================================================================================================
  =                                                                                                               =
  =  Grundzusammenfuehrung.c                                                                                      =
  =                                                                                                               =
  =  Redundanztest a la Martin-Nipkow                                                                             =
  =                                                                                                               =
  =  25.11.99 Thomas.                                                                                             =
  =================================================================================================================*/

#include "Ausgaben.h"
#include "compiler-extra.h"
#include "ConfluenceTrees.h"
#include "DSBaumOperationen.h"
#include "Grundzusammenfuehrung.h"
#include "MatchOperationen.h"
#include "MissMarple.h"
#include "NFBildung.h"
#include "Ordnungen.h"
#include "SpeicherVerwaltung.h"
#include "Subsumption.h"
#include "SymbolOperationen.h"

#ifndef CCE_Source

#include "Statistiken.h"

#else
#define IncAnzRedVersMitRegeln()    /* nix tun */
#define IncAnzRedErfRegeln()        /* nix tun */
#define IncAnzRedVersMitGleichgn()  /* nix tun */
#define IncAnzRedErfGleichgn()      /* nix tun */
#endif


/* Anzahl vorzunehmender Grundzusammenfuehrungen in Abhaengigkeit von Variablenanzahl:

   |Var(s=t)| |    0 |    1 |    2 |    3 |    4 |    5 |    6
   -----------+------+------+------+------+------+------+------
   Anz. Tests |    1 |    1 |    3 |   13 |   75 |  541 | 4838

   Die dem Test unterworfenen Terme werden variablennormiert. */


/* In SymbolOperationen.h Makros nur bedingt mit Extrakonstanten */

/*=================================================================================================================*/
/*= I. Wichtigst Konstante, Varianten und Debugging-Macros                                                        =*/
/*=================================================================================================================*/

#define USE_CONFLUENCE_TREES 0
#define USE_MN90 1
#define COMPARE_BOTH (1 && USE_MN90 && USE_CONFLUENCE_TREES)
#define NO_OF_VARIANTS 0

#define USE_WITNESSES 1
#define DO_VORTEST 0
#define DO_ACK_BEVORZUGUNG 0
#define DO_BACKWARDS 1
#define DO_SUBSUMPTION 1
#define PROTECT_3_PERMS 1
#define SCHUETZEN 1
#define DRUCKEN 0

BOOLEAN drucken = FALSE;
#define TIME_MEASURE 0
#define DO_STAT 0

const unsigned int GZ_MaximaleVariablenzahl = 5 /* 6 */;
BOOLEAN GZ_doGZFB;

#if TIME_MEASURE
static unsigned int number3;

#define START_TM(x)                             \
{                                               \
  struct timeval before, after;                 \
  gettimeofday(&before,NULL);                   \
  number ## x++;

#define STOP_TM(x,r)                                                           \
  gettimeofday(&after,NULL);                                                 \
  if (before.tv_usec > after.tv_usec){                                       \
    after.tv_sec  = after.tv_sec - 1 - before.tv_sec;                        \
    after.tv_usec = after.tv_usec + 1000000 - before.tv_usec;                \
  }                                                                          \
  else {                                                                     \
    after.tv_sec  = after.tv_sec - before.tv_sec;                            \
    after.tv_usec = after.tv_usec - before.tv_usec;                          \
  }                                                                          \
  printf("#%d %10ld %s %6d\n", x, after.tv_sec*1000000+after.tv_usec, r ? "Y" : "N", number ## x); \
}

#else
#define START_TM(x) /* nix tun */
#define STOP_TM(x,r)  /* nix tun */
#endif

#if DO_STAT
static int alle, blaetter;
#define onstat(x) x
#else
#define onstat(x) /*x*/
#endif

#if GZ_TEST
const char spc[] = "                       ";
#define pr(x) (printf(spc), IO_DruckeFlex x)
#else
#define pr(x)
#endif
#if GZ_TEST==2
#define pr2(x) IO_DruckeFlex x
#else
#define pr2(x)
#endif


/* Permsubsummieren */
/********************/
static DSBaumT gz_SAVE_R;
static DSBaumT gz_SAVE_E;

static DSBaumT gz_MY_R;
static DSBaumT gz_MY_E;
static BOOLEAN gz_initialisiert = FALSE;
static BOOLEAN gz_aktiviert = FALSE;
static BOOLEAN gz_II_aktiviert = FALSE;

static void gz_switchToMY(void)
{
  gz_SAVE_R = RE_Regelbaum;
  gz_SAVE_E = RE_Gleichungsbaum;

  RE_Regelbaum = gz_MY_R;
  RE_Gleichungsbaum = gz_MY_E;
  ORD_aufZwangsGrundUmstellen();
}

static void gz_switchBack(void)
{
  ORD_vonZwangsGrundZurueckstellen();
  RE_Regelbaum = gz_SAVE_R;
  RE_Gleichungsbaum = gz_SAVE_E;
}

RegelOderGleichungsT gz_faktumKopieren(RegelOderGleichungsT quelle)
{
  RegelOderGleichungsT ziel;
  TP_GNTermpaarHolen(ziel);
  *ziel = *quelle;
  /* sicherheitshalber alles ueberschreiben, was gefaehrlich aussieht: */
  ziel->Blatt = NULL;
  ziel->Gegenrichtung = NULL; /* Achtung bei Stereogleichungen !! */
  ziel->Vorg = NULL;
  ziel->Nachf = NULL;
  /* alle vernuenftigen Terme kopieren */
  ziel->linkeSeite = TO_FrischzellenTermkopie(quelle->linkeSeite);
  ziel->rechteSeite = TO_FrischzellenTermkopie(quelle->rechteSeite);
  return ziel;
}

void GZ_ACC_vorhanden(SymbolT f)
{
  RegelOderGleichungsT faktum;
  if (!PA_PermsubAC()&&!PA_PermsubACI()){
    return;
  }
  if (!gz_initialisiert){
    BO_DSBaumInitialisieren(&gz_MY_R);   
    BO_DSBaumInitialisieren(&gz_MY_E);   
    gz_initialisiert = TRUE;
  }
  faktum = gz_faktumKopieren(MM_ARegel(f));
  BO_ObjektEinfuegen(&gz_MY_R, faktum, faktum->tiefeLinks);
  faktum = gz_faktumKopieren(MM_CGleichung(f));
  BO_ObjektEinfuegen(&gz_MY_E, faktum, faktum->tiefeLinks);
  faktum = gz_faktumKopieren(MM_C_Gleichung(f));
  BO_ObjektEinfuegen(&gz_MY_E, faktum, faktum->tiefeLinks);
  IO_DruckeFlex("### ACC aktiviert fuer %S\n", f); 
  gz_aktiviert = TRUE;
}

void GZ_ACCII_vorhanden(SymbolT f)
     /* Als invariante muss vorher GZ_ACC_vorhanden aufgerufen worden sein! */
{
  RegelOderGleichungsT faktum;
  if (!PA_PermsubACI()){
    return;
  }
  faktum = gz_faktumKopieren(MM_IRegel(f));
  BO_ObjektEinfuegen(&gz_MY_R, faktum, faktum->tiefeLinks);
  faktum = gz_faktumKopieren(MM_I_Regel(f));
  BO_ObjektEinfuegen(&gz_MY_E, faktum, faktum->tiefeLinks);
  gz_II_aktiviert = TRUE;
  IO_DruckeFlex("### ACCII aktiviert fuer %S\n", f);
}

static void gz_MY_X_abraeumen(DSBaumP my_index)
{
  while (my_index->Wurzel) {
    RegelOderGleichungsT Faktum = BK_Regeln(my_index->ErstesBlatt);
    BO_ObjektEntfernen(my_index,Faktum);
    TP_LinksRechtsFrischLoeschen(Faktum);
    TP_GNTermpaarLoeschen(Faktum);
  }
}

static void gz_ACC_loeschen(void)
{
  gz_MY_X_abraeumen(&gz_MY_R);
  gz_MY_X_abraeumen(&gz_MY_E);
  gz_aktiviert = FALSE;
  gz_II_aktiviert = FALSE;
}

void GZ_Cleanup(void)
{
  gz_ACC_loeschen();
}

void GZ_SoftCleanup(void)
{
  gz_ACC_loeschen();
}

static BOOLEAN gz_top_OK(TermT l, TermT r)
{
  return TO_TopSymbol(l) == TO_TopSymbol(r);
}

static BOOLEAN gz_len_OK( TermT l, TermT r)
{
  unsigned int len = TO_Termlaenge(l);
  return (len >=3)&&(len == TO_Termlaenge(r));
}

#define PRETESTS 0
static BOOLEAN gz_ACGleich(TermT l, TermT r)
{
  BOOLEAN res = FALSE;
  if (gz_aktiviert){
    if (!PRETESTS || /* sonst Fehler! */
        gz_top_OK(l,r) && 
        (gz_II_aktiviert||gz_len_OK(l,r))){
      TermT lhs = TO_Termkopie(l);
      TermT rhs = TO_Termkopie(r);
      gz_switchToMY();
      NF_NormalformRE(lhs);
      NF_NormalformRE(rhs);
      gz_switchBack();
      res = TO_TermeGleich(lhs,rhs);
      TO_TermeLoeschen(lhs,rhs);
    }
  }
  return res;
}

#define WANT_TIME_MEASURE 1 /* 0, 1 */
#if WANT_TIME_MEASURE
#if !SUNOS5
typedef unsigned long long int hrtime_t;
#endif

#if defined DARWIN || defined(__aarch64__) || defined(__arm__)
/* macOS / Apple Silicon / ARM: use clock_gettime */
#include <time.h>

__inline__ static hrtime_t gethrtime(void)
{
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (hrtime_t)ts.tv_sec * 1000000000ULL + (hrtime_t)ts.tv_nsec;
}
#else
/* x86: use rdtsc */
#define rdtscll(val) \
     __asm__ __volatile__ ("rdtsc" : "=A" (val))

__inline__ static hrtime_t gethrtime(void)
{
        hrtime_t x;
        rdtscll(x);
        return x;
}
#endif
#endif

static BOOLEAN gz_2_ACGleich(TermT l, TermT r)
     /* Vergleich der beiden Fassungen */
{
  BOOLEAN res1, res2;
  if (!PA_PermsubAC() && !PA_PermsubACI()){
    return TO_ACGleich(l,r);
  }
#if WANT_TIME_MEASURE
  hrtime_t t_start, t_mid, t_end;
  t_start = gethrtime();
  res1 = TO_ACGleich(l,r);
  t_mid = gethrtime();
  res2 = gz_ACGleich(l,r);
  t_end = gethrtime();
  printf("==> %lld %lld    %d %d\n", t_mid - t_start, t_end - t_mid, res1, res2);
#else
  res1 = TO_ACGleich(l,r);
  res2 = gz_ACGleich(l,r);
#endif
  switch (((res1)?1:0)+((res2)?2:0)) {
  case 0: return FALSE;
  case 3: return TRUE;
  case 1: IO_DruckeFlex("# AC-Tests differ: old: %b, new: %b, input: %t = %t\n",
                         res1, res2, l, r);
    return TRUE;
  case 2: IO_DruckeFlex("#### AC-Tests differ: old: %b, new: %b, input: %t = %t\n",
                         res1, res2, l, r);
    return TRUE;
  }
}

BOOLEAN GZ_ACVerzichtbar(TermT l, TermT r)
{
  if (gz_2_ACGleich(l,r))
    if (!SO_IstACSymbol(TO_TopSymbol(l)))
      return TRUE;
    else {
      BO_TermpaarNormierenAlsKP(l,r);
      return !TO_IstAssoziativitaet(l,r) && !TO_IstKommutativitaet(l,r) && !TO_IstErweiterteKommutativitaet(l,r);
    }
  else
    return FALSE;
}


/*=================================================================================================================*/
/*= Neue Fassung: Rekursiv mit Backtracking                                                                       =*/
/*=================================================================================================================*/

typedef struct {
  unsigned int s_schuetzen : 1;
  unsigned int t_schuetzen : 1;
  unsigned int unvergleichbar :1;
} SchutzT;

static const SchutzT allesSch = {1, 1, 1};
static const SchutzT nixSch   = {0, 0, 0};
static const SchutzT sSch     = {1, 0, 0};

static unsigned int AnzahlVariablen(TermT l, TermT r)
{
  SymbolT xm = TO_neuesteVariable(l),
          xn = TO_neuesteVariable(r);
  return SO_VarNummer(min(xm,xn));
}

static TermT ApplyShiftSubst (TermT s_, SymbolT x, SymbolT cj)
{
  TermT s = TO_Termkopie(s_);
  UTermT subterm;
  SymbolT top;
  TO_doStellen(s,subterm,{
    top = TO_TopSymbol(subterm);
    if (top == x){
      TO_SymbolBesetzen(subterm,cj);
    }
    else if (top >= cj){ /* top ist Extrakonstante groesser-gleich cj -> eins hoeher setzen */
      TO_SymbolBesetzen(subterm,top+1);
    }
  });
  return s;
}

static TermT ApplyIdentSubst (TermT s_, SymbolT x, SymbolT cj)
{
  TermT s = TO_Termkopie(s_);
  UTermT subterm;
  SymbolT top;
  TO_doStellen(s,subterm,{
    top = TO_TopSymbol(subterm);
    if (top == x){
      TO_SymbolBesetzen(subterm,cj);
    }
  });
  return s;
}

static TermT ApplyDIdentSubst (TermT s_, SymbolT x, SymbolT cj, SymbolT y, SymbolT ci)
{
  TermT s = TO_Termkopie(s_);
  UTermT subterm;
  SymbolT top;
  TO_doStellen(s,subterm,{
    top = TO_TopSymbol(subterm);
    if (top == x){
      TO_SymbolBesetzen(subterm,cj);
    }
    else if (top == y){
      TO_SymbolBesetzen(subterm,ci);
    }
  });
  return s;
}

static TermT ApplyExtraSubst (TermT s_, unsigned noVar)
{
  TermT s = TO_Termkopie(s_);
  UTermT subterm;
  SymbolT top;
  TO_doStellen(s,subterm,{
    top = TO_TopSymbol(subterm);
    if (SO_SymbIstVar(top)){
      TO_SymbolBesetzen(subterm,SO_Extrakonstante(top+noVar+1)); /* ??????????????????????????*/
    }
  });
  return s;
}


static BOOLEAN DreieckOK (RegelOderGleichungsT re, TermT t)
/* t |> l  gdw es ex sigma mit sigma l == t und es ex. kein sigma mit sigma t == l */
/* da p = lambda, d.h. toplevel-Match! */
{
  return MO_existiertMatch(re->linkeSeite,t) && !MO_existiertMatch(t, re->linkeSeite);
}

static BOOLEAN RegelOderGleichungToplevelAngewendet(TermT Term, TermT t0)
{
  RegelOderGleichungsT re;

  if (TO_TermIstVar(Term))
    return FALSE;
  MO_AllRegelMatchInit(Term);
  IncAnzRedVersMitRegeln();
  while (MO_AllRegelMatchNext (&re)){
    if (DreieckOK(re,t0)) {
      IncAnzRedErfRegeln();
      MO_SigmaRInEZ();
      return TRUE;
    }
    IncAnzRedVersMitRegeln();
  }
  MO_AllGleichungMatchInit(Term);
  IncAnzRedVersMitGleichgn();
  while (MO_AllGleichungMatchNext(TRUE, &re)) {
    if (DreieckOK(re,t0)) {
      IncAnzRedErfGleichgn();
      MO_SigmaRInEZ();
      return TRUE;
    }
    IncAnzRedVersMitGleichgn();
  }

  return FALSE;
}

static BOOLEAN RegelOderGleichungInnenAngewendet(UTermT Term, UTermT Vorg)
{
  if (TO_TermIstVar(Term))
    return FALSE;
  IncAnzRedVersMitRegeln();
  if (MO_RegelGefunden(Term)) {
    IncAnzRedErfRegeln();
    TO_NachfBesetzen(Vorg, MO_SigmaRInLZ());
    return TRUE;
  }
  IncAnzRedVersMitGleichgn();
  if (MO_GleichungGefunden(Term)) {
    IncAnzRedErfGleichgn();
    TO_NachfBesetzen(Vorg, MO_SigmaRInLZ());
    return TRUE;
  }
  return FALSE;
}

static BOOLEAN geschuetzterSchrittErfolgreich(TermT t, TermT t0)
{
  /* es gilt: t = @t0 fuer eine extrakonsteinfuehrende Subst @                    */
  /* wenn t -> t' mit l = r, dann nur erlaubt, falls p != lambda oder t0 |> l     */
  /* wenn p != lambda einfach. Also sei p = lambda. Dann ist t = @'l.             */
  /* Also darf kein @'' existieren, mit l = @''t0.                                */
  /* [Problem: Wie sind Faelle zu handhaben, wenn @ versch Vars identifiziert???] */
  /* sigh: todo, Achtung: werden Seiteneffekte an t weitergegeben?                */ 

  UTermT subTerm, subVorg, NULLTerm;

  if (/*FALSE &&*/ RegelOderGleichungToplevelAngewendet(t,t0))
    return TRUE;
      
  subVorg  = t;
  subTerm  = TO_Schwanz(subVorg);
  NULLTerm = TO_NULLTerm(t);
      
  while (subTerm != NULLTerm){
    if (RegelOderGleichungInnenAngewendet(subTerm, subVorg))
      return TRUE;
    subVorg = subTerm;
    subTerm = TO_Schwanz(subVorg);
  }
  return FALSE;
}

static BOOLEAN geschuetzteNormalform(TermT t, TermT t0, BOOLEAN schuetzen)
{
#if 0
  if (SCHUETZEN && schuetzen && !geschuetzterSchrittErfolgreich(t,t0)){
    return TRUE;
  }
  NF_NormalformRE(t);
  return FALSE;
#endif
  if (SCHUETZEN && schuetzen){
    return !NF_geschuetzteNormalformRE(t,t0);
  }
  else {
    NF_NormalformRE(t);
    return FALSE;
  }
}

#if DO_ACK_BEVORZUGUNG
typedef BOOLEAN *Relation;

static Relation neueRelation (unsigned int n)
{
  return array_alloc(n*n,BOOLEAN);
}

static void RelationFreigeben (Relation R)
{
  array_dealloc(R);
}

/* Zum Berechnen der transitiven Huelle. */ 
static void RoyWarshall (Relation R, unsigned int n)
{
  unsigned int i,j,k;
  for (i=0; i<n; i++)
    for (j=0; j<n; j++)
      if (R[j*n+i])
	for (k=0; k<n; k++)
	  R[j*n+k] = R[j*n+k] || R[i*n+k];
}

static void RelationInit (Relation R, unsigned int n)
{
  unsigned int i,j;
  for (i=0; i<n; i++)
    for (j=0; j<n; j++)
      R[i*n+j] = FALSE;
}

static unsigned int Diagonaleinsen (Relation R, unsigned int n)
{
  unsigned int i,r=0;
  for (i=0; i<n; i++){
    /*    printf("%d ", R[i*n+i]);*/
    if (R[i*n+i])
      r++;
  }
  return r;
}
static void RelationSetzen(Relation linksvon, unsigned int n, UTermT t);

#define foreachACTeilterm(/*UTermT*/ t, /*SymbolT*/ f, /*UTermT*/ t1, /*BOOLEAN*/ finished, Block)	\
{													\
   UTermT  t__ = t;											\
   SymbolT f__ = f;											\
   finished = FALSE;											\
   do {													\
     if (TO_TopSymbol(t__) == f__){									\
       t1  = TO_ErsterTeilterm(t__);									\
       t__ = TO_ZweiterTeilterm(t__);									\
     }													\
     else {												\
       t1 = t__;											\
       finished = TRUE;											\
     }													\
     Block;												\
   } while (!finished);											\
}


static void ACTeiltermBehandlen(Relation linksvon, unsigned int n, SymbolT f, UTermT t)
{
  SymbolT x = f;
  UTermT t1;
  BOOLEAN lastTerm;
  foreachACTeilterm(t,f,t1,lastTerm,{
    SymbolT y = TO_TopSymbol(t1);
    if (SO_SymbIstVar(y)){
      if (SO_SymbUngleich(x,f) && SO_SymbUngleich(x,y))
	linksvon[(SO_VarNummer(x)-1)*n+(SO_VarNummer(y)-1)] = TRUE;
      x = y;
    }
    else{
      RelationSetzen(linksvon,n,t1);
      if (!lastTerm && TO_TermIstNichtGrund(t1)){
	UTermT t11;
	BOOLEAN fini;
	foreachACTeilterm(TO_NaechsterTeilterm(t1),f,t11,fini,{
	  SymbolT y = TO_TopSymbol(t11);
	  if (SO_SymbIstVar(y)){
	    SymbolT x;
	    TO_doSymboleOT(t1,x,{
	      if (SO_SymbIstVar(x))
		linksvon[(SO_VarNummer(x)-1)*n+(SO_VarNummer(y)-1)] = TRUE;
	    })
	  }
	})
      }
    }
  })
}


static void RelationSetzen(Relation linksvon, unsigned int n, UTermT t)
{
  UTermT NULLt = TO_NULLTerm(t);
  do { 
    SymbolT x = TO_TopSymbol(t);
    if (SO_IstACSymbol(x)){
      ACTeiltermBehandlen(linksvon, n, x, t);
      t = TO_TermEnde(t);
    }
  }
  while (TO_PhysischUngleich(t = TO_Schwanz(t),NULLt));
}

#endif

static SymbolT guteVariable (TermT s, TermT t) /* Zur Zeit dumm: die erste gefundene Variable */
{
  UTermT subterm;
  SymbolT top;
  TO_doStellen(s,subterm,{
    top = TO_TopSymbol(subterm);
    if (SO_SymbIstVar(top)){
      return top;
    }
  });  
  TO_doStellen(t,subterm,{
    top = TO_TopSymbol(subterm);
    if (SO_SymbIstVar(top)){
      return top;
    }
  });  
  our_error("Problem in guteVariable: keine Variable");
  return SO_ErstesVarSymb; /*  Shut up Compiler Warning */
}

#if 0
#define bla(x) IO_DruckeFlex x
#define blu(x) IO_DruckeFlex x
#else
#define bla(x) /* nix */
#define blu(x) /* nix */
#endif

static unsigned klammern = 0;
static UTermT links;
static UTermT rechts;
static SeitenT seite;

void druckeSchutzverletzung(RegelOderGleichungsT Objekt)
{
  if (drucken){
    klammern++;
    IO_DruckeFlex("{\\Guard}{%%\n\\Node{\\XParent{%s_{%d}}}{%t = %t}", 
		  TP_TermpaarIstRegel(Objekt) ? "R" : "E",
		  TP_TermpaarIstRegel(Objekt) ? TP_ElternNr(Objekt) : -TP_ElternNr(Objekt), 
		  links, rechts);
  }
}   

void ProtokolliereSchritt(RegelOderGleichungsT Objekt, UTermT Stelle)
{
  if (drucken){
    klammern++;
    IO_DruckeFlex("{\\Rewr}{%%\n\\Node{\\Parent{%s_{%d}}\\Pos{%p}}{%t = %t}", 
		  TP_TermpaarIstRegel(Objekt) ? "R" : "E",
		  TP_TermpaarIstRegel(Objekt) ? TP_ElternNr(Objekt) : -TP_ElternNr(Objekt), 
		  seite, (seite == linkeSeite) ? links : rechts, Stelle, links, rechts);
  }
}

void KlammernDrucken(unsigned i)
{
  while (i > 0){
    IO_DruckeFlex("}%%\n");
    i--;
  }
}

static BOOLEAN gzfb (TermT s, TermT t, int n, RegelOderGleichungsT opfer, SchutzT sch)
/* Destruktiv auf s und t: Beide werden veraendert und danach geloescht! */
{
  SymbolT x;
  int j;
  unsigned toclose;
  onstat(alle++;);
  bla(("%d> %t =?= %t\n", n, s, t));
  if (0 && drucken) IO_DruckeFlex("GZFB: %t = %t n = %d, sch = %d%d%d\n",
			     s,t,n,sch.unvergleichbar, sch.s_schuetzen, sch.t_schuetzen);
  if (drucken) IO_DruckeFlex("{%t = %t}", s,t);
  if (sch.unvergleichbar){
    switch (ORD_VergleicheTerme(s,t)){
    case Groesser:        sch.unvergleichbar = FALSE; sch.t_schuetzen = FALSE; break;
    case Kleiner:         sch.unvergleichbar = FALSE; sch.s_schuetzen = FALSE; break;
    case Gleich:          printf("traritrara\n"); TO_TermeLoeschen(s,t); return TRUE; /* Falsch fuer Quasi-Ordnungen ! */
    case Unvergleichbar:  break;
    default:              break;
    }
  }
  links = s;
  rechts = t;
  klammern = 0;
  seite = linkeSeite;
  sch.s_schuetzen = geschuetzteNormalform(s,opfer->linkeSeite,sch.s_schuetzen);
  seite = rechteSeite;
  sch.t_schuetzen = geschuetzteNormalform(t,opfer->rechteSeite,sch.t_schuetzen);
  toclose = klammern;
  if (0 && drucken) IO_DruckeFlex("gzfb: %t = %t  n = %d, sch = %d%d%d\n",
			   s,t,n,sch.unvergleichbar, sch.s_schuetzen, sch.t_schuetzen);
  if (TO_TermeGleich(s,t)){
    bla(("%d> %t =?= %t ==> TRUE\n", n, s, t));
    TO_TermeLoeschen(s,t);
    if(drucken){
      IO_DruckeFlex("{\\Joined}{}%%\n");
      KlammernDrucken(toclose);
    }
    onstat(blaetter++;)
    return TRUE;
  }
  if (TO_TermIstGrund(s) && TO_TermIstGrund(t)){
    bla(("%d> %t =?= %t ==> FALSE\n", n, s, t));
    if (USE_WITNESSES){
      opfer->GescheitertL = s;
      opfer->GescheitertR = t; /* Zeugen merken */
    }
    else {
      opfer->GescheitertL = NULL;
      opfer->GescheitertR = NULL; /* keine Zeugen merken */
    }
    if(drucken){
      IO_DruckeFlex("{\\Failed}{}%%\n");
      KlammernDrucken(toclose);
    }
    onstat(blaetter++;)
    return FALSE;
  }
  if (DO_SUBSUMPTION && SS_TermpaarSubsummiertVonGM (s,t)){
    bla(("%d> %t =?= %t =S=> TRUE\n", n, s, t));
    TO_TermeLoeschen(s,t);
    if (0) IO_DruckeFlex("###SUB\n");
    if(DRUCKEN && drucken){
      IO_DruckeFlex("{\\Subsumed}{}%%\n");
      KlammernDrucken(toclose);
    }
    onstat(blaetter++;)
    return TRUE;
  }
  x = guteVariable(s,t);
  if(drucken){
    IO_DruckeFlex("{\\Inst}{%%\n");
  }
  for (j = 1; j <= n+1; j++){
    if (drucken){
      int k;
      IO_DruckeFlex("\\Node{\\Subst{%S <- %S",x,SO_Extrakonstante(j));
      for (k = j; k <= n; k++){
	IO_DruckeFlex("\\\\ %S <- %S",SO_Extrakonstante(k),SO_Extrakonstante(k+1));
      }
      IO_DruckeFlex("}}");
    }
    if (!gzfb(ApplyShiftSubst(s,x,SO_Extrakonstante(j)),ApplyShiftSubst(t,x,SO_Extrakonstante(j)),n+1, opfer, sch)){
      bla(("%d> %t =?= %t ==> FALSE (%d)\n", n, s, t,j));
      TO_TermeLoeschen(s,t);
  if(drucken){
    KlammernDrucken(toclose+1);
  }  
      return FALSE;
    }
  }
  for (j = 1; j <= n; j++){
    if (drucken){
      IO_DruckeFlex("\\Node{\\Subst{%S <- %S}}", x, SO_Extrakonstante(j));
    }
    if (!gzfb(ApplyIdentSubst(s,x,SO_Extrakonstante(j)),ApplyIdentSubst(t,x,SO_Extrakonstante(j)),n, opfer, sch)){
      bla(("%d> %t =?= %t ==> FALSE [%d]\n", n, s, t,j));
      TO_TermeLoeschen(s,t);
  if(drucken){
    KlammernDrucken(toclose+1);
  }  
      return FALSE;
    }
  }
  bla(("%d> %t =?= %t ==> TRUE!!\n", n, s, t));
  if(drucken){
    KlammernDrucken(toclose+1);
  }  
  TO_TermeLoeschen(s,t);
  return TRUE;
}

static BOOLEAN vortest (TermT s, TermT t, RegelOderGleichungsT opfer)
{
  NF_NormalformRE(s); /* hier muss Dreieckstest nicht unbedingt beruecksichtigt werden! */
  NF_NormalformRE(t);
  if (TO_TermeGleich(s,t)){
    TO_TermeLoeschen(s,t);
    return TRUE;
  }
  else {
    if (USE_WITNESSES){
      opfer->GescheitertL = s;
      opfer->GescheitertR = t; /* Zeugen merken */
    }
    else {
      opfer->GescheitertL = NULL;
      opfer->GescheitertR = NULL; /* keine Zeugen merken */
    }
    return FALSE;
  }
}

static BOOLEAN wrap (RegelOderGleichungsT opfer, unsigned int n, SchutzT sch)
/* Wir gehen davon aus, dass n >= 2 und die Terme Var-normiert */
{
  TermT s = opfer->linkeSeite;
  TermT t = opfer->rechteSeite;

  if (DO_VORTEST && !vortest(ApplyExtraSubst(s,n),ApplyExtraSubst(t,n),opfer)){
    return FALSE;
  }

#if DO_ACK_BEVORZUGUNG
  {
    unsigned int x = 0, y = 0;
    Relation R;
    BOOLEAN res;
    int einsen;
    R = neueRelation(n);
    RelationSetzen(R,n,s);
    RelationSetzen(R,n,t);
    RoyWarshall(R,n);
    /*    printf("EINSEN: ");*/
    einsen = Diagonaleinsen(R,n);
    /*    printf("\n");*/
    if (einsen >= 2) {
      while (!R[x*n+x]) x++;
      y = x+1;
      while (!R[y*n+y]) y++;
      x++; y++; /* Indizes -> Varnummern */
#if 0
      IO_DruckeFlex("%t =g= %t\n", s, t);
      printf("x = %d, y = %d, n = %d\n", x, y, n);
#endif
      x = -x; 
      y = -y; /* Varnummern -> Symbole */
      /* x und y sind erstes vertauschtes Paar */
#if 1
    }
    else {
      x = SO_ErstesVarSymb;
      y = SO_NaechstesVarSymb(x);
    }
#endif
      RelationFreigeben(R);
      res =
        gzfb(ApplyDIdentSubst(s, x, SO_Extrakonstante(1), y, SO_Extrakonstante(2)), 
             ApplyDIdentSubst(t, x, SO_Extrakonstante(1), y, SO_Extrakonstante(2)), 2, opfer, sch);
      res = res &&
        gzfb(ApplyDIdentSubst(s, x, SO_Extrakonstante(2), y, SO_Extrakonstante(1)), 
             ApplyDIdentSubst(t, x, SO_Extrakonstante(2), y, SO_Extrakonstante(1)), 2, opfer, sch);
      res = res &&
        gzfb(ApplyDIdentSubst(s, x, SO_Extrakonstante(1), y, SO_Extrakonstante(1)), 
             ApplyDIdentSubst(t, x, SO_Extrakonstante(1), y, SO_Extrakonstante(1)), 1, opfer, sch);
      return res;
#if 0
    }
    else {
      RelationFreigeben(R);
      return gzfb(TO_Termkopie(s),TO_Termkopie(t),0, opfer, sch);
    }
#endif
  }
#else
#if 1
  return gzfb(TO_Termkopie(s),TO_Termkopie(t),0, opfer, sch);
#else
{
  BOOLEAN res;
  SymbolT    x = SO_ErstesVarSymb,
             y = SO_NaechstesVarSymb(x);
  res =
        gzfb(ApplyDIdentSubst(s, x, SO_Extrakonstante(1), y, SO_Extrakonstante(2)), 
             ApplyDIdentSubst(t, x, SO_Extrakonstante(1), y, SO_Extrakonstante(2)), 2, opfer, sch);
  res = res &&
        gzfb(ApplyDIdentSubst(s, x, SO_Extrakonstante(2), y, SO_Extrakonstante(1)), 
             ApplyDIdentSubst(t, x, SO_Extrakonstante(2), y, SO_Extrakonstante(1)), 2, opfer, sch);
  res = res &&
        gzfb(ApplyDIdentSubst(s, x, SO_Extrakonstante(1), y, SO_Extrakonstante(1)), 
             ApplyDIdentSubst(t, x, SO_Extrakonstante(1), y, SO_Extrakonstante(1)), 1, opfer, sch);
  return res;
}
#endif
#endif
}

/*=================================================================================================================*/
/*= III. Schnittstelle nach aussen:                                                                               =*/
/*=================================================================================================================*/

static BOOLEAN MN90Check(RegelOderGleichungsT opfer, SchutzT sch)
{
  BOOLEAN res;
  START_TM(3);
  {
    unsigned int noVar = AnzahlVariablen(opfer->linkeSeite,opfer->rechteSeite); 
    if (noVar <= 1){ /* Stimmt das so ?? */
      pr(("000  zu wenige Variablen\n"));
      opfer->gzfbStatus = GZ_aussichtslos;
      res = FALSE;
    }
    else if (noVar > GZ_MaximaleVariablenzahl){
      pr(("000  zu viele Variablen\n"));
      opfer->gzfbStatus = GZ_zuTeuer;
      res = FALSE;
    }
    else {
      onstat(blaetter = 0; alle = 0;) ;
      res = wrap(opfer,noVar,sch);
      if (res){
        pr(("!!!  konvergiert\n"));
        opfer->gzfbStatus = GZ_zusammenfuehrbar;
	onstat(printf("!!VY %d %d %d\n", noVar, blaetter, alle););
      }
      else {
        pr(("     divergiert\n"));
        /*        IO_DruckeFlex("Zeugen: %t und %t\n", *l0, *r0);*/
        opfer->gzfbStatus = GZ_gescheitert;
	onstat(printf("!!VN %d %d %d\n", noVar, blaetter, alle););
      }
    }
  }
  STOP_TM(3,res);
  return res;
}

static BOOLEAN GrundzusammenfuehrbarCheck(RegelOderGleichungsT opfer, 
					  BOOLEAN vorw MAYBE_UNUSEDPARAM, SchutzT sch)
{
  BOOLEAN res = FALSE;
  BOOLEAN res2 = FALSE;

  if (TO_TermIstGrund(opfer->linkeSeite) && TO_TermIstGrund(opfer->rechteSeite)){
    pr(("     beides Grundterme!\n"));
    opfer->gzfbStatus = GZ_aussichtslos;
    return FALSE;
  }

  res = MN90Check(opfer, sch);
  if (0 && (vorw||DRUCKEN && res)){
    /*    IO_DruckeFlex("MN90 %d: %t = %t\n", TP_ElternNr(opfer), TP_LinkeSeite(opfer), TP_RechteSeite(opfer));*/
    /* IO_DruckeFlex("\\MN{%s_{%d}}{%%\n\\Node{}", 
		  TP_TermpaarIstRegel(opfer) ? "R" : "E",
		  TP_TermpaarIstRegel(opfer) ? TP_ElternNr(opfer) : -TP_ElternNr(opfer)); */
    drucken = FALSE/*TRUE*/;
    res = MN90Check(opfer, sch);
    drucken = FALSE;
    /*IO_DruckeFlex("}%%\n");*/
  }
  /* hier koennen irgendwannmal noch andere Tests angeflanscht werden. */
#if USE_CONFLUENCE_TREES == 1
  {
    hrtime_t start, stop;
    start = gethrtime();
    res2 = CO_GroundJoinable(PA_CTreeVersion(),opfer->linkeSeite,opfer->rechteSeite);
    stop = gethrtime();
    IO_DruckeFlex("#%%%b %b ", res, res2);
    printf("%lld\n", stop - start);
  }
#endif
  return res;
}

BOOLEAN GZ_Grundzusammenfuehrbar(TermT l, TermT r)
{
  unsigned int k = 0;
  struct GNTermpaarTS dummy;
  RegelOderGleichungsT opfer = &dummy;
  BO_TermpaarNormierenAlsKP(l,r);

  opfer->linkeSeite = l;
  opfer->rechteSeite = r;
  opfer->GescheitertL = NULL;
  opfer->GescheitertR = NULL;
  BOOLEAN res = GZ_GrundzusammenfuehrbarVorwaerts(opfer); 
  if(opfer->GescheitertL != NULL){
    TO_TermeLoeschen(opfer->GescheitertL,opfer->GescheitertR);
  }
  return res;
}

BOOLEAN GZ_GrundzusammenfuehrbarVorwaerts(RegelOderGleichungsT opfer)
{
  if(!GZ_GRUNDZUSAMMENFUEHRUNG || !GZ_doGZFB())
    return FALSE;
#ifdef CCE_Protocol
  IO_CCElog(CCE_Test, opfer->linkeSeite, opfer->rechteSeite);
#endif
  pr(("???  %t =g= %t\n",opfer->linkeSeite, opfer->rechteSeite));
  if ( PROTECT_3_PERMS && 
       (TO_IstErweiterteKommutativitaet(opfer->linkeSeite, opfer->rechteSeite) 
        || TO_IstAssoziativitaet(opfer->linkeSeite, opfer->rechteSeite) 
        || TO_IstKommutativitaet(opfer->linkeSeite, opfer->rechteSeite))) {
    pr(("     zur permutativen Saeuberung doch betrachten\n"));
    opfer->gzfbStatus = GZ_wertvoll;
    return FALSE;
  }
  return GrundzusammenfuehrbarCheck(opfer, TRUE, nixSch);
}

BOOLEAN GZ_GrundzusammenfuehrbarRueckwaerts(RegelOderGleichungsT opfer, RegelOderGleichungsT NeuesFaktum)
{
  BOOLEAN res;
  if(!GZ_GRUNDZUSAMMENFUEHRUNG || !GZ_doGZFB() || !DO_BACKWARDS)
    return FALSE;

  pr(("???R %t =g= %t\n",opfer->linkeSeite, opfer->rechteSeite));
  if (opfer->gzfbStatus <= GZ_aussichtslos){
    pr(("     sinnlos\n"));
    return FALSE;
  }
  if (USE_WITNESSES && (opfer->gzfbStatus == GZ_gescheitert) 
      && (opfer->GescheitertL != NULL) && (opfer->GescheitertR != NULL)) {
    if (!MO_ObjektPasst(NeuesFaktum,opfer->GescheitertL) && !MO_ObjektPasst(NeuesFaktum,opfer->GescheitertR)){
      pr(("     Zeugen irreduzibel\n"));
      return FALSE;
    }
    else{
      pr(("     Zeugen reduzibel\n"));
      TO_TermeLoeschen(opfer->GescheitertL,opfer->GescheitertR);
      opfer->GescheitertL = NULL;
      opfer->GescheitertR = NULL;
    }
  }
  opfer->DarfNichtReduzieren = TRUE;
  res = GrundzusammenfuehrbarCheck(opfer, FALSE, TP_TermpaarIstRegel(opfer) ? sSch : allesSch);
  opfer->DarfNichtReduzieren = FALSE;
  return res;
}


