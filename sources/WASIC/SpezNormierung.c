/*****************************************************************************************/
/* Normieren von Spezifikationen                                                         */
/*****************************************************************************************/

#include "general.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "FehlerBehandlung.h"
#include "SpeicherVerwaltung.h"
#include "SpezNormierung.h"
#include "Ausgaben.h"
#include "PhilMarlow.h"

/* Achtung: Im Parser sind Funktionssymbole mit 1 beginnend aufsteigend nummeriert! */

/*****************************************************************************************/
/* 1) Variablen                                                                          */
/*****************************************************************************************/

#define SN_TEST 0

#if SN_TEST
#define pr(Argumente) printf Argumente
#define prp(x,y,z) print_pair(x,y,z)
#else
#define pr(Argumente) 
#define prp(x,y,z) 
#endif

#if SN_TEST
#define TEST(ebbes) ebbes
#define output_array(text)                                                                              \
{int i;                                                                                                 \
pr(("*******************************\n*Array-output %s*\n*******************************\n",text));     \
for (i=0; i<SN_AnzahlGleichungen; i++)                                                                  \
  {pr(("%2d ",i));prp(SN_LHS[i],"=",SN_RHS[i]);}                                                 \
}
#else
#define TEST(ebbes)
#define output_array(text)
#endif

typedef unsigned long kodierung_typ;
const double Primzahlen [] ={2,3,5,7,11,13,17,19,23,29,31,37,41,43,47};
const int AnzPrimzahlen = sizeof(Primzahlen) / sizeof(double);


static kodierung_typ *LFeld, *RFeld;

static kodierung_typ *Stelligkeit;

static kodierung_typ *AnzVorkGleichungen;
static kodierung_typ *PositionenGleichungen;

static kodierung_typ *AnzVorkZiele;
static kodierung_typ *PositionenZiele;


static kodierung_typ *KombiKontextGleichungenEinzeln;
static kodierung_typ *KombiKontextGleichungenEinzelnGegenueber;
static kodierung_typ *KombiKontextGleichungen;

static kodierung_typ *KombiKontextZieleEinzeln;
static kodierung_typ *KombiKontextZieleEinzelnGegenueber;
static kodierung_typ *KombiKontextZiele;

static kodierung_typ *FinaleBewertung;

static int AnzahlSymbole;
static int Variablenzaehler;

/* exportiert: */
int SN_AnzahlGleichungen,
    SN_AnzahlZiele;

/*****************************************************************************************/
/* 2) Daten sammeln                                                                      */
/*****************************************************************************************/

static void VorkommensDatenAusTermSammeln(term Term, kodierung_typ *WerteFeld)
     /* Zaehlt die Vorkommen der Symbole im angegebenen Term und erhoeht die entsprechenden
        Slots im WerteFeld.*/
{
  int i;
  if (!variablep(Term)){
    WerteFeld[term_func(Term)-1]++;

    i = Stelligkeit[term_func(Term)-1];
    while(i>0)
      VorkommensDatenAusTermSammeln(term_arg(Term,--i),WerteFeld);
  }
  else{
    if ( term_func(Term) > 0) our_assertion_failed("Positive Variablennummer in Discount-Term!");
    if ( (-term_func(Term)>Variablenzaehler)) Variablenzaehler=-term_func(Term);
  }
}
    
static void PositionsDatenAusTermSammeln(term Term, kodierung_typ *WerteFeld, kodierung_typ Position, int Tiefe)
     /* Berechnet die Positions-Vorkommen der Symbole in Term in eindeutiger Weise und addiert sie
        auf die entsprechenden Slots in WerteFeld.*/
{
  int i;
  if (Tiefe >=  AnzPrimzahlen) return;       /* Keine Primzahlen mehr! */
  if (!variablep(Term)){
    i = sig_arity(term_func(Term));
    while(i>0){
      PositionsDatenAusTermSammeln(term_arg(Term,i-1),
                                   WerteFeld, 
                                   Position*((kodierung_typ)pow(Primzahlen[Tiefe],(double)i)),
                                   Tiefe+1);
      i--;
    }
    WerteFeld[term_func(Term)-1] += Position;
  }
}

