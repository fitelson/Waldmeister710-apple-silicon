/*=================================================================================================================
  =                                                                                                               =
  =  LPOVortests.c                                                                                                =
  =                                                                                                               =
  =  Funktionen zum Abpruefen von Variablenverhaeltnis, Teiltermeigenschaft, Gleichheit                           =
  =                                                                                                               =
  =  04.03.94 Thomas                                                                                              =
  =                                                                                                               =
  =================================================================================================================*/


#include "LPOVortests.h"
#include "Praezedenz.h"
#include "SpeicherVerwaltung.h"
#include "SymbolOperationen.h"
#include "VariablenMengen.h"


/*=================================================================================================================*/
/* I. Realisierung einer besonderen Liste ueber Termen                                                             */
/*=================================================================================================================*/

static SV_SpeicherManagerT LV_FreispeicherManager;

/* Nachfolgend Funktionen wie Einfuegen und Entfernen fuer eine doppelt verkettete lineare Liste mit kleinem Gim-
   mick: Auf Wunsch laesst sich nachverfolgen, ob das als erstes in die Liste eingefuegte Element noch in der Li-
   ste vorhanden ist oder entfernt wurde. Zu jedem eingetragenem Term gibt es ausserdem einen Eintrag Termende.    */

typedef struct TermlistenzellenTS *TermlistenzellenZeigerT;                      /* Typen fuer Listen ueber Termen */
typedef struct TermlistenzellenTS {
  UTermT Term;
  UTermT NULLTerm;
  TermlistenzellenZeigerT Vorg,Nachf;
} TermlistenzellenT;
typedef TermlistenzellenZeigerT TermlistenT;                                /* nach aussen sichtbarer Termlistentyp */

static TermlistenT Termliste;                                     /* dateiglobale Variable, zeigt auf Listenanfang */
static BOOLEAN aeltestes;                                                                     /* Flag fuer Gimmick */


#define /*void*/ LeereTermliste(/*void*/)                                                                           \
  Termliste = NULL;                                                                                                 \
  aeltestes = FALSE;


#define AeltestesNochInTermliste(/*void*/)                                                                          \
  aeltestes 


#define AeltestesBeachten(/*void*/)                                                                                 \
  aeltestes = TRUE;


#define /*static void*/ TermEinfuegen(/*UTermT*/ UTerm)                        /* Term vor aktuelle Liste haengen */\
{                                                           /* dabei Verzeigerung vornehmen und NULLTerm besetzen */\
  TermlistenzellenZeigerT Zelle;                                                                                    \
  SV_Alloc(Zelle,LV_FreispeicherManager,TermlistenzellenZeigerT);                                                   \
  Zelle->Vorg = NULL;                                                                                               \
  Zelle->Nachf = Termliste;                                                                                         \
  Zelle->NULLTerm = TO_NaechsterTeilterm(UTerm);                                                                    \
  if (Termliste)                                                                                                    \
    Termliste->Vorg = Zelle;                                                                                        \
  (Termliste = Zelle)->Term = TO_ErsterTeilterm(UTerm);                                                             \
} 


#define /*static void*/ TermEntfernen0(/*TermlistenzellenZeigerT*/ Zelle) /* bezeichnetes Listenelement entfernen */\
{                                          /* dabei Verzeigerungen aktualisieren, ggf. Variable Termliste aendern */\
  TermlistenzellenZeigerT AlteZelle;                     /* ausserdem pruefen, ob aeltestes Element entfernt wird */\
                                                                                                                    \
  if (Zelle->Vorg)                                                                                                  \
    Zelle->Vorg->Nachf = Zelle->Nachf;                                                                              \
  else                                                                                                              \
    Termliste =  Zelle->Nachf;                                                                                      \
  if (Zelle->Nachf)                                                                                                 \
    Zelle->Nachf->Vorg = Zelle->Vorg;                                                                               \
  else                                 /* das dadurch charakterisiert ist, dass seine Zelle keinen Nachfolger hat */\
    aeltestes = FALSE;                  /* und das Flag aeltestes noch den  Wert TRUE hat -> dann Flag loeschen */\
  AlteZelle = Zelle;                                                                                                \
  Zelle = Zelle->Nachf;                                                                                             \
  SV_Dealloc(LV_FreispeicherManager,AlteZelle);                                             /* Speicher freigeben */\
}

