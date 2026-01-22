/* =================================================*/
/* ==                                             ==*/
/* ==                                             ==*/
/* ==    Konstanten fuer Speicherverwaltung       ==*/
/* ==                                             ==*/
/* ==                                             ==*/
/* =================================================*/

#ifndef SPEICHERPARAMETER_H
#define SPEICHERPARAMETER_H


/* Fuer alle Module, die die Speicherverwaltung verwenden,
   sind in dieser Datei die Element- und Block-Groessen
   abzulegen.
   Nach Inkludieren von SpeicherVerwaltung.h sind diese
   Daten automatisch sichtbar. */


/*--------- Uebergeordnete Groessen------------------------*/

#define SPP_AnzErsterVariablen 4


/*--------- KP-Verwaltung ---------------------------------*/

#define PQ_AnzahlInfoBloeckeJeBlock 1000
#define PQ_AnzahlKPZellenJeBlock 1000


/*--------- TermOperationen -------------------------------*/

#define TO_AnzahlTermZellenJeBlock 30000 /* statt 4*65500 */
/* #define TO_AnzahlTermZellenJeBlockBeiGepackterDarstellung 65500*/
#define TO_AnzahlBaumTermZellenJeBlock 16000
#define TO_AnzErsterVariablenterme SPP_AnzErsterVariablen
#define TO_AnzVLZellenJeBlock 52


/*--------- TermIteration ---------------------------------*/

#define TI_ErsteKopierstapelhoehe 10


/*--------- KBO -------------------------------------------*/

#define KB_AnzErsterVariablen SPP_AnzErsterVariablen


/*--------- LPOVortests -----------------------------------*/

#define LV_AnzListenzellenJeBlock 32


/*--------- VariablenMengen -------------------------------*/

#define VM_AnzListenzellenJeBlock BK_AnzInnereKnotenJeBlock+20


/*--------- DSBaumKnoten ----------------------------------*/

#define BK_VariablenZahlFuerAnzulegendeKnoten 16
#define BK_AnzahlNachfolgerArraysJeBlock 80
#define BK_AnzInnereKnotenJeBlock 100
#define BK_AnzBlaetterJeBlock 1000
#define BK_AnzSprungeintraegeJeBlock 1000
#define BK_AnzPraefixeintraegeJeBlock 500
#define BK_AnzahlErsterTieferBlaetter 50


/*--------- DSBaumOperationen -----------------------------*/

#define BO_AnzErsterVariablen SPP_AnzErsterVariablen
#define BO_ErsteSprungstapelhoehe 100
  /* proportional zur maximalen Termtiefe */


/*--------- Termpaare -------------------------------------*/

#define TP_AnzRegelnJeBlock 1000


/*--------- DSBaumAusgaben --------------------------------*/

#define BA_AnzErsterZeilen 100
  /* fuer Ausgabe-Koordinatensystem */
#define BA_maxAusgabelaengeVariable 12
#define BA_maxAusgabelaengeAdressenassoziation 12
#define BA_maxAusgabelaengeRegelnr 12



/*--------- DSBaumTest ------------------------------------*/

#define BT_ErsteStapelhoehe 32


/*--------- MatchOperationen ------------------------------*/

#define MO_AnzErsterVariablen SPP_AnzErsterVariablen


/*--------- Unifikation1 ----------------------------------*/

#define U1_AnzErsterVariablen (2*SPP_AnzErsterVariablen)


/*--------- Speicherverwaltung ----------------------------*/

#define SV_maxLaengeNameNestedManager 52


/*--------- SymbolOperationen -----------------------------*/

#define SO_AnzErsterVariablen SPP_AnzErsterVariablen


/*--------- RUndEVerwaltung -------------------------------*/

#define RE_ErstePermArgDimension BO_AnzErsterVariablen


/*--------- MNF -------------------------------------------*/

#define MNF_ColTermBlock 100
#define MNF_MengeSpeicherBlock 5


/*--------- ZielVerwaltung --------------------------------*/

#define Z_SpeicherBlock 2


/*--------- Hash ------------------------------------------*/

#define HA_SpeicherBlock 200


/*--------- Deque -----------------------------------------*/

#define DQ_SpeicherBlock 50


/*-------- LispListen -------------------------------------*/

#define LL_Blockgroesse 1000


/*-------- PhilMarlow -------------------------------------*/

#define PM_AnzKomprimateJeBlock 8
#define PM_ErsteOptionenlaenge 45 /* warum nicht? */


/*--------- Unifikation2 ----------------------------------*/

#define U2_AnzErsterVariablen SPP_AnzErsterVariablen

#endif
