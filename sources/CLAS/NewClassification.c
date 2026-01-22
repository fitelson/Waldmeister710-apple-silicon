/**********************************************************************
* Filename : NewClassification.
*
* Kurzbeschreibung : Classifikation von CPs und CGs
*
* Autoren : Andreas Jaeger
*
* 10.03.98: Erstellung
*
*
*
**********************************************************************/
#define __EXTENSIONS__ 1
#include "antiwarnings.h"
#include <string.h>

#include "compiler-extra.h"
#include "general.h"

#include "Ausgaben.h"
#include "ClasFunctions.h"
#include "ClasHeuristics.h"
#include "ClasPatch.h"
#include "Clas_CP_Goal.h"
#include "Hauptkomponenten.h"
#include "InfoStrings.h"
#include "Interreduktion.h"
#include "NewClassification.h"
#include "Ordnungen.h"
#include "RUndEVerwaltung.h"
#include "SymbolGewichte.h"
#include "TermOperationen.h"

#define AC_KONSISTENZ_BENUTZEN 0 /* 0 1 */

#if AC_KONSISTENZ_BENUTZEN
/* Zum Berechnen der transitiven Huelle. */ 
static void RoyWarshall (BOOLEAN *R, unsigned int n)
{
  unsigned int i,j,k;
  for (i=0; i<n; i++)
    for (j=0; j<n; j++)
      if (R[j*n+i])
	for (k=0; k<n; k++)
	  R[j*n+k] = R[j*n+k] || R[i*n+k];
}

static void RelationInit (BOOLEAN *R, unsigned int n)
{
  unsigned int i,j;
  for (i=0; i<n; i++)
    for (j=0; j<n; j++)
      R[i*n+j] = FALSE;
}

static unsigned int Diagonaleinsen (BOOLEAN *R, unsigned int n)
{
  unsigned int i,r=0;
  for (i=0; i<n; i++)
    if (R[i*n+i])
      r++;
  return r;
}
static void RelationSetzen(BOOLEAN *linksvon, unsigned int n, UTermT t);

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


static void ACTeiltermBehandlen(BOOLEAN *linksvon, unsigned int n, SymbolT f, UTermT t)
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

#if 0
static void ACTeiltermBehandlen(BOOLEAN *linksvon, unsigned int n, SymbolT f, UTermT t)
{
  SymbolT x = f;
  BOOLEAN letzteRunde = FALSE;
  for(;;) {
    UTermT t1 = !letzteRunde ? TO_ErsterTeilterm(t) : TO_ZweiterTeilterm(t);

    SymbolT y = TO_TopSymbol(t1);
    if (SO_SymbIstVar(y)){
      if (SO_SymbUngleich(x,f) && SO_SymbUngleich(x,y))
	linksvon[(SO_VarNummer(x)-1)*n+(SO_VarNummer(y)-1)] = TRUE;
      x = y;
    }
    else
      RelationSetzen(linksvon,n,t1);
    
    if (letzteRunde) return;

    if (SO_SymbGleich(f,TO_TopSymbol(TO_ZweiterTeilterm(t))))
      t = TO_ZweiterTeilterm(t);
    else
      letzteRunde = TRUE;
  } 
}
#endif

static void RelationSetzen(BOOLEAN *linksvon, unsigned int n, UTermT t)
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
static int ACKonsistenz (TermT lhs, TermT rhs)
{
  TermT l = TO_Termkopie(lhs);
  TermT r = TO_Termkopie(rhs);
  unsigned int n = CF_AnzahlVariablen(l,r);
  unsigned int res = 0;
  if (n>1){
    BOOLEAN *linksvon = array_alloc(n*n,BOOLEAN);
    RelationInit(linksvon,n);
    RelationSetzen(linksvon,n,l);
    RelationSetzen(linksvon,n,r);
    RoyWarshall(linksvon,n);
    res = (Diagonaleinsen(linksvon,n)*99) / n;
    array_dealloc(linksvon);
  }
  TO_TermeLoeschen(l,r);
  return res;
}

#endif

/* ------------------------------------------------------------------------*/
/* -------- globale Variablen (siehe Classification.h) --------------------*/
/* ------------------------------------------------------------------------*/


