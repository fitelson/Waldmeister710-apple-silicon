#ifndef FILEMENUE_H
#define FILEMENUE_H

char *FM_SpezifikationErkannt(int argc, char *argv[]);
/* String fn zu komplettem Dateinamen machen.
   1. Als Datei suchen
   2. In Beispielverzeichnissen suchen
   Rueckgabewert ist der komplette Pfad oder NULL, falls Suche erfolglos war.
   Die Argument-Position des Filenamens wird - falls existent - auf NULL gesetzt.
*/

#endif
