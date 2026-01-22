/*

07.03.94 Angelegt. Arnim.

*/


/* ------------------------------------------------*/
/* ------------- Includes -------------------------*/
/* ------------------------------------------------*/


#include "Ausgaben.h"
#include "DSBaumOperationen.h"
#include "Grundzusammenfuehrung.h"
#include "KBO.h"
#include "Praezedenz.h"
#include "SpeicherParameter.h"
#include "SymbolGewichte.h"
#include "SymbolOperationen.h"
#include "TermOperationen.h"
#include "general.h"

/* -------------------------------------------------------*/
/* --------------- Globals -------------------------------*/
/* -------------------------------------------------------*/


static int VarlistenDimension;  /* Die Varliste realisiert die Abbildung Variablensymbol -> Vorkommensdifferenz */
static int *Varliste;           /* in den beiden Anfragetermen. Das Feld ist negativ indiziert, fuer Eintrag 0 */
                                /* wird kein Speicher angelegt. */

struct KB_FliegengewichtS KB_Fliegengewicht;

/* -------------------------------------------------------*/
/* ---------------Verwaltung der Variablen-'Menge' -------*/
/* -------------------------------------------------------*/


/* ++++++++++++++ Zugriffe ++++++++++++++++++++++++++++++*/

#define VarlistenDurchlauf(Index) SO_forSymboleAufwaerts(Index,VarlistenDimension, SO_ErstesVarSymb) /* Reihenfolge ist ok, da Variablen abwaerts gezaehlt werden (-1 ...) */

static void VarlisteInitialisierenUndGgfAnpassen(void)
/* Maximal moegliche Variablenzahl bestimmen, das Variablen-GFeld ggf. anpassen und
   mit 0 initialisieren.*/
{
  SymbolT Variable = SO_NeuesteVariable /*BO_NeuesteBaumvariable()*2*/;

  if (SO_VariableIstNeuer(Variable,VarlistenDimension)){
    our_dealloc(Varliste+VarlistenDimension);    /* Addition der Dimension wg. negativer Adressierung */
    VarlistenDimension = SO_VarDelta(Variable,KB_AnzErsterVariablen);
    Varliste = (int *) our_alloc(-VarlistenDimension*sizeof(int)) - VarlistenDimension;
  }
  VarlistenDurchlauf(Variable)
    Varliste[Variable] = 0;        /* Varliste mit 0 initialisieren */
}


static VergleichsT VarlisteLoeschenMitEntscheidung(void)
/* Liefert das in der Variablenliste dokumentierte Variablenverhaeltnis, also <, >, = oder #.
   Gleichzeitig wird die Variablenliste freigegeben. */
{
  SymbolT Index;
  VergleichsT erg=Gleich;

  VarlistenDurchlauf(Index){
    switch (erg){
    case Gleich   :
      if (Varliste[Index] <0)
        erg = Kleiner;
      else if(Varliste[Index] >0)
        erg = Groesser;
      break;
    case Kleiner  :
      if(Varliste[Index] >0)
        erg = Unvergleichbar;
      break;
    case Groesser :
      if(Varliste[Index] <0)
        erg = Unvergleichbar;
      break;
    default:
      break;
    }
    Varliste[Index] = 0;
  }
  return erg;
}

static BOOLEAN VarlisteLoeschenMitGroesserTest(void)
/* Testet die Varliste, ob alle Eintraege >= 0 sind. Gleichzeitig wird die Varliste abgebaut. */ 
{
  SymbolT Index;
  BOOLEAN result = TRUE;

  VarlistenDurchlauf(Index){
    if (Varliste[Index] <0)
      result = FALSE;
    Varliste[Index] = 0;
  }
  return result;
}

static void VarlisteLoeschen(void)
/* Es wird die Varliste fuer den naechsten Aufruf initialisiert.*/
{
  SymbolT Index;

  VarlistenDurchlauf(Index)
    Varliste[Index] = 0;
}


#define /*void*/ VarZahlErhoehen(/*SymbolT*/ var) Varliste[var]++
#define /*void*/ VarZahlErniedrigen(/*SymbolT*/ var) Varliste[var]--


/* -------------------------------------------------------*/
/* --------------- Initialisieren ------------------------*/
/* -------------------------------------------------------*/

static BOOLEAN KBO_SchonInitialisiert = FALSE;

static void FehlerMitSymbol (SymbolT f, const char *text)
/* Ausgabe des Fehlertextes plus Symbol */
{
   char *s;
   char *symb;
   int len;

   symb = IO_FktSymbName(f);
   len = strlen(symb)+strlen (text) + 20;
   s = string_alloc(len);
   sprintf (s, "%s (symbol: `%s')", text, symb);
   our_error (s);
}

#if GZ_GRUNDZUSAMMENFUEHRUNG
static unsigned int *Haeufigkeit_l,
                    *Haeufigkeit_r;
#endif

void KB_InitAposteriori(void)
/* Richtet den Speicher-Manager ein.
   Annahmen verifizieren: Testet, ob die Funktionsgewichte der Definition entsprechend eingestellt sind.
   Da das Gewicht der Variablen auf das kleinste Konstantengewicht gesetzt wird, kann
   Klausel (iii) mit dem Test Vargewicht>0 ueberprueft werden. Die Klauseln (iv) und (v) werden explizit geprueft.

   ACHTUNG : Diese Funktion nur aufrufen, wenn KBO als Ordnung eingesstellt ist.
*/
{
   SymbolT f,g;
   KB_Fliegengewicht.Vorhanden = FALSE;

   if (!KBO_SchonInitialisiert){
     Varliste = (int *) our_alloc(KB_AnzErsterVariablen*sizeof(int))+KB_AnzErsterVariablen;
     VarlistenDimension = -KB_AnzErsterVariablen;
#if GZ_GRUNDZUSAMMENFUEHRUNG
     Haeufigkeit_l = posarray_alloc(GZ_MaximaleVariablenzahl,unsigned int,SO_Extrakonstante(1));
     Haeufigkeit_r = posarray_alloc(GZ_MaximaleVariablenzahl,unsigned int,SO_Extrakonstante(1));
#endif
   }
   SO_forFktSymbole(f)
     switch (SO_Stelligkeit(f)){
     case 0:
       if (SG_SymbolGewichtKBO(f)==0)
         FehlerMitSymbol(f, "KBO: constant symbol with weight 0");
       break;
     case 1:
       if (SG_SymbolGewichtKBO(f)==0){
         if (KB_Fliegengewicht.Vorhanden)
           FehlerMitSymbol(f, "KBO: more than one unary function symbol with weight 0");
         KB_Fliegengewicht.Vorhanden = TRUE;
         KB_Fliegengewicht.Symbol = f;
#if 0
         SO_forFktSymbole(g)
           if (f!=g && !PR_SymbGroesser(f,g))
             FehlerMitSymbol(f, "KBO: unary function symbol with weight 0 not maximal in precedence");
#endif
       }
       break;
     default:
       break;
     }
   KBO_SchonInitialisiert = TRUE;
}

