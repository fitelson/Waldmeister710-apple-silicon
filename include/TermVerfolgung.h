/*=================================================================================================================
  =                                                                                                               =
  =  TermVerfolgung.h                                                                                             =
  =                                                                                                               =
  =  Aufspueren verlorener oder ueberschriebener Termzellen                                                       =
  =                                                                                                               =
  =  04.09.95 Thomas.                                                                                             =
  =                                                                                                               =
  =================================================================================================================*/

#ifndef TERMVERFOLGUNG_H
#define TERMVERFOLGUNG_H


/*=================================================================================================================*/
/*= 0. Bedingte Kompilaterzeugung                                                                                 =*/
/*=================================================================================================================*/

#define TV_TEST 0

/* Alternativen:
   0 -- nur leere Funktionen im Kompilat
   1 -- Ausgabefunktionen zur Verfuegung stellen
*/


/*=================================================================================================================*/
/*= VI. Anzeigen lokaler Terme                                                                                    =*/
/*=================================================================================================================*/

void TV_LokaleTermeAnVerfolgerMelden(unsigned int Anzahl, ...);
/* Achtung: Die lokalen Variablen werden ueber ihre Adresse uebergeben (->Brezel), duerfen also  Inhalt aendern. */

void TV_LokaleTermeAbmelden(unsigned int Anzahl);              /* Komplementaer zu TV_LokaleTermeAnVerfolgerMelden */



/*=================================================================================================================*/
/*= VII. Testfunktion                                                                                             =*/
/*=================================================================================================================*/

void TV_TermzellenVerfolger(unsigned int Grad);
/* Grad von 0 bis 3.
   0 - nichts tun.
   1 - Durchlauf der Regelmenge, der Gleichungsmenge, der KP-Menge, der TO_Variablenterme
       dabei Anzahl der verwendeten Termzellen bestimmen
       schlussendlich Vergleich mit Wert im Freispeichermanager der Termoperationen
       gegebenenfalls Fehlermeldung
   2 - Zusaetzlich ueberpruefen, dass keine Zelle mehrfach verwendet wird.
       Durchlauf der Regelmenge, der Gleichungsmenge, der KP-Menge, der TO_Variablenterme
       dabei Anzahl der verwendeten Termzellen bestimmen und jede erreichte Termzelle markieren
       Es wird vorausgesetzt, dass die Zelleninhalte sinnvoll sind.
       Fehler erkannt, falls bereits markierte Termzelle wieder erreicht wird
         dann nochmaliger Durchlauf, Feststellen des ersten Auftretens und Fehlermeldung
       ansonsten erkannte Anzahl mit Wert im Freispeichermanager der Termoperationen vergleichen
       gegebenenfalls Fehlermeldung
       andernfalls Markierungen aufheben
   3 - Zusaetzlich ueberpruefen, dass die Zelleninhalte sinnvoll sind.
*/



/*=================================================================================================================*/
/*= VIII. Statistik                                                                                               =*/
/*=================================================================================================================*/

unsigned int TV_AusdehnungKopierstapel(void);

#endif