static kodierung_typ TermKontext(term Term, kodierung_typ context)
     /* Verwendet Daten aus Zielen und Gleichungen! Darf daher erst nach der Berechnung
        der AnzVork und Positionen aufgerufen werden.*/
{
  if (variablep(Term)) context++;
  else{
    int i = sig_arity(term_func(Term));
    context+= 
      AnzVorkGleichungen[term_func(Term)-1]
      +AnzVorkZiele[term_func(Term)-1]
      +PositionenGleichungen[term_func(Term)-1]
      +PositionenZiele[term_func(Term)-1];
    
    while(i>0)
      context += TermKontext(term_arg(Term,--i), context);
  }
  return context;
}

static kodierung_typ TermKontext2(term Term, kodierung_typ context)
{
  if (variablep(Term)) context++;
  else{
    int i = sig_arity(term_func(Term));
    context+=
      KombiKontextGleichungen[term_func(Term)-1]
      +KombiKontextZiele[term_func(Term)-1];
    
    while(i>0)
      context += TermKontext2(term_arg(Term,--i), context);
  }
  return context;
}

static void FinaleSymbolBewertung(term Term, kodierung_typ VaterBewertung)
     /* Jedes Symbol erhaelt die Bewertung seines direkten Vater-Terms
        addiert.*/
{
  if (variablep(Term)) return;
  else{
    int i = sig_arity(term_func(Term));

    FinaleBewertung[term_func(Term)-1] += VaterBewertung;

    VaterBewertung = TermKontext2(Term,0);
    
    while(i>0)
      FinaleSymbolBewertung(term_arg(Term,--i), VaterBewertung);
  }
}

#define KontextBewertungAusfuehren(getWhat,Was)                                  \
   for (eql=get_##getWhat(); eql; eql=eq_rest(eql)){                             \
     kodierung_typ GesKonL = TermKontext(eq_first_lhs(eql),0),                          \
         GesKonR = TermKontext(eq_first_rhs(eql),0);                             \
     VorkommensDatenAusTermSammeln(eq_first_lhs(eql), LFeld);                    \
     VorkommensDatenAusTermSammeln(eq_first_rhs(eql), RFeld);                    \
     for (p=get_sig(); p ; p = sig_rest(p)){                                     \
       if (RFeld[sig_first(p)-1] > 0){                                           \
         KombiKontext##Was[sig_first(p)-1] += GesKonR+GesKonL;                   \
         KombiKontext##Was##Einzeln[sig_first(p)-1] += GesKonR;                  \
       }                                                                         \
       if (LFeld[sig_first(p)-1] > 0){                                           \
         KombiKontext##Was[sig_first(p)-1] += GesKonR+GesKonL;                   \
         KombiKontext##Was##Einzeln[sig_first(p)-1] += GesKonL;                  \
       }                                                                         \
                                                                                 \
       if ((RFeld[sig_first(p)-1] > 0) && (LFeld[sig_first(p)-1] == 0))          \
         KombiKontext##Was##EinzelnGegenueber[sig_first(p)-1] += GesKonL;        \
       if ((LFeld[sig_first(p)-1] > 0) && (RFeld[sig_first(p)-1] == 0))          \
         KombiKontext##Was##EinzelnGegenueber[sig_first(p)-1] += GesKonR;        \
                                                                                 \
       if ((RFeld[sig_first(p)-1] > 0) || (LFeld[sig_first(p)-1] > 0))           \
       LFeld[sig_first(p)-1] = RFeld[sig_first(p)-1] = 0;                        \
     }                                                                           \
   }


#define FinaleBewertungAusfuehren(getWhat)                                             \
   for (eql=get_##getWhat(); eql; eql=eq_rest(eql)){                                   \
     FinaleSymbolBewertung(eq_first_lhs(eql), 0);                                      \
     FinaleSymbolBewertung(eq_first_rhs(eql), 0);                                      \
   }
    