static void TermEntfernen(TermlistenzellenZeigerT *Zelle)                  /* bezeichnetes Listenelement entfernen */
{                                           /* dabei Verzeigerungen aktualisieren, ggf. Variable Termliste aendern */
  TermlistenzellenZeigerT AlteZelle;                      /* ausserdem pruefen, ob aeltestes Element entfernt wird */
                                                                                                                    
  if ((*Zelle)->Vorg)                                                                                                  
    (*Zelle)->Vorg->Nachf = (*Zelle)->Nachf;                                                                              
  else                                                                                                              
    Termliste =  (*Zelle)->Nachf;                                                                                      
  if ((*Zelle)->Nachf)                                                                                                 
    (*Zelle)->Nachf->Vorg = (*Zelle)->Vorg;
  else                                  /* das dadurch charakterisiert ist, dass seine Zelle keinen Nachfolger hat */
    aeltestes = FALSE;                   /* und das Flag aeltestes noch den  Wert TRUE hat -> dann Flag loeschen */
  AlteZelle = (*Zelle);
  (*Zelle) = (*Zelle)->Nachf;
  SV_Dealloc(LV_FreispeicherManager,AlteZelle);                                              /* Speicher freigeben */
}


#define /*static void*/ TermlisteLoeschen(/*void*/)                  /* Termliste loeschen und Speicher freigeben */\
{                                                                                                                   \
  TermlistenzellenZeigerT Nachf;                                                                                    \
                                                                                                                    \
  Nachf = Termliste;                                                                                                \
  while ((Termliste = Nachf)) {                                                                                     \
    Nachf = Termliste->Nachf;                                                                                       \
    SV_Dealloc(LV_FreispeicherManager,Termliste);                                                                   \
  }                                                                                                                 \
}



/*=================================================================================================================*/
/*= II. Initialisierung                                                                                           =*/
/*=================================================================================================================*/

void LV_InitAposteriori(void)
{
  SV_ManagerInitialisieren(&LV_FreispeicherManager,TermlistenzellenT,LV_AnzListenzellenJeBlock,"LPOVortests");
}



/*=================================================================================================================*/
/* III. Hilfsfunktion fuer Vortests                                                                                */
/*=================================================================================================================*/

#define TermeKuerzen(/*UTermT*/ Term1, /*UTermT*/ Term2)                                                            \
/* so lange von beiden Termen von links nach rechts die Topsymbole entfernen, bis ungleiche Symbole erreicht      */\
/* sind oder nicht einstellige                                                                                    */\
  while ((SO_SymbGleich(TO_TopSymbol(Term1),TO_TopSymbol(Term2))) && (TO_AnzahlTeilterme(Term1) == 1)) {            \
    (Term1) = TO_ErsterTeilterm(Term1); (Term2) = TO_ErsterTeilterm(Term2);                                         \
  }



/*=================================================================================================================*/
/* IV. Vortest fuer Entscheidungsfunktion                                                                          */
/*=================================================================================================================*/

static BOOLEAN VortestLPOVar(UTermT Term1, UTermT Term2, VergleichsT *Vergleich)
/* Behandelt simple Faelle: Term1 oder Term2 Variable. Ergebnis TRUE und *Vergleich Vergleichsresultat, falls
   einer der beteiligten Terme eine Variable ist; damit ist der Termvergleich schon endgueltig entschieden.
   Ansonsten FALSE und *Vergleich undefiniert.                                                                     */
{
  UTermT Index;

  if (TO_TermIstVar(Term1)) {
    if (TO_TermIstVar(Term2)) {
      *Vergleich = SO_SymbGleich(TO_TopSymbol(Term1),TO_TopSymbol(Term2)) ? Gleich : Unvergleichbar;
      return TRUE;
    }
    else {
      TO_forStellenOT(Term2,Index,
        if (SO_SymbGleich(TO_TopSymbol(Term1),TO_TopSymbol(Index))) {
          *Vergleich = Kleiner; return TRUE;
        }
      )
      *Vergleich = Unvergleichbar; return TRUE;
    }
  }
  else if (TO_TermIstVar(Term2)) {
    TO_forStellenOT(Term1,Index,
      if (SO_SymbGleich(TO_TopSymbol(Term2),TO_TopSymbol(Index))) {
        *Vergleich = Groesser; return TRUE;
      }
    )
    *Vergleich = Unvergleichbar; return TRUE;
  }
  else
    return FALSE;
}


