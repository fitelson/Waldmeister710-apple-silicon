/* -------------------------------------------------------------------------------------------------------------*/
/* -                                                                                                            */
/* - NFBildung.c                                                                                                */
/* - ***********                                                                                                */
/* -                                                                                                            */
/* - Enthaelt die Operationen zur Normalform-Bildung (normales Verfahren)                                       */
/* -                                                                                                            */
/* - 23.03.94 Arnim. Angelegt.                                                                                  */
/* -                                                                                                            */
/* -                                                                                                            */
/* -                                                                                                            */
/* -------------------------------------------------------------------------------------------------------------*/

#include "Ausgaben.h"
#include "FehlerBehandlung.h"
#include "MatchOperationen.h"
#include "NFBildung.h"
#include "RUndEVerwaltung.h"
#include "TermOperationen.h"
#include "compiler-extra.h"
#include "general.h"
#include <string.h>

#ifndef CCE_Source
#include "BeweisObjekte.h"
#include "Statistiken.h"
#include "Parameter.h"
#else
#define IncAnzRedVersMitRegeln() (void) 0
#define IncAnzRedVersMitGleichgn() (void) 0
#define IncAnzRedVers(x) /* nix tun */
#define IncAnzRedErf(x) /* nix tun */
#define BOB_NFCallback(x,y,z) /* nix tun */
#endif

typedef unsigned short StellenT;
#define MaxStelle USHRT_MAX

#define VERBOSE 1
extern BOOLEAN drucken;

#if DO_FLAGS
#define DO_REALLY 1
#endif

#define NFB_TEST 0                                                                                         /* 0, 1 */

#if NFB_TEST
#define pr(x) IO_DruckeFlex x
#else
#define pr(x)
#endif

#define NF_PfadArrayGroesse 20

BOOLEAN (*NF_Normalform)(BOOLEAN doR, BOOLEAN doE, TermT Term);

/*-------------------------------------------------------------------------------------------------------------------------*/
/*------ Typen und Operationen zum Arbeiten auf einem Pfad ----------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------*/
/*                                                                                                                         */
/* Der Reduktions-Pfad                                                                                                     */
/* -------------------                                                                                                     */
/*                                                                                                                         */
/* Bei diesem Pfad      handelt es sich im wesentlichen um einen Stack, der den Durchlauf durch einen Term nach einer      */
/* beliebigen Strategie organisiert.                                                                                       */
/*                                                                                                                         */
/* Neben den verschiedenen Verwaltungs-Informationen, die unten erklaert sind, besteht dieser Pfad aus einem Array, auf    */
/* dem ein Stack simuliert wird. Der am weitesten rechts stehende Eintrag, auf den die Komponente aktuelleStelle verweist, */
/* beinhaltet den gerade behandelten Teilterm. Wird ein neuer Eintrag auf den Stack gelegt, so wird die Stelligkeit des    */
/* entsprechenden Topsymbols sofort als AnzNochZuBehandelnderTeilterme eingetragen, um beim Backtracking die noch nicht    */
/* abgearbeiteten Stellen im Term leichter auffinden zu koennen. Ferner wird der EinfuegePunkt initial auf NULL gesetzt,   */
/* um eine Unterscheidung zwischen neu zu behandelnden und schon teilweise abgearbeiteten Stellen zu ermoeglichen.         */
/*                                                                                                                         */
/* Ein Eintrag auf dem Stack                                                                                               */
/* =========================                                                                                               */
/*                                                                                                                         */

/* Der folgende Datentyp erleichtert das Fuehren der Reduktions-Statistik.*/

typedef struct NFB_PfadZellenTS{                   /* Ein Eintrag im eigentlichen Stack besteht aus:                       */
   UTermT           Stelle;                        /* - der in diesem Eintrag gespeicherten Stelle (Teilterm)              */
   UTermT           EinfuegePunkt;                 /* - derjenigen Termzelle, die nach Reduktion des aktuell behandelten   */
                                                   /*   Nachfolgers ggf. destruktiv geaendert werden muss                  */
   StellenT         AnzNochZuBehandelnderTeilterme;/* - ... s.Name                                                         */
} NFB_PfadZellenT;

static StellenT        aktuelleStelle;                 /* - einen Verweis auf die aktuelle Stelle                          */
static StellenT        aktuelleGroessePfad;            /* - die # der aktuell eingerichteten PfadZellen -1                 */
                                                   /*   (-1, um den Test auf ueberlaufenden Stack zu erleichtern)          */
static NFB_PfadZellenT *NFPfad;                    /* - den eigentlichen Durchlauf-'Stack'                                 */

static StellenT        letzteRed;         /* Verweis auf die letzte reduzierte Stelle   */
static StellenT        hoechsteRed;       /* Verweis auf die hoechste reduzierte Stelle */


static BOOLEAN Reduziert;
/* Diese Variable muss am Anfang jeder NF-Prozedur auf FALSE gesetzt werden. Sie wird in jeder Regel-Angewendet-Prozedur im
   Erfolgsfalle auf TRUE gesetzt und kann dann als Rueckgabewert der NF-Bildungs-Funktion genutzt werden.
   Bei Verwendung des 'normalen' (dh. nicht des Adhoc-Pfades) wird das Flag von PfadNeu automatisch auf FALSE gesetzt.
   
   Das Flag wird von folgenden Prozeduren im Erfolgsfalle auf TRUE gesetzt:
   - NFB_RegelAngewendet
   - NFB_GleichungAngewendet
*/

/*-------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------- PfadEinsWeiterSetzen -----------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------*/
/*                                                                                                                         */
/* Vor.: die aktuelle Stelle im Pfad zeigt auf einen Knoten im Term (Term als Baum interpretiert), dessen Nachfolger noch  */
/*       nicht alle abgehandelt sind. Dann wird ein neuer Eintrag im Pfad erzeugt, der den ersten noch nicht behandelten   */
/*       Teilterm des aktuellen Terms als Term hat; der aktuelle Term ist jetzt dieser Teilterm.                           */
/*                                                                                                                         */
/*-------------------------------------------------------------------------------------------------------------------------*/