/* lokale, globale Vars */
/* fuer FIFO-Strategie: Einfacher Zaehler */
static long int CPNr;

CriterionType *ClasCriteria;
CriterionType *ReClasCriteria;
ClassifyProcT ClassifyProc;
ClassifyProcT CGClassifyProc;
static ClassifyProcT ReClassifyProc;

#if !COMP_POWER  
static struct {
  CPHistoryT crit;
  const char *name;
} NameContext[] = { /* Reihenfolge muss mit SUEBasis uebereinstimmen */
  {CPWithFather, "CPWithFather"},
  {CPWithMother, "CPWithMother"},
  {Initial, "Initial"},
  {KilledRE, "KilledRE"},
  {ChaseManhattanCP, "Database"},
  {TWreceived, "TWreceived"},
  {CPHistory_Last, "The End"}
};
#endif

static struct {
  CriteriaEnumType crit;
  const char *name;
  CriteriaProcType critProc;
} NameCriterion[] = {
  {Crit_EChild, "EChild", CF_EChildPraedikat},
  {Crit_KillerR, "KillerR", CF_KillerPraedikatR},
  {Crit_KillerRE, "KillerRE", CF_KillerPraedikatRE},
  {Crit_KillerE, "KillerE", CF_KillerPraedikatE},
  {Crit_LastMarker, "The End", NULL}
}
;

static struct 
{
  const char *name;
  ActionEnumType action;
} action_parse [] = {
    {"ultimate", Act_ultimate},
    {"normal", Act_normal},
    {"double", Act_double},
    {"half", Act_half},
    {"last", Act_never},
    {"", Act_LastMarker}
};


static struct 
{
  const char *name;
  HeuristicsEnumType heuristic;
  ClassifyProcT classProc;
} heuristics_parse [] = {
    {"addweight", Heu_AddWeight, &CH_AddWeight},
    {"cpingoal", Heu_CP_In_Goal, &CPinGoal},
    {"glam", Heu_GLAM, NULL},
    {"goalincp", Heu_Goal_In_CP, &GoalinCP},
    {"gtweight", Heu_GtWeight, &CH_GtWeight},
    {"Mixweight", Heu_MixWeight2, &CH_MixWeight2},
    {"mixweight2", Heu_MixWeight2, &CH_MixWeight2},
    {"mixweight", Heu_MixWeight, &CH_MixWeight},
    {"maxweight", Heu_MaxWeight, &CH_MaxWeight},
    {"ordweight", Heu_OrdWeight, &CH_OrdWeight},
    {"polyweight2", Heu_PolyWeight2, &CH_PolyWeight2},
    {"polyweight", Heu_PolyWeight, &CH_PolyWeight},
    {"unif", Heu_Unifikationsmass, &CH_Unifikationsmass},
    {"cppatch", Heu_CPPatch, &CPA_CPPatch},
    {"cgpatch", Heu_CGPatch, &CPA_CGPatch},
    {"fifo", Heu_Fifo, &CH_Fifo},
    {"Addweight", Heu_Golem, &CH_Golem},
    {"golem", Heu_Golem, &CH_Golem},
    {"", Heu_LastMarker, NULL}
};

static ContextType HistorySpec[CPHistory_Last] =
{ /* Reihenfolge muss mit SUEBasis uebereinstimmen */
  {CPWithFather, Act_normal},
  {CPWithMother, Act_normal},
  {Initial, Act_normal},
  {KilledRE, Act_normal},
  {ChaseManhattanCP, Act_normal},
  {TWreceived, Act_normal}
};


/* ------------------------------------------------------------------------------------------------*/
/* ------------ Classifikationen ------------------------------------------------------------------*/
/* ------------------------------------------------------------------------------------------------*/

/* Normale Classifikation von CPs mit Criterium */

