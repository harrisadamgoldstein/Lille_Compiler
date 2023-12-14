#include <iostream>
#include "id_table.h"
#include "id_table_entry.h"

// Default constructor
id_table_entry::id_table_entry() {
    // Initialize member variables to default values
    debug_mode = false;
    id_entry = NULL;
    lev_entry = 0;
    offset_entry = 0;
    kind_entry = lille_kind::unknown;
    trace_entry = false;
    typ_entry = lille_type::type_unknown;
    i_val_entry = 0;
    r_val_entry = 0.0;
    s_val_entry = "";
    b_val_entry = false;
    p_list_entry = NULL;
    n_par_entry = 0;
    r_ty_entry = lille_type::type_unknown;
}

// Parameterized constructor
id_table_entry::id_table_entry(token* id, lille_type typ, lille_kind kind, int level, int offset, lille_type return_tipe) {
    // Initialize member variables with provided values
    id_entry = id;
    typ_entry = typ;
    kind_entry = kind;
    lev_entry = level;
    offset_entry = offset;
    r_ty_entry = return_tipe;

    // Initialize other member variables to default values
    p_list_entry = NULL;
    n_par_entry = 0;
    lev_entry = 0;
    offset_entry = 0;
    trace_entry = false;
    i_val_entry = 0;
    r_val_entry = 0.0;
    s_val_entry = "";
    b_val_entry = false;
}

// Getter for offset
int id_table_entry::offset() {
    return offset_entry;
}

// Getter for lexical level
int id_table_entry::level() {
    return lev_entry;
}

// Getter for kind of identifier
lille_kind id_table_entry::kind() {
    return kind_entry;
}

// Getter for data type
lille_type id_table_entry::tipe() {
    return typ_entry;
}

// Getter for the identifier's token
token* id_table_entry::token_value() {
    return id_entry;
}

// Getter for the identifier's name
string id_table_entry::name() {
    // Determine the type of token and return the corresponding value
    if(id_entry->get_sym() == symbol::identifier)
        return id_entry->get_identifier_value();
    else if(id_entry->get_sym() == symbol::program_sym)
        return id_entry->get_prog_value();
    else if(id_entry->get_sym() == symbol::procedure_sym)
        return id_entry->get_proc_value();
    else if(id_entry->get_sym() == symbol::function_sym)
        return id_entry->get_fun_value();
    return "";
}

// Getter for integer value
int id_table_entry::integer_value() {
    return i_val_entry;
}

// Getter for real value
float id_table_entry::real_value() {
    return r_val_entry;
}

// Getter for string value
string id_table_entry::string_value() {
    return s_val_entry;
}

// Getter for boolean value
bool id_table_entry::bool_value() {
    return b_val_entry;
}

// Getter for return type (for functions)
lille_type id_table_entry::return_tipe() {
    return r_ty_entry;
}

// Method to add a parameter to the list
void id_table_entry::add_param(id_table_entry* param_entry) {
    bool finished = false;
    id_table_entry* ptr = this;
    while (!finished) {
        if (ptr->p_list_entry == NULL) {
            // If the current parameter list entry is NULL, link the new parameter and finish
            ptr->p_list_entry = param_entry;
            finished = true;
            this->n_par_entry++;
            if (debug_mode)
                cout << "LINKED PARAM: Linked " << param_entry->name() << " to " << this->name() << endl;
        }
        else
            ptr = ptr->p_list_entry;
    }
}

// Method to fix constant values
void id_table_entry::fix_const(int integer_value, float real_value, string string_value, bool bool_value) {
    // Set constant values
    i_val_entry = integer_value;
    r_val_entry = real_value;
    s_val_entry = string_value;
    b_val_entry = bool_value;
}

// Method to fix return type (for functions)
void id_table_entry::fix_return_type(lille_type ret_ty) {
    this->r_ty_entry = ret_ty;
}

// Method to get the nth parameter
id_table_entry* id_table_entry::nth_parameter(int n) {
    id_table_entry* ptr = this;
    for (int i = 0; i < n + 1; i++) {
        if (ptr->p_list_entry != NULL) {
            ptr = ptr->p_list_entry;
        }
    }
    if (ptr == this)
        return NULL;
    return ptr;
}

// Method to get the number of parameters
int id_table_entry::number_of_params() {
    return n_par_entry;
}

// Getter for trace flag
bool id_table_entry::trace() {
    return trace_entry;
}

// Method to convert the entry to a string representation
string id_table_entry::to_string() {
    // Return a string representation of the entry
    return this->name() + ":\n" + "Type: " + this->tipe().to_string() + "\nKind: " + this->kind().to_string() + "\nScope: " + ::to_string(this->level()) + "\nReturn Type: " + this->r_ty_entry.to_string();
}