/* -------------------------------------------------------*/
/* --------------- Allgemeine Funktionen -----------------*/
/* -------------------------------------------------------*/

   
/* +++++++++++++++ Abschaelen ++++++++++++++++++++++++++++*/

/* 
Gemaess Pacman-Lemma so lange von beiden Termen die Topsymbole entfernen, bis ungleiche oder nicht einstellige 
Symbole erreicht sind:
*/
#define TermeKuerzen(/*UTermT*/ Term1, /*UTermT*/ Term2)                                                            \
  while ((SO_SymbGleich(TO_TopSymbol(Term1),TO_TopSymbol(Term2))) && (TO_AnzahlTeilterme(Term1) == 1)) {            \
    (Term1) = TO_ErsterTeilterm(Term1); (Term2) = TO_ErsterTeilterm(Term2);                                         \
  }

/* +++++++++++++++ Vortest ++++++++++++++++++++++++++++++*/

/* Durchlaufen beider Terme. Dabei werden:
   - Variablenvorkommen gezaehlt,
   - Phi berechnet.
   Dabei werden Variablenvorkommen und Phi bei identischen Symbolen
   NICHT erhoeht, um unnoetige Additionen zu sparen.*/

static void Vortest(UTermT term1, UTermT term2,      /* die zu durchlaufenden Terme */
                    long int *phidiff,                  /* phi(term1)-phi(term2) */
                    BOOLEAN *Identisch)
{
   UTermT ende1, ende2;
   BOOLEAN nichtfertig1, nichtfertig2; /* Flags, die angeben, welche(r) Term(e) nach Ende der gemeinsamen while-Schleife
                                          bereits fertigdurchlaufen ist(sind).*/
   SymbolT top1, top2;                 /* Die gerade bearbeiteten Topsymbole. */

   *phidiff = 0;
   *Identisch = TRUE;
   ende1 = TO_NULLTerm(term1), ende2 = TO_NULLTerm(term2);
   do{                                                                    /* Testen, ob die Topsymbole identisch sind.            */
      top1 = TO_TopSymbol(term1), top2 = TO_TopSymbol(term2);             /* Falls nein, wird phi jeweils unterschiedlich erhoeht */
      if (SO_SymbUngleich(top1, top2)){                                   /* und das Gleichheitsflag auf FALSE gesetzt.           */
         *phidiff += SG_SymbolGewichtKBO(top1) - SG_SymbolGewichtKBO(top2);
         *Identisch = FALSE;
         if (SO_SymbIstVar (top1))
            VarZahlErhoehen(top1);
         if (SO_SymbIstVar (top2))
            VarZahlErniedrigen(top2);
      }
      term1 = TO_Schwanz(term1), term2 = TO_Schwanz(term2);               /* beide Terme weiterschalten. */
      nichtfertig1 = TO_PhysischUngleich(term1, ende1), nichtfertig2 = TO_PhysischUngleich(term2, ende2);
   }while(nichtfertig1  && nichtfertig2);
                                                                          /* Weiter, solange beide noch nicht am Ende sind. */

   if (nichtfertig1)                                                      /* Term1 noch nicht fertig. Dann ist Term2 bereits fertig */
                                                                          /* (Invariante der ersten Schleife) */
      do{
         top1 = TO_TopSymbol(term1);
         *phidiff += SG_SymbolGewichtKBO(top1);
         if (SO_SymbIstVar (top1))
            VarZahlErhoehen(top1);
         term1 = TO_Schwanz(term1);
      }while(TO_PhysischUngleich(term1, ende1));                          /* ... bis Term1 beendet */

   else
                                                                             /* Term1 fertig gelesen */
      if (nichtfertig2)                                                      /* Term2 nicht -> Term2 weiter durchlaufen. */
         do{
            top2 = TO_TopSymbol(term2);
            *phidiff -= SG_SymbolGewichtKBO(top2);
            if (SO_SymbIstVar (top2))
               VarZahlErniedrigen(top2);
            term2 = TO_Schwanz(term2);
         }while(TO_PhysischUngleich(term2, ende2));                          /* ... bis Term2 beendet */
}


/* -------------------------------------------------------*/
/* --------------- Funktionen fuer GROESSER-Test ---------*/
/* -------------------------------------------------------*/


/* +++++++++++++++ Triviale Faelle +++++++++++++++++++++++*/