void
C_Classify (WeightEntryT *wtPtr, TermT lhs, TermT rhs,
            RegelOderGleichungsT actualParent, RegelOderGleichungsT otherParent,
            UTermT actualPosition,
            CPHistoryT History)
{
  int i;
  BOOLEAN res = FALSE;
  ActionEnumType action;
  IdMitTermenS tids = {lhs,rhs,{0,0,0},TRUE};

  action = HistorySpec [History].action;

  wtPtr->w2 = ++CPNr; /* FIFO */
  
  switch (action)
    {
    case Act_ultimate:
      wtPtr->w1 = minimalWeight();
      return;
    case Act_never:
      wtPtr->w1 = maximalWeight();
      return;
    default:
      break;
    }

  if (ClasCriteria) {
    for (i = 0; ClasCriteria[i].criterion != Crit_LastMarker; i++)
      {
        res = ClasCriteria[i].critProc (lhs, rhs, actualParent, otherParent);
        if (res) {
          ClasCriteria[i].fullfilled++;
          if (ClasCriteria[i].action == Act_ultimate) {
            wtPtr->w1 = minimalWeight();
            return;
          }
          if (ClasCriteria[i].action == Act_never) {
            wtPtr->w1 = maximalWeight();
            return;
          }
          action = ClasCriteria[i].action;
          /* OK, normale Classifikation durchfuehren */
          break;
        }
      }
  }

  /* normale Classifikation */
  /* XXX actual und otherParent werden nicht übergeben */
  wtPtr->w1 = (*ClassifyProc) (&tids);

#if AC_KONSISTENZ_BENUTZEN
  wtPtr->w1 = wtPtr->w1 * 100 + ACKonsistenz(lhs,rhs);
#endif

  switch (action)
    {
    case Act_double:
      wtPtr->w1 = wtPtr->w1 * 2;
      break;
    case Act_half:
      wtPtr->w1 = wtPtr->w1 / 2;
      break;
    default:
      break;
    }
}



void
C_CG_Classify (WeightEntryT *wtPtr, TermT lhs, TermT rhs,
               RegelOderGleichungsT actualParent,
               RegelOderGleichungsT otherParent)
{
  IdMitTermenS tids = {lhs,rhs,{0,0,0},FALSE};

  /* normale Classifikation */
  wtPtr->w2 = ++CPNr; /* FIFO */
  wtPtr->w1 = (*CGClassifyProc) (&tids);

#if AC_KONSISTENZ_BENUTZEN
  wtPtr->w1 = wtPtr->w1 * 100 + ACKonsistenz(lhs,rhs);
#endif

}


void
C_ReClassify (WeightEntryT *wtPtr, TermT lhs, TermT rhs,
               RegelOderGleichungsT actualParent, RegelOderGleichungsT otherParent)
{
  int i;
  BOOLEAN res = FALSE;
  ActionEnumType action = Act_normal;
  IdMitTermenS tids = {lhs,rhs,{0,0,0},TRUE};

  /* w2 wird nicht geaendert */

  if (ReClasCriteria)
    {
      for (i = 0; ReClasCriteria[i].criterion != Crit_LastMarker; i++)
        {
          res = ReClasCriteria[i].critProc (lhs, rhs, actualParent, otherParent);
          if (res) {
            ReClasCriteria[i].fullfilled++;
            if (ReClasCriteria[i].action == Act_ultimate) {
              wtPtr->w1 = minimalWeight();
              return;
            }
            if (ReClasCriteria[i].action == Act_never) {
              wtPtr->w1 = maximalWeight();
              return;
            }
            action = ReClasCriteria[i].action;
            /* OK, normale Classifikation durchfuehren */
            break;
          }
        }
    }
  
  /* normale Classifikation */
  wtPtr->w1 = (*ReClassifyProc) (&tids);

#if AC_KONSISTENZ_BENUTZEN
  wtPtr->w1 = wtPtr->w1 * 100 + ACKonsistenz(lhs,rhs);
#endif

  if (res)
    {
      switch (action)
        {
        case Act_double:
          wtPtr->w1 = wtPtr->w1 * 2;
          break;
        case Act_half:
          wtPtr->w1 = wtPtr->w1 / 2;
          break;
        default:
          break;
        }
    }
}




/* ------------------------------------------------------------------------------------------------*/
/* ------------ ParamInfo -------------------------------------------------------------------------*/
/* ------------------------------------------------------------------------------------------------*/

