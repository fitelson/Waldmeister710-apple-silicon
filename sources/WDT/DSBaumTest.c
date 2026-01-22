/*=================================================================================================================
  =                                                                                                               =
  =  DSBaumTest.c                                                                                                 =
  =                                                                                                               =
  =  Datenstrukur auf Konsistenz ueberpruefen                                                                     =
  =                                                                                                               =
  =                                                                                                               =
  =  16.11.95  Thomas.                                                                                            =
  =================================================================================================================*/

#include "Ausgaben.h"
#include "compiler-extra.h"
#include "DSBaumOperationen.h"
#include "DSBaumTest.h"
#include "RUndEVerwaltung.h"
#include "WaldmeisterII.h"

#if RE_TEST

/*=================================================================================================================*/
/*= I. Testen der Sprungverweise                                                                                  =*/
/*=================================================================================================================*/

static BOOLEAN SprungeintraegeEinesKnotensKorrekt(KantenT Knoten)
{
  SprungeintragsT SprungIndex;
  unsigned int NrSprungeintrag = 0;

  BK_forSprungziele(SprungIndex,BK_Sprungausgaenge(Knoten)) {
    UTermT TermIndex     = SprungIndex->Teilterm,
           TermIndexVorg = NULL,
           NULLTerm;
    unsigned int AnzFehlenderArgumente = 1;

    KantenT BaumIndex    = Knoten;

    NrSprungeintrag++;

    /* zunaechst Korrektheit der Termdarstellung ueberpruefen */
    if (!TermIndex) {
      printf("\nFehler: Sprungverweis Nr. %d mit Termeintrag NULL\n",NrSprungeintrag);
      return FALSE;
    }
    do {
      SymbolT Symbol;
      if (!TermIndex) {
        printf("\nFehler: Sprungverweis Nr. %d mit vorzeitig endendem Termeintrag\n",NrSprungeintrag);
        return FALSE;
      }
      Symbol = TO_TopSymbol(TermIndex);
      if (SO_SymbIstFkt(Symbol) && Symbol>SO_GroesstesFktSymb) {
        printf("\nFehler: Termeintrag mit unzulaessigem Symbol bei Sprungeintrag Nr. %d\n",NrSprungeintrag);
        return FALSE;
      }
      AnzFehlenderArgumente += SO_Stelligkeit(Symbol)-1;
      TermIndex = TO_Schwanz(TermIndexVorg = TermIndex);
    } while (AnzFehlenderArgumente);
    if (TO_PhysischUngleich(TermIndexVorg,TO_TermEnde(SprungIndex->Teilterm))) {
      printf("\nFehler: Termeintrag, bei dem Endeverweis nicht mit tatsaechlichem Ende uebereinstimmt,"
        " bei Sprungeintrag Nr. %d\n",NrSprungeintrag);
      return FALSE;
    }

    /* nun gemaess Sprungterm im Baum abzusteigen versuchen */
    if (SprungIndex->Startknoten!=Knoten) {
      printf("\nFehler: Sprungeintrag Nr. %d mit falschem Startknoten\n",NrSprungeintrag);
      return FALSE;
    }
    TermIndex = SprungIndex->Teilterm;
    NULLTerm = TO_NULLTerm(TermIndex);
    do {
      BaumIndex = BK_Nachf(BaumIndex,TO_TopSymbol(TermIndex));
      TermIndex = TO_Schwanz(TermIndex);
      if (!BaumIndex) {
        printf("\nFehler: Zum angegebenen Sprungterm existiert kein Pfad bei Sprungeintrag Nr. %d.\n",NrSprungeintrag);
        return FALSE;
      }
    } while (!BK_IstBlatt(BaumIndex) && TO_PhysischUngleich(TermIndex,NULLTerm));
    if (BK_IstBlatt(BaumIndex)) {
      UTermT TermIndex2 = BK_Rest(BaumIndex);
      while (TO_PhysischUngleich(TermIndex,NULLTerm)) {
        if (!TermIndex2) {
          printf("\nFehler: Sprungtermentsprechung im Regelsuffix zu kurz bei Sprungeintrag Nr. %d\n",NrSprungeintrag);
          return FALSE;
        }
        else if (SO_SymbUngleich(TO_TopSymbol(TermIndex),TO_TopSymbol(TermIndex2))) {
          printf("\nFehler: keine Sprungtermentsprechung im Regelsuffix bei Sprungeintrag Nr. %d\n",NrSprungeintrag);
          return FALSE;
        }
        TermIndex = TO_Schwanz(TermIndex);
        TermIndex2 = TO_Schwanz(TermIndex2);
      }
    }
    if (BaumIndex!=SprungIndex->Zielknoten) {
      printf("\nFehler: Falscher Zielknoten wurde erreicht bei Sprungeintrag Nr. %d.\n",NrSprungeintrag);
      return FALSE;
    }
  }

  return TRUE;
}


