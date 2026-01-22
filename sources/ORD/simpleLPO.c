
/* ==========================================================================================================
   Verschiedene LPO-Versionen zum Messen der rekursiven Aufrufe
   ========================================================================================================== */

#include "simpleLPO.h"
#include "SymbolOperationen.h"
#include "Ausgaben.h"


#define welcheVariante 4 /* 1, 2, 3, 4 */
/* 1 = Direkte / naive Implementierung
   2 = alpha-Fall nach hinten ziehen
   3 = Transfer von positivem Wissen im gamma-Fall (zusaetzlich)
   4 = Transfer von neagativem Wissen in beta/gamma-Fall (zusaetzlich)
*/

/* Das TRACE schaltet sich ein, wenn sich die Ergebnisse von LP_LPO und S_LPO in Ordnungen.c unterscheiden. */
/* wenn welcheLPO == 2 */
/* Dies wird ueber die externe Variable ordtracing erreicht. */
extern BOOLEAN ordtracing;
#define TRACE             1 /* 0, 1 */
#define INDENT_TRACE      1 /* 0, 1 */


/* Messungen werden in Ordnungen.c nur mit Aufrufen von S_LPOGroesser gemacht */
/* wird nur hier in S_LPOGroesser inkrementiert */
/* wird in Ordnungen.c benoetigt */
int RekAufrufeLPOGroesserZaehler = 0;




/* benoetigen alle Varianten */
/* -------------------------------------------------------------------------- */
/* - Tracing Infrastructure ------ BEGINN ----------------------------------- */
/* -------------------------------------------------------------------------- */

#if TRACE
static unsigned int tracelevel = 0;

static void traceindent (int level)
{
  if (INDENT_TRACE){
    int i;
    for (i=0; i < level; i++){
      fputs("| ", stdout);
    }
  }
  else {
    printf("%3d:", level);
  }
}
#endif
#if TRACE == 1
#define OPENTRACE(x)  {if(ordtracing){traceindent(tracelevel++);IO_DruckeFlex x;}}
#define CLOSETRACE(x) {if(ordtracing){traceindent(--tracelevel);IO_DruckeFlex x;}}
#define RESTRACE(x)   {if(ordtracing){traceindent(tracelevel);IO_DruckeFlex x;}}
#else
#define OPENTRACE(x)  /* nix */
#define CLOSETRACE(x) /* nix */
#define RESTRACE(x)   /* nix */
#endif

#if 0
static BOOLEAN checkConstraint (EConsT e, BOOLEAN soll)
{
  BOOLEAN res = FALSE;
  OPENTRACE  (("checkConstraint %E ->? %b\n", e, soll));
  {
    res = ...
  }
  CLOSETRACE (("checkConstraint -> %b\n", res));
  return res;
}
#endif

/* -------------------------------------------------------------------------- */
/* - Tracing Infrastructure ----- ENDE -------------------------------------- */
/* -------------------------------------------------------------------------- */







#if welcheVariante == 1

/* ==========================================================================================================
   1 = Direkte / naive Implementierung
   ========================================================================================================== */

/* -------------------------------------------------------------------------------------------------------------*/
/* ------------------- Funktions-Deklarationen fuer LPO --------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

VergleichsT S_LPO(UTermT Term1, UTermT Term2);

BOOLEAN S_LPOGroesser(UTermT Term1, UTermT Term2); /* nur Wrapper fuer klein s_LPOGroesser */
/* Intern werden nur die Prozeduren mit kleinem s benutzt. */

static BOOLEAN s_LPOGroesser(UTermT Term1, UTermT Term2);
static BOOLEAN s_LPOGroesserGleich(UTermT Term1, UTermT Term2);

static BOOLEAN s_LPOAlpha(UTermT Term1, UTermT Term2);
static BOOLEAN s_LPOGamma(UTermT Term1, UTermT Term2);
static BOOLEAN s_LPOBeta(UTermT Term1, UTermT Term2);
static BOOLEAN s_LPODelta(UTermT Term1, UTermT Term2);


/* -------------------------------------------------------------------------------------------------------------*/
/* ------------------- S_LPO ------ Wird von Aussen benutzt ! --------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

VergleichsT S_LPO(UTermT Term1, UTermT Term2)
{
  VergleichsT retWert = Unvergleichbar;

  OPENTRACE(("S_LPO: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    
    /* OPTIMIERT TO_TermeGleich zuerst getestet */
    
    if(TO_TermeGleich(Term1, Term2))
      {
	retWert = Gleich;
      }
    else if(s_LPOGroesser(Term2, Term1))
      {
	retWert = Kleiner;
      }
    else if(s_LPOGroesser(Term1, Term2))
      {
	retWert = Groesser;
      }
  }

    /* retWert wird mit Unvergleichbar initialisiert */    

  /* %v VergleichsT */
  CLOSETRACE(("S_LPO: Term1 = %t,  Term2 = %t --> %v\n", Term1, Term2, retWert));
  return retWert;
}




/* -------------------------------------------------------------------------------------------------------------*/
/* --- S_LPOGroesser ---- Wird von Aussen benutzt ! ----- Wrapper-Prozedur fuer klein s_LPOGroesser ------------*/
/* -------------------------------------------------------------------------------------------------------------*/


BOOLEAN S_LPOGroesser(UTermT Term1, UTermT Term2)
{
  RekAufrufeLPOGroesserZaehler = 0;

  return s_LPOGroesser(Term1, Term2);
}



