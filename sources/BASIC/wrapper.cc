/*
  Christian Schmidt

  Diese C++ -Datei bentuzt die Omega-Lib 
  und bietet extern "C" Deklarationen (siehe wrapper.h).

  Die angebotene Schnittstelle ermoeglicht es
  arithmetische Constraints zu erstellen und auf Erfuellbarkeit zu testen.
*/

#include "wrapper.h"
#include "omega.h"
#include <stdio.h>

/* Zum Benutzen der Lib durch andere Programme, (z.B. Waldmeister)
   muss hier die Main gewrappt werden. */
extern "C" {

    /* Leere Structs als Rueckgabewerte,
       damit nicht mit void * hantiert wird. */
    struct RelationS {};
    struct VarIdS {};
    struct F_AndS {};
    struct F_OrS {};
    struct HandleS {};

    int C_main(int argc, char* argv[]);
    int main(int argc, char* argv[]) {
	return C_main(argc, argv);
    }


/* ---- Schnittstellenfunktionen -------------------------------------------- */

    /* Neue (leere) Relation erzeugen
       - 0.Arg: int, Anzahl der Variablen
       Rueckgabe: Relation_p, Pointer auf die erzeugte Relation */
    Relation_p OM_create_Relation(int i) {
	Relation* rel = new Relation(i);
	return (Relation_p) rel;
    }

    /* Relation loeschen
       - 0.Arg: Relation */
    void OM_delete_Relation(Relation_p Rel) {
        delete ((Relation*) Rel); /* Destruktor und Freigabe */
    }

    /* Variablenname Setzen
       - 0.Arg: Relation
       - 1.Arg: int, Nummer der Variablen (die Variablen sind ab 1 durchnumeriert)
       - 2.Arg: String, Name der Variable */
    void OM_set_Varname(Relation_p Rel, int id, const char * name) {
	((Relation*) Rel)->name_set_var(id, name);
    }

    /* Variablen ID holen
       - 0.Arg: Relation
       - 1.Arg: int Id der Variablen
       Rueckgabe: Variable_ID */
    Variable_ID_p OM_get_Var_ID(Relation_p Rel, int i) {
	Variable_ID *varid_p = new Variable_ID(((Relation*) Rel)->set_var(i));
	return (Variable_ID_p) varid_p;
    }

    /* Und-Wurzel anlegen (AND)
       - 0.Arg: Relation
       Rueckgabe: F_And (Pointer auf leere Und-Wurzel) */
    F_And_p OM_create_AND_root(Relation_p Rel) {
	F_And *Rel_root = ((Relation*) Rel)->add_and();
	return (F_And_p) Rel_root;
    }

    /* Und-Kind-Knoten anlegen (AND)
       - 0.Arg: parent (ein Oder-Knoten)
       Rueckgabe: F_And (Pointer auf leeren Und-Kind-Knoten) */
    F_And_p OM_create_AND_child(F_Or_p parent) {
	F_And *child = ((F_Or*)parent)->add_and();
	return (F_And_p) child;
    }

    /* Oder-Kind-Knoten anlegen (OR)
       - 0.Arg: parent (ein Und-Knoten)
       Rueckgabe: F_Or (Pointer auf leeren Oder-Kind-Knoten) */
    F_Or_p OM_create_OR_child(F_And_p parent) {
	F_Or *child = ((F_And*)parent)->add_or();
	return (F_Or_p) child;
    }

    /* Ungleichung (GEQ) hinzufuegen
       - 0.Arg: Wurzel
       Rueckgabe: Handle (Pointer auf Ungleichung) */
    Handle_p OM_add_geq(F_And_p root) {
	GEQ_Handle *handle_p = new GEQ_Handle (((F_And*) root)->add_GEQ());
	return (Handle_p) handle_p;
    }

    /* Gleichung (EQ) hinzufuegen
       - 0.Arg: Wurzel
       Rueckgabe: Handle (Pointer auf Ungleichung) */
    Handle_p OM_add_eq(F_And_p root) {
	EQ_Handle *handle_p = new EQ_Handle (((F_And*) root)->add_EQ());
	return (Handle_p) handle_p;
    }

    /* Koeffizienten einer Variablen veraendern
       - 0.Arg: Handle
       - 1.Arg: Variable_ID
       - 2.Arg: int, Summand der zum bestehenden(!) Koeffizienten addiert wird.
       Achtung! Initial sind alle Koeffizienten 0. */
    void OM_update_Coeff(Handle_p handle, Variable_ID_p varid, int summand) {
	((Constraint_Handle*) handle)->update_coef((*(Variable_ID*) varid), summand);
    }

    /* Konstantes Glied veraendern
       - 0.Arg: Handle
       - 1.Arg: int, Summand der zum bestehenden(!) konstanten Glied addiert wird.
       Achtung! Initial ist das konstante Glied 0. */
    void OM_update_Const(Handle_p handle, int summand) {
	((Constraint_Handle*) handle)->update_const(summand);
    }
   
    /* Test auf Erfuellbarkeit
       - 0.Arg: Relation
       Rueckgabe: TRUE(1) oder FALSE(0) */
    int OM_is_Satisfiable(Relation_p Rel) {
	bool result = ((Relation*) Rel)->is_satisfiable();
	return result ? 1 : 0;
    }

    /* Ausgabe der Relation zum Testen
       - 0.Arg: Relation */
    void OM_print_Relation(Relation_p Rel) {
        ((Relation *) Rel)->print_with_subs(stdout); /* geht das so? */
    }

}