static BOOLEAN SpruengeInsBlattKorrekt(KantenT Blatt)
{
  SprungeintragsT SprungIndex = BK_Sprungeingaenge(Blatt);

  while (SprungIndex)
    if (SprungIndex->Praefixeintrag) {
      printf("\nFehler: Sprung mit Praefixverweis geht in Blattknoten.\n");
      return FALSE;
    }
    else
      SprungIndex = SprungIndex->NaechsterStarteintrag;

  return TRUE;
}


static BOOLEAN SprungeintraegeKorrektRek(KantenT Knoten)
{
  SymbolT SymbolIndex;

  if (!SprungeintraegeEinesKnotensKorrekt(Knoten)) {
    printf("Fehler aufgetreten nach einer Kante mit Symbol ");
    IO_DruckeSymbol(BK_VorgSymb(Knoten));
    printf("\nFehlerpfad rueckwaerts: ");
    return FALSE;
  }

  SO_forSymboleAufwaerts(SymbolIndex,BK_minSymb(Knoten),BK_maxSymb(Knoten)) {
    KantenT Nachf = BK_Nachf(Knoten,SymbolIndex);
    if (!Nachf)
      continue;
    switch (BK_IstBlatt(Nachf)) {
    case TRUE:
      if (SpruengeInsBlattKorrekt(Nachf))
        continue;
      break;
    case FALSE:
      if (SprungeintraegeKorrektRek(Nachf))
        continue;
      break;
    }
    IO_DruckeSymbol(SymbolIndex);
    printf(" ");
    return FALSE;
  }

  return TRUE;
}


static BOOLEAN SprungverweiseTesten(DSBaumT Baum)
{
  if (Baum.Wurzel) {
    if (BK_Sprungausgaenge(Baum.Wurzel)) {
      printf("\nFehler: Wurzel ist Sprungausgang.\n");
      return FALSE;
    }
    return SprungeintraegeKorrektRek(Baum.Wurzel);
  }
  return TRUE;
}



/*=================================================================================================================*/
/*= II. Testen von inneren und Blattknoten                                                                        =*/
/*=================================================================================================================*/

static SymbolT *Stapel;
static unsigned int StapelDimension,
                    StapelZeiger;


static void Push(SymbolT Symbol)
{
  if (StapelZeiger==StapelDimension)
    posarray_realloc(Stapel,StapelDimension += BT_ErsteStapelhoehe,SymbolT,1);
  Stapel[++StapelZeiger] = Symbol;
}


static void Pop(void)
{
  StapelZeiger--;
}


static void FehlerpfadAusgeben(void)
{
  unsigned int i = 1;
  printf("Pfad zur Fehlerstelle: ");
  while (i<=StapelZeiger) {
    IO_DruckeSymbol(Stapel[i++]);
    printf(" ");
  }
  printf("\n");
}


static BT_TPPraedikatsT TPPraedikat;

static KantenT ErstesBlatt;

static BOOLEAN EinenKnotenTesten(KantenT Knoten, KantenT Vorg, SymbolT VorgSymbol);


