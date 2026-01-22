/* KBOSuffSolver.c

Modifikationen von Bernd Loechner. Implementiert jetzt das Verfahren von
Hurd, bis auf einstellige Funktionssymbole mit Gewicht 0.

 * 3.7.2003
 * Christian Schmidt
 *
 * Benutzt ueber wrapper.cc die Omega-lib um KBO-Constraints
 * auf Unerfuellbarkeit zu testen.
 */

#include "KBOSuffSolver.h"
#include "Ausgaben.h"
#include "Ordnungen.h"
#include "SymbolGewichte.h"
#include "SymbolOperationen.h"
#include "Constraints.h"

#include "wrapper.h"

#if DO_OMEGA

/* - Parameter -------------------------------------------------------------- */

#define ausgaben 0 /* 0, 1 */

/* - Macros ----------------------------------------------------------------- */

#if ausgaben
#define blah_WM(x) { IO_DruckeFlex x ; }
#define blah_Omega(x) { print_Relation x ;}
#else
#define blah_WM(x)
#define blah_Omega(x)
#endif



/* - Variablen-Koeffizienten-Vektor -----------------------------------------
 * Das erste Feld (0) hat eine spezielle Bedeutung (s.u.). */
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

static BOOLEAN varKoeffVecAllesNull(void)
{
  unsigned int i;
  for (i = 0; i < varKoeffVecSize; i++){ /* ab i=0 (Zelle fuer Summe der
                                            Gewichte der Funktionssymbole) */
    if (varKoeffVec[i] > 0){
      return FALSE;
    }
  }
  return TRUE;
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


/* === Transformation von KBO-Constraints nach Omega-Constrainst ============ */

/* Erweitert die bestehende Omega-Relation Rel um den KBO-Cons cons.
   Dafür wird cons in einen arithmetischen Constraint umgewandelt.
   (Rel_root wird benötigt, weil man über Rel nicht dran kommt.) */
static void addEConsToOmegaCons(Relation_p Rel, F_And_p Rel_root, EConsT cons)
{
  unsigned int i;
  Variable_ID_p vID;
  Handle_p h; 
  BOOLEAN groessergleich;

  blah_WM(("### ## KBO-Suffsolver testet: %e\n", cons));

  varKoeffVecNullen(); /* VarKoeffVec fuer jede Ungleichung 
                          neu mit Nullen fuellen */
  varKoeffVecTermModifizieren(cons->s, 1); /* Vars und Funks von s addieren */
  varKoeffVecTermModifizieren(cons->t, -1); /*Vars und Funks von t subtrahieren*/

  if (cons->tp == Eq_C ) { /* Je nach EConsTyp: Gt_C, Eq_C, Ge_C */
    h = OM_add_eq(Rel_root); /* neuer "=" Knoten */
    groessergleich = FALSE;
  } else {
    h = OM_add_geq(Rel_root); /* neuer ">=" Knoten */
    groessergleich = TRUE;

    {
      UTermT s = cons->s;
      UTermT t = cons->t;
      UTermT s0 = TO_NULLTerm(s);
      UTermT t0 = TO_NULLTerm(t);
      UTermT s1 = TO_Schwanz(s); /* Bezeichnen naechstes anzuguckendes Termpaar */
      UTermT t1 = TO_Schwanz(t);
      
      while (s1 != s0 && t1 != t0 && varKoeffVecAllesNull()){
        if (!TO_TermeGleich(s1,t1)){
          varKoeffVecTermModifizieren(s1, 1); /* Vars und Funks von s1 addieren */
          varKoeffVecTermModifizieren(t1, -1); /*Vars und Funks von t1 subtrahieren*/
          s1 = TO_Schwanz(s1);
          t1 = TO_Schwanz(t1);
        }
        else {
          s1 = TO_NaechsterTeilterm(s1);
          t1 = TO_NaechsterTeilterm(t1);
        }
      }
    }

  }

  OM_update_Const(h, varKoeffVec[0]); /* Konstanter Teil des Constraints 
                                         = Summe der Gewichte der Funks */
  blah_WM(("### ## arithmetisch: %d", varKoeffVec[0]));

  for (i = 1; i < varKoeffVecSize; i++){ /*ab 1, weil hier nur Vars interessant*/
    if (varKoeffVec[i] != 0) { /* die Koeffezienten aller Vars im Omega-Cons 
                                  sind initial schon 0 */
      vID = OM_get_Var_ID(Rel, i); /* ID der i-ten Variable holen */
      OM_update_Coeff(h, vID, varKoeffVec[i]); /* Keoffizient der i-ten 
                                                  Variable setzen */
      blah_WM((" + %dx%d", varKoeffVec[i], i));
    }
  }

  if (groessergleich) { blah_WM((" >= 0\n")); } else { blah_WM((" = 0\n")); }

  blah_WM(("### ## Omega-Constraint: "));
  blah_Omega((Rel)); /* Omega-Constraint ausgeben */
}


/* Fuegt fuer alle Variablen die Bedingung >= 1 hinzu. */
static void addPosVarToOmegaCons(F_And_p Rel_root, Relation_p Rel)
{
  unsigned int i;
  Handle_p h; /* ">=" Knoten */
  Variable_ID_p vID;

  for (i = 1; i < varKoeffVecSize; i++){ /* ab 1, weil nur Vars >= 1 */
    vID = OM_get_Var_ID(Rel, i); /* ID der i-ten Variable holen */
    h = OM_add_geq(Rel_root);    /* neuer ">=" Knoten, soll  werden : vID >= 1 */
    OM_update_Const(h,-1);
    OM_update_Coeff(h, vID, 1);  /* vID hat jetzt den Koeffizienten 1
                                    (update_Coeff addiert den Parameter auf den
                                    bereits bestehenden Koeffizienten
                                    (initial = 0)) */
  }
}


/* testet, ob der uebergebene Constraint erfuellbar ist */
static BOOLEAN isSatisfiable(EConsT e)
{
  EConsT cons;
  int varAnz;
  Relation_p Rel;
  BOOLEAN ret;
  F_And_p Rel_root;

  blah_WM(("-----------------------------------------------\n"));
  blah_WM(("### KBO-Suffsolver testet: %E\n", e));
  varKoeffVecAnpassen(); /* Groesse des VarKoeffVec anpassen */
  varAnz = SO_VarNummer(SO_NeuesteVariable); /* Variablenanzahl im System
                                                bestimmen */
  Rel = OM_create_Relation(varAnz); /* neue Omega-Relation mit varAnz vielen
                                     * Variablen erzeugen */
  Rel_root = OM_create_AND_root(Rel); /* erzeuge AND Knoten an die alle
                                       * Constraints gehaengt werden */

  blah_WM(("### Omega-Constraint: "));
  blah_Omega((Rel)); /* Omega-Constraint ausgeben */

  for (cons = e ; cons != NULL ; cons = cons->Nachf){
    addEConsToOmegaCons(Rel, Rel_root, cons);
  }
  addPosVarToOmegaCons(Rel_root, Rel); /* Vars muessen >= 1 sein */
    

  blah_WM(("### Omega-Constraint: (PosVar Bedingung)\n"));
  blah_Omega((Rel)); /* Omega-Constraint ausgeben */

  ret = OM_is_Satisfiable(Rel); /* ist der Omega-Constraint erfuellbar? */

  blah_WM(("### erfuellbar == %b \n", ret));

  OM_delete_Relation(Rel); /* Omega-Constraint abraeumen */
  return ret;
}

#endif
/* ------------------------------------------------------------------------- */
/* -------- Interface nach aussen ------------------------------------------ */
/* ------------------------------------------------------------------------- */

BOOLEAN Hurd_isSatisfiable(EConsT e) 
{
#if DO_OMEGA
  return isSatisfiable(e);
#else
  return TRUE;
#endif
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