static void NFB_PfadEinsWeiterSetzen(void)
/* Der naechste noch zu behandelnde Teilterm an der aktuellen Stelle wird als neue aktuelle Stelle eingerichtet.
   Vor.: aktuelle Stelle zeigt auf einen Eintrag, der noch nicht abgearbeitete Teilterme besitzt. */
{
#if 0

  d e f u n c t ;;;;

   if (aktuelleStelle == aktuelleGroessePfad){            /* Im Pfad kein Platz mehr zum Tiefersteigen => realloc versuchen*/
      if (aktuelleGroessePfad >= MaxStelle - NF_PfadArrayGroesse)
         our_error ("Ueberlauf in StellenT, ggf. anpassen");
      aktuelleGroessePfad += NF_PfadArrayGroesse;
      NFPfad = (NFB_PfadZellenT *) our_realloc(NFPfad, sizeof(NFB_PfadZellenT)*(aktuelleGroessePfad+1),
                                                    "Erweitern des Normalform-Pfades");
      if (Ausdehnungsproz)
         (* Ausdehnungsproz) (aktuelleGroessePfad+1);
   }
   NFPfad[aktuelleStelle].AnzNochZuBehandelnderTeilterme--; /* ein Teilterm wird jetzt behandelt */
   if (NFPfad[aktuelleStelle].EinfuegePunkt == NULL){      /* bisher noch kein Teilterm behandelt */
      NFPfad[aktuelleStelle].EinfuegePunkt =               /* => der behandelte Teilterm faengt direkt hinter Topsymbol an*/
         NFPfad[aktuelleStelle].Stelle;       
   }
   else{                                                     /* es wurde bereits ein Teilterm behandelt */
      NFPfad[aktuelleStelle].EinfuegePunkt =              /* => EinfuegePunkt Ende des zuletzt behandelten Teilterms setzen */
         TO_TermEnde( TO_Schwanz( NFPfad[aktuelleStelle].EinfuegePunkt)); /* (der direkt hinter altem EinfuegePunkt anfing.) */
   }
   NFPfad[aktuelleStelle+1].Stelle =                       /* der jetzt zu behandelnde Teilterm faengt direkt hinter dem */
      TO_Schwanz(NFPfad[aktuelleStelle].EinfuegePunkt);    /* neuen EinfuegePunkt an */
   aktuelleStelle++;
   NFPfad[aktuelleStelle].AnzNochZuBehandelnderTeilterme = 
      TO_AnzahlTeilterme(NFPfad[aktuelleStelle].Stelle);
   NFPfad[aktuelleStelle].EinfuegePunkt = NULL;
   NFB_PfadZellenT *aNFPfad;
   if (aktuelleStelle == aktuelleGroessePfad){              /* Im Pfad kein Platz mehr zum Tiefersteigen => realloc versuchen*/
      if (aktuelleGroessePfad >= MaxStelle - NF_PfadArrayGroesse)
         our_error ("Ueberlauf in StellenT, ggf. anpassen");
      aktuelleGroessePfad += NF_PfadArrayGroesse;
      NFPfad = (NFB_PfadZellenT *) our_realloc(NFPfad, sizeof(NFB_PfadZellenT)*(aktuelleGroessePfad+1),
                                                    "Erweitern des Normalform-Pfades");
      if (Ausdehnungsproz)
         (* Ausdehnungsproz) (aktuelleGroessePfad+1);
   }
#else
   /* Diese Fassung spart 24 Instruktionen! */
   /* evtl. koennte man noch was mit lokalen Variablen machen (Aliasierungsproblem) */
   NFB_PfadZellenT *aNFPfad;
   if (aktuelleStelle == aktuelleGroessePfad){           /* Im Pfad kein Platz mehr zum Tiefersteigen => realloc versuchen*/
     if (aktuelleGroessePfad >= MaxStelle - NF_PfadArrayGroesse)
         our_error ("Ueberlauf in StellenT, ggf. anpassen");
      aktuelleGroessePfad += NF_PfadArrayGroesse;
      NFPfad = (NFB_PfadZellenT *) our_realloc(NFPfad, sizeof(NFB_PfadZellenT)*(aktuelleGroessePfad+1),
                                                    "Erweitern des Normalform-Pfades");
   }
   aNFPfad = &(NFPfad[aktuelleStelle]);
   (*aNFPfad).AnzNochZuBehandelnderTeilterme--; /* ein Teilterm wird jetzt behandelt */
   if ((*aNFPfad).EinfuegePunkt == NULL){      /* bisher noch kein Teilterm behandelt */
      (*aNFPfad).EinfuegePunkt =               /* => der behandelte Teilterm faengt direkt hinter Topsymbol an*/
         (*aNFPfad).Stelle;       
   }
   else{                                                     /* es wurde bereits ein Teilterm behandelt */
      (*aNFPfad).EinfuegePunkt =               /* => EinfuegePunkt Ende des zuletzt behandelten Teilterms setzen */
         TO_TermEnde( TO_Schwanz( (*aNFPfad).EinfuegePunkt)); /* (der direkt hinter altem EinfuegePunkt anfing.) */
   }
   aNFPfad[1].Stelle =                       /* der jetzt zu behandelnde Teilterm faengt direkt hinter dem */
      TO_Schwanz((*aNFPfad).EinfuegePunkt);    /* neuen EinfuegePunkt an */
   aktuelleStelle++; aNFPfad++; /*PfadEinsAb();*/
   (*aNFPfad).AnzNochZuBehandelnderTeilterme = 
      TO_AnzahlTeilterme((*aNFPfad).Stelle);
   (*aNFPfad).EinfuegePunkt = NULL;
#endif
}

/*-------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------- Regel oder Gleichung angewendet ? ----------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------*/

static BOOLEAN BL_RegelOderGleichungAngewendet(BOOLEAN doR, BOOLEAN doE, TermT Wurzel, UTermT Term, UTermT Vorg);

static BOOLEAN NFB_RegelOderGleichungAngewendet(BOOLEAN doR, BOOLEAN doE)
{
  if (TO_TermIstNichtVar(NFPfad[aktuelleStelle].Stelle) &&
      BL_RegelOderGleichungAngewendet(doR, doE, NFPfad[0].Stelle, NFPfad[aktuelleStelle].Stelle, 
				      (aktuelleStelle == 0) ? NULL : NFPfad[aktuelleStelle-1].EinfuegePunkt)){
    if (aktuelleStelle > 0){
      NFPfad[aktuelleStelle].Stelle = TO_Schwanz(NFPfad[aktuelleStelle-1].EinfuegePunkt);
    }
    NFPfad[aktuelleStelle].AnzNochZuBehandelnderTeilterme = TO_AnzahlTeilterme(NFPfad[aktuelleStelle].Stelle); 
    NFPfad[aktuelleStelle].EinfuegePunkt = NULL;
    return TRUE;
  }
  return FALSE;
}