static BOOLEAN EinenInnerenKnotenTesten(KantenT Knoten)
{
  /* 1. Sitzen minSymb und maxSymb korrekt? */

  SymbolT SymbolIndex;
  BOOLEAN NachfolgerVorhanden = FALSE;

  if (!BK_Nachf(Knoten,BK_minSymb(Knoten))) {
    printf("\nFehler: Zu innerem Knoten kein Nachfolger zu minSymb vorhanden.\n");
    return FALSE;
  }
  if (!BK_Nachf(Knoten,BK_maxSymb(Knoten))) {
    printf("\nFehler: Zu innerem Knoten kein Nachfolger zu maxSymb vorhanden.\n");
    return FALSE;
  }

  SO_forSymboleAufwaerts(SymbolIndex,BK_letzteVar(Knoten),SO_GroesstesFktSymb)
    if (BK_Nachf(Knoten,SymbolIndex)) {
      NachfolgerVorhanden = TRUE;
      if (SymbolIndex<BK_minSymb(Knoten)) {
        printf("\nFehler: Innerer Knoten mit Nachfolger zu Symbol kleiner als minSymb");
        return FALSE;
      }
      else if (SymbolIndex>BK_maxSymb(Knoten)) {
        printf("\nFehler: Innerer Knoten mit Nachfolger zu Symbol groesser als maxSymb");
        return FALSE;
      }
    }

  /* 2. Existieren ueberhaupt Nachfolgerknoten? */

  if (!NachfolgerVorhanden) {
    printf("\nFehler: Innerer Knoten ohne eingetragenen Nachfolger.\n");
    return FALSE;
  }

  /* Rekursion */

  SO_forSymboleAufwaerts(SymbolIndex,BK_minSymb(Knoten),BK_maxSymb(Knoten)) {
    KantenT Nachf = BK_Nachf(Knoten,SymbolIndex);
    if (Nachf) {
      Push(SymbolIndex);
      if (!EinenKnotenTesten(Nachf,Knoten,SymbolIndex))
        return FALSE;
    Pop();
    }
  }

  return TRUE;
}


static BOOLEAN EinenBlattknotenTesten(KantenT Blatt)
{
  GNTermpaarT TPIndex = BK_Regeln(Blatt);
  KantenT BlattIndex = ErstesBlatt;
  unsigned int i = 1;
  TermT TermIndex = BK_linkeSeite(Blatt);

  /* 3. Entspricht der Praefix der im Blatt angegebenen linken Seite dem Term auf den Kanten? */

  do
    if (SO_SymbUngleich(Stapel[i],TO_TopSymbol(TermIndex))) {
      printf("\nFehler: Blatt mit von dem Pfadeintrag verschiedenem Praefix\n");
      return FALSE;
    }
  while (TermIndex = TO_Schwanz(TermIndex), ++i<=StapelZeiger);


  /* 4. Steht Suffix in BK_linkeSeite? */

  TermIndex = BK_linkeSeite(Blatt);
  i = 1;
  while (TermIndex && TO_PhysischUngleich(TermIndex,BK_Rest(Blatt))) {
    TermIndex = TO_Schwanz(TermIndex);
    i++;
  }
  if (TO_PhysischUngleich(TermIndex,BK_Rest(Blatt))) {
    printf("\nFehler: Blatt mit Suffixeintrag, der nicht in indizierte linke Seite verweist.\n");
    return FALSE;
  }

  /* 5. Testen von Suffix und SuffixPosition */

  if (i!=BK_SuffixPosition(Blatt)) {
    printf("\nFehler: Suffix und Suffixposition passen nicht zusammen.\n");
    return FALSE;
  }

  /* 6. Kann das Blatt auch ueber ErstesBlatt gefunden werden? */

  while (BlattIndex && BlattIndex!=Blatt)
    BlattIndex = BK_NachfBlatt(BlattIndex);
  if (!BlattIndex) {
    printf("\nFehler: Von der Wurzel aus erreichtes Blatt nicht in der Blaetterverzeigerung eingetragen.\n");
    return FALSE;
  }

  /* 7. Verweist BK_linkeSeite in das letzte Termpaar aus der Verzeigerung? */

  if (!TPIndex) {
    printf("\nFehler: Blatt ohne eingetragene Termpaare.\n");
    return FALSE;
  }
  while (TP_Nachf(TPIndex))
    TPIndex = TP_Nachf(TPIndex);
  if (TO_PhysischUngleich(TP_LinkeSeite(TPIndex),BK_linkeSeite(Blatt))) {
    printf("\nFehler: Blattverweis auf linke Seite geht nicht in linke Seite des letzten Termpaares.\n");
    return FALSE;
  }

  TPIndex = BK_Regeln(Blatt);
  do {

    /* 8. Haben alle an diesem Blatt angehaengten Termpaare syntaktisch dieselbe linke Seite? */

    if (!TO_TermeGleich(TP_LinkeSeite(TPIndex),BK_linkeSeite(Blatt))) {
      printf("\nFehler: In einem Blatt Termpaare mit unterschiedlichen linken Seiten.\n");
      return FALSE;
    }

    /* 9. Haben die Termpaare den richtigen Typ? */

    if (!(*TPPraedikat)(TPIndex)) {
      printf("\nFehler: Termpaar in Blatt hat nicht den richtigen Typ.\n");
      return FALSE;
    }

  } while ((TPIndex = TP_Nachf(TPIndex)));

  return TRUE;
}