static BOOLEAN VortestLPOKonst(UTermT Term1, UTermT Term2, VergleichsT *Vergleich)
/* Behandelt simple Faelle: Term1 oder Term2 Konstante.
   Ergebnis TRUE, falls dem so ist. - Zu Rate gezogen werden schon hier die Praezedenzen auf Konstanten, mit denen
   sich, falls eine Konstante im Spiel ist, die Entscheidungsfunktion in einem einzigen Durchlauf des jeweils
   anderen Terms berechnen laesst.                                                                                  */
{
  UTermT Index;
  SymbolT Symbol;
  VergleichsT Vgl;
  BOOLEAN Flag;

  if (TO_Nullstellig(Term1)) {            /* Dann ist Term1 eine Konstante, da vorher VortestLPOVar aufgerufen wurde. */
    if (SO_SymbGleich(TO_TopSymbol(Term1),TO_TopSymbol(Term2))) {
      *Vergleich = Gleich; return TRUE;
    }
    else {
      Symbol = TO_TopSymbol(Term1); Flag = TRUE;
      TO_doStellen(Term2,Index,
        if (TO_TermIstVar(Index))
          Flag = FALSE;
        else {
          Vgl = PR_Praezedenz(Symbol,TO_TopSymbol(Index));
          if ((Vgl==Kleiner) || (Vgl==Gleich)) {
            *Vergleich = Kleiner; return TRUE;
          }
          else if (Vgl==Unvergleichbar)
            Flag = FALSE;
        }
      )
      *Vergleich = Flag ? Groesser : Unvergleichbar; return TRUE;
    }
  }
  else if (TO_Nullstellig(Term2)) {       /* Nun in umgekehrter Richtung. Term1 ist offensichtlich keine Konstante. */
    Symbol = TO_TopSymbol(Term2); Flag = TRUE;
    TO_doStellen(Term1,Index,
      if (TO_TermIstVar(Index))
        Flag = FALSE;
      else {
        Vgl = PR_Praezedenz(TO_TopSymbol(Index),Symbol);
        if ((Vgl==Groesser) || (Vgl==Gleich)) {
          *Vergleich = Groesser; return TRUE;
        }
        else if (Vgl==Unvergleichbar)
          Flag = FALSE;
      }
    )
    *Vergleich = Flag ? Kleiner : Unvergleichbar; return TRUE;
  }
  else
    return FALSE;
}