#if !COMP_POWER
static const char *
get_action (ActionEnumType action)
{
  int i;

  for (i = 0; action_parse[i].action != Act_LastMarker; i++)
    {
      if (action_parse[i].action == action)
        return action_parse[i].name;
    }

  return "";
}
#endif

const char *
get_criterion (CriteriaEnumType singleCrit)
{
  int i;

  for (i = 0; NameCriterion[i].crit != Crit_LastMarker; i++)
    {
      if (NameCriterion[i].crit == singleCrit)
        return NameCriterion[i].name;
    }
  
  return "";
}

static CriteriaProcType
get_crit_proc (CriteriaEnumType singleCrit)
{
  int i;

  for (i = 0; NameCriterion[i].crit != Crit_LastMarker; i++)
    {
      if (NameCriterion[i].crit == singleCrit)
        return NameCriterion[i].critProc;
    }
  
  return NULL;
}


#if !COMP_POWER
static const char *
get_heuristic (HeuristicsEnumType heuristic)
{
  int i;

  for (i = 0; heuristics_parse[i].heuristic != Heu_LastMarker; i++)
    {
      if (heuristics_parse[i].heuristic == heuristic)
        return heuristics_parse[i].name;
    }
  return "";
}
#endif

static ClassifyProcT 
get_heuristicClassProc (HeuristicsEnumType heuristic)
{
  int i;

  for (i = 0; heuristics_parse[i].heuristic != Heu_LastMarker; i++)
    {
      if (heuristics_parse[i].heuristic == heuristic)
        return heuristics_parse[i].classProc;
    }
  return NULL;
}



#if !COMP_POWER
static void
print_options (CriterionType **Crit, HeuristicsEnumType heuristic,
               ContextType Cont[])
{
#if 0
  int i;

  if ((Crit && *Crit) || Cont)
    printf("  Criteria:\n");
  if (Crit && *Crit)
    {
      for (i = 0; (*Crit)[i].criterion != Crit_LastMarker; i++)
        {
          printf("    %s: %s\n", get_criterion ((*Crit)[i].criterion),
                   get_action ((*Crit)[i].action));
        }
    }
  if (Cont)
    {
      for (i = 0; i < CPHistory_Last; i++)
        {
          if (Cont[i].action != Act_normal)
            printf("    %s: %s\n", NameContext[i].name, get_action (Cont[i].action));
        }
    }
  printf("  Heuristic: %s\n", get_heuristic (heuristic));
#endif
}
#endif


static
BOOLEAN parse_heuristic (const char *str, HeuristicsEnumType *heuristic)
{
  int i;
  
  for (i = 0; heuristics_parse[i].heuristic != Heu_LastMarker; i++)
    {
      if (strncmp (str, heuristics_parse[i].name, strlen (heuristics_parse[i].name)) == 0)
        {
          *heuristic = heuristics_parse[i].heuristic;
          return TRUE;
        }
    }

  return FALSE;
}


static BOOLEAN
add_action (CriteriaEnumType singleCrit, const char *str, CriterionType **Crit)
{
  int i, j;
  BOOLEAN found = FALSE;

  if (Crit == NULL)
    our_error ("criteria are only allowed for critical pair (re)classification.");
  
  
  for (i = 0; action_parse [i].action != Act_LastMarker && !found; i++)
    {
      if (strncasecmp (str, action_parse[i].name, strlen (action_parse[i].name)) == 0)
        {
          found = TRUE;
          break;
        }
    }
  
  if (!found)
    return FALSE;
  
  if (*Crit  == NULL)
    {
      *Crit = our_alloc (sizeof (CriterionType) * 2);
      (*Crit)[0].criterion = singleCrit;
      (*Crit)[0].action = action_parse[i].action;
      (*Crit)[0].critProc = get_crit_proc (singleCrit);
      (*Crit)[1].criterion = Crit_LastMarker;
    }
  else {
    /* Kriterium hinten anhaengen bzw. alten Wert ueberschreiben */
    for (j = 0; (*Crit)[j].criterion != Crit_LastMarker; j++)
      {
        if ((*Crit)[j].criterion == singleCrit)
          {
            (*Crit)[j].action = action_parse[i].action;
            return TRUE;
          }
      }
    *Crit = our_realloc (*Crit, sizeof (CriterionType) * (j + 2), "Parsing criteria");
    (*Crit)[j].criterion = singleCrit;
    (*Crit)[j].fullfilled = 0;
    (*Crit)[j].action = action_parse[i].action;
    (*Crit)[j].critProc = get_crit_proc (singleCrit);
    (*Crit)[j+1].criterion = Crit_LastMarker;
  }
  
  return TRUE;
}