static BOOLEAN TrivialeFaelleLiefernEntscheidungBeiGroesser(UTermT term1, UTermT term2, BOOLEAN *ergebnis)
/* Liefert TRUE als Rueckgabe, wenn eine Entscheidung schon getroffen werden konnte. Das Ergebnis steht dann im
   Var-Parameter ergebnis.
   Waehrend der einzelnen Test-Faelle wird die Funktion mit return verlassen, sobald das Ergebnis feststeht.
   (Es gibt also mehr als einen Ausgang!)*/
{

   if TO_TermIstVar(term1){
      *ergebnis = FALSE;                                              /* Term1 ist Variable => kann nicht groesser */
      return TRUE;                                                    /* sein als Term2 */
   }
   else if TO_TermIstVar(term2){                                      /* Term2 ist Variable, Term1 nicht  */
      SymbolT index, top2 = TO_TopSymbol(term2);                      /* => nur einfacher Teiltermtest */

      TO_forSymboleOT(term1, index,
                   if (SO_SymbGleich(top2, index)){
                      *ergebnis = TRUE;
                      return TRUE;
                   };);
      *ergebnis = FALSE;
      return TRUE;
   }
   else if TO_Nullstellig(term1){                                     /* Term1 ist Konstante, Term2 keine Variable */
      long int     phidiff    = -SG_SymbolGewichtKBO(TO_TopSymbol(term1));
      SymbolT   index;

#if GZ_GRUNDZUSAMMENFUEHRUNG
      SymbolT c = TO_TopSymbol(term1);       /* Fuer s Extrakonstante gilt s>t <=> t Extrakonstante und s >prec t. */
      if (SO_IstExtrakonstante(c)) {
        SymbolT d = TO_TopSymbol(term2);
        *ergebnis = SO_IstExtrakonstante(d) && PR_SymbGroesser(c,d);
        return TRUE;
      }
#endif
      if (TO_TopSymboleGleich(term1, term2)){                                     /* Terme sind gleich => nicht groesser */
         *ergebnis = FALSE;
         return TRUE;
      }
      TO_doSymbole(term2, index,                                     /* Term2 durchlaufen bis Variable gef. oder phi zu gross */
                   phidiff += SG_SymbolGewichtKBO(index);
                   if ((phidiff > 0) || SO_SymbIstVar(index)){        /* phi(Term2)>phi(Term1) oder Term2 enthaelt Variable */
                      *ergebnis = FALSE;                             /* => Term1 ist nicht groesser als Term2 */
                      return TRUE;
                   };);
      *ergebnis = ((phidiff < 0) || (PR_SymbGroesser(TO_TopSymbol(term1),TO_TopSymbol(term2)))); 
      return TRUE;                                                   /* sonst : wende erste beiden KBO-Klauseln an */
      /* Falls beim Durchlaufen von Term2 eine Variable gefunden oder phi groesser als phi2 wurde, wird direkt FALSE geliefert.
         Sonst werden die ersten beiden KBO-Klauseln angewandt:
           - phi1 > phi2 oder
           - (phi1 = phi2 steht bereits fest, da aus (*ergebnis = TRUE -> !(phi2>phi1)) und !(phi1 > phi2) folgt: phi1 = phi2)
             und Top(Term1) > Top(Term2)
      */
   }
   else if TO_Nullstellig(term2){                                     /* Term2 ist Konstante, Term1 nicht einstellig */
      SymbolT index;
      long int   phidiff = SG_SymbolGewichtKBO(TO_TopSymbol(term2));

#if GZ_GRUNDZUSAMMENFUEHRUNG
      SymbolT d = TO_TopSymbol(term2);   /* Fuer t Extrakonstante gilt s>t <=> s >TT c >=prec t mit Extrakonstante c. */
      if (SO_IstExtrakonstante(d)) {
        UTermT term10 = TO_NULLTerm(term1);
        *ergebnis = FALSE;
        do {
          SymbolT c = TO_TopSymbol(term1);
          if (SO_IstExtrakonstante(c) && (SO_SymbGleich(c,d) || PR_SymbGroesser(c,d)))
            *ergebnis = TRUE;
        } while (TO_PhysischUngleich(term1 = TO_Schwanz(term1),term10));
        return TRUE;
      }
#endif
 
      TO_doSymbole(term1, index,                                      /* Berechnet Phi(term1), bis phi1>phi2 (Dann term1>term2). */
                   phidiff -= SG_SymbolGewichtKBO(index);
                   if (phidiff < 0){
                      *ergebnis = TRUE;
                      return TRUE;
                   };);
      *ergebnis = ( (phidiff == 0) && 
                   (PR_SymbGroesser(TO_TopSymbol(term1),TO_TopSymbol(term2)))); /* Sonst wird die erste Klausel der KBO angewendet */
      return TRUE;                                                    /* und das Ergebnis zurueckgegeben. */
   }
   else
      return FALSE;                                                   /* FALSE zurueckliefern */
}
/* +++++++++++++++ Triviale Faelle fuer GroesserLex +++++++++++++++++++++++*/