static void PfadInit(TermT Term)
{
    aktuelleStelle = 0;
    Reduziert = FALSE;
    if (NFPfad == NULL) {
      NFPfad = (NFB_PfadZellenT *) our_alloc (NF_PfadArrayGroesse*sizeof(NFB_PfadZellenT));
      aktuelleGroessePfad = NF_PfadArrayGroesse - 1;
    }
    NFPfad[0].Stelle = Term;
    NFPfad[0].AnzNochZuBehandelnderTeilterme = TO_AnzahlTeilterme(Term);
    NFPfad[0].EinfuegePunkt = NULL;
    letzteRed =0;
    hoechsteRed = 0;
}

   
/*-------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------- SetzePfadAufNaechstenInVorOrdnung ----------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------*/
/*                                                                                                                         */
/* Nachdem die aktuelle Stelle im Pfad nicht mehr reduzierbar ist, wird der naechste Teilterm in Vorordnung gesucht und    */
/* zum aktuellen Term gemacht. Dabei wird auch das noetige Backtracking durchgefuehrt                                      */
/*                                                                                                                         */
/*-------------------------------------------------------------------------------------------------------------------------*/

static BOOLEAN WeiterInVO(void)
{
  while (NFPfad[aktuelleStelle].AnzNochZuBehandelnderTeilterme == 0)     
    /* Aufstieg bis Wurzel oder Knoten mit noch nicht abgearbeiteten Teiltermen */
    if (aktuelleStelle == 0)
      return FALSE;
    else
      aktuelleStelle--;
 
  NFB_PfadEinsWeiterSetzen();                                                        /* sonst: Weiter im Durchlauf */
  return TRUE;
}



static BOOLEAN WeiterInVOTeilterm(void)
/* Setzt die aktuelle Stelle im Pfad auf ihren Nachfolger in Vorordnung. Dabei wird aber der an der Stelle hoechsteRed
   beginnende Teilterm nicht verlassen.
   Rueckgabe ist TRUE, wenn eine solche Stelle gefunden wurde. In diesem Fall ist aktStelle dann die gefundene Stelle.
   Ist aktStelle bereits die letzte in Vorordnung zu behandelnde Stelle im Teilterm, so wird FALSE zurueckgegeben. aktStelle
   ist dann die Wurzel des in hoechsteRed beginnenden Teiltermes.
   Damit ist am Ende aktuelleStelle==hoechsteRed.
*/
{
  while ((aktuelleStelle != hoechsteRed) &&                              /* Aufstieg bis Wurzel des Teilterms oder Knoten */
	  (NFPfad[aktuelleStelle].AnzNochZuBehandelnderTeilterme == 0))           /* mit noch nicht abgearbeitetem Teilt. */
      aktuelleStelle--;
   if (NFPfad[aktuelleStelle].AnzNochZuBehandelnderTeilterme == 0)/* Keinen Knoten mit noch nicht abgearbeiteten Teiltermen gefunden */
      return FALSE;                                     /* => Pfad ist am Ende, dh. Teilterm ist fertig durchlaufen, und*/
                                                        /*    aktStelle ist hoechsteRed, also Wurzel des Teilterms */
   else{			                        /*    (dessen AnzNochZuBehNachf gleich 0 ist) */
      NFB_PfadEinsWeiterSetzen();                   /* sonst: Weiter im Durchlauf */
      return TRUE;
   }
}

/*-------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------- Backtracking -------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------*/

static BOOLEAN AlleAufsteigenBisReduziertOderStop(BOOLEAN withR, BOOLEAN withE, StellenT start)
/*
- alle Stellen (start, 0] probieren (Ist start = 0, wird 0 nicht probiert).
- FALSE: aktuelleStelle = 0
- TRUE : aktuelleStelle = reduzierte Stelle = letzteRed
*/
{
   aktuelleStelle = start;

   while(aktuelleStelle > 0){
      aktuelleStelle--;
      if (NFB_RegelOderGleichungAngewendet(withR, withE))
         return TRUE;
   }
   return FALSE;
}


/*-------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------- Normalform zu Regeln oder Gleichungen-------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------*/

static BOOLEAN NormalformZuRegelnOderGleichungenAufNochmal(BOOLEAN withR, BOOLEAN withE, TermT Term)
 /*  nach Reduktion aktuelle Stelle nochmal, Aufstieg */
{
   PfadInit(Term);

   while (NFB_RegelOderGleichungAngewendet(withR, withE));        /* Es gibt keine leeren Terme, also mind. ein Red.versuch */

   while(WeiterInVO()){
      if (NFB_RegelOderGleichungAngewendet(withR, withE)){			      /* Reduktionserfolg => Stelle merken */
	 while(NFB_RegelOderGleichungAngewendet(withR, withE));	 		      /*           Stelle fertigreduzieren */
	 letzteRed = aktuelleStelle;	                  /* (Abfrage, ob Wurzel, wuerde zwar an der Wurzel OPs einsparen, */
							    /*  dies passiert aber zu selten (s.o.))*/
         while (AlleAufsteigenBisReduziertOderStop(withR, withE, letzteRed)){
            while(NFB_RegelOderGleichungAngewendet(withR, withE));
            letzteRed = aktuelleStelle;
         }

	 aktuelleStelle = letzteRed;			/* nach Aufstieg-Ende an der zuletzt red. Stelle mit dem Durchlauf */
      }	                                                    /* fortfahren, da diese Stelle schon erledigt ist  */
   }

   return Reduziert;
}