static BOOLEAN
add_action_context (CPHistoryT context, const char *str, ContextType Cont[])
{
  int i;
  BOOLEAN found = FALSE;

  if (Cont == NULL)
    our_error ("History only allowed for critical pair classification.");
  
  for (i = 0; action_parse [i].action != Act_LastMarker && !found; i++)
    {
      if (strncasecmp (str, action_parse[i].name, strlen (action_parse[i].name)) == 0)
        {
          found = TRUE;
          break;
        }
    }
  
  if (!found)
    return FALSE;

  Cont[context].action = action_parse[i].action;
  
  return TRUE;
}


static BOOLEAN
parse_str (char *options, CriterionType **Crit, HeuristicsEnumType *heuristic,
           ContextType Cont[],
           char **glam_file,
           WeightT *cigic_single_match, WeightT *cigic_no_match, WeightT *cigic_min_struct,
	   int *CPpatchVariante, int *CGpatchVariante)
{  
  char *token;
  BOOLEAN ok = TRUE;

  token = options;
  while (token != NULL && ok) {
    if (strncasecmp (token, "echild=", 7) == 0) {
      ok = add_action (Crit_EChild, &token[7], Crit);
    }
    else if (strncasecmp (token, "heuristic=", 10) == 0) {
      ok = parse_heuristic (&token[10], heuristic);
    }
    else if (strncasecmp (token, "initial=", 8) == 0) {
      ok = add_action_context (Initial, &token[8], Cont);
    }
    else if (strncasecmp (token, "killedRE=", 9) == 0) {
      ok = add_action_context (KilledRE, &token[9], Cont);
    }
    else if (strncasecmp (token, "killerE=", 8) == 0) {
      ok = add_action (Crit_KillerE, &token[8], Crit);
    }
    else if (strncasecmp (token, "killerRE=", 9) == 0) {
      ok = add_action (Crit_KillerRE, &token[9], Crit);
    }
    else if (strncasecmp (token, "killerR=", 7) == 0) {
      ok = add_action (Crit_KillerR, &token[7], Crit);
    }
    else if (strncasecmp (token, "cpgoal=", 7) == 0) {
      ok = (sscanf (&token[7], "%ld.%ld.%ld", 
                    cigic_single_match, cigic_no_match, cigic_min_struct) == 3);
    }
    else if (strncasecmp (token, "cppatchno=", 10) == 0) {
      *CPpatchVariante = atoi (&token[10]);
    }
    else if (strncasecmp (token, "cgpatchno=", 10) == 0) {
      *CGpatchVariante = atoi (&token[10]);
    }
    else {
      /* Funktionssymbol, wird ignoriert und an SymbolGewichte weitergereicht */
      /* statistics, statint wird an CStatistics weitergereicht */
    }
    
    token = strchr (token, ':');
    if (token != NULL) token++;
  }
  if (!ok) {
    our_error ("Unknown Options for classification specified.");
  }  
  return ok;
}



static
BOOLEAN parse_options (InfoStringT strDEF, InfoStringT strUSR,
                       CriterionType **Crit, ContextType Cont[],
                       HeuristicsEnumType defaultHeuristic,
                       HeuristicsEnumType *chosenHeuristic,
                       ClassifyProcT *ClassProc,
                       WeightT *cigic_single_match, WeightT *cigic_no_match, WeightT *cigic_min_struct,
		       int *CPpatchVariante, int *CGpatchVariante)
{
  BOOLEAN ok;
  HeuristicsEnumType heuristic;
  char *glam_file = NULL;
  
  heuristic = defaultHeuristic; /* Schmarrn; Default kommt aus Parameter.c; Th. */
  ok = parse_str (strDEF, Crit, &heuristic, Cont, &glam_file,
		  cigic_single_match, cigic_no_match, cigic_min_struct,
		  CPpatchVariante, CGpatchVariante);

  ok = ok && parse_str (strUSR, Crit, &heuristic, Cont, &glam_file,
                        cigic_single_match, cigic_no_match, cigic_min_struct,
			CPpatchVariante, CGpatchVariante);

  *chosenHeuristic = heuristic;

  *ClassProc = get_heuristicClassProc (heuristic);
  if (*ClassProc == NULL)
    our_error ("Heuristic not implemented yet");
  
#if !COMP_POWER  
  print_options (Crit, heuristic, Cont);
#endif
  
  return ok;
}