BOOLEAN LV_VortestLPO(UTermT *Term1, UTermT *Term2, VergleichsT *Vergleich)
/* prueft, ob einer der Terme Teilterm des anderen ist, in welchem Verhaeltnis die Variablenmengen zueinander ste-
   hen, ob die Terme gleich sind, kuerzt Term1 und Term2. Es wird TRUE zurueckgegeben, wenn schon hier geklaert
   werden konnte, in welchem Verhaeltnis Term1 und Term2 stehen; dann steht dieses Verhaeltnis in *Vergleich.
   Sonst wird FALSE geliefert; *Vergleich enthaelt das Ergebnis des Variablenmengenvergleichs. Sind die Mengen un-
   vergleichbar, wird natuerlich TRUE zurueckgegeben.                                                              */
{
  UTermT Index;
  TermlistenT Kandidat;
  VMengenT VarMenge1, VarMenge2;
  SymbolT Symbol;
  TermeKuerzen(*Term1,*Term2);                                  /* alle gleichen einstelligen Topsymbole entfernen */
  if (VortestLPOVar(*Term1,*Term2,Vergleich))                                          /* Trivialitaeten abhandeln */
    return TRUE;
  else if (VortestLPOKonst(*Term1,*Term2,Vergleich))
    return TRUE;

  LeereTermliste(); VM_NeueMengeSeiLeer(VarMenge1);  
  if (SO_SymbGleich(TO_TopSymbol(*Term1),TO_TopSymbol(*Term2))) {
    TermEinfuegen(*Term2);
    AeltestesBeachten();
  }
  /* In der nachfolgenden Schleife wird Term1 symbolweise von links nach rechts durchlaufen, beginnend mit dem
     Topsymbol des ersten Teilterms. Termliste enthaelt eine Liste von Termstellen von Term2, bis zu denen vom
     Anfang von Term2 her Gleichheit mit einem Teilterm von Term1 gilt. Bei jedem Schleifendurchlauf wird fuer die
     Elemente von Termliste geprueft, ob die dort gespeicherte Stelle das Ende von Term2 bezeichnet. Term2 ist
     dann echter Teilterm von Term1 (wg. "echt" s.u.). Sonst wird geprueft, ob das gerade bezeichnete Symbol dem
     aktuellen in Term1 entspricht. Falls ja, wird auf die naechste Termstelle fortgeschaltet, falls nein Termli-
     ste entsprechend gekuerzt.
     Es muss unterschieden werden koennen zwischen Gleichheit auf Toplevel und Gleichheit mit einem Teilterm. Zu
     diesem Behuf beginnt die Suche in der Schleife erst mit dem Topsymbol des ersten Teilterms. Sind die Topsym-
     bole von Term1 und Term2 gleich, so wird oben ein besonderer Kandidat in die Termliste geschoben und als
     "aeltester" bei den Einfuege- und Loeschoperationen verfolgt. Term1 und Term2 sind gleich, wenn nach Abarbei-
     tung der Symbole von Term1 noch der erste Matchversuch in der Termliste steht. Daher darf in der Schleife
     nicht der Test auf Termende der letzten verbliebenen Kandidaten erfolgen. Zu diesem Zweck wurden Zeigerfort-
     schaltung und Abfrage auf Termende in zwei Schleifendurchlaeufe gelegt; nach dem Verlassen der Schleife
     muss dann noch in der Termliste nach Termende-Stellen gesucht werden.
     Zu beachten ist noch, dass TermEinfuegen nicht die Stelle des ersten Symbols mit Gleichheit speichert, son-
     dern schon die naechste. Diese ist aber von NULLTerm verschieden, weil Variablen und Konstanten schon oben
     abgearbeitet worden sind.
     Ausserdem wird die Menge der Variablen in Term1 mit Hilfe der Operationen aus VariablenMengen gebildet.       */
  TO_doStellenOT(*Term1,Index,                /* Erstens begonnene Ueberdeckungsversuche fortsetzen oder abbrechen */
    Symbol = TO_TopSymbol(Index);
    Kandidat = Termliste;
    while (Kandidat) {
      if (TO_PhysischGleich(Kandidat->Term,Kandidat->NULLTerm)) {
        *Vergleich = Groesser; VM_MengeLoeschen(&VarMenge1);                /* Term2 ist echter Teilterm von Term1 */
        TermlisteLoeschen(); return TRUE;                       /* da wegen Kandidat<>NULL die Terme ungleich sind */
      }
      else if (SO_SymbGleich(TO_TopSymbol(Kandidat->Term),Symbol)) {
        Kandidat->Term = TO_ErsterTeilterm(Kandidat->Term);
        Kandidat = Kandidat->Nachf;
      }
      else
        TermEntfernen(&Kandidat);
    }
    if (SO_SymbGleich(Symbol,TO_TopSymbol(*Term2)))      /* Zweitens ggf. einen neuen Ueberdeckungsversuch starten */
      TermEinfuegen(*Term2);
    if (SO_SymbIstVar(Symbol))
      VM_ElementEinfuegen(&VarMenge1,Symbol);
  )
  if (AeltestesNochInTermliste()) {
    *Vergleich = Gleich; TermlisteLoeschen(); VM_MengeLoeschen(&VarMenge1); return TRUE;
  }
  forListe(Termliste,Kandidat,
    if (TO_PhysischGleich(Kandidat->Term,Kandidat->NULLTerm)) {
      *Vergleich = Groesser; VM_MengeLoeschen(&VarMenge1);                   /* Term2 ist echter Teilterm von Term1 */
      TermlisteLoeschen(); return TRUE;                                                   /* da Aeltestes entfernt */
    }
  )
  TermlisteLoeschen();

  LeereTermliste(); VM_NeueMengeSeiLeer(VarMenge2);
  /* Die Schleife fuer den Test, ob Term1 Teilterm von Term2 ist, arbeitet einfacher, da hier nicht mehr auf Gleich-
     heit getestet werden muss. Dementsprechend erfolgen Weiterschalten der Termstelle und Test auf NULLTerm schon
     in einem Schleifendurchlauf.                                                                                  */
  TO_doStellenOT(*Term2,Index,                /* Erstens begonnene Ueberdeckungsversuche fortsetzen oder abbrechen */
    Symbol = TO_TopSymbol(Index);
    Kandidat = Termliste;
    while (Kandidat) {
      if (SO_SymbGleich(TO_TopSymbol(Kandidat->Term),Symbol)) {
        if (TO_PhysischGleich(Kandidat->Term = TO_ErsterTeilterm(Kandidat->Term),Kandidat->NULLTerm)) {
          *Vergleich = Kleiner; VM_MengeLoeschen(&VarMenge1);                /* Term1 ist echter Teilterm von Term2 */
          VM_MengeLoeschen(&VarMenge2); TermlisteLoeschen(); return TRUE;
        }
        else
          Kandidat = Kandidat->Nachf;
      }
      else
        TermEntfernen(&Kandidat);
    }
    if (SO_SymbGleich(Symbol,TO_TopSymbol(*Term1)))      /* Zweitens ggf. einen neuen Ueberdeckungsversuch starten */
      TermEinfuegen(*Term1);
    if (SO_SymbIstVar(Symbol))
      VM_ElementEinfuegen(&VarMenge2,Symbol); 
 ) 
  TermlisteLoeschen();
  
  /* Falls Term1 und Term2 in keiner Teiltermrelation stehen, werden nun die Variablenmengen verglichen.           */
  *Vergleich = VM_MengenVergleich(&VarMenge1,&VarMenge2);
  VM_MengeLoeschen(&VarMenge1); VM_MengeLoeschen(&VarMenge2); return *Vergleich==Unvergleichbar;
}



/*=================================================================================================================*/
/* V. Vortest fuer Testfunktion                                                                                    */
/*=================================================================================================================*/