/*-------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------- Normalform zu Regeln TEILTERM --------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------*/
/* Die folgenden Prozeduren arbeiten analog den normalen Normalform-Prozeduren zum Regelsystem weiter unten.               */
/* Sie kommen zur Anwendung, wenn ein Term, der vorher in Normalform bezueglich R war, an einer Stelle (hoechsteRed) ver-  */
/* aendert wurde. Dieser Term wird dann wieder in Normalform bzgl. R gebracht, wobei der Termdurchlauf auf den veraenderten*/
/* Bereich des Termes beschraenkt bleibt (ggf. wird mit Backtracking ein groesserer Bereich abgedeckt).                    */
/* Dabei muss das Backtracking bis zur Wurzel des Termes laufen.                                                           */
/* Alle vier Prozeduren sind im Prinzip wie folgt strukturiert:                                                            */
/* - Aufstieg im Term, um moeglicherweise reduzierbare Vorgaenger zu finden (variantenabhaengig)           (1. Schleife)   */
/* - 'normaler' Normalform-Algorithmus, der aber nur den Teilterm bearbeitet und ggf. hoechsteRed anpasst. (2. Schleife)   */
/*                                                                                                                         */
/* Vorbedingung : aktStelle = hoechsteRed (Dies ist die veraenderte Stelle)                                                */
/* Aenderungen waehrend des Ablaufes: falls beim Aufsteigen ueber hoechsteRed hinaus eine Reduktion stattfand, wird deren  */
/*                                    Stelle als hoechsteRed gespeichert.                                                  */
/* Nachbedingung: der uebergebene Term ist in Normalform zu R, die hoechste veraenderte Stelle (also entweder diejenige,   */
/*                deren Reduktion den Prozeduraufruf ausgeloest hat, oder eine waehrend des Proz.ablaufs reduzierte, im    */
/*                Term hoehergelegene Stelle) ist in hoechsteRed gespeichert; aktStelle=hoechsteRed.                       */
/*                                                                                                                         */
/* Verifikation der Nachbedingung:                                                                                         */
/*  letztes ausgefuehrtes Statement, bevor die hintere do-while-Schleife verlassen wird, ist SetzePfadAufNachfInVOTeilterm,*/
/*  und zwar mit dem Resultat FALSE. => Gemaess Nachbedingung dieser Prozedur ist aktStelle = Gipfel akt. Teilterm =       */
/*  hoechsteRed.                                                                                                           */
/*                                                                                                                         */
/* Nur die erste der vier Prozeduren ist ausfuehrlich kommentiert, da die uebrigen sich nur im Detail von dieser unter-    */
/* scheiden. Bei diesen sind dann die sich unterscheidenden Stellen kommentiert.                                           */
/*                                                                                                                         */
/* Aufstieg im Term (1.Schleife):                                                                                          */
/*                                                                                                                         */
/* - Abstieg:                                                                                                              */
/*    * Von Wurzel absteigen bis hoechsteRed                                                                               */
/*    * Dabei jede Stelle auf Reduzierbarkeit pruefen                                                                      */
/*    * Nach Reduktion: +hoechsteRed anpassen (die reduzierte Stelle kann nicht unterhalb von hoechsteRed liegen)          */
/*                      +Erneut ganz oben beginnen                                                                         */
/*                                                                                                                         */
/* - Aufstieg:                                                                                                             */
/*    * Aufstieg von hoechsteRed bis zur Wurzel                                                                            */
/*    * Dabei jede Stelle auf Reduzierbarkeit pruefen                                                                      */
/*    * Nach Reduktion: hoechsteRed auf aktuelleStelle setzen, weiter aufsteigen                                           */
/*                                                                                                                         */
/*-------------------------------------------------------------------------------------------------------------------------*/

static void NormalformZuRegelnAufNochmalTeilterm(void)
 /*  nach Reduktion aktuelle Stelle nochmal, Aufstieg */
{
   while(NFB_RegelOderGleichungAngewendet(TRUE, FALSE));

   while(AlleAufsteigenBisReduziertOderStop(TRUE, FALSE, aktuelleStelle)){
      while(NFB_RegelOderGleichungAngewendet(TRUE, FALSE));
      hoechsteRed = aktuelleStelle;
   }

   aktuelleStelle = hoechsteRed;

   while(WeiterInVOTeilterm()){
      if (NFB_RegelOderGleichungAngewendet(TRUE, FALSE)){
	 while(NFB_RegelOderGleichungAngewendet(TRUE, FALSE));
	 letzteRed = aktuelleStelle;

         while (AlleAufsteigenBisReduziertOderStop(TRUE, FALSE, letzteRed)){
            while(NFB_RegelOderGleichungAngewendet(TRUE, FALSE));
            letzteRed = aktuelleStelle;
         }
	 aktuelleStelle = letzteRed;
	 if (letzteRed < hoechsteRed)
	   hoechsteRed = letzteRed;
      }
   }
}

/*-------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------- Normalform zu Regeln und Gleichungen -------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------*/
/*                                                                                                                         */
/* Bringt den uebergebenen Term bzgl. des aktuellen Regel- und Gleichungssystems auf Normalform. Dazu wird zunaechst eine  */
/* 'abstrakte' Variable Pfad initialisiert, die dann gemaess einer -aenderbaren- Strategie das Durchlaufen des Termes      */
/* organisiert, wobei das weitere Vorgehen in Abhaengigkeit vom Erfolg der letzten versuchten Reduktion sein kann.         */
/* Dies wird solange fortgefuehrt, bis es keine reduzierbaren Stellen mehr im Anfrageterm gibt.                            */
/* Es wird erst dann versucht, Gleichungen anzuwenden, wenn es keine anwendbaren Regeln mehr gibt.                         */
/* In diesem Fall wird versucht, eine anwendbare Gleichung zu finden. Gelingt dies, so wird die Gleichung angewendet, und  */
/* anschliessend wird an dieser Stelle mit Regeln fortgefahren. Dies wiederholt sich, bis es weder anwendbare Regeln noch  */
/* Gleichungen gibt.                                                                                                       */
/*                                                                                                                         */
/*-------------------------------------------------------------------------------------------------------------------------*/