/* whichModule: 1 - CLAS
                2 - RECLAS
                3 - CGCLAS
*/
void C_ParamInfo(ParamInfoArtT what, int whichModule, 
                 InfoStringT clasDEF, InfoStringT clasUSR, 
                 InfoStringT reclasDEF, InfoStringT reclasUSR, 
                 InfoStringT cgDEF, InfoStringT cgUSR)
{
  CriterionType *Crit;
  ContextType Cont[CPHistory_Last];
  ClassifyProcT ClassProc;
  int i;
  WeightT cigic_single_match, cigic_no_match, cigic_min_struct;
  HeuristicsEnumType heuristic;
  int CPpatchVariante, CGpatchVariante;
  
  switch (what)
    {
    case ParamEvaluate:
      for (i = 0; i < CPHistory_Last; i++) {
        Cont[i].context = HistorySpec [i].context;
        Cont[i].action = Act_normal;
      }
  
      Crit = NULL;
      switch (whichModule)
        {
        case 1:
          printf("\n* Critical Pairs\n");
          parse_options (clasDEF, clasUSR, &Crit, Cont, Heu_MixWeight, &heuristic, &ClassProc,
                         &cigic_single_match, &cigic_no_match, &cigic_min_struct,
			 &CPpatchVariante, &CGpatchVariante);
          break;
        case 2:
          /*  CP-Re-Klassifikation */
          printf("\n* Critical Pairs Reclassification\n");
          parse_options (reclasDEF, reclasUSR, &Crit, NULL, Heu_MixWeight, &heuristic, &ClassProc,
                         &cigic_single_match, &cigic_no_match, &cigic_min_struct,
			 &CPpatchVariante, &CGpatchVariante);
          break;
        case 3:
          /* CG-Klassifikation */
          printf("\n* Critical Goals\n");
          parse_options (cgDEF, cgUSR, NULL, NULL, Heu_Unifikationsmass, &heuristic, &ClassProc,
                         &cigic_single_match, &cigic_no_match, &cigic_min_struct,
			 &CPpatchVariante, &CGpatchVariante);
          break;
        default:
          our_error ("only values 1-3 are allowed in C_ParamInfo");
          break; 
        }
      if (Crit != NULL)
        our_dealloc (Crit);
      break;
    case ParamExplain:
      switch (whichModule)
        {
        case 1:
          printf("Classification\n");
          printf("**************\n");
          printf("This parameter guides the assessment of critical pairs,\n");
          printf("i.e. the computation of one or more integers (w1/w2) that make up an ordering\n");
          printf("of the stored cps. This computation of basic weights can be influenced\n");
          printf("by assigning different weights to single symbols, or by some criteria.\n\n");
          break;
        case 2:
          printf("Re-Classification\n");
          printf("*****************\n");
          printf("This parameter guides the criteria to apply on critical pairs during the\n");
          printf("intermediate reprocessing of the CP-Set. The basic classifying weight is\n");
          printf("the same as given by the -clas option.\n\n");
          break;
        case 3:
          printf("CG-Classification\n");
          printf("*****************\n");
          printf("This parameter guides the classification of critical goals, and is similar to\n");
          printf("the -clas option. For critical goals, no criteria are available.\n\n");
          break;
        }
      if (whichModule < 3){
         printf("Criteria that can be applied to cps detect certain properties. If such a property\n");
         printf("has been detected, the cp will be prefered resp. defered as stated in <treatment>.\n");
         printf("\n");
         printf("<criterion>=<treatment>\n");
         printf("\n");
         printf("<criterion>=killerR|killerE|killerRE|echild|initial|killedRE|database\n");
         printf("<treatment>=ultimate|half|double|last\n");
         printf("\n");
         printf("killerR/E/RE  detects those cps that would remove a rule/equation via interreduction\n");
         printf("echild        detects cps a parent of which is an equation\n");
         printf("initial       detects cps from the initial specification\n");
         printf("killedRE      detects cps that arised from interreduction victims\n\n");
      }
      printf ("Heuristics can be specified with:\n"
              "heuristic=addweight|cpingoal|glam|goalincp|gtweight|mixweight|mixweight2|maxweight|ordweight|\n"
              "          unif|fifo|golem|cppatch|cgpatch\n");
      printf("  Please note that heuristics cpingoal and goalincp only work on critical pairs.\n");
      printf("  The heuristics cpingoal/goalincp are parametrised by:\n");
      printf("    cpgoal=single-match.no-match.min-struct\n");
      printf("  The heuristics cppatch/cgpatch are experimental and are parametrised by:\n");
      printf("    cppatchno=X:cgpatchno=Y\n");
      
      break;
    }
}


