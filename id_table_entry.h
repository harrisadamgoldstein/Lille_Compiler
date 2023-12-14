#ifndef ID_TABLE_ENTRY_H_
#define ID_TABLE_ENTRY_H_

#include <iostream>
#include <string>
#include "token.h"
#include "lille_type.h"
#include "lille_kind.h"

using namespace std;

// Class representing an entry in the identifier table
class id_table_entry {
private:
    // Private member variables
    bool debug_mode;           // Debug mode flag
    token* id_entry;           // Token representing the identifier
    int lev_entry;             // Lexical level
    int offset_entry;          // Offset within its lexical level
    lille_kind kind_entry;     // Kind of identifier (variable, function, etc.)
    bool trace_entry;          // Trace flag
    lille_type typ_entry;      // Data type
    int i_val_entry;           // Integer value
    float r_val_entry;         // Real value
    string s_val_entry;        // String value
    bool b_val_entry;          // Boolean value
    id_table_entry* p_list_entry;  // Pointer to the list of parameters
    int n_par_entry;           // Number of parameters
    lille_type r_ty_entry;     // Return type for functions

public:
    // Public member functions

    // Default constructor
    id_table_entry();

    // Parameterized constructor
    id_table_entry(token* id,
                   lille_type typ = lille_type::type_unknown,
                   lille_kind kind = lille_kind::unknown,
                   int level = 0,
                   int offset = 0,
                   lille_type return_tipe = lille_type::type_unknown);

    // Setter for trace flag
    void trace_obj(bool trac);

    // Getter for trace flag
    bool trace();

    // Getter for offset
    int offset();

    // Getter for lexical level
    int level();

    // Getter for kind of identifier
    lille_kind kind();

    // Getter for data type
    lille_type tipe();

    // Getter for the identifier's token
    token* token_value();

    // Getter for the identifier's name
    string name();

    // Getter for integer value
    int integer_value();

    // Getter for real value
    float real_value();

    // Getter for string value
    string string_value();

    // Getter for boolean value
    bool bool_value();

    // Getter for return type (for functions)
    lille_type return_tipe();

    // Method to fix constant values
    void fix_const(int integer_value = 0,
                   float real_value = 0,
                   string string_value = "",
                   bool bool_value = false);

    // Method to fix return type (for functions)
    void fix_return_type(lille_type ret_ty);

    // Method to add a parameter to the list
    void add_param(id_table_entry* param_entry);

    // Method to get the nth parameter
    id_table_entry* nth_parameter(int n);

    // Method to get the number of parameters
    int number_of_params();

    // Method to convert the entry to a string representation
    string to_string();
};

#endif