static BOOLEAN NFO_NormalformAufNochmal(TermT Term)
/* Bildet destruktiv! die Normalform zu Term bezueglich des aktuellen Regel- und Gleichungssystems.
   Rueckgabe ist, ob Reduktionen stattgefunden haben.*/
{
   BOOLEAN reduziert, nochmal;
   
   reduziert = NormalformZuRegelnOderGleichungenAufNochmal(TRUE, FALSE, Term);
   
   PfadInit(Term);
   
   do {
      if (NFB_RegelOderGleichungAngewendet(FALSE, TRUE)){
         reduziert = TRUE;
         do {
            hoechsteRed = aktuelleStelle;
            NormalformZuRegelnAufNochmalTeilterm();
	    NFPfad[aktuelleStelle].AnzNochZuBehandelnderTeilterme = TO_AnzahlTeilterme(NFPfad[aktuelleStelle].Stelle); 
	    NFPfad[aktuelleStelle].EinfuegePunkt = NULL;
         } while(AlleAufsteigenBisReduziertOderStop(FALSE, TRUE, hoechsteRed));
         aktuelleStelle = hoechsteRed;
         nochmal = TRUE;
      }
      else{
         nochmal = WeiterInVO();
      }
   } while(nochmal);

   return reduziert;                                                              /* klar */
}

/*-------------------------------------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------*/
/*- D I E   N E U E N   F A S S U N G E N   -------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------*/

void ProtokolliereSchritt(RegelOderGleichungsT Objekt, UTermT Stelle);

#if 1
static BOOLEAN bla = FALSE;
#else
#define bla FALSE
#endif

static BOOLEAN BL_RegelOderGleichungAngewendet(BOOLEAN doR, BOOLEAN doE, TermT Wurzel, UTermT Term, UTermT Vorg)
{
  if (bla) IO_DruckeFlex("BL_RG: %t \n", Term);
  if ((doR && (IncAnzRedVersMitRegeln(), MO_RegelGefunden(Term))) ||
      (doE && (IncAnzRedVersMitGleichgn(), MO_GleichungGefunden(Term)))) {
        Reduziert = TRUE;
	if (Vorg == NULL){
	    MO_SigmaRInEZ();
            BOB_NFCallback(Wurzel, Wurzel, MO_AngewendetesObjekt);
	}
	else {
	    TO_NachfBesetzen(Vorg, MO_SigmaRInLZ());
            BOB_NFCallback(Wurzel, TO_Schwanz(Vorg), MO_AngewendetesObjekt);
	}
        IncAnzRedErf(TP_TermpaarIstRegel(MO_AngewendetesObjekt));
	MO_keinenSchutz();
#if 1
	if(bla)IO_DruckeFlex("=> %t mit %d (%d): %t -> %t\n", (Vorg == NULL) ? Wurzel : TO_Schwanz(Vorg),
		      TP_getGeburtsTag_M(MO_AngewendetesObjekt), TP_getTodesTag_M(MO_AngewendetesObjekt),
		      TP_LinkeSeite(MO_AngewendetesObjekt), TP_RechteSeite(MO_AngewendetesObjekt));
#endif
        if (VERBOSE && drucken){
	  ProtokolliereSchritt(MO_AngewendetesObjekt, (Vorg == NULL) ? Wurzel : TO_Schwanz(Vorg));
	}
        return TRUE;
    }
    else{
        return FALSE;
    }
}

static void BL_NormalformInnermost1(BOOLEAN withR, BOOLEAN withE, TermT Wurzel, UTermT Term, UTermT Vorg)
{
    UTermT subVorg, subTerm, NULLTerm;

    if (TO_TermIstVar(Term))
	return;
    do {
	if (Vorg != NULL) /* Wichtig: Wenn innendrin, muss Term ueber Vorg erfasst werden! */
	    Term = TO_Schwanz(Vorg);

	subVorg  = Term;
	subTerm  = TO_Schwanz(subVorg);
	NULLTerm = TO_NULLTerm(Term);

	while (subTerm != NULLTerm) {
	    BL_NormalformInnermost1(withR,withE,Wurzel, subTerm,subVorg);
	    subVorg = TO_TermEnde(TO_Schwanz(subVorg));
	    subTerm = TO_Schwanz(subVorg);
	}
    } while (BL_RegelOderGleichungAngewendet(withR, withE, Wurzel, Term, Vorg));
}

static void BL_ClearFlags(UTermT Term)
{
#if DO_FLAGS
  UTermT Teilterm;
  TO_doStellen(Term, Teilterm, {TO_ClearSubstFlag(Teilterm);})
#endif
}

static void BL_NormalformInnermost2(BOOLEAN withR, BOOLEAN withE, TermT Wurzel, UTermT Term, UTermT Vorg)
{
    UTermT subVorg, subTerm, NULLTerm;

    if (TO_TermIstVar(Term))
	return;
#if DO_FLAGS
    if (DO_REALLY && TO_SubstFlagSet(Term)){
      /*      IO_DruckeFlex("Hurray! %t\n", Term);*/
      return;
    }
#endif
    do {
	if (Vorg != NULL) /* Wichtig: Wenn innendrin, muss Term ueber Vorg erfasst werden! */
	    Term = TO_Schwanz(Vorg);

	subVorg  = Term;
	subTerm  = TO_Schwanz(subVorg);
	NULLTerm = TO_NULLTerm(Term);

	while (subTerm != NULLTerm) {
	    BL_NormalformInnermost2(withR,withE,Wurzel, subTerm,subVorg);
	    subVorg = TO_TermEnde(TO_Schwanz(subVorg));
	    subTerm = TO_Schwanz(subVorg);
	}
    } while (BL_RegelOderGleichungAngewendet(withR, withE, Wurzel, Term, Vorg));
}

static void BL_NormalformOutermost(BOOLEAN withR, BOOLEAN withE, UTermT Term)
{
    UTermT subTerm, subVorg, NULLTerm;

    root: 
      if (!TO_TermIstVar(Term) &&
	  BL_RegelOderGleichungAngewendet(withR, withE, Term, Term,NULL)){
        goto root;
      }
      
      subVorg  = Term;
      subTerm  = TO_Schwanz(subVorg);
      NULLTerm = TO_NULLTerm(Term);
      
      while (subTerm != NULLTerm){
	if (!TO_TermIstVar(subTerm) &&
	    BL_RegelOderGleichungAngewendet(withR, withE, Term,subTerm, subVorg)){
          goto root;
	}
	subVorg = subTerm;
	subTerm = TO_Schwanz(subVorg);
      }
}

