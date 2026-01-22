#include "general.h"
#include "Ausgaben.h"
#include "FehlerBehandlung.h"
#include "KBO.h"
#include "Praezedenz.h"
#include "SymbolOperationen.h"


#ifndef CCE_Source
#include "Parameter.h"
#include "SpezNormierung.h"
#include "PhilMarlow.h"
#else
#include "parse-util.h"
#endif

#include <stdlib.h>

typedef VergleichsT *pr_PraezedenzT;

SymbolT      PR_minimaleKonstante;
BOOLEAN      PR_PraezedenzTotal;

SymbolT *pr_TotaleKette;
static pr_PraezedenzT pr_Praezedenz;


static VergleichsT pr_LesePraezedenz(pr_PraezedenzT Praezedenz, SymbolT f, SymbolT g)
{
  return Praezedenz[f*SO_AnzahlFunktionen+g];
}

static void pr_SetzePraezedenz(pr_PraezedenzT Praezedenz, SymbolT f, SymbolT g, VergleichsT v)
{
  Praezedenz[f*SO_AnzahlFunktionen+g] = v;
}

static pr_PraezedenzT NeuesPraezedenzfeldAllokieren(void)
{
  SymbolT i,j;
  unsigned int size = SO_AnzahlFunktionen * SO_AnzahlFunktionen;
  pr_PraezedenzT p = array_alloc(size,VergleichsT);
  SO_forFktSymbole(i)
    SO_forFktSymbole(j)
      pr_SetzePraezedenz(p,i,j,Unvergleichbar);
  return p;
}

static void pr_PraezedenzDrucken(pr_PraezedenzT p)
{
  SymbolT i,j;
  IO_DruckeFlex("---------------------------\n");
  SO_forFktSymbole(i) {                                                /* Praezedenz-Matrix zeilenweise durchlaufen. */
    SO_forFktSymbole(j) {
      IO_DruckeFlex("%v", pr_LesePraezedenz(p,i,j));
    }
    IO_DruckeFlex("\n");
  }
  IO_DruckeFlex("---------------------------\n");
}

static void pr_KetteAusgeben(SymbolT *Kette)
{
  unsigned int i; 
  for(i = 0; i < SO_AnzahlFunktionen; i++) { 
    IO_DruckeFlex("%S", Kette[i]); 
    if (i < SO_AnzahlFunktionen - 1) 
      printf(" > "); 
  } 
}

static BOOLEAN pr_SymbolNochUnvergleichbar(pr_PraezedenzT Praezedenz, SymbolT f)
{
  SymbolT g;
  SO_forSpezSymbole(g){
    if ((pr_LesePraezedenz(Praezedenz,g,f) != Unvergleichbar) ||
        (pr_LesePraezedenz(Praezedenz,f,g) != Unvergleichbar)){
      return FALSE;
    }
  }
  return TRUE;
}

static void pr_ParaSymboleEinbauen(pr_PraezedenzT Praezedenz)         /* ggf. Eintraege f>p und EQ>TRUE>FALSE setzen */
{
  if (PM_Existenzziele() &&
      pr_SymbolNochUnvergleichbar(Praezedenz,SO_EQ)) {
    SymbolT f,p;
    SO_forSpezSymbole(f)                                                 /* Jeder Operator f aus der Spezifikation */
      SO_forParaSymbole(p)                            /* ist groesser als jeder Existenzquantifizierungsoperator p */
        pr_SetzePraezedenz(Praezedenz,f,p,Groesser);
    pr_SetzePraezedenz(Praezedenz,SO_EQ,SO_TRUE,Groesser);
    pr_SetzePraezedenz(Praezedenz,SO_TRUE,SO_FALSE,Groesser);
  }
}

