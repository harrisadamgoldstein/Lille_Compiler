#include <iostream>
#include <algorithm>
#include <string>  
#include <list>
#include "symbol.h"
#include "scanner.h"
#include "token.h"
#include "parser.h"
#include "lille_type.h"
#include "lille_kind.h"
#include "id_table.h"
#include "id_table_entry.h"

using namespace std;

parser::parser(scanner* s, id_table* t, error_handler* e) {
    scan=s;
    table=t;
    error=e;

    current_entry = NULL;
    current_fun_or_proc = NULL;
    current_ident = NULL;
}

parser::~parser() {
    delete scan;
    scan = NULL;

    delete table;
    table = NULL;

    delete error;
    error = NULL;

    delete current_entry;
    current_entry = NULL;

    delete current_fun_or_proc ;
    current_fun_or_proc = NULL;

    delete current_ident;
    current_ident = NULL;
}

void parser::define_function(string name, lille_type t, lille_type p) {
    // Create variables 
    token* fun, * arg;
    symbol* sym;
    id_table_entry* fun_id, * param_id;
    // Generate the entry
    sym = new symbol(symbol::identifier);
    fun = new token(sym, 0, 0);
    fun->set_identifier_value(name);
    fun_id = table->enter_id(fun, lille_type::type_func, lille_kind::unknown, 0, 0, t);
    table->add_table_entry(fun_id);

    // Generate the Arguments
    arg = new token(sym, 0, 0);
    arg->set_identifier_value("__" + name + "_arg__"); // predefined functions have one arg
                                                       // that is `__NAME__arg_`
    param_id = new id_table_entry(arg, p, lille_kind::value_param, 0, 0, lille_type::type_unknown);
    fun_id->add_param(param_id);
}

void parser::PROG() { // Begin program

    scan->must_be(symbol::program_sym); 

    // Add the program call to the id table
    symbol* sym = new symbol(symbol::program_sym);
    token* prog = new token(sym, 0, 0);
    prog->set_prog_value(scan->get_current_identifier_name());
    id_table_entry* prog_id = table->enter_id(prog, lille_type::type_prog, lille_kind::unknown, table->scope(), 0, lille_type::type_unknown);
    table->add_table_entry(prog_id);
    current_entry = prog_id;

    scan->must_be(symbol::identifier);

    // Define the predifined functions
    parser::define_function("INT2REAL", lille_type::type_real, lille_type::type_integer);
    parser::define_function("REAL2INT", lille_type::type_integer, lille_type::type_real);
    parser::define_function("INT2STRING", lille_type::type_string, lille_type::type_integer);
    parser::define_function("REAL2STRING", lille_type::type_string, lille_type::type_real);

    scan->must_be(symbol::is_sym);

    // Begin parsing through block
    BLOCK();

    scan->must_be(symbol::semicolon_sym);
    table->dump_id_table(true);
}

void parser::BLOCK() {

    // Enter a new scope
    table->enter_scope();

    // Find all variable declerations, as well as
    // all function and procedure definitions
    while (IS_DECLERATION()) 
         DECLERATION();

    scan->must_be(symbol::begin_sym);
    STATEMENT_LIST();
    scan->must_be(symbol::end_sym);
    if (scan->have(symbol::identifier)) {

        scan->must_be(symbol::identifier);
    }
    table->exit_scope();
}