static void BL_NormalformOutermost2(BOOLEAN withR, BOOLEAN withE, UTermT Term)
{
    UTermT subTerm, subVorg, NULLTerm, redStelle;

      redStelle = NULL;
    root: 
      if (!TO_TermIstVar(Term) &&
	  BL_RegelOderGleichungAngewendet(withR, withE, Term, Term,NULL)){
        redStelle = NULL;
        goto root;
      }
      
      subVorg  = Term;
      subTerm  = TO_Schwanz(subVorg);
      NULLTerm = TO_NULLTerm(Term);
      
      while (subTerm != NULLTerm){
        if ((redStelle != NULL) &&
            TO_GetFlag(TO_TermEnde(subTerm))){ /* skip subterm */
          subVorg = TO_TermEnde(subTerm);
          subTerm = TO_Schwanz(subVorg);
        }
        else {
          if (!TO_TermIstVar(subTerm) &&
              BL_RegelOderGleichungAngewendet(withR, withE, Term, subTerm, subVorg)){
            redStelle = TO_Schwanz(subVorg);
            TO_SetFlag(TO_TermEnde(redStelle),FALSE);
            goto root;
          }
          if (subTerm == redStelle) {
            redStelle = NULL;
          }
          TO_SetFlag(TO_TermEnde(subTerm), subTerm == TO_TermEnde(subTerm));
          subVorg = subTerm;
          subTerm = TO_Schwanz(subVorg);
        }
      }
}

static void BL_NormalformMixMost2(BOOLEAN withR, BOOLEAN withE, UTermT Term)
{
    UTermT subTerm, subVorg, NULLTerm, redStelle;

    redStelle = NULL;
    root: 
    if (withR){
      if (!TO_TermIstVar(Term) &&
	  BL_RegelOderGleichungAngewendet(TRUE, FALSE, Term, Term,NULL)){
        redStelle = NULL;
        goto root;
      }
      
      subVorg  = Term;
      subTerm  = TO_Schwanz(subVorg);
      NULLTerm = TO_NULLTerm(Term);
      
      while (subTerm != NULLTerm){
        if ((redStelle != NULL) &&
            TO_GetFlag(TO_TermEnde(subTerm))){ /* skip subterm */
          subVorg = TO_TermEnde(subTerm);
          subTerm = TO_Schwanz(subVorg);
        }
        else {
          if (!TO_TermIstVar(subTerm) &&
              BL_RegelOderGleichungAngewendet(TRUE, FALSE, Term, subTerm, subVorg)){
            redStelle = TO_Schwanz(subVorg);
            TO_SetFlag(TO_TermEnde(redStelle),FALSE);
            goto root;
          }
          if (subTerm == redStelle) {
            redStelle = NULL;
          }
          TO_SetFlag(TO_TermEnde(subTerm), subTerm == TO_TermEnde(subTerm));
          subVorg = subTerm;
          subTerm = TO_Schwanz(subVorg);
        }
      }
    }
    if (withE){
      if (!TO_TermIstVar(Term) &&
	  BL_RegelOderGleichungAngewendet(FALSE, TRUE, Term, Term,NULL)){
        redStelle = NULL;
        goto root;
      }
      
      subVorg  = Term;
      subTerm  = TO_Schwanz(subVorg);
      NULLTerm = TO_NULLTerm(Term);
      
      while (subTerm != NULLTerm){
        if (!TO_TermIstVar(subTerm) &&
            BL_RegelOderGleichungAngewendet(FALSE, TRUE, Term, subTerm, subVorg)){
          redStelle = TO_Schwanz(subVorg);
          TO_SetFlag(TO_TermEnde(redStelle),FALSE);
          goto root;
        }
        TO_SetFlag(TO_TermEnde(subTerm), subTerm == TO_TermEnde(subTerm));
        subVorg = subTerm;
        subTerm = TO_Schwanz(subVorg);
      }
    }
}

static void BL_NormalformMixMost3(BOOLEAN withR, BOOLEAN withE, UTermT Term)
{
    UTermT subTerm, subVorg, NULLTerm, redStelle;

    redStelle = NULL;
    root: 
    if (withR){
      if (!TO_TermIstVar(Term) &&
	  BL_RegelOderGleichungAngewendet(TRUE, FALSE, Term, Term,NULL)){
        redStelle = NULL;
        goto root;
      }
      
      subVorg  = Term;
      subTerm  = TO_Schwanz(subVorg);
      NULLTerm = TO_NULLTerm(Term);
      
      while (subTerm != NULLTerm){
        if ((redStelle != NULL) &&
            TO_GetFlag(TO_TermEnde(subTerm))){ /* skip subterm */
          subVorg = TO_TermEnde(subTerm);
          subTerm = TO_Schwanz(subVorg);
        }
        else {
          if (!TO_TermIstVar(subTerm) &&
              BL_RegelOderGleichungAngewendet(TRUE, FALSE, Term, subTerm, subVorg)){
            do {
              subTerm = TO_Schwanz(subVorg);
            } while (!TO_TermIstVar(subTerm) &&
                     BL_RegelOderGleichungAngewendet(withR, withR, Term, subTerm, subVorg));
            redStelle = TO_Schwanz(subVorg);
            TO_SetFlag(TO_TermEnde(redStelle),FALSE);
            goto root;
          }
          if (subTerm == redStelle) {
            redStelle = NULL;
          }
          TO_SetFlag(TO_TermEnde(subTerm), subTerm == TO_TermEnde(subTerm));
          subVorg = subTerm;
          subTerm = TO_Schwanz(subVorg);
        }
      }
    }
    if (withE){
      if (!TO_TermIstVar(Term) &&
	  BL_RegelOderGleichungAngewendet(FALSE, TRUE, Term, Term,NULL)){
        redStelle = NULL;
        goto root;
      }
      
      subVorg  = Term;
      subTerm  = TO_Schwanz(subVorg);
      NULLTerm = TO_NULLTerm(Term);
      
      while (subTerm != NULLTerm){
        if (!TO_TermIstVar(subTerm) &&
            BL_RegelOderGleichungAngewendet(FALSE, TRUE, Term, subTerm, subVorg)){
            do {
              subTerm = TO_Schwanz(subVorg);
            } while (!TO_TermIstVar(subTerm) &&
                     BL_RegelOderGleichungAngewendet(withR, withR, Term, subTerm, subVorg));
          redStelle = TO_Schwanz(subVorg);
          TO_SetFlag(TO_TermEnde(redStelle),FALSE);
          goto root;
        }
        TO_SetFlag(TO_TermEnde(subTerm), subTerm == TO_TermEnde(subTerm));
        subVorg = subTerm;
        subTerm = TO_Schwanz(subVorg);
      }
    }
}