static void pr_ExtraSymboleEinbauen(pr_PraezedenzT Praezedenz)
{
  if (pr_SymbolNochUnvergleichbar(Praezedenz, SO_const1)){
    SymbolT f,c;
    SO_forSymboleAufwaerts(c,SO_const1,SO_const2){
      SO_forSymboleAufwaerts(f,SO_KleinstesFktSymb,c-1){
        pr_SetzePraezedenz(Praezedenz,f,c,Groesser);
      }
    }
  }
#if 1
  if (pr_SymbolNochUnvergleichbar(Praezedenz, SO_EQN)){
    SymbolT f;
    SO_forSymboleAufwaerts(f,SO_KleinstesFktSymb,SO_EQN-1){
      pr_SetzePraezedenz(Praezedenz,f,SO_EQN,Groesser);
    }
  }
#endif
}

/*=================================================================================================================*/
/*= V. Von der Praezedenz zur Totalordnung                                                                        =*/
/*=================================================================================================================*/

static void pr_NeueFunktionenEinbauen(pr_PraezedenzT Praezedenz)
{
  pr_ExtraSymboleEinbauen(Praezedenz);
  pr_ParaSymboleEinbauen(Praezedenz);
}

static void TransitivenAbschlussBilden(pr_PraezedenzT Praezedenz)
{ 
  BOOLEAN geaendert;
  do {
    SymbolT f,g,h;
    geaendert = FALSE;
    SO_forFktSymbole(f)                               /* Zeilenweise durchlaufen. */
      SO_forFktSymbole(g)
        if (pr_LesePraezedenz(Praezedenz,f,g) == Groesser)                     /* f>g */
          SO_forFktSymbole(h)
            if (pr_LesePraezedenz(Praezedenz,g,h) == Groesser &&               /* g>h */
                pr_LesePraezedenz(Praezedenz,f,h) == Unvergleichbar) {
              pr_SetzePraezedenz(Praezedenz,f,h,Groesser);              /* => f>h */
              geaendert = TRUE;
            }
  } while (geaendert);
}


VergleichsT *qcmp_Praezedenz;

static int qcmp(const void *x, const void *y)
{
  SymbolT f = *(SymbolT *)x,
          g = *(SymbolT *)y;
  switch (pr_LesePraezedenz(qcmp_Praezedenz, f,g)) {
  case Groesser:
    return -1;
  case Kleiner:
    return +1;
  default:
    return 0;
  }
}

static BOOLEAN Komplettiert(pr_PraezedenzT Praezedenz, BOOLEAN *PraezedenzTotal, SymbolT *TotaleKette)
/* Fuegt die '=' auf der Diagonalen und die zu den '>' korrespondierenden '<' ein. Dabei wird auf Zyklen getestet
   und festgestellt, ob die Praezedenz total ist.
   Wurde ein Zyklus entdeckt, so wird TRUE zurueckgeliefert.*/
{
  SymbolT i,j;
  *PraezedenzTotal = TRUE;
  SO_forFktSymbole(i)                                                /* Praezedenz-Matrix zeilenweise durchlaufen. */
    SO_forFktSymbole(j) {
      if (i==j) {                                                                                    /* Diagonale. */
        if (pr_LesePraezedenz(Praezedenz,i,j) == Unvergleichbar)                          /* Noch nicht besetzt -> '=' */
          pr_SetzePraezedenz(Praezedenz,i,j,Gleich);
        else                                                                            /* Schon besetzt -> Fehler */
          return FALSE;
      }
      else if (pr_LesePraezedenz(Praezedenz,i,j) == Groesser) {                                                 /* i>j */
        if (pr_LesePraezedenz(Praezedenz,j,i) == Unvergleichbar || 
            pr_LesePraezedenz(Praezedenz,j,i) == Kleiner)                               /* => j<i */
          pr_SetzePraezedenz(Praezedenz,j,i,Kleiner);
        else
          return FALSE;                                                                /* falls auch j>i => Fehler */
      }
      if (j < i && pr_LesePraezedenz(Praezedenz,i,j) == Unvergleichbar)     /* i und j bleiben nach dem Komplettieren unvergleichbar. */
        *PraezedenzTotal = FALSE;                                                /* => Praezedenz ist nicht total. */
    }
  if (*PraezedenzTotal) {
    SO_forFktSymbole(i)
      TotaleKette[i] = i;
    qcmp_Praezedenz = Praezedenz;
    qsort(TotaleKette, SO_AnzahlFunktionen, sizeof(SymbolT), qcmp);
  }
  return TRUE;                                                                         /* Kein Fehler aufgetreten. */
}

