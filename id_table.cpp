#include <iostream>
#include <string>

#include "token.h"
#include "error_handler.h"
#include "id_table.h"
#include "id_table_entry.h"
#include "lille_kind.h"
#include "lille_type.h"

using namespace std;

// Constructor for the id_table class
id_table::id_table(error_handler* err) {
    // Initialize error handler and other variables
    error = err;
    debug_mode = false;
    scope_level = 0;

    // Initialize symbol table entries
    for (int i = 0; i < max_depth; i++) {
        sym_table[i] = new node;
        sym_table[i]->idt = NULL;
        sym_table[i]->left = NULL;
        sym_table[i]->right = NULL;
    }
}

// Function to dump the id_table
void id_table::dump_id_table(bool dump_all) {
    node* ptr;
    if (!dump_all) {
        if (debug_mode) {
            cout << "Dump of idtable for current scope only." << endl;
            cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
        }

        ptr = sym_table[scope()];
        exit_scope();
        delete ptr;
        ptr = NULL;
    } else {
        if (debug_mode) {
            cout << "Dump of the entire symbol table." << endl;
            cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
        }

        while (scope() > 0) {
            ptr = sym_table[scope()];
            exit_scope();
            delete ptr;
            ptr = NULL;
        }
    }
}

// Function to enter a new scope
void id_table::enter_scope() {
    scope_level++;
}

// Function to exit the current scope
void id_table::exit_scope() {
    scope_level--;
}

// Function to get the current scope level
int id_table::scope() {
    return scope_level;
}

// Function to add a new entry to the symbol table
void id_table::add_table_entry(id_table_entry* id) {
    node* entry = new node;
    entry->idt = id;
    entry->right = NULL;
    entry->left = NULL;

    node* x = sym_table[scope()], * y = NULL;

    // Find the correct position in the binary search tree
    while (x->idt != NULL) {
        y = x;
        if (id->name() < x->idt->name()) {
            x = x->left;
        } else {
            x = x->right;
        }
        if (x == NULL)
            break;
    }

    // Insert the new entry
    if (y == NULL)
        sym_table[scope()] = entry;
    else if (id->name() < y->idt->name())
        y->left = entry;
    else
        y->right = entry;

    if (debug_mode)
        cout << "ADDED ENTRY: Created Entry " << id->name() << " in Scope " << scope() << endl;
}

// Function to create a new id_table_entry
id_table_entry* id_table::enter_id(token* id, lille_type typ, lille_kind kind, int level, int offset, lille_type return_tipe) {
    return new id_table_entry(id, typ, kind, level, offset, return_tipe);
}

// Function to look up an entry in the symbol table by name
id_table_entry* id_table::lookup(string s) {
    int sc = scope();
    node* ptr = sym_table[sc];
    bool found = false;

    while (sc >= 0) {
        if (ptr == NULL or ptr->idt == NULL) {
            if (sc > 0)
                ptr = sym_table[--sc];
            else
                sc--;
        } else if (s < ptr->idt->name()) {
            ptr = ptr->left;
        } else if (s > ptr->idt->name()) {
            ptr = ptr->right;
        } else if (s == ptr->idt->name()) {
            if (debug_mode)
                cout << "FOUND ENTRY: Found entry " << s << " of type " << ptr->idt->tipe().to_string() << endl;
            return ptr->idt;
        }
    }
    if (debug_mode)
        cout << "DID NOT FIND: Failed to find entry " << s << endl;
    return NULL;
}

// Function to look up an entry in the symbol table by token
id_table_entry* id_table::lookup(token* tok) {
    int sc = scope();
    node* ptr = sym_table[sc];
    bool found = false;

    while (sc >= 0) {
        if (ptr == NULL or ptr->idt == NULL) {
            if (sc > 0)
                ptr = sym_table[--sc];
            else
                sc--;
        } else if (tok < ptr->idt->token_value()) {
            ptr = ptr->left;
        } else if (tok > ptr->idt->token_value()) {
            ptr = ptr->right;
        } else if (tok == ptr->idt->token_value()) {
            if (debug_mode)
                cout << "FOUND ENTRY: Found entry " << tok->to_string() << endl;
            return ptr->idt;
        }
    }
    if (debug_mode)
        cout << "DID NOT FIND: Failed to find entry " << tok->to_string() << endl;
    return NULL;
}