void parser::DECLERATION() { 

    // If declaring identifier ->
    if (scan->have(symbol::identifier)) {

        list<token*> variables = IDENT_LIST();

         scan->must_be(symbol::colon_sym);

        bool const_flag = false;

        // By default, the kind will be variable
        lille_kind knd = lille_kind(lille_kind::variable);

        // If constant is found ->
        if(scan->have(symbol::constant_sym)) {
            // Flip const flag, and make kind constant
            const_flag = true;
            scan->must_be(symbol::constant_sym);
            knd = lille_kind(lille_kind::constant);
        }

        // Get the variable type (Int, Real, String, or Boolean)
        lille_type ty = get_ident_type();
        if (scan->have(symbol::integer_sym)) 
            scan->must_be(symbol::integer_sym);
        else if (scan->have(symbol::real_sym)) 
            scan->must_be(symbol::real_sym);
        else if(scan->have(symbol::string_sym)) 
            scan->must_be(symbol::string_sym);
        else 
            scan->must_be(symbol::boolean_sym);

        // Decalare const variables in the case it is a constant
        float r_value;
        int i_value;
        string s_value;
        bool b_value; 

        // If was const, find the const value ->
        if(const_flag) {
            scan->must_be(symbol::becomes_sym);
            // Find what type the value is ->
            int sym = scan->this_token()->get_symbol()->get_sym();
            switch(sym) {
                case symbol::real_num: {
                    r_value = scan->this_token()->get_real_value();
                    if (!ty.is_type(lille_type::type_real))
                        error->flag(scan->this_token(), 111);
                        // Const expr does not match type declaration.
                    break;
                }
                case symbol::integer: {
                    i_value = scan->this_token()->get_integer_value();
                    if (!ty.is_type(lille_type::type_integer))
                        error->flag(scan->this_token(), 111);
                        // Const expr does not match type declaration.
                    break;
                }
                case symbol::strng: {
                    s_value = scan->this_token()->get_string_value();
                    if (!ty.is_type(lille_type::type_string))
                        error->flag(scan->this_token(), 111);
                        // Const expr does not match type declaration.
                    break;
                }
                case symbol::boolean_sym: {
                    b_value = true ? scan->have(symbol::true_sym) : false;
                    if (!ty.is_type(lille_type::type_boolean))
                        error->flag(scan->this_token(), 111);
                        // Const expr does not match type declaration.
                    break;
                }
            }
            scan->get_token();
        }

        // Loop through the array of tokens ->
        id_table_entry* id;
        for(token* &v : variables) {
            // if the token isnt null ->
            if(v != NULL) {
                // Create the table entry object with respective values 
                id = table->enter_id(v, ty, knd, table->scope(), 0, lille_type::type_unknown);
                // If const, apply the constant value
                if(const_flag) {
                    id->fix_const(i_value, r_value, s_value, b_value);
                }
                // add entry to the id table
                table->add_table_entry(id);
            }
        }
        scan->must_be(symbol::semicolon_sym);
    }

    // Else is a procedure/function decleration ->
    else {
        bool is_func = false;
        // Is a prcedure ->
        if(scan->have(symbol::procedure_sym)) {
            scan->must_be(symbol::procedure_sym);

            // Add the procedure to the id table
            symbol* sym = new symbol(symbol::procedure_sym);
            token* proc = new token(sym, 0, 0);
            proc->set_proc_value(scan->get_current_identifier_name());
            id_table_entry* proc_id = table->enter_id(proc, lille_type::type_proc, lille_kind::unknown, table->scope(), 0, lille_type::type_unknown);
            table->add_table_entry(proc_id);
            current_fun_or_proc = proc_id;

            scan->must_be(symbol::identifier);
        }
        // Is a function ->
        else {
            scan->must_be(symbol::function_sym);

            // Add the function to the id table
            symbol* sym = new symbol(symbol::function_sym);
            token* fun = new token(sym, 0, 0);
            fun->set_fun_value(scan->get_current_identifier_name());
            id_table_entry* fun_id = table->enter_id(fun, lille_type::type_func, lille_kind::unknown, table->scope(), 0, lille_type::type_unknown);
            table->add_table_entry(fun_id);
            current_fun_or_proc = fun_id;

            scan->must_be(symbol::identifier);

            // Trip flag to remember it is a function
            is_func = true;
        }
        
        table->enter_scope();
        PARAM();

        // If is a function ->
        if(is_func) {
            scan->must_be(symbol::return_sym);
            current_fun_or_proc->fix_return_type(get_ident_type());
             if (scan->have(symbol::integer_sym)) 
                scan->must_be(symbol::integer_sym);
            else if (scan->have(symbol::real_sym)) 
                scan->must_be(symbol::real_sym);
            else if(scan->have(symbol::string_sym)) 
                scan->must_be(symbol::string_sym);
            else 
                scan->must_be(symbol::boolean_sym);
        }

        // Continue into the body of the procedure/function
        scan->must_be(symbol::is_sym);
        BLOCK();
        scan->must_be(symbol::semicolon_sym);
        current_entry = NULL;
        current_fun_or_proc = NULL;
        table->exit_scope();
    }
}

void parser::STATEMENT_LIST() {

    STATEMENT();
    scan->must_be(symbol::semicolon_sym);
    while (IS_STATEMENT()) {
        STATEMENT();
        scan->must_be(symbol::semicolon_sym);
    }
    
}

void parser::STATEMENT() {

    if (scan->have(symbol::if_sym) or scan->have(symbol::while_sym) or scan->have(symbol::for_sym) or scan->have(symbol::loop_sym)) 
        COMPOUND_STATEMENT();
    else 
        SIMPLE_STATEMENT();
    
}

void parser::COMPOUND_STATEMENT() {

    if (scan->have(symbol::if_sym)) 
        IF_STATEMENT();
    else if (scan->have(symbol::loop_sym)) 
        LOOP_STATEMENT();
    else if (scan->have(symbol::for_sym)) 
        FOR_STATEMENT();
    else if (scan->have(symbol::while_sym)) 
        WHILE_STATEMENT();
}

