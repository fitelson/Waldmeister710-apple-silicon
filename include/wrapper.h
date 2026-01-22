#ifndef WRAPPER_H
#define WRAPPER_H
#ifndef DO_OMEGA
#define DO_OMEGA 0
#endif

#if DO_OMEGA
/* Christian Schmidt 27.6.2003 */

/* Da auch der C-Compiler diese Datei zu sehen bekommt,
   muss man ihn vor extern "C" schuetzen. */
#ifdef __cplusplus
extern "C" {
#endif

    /* Typedefs fuer Rueckgabewerte, 
       damit nicht mit void * hantiert wird. */
    typedef struct RelationS * Relation_p;
    typedef struct VarIdS * Variable_ID_p;
    typedef struct F_AndS * F_And_p;
    typedef struct F_OrS * F_Or_p;
    typedef struct HandleS * Handle_p;
    

/* ---- Schnittstellenfunktionen --- OM_ steht fuer Omega-lib --------------- */

    /* Neue (leere) Relation erzeugen
       - 0.Arg: int, Anzahl der Variablen
       Rueckgabe: Relation_p, Pointer auf die erzeugte Relation */
    Relation_p OM_create_Relation(int i);

    /* Relation loeschen
       - 0.Arg: Relation */
    void OM_delete_Relation(Relation_p Rel);
 
    /* Variablenname Setzen
       - 0.Arg: Relation
       - 1.Arg: int, Nummer der Variablen (die Var sind ab 1 durchnumeriert)
       - 2.Arg: String, Name der Variable */
    void OM_set_Varname(Relation_p Rel, int id, const char * name);
 
    /* Variablen ID holen
       - 0.Arg: Relation
       - 1.Arg: int Id der Variablen
       Rueckgabe: Variable_ID */
    Variable_ID_p OM_get_Var_ID(Relation_p Rel, int i);
 
    /* Und-Wurzel anlegen (AND)
       - 0.Arg: Relation
       Rueckgabe: F_And (Pointer auf leere Und-Wurzel) */
    F_And_p OM_create_AND_root(Relation_p Rel);
 
    /* Und-Kind-Knoten anlegen (AND)
       - 0.Arg: parent (ein Oder-Knoten)
       Rueckgabe: F_And (Pointer auf leeren Und-Kind-Knoten) */
    F_And_p OM_create_AND_child(F_Or_p parent);

    /* Oder-Kind-Knoten anlegen (OR)
       - 0.Arg: parent (ein Und-Knoten)
       Rueckgabe: F_Or (Pointer auf leeren Oder-Kind-Knoten) */
    F_Or_p OM_create_OR_child(F_And_p parent);

    /* Ungleichung (GEQ) hinzufuegen
       - 0.Arg: Knoten
       Rueckgabe: Handle (Pointer auf Ungleichung) */
    Handle_p OM_add_geq(F_And_p node);
 
    /* Gleichung (EQ) hinzufuegen
       - 0.Arg: Knoten
       Rueckgabe: Handle (Pointer auf Ungleichung) */
    Handle_p OM_add_eq(F_And_p node);

    /* Koeffizienten einer Variablen veraendern
       - 0.Arg: Handle
       - 1.Arg: Variable_ID
       - 2.Arg: int, Summand der zum bestehenden(!) Koeffizienten addiert wird.
       Achtung! Initial sind alle Koeffizienten 0. */
    void OM_update_Coeff(Handle_p handle, Variable_ID_p varid, int summand);
 
    /* Konstantes Glied veraendern
       - 0.Arg: Handle
       - 1.Arg: int, Summand der zum bestehenden(!) konstanten Glied addiert wird.
       Achtung! Initial ist das konstante Glied 0. */
    void OM_update_Const(Handle_p handle, int summand);

    /* Test auf Erfuellbarkeit
       - 0.Arg: Relation
       Rueckgabe: TRUE(1) oder FALSE(0) */
    int OM_is_Satisfiable(Relation_p Rel);
 
    /* Ausgabe der Relation zum Testen
       - 0.Arg: Relation */
    void OM_print_Relation(Relation_p Rel);

#ifdef __cplusplus
}
#endif

#endif
#endif