static BOOLEAN EinenKnotenTesten(KantenT Knoten, KantenT Vorg, SymbolT VorgSymbol)
{
  /* 10. Sind die Knoten untereinander richtig verzeigert? */

  if (BK_Vorg(Knoten)!=Vorg) {
    printf("\nFehler: Vorgaengerverzeigerung der Knoten nicht korrekt.\n");
    return false;
  }

  /* 11. Ist das richtige Vorgaengersymbol eingetragen? */

  if (BK_VorgSymb(Knoten)!=VorgSymbol) {
    printf("\nFehler: Vorgaengersymbol eines Knotens nicht korrekt.\n");
    return false;
  }

  return BK_IstBlatt(Knoten) ?
    EinenBlattknotenTesten(Knoten) : EinenInnerenKnotenTesten(Knoten);
}


static BOOLEAN AlleKnotenTesten(DSBaumT Baum, BT_TPPraedikatsT Praedikat)

/* I. Innere Knoten
   ----------------
   1. Sitzen minSymb und maxSymb korrekt?
   2. Existieren ueberhaupt Nachfolgerknoten?

   II. Blattknoten
   ---------------
   3. Entspricht der Praefix der im Blatt angegebenen linken Seite dem Term auf den Kanten?
   4. Steht Suffix in BK_linkeSeite?
   5. Testen von Suffix und SuffixPosition
   6. Kann das Blatt auch ueber ErstesBlatt gefunden werden?
   7. Verweist BK_linkeSeite in das letzte Termpaar aus der Verzeigerung?
   8. Haben alle an diesem Blatt angehaengten Termpaare syntaktisch dieselbe linke Seite?
   9. Haben die Termpaare den richtigen Typ?

   III. Allgemeine Knoten
   ----------------------
   10. Sind die Knoten untereinander richtig verzeigert?
   11. Ist das richtige Vorgaengersymbol eingetragen?

   IV. DSBaum
   ----------------------
   12. Hat nichtleerer Baum Eintrag fuer ErstesBlatt?
   13. Hat nichtleerer Baum inneren Knoten als Wurzel? */

