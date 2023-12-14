#ifndef PARSER_H_
#define PARSER_H_

#include <iostream>
#include <string>
#include <list>
#include <algorithm>

#include "id_table.h"
#include "symbol.h"
#include "scanner.h"

using namespace std;

class parser {
public:

    parser(scanner* s, id_table* t, error_handler* e);
    ~ parser();
    void PROG(); 

private:

    bool debug {false};

    scanner* scan; // Copy of scanner
    id_table* table;
    error_handler* error;

    // Functions
    void BLOCK(); 
    void DECLERATION(); 
    void STATEMENT_LIST();
    void STATEMENT();
    void SIMPLE_STATEMENT();
    void COMPOUND_STATEMENT();
    void IF_STATEMENT();
    void LOOP_STATEMENT();
    void FOR_STATEMENT();
    void WHILE_STATEMENT();

    // Boolean Functions
    bool IS_EXPR();
    bool IS_BOOL();
    bool IS_RELOP();
    bool IS_MULTOP();
    bool IS_ADDOP();
    bool IS_PRIMARY();
    bool IS_NUMBER();
    bool IS_DECLERATION();
    bool IS_STATEMENT();

    // ID_table stuff
    void define_function(string name, lille_type t, lille_type p);
    lille_type get_ident_type();    
    id_table_entry* current_entry;
    id_table_entry* current_fun_or_proc;
    id_table_entry* current_ident;
    void handle_function_or_procedure_call(id_table_entry* current_entry);
    void handle_if_and_while();
    lille_type get_type();
    list<token*> IDENT_LIST();
    void PARAM();
};

#endif
