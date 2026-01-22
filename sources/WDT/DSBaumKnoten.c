/*=================================================================================================================
  =                                                                                                               =
  =  DSBaumKnoten.c                                                                                               =
  =                                                                                                               =
  =  Anlegen, freigeben, auslesen und beschreiben von Regelbaumknoten                                             =
  =                                                                                                               =
  =                                                                                                               =
  =  17.03.94 Nach einigen Geburtswehen.                                                                          =
  =                                                                                                               =
  =================================================================================================================*/

#include "Ausgaben.h"
#include "DSBaumKnoten.h"
#include "FehlerBehandlung.h"
#include "SpeicherVerwaltung.h"


/*=================================================================================================================*/
/*= II. Spezielle Zugriffsfunktionen fuer innere Knoten                                                           =*/
/*=================================================================================================================*/

unsigned int BK_AnzNachfolger(KantenT Knoten)                   /* Anzahl der existierenden Nachfolger des Knotens */
{
  unsigned int Anzahl = 0;
  SymbolT SymbolIndex;
  SO_forSymboleAufwaerts(SymbolIndex,BK_minSymb(Knoten),BK_maxSymb(Knoten))
    if (BK_Nachf(Knoten,SymbolIndex))
      Anzahl++;
  return Anzahl;
}



/*=================================================================================================================*/
/*= IV. Gemeinsame Zugriffsfunktionen                                                                             =*/
/*=================================================================================================================*/

void BK_NachfolgenLassen(KantenT Kante, SymbolT Symbol, KantenT NachfKante)
{
  BK_NachfSei(Kante,Symbol,NachfKante);
  BK_VorgSei(NachfKante,Kante);
  BK_VorgSymbSei(NachfKante,Symbol);
}



/*=================================================================================================================*/
/*= V. Speicherverwaltung fuer Regelbaum                                                                          =*/
/*=================================================================================================================*/

typedef struct {                                                     /* Typ-Definitionen fuer Groessen-Bestimmung. */
  InnereUndBlattObjekte;
  InnererKnotenT Innen;
}_RKT;

typedef struct {
  InnereUndBlattObjekte;
  BlattKnotenT Blatt;
}_RBT;


static SV_SpeicherManagerT InnereKnotenManager,
                           BlattManager,
                           SprungManager,
                           PraefixManager;

unsigned long int BK_AnzEinzelnAllokierterNachfolgerArrays = 0;

void BK_InitApriori(void)                  /* Initialisiert die Speicherverwaltungen fuer Blaetter, innere Knoten, */
{                                                                              /* Sprungeintraege, Suffixeintraege */
  if ((sizeof(KnotenT) != sizeof(_RBT)) && (sizeof(KnotenT) != sizeof(_RKT)))
    our_error("Groessenverhaeltnisse stimmen nicht in BK_InitApriori!\n");
  SV_ManagerInitialisieren(&InnereKnotenManager,_RKT,BK_AnzInnereKnotenJeBlock,"Innere Knoten");
  SV_ManagerInitialisieren(&BlattManager,_RBT,BK_AnzBlaetterJeBlock,"Blaetter");
  SV_ManagerInitialisieren(&SprungManager,struct SprungeintragsTS,BK_AnzSprungeintraegeJeBlock,"Sprungeintraege");
  SV_ManagerInitialisieren(&PraefixManager,struct PraefixeintragsTS,BK_AnzPraefixeintraegeJeBlock,"Praefixeintraege");
}


static SV_SpeicherManagerT *FreieNachfolgerArrayManager,
                           *AnfFreieNachfolgerArrayManager;

static BOOLEAN BK_SchonInitialisiert = FALSE;

static unsigned int ErsteFunktionsanzahl,
                    Funktionendelta;

void BK_InitAposteriori(void)                  /* Initialisiert die Speicherverwaltung fuer die Nachfolger-Arrays. */
{
  if (BK_SchonInitialisiert) {
    Funktionendelta = SO_AnzahlFunktionen>ErsteFunktionsanzahl ? SO_AnzahlFunktionen-ErsteFunktionsanzahl : 0;
    FreieNachfolgerArrayManager = AnfFreieNachfolgerArrayManager+Funktionendelta;
  }
  else {
    int i;
    Funktionendelta = 0;
    AnfFreieNachfolgerArrayManager = FreieNachfolgerArrayManager =
      array_alloc(BK_VariablenZahlFuerAnzulegendeKnoten,SV_SpeicherManagerT);
    for (i=0; i<BK_VariablenZahlFuerAnzulegendeKnoten; i++) {
      FreieNachfolgerArrayManager[i].block_size = 0;                                  /* Initialisierung erzwingen */
      SV_BeliebigenManagerInitialisieren(&(FreieNachfolgerArrayManager[i]),
        sizeof(KantenT)*(SO_AnzahlFunktionen+i+1),              /* ElementGroesse, ('+1', weil immer >= eine Var */
        BK_AnzahlNachfolgerArraysJeBlock,                                 /* Anzahl anzulegender Elemente je Block */
        FALSE,"Nachfolger-Array");
    }
    ErsteFunktionsanzahl = SO_AnzahlFunktionen;
    BK_SchonInitialisiert = TRUE;
  }
}