void parser::SIMPLE_STATEMENT() {
    if (scan->have(symbol::identifier)) {
        // Lookup the identifier
        string current_entry_name = scan->get_current_identifier_name();
        current_entry = table->lookup(current_entry_name);

        if(current_entry == NULL) {
            error->flag(scan->this_token(), 81);
            throw lille_exception("Identifier not previously declared.");
        }

        else if(current_entry->tipe().is_type(lille_type::type_prog)) {
            error->flag(scan->this_token(), 91);
        }

        scan->must_be(symbol::identifier);
        
        // If a left parantheses is found, handle 
        // function or procedure ->
        if(current_entry->tipe().is_type(lille_type::type_func) or current_entry->tipe().is_type(lille_type::type_proc)) {
            handle_function_or_procedure_call(current_entry);
        }
        // If not a function/procedure call, must be becomes
        else {
            scan->must_be(symbol::becomes_sym);
            /**** HANDLE BECOMES ****/
            bool finished = true;

            if(current_entry == NULL) {
                error->flag(scan->this_token(), 81);
            }
            
            if(current_entry->kind().is_kind(lille_kind::for_ident))
                error->flag(scan->this_token(), 85);

            if(current_entry->kind().is_kind(lille_kind::value_param)) {
                error->flag(scan->this_token(), 85);
            }

            do {
                // If wrapped in paren, eat the symbols
                if(scan->have(symbol::left_paren_sym)) {
                    scan->must_be(symbol::left_paren_sym);
                    finished = false;
                }
                else if(scan->have(symbol::right_paren_sym)) {
                    scan->must_be(symbol::right_paren_sym);
                    finished = false;
                }

                // If a not, check if value is boolean
                else if(scan->have(symbol::not_sym)) {
                    if(not current_entry->tipe().is_type(lille_type::type_boolean))
                        throw lille_exception("Cannot use operator 'not' in a non-boolean ");
                    scan->must_be(symbol::not_sym);
                    finished = false;
                }
                else if(scan->have(symbol::identifier)) {
                    current_ident = table->lookup(scan->get_current_identifier_name());
                    if(not current_entry->tipe().is_type(current_ident->tipe()))
                        error->flag(scan->this_token(), 121);
                    scan->must_be(symbol::identifier);

                    /**** HANDLE FUNCTION/PROCEDURE CALL ****/
                    if(current_ident->tipe().is_type(lille_type::type_func) or current_ident->tipe().is_type(lille_type::type_proc)) {
                        if(current_ident->tipe().is_type(lille_type::type_func) or current_ident->tipe().is_type(lille_type::type_proc))
                            handle_function_or_procedure_call(current_ident);
                        else
                            error->flag(scan->this_token(), 121);
                    }
                    finished = false;
                }
                // If is a number ->
                else if(IS_NUMBER()) {
                    // Make sure variable is the same type
                    lille_type ty;
                    if(scan->have(symbol::integer))
                        ty = lille_type::type_integer;
                    else
                        ty = lille_type::type_real;
                    if(not current_entry->tipe().is_type(ty)) 
                        throw lille_exception("Value given does not match the type of " + current_entry_name);
                    if(scan->have(symbol::integer)) {
                        scan->must_be(symbol::integer);
                    }
                    else if(scan->have(symbol::real_num)) {
                        scan->must_be(symbol::real_num);
                    }
                    finished = false;
                }
                // If is a mathmatical operation 
                else if(IS_ADDOP()) {
                    if (scan->have(symbol::plus_sym)) 
                        scan->must_be(symbol::plus_sym);
                    else 
                        scan->must_be(symbol::minus_sym);
                    // Check if variable is of number type
                    if(not current_entry->tipe().is_type(lille_type::type_integer) and not current_entry->tipe().is_type(lille_type::type_real))
                        throw lille_exception("Cannot add or subtract non-numbers");
                    finished = false;
                }
                else if(IS_MULTOP()) {
                    if (scan->have(symbol::asterisk_sym)) 
                        scan->must_be(symbol::asterisk_sym);
                    else 
                        scan->must_be(symbol::slash_sym);
                    if(not current_entry->tipe().is_type(lille_type::type_integer) and not current_entry->tipe().is_type(lille_type::type_real))
                        throw lille_exception("Cannot multiply or divide non-numbers");
                    finished = false;
                }
                // If is a boolean comparison
                else if(IS_RELOP()) {
                    if (scan->have(symbol::greater_than_sym)) 
                        scan->must_be(symbol::greater_than_sym);
                    else if (scan->have(symbol::less_than_sym)) 
                        scan->must_be(symbol::less_than_sym);
                    else if (scan->have(symbol::equals_sym)) 
                        scan->must_be(symbol::equals_sym);
                    else if (scan->have(symbol::not_equals_sym)) 
                        scan->must_be(symbol::not_equals_sym);
                    else if (scan->have(symbol::less_or_equal_sym)) 
                        scan->must_be(symbol::less_or_equal_sym);
                    else  if (scan->have(symbol::greater_or_equal_sym))
                        scan->must_be(symbol::greater_or_equal_sym);
                    else
                        throw lille_exception("Expected a logical symbol (> < <> = >= <=)");
                    // Check if variable is of boolean type
                    if(not current_entry->tipe().is_type(lille_type::type_boolean))
                        throw lille_exception("Cannot compare non-boolean values");
                    finished = false;
                }
                else if(IS_BOOL()) {
                    if(not current_entry->tipe().is_type(lille_type::type_boolean))
                        throw lille_exception("Cannot assign boolean values to a non boolean identifier");
                    finished = false;
                    if(scan->have(symbol::true_sym))
                        scan->must_be(symbol::true_sym);
                    else if(scan->have(symbol::false_sym))
                        scan->must_be(symbol::false_sym);
                    else
                        throw lille_exception("Expected a boolean value");
                }
                else if(scan->have(symbol::strng)) {
                    if(not current_entry->tipe().is_type(lille_type::type_string))
                        throw lille_exception("Cannot assign string values to a non string identifier");
                    finished = false;
                    scan->must_be(symbol::strng);
                }
                else {
                    finished = true;
                }
                
            }
            while(not finished);
        }
    } 

    // If exit sym is found ->
    else if (scan->have(symbol::exit_sym)) {
        scan->must_be(symbol::exit_sym);
        if (scan->have(symbol::when_sym)) {
            scan->must_be(symbol::when_sym);
            // Make sure indent given is an integer
            current_ident = table->lookup(scan->get_current_identifier_name());
            if(not current_ident->tipe().is_type(lille_type::type_integer))
                throw lille_exception("Exit condition must be of Integer type");
            scan->must_be(symbol::identifier);
            if (scan->have(symbol::greater_than_sym)) 
                scan->must_be(symbol::greater_than_sym);
            else if (scan->have(symbol::less_than_sym)) 
                scan->must_be(symbol::less_than_sym);
            else if (scan->have(symbol::equals_sym)) 
                scan->must_be(symbol::equals_sym);
            else if (scan->have(symbol::not_equals_sym)) 
                scan->must_be(symbol::not_equals_sym);
            else if (scan->have(symbol::less_or_equal_sym)) 
                scan->must_be(symbol::less_or_equal_sym);
            else  if (scan->have(symbol::greater_or_equal_sym))
                scan->must_be(symbol::greater_or_equal_sym);
            else
                throw lille_exception("Expected a logical symbol (> < <> = >= <=)");
            // Make sure next ident is also an integer
            if(scan->have(symbol::identifier)) {
                current_ident = table->lookup(scan->get_current_identifier_name());
                if(not current_ident->tipe().is_type(lille_type::type_integer))
                    throw lille_exception("Exit condition must be of Integer type");
                scan->must_be(symbol::identifier);
            }
            else {
                // else make sure an integer is given
                scan->must_be(symbol::integer);
            }
        }
    } 
    // If a return sym is found ->
    else if (scan->have(symbol::return_sym)) {
        scan->must_be(symbol::return_sym);
        while (IS_EXPR() or IS_ADDOP() or IS_MULTOP()) {
            // Make sure the identifier given is the functions return type
            if(scan->have(symbol::identifier)) {
                id_table_entry* current_return_ident = table->lookup(scan->get_current_identifier_name());
                if(not current_fun_or_proc->tipe().is_type(lille_type::type_func))
                    throw lille_exception("Return in non-function");
                else if(current_return_ident->tipe().is_equal(lille_type::type_func)) {
                    if(not current_fun_or_proc->return_tipe().is_type(current_return_ident->return_tipe()))
                        throw lille_exception("ERROR: Function return value is " + current_fun_or_proc->return_tipe().to_string() + " but given " + current_return_ident->tipe().to_string());
                }
                else if(not current_fun_or_proc->return_tipe().is_type(current_return_ident->tipe()))
                        throw lille_exception("Function return value is " + current_fun_or_proc->return_tipe().to_string() + " but given " + current_return_ident->tipe().to_string());
                scan->must_be(symbol::identifier);
            }
            // Else if a number given is of functions return type
            else if(IS_NUMBER()) {
                if(scan->have(symbol::integer)) {
                    if(not current_fun_or_proc->return_tipe().is_type(lille_type::type_integer))
                        throw lille_exception("Function return value is " + current_fun_or_proc->tipe().to_string() + " but given Integer");
                    scan->must_be(symbol::integer);
                }
                if(scan->have(symbol::real_num)) {
                    if(not current_fun_or_proc->return_tipe().is_type(lille_type::type_integer))
                        throw lille_exception("Function return value is " + current_fun_or_proc->tipe().to_string() + " but given Real Number");
                    scan->must_be(symbol::real_num);
                }
            }
            // Else if a boolean given is of functions return type
            else if(IS_BOOL()) {
                if(scan->have(symbol::true_sym)) 
                    scan->must_be(symbol::true_sym);
                else if(scan->have(symbol::false_sym)) 
                    scan->must_be(symbol::false_sym);
                if(not current_fun_or_proc->tipe().is_type(lille_type::type_integer))
                        throw lille_exception("Function return value is " + current_fun_or_proc->tipe().to_string() + " but given Boolean");
                    
            }
            // Else if a string given is of functions return type
            else if(scan->have(symbol::strng)) {
                if(not current_fun_or_proc->tipe().is_type(lille_type::type_integer))
                        throw lille_exception("Function return value is " + current_fun_or_proc->tipe().to_string() + " but given String");
                scan->must_be(symbol::strng);
            }
            else throw lille_exception("Function return is " + current_fun_or_proc->tipe().to_string() + " but given Unknown");
        
            if(IS_ADDOP()) {
                    if (scan->have(symbol::plus_sym)) 
                        scan->must_be(symbol::plus_sym);
                    else 
                        scan->must_be(symbol::minus_sym);
                }
                else if(IS_MULTOP()) {
                    if (scan->have(symbol::asterisk_sym)) 
                        scan->must_be(symbol::asterisk_sym);
                    else 
                        scan->must_be(symbol::slash_sym);
                }
        }
    } 

    // If a read sym is found ->
    else if (scan->have(symbol::read_sym)) {
        scan->must_be(symbol::read_sym);
        bool lp = false, comma_sym = false;
        // Could have a parentheses wrapped around the call, if so eat the symbols
        if (scan->have(symbol::left_paren_sym)) {
            scan->must_be(symbol::left_paren_sym);
            lp = true;
        }
        // Find if variables given in function exist
        do {
            current_ident = table->lookup(scan->get_current_identifier_name());
            if(current_ident == NULL)
                throw lille_exception("Undeclared Variable " + scan->get_current_identifier_name());
            scan->must_be(symbol::identifier);
            if(scan->have(symbol::comma_sym)) {
                comma_sym = true;
                scan->must_be(symbol::comma_sym);
            }
            else comma_sym = false;
        }
        while (comma_sym);

        if(lp) {
            scan->must_be(symbol::right_paren_sym);
        }
    } 

    // If write sym is found ->
    else if (scan->have(symbol::write_sym)) {
        scan->must_be(symbol::write_sym);
        bool lp = false, amp_sym = false;
        // Could have a parentheses wrapped around the call, if so eat the symbols
        if (scan->have(symbol::left_paren_sym)) {
            scan->must_be(symbol::left_paren_sym);
            lp = true;
        }
        // Register the entire string, accounting for `&`s and `,`s
        do {
            if(scan->have(symbol::identifier)) {
                current_ident = table->lookup(scan->get_current_identifier_name());
                scan->must_be(symbol::identifier);
                if(current_ident->tipe().is_type(lille_type::type_func) or current_ident->tipe().is_type(lille_type::type_proc))
                    handle_function_or_procedure_call(current_ident);
            }
            else if(scan->have(symbol::strng)) {
                scan->must_be(symbol::strng);
            }
            if(scan->have(symbol::ampersand_sym)) {
                scan->must_be(symbol::ampersand_sym);
                amp_sym = true;
            }   
            else if(scan->have(symbol::comma_sym)) {
                scan->must_be(symbol::comma_sym);
                amp_sym = true;
            }
            else amp_sym = false;
        }
        while(amp_sym);
        if (lp) {
            scan->must_be(symbol::right_paren_sym);
        } 
    }

    // If writeln sym is found ->
    else if (scan->have(symbol::writeln_sym)) {
        scan->must_be(symbol::writeln_sym);
        bool lp = false, amp_sym = false;
        // Could have a parentheses wrapped around the call, if so eat the symbols
        if (scan->have(symbol::left_paren_sym)) {
            scan->must_be(symbol::left_paren_sym);
            lp = true;
        }
        // Same as write, register the entire string, accounting for `&`s and `,`s
        do {
            if(scan->have(symbol::identifier)) {
                current_ident = table->lookup(scan->get_current_identifier_name());
                scan->must_be(symbol::identifier);
                if(current_ident->tipe().is_type(lille_type::type_func) or current_ident->tipe().is_type(lille_type::type_proc))
                    handle_function_or_procedure_call(current_ident);
                
            }
            else if(scan->have(symbol::strng)) {
                scan->must_be(symbol::strng);
            }
            if(scan->have(symbol::ampersand_sym)) {
                scan->must_be(symbol::ampersand_sym);
                amp_sym = true;
            }   
            else if(scan->have(symbol::comma_sym)) {
                scan->must_be(symbol::comma_sym);
                amp_sym = true;
            }
            else amp_sym = false;
        }
        while(amp_sym);
        if (lp) {
            scan->must_be(symbol::right_paren_sym);
        }
    } 
    else {
        scan->must_be(symbol::null_sym);
    }
    current_entry = NULL;
}