{
  if (Baum.Wurzel) {
    TPPraedikat = Praedikat;                               /* merken fuer spaetere Aufrufe aus tieferen Funktionen */
    ErstesBlatt = Baum.ErstesBlatt;

    /* 12. Hat nichtleerer Baum Eintrag fuer ErstesBlatt? */

    if (!ErstesBlatt) {
      printf("\nFehler: Nichtleerer Baum ohne Eintrag fuer Erstes Blatt.\n");
      return FALSE;
    }

    /* 13. Hat nichtleerer Baum inneren Knoten als Wurzel? */

    if (BK_IstBlatt(Baum.Wurzel)) {
      printf("\nFehler: Wurzelknoten ist Blatt.\n");
      return FALSE;
    }

    if (EinenKnotenTesten(Baum.Wurzel,NULL,SO_KleinstesFktSymb))
      return TRUE;

    FehlerpfadAusgeben();
    return FALSE;
  }

  return TRUE;
}



/*=================================================================================================================*/
/*= III. Testen der Verzeigerung ueber TiefeBlaetter                                                              =*/
/*=================================================================================================================*/

static BOOLEAN BlattAuffindbar(KantenT Blatt, DSBaumT Baum)
{
  KantenT Knoten = Baum.Wurzel;
  TermT TermIndex = BK_linkeSeite(Blatt);

  do
    if (!(Knoten = BK_Nachf(Knoten,TO_TopSymbol(TermIndex)))) {
      printf("\nFehler: Beim Suchen eines Blattes zu innerem Knoten nicht den richtigen Nachfolger gefunden.\n");
      return FALSE;
    }
  while (TermIndex = TO_Schwanz(TermIndex),!BK_IstBlatt(Knoten));

  if (Knoten != Blatt) {
    printf("\nFehler: Beim Suchen eines Blattes zu anderem Blatt gelangt.\n");
    return FALSE;
  }

  return TRUE;
}