static void SymboldatenSammeln(void)
{
   func_symb_list p;
   unsigned int anz;
   eq_list eql;

   /* 1) Funktionssymbole zaehlen */
   for (p=get_sig(), anz=0; p ; p = sig_rest(p),anz++); 

   AnzahlSymbole = anz;
   
   /* 2) Felder allokieren */
   LFeld = our_alloc(anz*sizeof(kodierung_typ));
   RFeld = our_alloc(anz*sizeof(kodierung_typ));

   Stelligkeit = our_alloc(anz*sizeof(kodierung_typ));

   AnzVorkGleichungen = our_alloc(anz*sizeof(kodierung_typ));
   PositionenGleichungen = our_alloc(anz*sizeof(kodierung_typ));      

   AnzVorkZiele = our_alloc(anz*sizeof(kodierung_typ));
   PositionenZiele = our_alloc(anz*sizeof(kodierung_typ));
 
   KombiKontextGleichungenEinzeln = our_alloc(anz*sizeof(kodierung_typ));
   KombiKontextGleichungenEinzelnGegenueber = our_alloc(anz*sizeof(kodierung_typ));
   KombiKontextGleichungen= our_alloc(anz*sizeof(kodierung_typ));
   KombiKontextZieleEinzeln = our_alloc(anz*sizeof(kodierung_typ));
   KombiKontextZieleEinzelnGegenueber = our_alloc(anz*sizeof(kodierung_typ));
   KombiKontextZiele= our_alloc(anz*sizeof(kodierung_typ));

   FinaleBewertung = our_alloc(anz*sizeof(kodierung_typ));

   /* 3) Felder initialisieren */
   for (p=get_sig(), anz=0; p ; p = sig_rest(p),anz++){
     /*     prunsigned long intf("%d) Code=%d\n", anz, sig_first(p));*/
     Stelligkeit[sig_first(p)-1] = sig_arity(sig_first(p));
     AnzVorkGleichungen[sig_first(p)-1] 
       = LFeld[sig_first(p)-1] 
       = RFeld[sig_first(p)-1] 
       = AnzVorkZiele[sig_first(p)-1] 
       = PositionenGleichungen[sig_first(p)-1] 
       = PositionenZiele[sig_first(p)-1] 
       = KombiKontextGleichungenEinzeln[sig_first(p)-1] 
       = KombiKontextGleichungenEinzelnGegenueber[sig_first(p)-1] 
       = KombiKontextGleichungen[sig_first(p)-1] 
       = KombiKontextZieleEinzeln[sig_first(p)-1] 
       = KombiKontextZieleEinzelnGegenueber[sig_first(p)-1] 
       = KombiKontextZiele[sig_first(p)-1] 
       = FinaleBewertung[sig_first(p)-1] 
       = 0;
   }
   /* 4) Erster Durchlauf durch Ziele und Gleichungen:
      Vorkommens- und positionsabhaengige Daten berechnen
      */
   
   for (eql=get_conclusions(); eql; eql=eq_rest(eql)){
     SN_AnzahlZiele++;
     VorkommensDatenAusTermSammeln(eq_first_lhs(eql), LFeld);
     VorkommensDatenAusTermSammeln(eq_first_rhs(eql), RFeld);
     for (p=get_sig(); p ; p = sig_rest(p)){
       AnzVorkZiele[sig_first(p)-1] += RFeld[sig_first(p)-1] +LFeld[sig_first(p)-1];
       LFeld[sig_first(p)-1] = RFeld[sig_first(p)-1] = 0;
     }
     PositionsDatenAusTermSammeln(eq_first_lhs(eql), LFeld,1,0);
     PositionsDatenAusTermSammeln(eq_first_rhs(eql), RFeld,1,0);
     for (p=get_sig(); p ; p = sig_rest(p)){
       PositionenZiele[sig_first(p)-1] += RFeld[sig_first(p)-1] +LFeld[sig_first(p)-1];
       LFeld[sig_first(p)-1] = RFeld[sig_first(p)-1] = 0;
     }
   }
   for (eql=get_equations(); eql; eql=eq_rest(eql)){
     SN_AnzahlGleichungen++;
     VorkommensDatenAusTermSammeln(eq_first_lhs(eql), LFeld);
     VorkommensDatenAusTermSammeln(eq_first_rhs(eql), RFeld);
     for (p=get_sig(); p ; p = sig_rest(p)){
       AnzVorkGleichungen[sig_first(p)-1] += RFeld[sig_first(p)-1] +LFeld[sig_first(p)-1];
       LFeld[sig_first(p)-1] = RFeld[sig_first(p)-1] = 0;
     }
     
     PositionsDatenAusTermSammeln(eq_first_lhs(eql), LFeld,1,0);
     PositionsDatenAusTermSammeln(eq_first_rhs(eql), RFeld,1,0);
     for (p=get_sig(); p ; p = sig_rest(p)){
       PositionenGleichungen[sig_first(p)-1] += RFeld[sig_first(p)-1] +LFeld[sig_first(p)-1];
       LFeld[sig_first(p)-1] = RFeld[sig_first(p)-1] = 0;
     }
   }

   
   /* 5) Zweiter Durchlauf durch Ziele und Gleichungen:
         Kontextabhaengige Daten auf Basis von Vorkommen und Positionen berechnen
      */
   
   KontextBewertungAusfuehren(equations,Gleichungen)
   KontextBewertungAusfuehren(conclusions,Ziele)

   /* 6) Dritter Durchlauf durch Ziele und Gleichungen:
         Kontextdaten aus 2. Durchlauf verwenden
      */

   FinaleBewertungAusfuehren(equations)
   FinaleBewertungAusfuehren(conclusions)
     
}
 