static BOOLEAN TrivialeFaelleLiefernEntscheidungBeiLexFuerGroesser(UTermT term1, UTermT term2, VergleichsT *ergebnis)
/* Dies ist der Trivial-Test bei der Lex-Erweiterung beim Groesser-Test. Er wird sukessive auf die Teilterme
   angewendet.

   Liefert TRUE als Rueckgabe, wenn eine Entscheidung schon getroffen werden konnte. Das Ergebnis steht dann im
   Var-Parameter ergebnis:
   Groesser, falls term1 > term2 gemaess lexik. Erweiterung;
   Gleich, falls term1 und term2 identisch sind,
   Unvergleichbar, wenn definitiv term1 nicht groesser als term2 ist.

   Waehrend der einzelnen Test-Faelle wird die Funktion mit return verlassen, sobald das Ergebnis feststeht.
   (Es gibt also mehr als einen Ausgang!)
*/
{

   if TO_TermIstVar(term1){
      if SO_SymbGleich(TO_TopSymbol(term1), TO_TopSymbol(term2))     /* Term1 ist Variable => kann nicht groesser */
         *ergebnis = Gleich;                                          /* sein als Term2, hoechstens gleich */                 
      else
         *ergebnis = Unvergleichbar;
      return TRUE;
   }
   else if TO_TermIstVar(term2){                                      /* Term2 ist Variable, Term1 nicht => nur einfacher Teiltermtest */
      SymbolT index, top2 = TO_TopSymbol(term2);
      
      TO_forSymboleOT(term1, index,
                   if (SO_SymbGleich(top2, index)){
                      *ergebnis = Groesser;                           /* term2 in term1 enthalten => groesser */
                      return TRUE;
                   };);
      *ergebnis = Unvergleichbar;                                     /* term2 ist Var und nicht in term1 enth. => unvergleichbar */
      return TRUE;
   }
   else if TO_Nullstellig(term1){                                     /* Term1 ist Konstante */
      long int     phidiff = -SG_SymbolGewichtKBO(TO_TopSymbol(term1));
      SymbolT   top2;

      if (TO_TopSymboleGleich(term1, term2)){                                     /* Terme sind gleich */
         *ergebnis = Gleich;
         return TRUE;
      }

#if GZ_GRUNDZUSAMMENFUEHRUNG
    {
      SymbolT c = TO_TopSymbol(term1);       /* Fuer s Extrakonstante gilt s>t <=> t Extrakonstante und s >prec t */
      if (SO_IstExtrakonstante(c)) {         /* Dann Groesser zurueck, Unvergleichbar sonst. */
        SymbolT d = TO_TopSymbol(term2);
        *ergebnis = SO_IstExtrakonstante(d) && PR_SymbGroesser(c,d) ? Groesser : Unvergleichbar;
        return TRUE;
      }
    }
#endif

      TO_doSymbole(term2, top2,                                     /* Term2 durchlaufen bis Variable gef. oder phi zu gross */
                   phidiff += SG_SymbolGewichtKBO(top2);
                   if ((phidiff > 0) || SO_SymbIstVar(top2)){        /* phi(Term2)>phi(Term1) oder Term2 enthaelt Variable */
                      *ergebnis = Unvergleichbar;                    /* => Term1 ist nicht groesser/gleich Term2 */
                      return TRUE;
                   };);
      /* sonst : wende erste beiden KBO-Klauseln an */
      *ergebnis = ((phidiff < 0) || (PR_SymbGroesser(TO_TopSymbol(term1),TO_TopSymbol(term2))))? Groesser:Unvergleichbar;
      return TRUE;
      /* Falls beim Durchlaufen von Term2 eine Variable gefunden oder phi groesser als phi2 wurde, 
         wird direkt Unvergleichbar geliefert. Sonst werden die ersten beiden KBO-Klauseln angewandt:
           - phi1 > phi2 oder
           - (phi1 = phi2 steht bereits fest, da aus (Schleife) und !(phi1 > phi2) folgt: phi1 = phi2)
             und Top(Term1) > Top(Term2)
      */
   }
   else if TO_Nullstellig(term2){                                     /* Term2 ist Konstante, Term1 nicht => keine Gleichheit moeglich */
      SymbolT index;
      long int   phidiff = -SG_SymbolGewichtKBO(TO_TopSymbol(term2));

#if GZ_GRUNDZUSAMMENFUEHRUNG
      SymbolT d = TO_TopSymbol(term2);   /* Fuer t Extrakonstante gilt s>t <=> s >TT c >=prec t mit Extrakonstante c. */
      if (SO_IstExtrakonstante(d)) {     /* Dann Groesser zurueck, Unvergleichbar sonst. */
        UTermT term10 = TO_NULLTerm(term1);
        *ergebnis = Unvergleichbar;
        do {
          SymbolT c = TO_TopSymbol(term1);
          if (SO_IstExtrakonstante(c) && (SO_SymbGleich(c,d) || PR_SymbGroesser(c,d)))
            *ergebnis = Groesser;
        } while (TO_PhysischUngleich(term1 = TO_Schwanz(term1),term10));
        return TRUE;
      }
#endif

      TO_doSymbole(term1, index,                                      /* Berechnet Phi(term1), bis phi1>phi2 (Dann ist term1>term2). */
                   phidiff += SG_SymbolGewichtKBO(index);
                   if (phidiff > 0){
                      *ergebnis = Groesser;
                      return TRUE;
                   };);
       /* Sonst wird die erste Klausel der KBO angewendet: */
      *ergebnis = ((phidiff == 0)&&(PR_SymbGroesser(TO_TopSymbol(term1),TO_TopSymbol(term2))))?Groesser:Unvergleichbar;
      return TRUE;                                                    /* und das Ergebnis zurueckgegeben. */
   }
   else
      return FALSE;                                                   /* FALSE zurueckliefern (dh. keine Entscheidung)*/
}

/* +++++++++++++++++ KBO-GroesserRek +++++++++++++++++++++++++++*/

static BOOLEAN ExtrakonstantenUnvertraeglich(UTermT l, UTermT r)
{
#if GZ_GRUNDZUSAMMENFUEHRUNG
  UTermT t,
         l0 = TO_NULLTerm(l),
         r0 = TO_NULLTerm(r);
  SymbolT c1 = SO_Extrakonstante(1),
          cn = SO_Extrakonstante(GZ_MaximaleVariablenzahl);
  SymbolT c;
  BOOLEAN ExtrasVorhanden = FALSE,
          Resultat = FALSE;
  SO_forSymboleAufwaerts(c,c1,cn)
    Haeufigkeit_l[c] = Haeufigkeit_r[c] = 0;
  t = l;
  do {
    c = TO_TopSymbol(t);
    if (SO_IstExtrakonstante(c))
      ExtrasVorhanden = TRUE, Haeufigkeit_l[c]++;
  } while (TO_PhysischUngleich(t = TO_Schwanz(t),l0));
  t = r;
  do {
    c = TO_TopSymbol(t);
    if (SO_IstExtrakonstante(c))
      ExtrasVorhanden = TRUE, Haeufigkeit_r[c]++;
  } while (TO_PhysischUngleich(t = TO_Schwanz(t),r0));

  if (ExtrasVorhanden) {
    signed int delta = 0;
    SO_forSymboleAbwaerts(c,cn,c1)                                              /* rueckwaerts wegen cn > ... > c1 */
      if ((delta += Haeufigkeit_l[c]-Haeufigkeit_r[c])<0) {
        Resultat = TRUE;
        break;
      }
  }
  /*IO_DruckeFlex("$$$ Extrakonstanten %t > %t -- %s\n", l,r,Resultat ? "unvertraeglich" : "ok");*/
  return Resultat;
#else
  return FALSE; /* Argl. BL. */
#endif
}

/* Forward-Deklaration */
BOOLEAN KBOGroesserLex(UTermT term1, UTermT term2);

static BOOLEAN KBOGroesserRek(UTermT term1, UTermT term2, long int phidiff)
/* nach Durchfuehrung von Trivialitaeten- und Vortest wird mit der Errechneten Phi-Differenz und der Variablen-
   liste diese Funktion aufgerufen. Sie liefert TRUE, wenn term1 > term2, und FALSE sonst.
   Vor dem Aufruf wurden folgende Operationen durchgefuehrt:
   - Terme schaelen.
   - Trivialitaetentest,
   - Vortest (wg. der uebergebenen Parameter),
   - Identitaetstest (da unterschiedliche Behandlung in KBO>Lex und KBOGroesser)
*/