static BOOLEAN TiefeBlaetterTesten(DSBaumT Baum)
{
/* Die Blatter sind als doppelt verkettete Liste miteinander verzeigert.
   Die Liste ist steigend sortiert nach der Tiefe der zu den Blaettern gehoerenden linken Seiten.
   In der Struktur DSBaumT existiert ein Feld TiefeBlaetter, das eine Abbildung Nat -> Blaetter U NULL realisiert.
   Die Ausdehnung des um -1 verschobenen Feldes ist in TiefeBlaetterDimension vermerkt.
   TiefeBlaetter[n] verweist fuer 1<=n<=TiefeBlaetterDimension auf das erste Blatt mit der Tiefe n, falls ein
   solches existiert, andernfalls auf NULL.
   Die maximal auftretende Tiefe wird in maxTiefeLinks vorraetig gehalten.
   Die Komponente ErstesBlatt verweist jeweils auf das erste Blatt mit der niedrigsten Termtiefe.
*/

/* I. Sonderfall: leerer Baum
   --------------------------
   1. Ist ErstesBlatt gleich NULL?
   2. Ist maxTiefeLinks gleich Null?
   3. Sind alle Feldeintraege gleich NULL?

   II. Durchlauf durch Feldeintraege 1 bis TiefeBlaetterDimension
   --------------------------------------------------------------
   4. Hat nichtleerer Baum Eintrag fuer ErstesBlatt?
   5. Ist maxTiefeLinks nicht groesser als TiefeBlaetterDimension?
   6. Existiert ueberhaupt ein indiziertes Blatt?
   7. Ist das erste so indizierte Blatt mit ErstesBlatt identisch?
   8. Hat die zum Blatt gehoerende linke Seite die geforderte Tiefe?
   9. Laesst sich das Blatt ueber die Wurzel finden?
   10. Gibt es Eintraege nach maxTiefeLinks?

   III. Durchlaufen der Blaetterverzeigerung
   -----------------------------------------
   11. Sind die Blaetter korrekt untereinander verzeigert? (Vorg-Nachf-Entsprechung)
   12. Laesst sich jedes Blatt auch ueber die Wurzel finden?
   13. Sind die Tiefen monoton steigend?
   14. Ist ein Blatt mit derselben Tiefe im Feld TiefeBlaetter vermerkt?
   15. Ist das je erste Blatt mit groesserer Tiefe auch im Feld TiefeBlaetter vermerkt? */

  KantenT BlattIndex,
          BlattIndexVorg;
  unsigned int i,
               bisherigeTiefe = 0,
               aktuelleTiefe;


  /* I. Sonderfall: leerer Baum */
  /* -------------------------- */

  if (!Baum.Wurzel) {

    /* 1. Ist ErstesBlatt gleich NULL? */

    if (Baum.ErstesBlatt) {
      printf("\nFehler: Leerer Baum mit Eintrag fuer Erstes Blatt.\n");
      return FALSE;
    }

    /* 2. Ist maxTiefeLinks gleich Null? */

    if (Baum.maxTiefeLinks) {
      printf("\nFehler: Leerer Baum mit Eintrag fuer maxTiefeLinks.\n");
      return FALSE;
    }

    /* 3. Sind alle Feldeintraege gleich NULL? */

    for (i=1; i<=Baum.TiefeBlaetterDimension; i++)
      if (Baum.TiefeBlaetter[i]) {
        printf("\nFehler: Leerer Baum mit Eintrag in Feld TiefeBlaetter an Position %d.\n",i);
        return FALSE;
      }
    return TRUE;
  }


  /* II. Durchlauf durch Feldeintraege 1 bis TiefeBlaetterDimension */
  /* -------------------------------------------------------------- */

  /* 4. Hat nichtleerer Baum Eintrag fuer ErstesBlatt? */

  if (!Baum.ErstesBlatt) {
    printf("\nFehler: Nichtleerer Baum ohne Eintrag fuer Erstes Blatt.\n");
    return FALSE;
  }

  /* 5. Ist maxTiefeLinks nicht groesser als TiefeBlaetterDimension? */

  if (Baum.maxTiefeLinks>Baum.TiefeBlaetterDimension) {
    printf("\nFehler: Baum mit maxTiefeLinks groesser als TiefeBlaetterDimension.\n");
    return FALSE;
  }

  /* 6. Existiert ueberhaupt ein indiziertes Blatt? */

  BlattIndex = NULL;
  for (i=1; i<=Baum.TiefeBlaetterDimension; i++)
    if ((BlattIndex = Baum.TiefeBlaetter[i]))
      break;
  if (!BlattIndex) {
    printf("\nFehler: Baum ohne ueber TiefeBlaetter indizierte Blaetter.\n");
    return FALSE;
  }

  /* 7. Ist das erste so indizierte Blatt mit ErstesBlatt identisch? */

  if (BlattIndex!=ErstesBlatt) {
    printf("\nFehler: Baum.ErstesBlatt ist nicht das erste indizierte Blatt.\n");
    return FALSE;
  }

  for (i=1; i<=Baum.maxTiefeLinks; i++)
    if ((BlattIndex = Baum.TiefeBlaetter[i])) {

      /* 8. Hat die zum Blatt gehoerende linke Seite die geforderte Tiefe? */

      if (TO_Termtiefe(BK_linkeSeite(BlattIndex))!=i) {
        printf("\nFehler: Baum.TiefeBlaetter[%d] weist in Term der Tiefe %d.",i,TO_Termtiefe(BK_linkeSeite(BlattIndex)));
        return FALSE;
      }

      /* 9. Laesst sich das Blatt ueber die Wurzel finden? */

      if (!BlattAuffindbar(BlattIndex,Baum)) {
        printf("\nFehler: Baum.TiefeBlaetter[%d] weist auf nicht ueber Wurzel auffindbares Blatt.",i);
        return FALSE;
      }
    }

  /* 10. Gibt es Eintraege nach maxTiefeLinks? */

  for (i=Baum.maxTiefeLinks+1; i<=Baum.TiefeBlaetterDimension; i++)
    if (Baum.TiefeBlaetter[i]) {
      printf("\nFehler: Verweis in Baum.TiefeBlaetter[%d] bei maxTiefeLinks = %d.",i,Baum.maxTiefeLinks);
      return FALSE;
    }



  /* III. Durchlaufen der Blaetterverzeigerung */
  /* ----------------------------------------- */

  BlattIndex = Baum.ErstesBlatt;
  BlattIndexVorg = NULL;

  do {

    /* 11. Sind die Blaetter korrekt untereinander verzeigert? (Vorg-Nachf-Entsprechung) */

    if (BlattIndexVorg!=BK_VorgBlatt(BlattIndex)) {
      printf("\nFehler: Blaetter nicht korrekt untereinander verzeigert.\n");
      return FALSE;
    }

    /* 12. Laesst sich jedes Blatt auch ueber die Wurzel finden? */

    if (!BlattAuffindbar(BlattIndex,Baum)) {
      printf("\nFehler: In der Blaetterverzeigerung befindet sich ein nicht ueber die Wurzel auffindbares Blatt.");
      return FALSE;
    }

    /* 13. Sind die Tiefen monoton steigend? */

    aktuelleTiefe = TO_Termtiefe(BK_linkeSeite(BlattIndex));
    if (aktuelleTiefe<bisherigeTiefe) {
      printf("\nFehler: Die Blaetterverzeigerung ist nicht monoton steigend tiefensortiert.");
      return FALSE;
    }

    if (aktuelleTiefe>bisherigeTiefe) {

      /* 14. Ist ein Blatt mit derselben Tiefe im Feld TiefeBlaetter vermerkt? */

      if (!Baum.TiefeBlaetter[bisherigeTiefe = aktuelleTiefe]) {
        printf("\nFehler: In Blaetterverzeigerung Blatt mit Tiefe %d, aber nicht aus TiefeBlaetter indiziert.",aktuelleTiefe);
        return FALSE;
      }

      /* 15. Ist das je erste Blatt mit groesserer Tiefe auch im Feld TiefeBlaetter vermerkt? */

      if (Baum.TiefeBlaetter[aktuelleTiefe]!=BlattIndex) {
        printf("\nFehler: In Blaetterverzeigerung anderes erstes Blatt mit Tiefe %d als aus TiefeBlaetter indiziert.",aktuelleTiefe);
        return FALSE;
      }
    }

  } while ((BlattIndex = BK_NachfBlatt(BlattIndexVorg = BlattIndex)));

  return TRUE;
}