static BOOLEAN VortestLPOGroesserVar(UTermT Term1, UTermT Term2, BOOLEAN *Vergleich)
/* Behandelt simple Faelle: Term1 oder Term2 Variable. Ergebnis TRUE und *Vergleich Vergleichsresultat, falls
   einer der beteiligten Terme eine Variable ist; damit ist der Termvergleich schon endgueltig entschieden.
   Ansonsten FALSE und *Vergleich undefiniert.                                                                     */
{
  UTermT Index;
  SymbolT Symbol;

  if (TO_TermIstVar(Term1)) {
    *Vergleich = FALSE; return TRUE;
  }
  else if (TO_TermIstVar(Term2)) {
    Symbol = TO_TopSymbol(Term2);
    TO_forStellenOT(Term1,Index,
      if (SO_SymbGleich(TO_TopSymbol(Index),Symbol)) {
        *Vergleich = TRUE; return TRUE;
      }
    )
    *Vergleich = FALSE; return TRUE;
  }
  else
    return FALSE;
}


static BOOLEAN VortestLPOGroesserKonst(UTermT Term1, UTermT Term2, BOOLEAN *Vergleich)
/* Behandelt simple Faelle: Term1 oder Term2 Konstante.
   Ergebnis TRUE, falls dem so ist. - Zu Rate gezogen werden schon hier die Praezedenzen auf Konstanten, mit denen
   sich, falls eine Konstante im Spiel ist, die Entscheidungsfunktion in einem einzigen Durchlauf des jeweils
   anderen Terms berechnen laesst.                                                                                 */
{
  UTermT Index;
  SymbolT Symbol;
  VergleichsT Vgl;

  if (TO_Nullstellig(Term1)) {         /* Dann ist Term1 eine Konstante, da vorher VortestLPOVar aufgerufen wurde. */
    Symbol = TO_TopSymbol(Term1);
    TO_doStellen(Term2,Index,
      if (TO_TermIstVar(Index)) {
        *Vergleich = FALSE; return TRUE;
      }
      else if (PR_Praezedenz(Symbol,TO_TopSymbol(Index))!=Groesser) {
        *Vergleich = FALSE; return TRUE;
      }
    )
    *Vergleich = TRUE; return TRUE;
  }
  else if (TO_Nullstellig(Term2)) {      /* Nun in umgekehrter Richtung. Term1 ist offensichtlich keine Konstante. */
    Symbol = TO_TopSymbol(Term2);
    TO_doStellen(Term1,Index,
      if (TO_TermIstNichtVar(Index)) {
        Vgl = PR_Praezedenz(TO_TopSymbol(Index),Symbol);
        if ((Vgl==Groesser) || (Vgl==Gleich)) {
          *Vergleich = TRUE; return TRUE;
        }
      }
    )
    *Vergleich = FALSE; return TRUE;
  }
  else
    return FALSE;
}