{
  if (phidiff<0)
    return FALSE;
  else
    if (VarlisteLoeschenMitGroesserTest()) {          /* Varmengenvergleich ist ok */
      if (ExtrakonstantenUnvertraeglich(term1,term2))
        return FALSE;
      if (phidiff > 0)
        return TRUE;
      else{                                                  /* phi1 = phi2 */
        
        switch (PR_Praezedenz(TO_TopSymbol(term1),TO_TopSymbol(term2))){
        case Groesser: return TRUE;                         /* ... und f > g */
          break;
        case Gleich  : return KBOGroesserLex(term1, term2); /* ... und f = g */
          break;
        default: return FALSE;                              /* ... und f < g oder f # g */
          break;
        }
      }
    }
    else /* Varmenge nicht ok */
      return FALSE;                                          /* term2 enthaelt Variablen, die nicht in term1 enthalten sind */
                                                             /* => term1 nicht groesser als term2 */
}
      


/* +++++++++++++++++ KBO-GroesserLex +++++++++++++++++++++++++++*/


BOOLEAN KBOGroesserLex(UTermT term1, UTermT term2)
/* Fuehrt den lexikographischen Groesser-Test der KBO durch.
   Dabei kann aufgrund der dem Aufruf vorangehenden Operationen davon ausgegangen werden, dass
   1. beide Terme nicht nullstellig sind,
   2. phi(term1) == phi(term2) ist,
   3. die Topsymbole von term1 und term2 gleich sind und damit insbesondere gleiche Stelligkeiten haben,
   4. die Varliste geleert ist.
*/
{
   UTermT tt1 = TO_ErsterTeilterm(term1), tt2 = TO_ErsterTeilterm(term2),
          ende1 = TO_NULLTerm(term1);   /* wegen gleicher Topsymbole nur ein Ende-Test noetig. */
   VergleichsT resultat;
   BOOLEAN identisch;
   long int phidiff;

   do{
      TermeKuerzen(tt1, tt2);
      if (TrivialeFaelleLiefernEntscheidungBeiLexFuerGroesser(tt1, tt2, &resultat)){
         switch (resultat){
         case Groesser: return TRUE;
            break;
         case Kleiner  : case Unvergleichbar : return FALSE;
            break;
         default :break; /* Gleich => weiter */
         }
      }
      else{
         Vortest(tt1 ,tt2, &phidiff, &identisch); /* Gemaess Voraussetzung 4. ist die Varliste hier leer */
         if (! identisch){
            return KBOGroesserRek(tt1, tt2, phidiff);
         }
         /* else weiter */
         /* Waren die Teilterme identisch, so ist die Varliste auch ohne erneutes Leeren wieder leer, dh. sie ist auch fuer die
            naechsten Teilterme wieder verwendbar.*/
      }
         tt1 = TO_NaechsterTeilterm(tt1); /* Funktioniert auch mit gekuerzten Teiltermen, da deren Termenden mit denen */
         tt2 = TO_NaechsterTeilterm(tt2); /* der Anfrageterme identisch sind */
   }while(TO_PhysischUngleich(tt1, ende1));
   our_assertion_failed("Fehler in der KBO: identische Terme mit KBO-Lex abgehandelt !");
   return FALSE;
}


/* -------------------------------------------------------*/
/* --------------- Funktionen fuer Entscheidungsfkt. -----*/
/* -------------------------------------------------------*/

/* +++++++++++++++++++++++++ Trivialitaeten-Test ++++++++++*/

static BOOLEAN TrivialTestFuerEntscheidungsfunktionLiefertEntscheidung(UTermT term1,UTermT term2,VergleichsT *ergebnis)
/* Rueckgabewert==TRUE, falls eine Entscheidung schon hier getroffen werden konnte.
   Die Entscheidung steht dann in ergebnis.
   Waehrend der einzelnen Test-Faelle wird die Funktion mit return verlassen, sobald das Ergebnis feststeht.
   (Es gibt also mehr als einen Ausgang!)

   Die einzelnen Entscheidungen beruhen auf Lemma 1 und Lemma 2 der KBO_Dokumentation.
*/
{
   if TO_TermIstVar(term1){
      if SO_SymbGleich(TO_TopSymbol(term1), TO_TopSymbol(term2)) /* Term1 ist Variable => kann nicht groesser */
         *ergebnis = Gleich;                                          /* sein als Term2, hoechstens kleiner o. gleich */                 
      else{                                                           /* nicht identisch => ... */
         SymbolT index, top1 = TO_TopSymbol(term1);

         TO_doSymbole(term2,                                          /* ... alle Symbole von term2 durchlaufen */
                       index,                                         /* und auf Variablenvorkommen testen. */
                       if (SO_SymbGleich(index,top1)){                /* Wird term1 in term2 gefunden => Kleiner */
                          *ergebnis = Kleiner;
                          return TRUE;
                       };);
         *ergebnis = Unvergleichbar;                                  /* Andernfalls enthaelt term2 eine andere Var */
      }                                                               /* oder ist ein Grundterm mit groesserem Phi, */
      return TRUE;                                                    /* aber enthaelt die Variable term1 nicht => # */
   }
   else if (TO_TermIstVar(term2)){                                /* Term2 ist Variable => Tests wie oben, aber ohne */
      SymbolT index, top2 = TO_TopSymbol(term2);                      /* Gleichheit (oben schon erledigt) */

      TO_doSymbole(term1,                                             /* ... alle Symbole von term1 durchlaufen */
                   index,                                             /* und auf Variablenvorkommen testen. */
                   if (SO_SymbGleich(index,top2)){                    /* Wird term2 in term1 gefunden => Groesser */
                      *ergebnis = Groesser;
                      return TRUE;
                   };);
      *ergebnis = Unvergleichbar;                                     /* Andernfalls enthaelt term1 eine andere Var */
      return TRUE;                                                    /* oder ist ein Grundterm mit groesserem Phi, */
   }                                                                  /* aber enthaelt die Variable term1 nicht => # */

   else if TO_Nullstellig(term1){                                /* Term1 ist Konstante (Vartest ist < oder = ) */
      long int     phidiff = SG_SymbolGewichtKBO(TO_TopSymbol(term1));        /* phidiff = phi1 - phi2 */
      SymbolT   top2;
      BOOLEAN varGefunden = FALSE;

      if (TO_TopSymboleGleich(term1, term2))                              /* Terme sind gleich */
         *ergebnis = Gleich;
      else{
        TO_doSymbole(term2, top2,                                      /* Term2 durchlaufen, phi berechnen und Vars suchen */
                     phidiff -= SG_SymbolGewichtKBO(top2);
                     if (phidiff < 0){                                 /* phi(Term2)>phi(Term1) => term1 < term2 */
                       *ergebnis = Kleiner;
                       return TRUE;
                     };
                     varGefunden = varGefunden || SO_SymbIstVar(top2);
                     );

        /* Ab hier kann phidiff<0 nicht mehr vorkommen!*/
        if (varGefunden) {                                             /* Variable gefunden (dh. Varvergleich ist <), dann .. */
          if (phidiff > 0)                                                 /* ... phi1>phi2 => Unvergleichbar */
            *ergebnis = Unvergleichbar;
          else                                                             /* phidiff == 0, dh. phi1 == phi2 */
            *ergebnis = (PR_SymbGroesser(TO_TopSymbol(term2), TO_TopSymbol(term1)))? Kleiner:Unvergleichbar;
	}                                                                 /* ... f<g => Kleiner, sonst Unvergleichbar */
        else {                                                           /* Keine Variable gefunden */
          if (phidiff > 0)                                                 /* ... phi1>phi2 => Groesser */
            *ergebnis = Groesser;
          else                                                             /* ... phi1==phi2 => Topsymbole vergleichen */
            *ergebnis = PR_Praezedenz(TO_TopSymbol(term1), TO_TopSymbol(term2));
	}
      }
      return TRUE;
   }
   else if TO_Nullstellig(term2){                                 /* Term2 ist Konstante, Term1 nicht =>keine Gleichheit moeglich */
      long int     phidiff = -SG_SymbolGewichtKBO(TO_TopSymbol(term2));        /* phidiff = phi1 - phi2 */
      SymbolT   top1;
      BOOLEAN varGefunden = FALSE;

      TO_doSymbole(term1, top1,                                      /* Term1 durchlaufen, phi berechnen und Vars suchen */
                    phidiff += SG_SymbolGewichtKBO(top1);
                    if (phidiff > 0){                                 /* phi(Term1)>phi(Term2) => term1 > term2 */
                       *ergebnis = Groesser;
                       return TRUE;
                    };
		   varGefunden = varGefunden || SO_SymbIstVar(top1);
		   );
      /* Ab hier kann phidiff<0 nicht mehr vorkommen!*/
      if (varGefunden)  {                                            /* Variable gefunden (dh. Varvergleich ist >), dann .. */
         if (phidiff < 0)                                                 /* ... phi1<phi2 => Unvergleichbar */
            *ergebnis = Unvergleichbar;
         else                                                             /* phidiff == 0, dh. phi1 == phi2 */
            *ergebnis = (PR_SymbGroesser(TO_TopSymbol(term1), TO_TopSymbol(term2)))? Groesser:Unvergleichbar;
      }     /* f>g => Groesser, sonst Unvergleichbar */
      else {                                                          /* Keine Variable gefunden */
         if (phidiff < 0)                                                 /* ... phi1<phi2 => Kleiner */
            *ergebnis = Kleiner;
         else                                                             /* ... phi1==phi2 => Topsymbole vergleichen */
            *ergebnis = PR_Praezedenz(TO_TopSymbol(term1), TO_TopSymbol(term2));
      }
      return TRUE;
   }
   else
     return FALSE;
}




