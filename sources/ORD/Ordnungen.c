/**********************************************************************
* Filename : Ordnungen.c
*
* Kurzbeschreibung : Ordnungen
*
* Autoren : Andreas Jaeger, Arnim Buch
*
* 14.12.96: Erstellung
* 25.02.97: Vergleichs- und Groessertest-Funktionen aus Hauptkomponenten
*           hierher verlagert. Arnim.
*
*
* Benoetigt folgende Module:
*
* Benutzte Globale Variablen aus anderen Modulen:
*
**********************************************************************/

#include "Ausgaben.h"
#include "compiler-extra.h"
#include <ctype.h>
#include "general.h"
#include "FehlerBehandlung.h"
#include "KBO.h"
#include "LPO.h"
#include "LPOVortests.h"
#include "Ordnungen.h"
#include "Praezedenz.h"
#include <string.h>
#include "SymbolGewichte.h"
#include "SymbolOperationen.h"
#include "ZwangsGrund.h"
#include "simpleLPO.h"

#ifndef CCE_Source
#include "parser.h"
#include "InfoStrings.h"
#include "Parameter.h"
#include "PhilMarlow.h"
#include "Praezedenzgenerator.h"
#include "Statistiken.h"
#include "Constraints.h"
#else
#include "parse-util.h"
#endif


/* Achtung: welcheLPO wurde eliminiert! 
   Dies kann jetzt mit dem Oracle emuliert werden.
   Es gibt jetzt also folgende Wahlmoeglichkeiten:

   * ORACLE = 1: Mit -ord oracle aktivieren.
     write=filename  schreibe die Ergebnisse der Ordnungsaufrufe in Datei
     check=filename  vergleiche die Ergebnisse der Ordnungsaufrufen mit denen der Datei
     read=filename   nimm die Werte aus der Datei, keine Ordnungsaufrufe

   * Ordnungsversion: lpoversion=<n>/kboversion=<n>
     n = 0 (default): urspruengliche WM-Implementierung
     n = 1, ....:     neue Test-Implementierungen

   * ANALYZE = <n>: Funktioniert nur, wenn Oracle ausgeschaltet ist
     n = 0: keine zusaetzlichen Analysen
     n = 1: Es wird gezaehlt (so die verwendete Ordnung das unterstuetzt)
            Die Werte werden in die Datei Aufgabe.pr.c geschrieben
     n = 2: Es wird hochgenau die Zeit der Aufrufe bestimmt (so die Platform das zulaesst)
            Es werden AnzahlMessungen nacheinander gemacht.
            Achtung: Fuer realistische Werte sollte man mehrere Messungen nacheinander machen.
            Die Werte werden in die Datei Aufgabe.pr.t.i geschrieben, i ist die kleinste
            Zahl groesser 0, fuer die es noch nicht so eine Datei gibt.
     n = 3: Wie bei n = 2, nur werden noch beide Performance-Counter ausgelesen.
            Die Werte werden in die Dateien Aufgabe.pr.t.i, Aufgabe.pr.p.i (fuer Counter 1)
            bzw. Aufgabe.pr.q.i (fuer Counter 2) geschrieben (i wie unter n = 2).

*/

#define ORACLE 0 /* 0 */
#define ANALYZE 0 /* 0 */
#define VERBOSE_COUNTING 0 /* 0 */

typedef enum {oracle_write, oracle_read, oracle_check, oracle_none} oracleT;
static oracleT oracle_mode = oracle_none;
static char* oracleTNames [] = {
  "oracle_write", "oracle_read", "oracle_check", "oracle_none"
};
#if ORACLE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

static FILE *oracle_out;
static char *oracle;
static BOOLEAN oracle_initialized = FALSE;
#endif

#if (ANALYZE > 0) && (ORACLE)
#error "ANALYZE and ORACLE are mutual exclusive"
#endif

#if (ANALYZE > 1) && (S_doCounting)
#error "ANALYZE and S_doCounting are only sensible for ANALYZE=1"
#endif

#if (ANALYZE > 1) && (S_doTRACE)
#error "ANALYZE and S_doTRACE are only sensible for ANALYZE=1"
#endif

#ifdef SUNOS5

#if ANALYZE == 3
#error performance counter not supported under Solaris
#endif

#elif defined LINUX || defined DARWIN

#if (ANALYZE == 2) || (ANALYZE == 3)
#define TIME_MEASURE WANT_TIME_MEASURE

typedef unsigned long long int hrtime_t;

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
/* x86: Abhilfe: rdtsc als Ersatz fuer gethrtime */

/*#include <asm/msr.h>*/

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

#if (ANALYZE == 3) && !defined(DARWIN) && !defined(__aarch64__) && !defined(__arm__)
/* read perfctr implementieren... (x86 only) */

typedef unsigned long long int pcounter_t;

