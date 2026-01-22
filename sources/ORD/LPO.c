/*=================================================================================================================
  =                                                                                                               =
  =  LPO.c                                                                                                        =
  =                                                                                                               =
  =  Realisation von Test- und Entscheidungsfunktion fuer LPO                                                     =
  =                                                                                                               =
  =                                                                                                               =
  =  11.03.94 Thomas                                                                                              =
  =                                                                                                               =
  =================================================================================================================*/




#include "LPOVortests.h"
#include "LPO.h"
#include "Praezedenz.h"

/*=================================================================================================================*/
/*= I. Funktionsprototyp fuer verschraenkte Rekursion                                                             =*/
/*=================================================================================================================*/

BOOLEAN LPOGroesserOhneVortest(UTermT Term1, UTermT Term2);



/*=================================================================================================================*/
/*= II. Allquantifizierungen                                                                                      =*/
/*=================================================================================================================*/

static BOOLEAN LPOGroesserAlle(UTermT Term1, UTermT Termliste, UTermT NULListe)     /* Dominiert Term1 alle bis zur Marke */
{                                                                   /* NULListe hintereinandergeschriebenen Terme? */
  /*printf("LPOGroesserAlle ");
  if (TO_PhysischUngleich(Termliste,NULListe)) IO_DruckeFlex("%t %t\n",Term1,Termliste);*/
  while TO_PhysischUngleich(Termliste,NULListe) {
    if (!LP_LPOGroesser(Term1,Termliste))
      return FALSE;
    Termliste = TO_NaechsterTeilterm(Termliste);
  }
  return TRUE;
}

static BOOLEAN LPOGroesserAlleDiff(UTermT Term1, UTermT Termliste, UTermT NULListe, UTermT *Diff)  /* wie LPOGroesserAlle */
{    /* jedoch wird, wenn s=Term1 und t(1)..t(n)=Termliste, s>t1, ..., s>t(i-1), s!>t(i), in *Diff t(i) retourniert*/
  /*printf("LPOGroesserAlleDiff ");
  if (TO_PhysischUngleich(Termliste,NULListe)) IO_DruckeFlex("%t %t\n",Term1,Termliste);*/
  while TO_PhysischUngleich(Termliste,NULListe) {
    if (!LP_LPOGroesser(Term1,Termliste)) {
      *Diff = Termliste; return FALSE;
    }
    Termliste = TO_NaechsterTeilterm(Termliste);
  }
  return TRUE;
}



/*=================================================================================================================*/
/*= III. Existenzquantifizierung                                                                                  =*/
/*=================================================================================================================*/

static BOOLEAN LPOGroesserEx(UTermT Termliste, UTermT NULListe, UTermT Term2)     /* Existiert ein Term in der Termliste, */
{                                                                                          /* der Term2 dominiert? */
  /*printf("LPOGroesserEx ");
  if (TO_PhysischUngleich(Termliste,NULListe)) IO_DruckeFlex("%t %t\n",Termliste,Term2);*/
  while (TO_PhysischUngleich(Termliste,NULListe)) {
    if (LP_LPOGroesser(Termliste,Term2))
      return TRUE;
    Termliste = TO_NaechsterTeilterm(Termliste);
  }
  return FALSE;
}



/*=================================================================================================================*/
/*= IV. Lexikographische Erweiterungen                                                                            =*/
/*=================================================================================================================*/

static BOOLEAN LPOGroesserLex(UTermT Term1, UTermT Term2, UTermT *Diff)     /* Erweiterung der Testfunktion. Term1, Term2
 haben dasselbe Topsymbol. *Diff dient zur Rueckgabe von Differenzstellen, und zwar bei TRUE: Diff erster Teilterm
     von Term2, der kleiner war; FALSE: Diff erster Teilterm von Term1, der nicht gleich (und nicht groesser) war. */
{
  VergleichsT Vortest;
  UTermT Term1Alt, Term2Alt;

  Term1 = TO_ErsterTeilterm(Term1); Term2 = TO_ErsterTeilterm(Term2);
  do {
    /*IO_DruckeFlex("LPOGroesserLex %t %t\n",Term1,Term2);*/
    Term1Alt = Term1; Term2Alt = Term2;
    /*printf("Vortest ist %d. ",LV_VortestLPOGroesserGleich(&Term1,&Term2,&Vortest));*/
    if (LV_VortestLPOGroesserGleich(&Term1,&Term2,&Vortest)) {        /* behandelt Gleichheit und triviales Groesser */
      if (Vortest==Groesser) {
        *Diff = Term2; return TRUE;
      }
      else if (Vortest==Unvergleichbar) {
        *Diff = Term1; return FALSE;
      }
      else {
        Term1 = TO_NaechsterTeilterm(Term1Alt);
        Term2 = TO_NaechsterTeilterm(Term2Alt);
      }
    }
    else if (LPOGroesserOhneVortest(Term1,Term2)) {             /* anstelle LPOGroesser aufrufen, da schon eigener */
      *Diff = Term2; return TRUE;                                                            /* Vortest aufgerufen */
    }
    else {
      *Diff = Term1; return FALSE;
    }
  } while (TRUE);                                               /* Schleife wird, da Term1<>Term2, immer abbrechen */
}