static BOOLEAN NormalformInnermost(BOOLEAN doR, BOOLEAN doE, TermT Term)
{
    Reduziert = FALSE;
    BL_NormalformInnermost1(doR,doE,Term,Term,NULL);
    return Reduziert;
}

static BOOLEAN NormalformInnermost2(BOOLEAN doR, BOOLEAN doE, TermT Term)
{
    Reduziert = FALSE;
    BL_ClearFlags(Term);
    BL_NormalformInnermost2(doR,doE,Term,Term,NULL);
    return Reduziert;
}

static BOOLEAN NormalformOutermost(BOOLEAN doR, BOOLEAN doE, TermT Term)
{
    Reduziert = FALSE;
    BL_NormalformOutermost(doR,doE,Term);
    return Reduziert;
}

static BOOLEAN NormalformOutermost2(BOOLEAN doR, BOOLEAN doE, TermT Term)
{
#if 0
  TermT Kopie1 = TO_Termkopie(Term);
  TermT Kopie2 = TO_Termkopie(Term);
  BL_NormalformOutermost(doR,doE,Kopie1);
  Reduziert = FALSE;
  BL_NormalformOutermost2(doR,doE,Term);
  if (!TO_TermeGleich(Kopie1,Term)){
    IO_DruckeFlex("Scheisse:\n%t ohne Flags\n%t mit Flags\n", Kopie1, Term);
    bla = TRUE;
    BL_NormalformOutermost2(doR,doE,Kopie2);
    bla = FALSE;
  }
  TO_TermeLoeschen(Kopie1,Kopie2);
  return Reduziert;
#else
  Reduziert = FALSE;
  BL_NormalformOutermost2(doR,doE,Term);
  return Reduziert;
#endif
}

static BOOLEAN NormalformMixMost(BOOLEAN doR, BOOLEAN doE, TermT Term)
{
    BOOLEAN res;
    if (FALSE && doR && doE) 
       res = NFO_NormalformAufNochmal(Term);
    res = NormalformZuRegelnOderGleichungenAufNochmal(doR, doE, Term);
    return res;
}

static BOOLEAN NormalformMixMost2(BOOLEAN doR, BOOLEAN doE, TermT Term)
{
#if 1
    Reduziert = FALSE;
    BL_NormalformMixMost2(doR,doE,Term);
    return Reduziert;
#else
  TermT Kopie = TO_Termkopie(Term);
  BL_NormalformMixMost2(doR,doE,Term);
  if (NF_TermReduzibel(doR,doE,Term)){
    IO_DruckeFlex("Scheisse:\n%t reduzibel\n", Term);
    bla = TRUE;
    BL_NormalformOutermost2(doR,doE,Kopie);
    bla = FALSE;
  }
  TO_TermLoeschen(Kopie);
  return Reduziert;    
#endif
}

static BOOLEAN NormalformMixMost3(BOOLEAN doR, BOOLEAN doE, TermT Term)
{
#if 1
    Reduziert = FALSE;
    BL_NormalformMixMost3(doR,doE,Term);
    return Reduziert;
#else
  TermT Kopie = TO_Termkopie(Term);
  BL_NormalformMixMost3(doR,doE,Term);
  if (NF_TermReduzibel(doR,doE,Term)){
    IO_DruckeFlex("Scheisse:\n%t reduzibel\n", Term);
    bla = TRUE;
    BL_NormalformOutermost2(doR,doE,Kopie);
    bla = FALSE;
  }
  TO_TermLoeschen(Kopie);
  return Reduziert;    
#endif
}

/*-------------------------------------------------------------------------------------------------------------------------*/
/*------ Reduzibilitaetstest (fuer subcon-Kriterium) ----------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------*/

static BOOLEAN TermReduzibelE(UTermT Term, UTermT NULLTerm)
{
  while (TO_PhysischUngleich(Term,NULLTerm)){
    if (TO_TermIstNichtVar(Term) && MO_GleichungGefunden(Term))
      return TRUE;
    Term = TO_Schwanz(Term);
  }
  return FALSE;
}

static BOOLEAN TermReduzibelR(UTermT Term, UTermT NULLTerm)
{
  while (TO_PhysischUngleich(Term,NULLTerm)){
    if (TO_TermIstNichtVar(Term) && MO_RegelGefunden(Term))
      return TRUE;
    Term = TO_Schwanz(Term);
  }
  return FALSE;
}

BOOLEAN NF_TermInnenReduzibel(BOOLEAN doR, BOOLEAN doE, UTermT Term)
{
  return (doR && TermReduzibelR(TO_Schwanz(Term),TO_NULLTerm(Term))) || 
         (doE && TermReduzibelE(TO_Schwanz(Term),TO_NULLTerm(Term)));
}

BOOLEAN NF_TermReduzibel(BOOLEAN doR, BOOLEAN doE, UTermT Term)
{
  return (doR && TermReduzibelR(Term,TO_NULLTerm(Term))) || 
         (doE && TermReduzibelE(Term,TO_NULLTerm(Term)));
}

/*-------------------------------------------------------------------------------------------------------------------------*/
/*------ Anwendbarkeit eines bestimmten Objektes irgendwo auf einen Term --------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------*/

static BOOLEAN ObjektAngewendet(RegelOderGleichungsT Objekt, TermT Term)
{
   UTermT subTerm = Term;
   UTermT nullTerm = TO_NULLTerm(Term);
   UTermT vorg = NULL;
   do {
     if (TO_TermIstNichtVar(subTerm)){
       IncAnzRedVers(TP_TermpaarIstRegel(Objekt));
       if (MO_ObjektPasst(Objekt,subTerm)) {                    /* Reduktionsversuch erfolgreich? */
         if (vorg == NULL){/* Wurzel */
           MO_SigmaRInEZ();
           /* BOB_Schritt ? */
         }
         else{
           TO_NachfBesetzen(vorg, MO_SigmaRInLZ());
           /* BOB_Schritt ? */
        }
         IncAnzRedErf(TP_TermpaarIstRegel(Objekt));
         return TRUE;
       }
     }
     vorg = subTerm;
     subTerm = TO_Schwanz(subTerm);
   } while(subTerm != nullTerm);
   return FALSE;
}

