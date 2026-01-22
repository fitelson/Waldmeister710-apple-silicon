/* KBOSuffSolver.c
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
#include "Unifikation2.h" /* fuer Phi' */
#include "Praezedenz.h" /* fuer Phi' */

#include "wrapper.h"

#if DO_OMEGA

/* - Parameter -------------------------------------------------------------- */

#define ausgaben 0 /* 0, 1 */
#define TRACE             0 /* 0, 1 */
#define INDENT_TRACE      0 /* 0, 1 */

/* - Macros ----------------------------------------------------------------- */

#if ausgaben
#define blah_WM(x) { IO_DruckeFlex x ; }
#define blah_Omega(x) {if(0){ OM_print_Relation x ;}}
#else
#define blah_WM(x)
#define blah_Omega(x)
#endif


/* -------------------------------------------------------------------------- */
/* - Tracing Infrastructure ------------------------------------------------- */
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

#if TRACE
#define OPENTRACE(x)  {traceindent(tracelevel++);blah_WM(x);}
#define CLOSETRACE    {traceindent(--tracelevel);blah_WM(("*\n"));}
#define RESTRACE(x)   {traceindent(tracelevel);blah_WM(x);}
#else
#define OPENTRACE(x)  /* nix */
#define CLOSETRACE    /* nix */
#define RESTRACE(x)   /* nix */
#endif

/* -------------------------------------------------------------------------- */


typedef enum {eq,ge,gt} RelPhiT;
char * RPT[] = {"eq", "ge", "gt"};

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


/* === Transformation von KBO-Constraints nach Omega-Constrainst == 2.Version */

/* Hilfsfunktion: Haengt an current_node den Knoten :
 * phi(const->s)-phi(cons->t) <relPhi> 0
 * Dabei ist relphi in "eq","ge" oder "gt". */
static void addEConsToOmegaCons2phiDiff(Relation_p Rel, F_And_p current_node,
                                        TermT s, TermT t, RelPhiT relPhi)
{
  Handle_p h; 
  unsigned int i;
  Variable_ID_p vID;

  varKoeffVecNullen(); /* VarKoeffVec fuer jede Ungleichung 
                          neu mit Nullen fuellen */
  varKoeffVecTermModifizieren(s, 1); /* Vars und Funks von s addieren */
  varKoeffVecTermModifizieren(t, -1); /*Vars und Funks von t subtrahieren*/

  if (relPhi == eq){
    h = OM_add_eq(current_node); /* neuer "=" Knoten */
  } else {/* relPhi == ge || relPhi == gt
          * In beiden Faellen ein neuer ">=" Knoten, weil die Omega-lib keine
          * ">" Knoten kennt. */
    h = OM_add_geq(current_node);
  }

/*   blah_WM(("addEConsToOmegaCons2phiDiff: %t %s %t arithmetisch %d\n",  */
/*            s, RPT[relPhi], t, varKoeffVec[0])); */

  if (relPhi == gt){
    /* "gt" wird erreicht durch Const-1 */
    varKoeffVec[0] -= 1;
  }

  OM_update_Const(h, varKoeffVec[0]); /* Konstanter Teil des Constraints 
                                         = Summe der Gewichte der Funks */
  RESTRACE(("%d", varKoeffVec[0]));

  for (i = 1; i < varKoeffVecSize; i++){ /*ab 1, weil hier nur Vars interessant*/
    if (varKoeffVec[i] != 0) { /* die Koeffezienten aller Vars im Omega-Cons 
                                  sind initial schon 0 */
      vID = OM_get_Var_ID(Rel, i); /* ID der i-ten Variable holen */
      OM_update_Coeff(h, vID, varKoeffVec[i]); /* Keoffizient der i-ten 
                                                  Variable setzen */
      blah_WM((" + %dx%d", varKoeffVec[i], i));
    }
  }

  if (relPhi == eq){
    blah_WM((" = 0\n"));
  } else { /* relPhi == ge || relPhi == gt */
    blah_WM((" >= 0\n")); /* s.o. */
  }

/*   blah_WM(("addEConsToOmegaCons2phiDiff: Omega-Constraint: ")); */
  blah_Omega((Rel)); /* Omega-Constraint ausgeben */
}

static void addEConsToOmegaCons2addFalse(F_And_p current_node)
{
  /* leerer Or-Knoten = False */
  OM_create_OR_child(current_node);
  RESTRACE(("False "));
}

/* Der atomare Constraint hat die Form s = t.
 * Entspricht Phi= bei Bernd. */
static void addEConsToOmegaCons2phiEq(Relation_p Rel, F_And_p current_node, 
                                         TermT s, TermT t)
{
  SymbolT x;
  U2_SubstAufIdentitaetSetzen();
  if (U2_WeiterUnifiziert(s, t)){
    SO_forSymboleAbwaerts(x, SO_ErstesVarSymb, SO_NeuesteVariable){
      if (U2_VarGebunden(x)){
        TermT u, sigma_u;
        u = TO_(x);
        sigma_u = U2_MGU(u);
        if (!TO_TermeGleich(u,sigma_u)){
          /* x - phi(sigma(x)) = 0 anhaengen */
          addEConsToOmegaCons2phiDiff(Rel, current_node, u, sigma_u, eq);
        }
        TO_TermeLoeschen(u, sigma_u);
      }
    }
    U2_SubstAufIdentitaetSetzen();
  }
  else {
    addEConsToOmegaCons2addFalse(current_node);
    blah_WM(("(nicht unifizierbar)\n"));
  }
}