BOOLEAN LV_VortestLPOGroesser(UTermT *Term1, UTermT *Term2, BOOLEAN *Vergleich)
{
  UTermT Index;
  TermlistenT Kandidat;
  VMengenT VarMenge;
  SymbolT Symbol;

  /* IO_DruckeFlex("VortestLPOGroesser %t %t\n",*Term1,*Term2); */
  TermeKuerzen(*Term1,*Term2);                                  /* alle gleichen einstelligen Topsymbole entfernen */
  if (VortestLPOGroesserVar(*Term1,*Term2,Vergleich))                                  /* Trivialitaeten abhandeln */
    return TRUE;
  else if (VortestLPOGroesserKonst(*Term1,*Term2,Vergleich))
    return TRUE;

  LeereTermliste(); VM_NeueMengeSeiLeer(VarMenge);  
  if (SO_SymbGleich(TO_TopSymbol(*Term1),TO_TopSymbol(*Term2))) {
    TermEinfuegen(*Term2);
    AeltestesBeachten();
  }
  /* In der nachfolgenden Schleife wird Term1 symbolweise von links nach rechts durchlaufen, beginnend mit dem
     Topsymbol des ersten Teilterms. Termliste enthaelt eine Liste von Termstellen von Term2, bis zu denen vom
     Anfang von Term2 her Gleichheit mit einem Teilterm von Term1 gilt. Bei jedem Schleifendurchlauf wird fuer die
     Elemente von Termliste geprueft, ob die dort gespeicherte Stelle das Ende von Term2 bezeichnet. Term2 ist
     dann echter Teilterm von Term1 (wg. "echt" s.u.). Sonst wird geprueft, ob das gerade bezeichnete Symbol dem
     aktuellen in Term1 entspricht. Falls ja, wird auf die naechste Termstelle fortgeschaltet, falls nein Termli-
     ste entsprechend gekuerzt.
     Es muss unterschieden werden koennen zwischen Gleichheit auf Toplevel und Gleichheit mit einem Teilterm. Zu
     diesem Behuf beginnt die Suche in der Schleife erst mit dem Topsymbol des ersten Teilterms. Sind die Topsym-
     bole von Term1 und Term2 gleich, so wird oben ein besonderer Kandidat in die Termliste geschoben und als
     "aeltester" bei den Einfuege- und Loeschoperationen verfolgt. Term1 und Term2 sind gleich, wenn nach Abarbei-
     tung der Symbole von Term1 noch der erste Matchversuch in der Termliste steht. Daher darf in der Schleife
     nicht der Test auf Termende der letzten verbliebenen Kandidaten erfolgen. Zu diesem Zweck wurden Zeigerfort-
     schaltung und Abfrage auf Termende in zwei Schleifendurchlaeufe gelegt; nach dem Verlassen der Schleife
     muss dann noch in der Termliste nach Termende-Stellen gesucht werden.
     Zu beachten ist noch, dass TermEinfuegen nicht die Stelle des ersten Symbols mit Gleichheit speichert, son-
     dern schon die naechste. Diese ist aber von NULLTerm verschieden, weil Variablen und Konstanten schon oben
     abgearbeitet worden sind.
     Ausserdem wird die Menge der Variablen in Term1 mit Hilfe der Operationen aus VariablenMengen gebildet.       */
  TO_doStellenOT(*Term1,Index,                /* Erstens begonnene Ueberdeckungsversuche fortsetzen oder abbrechen */
    Symbol = TO_TopSymbol(Index);
    Kandidat = Termliste;
    while (Kandidat) {
      if (TO_PhysischGleich(Kandidat->Term,Kandidat->NULLTerm)) {
        *Vergleich = TRUE; VM_MengeLoeschen(&VarMenge);                     /* Term2 ist echter Teilterm von Term1 */
        TermlisteLoeschen(); return TRUE;                       /* da wegen Kandidat<>NULL die Terme ungleich sind */
      }
      else if (SO_SymbGleich(TO_TopSymbol(Kandidat->Term),Symbol)) {
        Kandidat->Term = TO_ErsterTeilterm(Kandidat->Term);
        Kandidat = Kandidat->Nachf;
      }
      else
        TermEntfernen(&Kandidat);
    }
    if (SO_SymbGleich(Symbol,TO_TopSymbol(*Term2)))      /* Zweitens ggf. einen neuen Ueberdeckungsversuch starten */
      TermEinfuegen(*Term2);
    if (SO_SymbIstVar(Symbol))
      VM_ElementEinfuegen(&VarMenge,Symbol);
  )
  if (AeltestesNochInTermliste()) {
    *Vergleich = FALSE; TermlisteLoeschen(); VM_MengeLoeschen(&VarMenge); return TRUE;
  }
  forListe(Termliste,Kandidat,
    if (TO_PhysischGleich(Kandidat->Term,Kandidat->NULLTerm)) {
      *Vergleich = TRUE; VM_MengeLoeschen(&VarMenge);                       /* Term2 ist echter Teilterm von Term1 */
      TermlisteLoeschen(); return TRUE;                                                   /* da Aeltestes entfernt */
    }
  )
  TermlisteLoeschen();

  LeereTermliste();
  /* Die Schleife fuer den Test, ob Term1 Teilterm von Term2 ist, arbeitet einfacher, da hier nicht mehr auf Gleich-
     heit getestet werden muss. Dementsprechend erfolgen Weiterschalten der Termstelle und Test auf NULLTerm schon
     in einem Schleifendurchlauf.                                                                                  */
  TO_doStellenOT(*Term2,Index,                /* Erstens begonnene Ueberdeckungsversuche fortsetzen oder abbrechen */
    Symbol = TO_TopSymbol(Index);
    Kandidat = Termliste;
    while (Kandidat) {
      if (SO_SymbGleich(TO_TopSymbol(Kandidat->Term),Symbol)) {
        if (TO_PhysischGleich(Kandidat->Term = TO_ErsterTeilterm(Kandidat->Term),Kandidat->NULLTerm)) {
          *Vergleich = FALSE; VM_MengeLoeschen(&VarMenge);                  /* Term1 ist echter Teilterm von Term2 */
          TermlisteLoeschen(); return TRUE;
        }
        else
          Kandidat = Kandidat->Nachf;
      }
      else
        TermEntfernen(&Kandidat);
    }
    if (SO_SymbGleich(Symbol,TO_TopSymbol(*Term1)))      /* Zweitens ggf. einen neuen Ueberdeckungsversuch starten */
      TermEinfuegen(*Term1);
    if ((SO_SymbIstVar(Symbol)) && VM_NichtEnthaltenIn(&VarMenge,Symbol)) {
      *Vergleich = FALSE; VM_MengeLoeschen(&VarMenge);
      TermlisteLoeschen(); return TRUE;
    }
 ) 
  TermlisteLoeschen();

  VM_MengeLoeschen(&VarMenge); return FALSE;
}



/*=================================================================================================================*/
/* VI. Vortest fuer eingeschraenkte Entscheidungsfunktion GroesserGleich                                           */
/*=================================================================================================================*/