/*=================================================================================================================*/
/*= IV. Testaufruf                                                                                                =*/
/*=================================================================================================================*/

BOOLEAN BT_IndexTesten(DSBaumT Baum, BT_TPPraedikatsT TPPraedikat, BOOLEAN MitSprungverweisen)
{
  if (!AlleKnotenTesten(Baum,TPPraedikat)) {
    printf("\nFehler gefunden in AlleKnotenTesten.\n");
    return FALSE;
  }
  if (!TiefeBlaetterTesten(Baum)) {
    printf("\nFehler gefunden in TiefeBlaetterTesten.\n");
    return FALSE;
  }
  if (MitSprungverweisen && !SprungverweiseTesten(Baum)) {
    printf("\nFehler gefunden in SprungverweiseTesten.\n");
    return FALSE;
  }
  return TRUE;
}



/*=================================================================================================================*/
/*= V. Initialisierung                                                                                            =*/
/*=================================================================================================================*/

void BT_InitApriori(void)
{
  if (WM_ErsterAnlauf()) {
    Stapel = posarray_alloc(StapelDimension = BT_ErsteStapelhoehe,SymbolT,1);
    StapelZeiger = 0;
  }
}



/*=================================================================================================================*/
/*= VI. Leere Funktionen                                                                                          =*/
/*=================================================================================================================*/

#else

BOOLEAN BT_IndexTesten(DSBaumT Baum MAYBE_UNUSEDPARAM, BT_TPPraedikatsT TPPraedikat MAYBE_UNUSEDPARAM, 
  BOOLEAN MitSprungverweisen MAYBE_UNUSEDPARAM) {return TRUE;}

void BT_InitApriori(void) {;}

#endif