#define rdpmcl(ctr,low) \
        __asm__ __volatile__("rdpmc" : "=a"(low) : "c"(ctr) : "edx")

#define rdpmcll(ctr,val) \
        __asm__ __volatile__("rdpmc" : "=A"(val) : "c"(ctr))

__inline__ static pcounter_t getpcounter(unsigned int i)
{
        pcounter_t x;
        rdpmcll(i,x);
        return x;
}

#elif (ANALYZE == 3) && (defined(DARWIN) || defined(__aarch64__) || defined(__arm__))
/* Performance counters not available on ARM/Apple Silicon in this form */
#warning "ANALYZE == 3 (performance counters) not fully supported on ARM/Apple Silicon"
typedef unsigned long long int pcounter_t;
__inline__ static pcounter_t getpcounter(unsigned int i) { (void)i; return 0; }
#endif

#else /* !SUNOS5 && !LINUX && !DARWIN */

#if (ANALYZE == 2) || (ANALYZE == 3)
#error "ANALYZE > 1 not supported for this platform"
/* Abhilfe: finde Ersatz fuer gethrtime etc.*/
#endif
#endif

#if (ANALYZE == 2) || (ANALYZE == 3)
#define AnzahlMessungen 1 
#if !AnzahlMessungen
#error "AnzahlMessungen must be greater 0!"
#endif
#endif

CounterT S_counter; /* fuer's Zaehlen rekursiver Aufrufe/Symbolops */

#if ANALYZE == 1
static FILE *c_Strom;
#elif (ANALYZE == 2) || (ANALYZE == 3)
static FILE *t_Strom;
#if ANALYZE == 3
static FILE *p_Strom;
static FILE *q_Strom;
#endif
#endif

BOOLEAN ordtracing = FALSE;
static int lpoversion = 0;
static int kboversion = 0;


BOOLEAN ORD_OrdnungTotal = FALSE;
BOOLEAN ORD_OrdIstLPO = FALSE;
BOOLEAN ORD_OrdIstKBO = FALSE;

/* -------------------------------------------------------------------------------*/
/* --------------- Term-Vergleich (ORD-1) ----------------------------------------*/
/* -------------------------------------------------------------------------------*/

typedef VergleichsT (*ORDVergleichsT)  (UTermT, UTermT);
typedef BOOLEAN     (*ORDTestT)  (UTermT, UTermT);

static ORDVergleichsT ORD_Vergleichsfkt;
static ORDTestT       ORD_RedTestfkt;



/* -------------------------------------------------------------------------------*/
/* --------------- Spezialordnungen ----------------------------------------------*/
/* -------------------------------------------------------------------------------*/
#ifndef CCE_Source

#define /*BOOLEAN*/ Vergleich2Test(s,t, eq, gt) \
  (eq(s,t) ? Gleich : gt(s,t) ? Groesser : gt(t,s) ? Kleiner : Unvergleichbar)
  /* zu selten / speziell fuer Plazierung in general.h */

static BOOLEAN LeererTest(UTermT s MAYBE_UNUSEDPARAM, UTermT t MAYBE_UNUSEDPARAM)
{ return FALSE; }

static VergleichsT LeererVergleich(UTermT s, UTermT t)
{ return TO_TermeGleich(s,t) ? Gleich : Unvergleichbar; }

/* Fassung fuer allquantifizierte Ziele.. */
static BOOLEAN Col3Test(UTermT s, UTermT t)
{ 
  if (TO_IstKombinatorTupel(s,t))
    return TRUE;
  else {
    SymbolT c = TO_TopSymbol(s);
    if (SO_SymbIstKonst(c))
      if ( (  !PM_Existenzziele()
	    || (SO_SymbUngleich(c,SO_TRUE) && SO_SymbUngleich(c,SO_FALSE)))
           && TO_TermIstGrund(t))
	return TRUE;
    if (TO_KombinatorapparatAktiviert && SO_SymbIstFkt(c) && !SO_SymbIstKonst(c) && SO_SymbUngleich(c,TO_ApplyOperator) && SO_SymbGleich(TO_ApplyOperator,TO_TopSymbol(t)))
      return TRUE;
    return FALSE;
  }
}

static VergleichsT Col3Vergleich(UTermT s, UTermT t)
{ return Vergleich2Test(s,t, TO_TermeGleich, Col3Test); }


static BOOLEAN Col4Test(UTermT s_, UTermT t_)
{ 
  if (PM_Existenzziele()) {
    SymbolT s = TO_TopSymbol(s_),
            t = TO_TopSymbol(t_);
    return 
      (s==SO_EQ && (t==SO_TRUE || t==SO_FALSE)) ||
      (s==SO_TRUE && t==SO_FALSE);
  }
  else
    return FALSE;
}