/* -------------------------------------------------------------------------------------------------------------*/
/* ------------------- Gehe die Faelle der LPO-Definition durch ------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN s_LPOGroesser(UTermT Term1, UTermT Term2)
{
  BOOLEAN retWert;

  RekAufrufeLPOGroesserZaehler++;



  /* Terme sind gleich! also s _nicht_ groesser als t ! --------------------------------------------------------*/
  /* Diesr Test ist notwendig, weil s_LPOGroesser auch direkt von Aussen benutzt wird (ueber S_LPOGroesser) ----*/
  if(TO_TermeGleich(Term1, Term2)) 
    {
      OPENTRACE(("s_LPOGroesser: Term1 = %t,  Term2 = %t\n", Term1, Term2));
      {
	retWert = FALSE;
	/* ---------------------------------------------------------------------------------- retWert = FALSE --*/
      }
      CLOSETRACE(("s_LPOGroesser: Term1 = %t,  Term2 = %t --> %b Terme gleich\n", Term1, Term2, retWert));
      return retWert;
    }
  /* -----------------------------------------------------------------------------------------------------------*/

  OPENTRACE(("s_LPOGroesser: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {


    if(s_LPOAlpha(Term1, Term2))
      {
	retWert = TRUE;
      }
    else if(s_LPOBeta(Term1, Term2)) 
      {                                              
	retWert = TRUE;
      }
    else if(s_LPOGamma(Term1, Term2)) 
      {						         
	retWert = TRUE;
      }
    else if(s_LPODelta(Term1, Term2))
      {
	retWert = TRUE;
      }
    else
      {
	retWert = FALSE;
      }
  }
  CLOSETRACE(("s_LPOGroesser: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}

/* -------------------------------------------------------------------------------------------------------------*/
/* ---------------------- Test fuer ALPHA-Fall: s_i >=lpo t fuer _ein_ i ---------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN s_LPOAlpha(UTermT Term1, UTermT Term2) {


  BOOLEAN retWert = FALSE; /* wenn es in der Schleife nicht auf TRUE gesetzt wird, gilt FALSE */
  /* weil dann gilt: s_i >=lpo t fuer _kein_ i */

  OPENTRACE(("s_LPOAlpha: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
 
    UTermT NULLt = TO_NULLTerm(Term1);  /* stellt das Ende des Terms dar */
    UTermT teilterm = TO_ErsterTeilterm(Term1);


    /* gehe durch alle Teilterme s_i */
    while(teilterm != NULLt) { /* bis wir am Ende von s angekommen sind */
    
      if(s_LPOGroesserGleich(teilterm, Term2)) {
	retWert = TRUE; /* ---------- der ALPHA-Fall hat zugeschlagen ----------------------- retWert = TRUE ---*/
	break; /* keine weiteren Vergleiche nötig */
      }
    
      teilterm = TO_NaechsterTeilterm(teilterm);
    }
  }
  /* retWert wird mit FALSE initialisiert */    
  CLOSETRACE(("s_LPOAlpha: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}



/* -------------------------------------------------------------------------------------------------------------*/
/* ---------------------- Test fuer Beta-Fall: f > g und s >lpo t_j fuer _alle_ j ------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/


BOOLEAN s_LPOBeta(UTermT Term1, UTermT Term2) {

  BOOLEAN retWert = FALSE;

  /* OPTIMIERT hier nur Deklaration, Zuweisung erst im If-Zweig in dem Variablen verwendet werden */
  UTermT NULLt, teilterm;


  /* s oder t ist eine Variable --------------------------------------------------------------------------------*/
  if(TO_TermIstVar(Term1) || TO_TermIstVar(Term2)) {
    OPENTRACE(("s_LPOBeta: Term1 = %t,  Term2 = %t\n", Term1, Term2));
    {
      retWert = FALSE;
      /* ist s oder t eine Variable, dann ist der Beta-Fall nicht zustaendig ---------------- retWert = FALSE --*/
    }
    CLOSETRACE(("s_LPOBeta: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
    return retWert;
  }
  /* -----------------------------------------------------------------------------------------------------------*/


  OPENTRACE(("s_LPOBeta: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    /* ------------------ Vergleiche die Topsymbole anhand der Preazedenz miteinander --------------------------*/

    if(PR_Praezedenz(TO_TopSymbol(Term1), TO_TopSymbol(Term2)) == Groesser)
      {
	/* ---------------- hier gilt: f > g, also teste ob s >lpo t_j fuer _alle_ j ---------------------------*/

	/* OPTIMIERT Zuweisung erst hier (s.o.) */
	NULLt = TO_NULLTerm(Term2);  /* stellt das Ende des Terms dar */
	teilterm = TO_ErsterTeilterm(Term2);

	retWert = TRUE;
	/* - a priori -- s >lpo t_j fuer _alle_ j ---------------------------------------- retWert = TRUE ------*/
	/* es sei denn in der Schleife wird bei _einem_ Vergleich retWert = FALSE gesetzt */

	/* gehe durch alle Teilterme t_j */
	while(teilterm != NULLt) { /* bis wir am Ende von t angekommen sind */
	
	  if (!(s_LPOGroesser (Term1, teilterm))) 
	    {
	      retWert = FALSE; /* weil fuer _alle_ : sobald einer nicht: --------------------> retWert = FALSE -*/
	      break; /* keine weiteren Vergleiche nötig */
	    }
	  
	  teilterm = TO_NaechsterTeilterm(teilterm);
	}
      }
    else
      {
	retWert = FALSE; /* hier gilt: f !> g    also nicht der Beta-Fall -------------------- retWert = FALSE -*/
      }
  }
  CLOSETRACE(("s_LPOBeta: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}



/* -------------------------------------------------------------------------------------------------------------*/
/* ----------------------- Test fuer Gamma-Fall: f = g ---------------------------------------------------------*/
/* ----------------------------------------- und s_1, ... , s_m >*lpo t_1, ... , t_n ---------------------------*/
/* ----------------------------------------- und s >lpo t_j fuer _alle_ j --------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/


BOOLEAN s_LPOGamma(UTermT Term1, UTermT Term2) {


  BOOLEAN retWert = FALSE;
  BOOLEAN lexGroesser = FALSE;
  BOOLEAN einerNicht = FALSE;


  /* OPTIMIERT hier nur Deklaration, Zuweisung erst im If-Zweig in dem Variablen verwendet werden */
  UTermT NULLtT1, NULLtT2, teiltermT1, teiltermT2;

  /* s oder t ist eine Variable --------------------------------------------------------------------------------*/
  if(TO_TermIstVar(Term1) || TO_TermIstVar(Term2)) {
    OPENTRACE(("s_LPOGamma: Term1 = %t,  Term2 = %t\n", Term1, Term2));
    {
      retWert = FALSE;
      /* ist s oder t eine Variable, dann ist der Gamma-Fall nicht zustaendig --------------- retWert = FALSE --*/
    }
    CLOSETRACE(("s_LPOGamma: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
    return retWert;
  }
  /* -----------------------------------------------------------------------------------------------------------*/


  /* ---------------- Vergleiche die Topsymbole anhand der Preazedenz miteinander ------------------------------*/
  OPENTRACE(("s_LPOGamma: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    if(PR_Praezedenz(TO_TopSymbol(Term1), TO_TopSymbol(Term2)) == Gleich)
      { 
	/* ------------ hier gilt: f = g, also teste zunaechst ob s_1, ... , s_n >*lpo t_1, ... , t_n ----------*/
	/* ------------ falls ja, teste ob s >lpo t_j fuer _alle_ j --------------------------------------------*/

	/* OPTIMIERT Zuweisung erst hier (s.o.) */
	NULLtT1 = TO_NULLTerm(Term1);  /* stellt das Ende des Terms s dar */
	NULLtT2 = TO_NULLTerm(Term2);  /* stellt das Ende des Terms t dar */
	
	teiltermT1 = TO_ErsterTeilterm(Term1);
	teiltermT2 = TO_ErsterTeilterm(Term2);


	/* Sobald ein Teilterm von Term1 groesser als derjenige von Term2 ist, ... */
	/* wird geprueft, ob s >lpo t_j fuer _alle_ j, falls ja, wird TRUE returniert */
	/* Sobald ein Teilterm von Term1 nicht gleich und nicht groesser als derjenige von Term2 ist, ... */
	/* wird FALSE returniert */

	/* Schleifeninvariante: alle bisherigen Teilterme von Term1 sind gleich denen von Term2 */
	while((teiltermT1 != NULLtT1) && (teiltermT2 != NULLtT2)) 
	  /* bis wir am Ende von t oder s angekommen sind */
	  /* eigentlich kann man an dieser Stelle davon ausgehen, dass die Terme gleich viele Teilterme haben */
	  /* dies gilt, weil die TopSymbole gleich sind (-> Termstruktur) */
	  {
	    if(s_LPOGroesser(teiltermT1, teiltermT2))
	      {
		/* hier gilt: teiltermT1 >lpo teiltermT2  also teste ob s >lpo t_j fuer _alle_ j ---------------*/
		/* dafuer wird die while-Schleife mit BREAK beendet */
		
		lexGroesser = TRUE; /* Damit unten festgestellt werden kann, weshalb gebreaked wurde, ... */
		/* das heisst, ob ein teiltermT1 >lpo teiltermT2 */
		/* in diesem Fall wird naemlich getestet ob s >lpo t_j fuer _alle_ j */
		/* ansonsten wird FALSE returniert */
		break;
	      }
	    else if(!(TO_TermeGleich(teiltermT1, teiltermT2)))
	      {
		retWert = FALSE; /* hier gilt: teiltermT1 !>lpo teiltermT2 und teiltermT1 != teiltermT2 --------*/
		/* ---------------------------------------------------------------------------- retWert = FALSE */
		break; /* keine weiteren Vergleiche noetig */
	      }

	    /* hier gilt: teiltermT1 !>lpo teiltermT2 und teiltermT1=teiltermT2 -> lexikographisch weiterpruefen*/
	    teiltermT1 = TO_NaechsterTeilterm(teiltermT1);
	    teiltermT2 = TO_NaechsterTeilterm(teiltermT2);
	  }

	/* Wenn wir die Schleife _ohne_ BREAK verlassen, dann sind die Terme gleich! */


	if(lexGroesser == TRUE)
	  {
	    /* hier gilt: die obere while-Schleife wurde verlassen, weil ein teiltermT1 >lpo teiltermT2, ... */
	    /* also teste ob s >lpo t_j fuer _alle_ j */

	    teiltermT2 = TO_ErsterTeilterm(Term2);

	    while((teiltermT2 != NULLtT2)) /* bis an das Ende von Term2 */
	      {
		if(!(s_LPOGroesser(Term1,teiltermT2)))
		  {
		    retWert = FALSE; /* weil 'fuer _alle_' : sobald einer nicht: --------> retWert = FALSE -*/
		    einerNicht = TRUE;
		    break; /* keine weiteren Vergleiche noetig */
		  }
		teiltermT2 = TO_NaechsterTeilterm(teiltermT2);
	      }
	    
	    if(einerNicht == FALSE)
	      {
		/* hier wissen wir, dass die obige While-Schleife _nicht_ mit break verlassen wurde. */
		/* also hier gilt: ein teiltermT1 >lpo teiltermT2 und  s >lpo t_j fuer _alle_ j  */
		/* --------------------------------------------------------------------------- retWert = TRUE --*/
		retWert = TRUE; 
	      }
	  } /* endif (lexGroesser == TRUE) */
	else 
	  {
	    retWert = FALSE; /* hier gilt:             s !> t ----------------------------- retWert = FALSE ---*/
	  }
      }
    else
      {
	retWert = FALSE; /* hier gilt: f != g     also nicht der GAMMA-Fall ---------------- retWert = FALSE ---*/
      }
  }
  CLOSETRACE(("s_LPOGamma: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}




/* -------------------------------------------------------------------------------------------------------------*/
/* ------------------ Test fuer Delta-Fall: x element Var(t) und t != x ----------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN s_LPODelta(UTermT Term1, UTermT Term2) {

  BOOLEAN retWert = FALSE;

  OPENTRACE(("s_LPODelta: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    if(TO_TermIstVar(Term2))
      {
	if(TO_SymbolEnthalten (TO_TopSymbol(Term2), Term1))
	  {
	    if(!TO_TermeGleich(Term1, Term2))
	      {
		retWert = TRUE; /* ------------ Der Delta-Fall schlaegt zu ----------------- retWert = TRUE ----*/
	      }
	  }
      }
    else /* hier gilt: Term2 ist keine Variable */
      {
	retWert = FALSE; /* Delta-Fall nicht zustaendig ------------------------------------ retWert = FALSE ---*/
      }
  }
  CLOSETRACE(("s_LPODelta: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}




/* -------------------------------------------------------------------------------------------------------------*/
/* -------------- Kombination von TO_TermeGleich und s_LPOGroesser ---------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN s_LPOGroesserGleich(UTermT Term1, UTermT Term2)
{
  BOOLEAN retWert = FALSE; /* wenn beide ifs zu FALSE ausgewertet werden gilt: t != s und t !> s */

  OPENTRACE(("s_LPOGroesserGleich: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    if(TO_TermeGleich(Term1, Term2))  /* aus t = s folgt t >= s */
      {
	retWert = TRUE; 
      }
    else if(s_LPOGroesser(Term1, Term2))  /* aus t > s folgt t >= s */
      {
	retWert = TRUE;
      }
  }
  CLOSETRACE(("s_LPOGroesserGleich: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}



#endif /* welcheVariante == 1 */














#if welcheVariante == 2
/* ==========================================================================================================
   2 = alpha-Fall nach hinten ziehen
   ========================================================================================================== */

/* -------------------------------------------------------------------------------------------------------------*/
/* ------------------- Funktions-Deklarationen fuer LPO --------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

VergleichsT S_LPO(UTermT Term1, UTermT Term2);

BOOLEAN S_LPOGroesser(UTermT Term1, UTermT Term2); /* nur Wrapper fuer klein s_LPOGroesser */
/* Intern werden nur die Prozeduren mit kleinem s benutzt. */

static BOOLEAN s_LPOGroesser(UTermT Term1, UTermT Term2);
static BOOLEAN s_LPOGroesserGleich(UTermT Term1, UTermT Term2);

static BOOLEAN s_LPOAlpha(UTermT Term1, UTermT Term2);
static BOOLEAN s_LPOGamma(UTermT Term1, UTermT Term2);
static BOOLEAN s_LPOBeta(UTermT Term1, UTermT Term2);
static BOOLEAN s_LPODelta(UTermT Term1, UTermT Term2);


/* -------------------------------------------------------------------------------------------------------------*/
/* ------------------- S_LPO ------ Wird von Aussen benutzt ! --------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

VergleichsT S_LPO(UTermT Term1, UTermT Term2)
{
  VergleichsT retWert = Unvergleichbar;

  OPENTRACE(("S_LPO: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    
    /* OPTIMIERT TO_TermeGleich zuerst getestet */
 
   if(TO_TermeGleich(Term1, Term2))
      {
	retWert = Gleich;
      }
    else if(s_LPOGroesser(Term2, Term1))
      {
	retWert = Kleiner;
      }
    else if(s_LPOGroesser(Term1, Term2))
      {
	retWert = Groesser;
      }
    

    /* retWert wird mit Unvergleichbar initialisiert */    

  }
  /* %v VergleichsT */
  CLOSETRACE(("S_LPO: Term1 = %t,  Term2 = %t --> %v\n", Term1, Term2, retWert));
  return retWert;
}




/* -------------------------------------------------------------------------------------------------------------*/
/* --- S_LPOGroesser ---- Wird von Aussen benutzt ! ----- Wrapper-Prozedur fuer klein s_LPOGroesser ------------*/
/* -------------------------------------------------------------------------------------------------------------*/


BOOLEAN S_LPOGroesser(UTermT Term1, UTermT Term2)
{
  RekAufrufeLPOGroesserZaehler = 0;

  return s_LPOGroesser(Term1, Term2);
}


/* -------------------------------------------------------------------------------------------------------------*/
/* ------------------- Gehe die Faelle der LPO-Definition durch ------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN s_LPOGroesser(UTermT Term1, UTermT Term2)
{
  BOOLEAN retWert;

  RekAufrufeLPOGroesserZaehler++;



  /* Terme sind gleich! also s _nicht_ groesser als t ! --------------------------------------------------------*/
  /* Diesr Test ist notwendig, weil s_LPOGroesser auch direkt von Aussen benutzt wird (ueber S_LPOGroesser) ----*/
  if(TO_TermeGleich(Term1, Term2)) 
    {
      OPENTRACE(("s_LPOGroesser: Term1 = %t,  Term2 = %t\n", Term1, Term2));
      {
	retWert = FALSE;
	/* ---------------------------------------------------------------------------------- retWert = FALSE --*/
      }
      CLOSETRACE(("s_LPOGroesser: Term1 = %t,  Term2 = %t --> %b Terme gleich\n", Term1, Term2, retWert));
      return retWert;
    }
  /* -----------------------------------------------------------------------------------------------------------*/

  OPENTRACE(("s_LPOGroesser: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {


    if(s_LPOBeta(Term1, Term2)) 
      {                                              
	retWert = TRUE;
      }
    else if(s_LPOGamma(Term1, Term2)) 
      {						         
	retWert = TRUE;
      }
    else if(s_LPOAlpha(Term1, Term2))
      {
	retWert = TRUE;
      }  
    else if(s_LPODelta(Term1, Term2))
      {
	retWert = TRUE;
      }
    else
      {
	retWert = FALSE;
      }
  }
  CLOSETRACE(("s_LPOGroesser: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}

/* -------------------------------------------------------------------------------------------------------------*/
/* ---------------------- Test fuer ALPHA-Fall: s_i >=lpo t fuer _ein_ i ---------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN s_LPOAlpha(UTermT Term1, UTermT Term2) {


  BOOLEAN retWert = FALSE; /* wenn es in der Schleife nicht auf TRUE gesetzt wird, gilt FALSE */
  /* weil dann gilt: s_i >=lpo t fuer _kein_ i */

  OPENTRACE(("s_LPOAlpha: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
 
    UTermT NULLt = TO_NULLTerm(Term1);  /* stellt das Ende des Terms dar */
    UTermT teilterm = TO_ErsterTeilterm(Term1);


    /* gehe durch alle Teilterme s_i */
    while(teilterm != NULLt) { /* bis wir am Ende von s angekommen sind */
    
      if(s_LPOGroesserGleich(teilterm, Term2)) {
	retWert = TRUE; /* ---------- der ALPHA-Fall hat zugeschlagen ----------------------- retWert = TRUE ---*/
	break; /* keine weiteren Vergleiche nötig */
      }
    
      teilterm = TO_NaechsterTeilterm(teilterm);
    }
  }
  /* retWert wird mit FALSE initialisiert */    
  CLOSETRACE(("s_LPOAlpha: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}



/* -------------------------------------------------------------------------------------------------------------*/
/* ---------------------- Test fuer Beta-Fall: f > g und s >lpo t_j fuer _alle_ j ------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/


BOOLEAN s_LPOBeta(UTermT Term1, UTermT Term2) {

  BOOLEAN retWert = FALSE;

  /* OPTIMIERT hier nur Deklaration, Zuweisung erst im If-Zweig in dem Variablen verwendet werden */
  UTermT NULLt, teilterm;


  /* s oder t ist eine Variable --------------------------------------------------------------------------------*/
  if(TO_TermIstVar(Term1) || TO_TermIstVar(Term2)) {
    OPENTRACE(("s_LPOBeta: Term1 = %t,  Term2 = %t\n", Term1, Term2));
    {
      retWert = FALSE;
      /* ist s oder t eine Variable, dann ist der Beta-Fall nicht zustaendig ---------------- retWert = FALSE --*/
    }
    CLOSETRACE(("s_LPOBeta: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
    return retWert;
  }
  /* -----------------------------------------------------------------------------------------------------------*/


  OPENTRACE(("s_LPOBeta: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    /* ------------------ Vergleiche die Topsymbole anhand der Preazedenz miteinander --------------------------*/

    if(PR_Praezedenz(TO_TopSymbol(Term1), TO_TopSymbol(Term2)) == Groesser)
      {
	/* ---------------- hier gilt: f > g, also teste ob s >lpo t_j fuer _alle_ j ---------------------------*/

	/* OPTIMIERT Zuweisung erst hier (s.o.) */
	NULLt = TO_NULLTerm(Term2);  /* stellt das Ende des Terms dar */
	teilterm = TO_ErsterTeilterm(Term2);

	retWert = TRUE;
	/* - a priori -- s >lpo t_j fuer _alle_ j ---------------------------------------- retWert = TRUE ------*/
	/* es sei denn in der Schleife wird bei _einem_ Vergleich retWert = FALSE gesetzt */

	/* gehe durch alle Teilterme t_j */
	while(teilterm != NULLt) { /* bis wir am Ende von t angekommen sind */
	
	  if (!(s_LPOGroesser (Term1, teilterm))) 
	    {
	      retWert = FALSE; /* weil fuer _alle_ : sobald einer nicht: --------------------> retWert = FALSE -*/
	      break; /* keine weiteren Vergleiche nötig */
	    }
	  
	  teilterm = TO_NaechsterTeilterm(teilterm);
	}
      }
    else
      {
	retWert = FALSE; /* hier gilt: f !> g    also nicht der Beta-Fall -------------------- retWert = FALSE -*/
      }
  }
  CLOSETRACE(("s_LPOBeta: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}



/* -------------------------------------------------------------------------------------------------------------*/
/* ----------------------- Test fuer Gamma-Fall: f = g ---------------------------------------------------------*/
/* ----------------------------------------- und s_1, ... , s_m >*lpo t_1, ... , t_n ---------------------------*/
/* ----------------------------------------- und s >lpo t_j fuer _alle_ j --------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/


BOOLEAN s_LPOGamma(UTermT Term1, UTermT Term2) {


  BOOLEAN retWert = FALSE;
  BOOLEAN lexGroesser = FALSE;
  BOOLEAN einerNicht = FALSE;


  /* OPTIMIERT hier nur Deklaration, Zuweisung erst im If-Zweig in dem Variablen verwendet werden */
  UTermT NULLtT1, NULLtT2, teiltermT1, teiltermT2;

  /* s oder t ist eine Variable --------------------------------------------------------------------------------*/
  if(TO_TermIstVar(Term1) || TO_TermIstVar(Term2)) {
    OPENTRACE(("s_LPOGamma: Term1 = %t,  Term2 = %t\n", Term1, Term2));
    {
      retWert = FALSE;
      /* ist s oder t eine Variable, dann ist der Gamma-Fall nicht zustaendig --------------- retWert = FALSE --*/
    }
    CLOSETRACE(("s_LPOGamma: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
    return retWert;
  }
  /* -----------------------------------------------------------------------------------------------------------*/


  /* ---------------- Vergleiche die Topsymbole anhand der Preazedenz miteinander ------------------------------*/
  OPENTRACE(("s_LPOGamma: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    if(PR_Praezedenz(TO_TopSymbol(Term1), TO_TopSymbol(Term2)) == Gleich)
      { 
	/* ------------ hier gilt: f = g, also teste zunaechst ob s_1, ... , s_n >*lpo t_1, ... , t_n ----------*/
	/* ------------ falls ja, teste ob s >lpo t_j fuer _alle_ j --------------------------------------------*/

	/* OPTIMIERT Zuweisung erst hier (s.o.) */
	NULLtT1 = TO_NULLTerm(Term1);  /* stellt das Ende des Terms s dar */
	NULLtT2 = TO_NULLTerm(Term2);  /* stellt das Ende des Terms t dar */
	
	teiltermT1 = TO_ErsterTeilterm(Term1);
	teiltermT2 = TO_ErsterTeilterm(Term2);


	/* Sobald ein Teilterm von Term1 groesser als derjenige von Term2 ist, ... */
	/* wird geprueft, ob s >lpo t_j fuer _alle_ j, falls ja, wird TRUE returniert */
	/* Sobald ein Teilterm von Term1 nicht gleich und nicht groesser als derjenige von Term2 ist, ... */
	/* wird FALSE returniert */

	/* Schleifeninvariante: alle bisherigen Teilterme von Term1 sind gleich denen von Term2 */
	while((teiltermT1 != NULLtT1) && (teiltermT2 != NULLtT2)) 
	  /* bis wir am Ende von t oder s angekommen sind */
	  /* eigentlich kann man an dieser Stelle davon ausgehen, dass die Terme gleich viele Teilterme haben */
	  /* dies gilt, weil die TopSymbole gleich sind (-> Termstruktur) */
	  {
	    if(s_LPOGroesser(teiltermT1, teiltermT2))
	      {
		/* hier gilt: teiltermT1 >lpo teiltermT2  also teste ob s >lpo t_j fuer _alle_ j ---------------*/
		/* dafuer wird die while-Schleife mit BREAK beendet */
		
		lexGroesser = TRUE; /* Damit unten festgestellt werden kann, weshalb gebreaked wurde, ... */
		/* das heisst, ob ein teiltermT1 >lpo teiltermT2 */
		/* in diesem Fall wird naemlich getestet ob s >lpo t_j fuer _alle_ j */
		/* ansonsten wird FALSE returniert */
		break;
	      }
	    else if(!(TO_TermeGleich(teiltermT1, teiltermT2)))
	      {
		retWert = FALSE; /* hier gilt: teiltermT1 !>lpo teiltermT2 und teiltermT1 != teiltermT2 --------*/
		/* ---------------------------------------------------------------------------- retWert = FALSE */
		break; /* keine weiteren Vergleiche noetig */
	      }

	    /* hier gilt: teiltermT1 !>lpo teiltermT2 und teiltermT1=teiltermT2 -> lexikographisch weiterpruefen*/
	    teiltermT1 = TO_NaechsterTeilterm(teiltermT1);
	    teiltermT2 = TO_NaechsterTeilterm(teiltermT2);
	  }

	/* Wenn wir die Schleife _ohne_ BREAK verlassen, dann sind die Terme gleich! */


	if(lexGroesser == TRUE)
	  {
	    /* hier gilt: die obere while-Schleife wurde verlassen, weil ein teiltermT1 >lpo teiltermT2, ... */
	    /* also teste ob s >lpo t_j fuer _alle_ j */

	    teiltermT2 = TO_ErsterTeilterm(Term2);

	    while((teiltermT2 != NULLtT2)) /* bis an das Ende von Term2 */
	      {
		if(!(s_LPOGroesser(Term1,teiltermT2)))
		  {
		    retWert = FALSE; /* weil 'fuer _alle_' : sobald einer nicht: --------> retWert = FALSE -*/
		    einerNicht = TRUE;
		    break; /* keine weiteren Vergleiche noetig */
		  }
		teiltermT2 = TO_NaechsterTeilterm(teiltermT2);
	      }
	    
	    if(einerNicht == FALSE)
	      {
		/* hier wissen wir, dass die obige While-Schleife _nicht_ mit break verlassen wurde. */
		/* also hier gilt: ein teiltermT1 >lpo teiltermT2 und  s >lpo t_j fuer _alle_ j  */
		/* --------------------------------------------------------------------------- retWert = TRUE --*/
		retWert = TRUE; 
	      }
	  } /* endif (lexGroesser == TRUE) */
	else 
	  {
	    retWert = FALSE; /* hier gilt:             s !> t ----------------------------- retWert = FALSE ---*/
	  }
      }
    else
      {
	retWert = FALSE; /* hier gilt: f != g     also nicht der GAMMA-Fall ---------------- retWert = FALSE ---*/
      }
  }
  CLOSETRACE(("s_LPOGamma: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}




/* -------------------------------------------------------------------------------------------------------------*/
/* ------------------ Test fuer Delta-Fall: x element Var(t) und t != x ----------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN s_LPODelta(UTermT Term1, UTermT Term2) {

  BOOLEAN retWert = FALSE;

  OPENTRACE(("s_LPODelta: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    if(TO_TermIstVar(Term2))
      {
	if(TO_SymbolEnthalten (TO_TopSymbol(Term2), Term1))
	  {
	    if(!TO_TermeGleich(Term1, Term2))
	      {
		retWert = TRUE; /* ------------ Der Delta-Fall schlaegt zu ----------------- retWert = TRUE ----*/
	      }
	  }
      }
    else /* hier gilt: Term2 ist keine Variable */
      {
	retWert = FALSE; /* Delta-Fall nicht zustaendig ------------------------------------ retWert = FALSE ---*/
      }
  }
  CLOSETRACE(("s_LPODelta: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}




/* -------------------------------------------------------------------------------------------------------------*/
/* -------------- Kombination von TO_TermeGleich und s_LPOGroesser ---------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN s_LPOGroesserGleich(UTermT Term1, UTermT Term2)
{
  BOOLEAN retWert = FALSE; /* wenn beide ifs zu FALSE ausgewertet werden gilt: t != s und t !> s */

  OPENTRACE(("s_LPOGroesserGleich: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    if(TO_TermeGleich(Term1, Term2))  /* aus t = s folgt t >= s */
      {
	retWert = TRUE; 
      }
    else if(s_LPOGroesser(Term1, Term2))  /* aus t > s folgt t >= s */
      {
	retWert = TRUE;
      }
  }
  CLOSETRACE(("s_LPOGroesserGleich: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}



#endif /* welcheVariante == 2 */ 














#if welcheVariante == 3
/* ==========================================================================================================
   3 = Transfer von positivem Wissen im gamma-Fall (zusaetzlich)
   ========================================================================================================== */

/* -------------------------------------------------------------------------------------------------------------*/
/* ------------------- Funktions-Deklarationen fuer LPO --------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

VergleichsT S_LPO(UTermT Term1, UTermT Term2);

BOOLEAN S_LPOGroesser(UTermT Term1, UTermT Term2); /* nur Wrapper fuer klein s_LPOGroesser */
/* Intern werden nur die Prozeduren mit kleinem s benutzt. */

static BOOLEAN s_LPOGroesser(UTermT Term1, UTermT Term2);
static BOOLEAN s_LPOGroesserGleich(UTermT Term1, UTermT Term2);

static BOOLEAN s_LPOAlpha(UTermT Term1, UTermT Term2);
static BOOLEAN s_LPOGamma(UTermT Term1, UTermT Term2);
static BOOLEAN s_LPOBeta(UTermT Term1, UTermT Term2);
static BOOLEAN s_LPODelta(UTermT Term1, UTermT Term2);


/* -------------------------------------------------------------------------------------------------------------*/
/* ------------------- S_LPO ------ Wird von Aussen benutzt ! --------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

VergleichsT S_LPO(UTermT Term1, UTermT Term2)
{
  VergleichsT retWert = Unvergleichbar;

  OPENTRACE(("S_LPO: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    
    /* OPTIMIERT TO_TermeGleich zuerst getestet */
 
    if(TO_TermeGleich(Term1, Term2))
      {
	retWert = Gleich;
      }
    else if(s_LPOGroesser(Term2, Term1))
      {
	retWert = Kleiner;
      }
    else if(s_LPOGroesser(Term1, Term2))
      {
	retWert = Groesser;
      }

    /* retWert wird mit Unvergleichbar initialisiert */    

  }
  /* %v VergleichsT */
  CLOSETRACE(("S_LPO: Term1 = %t,  Term2 = %t --> %v\n", Term1, Term2, retWert));
  return retWert;
}




/* -------------------------------------------------------------------------------------------------------------*/
/* --- S_LPOGroesser ---- Wird von Aussen benutzt ! ----- Wrapper-Prozedur fuer klein s_LPOGroesser ------------*/
/* -------------------------------------------------------------------------------------------------------------*/


BOOLEAN S_LPOGroesser(UTermT Term1, UTermT Term2)
{
  RekAufrufeLPOGroesserZaehler = 0;

  return s_LPOGroesser(Term1, Term2);
}


/* -------------------------------------------------------------------------------------------------------------*/
/* ------------------- Gehe die Faelle der LPO-Definition durch ------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN s_LPOGroesser(UTermT Term1, UTermT Term2)
{
  BOOLEAN retWert;

  RekAufrufeLPOGroesserZaehler++;


  /* Terme sind gleich! also s _nicht_ groesser als t ! --------------------------------------------------------*/
  /* Diesr Test ist notwendig, weil s_LPOGroesser auch direkt von Aussen benutzt wird (ueber S_LPOGroesser) ----*/
  if(TO_TermeGleich(Term1, Term2)) 
    {
      OPENTRACE(("s_LPOGroesser: Term1 = %t,  Term2 = %t\n", Term1, Term2));
      {
	  retWert = FALSE;
	  /* ---------------------------------------------------------------------------------- retWert = FALSE --*/
      }
      CLOSETRACE(("s_LPOGroesser: Term1 = %t,  Term2 = %t --> %b Terme gleich\n", Term1, Term2, retWert));
      return retWert;
    }
  /* -----------------------------------------------------------------------------------------------------------*/


  OPENTRACE(("s_LPOGroesser: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {


    if(s_LPOBeta(Term1, Term2)) 
      {                                              
	retWert = TRUE;
      }
    else if(s_LPOGamma(Term1, Term2)) 
      {						         
	retWert = TRUE;
      }
    else if(s_LPOAlpha(Term1, Term2))
      {
	retWert = TRUE;
      }  
    else if(s_LPODelta(Term1, Term2))
      {
	retWert = TRUE;
      }
    else
      {
	retWert = FALSE;
      }
  }
  CLOSETRACE(("s_LPOGroesser: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}

/* -------------------------------------------------------------------------------------------------------------*/
/* ---------------------- Test fuer ALPHA-Fall: s_i >=lpo t fuer _ein_ i ---------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN s_LPOAlpha(UTermT Term1, UTermT Term2) {


  BOOLEAN retWert = FALSE; /* wenn es in der Schleife nicht auf TRUE gesetzt wird, gilt FALSE */
  /* weil dann gilt: s_i >=lpo t fuer _kein_ i */

  OPENTRACE(("s_LPOAlpha: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
 
    UTermT NULLt = TO_NULLTerm(Term1);  /* stellt das Ende des Terms dar */
    UTermT teilterm = TO_ErsterTeilterm(Term1);


    /* gehe durch alle Teilterme s_i */
    while(teilterm != NULLt) { /* bis wir am Ende von s angekommen sind */
    
      if(s_LPOGroesserGleich(teilterm, Term2)) {
	retWert = TRUE; /* ---------- der ALPHA-Fall hat zugeschlagen ----------------------- retWert = TRUE ---*/
	break; /* keine weiteren Vergleiche nötig */
      }
    
      teilterm = TO_NaechsterTeilterm(teilterm);
    }
  }
  /* retWert wird mit FALSE initialisiert */    
  CLOSETRACE(("s_LPOAlpha: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}



/* -------------------------------------------------------------------------------------------------------------*/
/* ---------------------- Test fuer Beta-Fall: f > g und s >lpo t_j fuer _alle_ j ------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/


BOOLEAN s_LPOBeta(UTermT Term1, UTermT Term2) {

  BOOLEAN retWert = FALSE;

  /* OPTIMIERT hier nur Deklaration, Zuweisung erst im If-Zweig in dem Variablen verwendet werden */
  UTermT NULLt, teilterm;


  /* s oder t ist eine Variable --------------------------------------------------------------------------------*/
  if(TO_TermIstVar(Term1) || TO_TermIstVar(Term2)) {
    OPENTRACE(("s_LPOBeta: Term1 = %t,  Term2 = %t\n", Term1, Term2));
    {
      retWert = FALSE;
      /* ist s oder t eine Variable, dann ist der Beta-Fall nicht zustaendig ---------------- retWert = FALSE --*/
    }
    CLOSETRACE(("s_LPOBeta: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
    return retWert;
  }
  /* -----------------------------------------------------------------------------------------------------------*/


  OPENTRACE(("s_LPOBeta: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    /* ------------------ Vergleiche die Topsymbole anhand der Preazedenz miteinander --------------------------*/

    if(PR_Praezedenz(TO_TopSymbol(Term1), TO_TopSymbol(Term2)) == Groesser)
      {
	/* ---------------- hier gilt: f > g, also teste ob s >lpo t_j fuer _alle_ j ---------------------------*/

	/* OPTIMIERT Zuweisung erst hier (s.o.) */
	NULLt = TO_NULLTerm(Term2);  /* stellt das Ende des Terms dar */
	teilterm = TO_ErsterTeilterm(Term2);

	retWert = TRUE;
	/* - a priori -- s >lpo t_j fuer _alle_ j ---------------------------------------- retWert = TRUE ------*/
	/* es sei denn in der Schleife wird bei _einem_ Vergleich retWert = FALSE gesetzt */

	/* gehe durch alle Teilterme t_j */
	while(teilterm != NULLt) { /* bis wir am Ende von t angekommen sind */
	
	  if (!(s_LPOGroesser (Term1, teilterm))) 
	    {
	      retWert = FALSE; /* weil fuer _alle_ : sobald einer nicht: --------------------> retWert = FALSE -*/
	      break; /* keine weiteren Vergleiche nötig */
	    }
	  
	  teilterm = TO_NaechsterTeilterm(teilterm);
	}
      }
    else
      {
	retWert = FALSE; /* hier gilt: f !> g    also nicht der Beta-Fall -------------------- retWert = FALSE -*/
      }
  }
  CLOSETRACE(("s_LPOBeta: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}



/* -------------------------------------------------------------------------------------------------------------*/
/* ----------------------- Test fuer Gamma-Fall: f = g ---------------------------------------------------------*/
/* ----------------------------------------- und s_1, ... , s_m >*lpo t_1, ... , t_n ---------------------------*/
/* ----------------------------------------- und s >lpo t_j fuer _alle_ j --------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/


BOOLEAN s_LPOGamma(UTermT Term1, UTermT Term2) {


  BOOLEAN retWert = FALSE;
  BOOLEAN lexGroesser = FALSE;
  BOOLEAN einerNicht = FALSE;


  /* OPTIMIERT hier nur Deklaration, Zuweisung erst im If-Zweig in dem Variablen verwendet werden */
  UTermT NULLtT1, NULLtT2, teiltermT1, teiltermT2;

  /* s oder t ist eine Variable --------------------------------------------------------------------------------*/
  if(TO_TermIstVar(Term1) || TO_TermIstVar(Term2)) {
    OPENTRACE(("s_LPOGamma: Term1 = %t,  Term2 = %t\n", Term1, Term2));
    {
      retWert = FALSE;
      /* ist s oder t eine Variable, dann ist der Gamma-Fall nicht zustaendig --------------- retWert = FALSE --*/
    }
    CLOSETRACE(("s_LPOGamma: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
    return retWert;
  }
  /* -----------------------------------------------------------------------------------------------------------*/


  /* ---------------- Vergleiche die Topsymbole anhand der Preazedenz miteinander ------------------------------*/
  OPENTRACE(("s_LPOGamma: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    if(PR_Praezedenz(TO_TopSymbol(Term1), TO_TopSymbol(Term2)) == Gleich)
      { 
	/* ------------ hier gilt: f = g, also teste zunaechst ob s_1, ... , s_n >*lpo t_1, ... , t_n ----------*/
	/* ------------ falls ja, teste ob s >lpo t_j fuer _alle_ j --------------------------------------------*/

	/* OPTIMIERT Zuweisung erst hier (s.o.) */
	NULLtT1 = TO_NULLTerm(Term1);  /* stellt das Ende des Terms s dar */
	NULLtT2 = TO_NULLTerm(Term2);  /* stellt das Ende des Terms t dar */
	
	teiltermT1 = TO_ErsterTeilterm(Term1);
	teiltermT2 = TO_ErsterTeilterm(Term2);


	/* Sobald ein Teilterm von Term1 groesser als derjenige von Term2 ist, ... */
	/* wird geprueft, ob s >lpo t_j fuer _alle_ j, falls ja, wird TRUE returniert */
	/* Sobald ein Teilterm von Term1 nicht gleich und nicht groesser als derjenige von Term2 ist, ... */
	/* wird FALSE returniert */

	/* Schleifeninvariante: alle bisherigen Teilterme von Term1 sind gleich denen von Term2 */
	while((teiltermT1 != NULLtT1) && (teiltermT2 != NULLtT2)) 
	  /* bis wir am Ende von t oder s angekommen sind */
	  /* eigentlich kann man an dieser Stelle davon ausgehen, dass die Terme gleich viele Teilterme haben */
	  /* dies gilt, weil die TopSymbole gleich sind (-> Termstruktur) */
	  {
	    if(s_LPOGroesser(teiltermT1, teiltermT2))
	      {
		/* hier gilt: teiltermT1 >lpo teiltermT2  also teste ob s >lpo t_j fuer _alle_ j ---------------*/
		/* dafuer wird die while-Schleife mit BREAK beendet */
		
		lexGroesser = TRUE; /* Damit unten festgestellt werden kann, weshalb gebreaked wurde, ... */
		/* das heisst, ob ein teiltermT1 >lpo teiltermT2 */
		/* in diesem Fall wird naemlich getestet ob s >lpo t_j fuer _alle_ j (nicht fuer alle, s.u.)*/
		/* ansonsten wird FALSE returniert */
		break;
	      }
	    else if(!(TO_TermeGleich(teiltermT1, teiltermT2)))
	      {
		retWert = FALSE; /* hier gilt: teiltermT1 !>lpo teiltermT2 und teiltermT1 != teiltermT2 --------*/
		/* ---------------------------------------------------------------------------- retWert = FALSE */
		break; /* keine weiteren Vergleiche noetig */
	      }

	    /* hier gilt: teiltermT1 !>lpo teiltermT2 und teiltermT1=teiltermT2 -> lexikographisch weiterpruefen*/
	    teiltermT1 = TO_NaechsterTeilterm(teiltermT1);
	    teiltermT2 = TO_NaechsterTeilterm(teiltermT2);
	  }

	/* Wenn wir die Schleife _ohne_ BREAK verlassen, dann sind die Terme gleich! */


	if(lexGroesser == TRUE)
	  {
	    /* hier gilt: die obere while-Schleife wurde verlassen, weil ein teiltermT1 >lpo teiltermT2, ... */
	    /* also teste ob s >lpo t_j fuer _alle_ j (nicht alle, s.u.)*/

	    /* OPTIMIERT wir muessen nicht mit t von vorne anfangen, weil wir an dieser Stelle wissen, ... */
	    /* dass alle bisherigen Teilterme von Term1 gleich denen von Term2 sind, ... */
	    /* damit ist Term1 >LPO als die bisherigen t_j (diese gleichen ja seinen eigenen Teiltermen!) */

	    /* also mache weiter mit test ob: Term1 >lpo t_i+1 */
	    teiltermT2 = TO_NaechsterTeilterm(teiltermT2);

	    while((teiltermT2 != NULLtT2)) /* bis an das Ende von Term2 */
	      {
		if(!(s_LPOGroesser(Term1,teiltermT2)))
		  {
		    retWert = FALSE; /* weil 'fuer _alle_' : sobald einer nicht: --------> retWert = FALSE -*/
		    einerNicht = TRUE;
		    break; /* keine weiteren Vergleiche noetig */
		  }
		teiltermT2 = TO_NaechsterTeilterm(teiltermT2);
	      }
	    
	    if(einerNicht == FALSE)
	      {
		/* hier wissen wir, dass die obige While-Schleife _nicht_ mit break verlassen wurde. */
		/* also hier gilt: ein teiltermT1 >lpo teiltermT2 und  s >lpo t_j fuer _alle_ j  */
		/* --------------------------------------------------------------------------- retWert = TRUE --*/
		retWert = TRUE; 
	      }
	  } /* endif (lexGroesser == TRUE) */
	else 
	  {
	    retWert = FALSE; /* hier gilt:             s !> t ----------------------------- retWert = FALSE ---*/
	  }
      }
    else
      {
	retWert = FALSE; /* hier gilt: f != g     also nicht der GAMMA-Fall ---------------- retWert = FALSE ---*/
      }
  }
  CLOSETRACE(("s_LPOGamma: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}




/* -------------------------------------------------------------------------------------------------------------*/
/* ------------------ Test fuer Delta-Fall: x element Var(t) und t != x ----------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN s_LPODelta(UTermT Term1, UTermT Term2) {

  BOOLEAN retWert = FALSE;

  OPENTRACE(("s_LPODelta: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    if(TO_TermIstVar(Term2))
      {
	if(TO_SymbolEnthalten (TO_TopSymbol(Term2), Term1))
	  {
	    if(!TO_TermeGleich(Term1, Term2))
	      {
		retWert = TRUE; /* ------------ Der Delta-Fall schlaegt zu ----------------- retWert = TRUE ----*/
	      }
	  }
      }
    else /* hier gilt: Term2 ist keine Variable */
      {
	retWert = FALSE; /* Delta-Fall nicht zustaendig ------------------------------------ retWert = FALSE ---*/
      }
  }
  CLOSETRACE(("s_LPODelta: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}




/* -------------------------------------------------------------------------------------------------------------*/
/* -------------- Kombination von TO_TermeGleich und s_LPOGroesser ---------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN s_LPOGroesserGleich(UTermT Term1, UTermT Term2)
{
  BOOLEAN retWert = FALSE; /* wenn beide ifs zu FALSE ausgewertet werden gilt: t != s und t !> s */

  OPENTRACE(("s_LPOGroesserGleich: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    if(TO_TermeGleich(Term1, Term2))  /* aus t = s folgt t >= s */
      {
	retWert = TRUE; 
      }
    else if(s_LPOGroesser(Term1, Term2))  /* aus t > s folgt t >= s */
      {
	retWert = TRUE;
      }
  }
  CLOSETRACE(("s_LPOGroesserGleich: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}

#endif /* welcheVariante == 3 */













#if welcheVariante == 4
/* ==========================================================================================================
   4 = Transfer von neagativem Wissen in beta/gamma-Fall (zusaetzlich)
   ========================================================================================================== */


/* -------------------------------------------------------------------------------------------------------------*/
/* ------------------- Funktions-Deklarationen fuer LPO --------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

VergleichsT S_LPO(UTermT Term1, UTermT Term2);

BOOLEAN S_LPOGroesser(UTermT Term1, UTermT Term2); /* nur Wrapper fuer klein s_LPOGroesser */
/* Intern werden nur die Prozeduren mit kleinem s benutzt. */

static BOOLEAN s_LPOGroesser(UTermT Term1, UTermT Term2);
static BOOLEAN s_LPOGroesserGleich(UTermT Term1, UTermT Term2);

static BOOLEAN s_LPOAlpha(UTermT Term1, UTermT Term2, 
			  UTermT LexVergleichGescheitertDirektVorDiesemTeiltermVonS);
static BOOLEAN s_LPOGamma(UTermT Term1, UTermT Term2,
			  UTermT *LexVergleichGescheitertDirektVorDiesemTeiltermVonS,
			  BOOLEAN *MajorisierungInGammaGescheitert);
static BOOLEAN s_LPOBeta(UTermT Term1, UTermT Term2,
			 BOOLEAN *MajorisierungInBetaGescheitert);
static BOOLEAN s_LPODelta(UTermT Term1, UTermT Term2);
/* -------------------------------------------------------------------------------------------------------------*/
/* ------------------- S_LPO ------ Wird von Aussen benutzt ! --------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

VergleichsT S_LPO(UTermT Term1, UTermT Term2)
{
  VergleichsT retWert = Unvergleichbar;

  OPENTRACE(("S_LPO: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {

    
    /* OPTIMIERT TO_TermeGleich zuerst getestet */
    
    if(TO_TermeGleich(Term1, Term2))
      {
	retWert = Gleich;
      }
    else if(s_LPOGroesser(Term2, Term1))
      {
	retWert = Kleiner;
      }
    else if(s_LPOGroesser(Term1, Term2))
      {
	retWert = Groesser;
      }

    /* retWert wird mit Unvergleichbar initialisiert */    

  }
  /* %v VergleichsT */
  CLOSETRACE(("S_LPO: Term1 = %t,  Term2 = %t --> %v\n", Term1, Term2, retWert));
  return retWert;
}

/* -------------------------------------------------------------------------------------------------------------*/
/* --- S_LPOGroesser ---- Wird von Aussen benutzt ! ----- Wrapper-Prozedur fuer klein s_LPOGroesser ------------*/
/* -------------------------------------------------------------------------------------------------------------*/


BOOLEAN S_LPOGroesser(UTermT Term1, UTermT Term2)
{
  RekAufrufeLPOGroesserZaehler = 0;

  return s_LPOGroesser(Term1, Term2);
}



/* -------------------------------------------------------------------------------------------------------------*/
/* ------------------- Gehe die Faelle der LPO-Definition durch ------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN s_LPOGroesser(UTermT Term1, UTermT Term2)
{
  BOOLEAN retWert;

  UTermT LexVergleichGescheitertDirektVorDiesemTeiltermVonS = TO_ErsterTeilterm(Term1);
  BOOLEAN MajorisierungInGammaGescheitert = FALSE;
  BOOLEAN MajorisierungInBetaGescheitert = FALSE;

  RekAufrufeLPOGroesserZaehler++;

  /* Terme sind gleich! also s _nicht_ groesser als t ! --------------------------------------------------------*/
  /* Diesr Test ist notwendig, weil s_LPOGroesser auch direkt von Aussen benutzt wird (ueber S_LPOGroesser) ----*/
  if(TO_TermeGleich(Term1, Term2)) 
    {
      OPENTRACE(("s_LPOGroesser: Term1 = %t,  Term2 = %t\n", Term1, Term2));
      {
	retWert = FALSE;
	/* ---------------------------------------------------------------------------------- retWert = FALSE --*/
      }
      CLOSETRACE(("s_LPOGroesser: Term1 = %t,  Term2 = %t --> %b Terme gleich\n", Term1, Term2, retWert));
      return retWert;
    }
  /* -----------------------------------------------------------------------------------------------------------*/
  
  
  OPENTRACE(("s_LPOGroesser: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
      
    /* OPTIMIERT Delta mit Alpha getauscht */
  
    if(s_LPODelta(Term1, Term2))
      {
	retWert = TRUE;
      }
    else if(s_LPOBeta(Term1, Term2,
		      &MajorisierungInBetaGescheitert)) 
      {                                              
	retWert = TRUE;
      }
    else if((!MajorisierungInBetaGescheitert) && (s_LPOGamma(Term1, Term2,                  
							     &LexVergleichGescheitertDirektVorDiesemTeiltermVonS,
							     &MajorisierungInGammaGescheitert)))
      /* ist die Majorisierung in Beta gescheitert, ... */
      /* kann der Gamma-Fall keinen Erfolg haben */
      {
	retWert = TRUE;
      }
    else if(!(MajorisierungInGammaGescheitert || MajorisierungInBetaGescheitert)
	    && s_LPOAlpha(Term1, Term2,
			  LexVergleichGescheitertDirektVorDiesemTeiltermVonS))
      /* ist die Majorisierung in Gamma oder Beta gescheitert, ... */
      /* kann der Alpha-Fall keinen Erfolg haben */
      {
	retWert = TRUE;
      }
    else
      {
	retWert = FALSE;
      }
  }
  CLOSETRACE(("s_LPOGroesser: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}


/* -------------------------------------------------------------------------------------------------------------*/
/* ---------------------- Test fuer ALPHA-Fall: s_i >=lpo t fuer _ein_ i ---------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN s_LPOAlpha(UTermT Term1, UTermT Term2, 
		   UTermT LexVergleichGescheitertDirektVorDiesemTeiltermVonS) {

  BOOLEAN retWert = FALSE; /* wenn es in der Schleife nicht auf TRUE gesetzt wird, gilt FALSE */
  /* weil dann gilt: s_i >=lpo t fuer _kein_ i */
  
  OPENTRACE(("s_LPOAlpha: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
 
    UTermT NULLt = TO_NULLTerm(Term1);  /* stellt das Ende des Terms dar */
  
    /* Dieses Wissen haben wir aus dem Gamma-Fall */
    UTermT teilterm = LexVergleichGescheitertDirektVorDiesemTeiltermVonS;


    /* gehe durch alle (oder, wenn GammaUndBetaHelfenAlpha == 1 erst ab einem bestimmten) Teilterm(e) s_i */
    while(teilterm != NULLt) { /* bis wir am Ende von s angekommen sind */
    
      if(s_LPOGroesserGleich(teilterm, Term2)) {
	retWert = TRUE; /* ---------- der ALPHA-Fall hat zugeschlagen ----------------------- retWert = TRUE ---*/
	break; /* keine weiteren Vergleiche nötig */
      }
    
      teilterm = TO_NaechsterTeilterm(teilterm);
    }
  }
  /* retWert wird mit FALSE initialisiert */    
  CLOSETRACE(("s_LPOAlpha: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}


/* -------------------------------------------------------------------------------------------------------------*/
/* ---------------------- Test fuer Beta-Fall: f > g und s >lpo t_j fuer _alle_ j ------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN s_LPOBeta(UTermT Term1, UTermT Term2, 
		  BOOLEAN *MajorisierungInBetaGescheitert) {


  BOOLEAN retWert = FALSE;

  /* OPTIMIERT hier nur Deklaration, Zuweisung erst im If-Zweig in dem Variablen verwendet werden */
  UTermT NULLt, teilterm;

  *MajorisierungInBetaGescheitert = FALSE;


  /* s oder t ist eine Variable --------------------------------------------------------------------------------*/
  if(TO_TermIstVar(Term1) || TO_TermIstVar(Term2)) {
    OPENTRACE(("s_LPOBeta: Term1 = %t,  Term2 = %t\n", Term1, Term2));
    {
      retWert = FALSE;
      /* ist s oder t eine Variable, dann ist der Beta-Fall nicht zustaendig ---------------- retWert = FALSE --*/
    }
    CLOSETRACE(("s_LPOBeta: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
    return retWert;
  }
  /* -----------------------------------------------------------------------------------------------------------*/



  OPENTRACE(("s_LPOBeta: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    /* ------------------ Vergleiche die Topsymbole anhand der Preazedenz miteinander --------------------------*/

    if(PR_Praezedenz(TO_TopSymbol(Term1), TO_TopSymbol(Term2)) == Groesser)
      {
	/* ---------------- hier gilt: f > g, also teste ob s >lpo t_j fuer _alle_ j ---------------------------*/

	/* OPTIMIERT Zuweisung erst hier (s.o.) */
	NULLt = TO_NULLTerm(Term2);  /* stellt das Ende des Terms dar */
	teilterm = TO_ErsterTeilterm(Term2);

	retWert = TRUE;
	/* - a priori -- s >lpo t_j fuer _alle_ j ---------------------------------------- retWert = TRUE ------*/
	/* es sei denn in der Schleife wird bei _einem_ Vergleich retWert = FALSE gesetzt */

	/* gehe durch alle Teilterme t_j */
	while(teilterm != NULLt) { /* bis wir am Ende von t angekommen sind */
	
	  if (!(s_LPOGroesser (Term1, teilterm))) 
	    {
	      *MajorisierungInBetaGescheitert = TRUE;

	      retWert = FALSE; /* weil fuer _alle_ : sobald einer nicht: --------------------> retWert = FALSE -*/
	      break; /* keine weiteren Vergleiche nötig */
	    }
	  
	  teilterm = TO_NaechsterTeilterm(teilterm);
	}
      }
    else
      {
	retWert = FALSE; /* hier gilt: f !> g    also nicht der Beta-Fall -------------------- retWert = FALSE -*/
      }
  }
  CLOSETRACE(("s_LPOBeta: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}




/* -------------------------------------------------------------------------------------------------------------*/
/* ----------------------- Test fuer Gamma-Fall: f = g ---------------------------------------------------------*/
/* ----------------------------------------- und s_1, ... , s_m >*lpo t_1, ... , t_n ---------------------------*/
/* ----------------------------------------- und s >lpo t_j fuer _alle_ j --------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN s_LPOGamma(UTermT Term1, UTermT Term2,
		   UTermT *LexVergleichGescheitertDirektVorDiesemTeiltermVonS,
		   BOOLEAN *MajorisierungInGammaGescheitert) {
 

  BOOLEAN retWert = FALSE;
  BOOLEAN lexGroesser = FALSE;
  BOOLEAN einerNicht = FALSE;


  UTermT NULLtT1, NULLtT2, teiltermT1, teiltermT2;


  *MajorisierungInGammaGescheitert = FALSE;


  
  /* s oder t ist eine Variable --------------------------------------------------------------------------------*/
  if(TO_TermIstVar(Term1) || TO_TermIstVar(Term2)) {
    OPENTRACE(("s_LPOGamma: Term1 = %t,  Term2 = %t\n", Term1, Term2));
    {
      retWert = FALSE;
      /* ist s oder t eine Variable, dann ist der Gamma-Fall nicht zustaendig --------------- retWert = FALSE --*/
    }
    CLOSETRACE(("s_LPOGamma: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
    return retWert;
  }
  /* -----------------------------------------------------------------------------------------------------------*/



  /* ---------------- Vergleiche die Topsymbole anhand der Preazedenz miteinander ------------------------------*/
  OPENTRACE(("s_LPOGamma: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    if(PR_Praezedenz(TO_TopSymbol(Term1), TO_TopSymbol(Term2)) == Gleich)
      { 
	/* ------------ hier gilt: f = g, also teste zunaechst ob s_1, ... , s_n >*lpo t_1, ... , t_n ----------*/
	/* ------------ falls ja, teste ob s >lpo t_j fuer _alle_ j --------------------------------------------*/

	/* OPTIMIERT Zuweisung erst hier (s.o.) */
	NULLtT1 = TO_NULLTerm(Term1);  /* stellt das Ende des Terms s dar */
	NULLtT2 = TO_NULLTerm(Term2);  /* stellt das Ende des Terms t dar */
	
	teiltermT1 = TO_ErsterTeilterm(Term1);
	teiltermT2 = TO_ErsterTeilterm(Term2);


	/* Sobald ein Teilterm von Term1 groesser als derjenige von Term2 ist, ... */
	/* wird geprueft, ob s >lpo t_j fuer _alle_ j, falls ja, wird TRUE returniert */
	/* Sobald ein Teilterm von Term1 nicht gleich und nicht groesser als derjenige von Term2 ist, ... */
	/* wird FALSE returniert */

	/* Schleifeninvariante: alle bisherigen Teilterme von Term1 sind gleich denen von Term2 */
	while((teiltermT1 != NULLtT1) && (teiltermT2 != NULLtT2)) 
	  /* bis wir am Ende von t oder s angekommen sind */
	  /* eigentlich kann man an dieser Stelle davon ausgehen, dass die Terme gleich viele Teilterme haben */
	  /* dies gilt, weil die TopSymbole gleich sind (-> Termstruktur) */
	  {
	    if(s_LPOGroesser(teiltermT1, teiltermT2))
	      {
		/* hier gilt: teiltermT1 >lpo teiltermT2  also teste ob s >lpo t_j fuer _alle_ j ---------------*/
		/* dafuer wird die while-Schleife mit BREAK beendet */
		
		lexGroesser = TRUE; /* Damit unten festgestellt werden kann, weshalb gebreaked wurde, ... */
		/* das heisst, ob ein teiltermT1 >lpo teiltermT2 */
		/* in diesem Fall wird naemlich getestet ob s >lpo t_j fuer _alle_ j */
		/* ansonsten wird FALSE returniert */
		break;

		/* zu GammaUndBetaHelfenAlpha: */
		/* wenn der Lex-Vergleich zuschlaegt,*/
		/* kann der Gamma-Fall nur noch in der Majorisierung scheitern */
		/* dann wird der Alpha-Fall sowieso ausgelassen */
		/* und LexVergleichGescheitertDirektVorDiesemTeiltermVonS wird nicht mehr angefasst */
	      }
	    else if(!(TO_TermeGleich(teiltermT1, teiltermT2)))
	      {
		retWert = FALSE; /* hier gilt: teiltermT1 !>lpo teiltermT2 und teiltermT1 != teiltermT2 --------*/
		/* ---------------------------------------------------------------------------- retWert = FALSE */

		/* Der Alpha-Fall kann mit dem _naechsten_ Teilterm weitermachen */
		*LexVergleichGescheitertDirektVorDiesemTeiltermVonS = TO_NaechsterTeilterm(teiltermT1);

		break; /* keine weiteren Vergleiche noetig */
	      }

	    /* hier gilt: teiltermT1 !>lpo teiltermT2 und teiltermT1=teiltermT2 -> lexikographisch weiterpruefen*/
	    teiltermT1 = TO_NaechsterTeilterm(teiltermT1);
	    teiltermT2 = TO_NaechsterTeilterm(teiltermT2);
	  }

	/* Wenn wir die Schleife _ohne_ BREAK verlassen, dann sind die Terme gleich! */


	if(lexGroesser == TRUE)
	  {
	    /* hier gilt: die obere while-Schleife wurde verlassen, weil ein teiltermT1 >lpo teiltermT2, ... */
	    /* also teste ob s >lpo t_j fuer _alle_ j */

	    /* OPTIMIERT wir muessen nicht mit t von vorne anfangen, weil wir an dieser Stelle wissen, ... */
	    /* dass alle bisherigen Teilterme von Term1 gleich denen von Term2 sind, ... */
	    /* damit ist Term1 >LPO als die bisherigen t_j (diese gleichen ja seinen eigenen Teiltermen!) */

	    /* also mache weiter mit test ob: Term1 >lpo t_i+1 */
	    teiltermT2 = TO_NaechsterTeilterm(teiltermT2);

	    while((teiltermT2 != NULLtT2)) /* bis an das Ende von Term2 */
	      {
		if(!(s_LPOGroesser(Term1,teiltermT2)))
		  {
		    retWert = FALSE; /* weil 'fuer _alle_' : sobald einer nicht: --------> retWert = FALSE -*/
		    einerNicht = TRUE;
		    *MajorisierungInGammaGescheitert = TRUE;

		    break; /* keine weiteren Vergleiche noetig */
		  }
		teiltermT2 = TO_NaechsterTeilterm(teiltermT2);
	      }
	    
	    if(einerNicht == FALSE)
	      {
		/* hier wissen wir, dass die obige While-Schleife _nicht_ mit break verlassen wurde. */
		/* also hier gilt: ein teiltermT1 >lpo teiltermT2 und  s >lpo t_j fuer _alle_ j  */
		/* --------------------------------------------------------------------------- retWert = TRUE --*/
		retWert = TRUE; 
	      }
	  } /* endif (lexGroesser == TRUE) */
	else 
	  {
	    retWert = FALSE; /* hier gilt:             s !> t ----------------------------- retWert = FALSE ---*/
	  }
      }
    else
      {
	retWert = FALSE; /* hier gilt: f != g     also nicht der GAMMA-Fall ---------------- retWert = FALSE ---*/
      }
  }
  CLOSETRACE(("s_LPOGamma: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}



/* -------------------------------------------------------------------------------------------------------------*/
/* ------------------ Test fuer Delta-Fall: x element Var(t) und t != x ----------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN s_LPODelta(UTermT Term1, UTermT Term2) {

  BOOLEAN retWert = FALSE;

  OPENTRACE(("s_LPODelta: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    if(TO_TermIstVar(Term2))
      {
	if(TO_SymbolEnthalten (TO_TopSymbol(Term2), Term1))
	  {
	    if(!TO_TermeGleich(Term1, Term2))
	      {
		retWert = TRUE; /* ------------ Der Delta-Fall schlaegt zu ----------------- retWert = TRUE ----*/
	      }
	  }
      }
    else /* hier gilt: Term2 ist keine Variable */
      {
	retWert = FALSE; /* Delta-Fall nicht zustaendig ------------------------------------ retWert = FALSE ---*/
      }
  }
  CLOSETRACE(("s_LPODelta: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}




/* -------------------------------------------------------------------------------------------------------------*/
/* -------------- Kombination von TO_TermeGleich und s_LPOGroesser ---------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN s_LPOGroesserGleich(UTermT Term1, UTermT Term2)
{
  BOOLEAN retWert = FALSE; /* wenn beide ifs zu FALSE ausgewertet werden gilt: t != s und t !> s */

  OPENTRACE(("s_LPOGroesserGleich: Term1 = %t,  Term2 = %t\n", Term1, Term2));
  {
    if(TO_TermeGleich(Term1, Term2))  /* aus t = s folgt t >= s */
      {
	retWert = TRUE; 
      }
    else if(s_LPOGroesser(Term1, Term2))  /* aus t > s folgt t >= s */
      {
	retWert = TRUE;
      }
  }
  CLOSETRACE(("s_LPOGroesserGleich: Term1 = %t,  Term2 = %t --> %b\n", Term1, Term2, retWert));
  return retWert;
}



#endif /* welcheVariante == 4 */ 