void parser::IF_STATEMENT() {
    scan->must_be(symbol::if_sym);
    handle_if_and_while();
    scan->must_be(symbol::then_sym);
    STATEMENT_LIST();
    while (scan->have(symbol::elsif_sym)) {
        scan->must_be(symbol::elsif_sym);
        handle_if_and_while();
        scan->must_be(symbol::then_sym);
        STATEMENT_LIST();
    }
    if (scan->have(symbol::else_sym)) {
        scan->must_be(symbol::else_sym);
        STATEMENT_LIST();
    }
    scan->must_be(symbol::end_sym);
    scan->must_be(symbol::if_sym);
}

void parser::LOOP_STATEMENT() {

    scan->must_be(symbol::loop_sym);
    STATEMENT_LIST();
    scan->must_be(symbol::end_sym);
    scan->must_be(symbol::loop_sym);
}

void parser::FOR_STATEMENT() {
    scan->must_be(symbol::for_sym);
    
    symbol* sym = new symbol(symbol::identifier);
    token* tok = new token(sym, 0, 0);
    tok->set_identifier_value(scan->get_current_identifier_name());
    id_table_entry* for_entry = table->enter_id(tok, lille_type::type_integer, lille_kind::for_ident, table->scope(), 0, lille_type::type_unknown);
    table->add_table_entry(for_entry);
    scan->must_be(symbol::identifier);

    scan->must_be(symbol::in_sym);
    if (scan->have(symbol::reverse_sym)) {
        scan->must_be(symbol::reverse_sym);
    }
    scan->must_be(symbol::integer);
    scan->must_be(symbol::range_sym);
    scan->must_be(symbol::integer);
    LOOP_STATEMENT();
}