/* +++++++++++++++++++++++++ KBOEntscheidungRek ++++++++++*/

/* FORWARD: */
VergleichsT KBOEntscheidungLex(UTermT term1, UTermT term2);

static VergleichsT KBOEntscheidungRek(UTermT term1, UTermT term2, long int phidiff)
/* nach Durchfuehrung von Trivialitaeten- und Vortest wird mit der Errechneten Phi-Differenz und der Variablen-
   liste diese Funktion aufgerufen. Sie liefert TRUE, wenn term1 > term2, und FALSE sonst.
   Vor dem Aufruf wurden folgende Operationen durchgefuehrt:
   - Terme schaelen.
   - Trivialitaetentest,
   - Vortest (wg. der uebergebenen Parameter),
   - Identitaetstest (da unterschiedliche Behandlung in KBO>Lex und KBOGroesser)
*/

{
   switch (VarlisteLoeschenMitEntscheidung()){
   case Groesser:                                                            /* term1 varmaessig groesser term2 und ... */
     if (phidiff <0)                                                  /* ... phi1 < phi2 => Unvergleichbar */
       return Unvergleichbar;
     else if (phidiff > 0)                                            /* ... phi1 > phi2 => Groesser */
       return Groesser;
     else                                                             /* ... phi1 = phi2 und*/
       switch (PR_Praezedenz(TO_TopSymbol(term1),TO_TopSymbol(term2))){
       case Groesser:                                                             /* ... f>g => Groesser */
         return Groesser;
         break;
       case Gleich:                                                               /* ... f=g => GroesserLex(term1, term2) */
         return ( (KBOGroesserLex(term1,term2))? Groesser:Unvergleichbar);
         break;
       default:                                                                   /* ... sonst : Unvergleichbar */
         return Unvergleichbar;
         break;
       }
     break;
   case Kleiner:                                                              /*  term1 varmaessig kleiner term2 und ...*/
      if (phidiff <0)                                                  /* ... phi1 < phi2 => Kleiner */
         return Kleiner;
      else if (phidiff > 0)                                            /* ... phi1 > phi2 => Unvergleichbar */
         return Unvergleichbar;
      else                                                             /* ... phi1 = phi2 und*/
         switch (PR_Praezedenz(TO_TopSymbol(term1),TO_TopSymbol(term2))){
         case Kleiner:                                                              /* ... f<g => Kleiner */
            return Kleiner;
            break;
         case Gleich:                                                               /* ... f=g => GroesserLex(term2, term1) */
            return ( (KBOGroesserLex(term2,term1))? Kleiner:Unvergleichbar);
            break;
         default:                                                                   /* ... sonst : Unvergleichbar */
            return Unvergleichbar;
            break;
         }
      break;
   case Gleich:                                                       /* term1 und term2 enthalten gleiche Variablen */
      if (phidiff <0)                                                   /* ... phi1 < phi2 => Kleiner */
         return Kleiner;
      else if (phidiff > 0)                                             /* ... phi1 > phi2 => Groesser */
         return Groesser;
      else                                                              /* ... phi1 = phi2 und*/
         switch (PR_Praezedenz(TO_TopSymbol(term1),TO_TopSymbol(term2))){
         case Kleiner:                                                              /* ... f>g => Groesser */
            return Kleiner;
            break;
         case Groesser:                                                             /* ... f<g => Kleiner */
            return Groesser;
            break;
         case Gleich:                                                               /* ... f=g =>                      */
            return KBOEntscheidungLex(term1, term2);                                 /* GroesserEntschLex(term1, term2) */
            break;
         default:                                                                   /* ... f#g => Unvergleichbar */
            return Unvergleichbar;
            break;
         }
      break;
   default: return Unvergleichbar;
      break;
   }
}