/*****************************************************************************************/
/* 3) Daten auswerten                                                                    */
/*****************************************************************************************/

#define DatenVergleichMin(datum)                      \
  if (resultat==Gleich)                               \
    resultat = (datum[f-1]>datum[g-1])?               \
      Groesser:(datum[f-1]<datum[g-1])?               \
      Kleiner:Gleich;                                 \
  else return resultat;

#define DatenVergleichMax(datum)                      \
  if (resultat==Gleich)                               \
    resultat = (datum[f-1]<datum[g-1])?               \
      Groesser:(datum[f-1]>datum[g-1])?               \
      Kleiner:Gleich;                                 \
  else return resultat;

static VergleichsT SymbolVergleich(func_symb f, func_symb g)
{
  VergleichsT resultat = Gleich;

  DatenVergleichMax(AnzVorkGleichungen);
  DatenVergleichMin(Stelligkeit);
  DatenVergleichMin(PositionenGleichungen);
  DatenVergleichMin(PositionenZiele);
  DatenVergleichMin(KombiKontextGleichungenEinzeln);
  DatenVergleichMin(KombiKontextGleichungenEinzelnGegenueber);
  DatenVergleichMin(KombiKontextGleichungen);
  DatenVergleichMin(KombiKontextZieleEinzeln);
  DatenVergleichMin(KombiKontextZieleEinzelnGegenueber);
  DatenVergleichMin(KombiKontextZiele);
  DatenVergleichMin(FinaleBewertung);
  return resultat;
}

static void   SymboldatenAusgeben(void)
{
  func_symb_list p1,p2;
  BOOLEAN IstEindeutig = TRUE, SkolemFall = FALSE, NichtSkolemFall = FALSE;
#if SN_TEST
  func_symb_list p;
  pr(( "\n\nAnzahl Symbole: %d\n", AnzahlSymbole));
  pr(( "\n        |Gleichungen                           |Ziele                         |Finale"));
  pr(( "\nSymb|tau| #G| POS    |   KOKOE|   KOKOZ|#Z |    POS |   KOKOE|   KOKOZ|Bew."));
  pr(( "\n----+---+---+--------+--------+--------+---+--------+--------+--------+---------\n"));
  for (p=get_sig(); p ; p = sig_rest(p)){
    char *nom = map_func(sig_first(p));
    /*               tau     #G   POS    KOK   KOK  #Z    POS   KOK   KOK   FBew */
    pr(( "%c%c%c%c|%8lu|%8lu|%8lu|%8lu|%8lu|%8lu|%8lu|%8lu|%8lu|%8lu\n", 
           nom[0],
           strlen(nom)>1?nom[1]:' ',
           strlen(nom)>2?nom[2]:' ',
           strlen(nom)>3?nom[3]:' ',
           Stelligkeit[sig_first(p)-1],
           AnzVorkGleichungen[sig_first(p)-1],
           PositionenGleichungen[sig_first(p)-1],
           KombiKontextGleichungenEinzeln[sig_first(p)-1],
           KombiKontextGleichungen[sig_first(p)-1],
           AnzVorkZiele[sig_first(p)-1],
           PositionenZiele[sig_first(p)-1],
           KombiKontextZieleEinzeln[sig_first(p)-1],
           KombiKontextZiele[sig_first(p)-1],
           FinaleBewertung[sig_first(p)-1]));
  }

  pr(( "\n  "));
  for (p1=get_sig(); p1 ; p1 = sig_rest(p1))
    pr(( "%c ", *map_func(sig_first(p1))));
  pr(( "\n"));
#endif
  for (p1=get_sig(); p1 ; p1 = sig_rest(p1)){
    pr(( "%c ", *map_func(sig_first(p1))));
    for (p2=get_sig(); p2 ; p2 = sig_rest(p2)){
      if (sig_first(p1)==sig_first(p2)){
        pr(( "= "));
        continue;
      }
      switch (SymbolVergleich(sig_first(p1), sig_first(p2))){
      case Gleich: 
        pr(( "= "));
        IstEindeutig = FALSE;
        if ((Stelligkeit[sig_first(p1)-1] == 0)
            && (Stelligkeit[sig_first(p2)-1] == 0)
            && (AnzVorkGleichungen[sig_first(p1)-1] == 0)
            && (AnzVorkGleichungen[sig_first(p2)-1] == 0))
          SkolemFall = TRUE;
        else
          NichtSkolemFall = TRUE;
        break;
      case Kleiner: pr(( "< "));break;
      case Groesser: pr(( "> "));break;
      case Unvergleichbar: pr(( "# "));break;
      default: break;
      }
    }
    pr(( "\n"));
  }
  pr(( "\n\n*****%sindeutig%s*****\n\n", IstEindeutig?"E":"Nicht e", 
         !IstEindeutig? NichtSkolemFall? "": (SkolemFall?"(Skol)": "(error)" ):"" ));
}

