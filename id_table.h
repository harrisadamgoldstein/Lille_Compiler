#ifndef ID_TABLE_H_
#define ID_TABLE_H_

#include <iostream>
#include <string>
#include "token.h"
#include "error_handler.h"
#include "id_table.h"
#include "id_table_entry.h"
#include "lille_type.h"
#include "lille_kind.h"

using namespace std;

class id_table {
private:
    error_handler* error;
    bool debug_mode;
    int scope_level;
    // maximum depth of nesting permitted in source code.
    static const int max_depth = 10;
    struct node {
        node* left;
        node* right;
        id_table_entry* idt;
    };
    node* sym_table[max_depth];
    void add_table_entry(id_table_entry* it, node* ptr);
    void dump_tree(node* ptr);

public:
    
    id_table(error_handler* err);
    // Constructor

    void enter_scope();
    // Increments Scope 

    void exit_scope();
    // Decrement Scope

    int scope();
    // Returns Current Scope Value

    id_table_entry* lookup(string s);
    // Searchs Binary Tree for an item
    id_table_entry* lookup(token* tok);
    // Searches a binary tree for a token

    void trace_all(bool b);
    // Traces the binary tree

    bool trace_all();

    void add_table_entry(id_table_entry* id);
    // Adds an item to the id_table

    id_table_entry* enter_id(token* id,
        lille_type typ = lille_type::type_unknown,
        lille_kind kind = lille_kind::unknown,
        int level = 0,
        int offset = 0,
        lille_type return_tipe = lille_type::type_unknown);
    // Generates an id_table_entry item
    
    void dump_id_table(bool dump_all = true);
    // Dumps the id_table
    
};
#endif /* ID_TABLE_H_ */