/* Muss hier forward deklariert werden. */
static void addEConsToOmegaCons2phiLex(Relation_p Rel, F_And_p current_node,
                                          TermT ss, TermT s_NULL, TermT ts);

/* Fall: TO_TopSymbol(s) == TO_TopSymbol(t) && !TO_TermIstVar(s) und
 * top(s) ist nicht 0-stellig.
 * Entspricht Phi3 bei Bernd. */
static void addEConsToOmegaCons2phi3(Relation_p Rel, F_And_p current_node, 
                                        TermT s, TermT t)
{
  F_Or_p node_Or;
  /* Da nur an Und-Knoten atomare Constraints gehaengt werden koennen (OM),
     brauchen wir einen Hilfsknoten.*/
  F_And_p node_And, hilfsknoten;
  OPENTRACE(("OR\n"));
  node_Or = OM_create_OR_child(current_node);
  OPENTRACE(("AND\n"));
  hilfsknoten = OM_create_AND_child(node_Or);
  addEConsToOmegaCons2phiDiff(Rel, hilfsknoten, s, t, gt);
  CLOSETRACE
  OPENTRACE(("AND\n"));
  node_And = OM_create_AND_child(node_Or);
  addEConsToOmegaCons2phiDiff(Rel, node_And, s, t, eq); 
  addEConsToOmegaCons2phiLex(Rel, node_And,
                             TO_ErsterTeilterm(s), TO_NULLTerm(s),
                             TO_ErsterTeilterm(t));
  CLOSETRACE
  CLOSETRACE
}

/* Fall: TO_TopSymbol(s) == TO_TopSymbol(t) && !TO_TermIstVar(s).
 * Filtert 0-stellige Symbole raus.
 * Entspricht Phi2 bei Bernd. */
static void addEConsToOmegaCons2phi2(Relation_p Rel, F_And_p current_node, 
                                        TermT s, TermT t)
{
  if (SO_Stelligkeit(TO_TopSymbol(s)) == 0){
    addEConsToOmegaCons2addFalse(current_node);
    blah_WM(("(nullstellig)\n"));
  } else {
    addEConsToOmegaCons2phi3(Rel, current_node, s, t);
  }
}
 
/* Der atomare Constraint hat die Form s > t.
 * Entspricht Phi> bei Bernd. */
static void addEConsToOmegaCons2phiGt(Relation_p Rel, F_And_p current_node, 
                                         TermT s, TermT t)
{
  if (TO_TopSymbol(s) == TO_TopSymbol(t) && !TO_TermIstVar(s)){
    addEConsToOmegaCons2phi2(Rel, current_node, s, t);
  } else if (!TO_TermIstVar(s) && !TO_TermIstVar(t) &&
             PR_Praezedenz(TO_TopSymbol(s), TO_TopSymbol(t)) == Kleiner){
    addEConsToOmegaCons2phiDiff(Rel, current_node, s, t, gt);
  } else {
    addEConsToOmegaCons2phiDiff(Rel, current_node, s, t, ge);
  }
}


/* Erweitert die bestehende Omega-Relation Rel um den KBO-Cons cons.
 * Dafür wird cons in einen arithmetischen Constraint umgewandelt.
 * (Rel_root wird benötigt, weil man über Rel nicht dran kommt.) 
 *
 * Dabei wird die verschaerfte Version Phi' implementiert.
 * Wendet auf den atomaren Constraint die Funktion Phi' an. (vgl. PA)
 * Entspricht Phi1 von Bernd. */
static void addEConsToOmegaCons2phi1(Relation_p Rel, F_And_p current_node, 
                                        EConsT cons)
{
  TermT s,t;
  s = cons->s;
  t = cons->t;

  if (cons->tp == Gt_C){
    addEConsToOmegaCons2phiGt(Rel, current_node, s, t);
  } else if (cons->tp == Eq_C){
    addEConsToOmegaCons2phiEq(Rel, current_node, s, t);
  } else { /* cons->tp == Ge_C */
    F_Or_p node_Or;
    F_And_p node_And;
    OPENTRACE(("OR\n"));
    node_Or = OM_create_OR_child(current_node); /* erzeuge OR-Kind-Knoten an den
                                              * die beiden Constraints gehaengt
                                              * werden */
    OPENTRACE(("AND\n"));
    node_And = OM_create_AND_child(node_Or);
    addEConsToOmegaCons2phiGt(Rel, node_And, s, t);
    CLOSETRACE

    OPENTRACE(("AND\n"));
    node_And = OM_create_AND_child(node_Or);
    addEConsToOmegaCons2phiEq(Rel, node_And, s, t);
    CLOSETRACE

/*     blah_WM(("addEConsToOmegaCons2Ge: Omega-Constraint: ")); */
    blah_Omega((Rel)); /* Omega-Constraint ausgeben */
    CLOSETRACE
  }
}