/* 
   0) Reihenfolge der Symbole festlegen
      Zugriff: PositionVon(Symbol) und SymbolNr(Position)
      
   1) Links-Rechts-Sortierung von Gleichungen
      a) Variablen mit Positionsmethode verarbeiten
      b) Lexikographischer Term-Vergleich mit Symbol- und Variablen-Nummern

   2) Variablen-Normierung der Gleichungen

   3) Reihenfolge der Gleichungen
      Lexikographischer Vergleich von Gleichungen
 */
/*****************************************************************************************/
/* 4) Reihenfolge der Symbole festlegen                                                  */
/*****************************************************************************************/


static int *SymbNr;
/* exportiert */
int *SN_PosVon;
#define PositionVon(symbol) SN_PosVon[symbol-1]
#define SymbolNr(position) SymbNr[position]

/* exportiert */
term *SN_LHS, *SN_RHS;
struct SN_ZieleS *SN_Ziele;

static void SymbolReihenfolgeFestlegen(void)
{
  func_symb_list p;
  int i,j,k;

  for (i=0,p=get_sig(); p ; p = sig_rest(p),i++)
    SymbolNr(i) = sig_first(p);
  
  for (i=1;i<AnzahlSymbole; i++){
    for(j=0; j<AnzahlSymbole-i;j++)
      if (SymbolVergleich(SymbolNr(j),SymbolNr(j+1)) == Groesser){
        func_symb temp = SymbolNr(j);
        SymbolNr(j) = SymbolNr(j+1);
        SymbolNr(j+1) = temp;
      }
  }
  pr(( "Symbols: "));
  for (k = 0; k < AnzahlSymbole; k++){
    pr(( "%s  ", map_func(SymbolNr(k))));
    PositionVon(SymbolNr(k)) = k;
  }
  pr(( "\n"));
  for (p=get_sig(); p ; p = sig_rest(p))
    pr(( "%s auf Pos. %d\n", map_func(sig_first(p)), PositionVon(sig_first(p))));
}

/*****************************************************************************************/
/* 5) Gleichungen anordnen                                                               */
/*****************************************************************************************/

static VergleichsT LRSortierenRek(term lhs, term rhs)
     /* Zunaechst Variablen als Joker behandeln */
{
  VergleichsT resultat;
  if (variablep(lhs) && variablep(rhs)) return Gleich;
  if (variablep(lhs)) return Kleiner;
  if (variablep(rhs)) return Groesser;
  if ((resultat=SymbolVergleich(term_func(lhs), term_func(rhs)))==Gleich){
    int i = 0;
    if (sig_arity(term_func(lhs))>0){
      do{
        resultat=LRSortierenRek(term_arg(lhs,i),term_arg(rhs,i));
        i++;
      }while((resultat==Gleich) && (i<sig_arity(term_func(lhs))));
    }
  }
  return resultat;
}    
      