/* +++++++++++++++++++++++++ Entscheidung-Lex ++++++++++*/

VergleichsT KBOEntscheidungLex(UTermT term1, UTermT term2)
/* Fuehrt den lexikographischen Groesser-Test der KBO durch.
   Dabei kann aufgrund der dem Aufruf vorangehenden Operationen davon ausgegangen werden, dass
   1. beide Terme nicht einstellig sind,
   2. phi(term1) == phi(term2) ist,
   3. die Topsymbole von term1 und term2 gleich sind und damit insbesondere gleiche Stelligkeiten haben und
   4. die Varliste geleert ist.
*/
{
   UTermT tt1 = TO_ErsterTeilterm(term1), tt2 = TO_ErsterTeilterm(term2),
          ende1 = TO_NULLTerm(term1);   /* wegen gleicher Topsymbole nur ein Ende-Test noetig. */
   VergleichsT resultat;
   BOOLEAN identisch;
   long int phidiff;

   do{
      TermeKuerzen(tt1, tt2);
      if (TrivialTestFuerEntscheidungsfunktionLiefertEntscheidung(tt1, tt2, &resultat) && resultat != Gleich)
        return resultat;
      else{
         Vortest(tt1 ,tt2, &phidiff, &identisch); /* Gemaess Vorbedingung 4. ist die Varliste hier leer */
         if (! identisch)
            return KBOEntscheidungRek(tt1, tt2, phidiff);
         /* else weiter */
         /* Waren die Teilterme identisch, so ist die Varliste auch ohne erneutes Leeren wieder leer, dh. sie ist auch fuer die
            naechsten Teilterme wieder verwendbar.*/
      }
      tt1 = TO_NaechsterTeilterm(tt1); /* Bei gekuerzten Teiltermen sind die Termenden die gleichen wie die des */
      tt2 = TO_NaechsterTeilterm(tt2); /* ungekuerzten Termes. */
   }while(TO_PhysischUngleich(tt1, ende1));
   our_assertion_failed("Fehler in der KBO: identische Terme mit KBOEntscheidungs-Lex abgehandelt !");
   return Unvergleichbar; /* Wert ist egal */
}

static BOOLEAN kb_KBOGroesser(UTermT term1, UTermT term2)
/* Testfunktion, die moeglichst schnell die Hypothese 'term1 > term2' abzulehnen bzw. zu bestaetigen sucht. */
{
   long int       phidiff;
   BOOLEAN     identisch, Ergebnis;

   TermeKuerzen(term1, term2);
   if (TrivialeFaelleLiefernEntscheidungBeiGroesser(term1, term2, &Ergebnis))
      return Ergebnis;
   else{   
      VarlisteInitialisierenUndGgfAnpassen();
      Vortest(term1, term2, &phidiff, &identisch);
      if (identisch){
         VarlisteLoeschen();    /* Eintraege auf Null setzen, geschieht sonst bei Auswertung der Varliste */
         return FALSE;                                          /* phi(term1)<phi(term2) => term1 nicht groesser als term2 */
      }
      else                                                         /* phi1 >= phi2 und Terme sind nicht identisch */
         return KBOGroesserRek(term1, term2, phidiff);
   }
}

static VergleichsT kb_KBOVergleich(UTermT term1, UTermT term2)
/* Entscheidungsfunktion, die das Groessenverhaeltnis von term1 und term2 nach der KBO bestimmt. */
{
   long int       phidiff;
   BOOLEAN     identisch;
   VergleichsT Ergebnis;

   TermeKuerzen(term1, term2);
   if (TrivialTestFuerEntscheidungsfunktionLiefertEntscheidung(term1, term2, &Ergebnis))
      return Ergebnis;
   else{
      VarlisteInitialisierenUndGgfAnpassen();
      Vortest(term1, term2, &phidiff, &identisch);
      if (identisch){
         VarlisteLoeschen();
         return Gleich;
      }
      else                                                         /* phi1 >= phi2 und Terme sind nicht identisch */
         return KBOEntscheidungRek(term1, term2, phidiff);
   }
}

/* ================================================================================ */
/* Neufassungen                                                                     */
/* ================================================================================ */

static BOOLEAN istKleinereExtrakonstante(SymbolT c, SymbolT sym)
{
  return SO_IstExtrakonstante(sym) && (c > sym);
}

static BOOLEAN enthaeltGroesserGleicheExtrakonstante(UTermT s, SymbolT c)
{
  UTermT s0 = TO_NULLTerm(s);
  UTermT s1 = s;
  while (s1 != s0){
    SymbolT tops1 = TO_TopSymbol(s1);
    if (SO_IstExtrakonstante(tops1) && (tops1 >= c)) return TRUE;
    s1 = TO_Schwanz(s1);
  }
  return FALSE;
}

static int *extraKonstMSet = NULL;
static unsigned int extraKonstMSetSize = 0;

static void extraKonstMSetAnpassen(void)
{
  if (extraKonstMSetSize < GZ_MaximaleVariablenzahl){
    extraKonstMSetSize = GZ_MaximaleVariablenzahl;
    extraKonstMSet = our_realloc(extraKonstMSet, extraKonstMSetSize * sizeof(*extraKonstMSet), "extraKonstMSet");
  }
}

static void extraKonstMSetNullen(void)
{
  unsigned int i;
  for (i = 0; i < extraKonstMSetSize; i++){
    extraKonstMSet[i] = 0;
  }
}
static void extraKonstMSetModifizieren(UTermT t, int offset)
{
  UTermT t0 = TO_NULLTerm(t);
  UTermT t1 = t;
  SymbolT c1 = SO_Extrakonstante(1);
  while (t0 != t1){
    SymbolT top = TO_TopSymbol(t1);
    if (SO_IstExtrakonstante(top)){
      extraKonstMSet[top - c1] += offset;
    }
    t1 = TO_Schwanz(t1);
  }
}
static BOOLEAN extraKonstMSetBedingung(void)
{
  int i;
  signed int delta = 0;
  for (i = extraKonstMSetSize - 1; i >= 0; i--){
    delta = delta + extraKonstMSet[i];
    if (delta < 0){
      return FALSE;
    }
  }
  return TRUE;
}