static BOOLEAN VortestLPOGroesserGleichVar(UTermT Term1, UTermT Term2, VergleichsT *Vergleich)
/* Behandelt simple Faelle: Term1 oder Term2 Variable. Ergebnis TRUE und *Vergleich Vergleichsresultat, falls
   einer der beteiligten Terme eine Variable ist; damit ist der Termvergleich schon endgueltig entschieden.
   Ansonsten FALSE und *Vergleich undefiniert.                                                                     */
{
  UTermT Index;
  SymbolT Symbol;

  if (TO_TermIstVar(Term1)) {
    *Vergleich = SO_SymbGleich(TO_TopSymbol(Term1),TO_TopSymbol(Term2)) ? Gleich : Unvergleichbar;
    return TRUE;
  }
  else if (TO_TermIstVar(Term2)) {
    Symbol = TO_TopSymbol(Term2);
    TO_forStellenOT(Term1,Index,
      if (SO_SymbGleich(TO_TopSymbol(Index),Symbol)) {
        *Vergleich = Groesser; return TRUE;
      }
    )
    *Vergleich = Unvergleichbar; return TRUE;
  }
  else
    return FALSE;
}


static BOOLEAN VortestLPOGroesserGleichKonst(UTermT Term1, UTermT Term2, VergleichsT *Vergleich)
/* Behandelt simple Faelle: Term1 oder Term2 Konstante.
   Ergebnis TRUE, falls dem so ist. - Zu Rate gezogen werden schon hier die Praezedenzen auf Konstanten, mit denen
   sich, falls eine Konstante im Spiel ist, die Entscheidungsfunktion in einem einzigen Durchlauf des jeweils
   anderen Terms berechnen laesst.                                                                                  */
{
  UTermT Index;
  SymbolT Symbol;
  VergleichsT Vgl;

  if (TO_Nullstellig(Term1)) {          /* Dann ist Term1 eine Konstante, da vorher VortestLPOVar aufgerufen wurde. */
    Symbol = TO_TopSymbol(Term1);
    if (SO_SymbGleich(Symbol,TO_TopSymbol(Term2))) {
      *Vergleich = Gleich; return TRUE;
    }
    TO_doStellen(Term2,Index,
      if (TO_TermIstVar(Index)) {
        *Vergleich = Unvergleichbar; return TRUE;
      }
      else if (PR_Praezedenz(Symbol,TO_TopSymbol(Index))!=Groesser) {
        *Vergleich = Unvergleichbar; return TRUE;
      }
    )
    *Vergleich = Groesser; return TRUE;
  }
  else if (TO_Nullstellig(Term2)) {       /* Nun in umgekehrter Richtung. Term1 ist offensichtlich keine Konstante. */
    Symbol = TO_TopSymbol(Term2);
    TO_doStellen(Term1,Index,
      if (TO_TermIstNichtVar(Index)) {
        Vgl = PR_Praezedenz(TO_TopSymbol(Index),Symbol);
        if ((Vgl==Groesser) || (Vgl==Gleich)) {
          *Vergleich = Groesser; return TRUE;
        }
      }
    )
    *Vergleich = Unvergleichbar; return TRUE;
  }
  else
    return FALSE;
}