void parser::WHILE_STATEMENT() {

    scan->must_be(symbol::while_sym);
    handle_if_and_while();
    LOOP_STATEMENT();
}

bool parser::IS_NUMBER() {
    if (scan->have(symbol::real_num) or scan->have(symbol::integer)) 
        return true;
    return false;
}

bool parser::IS_ADDOP() {
    if (scan->have(symbol::plus_sym) or scan->have(symbol::minus_sym)) 
        return true;
    return false;
}

bool parser::IS_MULTOP() {
    if (scan->have(symbol::asterisk_sym) or scan->have(symbol::slash_sym))
        return true;
    return false;
}

bool parser::IS_RELOP() {
    if (scan->have(symbol::less_than_sym) or scan->have(symbol::equals_sym) or scan->have(symbol::greater_than_sym) or scan->have(symbol::less_or_equal_sym) or scan->have(symbol::greater_or_equal_sym) or scan->have(symbol::not_equals_sym)) 
        return true;
    return false;
}

bool parser::IS_EXPR() {
    if (scan->have(symbol::not_sym) or scan->have(symbol::odd_sym) or scan->have(symbol::left_paren_sym) or scan->have(symbol::identifier) or IS_NUMBER() or scan->have(symbol::strng) or IS_BOOL() or IS_ADDOP() or IS_NUMBER()) 
        return true;
    return false;
}