static void LRSortieren(void)
     /* Alle Gleichungen LR-Sortieren */
{
  BOOLEAN ok=TRUE;
  int i=0;
  eq_list eql;
  for(eql=get_equations();eql;eql=eq_rest(eql)){
    switch(LRSortierenRek(eq_first_lhs(eql), eq_first_rhs(eql))){
    case Groesser:              /* l>r --> tauschen */
      SN_LHS[i] = eq_first_rhs(eql); SN_RHS[i] = eq_first_lhs(eql);
      prp(eq_first_rhs(eql), " X ", eq_first_lhs(eql));
      break;
    case Kleiner:               /* l<r --> nicht tauschen */
      SN_LHS[i] = eq_first_lhs(eql); SN_RHS[i] = eq_first_rhs(eql);
      prp(eq_first_lhs(eql), " || ", eq_first_rhs(eql));
      break;
    case Gleich: 
      if (!((sig_arity(term_func(eq_first_lhs(eql))) ==2) /* kein AC-Symbol?? */
            && (term_func(eq_first_lhs(eql))==term_func(eq_first_rhs(eql)))
            && (variablep(term_arg(eq_first_lhs(eql),0)))
            && (variablep(term_arg(eq_first_lhs(eql),1)))
            && (variablep(term_arg(eq_first_rhs(eql),0)))
            && (variablep(term_arg(eq_first_rhs(eql),1)))
            && (term_func(term_arg(eq_first_lhs(eql),0)) == term_func(term_arg(eq_first_rhs(eql),1)))
            && (term_func(term_arg(eq_first_lhs(eql),1)) == term_func(term_arg(eq_first_rhs(eql),0))))){
        pr(( "Nicht eindeutig: "));
        prp(eq_first_lhs(eql), " = ", eq_first_rhs(eql));
      }
      else{
        pr(( "Nicht eindeutig(AC): "));
        prp(eq_first_lhs(eql), " = ", eq_first_rhs(eql));
      }
      SN_LHS[i] = eq_first_lhs(eql); SN_RHS[i] = eq_first_rhs(eql);
      ok=FALSE;break;
    default: 
      SN_LHS[i] = eq_first_lhs(eql); SN_RHS[i] = eq_first_rhs(eql);
      pr(( "error: Nicht vergleichbare Gleichung\n"));ok=FALSE;break;
    }
    i++;
  }

  /* noch einmal fuer Ziele, Th. */
  for(i=0,eql=get_conclusions();eql;eql=eq_rest(eql)){
    switch(LRSortierenRek(eq_first_lhs(eql), eq_first_rhs(eql))){
    case Groesser:              /* l>r --> tauschen */
      SN_Ziele[i].l = eq_first_rhs(eql); SN_Ziele[i].r = eq_first_lhs(eql);
      prp(eq_first_rhs(eql), " X ", eq_first_lhs(eql));
      break;
    case Kleiner:               /* l<r --> nicht tauschen */
      SN_Ziele[i].l = eq_first_lhs(eql); SN_Ziele[i].r = eq_first_rhs(eql);
      prp(eq_first_lhs(eql), " || ", eq_first_rhs(eql));
      break;
    case Gleich: 
      if (!((sig_arity(term_func(eq_first_lhs(eql))) ==2) /* kein AC-Symbol?? */
            && (term_func(eq_first_lhs(eql))==term_func(eq_first_rhs(eql)))
            && (variablep(term_arg(eq_first_lhs(eql),0)))
            && (variablep(term_arg(eq_first_lhs(eql),1)))
            && (variablep(term_arg(eq_first_rhs(eql),0)))
            && (variablep(term_arg(eq_first_rhs(eql),1)))
            && (term_func(term_arg(eq_first_lhs(eql),0)) == term_func(term_arg(eq_first_rhs(eql),1)))
            && (term_func(term_arg(eq_first_lhs(eql),1)) == term_func(term_arg(eq_first_rhs(eql),0))))){
        pr(( "Nicht eindeutig: "));
        prp(eq_first_lhs(eql), " = ", eq_first_rhs(eql));
      }
      else{
        pr(( "Nicht eindeutig(AC): "));
        prp(eq_first_lhs(eql), " = ", eq_first_rhs(eql));
      }
      SN_Ziele[i].l = eq_first_lhs(eql); SN_Ziele[i].r = eq_first_rhs(eql);
      ok=FALSE;break;
    default: 
      SN_Ziele[i].l = eq_first_lhs(eql); SN_Ziele[i].r = eq_first_rhs(eql);
      pr(( "error: Nicht vergleichbare Gleichung\n"));ok=FALSE;break;
    }
    i++;
  }
}

static int *varlist;
static int AktVarNummer;