static void minimaleKonstanteBestimmen(void)
{
  int i;

  if (PR_PraezedenzTotal) {
    if (pr_TotaleKette == NULL)
      our_assertion_failed("pr_TotaleKette! (minimaleKonstanteBestimmen)");
    for (i = SO_AnzahlFunktionen - 1; i >= 0; i--){
      SymbolT sym = pr_TotaleKette[i];
      if (SO_SymbIstKonst(sym)){
        PR_minimaleKonstante = sym;
        return;
      }
    }
    our_error("no minimal constant in precedence!");
  }
}



static void PraezedenzEinlesen(pr_PraezedenzT p)
/* Liest die in der Spezifikation angegebene Praezedenz ein und besetzt das Ordnungs-Array damit. */
{
  ord_chain ketten = ord_chain_list(get_ordering());                           /* Alle OrdnungsKetten durchlaufen. */
  while (ketten) {
    ord_chain aktKette = ord_first(ketten);                                  /* Aktuelle OrdnungsKette durchlaufen */
    while (aktKette) {
      ord_chain rest = ord_chain_rest(aktKette);                   /* Rest der aktuellen Ordnungskette durchlaufen */
      SymbolT Symbol1 = SN_WMvonDISCOUNT(ord_chain_first(aktKette)); 
      while (rest) {                         /* und den Vergleich mit dem ersten Element der akt. Kette eintragen. */
        SymbolT SymbolN = SN_WMvonDISCOUNT(ord_chain_first(rest)); 
        pr_SetzePraezedenz(p,Symbol1,SymbolN,Groesser);
        rest = ord_chain_rest(rest);
      }
      aktKette = ord_chain_rest(aktKette);
    }
    ketten = ord_rest(ketten);
  }
}

void PR_PraezedenzVonSpecLesen(void)
{
  pr_Praezedenz = NeuesPraezedenzfeldAllokieren();
  pr_TotaleKette = array_alloc(SO_AnzahlFunktionen,SymbolT);
  PraezedenzEinlesen(pr_Praezedenz);                                                /* Praezedenz-Matrix besetzen. */
  pr_NeueFunktionenEinbauen(pr_Praezedenz);            /* Die Spalten und Zeilen mit den neuen Konstanten besetzen */
  TransitivenAbschlussBilden(pr_Praezedenz);
  if (!Komplettiert(pr_Praezedenz, &PR_PraezedenzTotal, pr_TotaleKette))         /* die entsprechenden '<' und '=' */
    our_error("precedence is cyclic!");                          /* einbauen und auf Zykel sowie Totalitaet testen */
  minimaleKonstanteBestimmen();
}

/*=================================================================================================================*/
/*= VII. Praezedenz aus Kommandozeile lesen und auswerten                                                         =*/
/*=================================================================================================================*/