BOOLEAN LV_VortestLPOGroesserGleich(UTermT *Term1, UTermT *Term2, VergleichsT *Vergleich)
{
  UTermT Index;
  TermlistenT Kandidat;
  VMengenT VarMenge;
  SymbolT Symbol;

  TermeKuerzen(*Term1,*Term2);                                  /* alle gleichen einstelligen Topsymbole entfernen */
  if (VortestLPOGroesserGleichVar(*Term1,*Term2,Vergleich))                            /* Trivialitaeten abhandeln */
    return TRUE;
  else if (VortestLPOGroesserGleichKonst(*Term1,*Term2,Vergleich))
    return TRUE;

  LeereTermliste(); VM_NeueMengeSeiLeer(VarMenge);  
  if (SO_SymbGleich(TO_TopSymbol(*Term1),TO_TopSymbol(*Term2))) {
    TermEinfuegen(*Term2);
    AeltestesBeachten();
  }
  /* In der nachfolgenden Schleife wird Term1 symbolweise von links nach rechts durchlaufen, beginnend mit dem
     Topsymbol des ersten Teilterms. Termliste enthaelt eine Liste von Termstellen von Term2, bis zu denen vom
     Anfang von Term2 her Gleichheit mit einem Teilterm von Term1 gilt. Bei jedem Schleifendurchlauf wird fuer die
     Elemente von Termliste geprueft, ob die dort gespeicherte Stelle das Ende von Term2 bezeichnet. Term2 ist
     dann echter Teilterm von Term1 (wg. "echt" s.u.). Sonst wird geprueft, ob das gerade bezeichnete Symbol dem
     aktuellen in Term1 entspricht. Falls ja, wird auf die naechste Termstelle fortgeschaltet, falls nein Termli-
     ste entsprechend gekuerzt.
     Es muss unterschieden werden koennen zwischen Gleichheit auf Toplevel und Gleichheit mit einem Teilterm. Zu
     diesem Behuf beginnt die Suche in der Schleife erst mit dem Topsymbol des ersten Teilterms. Sind die Topsym-
     bole von Term1 und Term2 gleich, so wird oben ein besonderer Kandidat in die Termliste geschoben und als
     "aeltester" bei den Einfuege- und Loeschoperationen verfolgt. Term1 und Term2 sind gleich, wenn nach Abarbei-
     tung der Symbole von Term1 noch der erste Matchversuch in der Termliste steht. Daher darf in der Schleife
     nicht der Test auf Termende der letzten verbliebenen Kandidaten erfolgen. Zu diesem Zweck wurden Zeigerfort-
     schaltung und Abfrage auf Termende in zwei Schleifendurchlaeufe gelegt; nach dem Verlassen der Schleife
     muss dann noch in der Termliste nach Termende-Stellen gesucht werden.
     Zu beachten ist noch, dass TermEinfuegen nicht die Stelle des ersten Symbols mit Gleichheit speichert, son-
     dern schon die naechste. Diese ist aber von NULLTerm verschieden, weil Variablen und Konstanten schon oben
     abgearbeitet worden sind.
     Ausserdem wird die Menge der Variablen in Term1 mit Hilfe der Operationen aus VariablenMengen gebildet.       */
  TO_doStellenOT(*Term1,Index,                /* Erstens begonnene Ueberdeckungsversuche fortsetzen oder abbrechen */
    Symbol = TO_TopSymbol(Index);
    Kandidat = Termliste;
    while (Kandidat) {
      if (TO_PhysischGleich(Kandidat->Term,Kandidat->NULLTerm)) {
        *Vergleich = Groesser; VM_MengeLoeschen(&VarMenge);                 /* Term2 ist echter Teilterm von Term1 */
        TermlisteLoeschen(); return TRUE;                       /* da wegen Kandidat<>NULL die Terme ungleich sind */
      }
      else if (SO_SymbGleich(TO_TopSymbol(Kandidat->Term),Symbol)) {
        Kandidat->Term = TO_ErsterTeilterm(Kandidat->Term);
        Kandidat = Kandidat->Nachf;
      }
      else
        TermEntfernen(&Kandidat);
    }
    if (SO_SymbGleich(Symbol,TO_TopSymbol(*Term2)))      /* Zweitens ggf. einen neuen Ueberdeckungsversuch starten */
      TermEinfuegen(*Term2);
    if (SO_SymbIstVar(Symbol))
      VM_ElementEinfuegen(&VarMenge,Symbol);
  )
  if (AeltestesNochInTermliste()) {
    *Vergleich = Gleich; TermlisteLoeschen(); VM_MengeLoeschen(&VarMenge); return TRUE;
  }
  forListe(Termliste,Kandidat,
    if (TO_PhysischGleich(Kandidat->Term,Kandidat->NULLTerm)) {
      *Vergleich = Groesser; VM_MengeLoeschen(&VarMenge);                   /* Term2 ist echter Teilterm von Term1 */
      TermlisteLoeschen(); return TRUE;                                                   /* da Aeltestes entfernt */
    }
  )
  TermlisteLoeschen();

  LeereTermliste();
  /* Die Schleife fuer den Test, ob Term1 Teilterm von Term2 ist, arbeitet einfacher, da hier nicht mehr auf Gleich-
     heit getestet werden muss. Dementsprechend erfolgen Weiterschalten der Termstelle und Test auf NULLTerm schon
     in einem Schleifendurchlauf.                                                                                  */
  TO_doStellenOT(*Term2,Index,                /* Erstens begonnene Ueberdeckungsversuche fortsetzen oder abbrechen */
    Symbol = TO_TopSymbol(Index);
    Kandidat = Termliste;
    while (Kandidat) {
      if (SO_SymbGleich(TO_TopSymbol(Kandidat->Term),Symbol)) {
        if (TO_PhysischGleich(Kandidat->Term = TO_ErsterTeilterm(Kandidat->Term),Kandidat->NULLTerm)) {
          *Vergleich = Unvergleichbar; VM_MengeLoeschen(&VarMenge);         /* Term1 ist echter Teilterm von Term2 */
          TermlisteLoeschen(); return TRUE;
        }
        else
          Kandidat = Kandidat->Nachf;
      }
      else
        TermEntfernen(&Kandidat);
    }
    if (SO_SymbGleich(Symbol,TO_TopSymbol(*Term1)))      /* Zweitens ggf. einen neuen Ueberdeckungsversuch starten */
      TermEinfuegen(*Term1);
    if ((SO_SymbIstVar(Symbol)) && VM_NichtEnthaltenIn(&VarMenge,Symbol)) {
      *Vergleich = Unvergleichbar; VM_MengeLoeschen(&VarMenge);
      TermlisteLoeschen(); return TRUE;
    }
 ) 
  TermlisteLoeschen();

  VM_MengeLoeschen(&VarMenge); return FALSE;
}