/* Lex-Fall. (vgl. PA)
 * Entspricht PhiLex bei Bernd. */
static void addEConsToOmegaCons2phiLex(Relation_p Rel, F_And_p current_node,
                                       TermT ss, TermT s_NULL, TermT ts)
{
  TermT nss, nts;
  nss = TO_Schwanz(ss);
  nts = TO_Schwanz(ts);

  if (nss == s_NULL){
    addEConsToOmegaCons2phiGt(Rel, current_node, ss, ts);
  } else {
    F_Or_p node_Or;
    F_And_p node_And_1, node_And_2;
    OPENTRACE(("OR\n"));
    node_Or = OM_create_OR_child(current_node);
    OPENTRACE(("AND\n"));
    node_And_1 = OM_create_AND_child(node_Or);
    addEConsToOmegaCons2phiGt(Rel, node_And_1, ss, ts);
    CLOSETRACE
    OPENTRACE(("AND\n"));
    node_And_2 = OM_create_AND_child(node_Or);
    addEConsToOmegaCons2phiEq(Rel, node_And_2, ss, ts);
    addEConsToOmegaCons2phiLex(Rel, node_And_2, nss, s_NULL, nts);
    CLOSETRACE
    CLOSETRACE
  }
}

/* Fuegt fuer alle Variablen die Bedingung >= 1 hinzu. */
static void addPosVarToOmegaCons(F_And_p current_node, Relation_p Rel)
{
  unsigned int i;
  Handle_p h; /* ">=" Knoten */
  Variable_ID_p vID;

  for (i = 1; i < varKoeffVecSize; i++){ /* ab 1, weil nur Vars >= 1 */
    vID = OM_get_Var_ID(Rel, i); /* ID der i-ten Variable holen */
    h = OM_add_geq(current_node);    /* neuer ">=" Knoten, soll  werden : vID >= 1 */
    OM_update_Const(h,-1);
    OM_update_Coeff(h, vID, 1);  /* vID hat jetzt den Koeffizienten 1
                                    (update_Coeff addiert den Parameter auf den
                                    bereits bestehenden Koeffizienten
                                    (initial = 0)) */
  }
}


/* testet, ob der uebergebene Constraint erfuellbar ist */
/* entspricht Phi0 von Bernd */
static BOOLEAN isSatisfiable(EConsT e, int version)
{
  EConsT cons;
  int varAnz;
  Relation_p Rel;
  BOOLEAN ret;
  F_And_p Rel_root;

  blah_WM(("-----------------------------------------------\n"));
  OPENTRACE(("### KBO-Suffsolver testet: %E\n", e));
  varKoeffVecAnpassen(); /* Groesse des VarKoeffVec anpassen */
  varAnz = SO_VarNummer(SO_NeuesteVariable); /* Variablenanzahl im System
                                                bestimmen */
  Rel = OM_create_Relation(varAnz); /* neue Omega-Relation mit varAnz vielen
                                     * Variablen erzeugen */
  Rel_root = OM_create_AND_root(Rel); /* erzeuge AND-Knoten an die alle
                                       * Constraints gehaengt werden */

  for (cons = e ; cons != NULL ; cons = cons->Nachf){
    if (version == 1){
      addEConsToOmegaCons(Rel, Rel_root, cons);
    } else if (version == 2){
      OPENTRACE(("AND\n"));
      addEConsToOmegaCons2phi1(Rel, Rel_root, cons); /* hier wird jetzt Phi'
                                                      * benutzt*/
      CLOSETRACE
    } else {
      our_error("KBO-Suffsolver: falsche Version!\n");
    }
  }
/*   blah_WM(("### Omega-Constraint: ")); */
  blah_Omega((Rel)); /* Omega-Constraint ausgeben */

  addPosVarToOmegaCons(Rel_root, Rel); /* Vars muessen >= 1 sein */
    

/*   blah_WM(("### Omega-Constraint: (PosVar Bedingung)\n")); */
  blah_Omega((Rel)); /* Omega-Constraint ausgeben */

  ret = OM_is_Satisfiable(Rel); /* ist der Omega-Constraint erfuellbar? */

  CLOSETRACE
  blah_WM(("### erfuellbar == %b \n", ret));

  OM_delete_Relation(Rel); /* Omega-Constraint abraeumen */
  return ret;
}

#endif
/* ------------------------------------------------------------------------- */
/* -------- Interface nach aussen ------------------------------------------ */
/* ------------------------------------------------------------------------- */

BOOLEAN KBOSuff_isSatisfiable1(EConsT e) 
{
#if DO_OMEGA
  return isSatisfiable(e,1);
#else
  return TRUE;
#endif
}

BOOLEAN KBOSuff_isSatisfiable2(EConsT e) 
{
#if DO_OMEGA
  return isSatisfiable(e,2);
#else
  return TRUE;
#endif
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