static KantenT *NachfolgerArrayAnlegen(unsigned int VariablenZahl)     /* Legt ein Nachfolger-Array mit Platz fuer
            die Funktionssymbole und VariablenZahl Variablen an. Dieses Array wird bereits mit NULL initialisiert. */
{
  KantenT *erg;
  SymbolT i;
  
  if (VariablenZahl>BK_VariablenZahlFuerAnzulegendeKnoten-Funktionendelta) {
         /* 03.04.95, Neu: kein Realloc mehr, uebergrosse Nachfolger-Arrays werden direkt ueber our_alloc angelegt */
    erg = our_alloc(sizeof(KantenT)*(SO_AnzahlFunktionen+VariablenZahl));
    BK_AnzEinzelnAllokierterNachfolgerArrays++;
  }
  else
    SV_Alloc(erg,FreieNachfolgerArrayManager[VariablenZahl-1],KantenT *);     /* Array beginnt bei 0 u. VarZahl >0 */
  for (i=0; i<(signed)(SO_AnzahlFunktionen+VariablenZahl);i++)
    erg[i]=NULL;
  return erg;
}



/*=================================================================================================================*/
/*= VI. Anlegen und Freigeben der verschiedenen Objekte                                                            */
/*=================================================================================================================*/

SprungeintragsT BK_SprungeintragAnlegen(void)        /* Legt einen Sprungeintrag an und gibt Zeiger darauf zurueck.*/
{
  SprungeintragsT Ergebnis;
  SV_Alloc(Ergebnis,SprungManager,SprungeintragsT);
  return Ergebnis;
}


void BK_SprungeintragFreigeben(SprungeintragsT Sprungeintrag)             /* Gibt einen Sprungeintrag wieder frei. */
{
  SV_Dealloc(SprungManager,Sprungeintrag);
}


PraefixeintragsT BK_PraefixeintragAnlegen(void)     /* Legt einen Praefixeintrag an und gibt Zeiger darauf zurueck.*/
{
  PraefixeintragsT Ergebnis;
  SV_Alloc(Ergebnis,PraefixManager,PraefixeintragsT);
  return Ergebnis;
}


void BK_PraefixeintragFreigeben(PraefixeintragsT Praefixeintrag)         /* Gibt einen Praefixeintrag wieder frei. */
{
  SV_Dealloc(PraefixManager,Praefixeintrag);
}


KantenT BK_BlattAnlegen(void)   /* Legt ein Blatt fuer den Regelbaum an und gibt Zeiger darauf als KantenT zurueck.*/
{
  KantenT blatt;
  SV_Alloc(blatt,BlattManager,KantenT);
  BK_IstBlattSei(blatt, TRUE);
  BK_SprungeingaengeSei(blatt,NULL);
  return blatt;
}


void BK_BlattFreigeben(KantenT blatt)                    /* Gibt ein mit BlattAnlegen angelegtes Blatt wieder frei.*/
{
  SV_Dealloc(BlattManager,blatt);
}


KantenT BK_KnotenAnlegen(unsigned int VariablenZahl)
/* Legt einen Knoten mit Platz fuer VariablenZahl Variablen an.
   Die entsprechenden Anpassungen fuer Variablenzahlen, die noch nicht
   vorkommen, werden dabei automatisch durchgefuehrt.
   Annahme/Voraussetzung: VariablenZahl > 0.*/
{
  KantenT erg;
  SV_Alloc(erg,InnereKnotenManager,KantenT);
  BK_IstBlattSei(erg,FALSE);                                                                         /* kein Blatt */
  BK_AnfNachfSei(erg,NachfolgerArrayAnlegen(VariablenZahl));
  BK_FeldNachfSei(erg,BK_AnfNachf(erg)+VariablenZahl);                                      /* Pointer-Arithmetik! */
  BK_SprungeingaengeSei(erg,NULL);
  BK_SprungausgaengeSei(erg,NULL);
  return erg;
}


void BK_KnotenFreigeben(KantenT Knoten)             /* Gibt einen mit KnotenAnlegen angelegten Knoten wieder frei. */
{
  unsigned int VariablenZahl = BK_AnzVariablen(Knoten);                              /* Anzahl Variablen bestimmen */
  if (VariablenZahl>BK_VariablenZahlFuerAnzulegendeKnoten-Funktionendelta)
    our_dealloc(BK_AnfNachf(Knoten));                                  /* Nachfolger-Array freigeben ueber dealloc */
  else
    SV_Dealloc(FreieNachfolgerArrayManager[VariablenZahl-1],BK_AnfNachf(Knoten));         /* oder ueber SV_Dealloc */
  SV_Dealloc(InnereKnotenManager,Knoten);                                         /* schliesslich Knoten freigeben */
}



/*=================================================================================================================*/
/*= VII. Zugriffe auf in Baeumen gespeicherte Objekte (Regeln und Gleichungen)                                     */
/*=================================================================================================================*/

static KantenT iBlatt;
static GNTermpaarT iTermpaar;

GNTermpaarT BK_ErstesTermpaar(DSBaumT Baum)
{
  return (iTermpaar = ((iBlatt = Baum.ErstesBlatt) ? BK_Regeln(iBlatt) : NULL));
}

GNTermpaarT BK_NaechstesTermpaar(void)
{
  return 
    (iTermpaar = 
     (TP_Nachf(iTermpaar) ? TP_Nachf(iTermpaar) 
      : ((iBlatt = BK_NachfBlatt(iBlatt)) ? BK_Regeln(iBlatt) 
         : NULL)));
}

GNTermpaarT BK_ErstesTermpaarM(DSBaumT Baum, KantenT *iBlatt, GNTermpaarT *iTermpaar)
{
  return (*iTermpaar = ((*iBlatt = Baum.ErstesBlatt) ? BK_Regeln(*iBlatt) : NULL));
}

GNTermpaarT BK_NaechstesTermpaarM(KantenT *iBlatt, GNTermpaarT *iTermpaar)
{
  return 
    (*iTermpaar = 
     (TP_Nachf(*iTermpaar) ? TP_Nachf(*iTermpaar) 
      : ((*iBlatt = BK_NachfBlatt(*iBlatt)) ? BK_Regeln(*iBlatt) 
         : NULL)));
}