static VergleichsT LPOVglLex(UTermT Term1, UTermT Term2, UTermT *Diff1, UTermT *Diff2) /* gibt Wert des ersten von Gleich
   verschiedenen Teiltermvergleichs (mit der Entscheidungsfunktion durchgefuehrt) zurueck. Ist definiert, da wegen
   Vortest Term1!=Term2. Groesser: Diff2 Differenzstelle in Term2; Kleiner: Diff1 Differenzstelle in Term1, Unver- */
{                                                                             /* gleichbar: Diff1 und Diff2 belegt */
  Term1 = TO_ErsterTeilterm(Term1); Term2 = TO_ErsterTeilterm(Term2);
  /*IO_DruckeFlex("LPOVglLex %t %t\n",Term1,Term2);*/
  do {
    switch (LP_LPO(Term1,Term2)) {
    case Groesser:
      *Diff2 = Term2; return Groesser;
    case Kleiner:
      *Diff1 = Term1; return Kleiner;
    case Unvergleichbar:
      *Diff1 = Term1; *Diff2 = Term2; return Unvergleichbar;
    case Gleich:
      Term1 = TO_NaechsterTeilterm(Term1);
      Term2 = TO_NaechsterTeilterm(Term2);
    }
  } while (TRUE);                                               /* Schleife wird, da Term1<>Term2, immer abbrechen */
}


/*=================================================================================================================*/
/*= V. Testfunktionen                                                                                             =*/
/*=================================================================================================================*/

BOOLEAN LP_LPOGroesser(UTermT Term1, UTermT Term2)
{
  BOOLEAN Vortest;
  UTermT Diff;

  /*IO_DruckeFlex("LPOGroesser %t %t\n",Term1,Term2);*/
  if (LV_VortestLPOGroesser(&Term1,&Term2,&Vortest))                                   /* Trivialitaeten abhandeln */
    return Vortest;

  /* Sonst: Die Terme sind keine Variablen, keine Konstanten, verschieden und nicht Teilterme voneinander; gleiche
        einstellige Topsymbole sind abgeschaelt; ausserdem schliesst ihr Variablenmengenverhaeltnis eine Groesser-
                                                                                              Beziehung nicht aus. */

  switch (PR_Praezedenz(TO_TopSymbol(Term1),TO_TopSymbol(Term2))) {             /* nach Topsymbol unterscheiden */
  case Groesser:
    return LPOGroesserAlle(Term1,TO_ErsterTeilterm(Term2),TO_NULLTerm(Term2));
  case Gleich:
    if (LPOGroesserLex(Term1,Term2,&Diff))
      return LPOGroesserAlle(Term1,TO_NaechsterTeilterm(Diff),TO_NULLTerm(Term2));
    else
      return LPOGroesserEx(TO_NaechsterTeilterm(Diff),TO_NULLTerm(Term1),Term2);
  case Kleiner:
    if (/*FALSE &&*/ LPOGroesserAlle(Term2,TO_ErsterTeilterm(Term1),TO_NULLTerm(Term1)))                          /* fakultativ */
      return FALSE;
    else
      return LPOGroesserEx(TO_ErsterTeilterm(Term1),TO_NULLTerm(Term1),Term2);
  case Unvergleichbar:
    return LPOGroesserEx(TO_ErsterTeilterm(Term1),TO_NULLTerm(Term1),Term2);
  }
  return FALSE;                   /* Dummy-Statement gegen ueberfluessige Compiler-Warnungen; wird nie ausgefuehrt */
}


BOOLEAN LPOGroesserOhneVortest(UTermT Term1, UTermT Term2)   /* analog LPOGroesser, jedoch fuer LPOGroesserLex mit */
{                                                                           /* eigenem Vortest auf Groesser-Gleich */
  UTermT Diff;

  /*IO_DruckeFlex("LPOGroesserOhneVortest %t %t\n",Term1,Term2);*/
  switch (PR_Praezedenz(TO_TopSymbol(Term1),TO_TopSymbol(Term2))) {
  case Groesser:
    return LPOGroesserAlle(Term1,TO_ErsterTeilterm(Term2),TO_NULLTerm(Term2));
  case Gleich:
    if (LPOGroesserLex(Term1,Term2,&Diff))
      return LPOGroesserAlle(Term1,TO_NaechsterTeilterm(Diff),TO_NULLTerm(Term2));
    else
      return LPOGroesserEx(TO_NaechsterTeilterm(Diff),TO_NULLTerm(Term1),Term2);
  case Kleiner:
    if (/*FALSE &&*/ LPOGroesserAlle(Term2,TO_ErsterTeilterm(Term1),TO_NULLTerm(Term1)))
      return FALSE;
    else
      return LPOGroesserEx(TO_ErsterTeilterm(Term1),TO_NULLTerm(Term1),Term2);
  case Unvergleichbar:
    return LPOGroesserEx(TO_ErsterTeilterm(Term1),TO_NULLTerm(Term1),Term2);
  }
  return FALSE;                   /* Dummy-Statement gegen ueberfluessige Compiler-Warnungen; wird nie ausgefuehrt */
}