bool parser::IS_BOOL() {
    if (scan->have(symbol::true_sym) or scan->have(symbol::false_sym)) 
        return true;
    return false;
}

bool parser::IS_STATEMENT() {
    if (scan->have(symbol::identifier) or scan->have(symbol::exit_sym) or scan->have(symbol::return_sym) or scan->have(symbol::read_sym) or scan->have(symbol::write_sym) or scan->have(symbol::writeln_sym) or scan->have(symbol::null_sym) or scan->have(symbol::if_sym) or scan->have(symbol::loop_sym) or scan->have(symbol::for_sym) or scan->have(symbol::while_sym))
        return true;
    return false;
}

bool parser::IS_DECLERATION() {
    if (scan->have(symbol::identifier) or scan->have(symbol::procedure_sym) or scan->have(symbol::function_sym)) 
        return true;
    return false;
}

lille_type parser::get_ident_type() {
    switch (scan->this_token()->get_symbol()->get_sym()) {
        case symbol::integer_sym:
            return lille_type::type_integer;
        case symbol::real_sym:
            return lille_type::type_real;
        case symbol::boolean_sym:
            return lille_type::type_boolean;
        case symbol::string_sym:
            return lille_type::type_string;
        default:
            return lille_type::type_unknown;
    }
}

void parser::handle_function_or_procedure_call(id_table_entry* current_entry) {
    bool lp = false;
    if(scan->have(symbol::left_paren_sym)) {
        scan->must_be(symbol::left_paren_sym);
        lp = true;
    }

    string current_entry_name = current_entry->name();
    if(current_entry->tipe().is_type(lille_type::type_func)) {
        /**** HANDEL FUNCTION CALL ****/
        for(int i = 0; i < current_entry->number_of_params(); i++) {
            if(scan->have(symbol::identifier)) {
                current_ident = table->lookup(scan->get_current_identifier_name());
                if(current_ident->kind().is_kind(lille_kind::for_ident)) {
                    error->flag(scan->this_token(), 96);
                }
                else if(current_ident->tipe().is_type(lille_type::type_func)) {
                    if(current_entry->nth_parameter(i)->kind().is_kind(lille_kind::ref_param)) {
                        error->flag(scan->this_token(), 123);
                    }
                    if(not current_entry->nth_parameter(i)->tipe().is_type(current_ident->return_tipe())) {
                        throw lille_exception("Identifier " + current_ident->name() + " Does Not Match Parameter Type in " + current_entry_name);
                    }
                }
                else if(not current_entry->nth_parameter(i)->tipe().is_type(current_ident->tipe())) {
                    throw lille_exception("Identifier " + current_ident->name() + " Does Not Match Parameter Type in " + current_entry_name);
                }
                scan->must_be(symbol::identifier);
            }
            else {
                if(not current_entry->nth_parameter(i)->tipe().is_type(get_type())) 
                    throw lille_exception("Value Given Does Not Match Parameter Type in " + current_entry_name);
                if(scan->have(symbol::integer))
                   scan->must_be(symbol::integer);
                else if(scan->have(symbol::real_num))
                    scan->must_be(symbol::real_num);
                else if(scan->have(symbol::strng))
                    scan->must_be(symbol::strng);
                else if(scan->have(symbol::true_sym))
                    scan->must_be(symbol::true_sym);
                else if(scan->have(symbol::false_sym))
                    scan->must_be(symbol::false_sym);
                else throw lille_exception("Invalid type given");
            }
            if(scan->have(symbol::comma_sym))
                scan->must_be(symbol::comma_sym);
        }
    }
    else if(current_entry->tipe().is_type(lille_type::type_proc)) {
        /**** HANDEL PROCEDURE CALL ****/
        for(int i = 0; i < current_entry->number_of_params(); i++) {
            if(scan->have(symbol::identifier)) {
                current_ident = table->lookup(scan->get_current_identifier_name());
                if(current_ident->kind().is_kind(lille_kind::value_param) and current_entry->nth_parameter(i)->kind().is_kind(lille_kind::ref_param)) {
                    error->flag(scan->this_token(), 98);
                }
                else if(current_ident->kind().is_kind(lille_kind::for_ident) and not current_entry->nth_parameter(i)->kind().is_kind(lille_kind::value_param)) {
                    throw lille_exception("Cannot pass FOR loop identifier into procedure");
                }
                else if(current_ident->tipe().is_type(lille_type::type_func)) {
                    if(current_entry->nth_parameter(i)->kind().is_kind(lille_kind::ref_param)) {
                        error->flag(scan->this_token(), 123);
                    }
                    if(not current_entry->nth_parameter(i)->tipe().is_type(current_ident->return_tipe())) {
                        throw lille_exception("Identifier " + current_ident->name() + " Does Not Match Parameter Type in " + current_entry_name);
                    }
                }
                else if(not current_entry->nth_parameter(i)->tipe().is_type(current_ident->tipe())) {
                    throw lille_exception("Identifier " + current_ident->name() + " Does Not Match Parameter Type in " + current_entry_name);
                }
                scan->must_be(symbol::identifier);
            }
            else {
                if(not current_entry->nth_parameter(i)->tipe().is_type(get_type())) 
                    throw lille_exception("Value Given Does Not Match Parameter Type in " + current_entry_name);
                if(scan->have(symbol::integer))
                   scan->must_be(symbol::integer);
                else if(scan->have(symbol::real_num))
                    scan->must_be(symbol::real_num);
                else if(scan->have(symbol::strng))
                    scan->must_be(symbol::strng);
                else if(scan->have(symbol::true_sym))
                    scan->must_be(symbol::true_sym);
                else if(scan->have(symbol::false_sym))
                    scan->must_be(symbol::false_sym);
                else throw lille_exception("Invalid type given");
            }
            if(scan->have(symbol::comma_sym))
                scan->must_be(symbol::comma_sym);
        }
        
    }
    else throw lille_exception("Unkown Function/Procedure " + current_entry_name);

    if(lp) {
        scan->must_be(symbol::right_paren_sym);
    }
}