static BOOLEAN extraKonstGTEQ(UTermT s, UTermT t)
{
  extraKonstMSetAnpassen();
  extraKonstMSetNullen();
  extraKonstMSetModifizieren(s,1);
  extraKonstMSetModifizieren(t,-1);
  return extraKonstMSetBedingung();
}

/* ================================================================================ */

static int *varMSet = NULL;
static unsigned int varMSetSize = 0;

static void varMSetAnpassen(void)
{
  if (varMSetSize <= SO_VarNummer(SO_NeuesteVariable)){
    varMSetSize = SO_VarNummer(SO_NeuesteVariable) + 1;
    varMSet = our_realloc(varMSet, varMSetSize * sizeof(*varMSet), "varMSet");
  }
}

static void varMSetNullen(void)
{
  unsigned int i;
  for (i = 1; i < varMSetSize; i++){
    varMSet[i] = 0;
  }
}
static void varMSetModifizieren(UTermT t, int offset)
{
  UTermT t0 = TO_NULLTerm(t);
  UTermT t1 = t;
  while (t0 != t1){
    if (TO_TermIstVar(t1)){
      varMSet[SO_VarNummer(TO_TopSymbol(t1))] += offset;
    }
    t1 = TO_Schwanz(t1);
  }
}
static BOOLEAN varMSetNichtNegativ(void)
{
  unsigned int i;
  for (i = 1; i < varMSetSize; i++){
    if (varMSet[i] < 0){
      return FALSE;
    }
  }
  return TRUE;
}

static BOOLEAN varMSetGTEQ(UTermT s, UTermT t)
{
  varMSetAnpassen();
  varMSetNullen();
  varMSetModifizieren(s,1);
  varMSetModifizieren(t,-1);
  return varMSetNichtNegativ();
}

/* ================================================================================ */

static long phi(UTermT s)
{
  long res = 0;
  UTermT s0 = TO_NULLTerm(s);
  UTermT s1 = s;
  while (s0 != s1){
    res = res + SG_SymbolGewichtKBO(TO_TopSymbol(s1));
    s1 = TO_Schwanz(s1);
  }
  return res;
}

/* ================================================================================ */

static BOOLEAN kb2_KBOGroesser(UTermT s, UTermT t)
/* Testfunktion, die moeglichst schnell die Hypothese 'term1 > term2' abzulehnen bzw. zu bestaetigen sucht. */
{
  SymbolT tops = TO_TopSymbol(s);
  SymbolT topt = TO_TopSymbol(t);

  if (SO_SymbIstVar(tops))                             return FALSE;
  if (SO_SymbIstVar(topt))                             return TO_SymbolEnthalten (topt,s);
  if (SO_IstExtrakonstante(tops))                      return istKleinereExtrakonstante(tops,topt);
  if (SO_IstExtrakonstante(topt))                      return enthaeltGroesserGleicheExtrakonstante(s,topt);
  if (!varMSetGTEQ(s,t))                               return FALSE;
  if (!extraKonstGTEQ(s,t))                            return FALSE;

  {
    long phidiff = phi(s) - phi(t);
    if (phidiff != 0)                                  return phidiff > 0;
  }
  {
    VergleichsT precVgl = PR_Praezedenz(tops,topt);
    if (precVgl == Unvergleichbar)                     our_error("Unvergleichbare Praez!");
    if (precVgl != Gleich)                             return precVgl == Groesser;
    if (tops != topt)                                  our_error("Quasipraezedenz!");
  }
  {
    UTermT s0 = TO_NULLTerm(s);
    UTermT s1 = TO_ErsterTeilterm(s);
    UTermT t1 = TO_ErsterTeilterm(t);
    while(s0 != s1){
      if (!TO_TermeGleich(s1,t1))                      return kb2_KBOGroesser(s1,t1);
      s1 = TO_NaechsterTeilterm(s1);
      t1 = TO_NaechsterTeilterm(t1);
    }
    return FALSE; /* Nur wenn TO_TermeGleich(s, t) */
  }
}

static VergleichsT kb2_KBOVergleich(UTermT term1, UTermT term2)
/* Entscheidungsfunktion, die das Groessenverhaeltnis von term1 und term2 nach der KBO bestimmt. */
{
  if (TO_TermeGleich(term1, term2))  return Gleich;
  if (kb2_KBOGroesser(term1, term2)) return Groesser;
  if (kb2_KBOGroesser(term2, term1)) return Kleiner;
  return Unvergleichbar;
}

/* -------------------------------------------------------*/
/* --------------- exportierte Funktionen ----------------*/
/* -------------------------------------------------------*/


BOOLEAN KB_KBOGroesser(UTermT term1, UTermT term2)
/* Testfunktion, die moeglichst schnell die Hypothese 'term1 > term2' abzulehnen bzw. zu bestaetigen sucht. */
{
  BOOLEAN res1 = kb_KBOGroesser(term1,term2);
#if 0
  BOOLEAN res2 = kb2_KBOGroesser(term1,term2);
  if (res1 != res2){
    IO_DruckeFlex("Unstimmigkeit KBOGroesser: %t >? %t ergibt %b bzw %b\n", term1, term2, res1, res2);
    {  BOOLEAN res1 = kb_KBOGroesser(term1,term2);}
  }
  return res2;
#else
  return res1;
#endif
}

VergleichsT KB_KBOVergleich(UTermT term1, UTermT term2)
/* Entscheidungsfunktion, die das Groessenverhaeltnis von term1 und term2 nach der KBO bestimmt. */
{
  VergleichsT res1 = kb_KBOVergleich(term1,term2);
#if 0
  VergleichsT res2 = kb2_KBOVergleich(term1,term2);
  if (res1 != res2){
    IO_DruckeFlex("Unstimmigkeit KBOVergleich: %t ?<=>? %t ergibt %v bzw %v\n", term1, term2, res1, res2);
    {  VergleichsT res1 = kb_KBOVergleich(term1,term2);}
  }
  return res2;
#else
  return res1;
#endif
}