/*=================================================================================================================*/
/*= VI. Entscheidungsfunktion                                                                                     =*/
/*=================================================================================================================*/

VergleichsT LP_LPO(UTermT Term1, UTermT Term2)
{
  VergleichsT Vortest;
  UTermT Diff, Diff1, Diff2;

  /*IO_DruckeFlex("LPO %t %t\n",Term1,Term2);*/
  if (LV_VortestLPO(&Term1,&Term2,&Vortest))                                           /* Trivialitaeten abhandeln */
    return Vortest;

                  
  switch (Vortest) {  /* Die Terme sind keine Variablen, keine Konstanten,verschieden, nicht Teilterm voneinander.
      Als naechstes Variablenmengenvergleich auswerten, ist hier hoechstens >, <, =, da # im Vortest TRUE liefert. */

  case Kleiner:                                          /* Variablenmengenverhaeltnis praejudiziert >-Verhaeltnis */
    return LP_LPOGroesser(Term2,Term1) ? Kleiner : Unvergleichbar;                   /* mit Testfunktion abpruefen */

  case Groesser:                                                                                         /* analog */
    return LP_LPOGroesser(Term1,Term2) ? Groesser : Unvergleichbar;

  case Gleich:                                                     /* sonst in Vergleich der Topsymbole einsteigen */
    switch (PR_Praezedenz(TO_TopSymbol(Term1),TO_TopSymbol(Term2))) {

    case Groesser:                                       /* und alle Klauseln durchnudeln (vgl. LPO-Dokumentation) */
      if (LPOGroesserAlleDiff(Term1,TO_ErsterTeilterm(Term2),TO_NULLTerm(Term2),&Diff))
        return Groesser;
      else if (LPOGroesserEx(Diff,TO_NULLTerm(Term2),Term1))
        return Kleiner;
      else
        return Unvergleichbar;

    case Kleiner:
      if (LPOGroesserAlleDiff(Term2,TO_ErsterTeilterm(Term1),TO_NULLTerm(Term1),&Diff))
        return Kleiner;
      else if (LPOGroesserEx(Diff,TO_NULLTerm(Term1),Term2))
        return Groesser;
      else
        return Unvergleichbar;

    case Unvergleichbar:
      if (LPOGroesserEx(TO_ErsterTeilterm(Term1),TO_NULLTerm(Term1),Term2))
        return Groesser;
      else if (LPOGroesserEx(TO_ErsterTeilterm(Term2),TO_NULLTerm(Term2),Term1))
        return Kleiner;
      else
        return Unvergleichbar;

    case Gleich:
      switch (LPOVglLex(Term1,Term2,&Diff1,&Diff2)) {
      case Groesser:
        if (LPOGroesserAlleDiff(Term1,TO_NaechsterTeilterm(Diff2),TO_NULLTerm(Term2),&Diff))
          return Groesser;
        else if (LPOGroesserEx(Diff,TO_NULLTerm(Term2),Term1))
          return Kleiner;
        else
          return Unvergleichbar;
      case Kleiner:
        if (LPOGroesserAlleDiff(Term2,TO_NaechsterTeilterm(Diff1),TO_NULLTerm(Term1),&Diff))
          return Kleiner;
        else if (LPOGroesserEx(Diff,TO_NULLTerm(Term1),Term2))
          return Groesser;
        else
          return Unvergleichbar;
      case Unvergleichbar:
        if (LPOGroesserEx(TO_NaechsterTeilterm(Diff1),TO_NULLTerm(Term1),Term2))
          return Groesser;
        else if (LPOGroesserEx(TO_NaechsterTeilterm(Diff2),TO_NULLTerm(Term2),Term1))
          return Kleiner;
        else
          return Unvergleichbar;
      case Gleich:                                                                 /* nur gegen Compiler-Warnungen */
        break;
      }
    }

  case Unvergleichbar:                                                             /* nur gegen Compiler-Warnungen */
    break;
  }
  return Gleich;                  /* Dummy-Statement gegen ueberfluessige Compiler-Warnungen; wird nie ausgefuehrt */
}