BOOLEAN NF_NormalformstRE(RegelOderGleichungsT Objekt, TermT Term)
{
  if (ObjektAngewendet (Objekt, Term)){
    NF_NormalformRE(Term);
    return TRUE;
  }
  else {
    return FALSE;
  }
}

BOOLEAN NF_ObjektAnwendbar(RegelOderGleichungsT Objekt, UTermT Term, unsigned int len)
/* len == TO_Termlaenge(Term) */
{
   UTermT subTerm = Term;
   unsigned int lenl = Objekt->laengeLinks;
   unsigned int lenr = RE_IstGleichung(Objekt) ? Objekt->laengeRechts : UINT_MAX;
   while ((len >= lenl) || (len >= lenr)){
     if (TO_TermIstNichtVar(subTerm)) {
       IncAnzRedVers(TP_TermpaarIstRegel(Objekt));
       if (MO_ObjektPasst(Objekt, subTerm))
         return TRUE;
     }
     subTerm = TO_Schwanz(subTerm);
     len--;
   }
   return FALSE;
}

BOOLEAN NF_TermpaarAnwendbar(TermT lhs, TermT rhs, UTermT Term) 
/* setzt voraus, dass lhs/rhs varnomiert !! */
{
   UTermT subTerm = Term;
   UTermT nullTerm = TO_NULLTerm(Term);
   do {
     if (TO_TermIstNichtVar(subTerm)) {
       if (MO_TermpaarPasst(lhs, rhs, subTerm))
         return TRUE;
     }
     subTerm = TO_Schwanz(subTerm);
   } while(subTerm != nullTerm);
   return FALSE;
}

BOOLEAN NF_geschuetzteNormalformRE(TermT Term, TermT Schutz)
/* bringt Term mit RE auf Normalform, bei toplevel werden nur Regeln/Gleichungen verwendet, die in der 
 * Dreiecksordnung kleiner sind. */
{
  BOOLEAN res;
  if(0 && VERBOSE && drucken){
    IO_DruckeFlex("Schuetze %t mit %t\n", Term, Schutz);
  }
  MO_schuetzeTerm(Term,Schutz);
  res = NF_NormalformRE(Term);
  MO_keinenSchutz();
  return res;
}

static BOOLEAN Gekuerzt(TermT lhs, TermT rhs)
{
  UTermT deltal;
  BOOLEAN gekuerzt = FALSE;
  do {
    SymbolT f = TO_TopSymbol(lhs),
            g = TO_TopSymbol(rhs);
    deltal = NULL;
    if (SO_SymbGleich(f,g) && SO_SymbIstFkt(f) && SO_Cancellation(f)) {                  /* bitte keine Konstanten */
      UTermT l = TO_ErsterTeilterm(lhs),
             NULLl = TO_NULLTerm(lhs),
             r = TO_ErsterTeilterm(rhs),
             deltar = NULL;
      do {
        if (!TO_TermeGleich(l,r)) {
          if (deltal && !(deltal = NULL))
            break;
          else
            deltal = l, deltar = r;
        }
        r = TO_NaechsterTeilterm(r),
        l = TO_NaechsterTeilterm(l);
      } while (TO_PhysischUngleich(l,NULLl));
      if (deltal) {
        if (TO_PhysischUngleich(TO_NaechsterTeilterm(deltal),NULLl)) {
          TO_TermzellenlisteLoeschen(TO_NaechsterTeilterm(deltal),TO_TermEnde(lhs));
          TO_TermzellenlisteLoeschen(TO_NaechsterTeilterm(deltar),TO_TermEnde(rhs));
        }
        l = TO_Schwanz(lhs), r = TO_Schwanz(rhs);
        TO_SetzeTermzelle(lhs,deltal); TO_SetzeTermzelle(rhs,deltar);
        TO_NULLBesetzen(lhs,NULL); TO_NULLBesetzen(rhs,NULL); 
        TO_TermzellenlisteLoeschen(l,deltal); TO_TermzellenlisteLoeschen(r,deltar);
        gekuerzt = TRUE;
      }
    }
  } while (deltal);
  return gekuerzt;
}


BOOLEAN NF_Normalform2(BOOLEAN doR, BOOLEAN doE, TermT lhs, TermT rhs)
{ 
  BOOLEAN reduziert_l = NF_Normalform(doR, doE, lhs),
          reduziert_r = NF_Normalform(doR, doE, rhs),
          gekuerzt    = Gekuerzt(lhs,rhs);
  return reduziert_l || reduziert_r || gekuerzt;
}


/* -------------------------------------------------------------------------------------------------------------*/
/* --------- Initialisierung -----------------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

void NF_InitAposteriori(InfoStringT cancellationUSR)
{
#ifdef CCE_Source
  NF_Normalform = NormalformInnermost2;
#else
   switch (PA_NFStrategie()){
   case PA_NFoutermost:      NF_Normalform = NormalformOutermost;  break;
   case PA_NFinnermost:      NF_Normalform = NormalformInnermost;  break;
   case PA_NFmixmost:        NF_Normalform = NormalformMixMost;    break;
   case PA_NFinnermostFlags: NF_Normalform = NormalformInnermost2; break;
   case PA_NFoutermostFlags: NF_Normalform = NormalformOutermost2; break;
   case PA_NFmixmost2:       NF_Normalform = NormalformMixMost2;   break;
   case PA_NFmixmost3:       NF_Normalform = NormalformMixMost3;   break;
   default: our_error("violation of internal invariant (NF_InitAposteriori)"); break;
   }

   if (PA_Cancellation()) {
     char *s0 = our_strdup(cancellationUSR),
          *s = s0,
          *t;
     SymbolT f;
     while ((t = strtok(s,":"))) {
       s = NULL;
       SO_forFktSymbole(f)
         if (!strcmp(t,IO_FktSymbName(f))) {
           if (SO_SymbIstKonst(f))
             our_error("no cancellation for constants");
           SO_SymbolInfo[f].Cancellation = TRUE;
           break;
         }
       if (f>SO_GroesstesFktSymb)
         our_error("illegal argument to -cc option");
     }
#if 0
     printf("Cancellation:");
     SO_forFktSymbole(f)
       if (SO_Cancellation(f))
         printf(" %s",IO_FktSymbName(f));
     printf("\n");
#endif
     our_dealloc(s0);
   }
#endif
}