static void TermVariablenNormieren(term Term)
{
  if(variablep(Term)){
    TEST(pr(( "Variable %d ", (int) Term->code));)
    if (varlist[-term_func(Term)] == 1){ /* erstes Vorkommen dieser Variablen */
      varlist[-term_func(Term)] = -AktVarNummer;
      AktVarNummer++;
      TEST(pr(( "(1.Vorkommen)"));)
    }
    Term->code = varlist[-term_func(Term)];
    TEST(pr(( " Term->code (neu) = %d\n", (int) Term->code));)
  }
  else{
    int j=0;
    while(j<sig_arity(term_func(Term)))
      TermVariablenNormieren(term_arg(Term,j++));
  }
}
  
static void GleichungenVariablenNormieren(void)
     /* Alle Gleichungen Variablen-Normieren */
{
  eq_list eql;
  int j=0;
  varlist = our_alloc((Variablenzaehler+1)*sizeof(func_symb));
  for(eql=get_equations();eql;eql=eq_rest(eql)){
    int i;
    for (i=0; i<Variablenzaehler+1;i++) varlist[i] = 1; /* Feld initialisieren bis max. Variablen-Index*/
    TEST(pr(( "Normiere Gleichung "));)
    TEST(prp(SN_LHS[j],"=",SN_RHS[j]);)
    AktVarNummer=1;
    TermVariablenNormieren(SN_LHS[j]);
    TermVariablenNormieren(SN_RHS[j]);
    j++;
  }
  TEST(pr(( "\n\n"));)
  for(j=0,eql=get_conclusions();eql;eql=eq_rest(eql)){
    int i;
    for (i=0; i<Variablenzaehler+1;i++) varlist[i] = 1; /* Feld initialisieren bis max. Variablen-Index*/
    TEST(pr(( "Normiere Gleichung "));)
    TEST(prp(SN_Ziele[j].l,"=",SN_Ziele[j].r);)
    AktVarNummer=1;
    TermVariablenNormieren(SN_Ziele[j].l);
    TermVariablenNormieren(SN_Ziele[j].r);
    j++;
  }
  our_dealloc(varlist);
}

static VergleichsT LRSymbSortierenRek(term SN_LHS, term SN_RHS)
     /* 2 variablen-normierte Terme vergleichen (Variablen _nicht_ als Joker) */
{
  int i=0;
  VergleichsT resultat=Gleich;
  pr(( "LRSymbSortierenRek aufgerufen mit "));
  prp(SN_LHS,"=",SN_RHS);
  if (variablep(SN_LHS) && variablep(SN_RHS)){
     if (SN_LHS->code < SN_RHS->code) return Kleiner;
     else if (SN_LHS->code > SN_RHS->code) return Groesser;
     else return Gleich;
  }
  if (variablep(SN_LHS)) return Kleiner;
  if (variablep(SN_RHS)) return Groesser;
  
  if (PositionVon(term_func(SN_LHS)) < PositionVon(term_func(SN_RHS))) return Kleiner;
  if (PositionVon(term_func(SN_LHS)) > PositionVon(term_func(SN_RHS))) return Groesser;
  
  while ((i<sig_arity(term_func(SN_LHS)))&&(resultat==Gleich)){
    resultat = LRSymbSortierenRek(term_arg(SN_LHS,i),term_arg(SN_RHS,i));
    i++;
  }
  return resultat;
}
  
 
static void GleichungenSortieren(void)
     /* Alle Gleichungen in pseudo-eindeutiger Art sortieren.
        Am Ende stehen sind die Gleichungen in (SN_LHS[i],SN_RHS[i]) zu finden.*/
{
  int i,j;
  for (i=1;i<SN_AnzahlGleichungen; i++){
    for(j=0; j<SN_AnzahlGleichungen-i;j++)
      switch (LRSymbSortierenRek(SN_LHS[j],SN_LHS[j+1])){
      case Groesser:
        {
          term temp = SN_LHS[j];
          SN_LHS[j]=SN_LHS[j+1];
          SN_LHS[j+1]=temp;
          temp = SN_RHS[j];
          SN_RHS[j]=SN_RHS[j+1];
          SN_RHS[j+1]=temp;
        }
      break;
      case Gleich: 
        if(LRSymbSortierenRek(SN_RHS[j],SN_RHS[j+1])==Groesser){
          term temp = SN_LHS[j];
          SN_LHS[j]=SN_LHS[j+1];
          SN_LHS[j+1]=temp;
          temp = SN_RHS[j];
          SN_RHS[j]=SN_RHS[j+1];
          SN_RHS[j+1]=temp;
        }
        break;
      default:
        break;
      }
  }
  for(j=0; j<SN_AnzahlZiele;j++)    prp(SN_LHS[j],"=",SN_RHS[j]);
  for (i=1;i<SN_AnzahlZiele; i++){
    for(j=0; j<SN_AnzahlZiele-i;j++)
      switch (LRSymbSortierenRek(SN_Ziele[j].l,SN_Ziele[j+1].l)){
      case Groesser:
        {
          term temp = SN_Ziele[j].l;
          SN_Ziele[j].l=SN_Ziele[j+1].l;
          SN_Ziele[j+1].l=temp;
          temp = SN_Ziele[j].r;
          SN_Ziele[j].r=SN_Ziele[j+1].r;
          SN_Ziele[j+1].r=temp;
        }
      break;
      case Gleich: 
        if(LRSymbSortierenRek(SN_Ziele[j].r,SN_Ziele[j+1].r)==Groesser){
          term temp = SN_Ziele[j].l;
          SN_Ziele[j].l=SN_Ziele[j+1].l;
          SN_Ziele[j+1].l=temp;
          temp = SN_Ziele[j].r;
          SN_Ziele[j].r=SN_Ziele[j+1].r;
          SN_Ziele[j+1].r=temp;
        }
        break;
      default:
        break;
      }
  }
}