void parser::handle_if_and_while() {

    bool and_or_flag;
    do {
        if(scan->have(symbol::identifier)) {
            id_table_entry* if_cond = table->lookup(scan->get_current_identifier_name());
            if(if_cond == NULL)
                throw lille_exception("Undeclared identifier " + scan->get_current_identifier_name());
            scan->must_be(symbol::identifier);
            if(IS_RELOP()) {
                if (scan->have(symbol::greater_than_sym)) 
                    scan->must_be(symbol::greater_than_sym);
                else if (scan->have(symbol::less_than_sym)) 
                    scan->must_be(symbol::less_than_sym);
                else if (scan->have(symbol::equals_sym)) 
                    scan->must_be(symbol::equals_sym);
                else if (scan->have(symbol::not_equals_sym)) 
                    scan->must_be(symbol::not_equals_sym);
                else if (scan->have(symbol::less_or_equal_sym)) 
                    scan->must_be(symbol::less_or_equal_sym);
                else  if (scan->have(symbol::greater_or_equal_sym))
                    scan->must_be(symbol::greater_or_equal_sym);
                else
                    throw lille_exception("Expected a logical symbol (> < <> = >= <=)");
                if(scan->have(symbol::identifier)) {
                    id_table_entry* if_cond2 = table->lookup(scan->get_current_identifier_name());
                    if(if_cond2 == NULL)
                        throw lille_exception("Undeclared identifier " + scan->get_current_identifier_name());
                    if(not if_cond->tipe().is_type(if_cond2->tipe())) 
                        throw lille_exception("Cannot compare 2 items of different types");
                }
                else if(scan->have(symbol::integer) or scan->have(symbol::real_num)) {
                    if(not if_cond->tipe().is_type(get_type()))
                        throw lille_exception("Can only compare values of same type");
                    if(scan->have(symbol::integer))
                        scan->must_be(symbol::integer);
                    else
                        scan->must_be(symbol::real_num);
                }
                else if(scan->have(symbol::true_sym)) {
                    scan->must_be(symbol::true_sym);
                }
                else if(scan->have(symbol::false_sym)) {
                    scan->must_be(symbol::false_sym);
                }
                else throw lille_exception("Invalid argument for loop");
            }
            else if(not if_cond->tipe().is_type(lille_type::type_boolean)) {
                throw lille_exception("Invalid argument for loop");
            }        
        }
        else if(scan->have(symbol::true_sym)) {
            scan->must_be(symbol::true_sym);
        }
        else if(scan->have(symbol::false_sym)) {
            scan->must_be(symbol::false_sym);
        }
        else throw lille_exception("Invalid argument for `if`");

        if(scan->have(symbol::and_sym)) {
                scan->must_be(symbol::and_sym);
                and_or_flag = true;
        }
        else if(scan->have(symbol::or_sym)) {
            scan->must_be(symbol::or_sym);
            and_or_flag = true;
        }
        else and_or_flag = false;
    }
    while(and_or_flag);
}