BOOLEAN C_InitAposteriori(InfoStringT clasDEF, InfoStringT clasUSR, 
                          InfoStringT reclasDEF, InfoStringT reclasUSR, 
                          InfoStringT cgDEF, InfoStringT cgUSR)
{
  WeightT cigic_single_match, cigic_no_match, cigic_min_struct;
  HeuristicsEnumType ClasHeuristic, ReClasHeuristic, CGHeuristic;
  int CGpatchVariante = 0;
  int CPpatchVariante = 0;

  CPNr = 0;
  cigic_single_match = 0;
  cigic_no_match = 0;
  cigic_min_struct = 0;
  

   
   SG_SymbGewichteEintragen (cgUSR, cgUSR, 0);
   SG_SymbGewichteEintragen (clasDEF, clasUSR, 1);

   /* normale CP-Klassifikation */

   ClasCriteria = NULL;           /* unten Schmarrn: Defaults kommen aus Parameter.c; Th. */
   parse_options (clasDEF, clasUSR, &ClasCriteria, HistorySpec, Heu_MixWeight, &ClasHeuristic, &ClassifyProc,
                  &cigic_single_match, &cigic_no_match, &cigic_min_struct,
		  &CPpatchVariante, &CGpatchVariante);
   
   switch (ClasHeuristic)
     {
     case Heu_CP_In_Goal:
     case Heu_Goal_In_CP:
       CIGICInit (cigic_single_match, cigic_no_match, cigic_min_struct);
       break;
      case Heu_CPPatch:
	CPA_CPInit (CPpatchVariante);
	break;
     case Heu_CGPatch:
	our_error ("This heuristic doesn't work on critical pairs!");
	break;
     case Heu_PolyWeight:
        CF_PolyWeightInitialisieren();
        break;
     default:
       break;
     }
   
   /*  CP-Re-Klassifikation */
   ReClasCriteria = NULL;
   parse_options (reclasDEF, reclasUSR, &ReClasCriteria, NULL, ClasHeuristic, &ReClasHeuristic, &ReClassifyProc,
                  &cigic_single_match, &cigic_no_match, &cigic_min_struct,
		  &CPpatchVariante, &CGpatchVariante);

   if (ClasHeuristic != ReClasHeuristic)
      our_error ("Classification and Reclassification must use the same heuristics!");
   
   /* CG-Klassifikation */
   parse_options (cgDEF, cgUSR, NULL, NULL, Heu_Unifikationsmass, &CGHeuristic, &CGClassifyProc,
                  &cigic_single_match, &cigic_no_match, &cigic_min_struct,
		  &CPpatchVariante, &CGpatchVariante);

   switch (CGHeuristic)
      {
      case Heu_CP_In_Goal:
      case Heu_Goal_In_CP:
      case Heu_CPPatch:
         our_error ("This heuristic doesn't work on critical goals!");
         break;
      case Heu_CGPatch:
	CPA_CGInit (CGpatchVariante);
	break;
      case Heu_PolyWeight:
        CF_PolyWeightInitialisieren();
        break;
      default:
         break;
      }
   
   return TRUE;
}