static BOOLEAN NichtGrundterm(term tt)                               /* Zum Erkennen existenzquantifizierter Ziele */
{
  if variablep(tt)
    return TRUE;
  else {
    int runny;
    for (runny=0; runny<sig_arity(term_func(tt)); runny++)
      if (NichtGrundterm(term_arg(tt,runny)))
        return TRUE;
    return FALSE;
  }
}


static void ExistenzzieleSuchen(void)
{
  eq_list ziele = get_conclusions();
  PM_Existenzziele = FALSE;
  while (ziele) {
    if (NichtGrundterm(eq_first_lhs(ziele)) || NichtGrundterm(eq_first_rhs(ziele))) {
      PM_Existenzziele = TRUE;
      return;
    }
    ziele = eq_rest(ziele);
  }
}

/*****************************************************************************************/
/* 6) Exportierte Funktion                                                               */
/*****************************************************************************************/

void SN_SpezifikationAuswerten(BOOLEAN Normiere)
{
  /* globale Variablen initialisieren */
  SN_AnzahlGleichungen = SN_AnzahlZiele = 0;
  Variablenzaehler = 0;
  
  SymboldatenSammeln();         /* In beiden Faellen benoetigt! */
  SymboldatenAusgeben();

  SN_PosVon = array_alloc(AnzahlSymbole,int);
  SymbNr    = array_alloc(AnzahlSymbole,int);
  SN_LHS    = array_alloc(SN_AnzahlGleichungen,term);
  SN_RHS    = array_alloc(SN_AnzahlGleichungen,term);
  SN_Ziele  = array_alloc(SN_AnzahlZiele,struct SN_ZieleS);

  if (Normiere) {
    SymbolReihenfolgeFestlegen();
    LRSortieren();
    output_array("LRSort->VarNorm");
    GleichungenVariablenNormieren();
    output_array("VarNorm->Esort");
    GleichungenSortieren();
    output_array("Esort->");
    pr(( " normiert"));
  }
  else{
    func_symb_list p;
    int k,i=0;
    eq_list eql;
    
    /* Symbole so uebernehmen wie sie in der Spezifikation stehen */
    for (i=0,p=get_sig(); p ; p = sig_rest(p),i++) 
      SymbolNr(i) = sig_first(p);
    
    pr(( "Symbols: "));
    for (k = 0; k < AnzahlSymbole; k++){
      pr(( "%s  ", map_func(SymbolNr(k))));
      PositionVon(SymbolNr(k)) = k;
    }

    /* Gleichungen einfach uebernehmen */
    eql=get_equations();i=0;
    while(eql){
      SN_LHS[i] = eq_first_lhs(eql); 
      SN_RHS[i] = eq_first_rhs(eql);
      eql=eq_rest(eql);
      i++;
    }
    /* dito Ziele, Th. */
    eql=get_conclusions();i=0;
    while(eql){
      SN_Ziele[i].l = eq_first_lhs(eql); 
      SN_Ziele[i].r = eq_first_rhs(eql);
      eql=eq_rest(eql);
      i++;
    }
    pr(( " nicht normiert"));
  }
  pr(( "\n"));
  ExistenzzieleSuchen();
}