lille_type parser::get_type() {
    if(scan->have(symbol::integer))
        return lille_type::type_integer;
    else if(scan->have(symbol::real_num))
        return lille_type::type_real;
    else if(scan->have(symbol::strng))
        return lille_type::type_string;
    else if(scan->have(symbol::true_sym) or scan->have(symbol::false_sym))
        return lille_type::type_boolean;
    else return lille_type::type_unknown;
}

list<token*> parser::IDENT_LIST() {
    // create an array of tokens to store all variables and their names
        list<token*> variables;

        bool comma_flag = false;
        
        do {
            // For each identifier found ->
            if(scan->have(symbol::identifier)) {
                // Add the new variable into the array
                variables.push_back(new token(new symbol(symbol::identifier), 0, 0));
                // Assign the name to the token
                variables.back()->set_identifier_value(scan->get_current_identifier_name());
                scan->must_be(symbol::identifier);
            }
            // If there are more variables ->
            comma_flag = scan->have(symbol::comma_sym);
            if(comma_flag)
                scan->must_be(symbol::comma_sym);
        }
        while(comma_flag);

    return variables;
}

void parser::PARAM() {
    
    id_table_entry* id;
    if(scan->have(symbol::left_paren_sym)) {
        scan->must_be(symbol::left_paren_sym);
       
        bool semi_flag = false;
        // Find all paramters ->
        do {
            // If a new parameter is found ->
            if(scan->have(symbol::identifier)) {
                symbol* sym = new symbol(symbol::identifier);
                token* ident = new token(sym, 0, 0);
                ident->set_identifier_value(scan->get_current_identifier_name());
                scan->must_be(symbol::identifier);
                scan->must_be(symbol::colon_sym);
                // Get the kind of param (ref or value)
                int k = scan->this_token()->get_symbol()->get_sym();
                lille_kind knd = lille_kind::unknown;
                switch(k) {
                    case symbol::ref_sym: {
                        scan->must_be(symbol::ref_sym);
                        knd = lille_kind::ref_param;
                        break;
                    }
                    case symbol::value_sym: {
                        scan->must_be(symbol::value_sym);
                        knd = lille_kind::value_param;
                        break;
                    }
                }
                // Create the entry for the parameter
                lille_type ty;
    
                
                if (scan->have(symbol::integer_sym)) {
                    ty = lille_type::type_integer;
                    scan->must_be(symbol::integer_sym);
                }
                else if (scan->have(symbol::real_sym)) {
                    ty = lille_type::type_real;
                    scan->must_be(symbol::real_sym);
                }
                else if(scan->have(symbol::string_sym)) {
                    ty = lille_type::type_string;
                    scan->must_be(symbol::string_sym);
                }
                else {
                    ty = lille_type::type_boolean;
                    scan->must_be(symbol::boolean_sym);
                }

                id = table->enter_id(ident, ty, knd, table->scope(), 0, lille_type::type_unknown);
            }
            // add the entry to the table
            table->add_table_entry(id);
            // link the parameter to the procedure
            current_fun_or_proc->add_param(id);

            // If a semi-colon is found, keep going
            semi_flag = scan->have(symbol::semicolon_sym);
            if(semi_flag)
                scan->must_be(symbol::semicolon_sym);
        }
        while(semi_flag);
        scan->must_be(symbol::right_paren_sym);
    }
}