static BOOLEAN PraezedenzAnalyse(char *ps, BOOLEAN PraezedenzSetzen)
{
  pr_PraezedenzT neuePraezedenz;
  BOOLEAN neuePraezedenzTotal;
  SymbolT *neueTotaleKette;
  
  /* 1. Neues Praezedenz-Array anlegen + initialisieren (eq,false,true beachten) */
  neuePraezedenz = NeuesPraezedenzfeldAllokieren();
  /* Der Einbau der Existenzquantifizierungsfunktionen erfolgt nur in der initialen Praezedenz.
     Gibt man in der Kommandozeile eine Praezedenz an oder laesst eine solche durch Permutation oder 
     Ordnungsgenerator erzeugen, so muessen sie explizit angegeben werden. */

  /* 2. Neue Praezedenz dort eintragen (Fehler -> freigeben, return FALSE) */
  do {
    SymbolT GroesseresSymbol = SO_ErstesVarSymb;
    do {                                                                      /* Start einer neuen Praezedenzkette */
      char *NaechsterPunkt = strchr(ps,'.');
      SymbolT Symbol;
      if (NaechsterPunkt)
        *NaechsterPunkt = '\0';
      SO_forFktSymbole(Symbol)
        if (strcmp(ps,IO_FktSymbName(Symbol)) == 0)
          break;
      if (Symbol>SO_GroesstesFktSymb){
        our_dealloc(neuePraezedenz);
        return FALSE;
      }
      if (SO_SymbUngleich(GroesseresSymbol,SO_ErstesVarSymb))
        pr_SetzePraezedenz(neuePraezedenz,GroesseresSymbol,Symbol,Groesser);
      GroesseresSymbol = Symbol;
      if (NaechsterPunkt) {
        *NaechsterPunkt = '.';
        ps = NaechsterPunkt+1;
      }
      else
        ps += strlen(ps);
    } while (*ps && *ps!='.');
    ps++;
  } while (*(ps-1));

  pr_NeueFunktionenEinbauen(neuePraezedenz);
  /* 3. Trans. Abschl. bilden */
  TransitivenAbschlussBilden(neuePraezedenz);

  /* 4. Komplettieren (Fehler -> freigeben, return FALSE) */
  neueTotaleKette = array_alloc(SO_AnzahlFunktionen,SymbolT);
  if (!Komplettiert(neuePraezedenz, &neuePraezedenzTotal, neueTotaleKette)){
    our_dealloc(neuePraezedenz);
    our_dealloc(neueTotaleKette);
    return FALSE;
  }

  /* 5. Ggf. neue Praezedenz setzen */
  if (PraezedenzSetzen) {
    our_dealloc(pr_Praezedenz);
    PR_PraezedenzTotal = neuePraezedenzTotal;
    pr_Praezedenz = neuePraezedenz;
    our_dealloc(pr_TotaleKette);
    pr_TotaleKette = neueTotaleKette;
    minimaleKonstanteBestimmen();
  }
  else {
    our_dealloc(neuePraezedenz);
    our_dealloc(neueTotaleKette);
  }
  return TRUE;
}  


BOOLEAN PR_PraezedenzKorrekt(char *ps)
{
  return PraezedenzAnalyse(ps,FALSE);
}

BOOLEAN PR_PraezedenzErneuert(char *ps)
{
  return PraezedenzAnalyse(ps,TRUE);
}

VergleichsT PR_Praezedenz(SymbolT f, SymbolT g)
{
  BOOLEAN fExtra = SO_IstExtrakonstante(f);
  BOOLEAN gExtra = SO_IstExtrakonstante(g);

  if (fExtra == gExtra)
    return fExtra ? IntegerVergleich(f,g) : pr_LesePraezedenz(pr_Praezedenz,f,g);
  else if (PA_Ordering() == ordering_kbo && KB_Fliegengewicht.Vorhanden)
    return SO_SymbGleich(f,KB_Fliegengewicht.Symbol) ? Groesser : 
      SO_SymbGleich(g,KB_Fliegengewicht.Symbol) ? Kleiner : Unvergleichbar;
  else
    return Unvergleichbar;
}

    
void PR_CleanUp(void)
{
  our_dealloc(pr_Praezedenz);
  our_dealloc(pr_TotaleKette);
}

void PR_PraezedenzAusgeben(void)
{
  if (PR_PraezedenzTotal){
    pr_KetteAusgeben(pr_TotaleKette);
  }
  else {
    pr_PraezedenzDrucken(pr_Praezedenz);
  }
}