static VergleichsT Col4Vergleich(UTermT s, UTermT t)
{ return Vergleich2Test(s,t, TO_TermeGleich, Col4Test); }
#endif

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

#define DO_OLD 1
#define DO_NEW4 3
#define DO_BOTH4 5
#define WHICH_TO_DO DO_NEW4


static BOOLEAN ConsTest(UTermT s, UTermT t)
{ 
#if WHICH_TO_DO == DO_OLD
  return LP_LPOGroesser(s,t);
#elif WHICH_TO_DO == DO_NEW4
  return CO_LPOCMPGTEQ(s,t) == Groesser; 
#elif WHICH_TO_DO == DO_BOTH4
  BOOLEAN b1, b2;
  b1 = CO_LPOCMPGTEQ(s,t) == Groesser; /* cLPOCMPG */
  b2 = LP_LPOGroesser(s,t);
  if (b1 != b2) {
    IO_DruckeFlex("%t ?> %t\n", s, t);
    our_error("LPOs differ!(2)");
  }
  return b1;
#else
#error "select new,  old, both "
#endif
}
 
static BOOLEAN ConsGleich(UTermT s, UTermT t)
{ 
  return CO_cEQ(NULL,s,t);
}
 
static VergleichsT ConsVergleich(UTermT s, UTermT t)
{ 
#if WHICH_TO_DO == DO_OLD
  return LP_LPO(s,t);
#elif WHICH_TO_DO == DO_NEW4
  return CO_LPOCMP(s,t);
#elif WHICH_TO_DO == DO_BOTH4
  VergleichsT r1,r2;
  r1 = CO_LPOCMP(s,t);
  r2 = LP_LPO(s,t);
  if (r1 != r2) {
    IO_DruckeFlex("%t ?> %t\n", s, t);
    our_error("LPOs differ!(4)");
  }
  return r1;
#else
#error "select new,  old, both "
#endif
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
static unsigned counter;
VergleichsT ORD_VergleicheTerme(UTermT term1, UTermT term2)
/* Vergleicht die beiden angegebenen Terme nach der eingestellten Ordnung. */
{
#if ORACLE
  if (oracle_mode != oracle_none){
    VergleichsT res1 = Unvergleichbar, res2 = Unvergleichbar;
    if ((oracle_mode == oracle_read) || (oracle_mode == oracle_check)){
      switch (*oracle){
      case 'S': res1 = Kleiner;        break;
      case 'G': res1 = Groesser;       break;
      case 'E': res1 = Gleich;         break;
      case 'N': res1 = Unvergleichbar; break;
      default: our_error("ORD_VergleicheTerme: Wrong input in reading oracle!\n");
      }
      oracle++;
    }
    if ((oracle_mode == oracle_write) || (oracle_mode == oracle_check)){
      res2 = ORD_Vergleichsfkt(term1, term2);
    }
    if ((oracle_mode == oracle_check) && (res1 != res2)){
#if !S_doTRACE
      our_error("### Ordering result and oracle differ!\n");
#else
      IO_DruckeFlex("### ORD %t # %t   results differ: oracle: %v ordering: %v\n",
                    s, t, res1, res2);
      ordtracing = TRUE;
      res2 = ORD_Vergleichsfkt(s,t);
      ordtracing = FALSE;
#endif /* !S_doTRACE */
    }
    if (oracle_mode == oracle_write){
      switch (res2){
      case Kleiner:        putc('S', oracle_out); break;
      case Groesser:       putc('G', oracle_out); break;
      case Gleich:         putc('E', oracle_out); break;
      case Unvergleichbar: putc('N', oracle_out); break;
      default: our_error("ORD_VergleicheTerme: Wrong result from ORD_Vergleichsfkt!\n");
      }
      return res2;
    }
    return res1;
  }
  else {
    return ORD_Vergleichsfkt(term1, term2);
  }
#else /* !ORACLE */
#if ANALYZE == 1
  VergleichsT res;
  S_counter = 0;
  res = ORD_Vergleichsfkt(term1, term2);
#if VERBOSE_COUNTING
  IO_DruckeFlexStrom(c_Strom, "%l %l %u %v %t %t\n", 
                     TO_Termlaenge(term1), TO_Termlaenge(term2), 
		     S_counter, res, term1, term2);
#else
  IO_DruckeFlexStrom(c_Strom, "%l %l %u %v\n", 
                     TO_Termlaenge(term1), TO_Termlaenge(term2), 
		     S_counter, res);
#endif
  return res;
#elif (ANALYZE == 2) || (ANALYZE == 3)
  VergleichsT res;
  hrtime_t t_start, t_end;
#if ANALYZE == 3
  pcounter_t p_start, p_end, q_start, q_end;
#endif
  int i;

  for(i = 0; i < AnzahlMessungen; i++) {
    t_start = gethrtime();
#if ANALYZE == 3
    p_start = getpcounter(0);
    q_start = getpcounter(1);
#endif
    res = ORD_Vergleichsfkt(term1, term2);
    t_end = gethrtime();
#if ANALYZE == 3
    p_end = getpcounter(0);
    q_end = getpcounter(1);
#endif
    fprintf(t_Strom, "%lld ", t_end - t_start);
#if ANALYZE == 3
    fprintf(p_Strom, "%lld ", p_end - p_start);
    fprintf(q_Strom, "%lld ", q_end - q_start);
#endif
  }
  fprintf(t_Strom, "\n");
#if ANALYZE == 3
  fprintf(p_Strom, "\n");
  fprintf(q_Strom, "\n");
#endif
  return res;
#else /* ANALYZE == ... */
  ordtracing = TRUE;
  return ORD_Vergleichsfkt(term1, term2);
#endif /* ANALYZE == ... */
#endif /* ORACLE */
}

static BOOLEAN TermGroesser(UTermT term1, UTermT term2)
/* Testet, ob term1 groesser als term2 ist (bzgl. der eingestellten Ordnung). */
{
#if ORACLE
  if (oracle_mode != oracle_none){
    BOOLEAN res1 = FALSE, res2 = FALSE;
    if ((oracle_mode == oracle_read) || (oracle_mode == oracle_check)){
      switch (*oracle){
      case 'T': res1 = TRUE;  break;
      case 'F': res1 = FALSE; break;
      default: our_error("TermGroesser: Wrong input in reading oracle!\n");
      }
      oracle++;
    }
    if ((oracle_mode == oracle_write) || (oracle_mode == oracle_check)){
      res2 = ORD_RedTestfkt(term1, term2);
    }
    if ((oracle_mode == oracle_check) && (res1 != res2)){
#if !S_doTRACE
      our_error("### Ordering result and oracle differ!\n");
#else
      IO_DruckeFlex("### ORD %t # %t   results differ: oracle: %b ordering: %b\n",
                    s, t, res1, res2);
      ordtracing = TRUE; 
      res2 = ORD_RedTestfkt(s,t); 
      ordtracing = FALSE;
#endif /* !S_doTRACE */
    }
    if (oracle_mode == oracle_write){
      switch (res2){
      case TRUE:  putc('T', oracle_out); break;
      case FALSE: putc('F', oracle_out); break;
      default: our_error("TermGroesser: Wrong result from ordering!\n");
      }
      return res2;
    }
    return res1;
  }
  else {
    return ORD_RedTestfkt(term1, term2);
  }
#else /* !ORACLE */
#if ANALYZE == 1
  BOOLEAN res;
  S_counter = 0;
  res = ORD_RedTestfkt(term1, term2);
#if VERBOSE_COUNTING
  IO_DruckeFlexStrom(c_Strom, "%l %l %u %b %t %t\n", 
                     TO_Termlaenge(term1), TO_Termlaenge(term2), 
		     S_counter, res, term1, term2);
#else
  IO_DruckeFlexStrom(c_Strom, "%l %l %u %b\n", 
                     TO_Termlaenge(term1), TO_Termlaenge(term2), 
		     S_counter, res);
#endif
  return res;
#elif (ANALYZE == 2) || (ANALYZE == 3)
  BOOLEAN res;
  hrtime_t t_start, t_end;
#if ANALYZE == 3
  pcounter_t p_start, p_end, q_start, q_end;
#endif
  int i;

  for(i = 0; i < AnzahlMessungen; i++) {
    t_start = gethrtime();
#if ANALYZE == 3
    p_start = getpcounter(0);
    q_start = getpcounter(1);
#endif
    res = ORD_RedTestfkt(term1, term2);
    t_end = gethrtime();
#if ANALYZE == 3
    p_end = getpcounter(0);
    q_end = getpcounter(1);
#endif
    fprintf(t_Strom, "%lld ", t_end - t_start);
#if ANALYZE == 3
    fprintf(p_Strom, "%lld ", p_end - p_start);
    fprintf(q_Strom, "%lld ", q_end - q_start);
#endif
  }
  fprintf(t_Strom, "\n");
#if ANALYZE == 3
  fprintf(p_Strom, "\n");
  fprintf(q_Strom, "\n");
#endif
  return res;
#else /* ANALYZE == ... */
  ordtracing = FALSE;
  return ORD_RedTestfkt(term1, term2);
#endif /* ANALYZE == ... */
#endif /* ORACLE */
}

/* ORD-1a*/
/*-------*/
BOOLEAN ORD_TermGroesserRed(UTermT term1, UTermT term2)
/* Testet, ob term1 groesser als term2 ist (bzgl. der eingestellten Ordnung). */
{
  return TermGroesser(term1,term2);
}

BOOLEAN ORD_TermGroesserUnif(UTermT term1, UTermT term2)
/* Testet, ob term1 groesser als term2 ist (bzgl. der eingestellten Ordnung). */
{
  return TermGroesser(term1,term2);
}

#ifndef CCE_Source
void ORD_ParamInfo(ParamInfoArtT what, InfoStringT DEF, InfoStringT USR)
{
  return;
  if (what == ParamEvaluate){
    ordering ord = PA_Ordering();
    char *newprec;
    char *LPOVersionString;
    char *KBOVersionString;
    int lpoversion = 0, kboversion = 0;

    printf("\n* ORDERING: "); 
    if (IS_FindOption (DEF, USR, 1, "kbo") == 0)  
      ord  = ordering_kbo; 
    else if (IS_FindOption (DEF, USR, 1, "lpo") == 0)
      ord  = ordering_lpo; 
    else if (IS_FindOption (DEF, USR, 1, "empty") == 0)                  
      ord  = ordering_empty; 
    else if (IS_FindOption (DEF, USR, 1, "col3") == 0)                  
      ord  = ordering_col3; 
    else if (IS_FindOption (DEF, USR, 1, "col4") == 0)                  
      ord  = ordering_col4; 
    else if (IS_FindOption (DEF, USR, 1, "cons") == 0)                  
      ord  = ordering_cons; 

    LPOVersionString = IS_ValueOfOption(DEF,USR, "lpoversion");
    if (LPOVersionString != NULL){
      lpoversion = atoi(LPOVersionString);
    }
    KBOVersionString = IS_ValueOfOption(DEF,USR, "kboversion");
    if (KBOVersionString != NULL){
      kboversion = atoi(KBOVersionString);
    }

    switch (ord ) { 
    case ordering_kbo:   
      printf("KBO (version %d)", kboversion); 
      break; 
    case ordering_lpo:   
      printf("LPO (version %d)", lpoversion); 
      break; 
    case ordering_empty:   
      printf("empty"); 
      break; 
    case ordering_col3:   
      printf("col3"); 
      break; 
    case ordering_col4:   
      printf("col4"); 
      break; 
    case ordering_cons:   
      printf("cons"); 
      break; 
    default:  
      ST_ErrorStatistik("Unknown Ordering",FALSE);
      break; 
    } 
    newprec =IS_ValueOfOption (DEF,USR,"prec"); 
    if (newprec) { 
      printf(", (new) precedence %s",newprec );
      if (!(PR_PraezedenzKorrekt(newprec)) )
        printf(" invalid, leaving unchanged");
    } 
#if 0
    printf("\n            current precedence is "); 
    PR_PraezedenzAusgeben();
#endif
    printf("\n"); 
  }
  else
    printf("-ord [kbo|lpo|empty]:prec=a.b.c..d.e.a  redefine ordering and precedence\n"
            "KBO only with weights=1; precedence above would be a>b>c;d>e>a\n");
}

/*=================================================================================================================*/
/*= N+1. Weiter im Text                                                                                           =*/
/*=================================================================================================================*/

#endif

static void KBOVersionSetzen(void)
{
  ORD_OrdIstKBO = TRUE;
#ifdef CCE_Source
  ORD_Vergleichsfkt = KB_KBOVergleich;
  ORD_RedTestfkt    = KB_KBOGroesser; /* alte Fassung! */
#else
  if (kboversion == 0){
    ORD_Vergleichsfkt = KB_KBOVergleich;
    ORD_RedTestfkt    = KB_KBOGroesser; /* alte Fassung! */
  }
#if 0
  else if (kboversion == 1){
    ORD_Vergleichsfkt = KB_KBOVergleich2;
    ORD_RedTestfkt    = KB_KBOGroesser2; /* alte Fassung von Bernd! */
  }
#endif
  else {
    ORD_Vergleichsfkt = KB_KBOVergleich;
    ORD_RedTestfkt    = KB_KBOGroesser; /* alte Fassung! */
  }
#endif
}


static void LPOVersionSetzen(void)
{
#ifdef CCE_Source
  lpoversion = 40;
#endif
#if COMP_COMP
  lpoversion = 5;
#endif
  ORD_OrdIstLPO = TRUE;
  if (lpoversion == 0){
    ORD_Vergleichsfkt = LP_LPO;
    ORD_RedTestfkt    = LP_LPOGroesser; /* alte Fassung! */
  }
  else if (lpoversion == 1){
    ORD_Vergleichsfkt = S_LPO1;
    ORD_RedTestfkt    = S_LPOGroesser1;
  }
  else if (lpoversion == 100){
    ORD_Vergleichsfkt = S_LPO1BU;
    ORD_RedTestfkt    = S_LPOGroesser1BU;
  }
  else if (lpoversion == 2){
    ORD_Vergleichsfkt = S_LPO2;
    ORD_RedTestfkt    = S_LPOGroesser2;
  }
  else if (lpoversion == 3){
    ORD_Vergleichsfkt = S_LPO3;
    ORD_RedTestfkt    = S_LPOGroesser3;
  }
  else if (lpoversion == 37){
    ORD_Vergleichsfkt = S_LPO37;
    ORD_RedTestfkt    = S_LPOGroesser37;
  }
  else if (lpoversion == 4){
    ORD_Vergleichsfkt = S_LPO4;
    ORD_RedTestfkt    = S_LPOGroesser4;
  }
  else if (lpoversion == 40){
    ORD_Vergleichsfkt = S_LPO40;
    ORD_RedTestfkt    = S_LPOGroesser40;
  }
  else if (lpoversion == 41){
    ORD_Vergleichsfkt = S_LPO41;
    ORD_RedTestfkt    = S_LPOGroesser41;
  }
  else if (lpoversion == 42){
    ORD_Vergleichsfkt = S_LPO42;
    ORD_RedTestfkt    = S_LPOGroesser42;
  }
  else if (lpoversion == 43){
    ORD_Vergleichsfkt = S_LPO43;
    ORD_RedTestfkt    = S_LPOGroesser43;
  }
  else if (lpoversion == 5){
    ORD_Vergleichsfkt = S_LPO5;
    ORD_RedTestfkt    = S_LPOGroesser5;
  }
  else if (lpoversion == 50){
    ORD_Vergleichsfkt = S_LPO50;
    ORD_RedTestfkt    = S_LPOGroesser50;
  }
  else if (lpoversion == 6){
    ORD_Vergleichsfkt = S_LPO6;
    ORD_RedTestfkt    = S_LPOGroesser6;
  }
  else if (lpoversion == 7){
    ORD_Vergleichsfkt = S_LPO7;
    ORD_RedTestfkt    = S_LPOGroesser7;
  }
  else if (lpoversion == 8){
    ORD_Vergleichsfkt = S_LPO8;
    ORD_RedTestfkt    = S_LPOGroesser8;
  }
  else if (lpoversion == 9){
    ORD_Vergleichsfkt = S_LPO9;
    ORD_RedTestfkt    = S_LPOGroesser9;
  }
  else if (lpoversion == 10){
    ORD_Vergleichsfkt = S_LPO10;
    ORD_RedTestfkt    = S_LPOGroesser10;
  }
  else if (lpoversion == 11){
    ORD_Vergleichsfkt = S_LPO11;
    ORD_RedTestfkt    = S_LPOGroesser11;
  }
  else {
    our_error("nicht vglb. LPO-Fassung!");
    ORD_Vergleichsfkt = S_LPO;
    ORD_RedTestfkt    = S_LPOGroesser; /* Christians Fassung! */
  }
}

#ifdef CCE_Source
BOOLEAN ORD_InitAposteriori (/*InfoStringT DEF, InfoStringT USR*/) 
#else
BOOLEAN ORD_InitAposteriori (InfoStringT DEF, InfoStringT USR) 
#endif
{
#ifndef CCE_Source
  ordering oldOrd = PA_Ordering();
#else
  ordering_type PA_Ordering = ord_name(get_ordering());
  ordering_type oldOrd = PA_Ordering;
#endif
  char *newprec;
  char *LPOVersionString;
  char *KBOVersionString;
#ifdef CCE_Source
  /*PA_Ordering = ordering_lpo;*/
  if (PA_Ordering == ordering_lpo){
    if (1){
      ORD_Vergleichsfkt = ConsVergleich; 
      ORD_RedTestfkt = ConsTest;
      ORD_OrdIstLPO = TRUE;
      ORD_OrdIstKBO = FALSE;
    }
    else {
      LPOVersionSetzen();
    }
  }
  else {
    KBOVersionSetzen();
  }
#if 0
#if 0
  ORD_Vergleichsfkt = LP_LPO; 
  ORD_RedTestfkt = LP_LPOGroesser; 
#else
  ORD_Vergleichsfkt = ConsVergleich; 
  ORD_RedTestfkt = ConsTest;
#endif
#endif
#else

  if (IS_FindOption (DEF, USR, 1, "kbo") == 0)  
    PA_SetOrdering(ordering_kbo);
  else if (IS_FindOption (DEF, USR, 1, "lpo") == 0)
    PA_SetOrdering(ordering_lpo);
  else if (IS_FindOption (DEF, USR, 1, "empty") == 0)
    PA_SetOrdering(ordering_empty);
  else if (IS_FindOption (DEF, USR, 1, "col3") == 0)
    PA_SetOrdering(ordering_col3);
  else if (IS_FindOption (DEF, USR, 1, "col4") == 0)
    PA_SetOrdering(ordering_col4);
  else if (IS_FindOption (DEF, USR, 1, "cons") == 0)
    PA_SetOrdering(ordering_cons);

  LPOVersionString = IS_ValueOfOption(DEF,USR, "lpoversion");
  if (LPOVersionString != NULL){
    lpoversion = atoi(LPOVersionString);
  }
  KBOVersionString = IS_ValueOfOption(DEF,USR, "kboversion");
  if (KBOVersionString != NULL){
    kboversion = atoi(KBOVersionString);
  }

  ORD_OrdnungTotal = FALSE;
  ORD_OrdIstLPO = FALSE;
  ORD_OrdIstKBO = FALSE;

  switch (PA_Ordering()) { 
  case ordering_kbo:
    if (oldOrd != PA_Ordering())
      if(!COMP_COMP)printf("changed to KBO");
    KBOVersionSetzen();
    ORD_OrdnungTotal = PR_PraezedenzTotal;
    break;
  case ordering_lpo:   
    if (oldOrd != PA_Ordering()) 
      if(!COMP_COMP)printf("changed to LPO");
    LPOVersionSetzen();
    if (lpoversion == 0) {
      LV_InitAposteriori();
    }
    ORD_OrdnungTotal = PR_PraezedenzTotal;
    break;
  case ordering_empty:
    if (oldOrd != PA_Ordering()) 
      if(0)printf("changed to empty");
    ORD_Vergleichsfkt = LeererVergleich;
    ORD_RedTestfkt = LeererTest;
    break;
  case ordering_col3:
    if (oldOrd != PA_Ordering()) 
      if(0)printf("changed to col3");
    ORD_Vergleichsfkt = Col3Vergleich;
    ORD_RedTestfkt = Col3Test;
    TO_KombinatorapparatAktivieren();
    break;
  case ordering_col4:
    if (oldOrd != PA_Ordering()) 
      if(0)printf("changed to col4");
    ORD_Vergleichsfkt = Col4Vergleich;
    ORD_RedTestfkt = Col4Test;
    TO_KombinatorapparatAktivieren();
    break;
  case ordering_cons:
    if (oldOrd != PA_Ordering()) 
      if(0)printf("changed to cons");
    ORD_Vergleichsfkt = ConsVergleich;
    ORD_RedTestfkt = ConsTest;
    LV_InitAposteriori();
    break;
  default:
    ST_ErrorStatistik("Unknown Ordering",FALSE);
    our_error("Unknown Ordering");
    break;
  } 
#endif

#ifndef CCE_Source
  newprec =IS_ValueOfOption (DEF,USR,"prec");
  if (newprec) {
    if(0)printf(", (new) precedence %s", newprec );
    if (!PR_PraezedenzErneuert(newprec))  
      our_error("Invalid Precedence");
  } 
  SG_SymbGewichteEintragen(DEF,USR,2);
#endif
  if (PA_Ordering() == ordering_kbo)
    KB_InitAposteriori();    

#if 0
  printf("\n            current precedence is ");
  PR_PraezedenzAusgeben();
#endif
#if ORACLE
  {
    if (!oracle_initialized && 
        (IS_FindOption (DEF, USR, 1, "oracle") == 0)){
      char *filename;
      oracle_initialized = TRUE;
      if ((filename = IS_ValueOfOption(DEF, USR, "read"))){
        oracle_mode = oracle_read;
      }
      else if ((filename = IS_ValueOfOption(DEF, USR, "write"))){
        oracle_mode = oracle_write;
      }
      else if ((filename = IS_ValueOfOption(DEF, USR, "check"))){
        oracle_mode = oracle_check;
      }
      else {
        oracle_mode = oracle_none;
      }
      if (oracle_mode == oracle_write){
        oracle_out = fopen(filename, "w");
        if (oracle_out == NULL){
          our_warning("cannot open oracle file for writing!\n");
          oracle_mode = oracle_none;
        }
      }
      else if ((oracle_mode == oracle_read)||(oracle_mode == oracle_check)){
        int fd;
        struct stat statbuf;

        fd = open(filename, O_RDONLY);
        if (fd < 0){
          our_warning("cannot open oracle file for reading!\n");
          oracle_mode = oracle_none;
        }
        else {
          if (fstat(fd,&statbuf) < 0){
            our_warning("cannot stat oracle file!\n");
            oracle_mode = oracle_none;
          }
          else {
            oracle = mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
            if (oracle == (char *) -1){
              our_warning("cannot mmap oracle file!\n");
              oracle_mode = oracle_none;
            }
          }
        }
      }
    }
    else {
      oracle_mode = oracle_none;
    }
  }
#else
#if ANALYZE > 0
  {
    char *basisname = IO_SpezifikationsBasisname();
    char dateiname[FILENAME_MAX];
    int i;
    if (basisname == NULL){
      our_error("basisname NULL");
    }

#if ANALYZE == 1
    snprintf(dateiname,FILENAME_MAX,"%s.%d.c",basisname,lpoversion);
    c_Strom = fopen(dateiname, "w");
    if (c_Strom == NULL){
      our_error("cannot open file for writing counter!\n");
    }
#endif

#if (ANALYZE == 2) || (ANALYZE == 3)
    for (i = 1; i < 100; i++){
      snprintf(dateiname,FILENAME_MAX,"%s.%d.t.%02d",basisname,lpoversion,i);
      t_Strom = fopen(dateiname, "r");
      if (t_Strom == NULL){
        t_Strom = fopen(dateiname, "w");
        if (t_Strom == NULL){
          our_error("cannot open file for writing times!\n");
        }
        break;
      }
    }
    if (i == 100){
      our_error("all files exists (*.01 .. *.99)\n");
    }
      
#if ANALYZE == 3
    snprintf(dateiname,FILENAME_MAX,"%s.%d.p.%02d",basisname,lpoversion,i);
    p_Strom = fopen(dateiname, "w");
    if (p_Strom == NULL){
      our_error("cannot open file for writing first perfctr!\n");
    }
    snprintf(dateiname,FILENAME_MAX,"%s.%d.q.%02d",basisname,lpoversion,i);
    q_Strom = fopen(dateiname, "w");
    if (q_Strom == NULL){
      our_error("cannot open file for writing second perfctr!\n");
    }
#endif
#endif
  }
#endif /* ANALYZE > 0 */
#endif /* ORACLE */
  return TRUE;
}

#ifndef CCE_Source
void ORD_OrdnungUmstellen(ordering Ordnung)
#else
void ORD_OrdnungUmstellen(ordering_type Ordnung)
#endif
{
  ORD_OrdnungTotal = FALSE;
  ORD_OrdIstLPO = FALSE;
  ORD_OrdIstKBO = FALSE;
  switch (Ordnung){
  case ordering_lpo:
    LPOVersionSetzen();
    if (lpoversion == 0){
      LV_InitAposteriori();
    }
    ORD_OrdnungTotal = PR_PraezedenzTotal;
    break;
  case ordering_kbo:
    KBOVersionSetzen();
    KB_InitAposteriori();   /* Phi auf Gueltigkeit testen.*/
    ORD_OrdnungTotal = PR_PraezedenzTotal;
    break;
#ifndef CCE_Source
  case ordering_empty:
    ORD_Vergleichsfkt = LeererVergleich;
    ORD_RedTestfkt = LeererTest;
    break;
  case ordering_col3:
    ORD_Vergleichsfkt = Col3Vergleich;
    ORD_RedTestfkt = Col3Test;
    break;
  case ordering_col4:
    ORD_Vergleichsfkt = Col4Vergleich;
    ORD_RedTestfkt = Col4Test;
    break;
  case ordering_cons:
    ORD_Vergleichsfkt = ConsVergleich;
    ORD_RedTestfkt = ConsTest;
    LV_InitAposteriori();
    break;
#endif
  default:
#ifndef CCE_Source
    ST_ErrorStatistik("Invalid Ordering in ORD_OrdnungUmstellen", FALSE);
#else
    our_error("Invalid Ordering in ORD_OrdnungUmstellen");
#endif
    break;
  }
#ifndef CCE_Source
  PA_SetOrdering(Ordnung);
#endif
}

BOOLEAN ORD_OrdIstcLPO(void)
{
  return ORD_Vergleichsfkt == ConsVergleich;
}

static BOOLEAN        aufZGUmgestellt = FALSE;
static ORDVergleichsT SAVE_Vergleichsfkt;
static ORDTestT       SAVE_RedTestfkt;

void ORD_aufZwangsGrundUmstellen(void)
{
  if (aufZGUmgestellt){
    our_error("inconsitent call to ORD_aufZwangsGrundUmstellen");
  }
  SAVE_Vergleichsfkt = ORD_Vergleichsfkt;
  SAVE_RedTestfkt    = ORD_RedTestfkt;
  if (ORD_OrdIstLPO){
    ORD_Vergleichsfkt = ZGO_LPO;
    ORD_RedTestfkt    = ZGO_LPOGroesser;
  }
  else if (ORD_OrdIstKBO){
    ORD_Vergleichsfkt = ZGO_KBOVergleich;
    ORD_RedTestfkt    = ZGO_KBOGroesser;
  }
  else {
    our_error("ORD_aufZwangsGrundUmstellen with unkown ordering");
  }
  aufZGUmgestellt    = TRUE;
}

void ORD_vonZwangsGrundZurueckstellen(void)
{
  if (!aufZGUmgestellt){
    our_error("inconsitent call to ORD_vonZwangsGrundZurueckstellen");
  }
  ORD_Vergleichsfkt = SAVE_Vergleichsfkt;
  ORD_RedTestfkt    = SAVE_RedTestfkt;
  aufZGUmgestellt   = FALSE;
}

