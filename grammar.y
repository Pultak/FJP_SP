%{
#include <cstdio>
#include <iostream>
#include "../file.h"

extern "C" int yylex();
int yyerror(const char *s);

bool print_symbol_match = false;
file* ast_root = NULL;

extern FILE *yyin, *yyout;
bool verbose_out = false;

static void _print_symbol_match(const std::string& symbol, const std::string& str) {
    if (print_symbol_match) {
        std::cout << "[" << symbol << "]: " << str << std::endl;
    }
}

#define YY_NO_UNISTD_H

%}

%union{
    char string_lit[200];
    int type;
    int number_lit;
    char identifier[30];
    char log_op[5];
    char math_op[5];
    char bin_op[5];
    char comparison[5];
    char parenthesis;
    char bracket;

    file* file;
    std::list<global_statement*>* global_statement;
    method_declaration* method_declaration;
    variable_declaration* variable_declaration;
    declaration* my_declaration;
    value* my_value;
    math* math;
    method_call* method_call;
    std::list<value*>* expressions;
    expression* expression;
    assign_expression* assign_expression;
    command* my_command;
    std::list<command*>* commands;
    block* block;
    std::list<declaration*>* parameters;
    boolean_expression* boolean_expression;
    condition* condition;
    loop* loop;
    std::list<declaration*>* multi_declaration;
    struct_definition* struct_definition;
}

%define parse.error verbose

%token STRING_LIT NUMBER_LIT OTHER SEMICOLON QUESTION COLON IDENTIFIER TYPE WHITESPACE LOG_OP BIN_OP COUNT_OP MULTI_OP COMPARISON
%token ASSIGN NOT
%token B_L_CURLY B_R_CURLY B_L_SQUARE B_R_SQUARE PAREN_L PAREN_R

// reserved keywords
%token STRUCT WHILE FOR IF ELSE CONST RETURN COMMA DOT TRUE FALSE

%token END 0 "end of file"

%type <file> file
%type <global_statement> global
%type <method_declaration> method
%type <variable_declaration> variable
%type <my_declaration> declaration
%type <my_value> value
%type <math> math
%type <method_call> method_call
%type <expressions> statements
%type <expression> statement
%type <assign_expression> assigning
%type <my_command> command
%type <commands> commands
%type <block> block
%type <parameters> parameters
%type <boolean_expression> logical_statement
%type <loop> loop
%type <condition> condition
%type <multi_declaration> multi_declaration
%type <struct_definition> shrek_def

%type <string_lit> STRING_LIT
%type <number_lit> NUMBER_LIT
%type <identifier> IDENTIFIER
%type <type> TYPE
%type <log_op> LOG_OP
%type <math_op> COUNT_OP
%type <math_op> MULTI_OP
%type <bin_op> BIN_OP
%type <comparison> COMPARISON

%type <bracket> B_L_CURLY
%type <bracket> B_R_CURLY
%type <bracket> B_L_SQUARE
%type <bracket> B_R_SQUARE

%type <parenthesis> PAREN_L
%type <parenthesis> PAREN_R

%left LOG_OP
%left COMPARISON
%left COUNT_OP
%left MULTI_OP
%left NOT

%start file
%%

file:
    global {
    	$$ = new file($1);
        ast_root = $$;
    }
;

global:
     global shrek_def {
    	$$ = $1;
        $1->push_back($2);
    }
    | global method {
    	$$ = $1;
        $1->push_back($2);
    }
    | global variable {
    	$$ = $1;
        $1->push_back($2);
    }
    | {
    	$$ = new std::list<global_statement*>();
    }
;

declaration:
    STRUCT IDENTIFIER IDENTIFIER {
    	_print_symbol_match("declaration", "variable structure: struct name=" + std::string($2) + " identifier=" + std::string($3));
	$$ = new declaration(TYPE_META_STRUCT, $2, $3);
    }
    | TYPE IDENTIFIER {
    	_print_symbol_match("declaration", "variable declaration: type=" + std::to_string($1) + " identifier=" + std::string($2));
        $$ = new declaration($1, $2);
    }
    | TYPE IDENTIFIER B_L_SQUARE NUMBER_LIT B_R_SQUARE {
    	_print_symbol_match("declaration", "array declaration: type=" + std::to_string($1) + " identifier=" + std::string($2) + " size=" + std::to_string($4));
        $$ = new declaration($1, $2, $4);
    }
;

variable:
    CONST declaration ASSIGN value SEMICOLON {
    	_print_symbol_match("variable", "constant declaration with initialization");
	$$ = new variable_declaration($2, true, $4);
    }
    |declaration SEMICOLON {
    	_print_symbol_match("variable", "variable declaration");
	$$ = new variable_declaration($1);
    }
    | declaration ASSIGN value SEMICOLON {
	_print_symbol_match("variable", "variable declaration with initialization");
        $$ = new variable_declaration($1, false, $3);
    }
;

condition:
    IF PAREN_L value PAREN_R block {
    	_print_symbol_match("condition", "if condition (without else)");
	$$ = new condition($3, $5);
    }
    | IF PAREN_L value PAREN_R block ELSE block {
    	_print_symbol_match("condition", "if condition (with else)");
	$$ = new condition($3, $5, $7);
    }
;

loop:
    FOR PAREN_L statement SEMICOLON value SEMICOLON statement PAREN_R block {
    	_print_symbol_match("loop", "for loop");
	$$ = new for_loop($3, $5, $7, $9);
    }
    | WHILE PAREN_L value PAREN_R block {
    	_print_symbol_match("loop", "while loop");
	$$ = new while_loop($3, $5);
    }
;

math:
    value COUNT_OP value {
    	_print_symbol_match("math", "math expression (+, -), A " + std::string($2) + " B");
	$$ = new math($1, math::get_operation($2), $3);
    }
    | value MULTI_OP value {
    	_print_symbol_match("math", "math expression (*, /), A " + std::string($2) + " B");
	$$ = new math($1, math::get_operation($2), $3);
    }
;

value:
    math {
    	_print_symbol_match("value", "math expression");
	$$ = new value($1);
    }
    | method_call {
    	_print_symbol_match("value", "function call");
	$$ = new value($1);
    }
    | logical_statement {
    	_print_symbol_match("value", "boolean expression");
	$$ = new value($1);
    }
    | logical_statement QUESTION value COLON value {
    	_print_symbol_match("value", "ternary operator");
	$$ = new value($1, $3, $5);
    }
    | assigning {
    	_print_symbol_match("value", "assign expression");
	$$ = new value($1);
    }
    | NUMBER_LIT {
    	_print_symbol_match("value", "integer literal '" + std::to_string($1) + "'");
	$$ = new value($1);
    }
    | STRING_LIT {
    	_print_symbol_match("value", "string literal '" + std::string($1) + "'");
	$$ = new value($1);
    }
    | IDENTIFIER {
    	_print_symbol_match("value", "scalar identifier '" + std::string($1) + "'");
	$$ = new value(new variable_ref($1));
    }
    | IDENTIFIER DOT IDENTIFIER {
    	_print_symbol_match("value", "struct '" + std::string($1) + "' member '" + std::string($3) + "'");
	$$ = new value($1, $3);
    }
    | IDENTIFIER B_L_SQUARE value B_R_SQUARE {
    	_print_symbol_match("value", "array '" + std::string($1) + "' (indexed)");
	$$ = new value(new variable_ref($1), $3);
    }
    | PAREN_L value PAREN_R {
    	_print_symbol_match("value", "parenthesis enclosure");
	$$ = $2;
    }
;

assigning:
    IDENTIFIER DOT IDENTIFIER ASSIGN value {
    	_print_symbol_match("assign_expression", "struct '" + std::string($1) + "' member '" + std::string($3) + "' as left-hand side");
	$$ = new assign_expression($1, $3, $5);
    }
    | IDENTIFIER ASSIGN value {
    	_print_symbol_match("assign_expression", "scalar left-hand side '" + std::string($1) + "'");
	$$ = new assign_expression($1, (value*)nullptr, $3);
    }
    | IDENTIFIER B_L_SQUARE value B_R_SQUARE ASSIGN value {
    	_print_symbol_match("assign_expression", "array element of left-hand side '" + std::string($1) + "'");
	$$ = new assign_expression($1, $3, $6);
    }
;

statement:
    value {
    	_print_symbol_match("expression", "value expression (discarding return value)");
	$$ = new expression($1);
    }
;

command:
    variable {
    	_print_symbol_match("command", "variable declaration");
	$$ = new command($1);
    }
    | statement SEMICOLON {
    	_print_symbol_match("command", "expression");
	$$ = new command($1);
    }
    | loop {
	_print_symbol_match("command", "loop");
	$$ = new command($1);
    }
    | condition {
	_print_symbol_match("command", "condition");
	$$ = new command($1);
    }
    | RETURN value SEMICOLON {
    	_print_symbol_match("command", "return statement");
	$$ = new command($2);
    }

;

commands:
    commands command {
    	_print_symbol_match("commands", "non-terminal command list match");
	$$ = $1;
	$1->push_back($2);
    }
    | {
    	_print_symbol_match("commands", "terminal command list match");
	$$ = new std::list<command*>();
    }
;

block:
    B_L_CURLY B_R_CURLY {
    	_print_symbol_match("block", "empty block");
	$$ = new block(nullptr);
    }
    | B_L_CURLY commands B_R_CURLY {
    	_print_symbol_match("block", "multi-line block");
	$$ = new block($2);
    }
    | command {
    	_print_symbol_match("block", "single-line block");
	std::list<command*>* lst = new std::list<command*>();
	lst->push_back($1);
	$$ = new block(lst);
    }
;

parameters:
    declaration {
    	_print_symbol_match("parameters", "terminal function parameters definition");
	$$ = new std::list<declaration*>{ $1 };
    }
    | parameters COMMA declaration {
    	_print_symbol_match("parameters", "non-terminal function parameters definition");
	$$ = $1;
	$1->push_back($3);
    }
;

method:
    declaration PAREN_L PAREN_R block {
    	_print_symbol_match("function", "non-parametric function definition");
	$$ = new method_declaration($1, $4);
    }
    | declaration PAREN_L PAREN_R SEMICOLON {
    	_print_symbol_match("function", "non-parametric function forward declaration");
	$$ = new method_declaration($1, nullptr);
    }
    | declaration PAREN_L parameters PAREN_R block {
    	_print_symbol_match("function", "parametric function definition");
	$$ = new method_declaration($1, $5, $3);
    }
    | declaration PAREN_L parameters PAREN_R SEMICOLON {
    	_print_symbol_match("function", "parametric function forward declaration");
	$$ = new method_declaration($1, nullptr, $3);
    }
;

logical_statement:
    TRUE {
        _print_symbol_match("boolean_expression", "TRUE token");
        $$ = new boolean_expression(true);
    }
    | FALSE {
        _print_symbol_match("boolean_expression", "FALSE token");
        $$ = new boolean_expression(false);
    }
    |NOT value {
        _print_symbol_match("boolean_expression", "negation of value");
        $$ = new boolean_expression($2, nullptr, boolean_expression::operation::negate);
    }
    | value COMPARISON value {
        _print_symbol_match("boolean_expression", "comparison, A " + std::string($2) + " B");
        $$ = new boolean_expression($1, $3, boolean_expression::convert_to_bool_operation($2));
    }
    | PAREN_L logical_statement PAREN_R {
        _print_symbol_match("boolean_expression", "parenthesis enclosure");
        $$ = $2;
    }
    | value LOG_OP value {
        _print_symbol_match("boolean_expression", "boolean operation on values, A " + std::string($2) + " B");
        $$ = new boolean_expression($1, $3, boolean_expression::convert_to_bool_operation($2));
    }
;

multi_declaration:
    multi_declaration declaration SEMICOLON {
    	_print_symbol_match("multi_declaration", "non-terminal declaration");
	$$ = $1;
	$1->push_back($2);
    }
    | {
    	_print_symbol_match("multi_declaration", "terminal declaration");
	$$ = new std::list<declaration*>();
    }
;

shrek_def:
    STRUCT IDENTIFIER B_L_CURLY multi_declaration B_R_CURLY SEMICOLON {
        _print_symbol_match("struct_definition", "structure definition");
        $$ = new struct_definition($2, $4);
    }
;

statements:
    statements COMMA value {
        _print_symbol_match("expressions", "expression list as a function call parameter (multiple parameters)");
        $$ = $1;
        $1->push_back($3);
    }
    | value {
        _print_symbol_match("expressions", "expression list as a function call parameter (single parameter)");
        $$ = new std::list<value*>{$1};
    }
;


method_call:
    IDENTIFIER PAREN_L PAREN_R {
        _print_symbol_match("method_call", "non-parametric function call");
        $$ = new method_call($1);
    }
    | IDENTIFIER PAREN_L statements PAREN_R {
        _print_symbol_match("method_call", "parametric function call");
        $$ = new method_call($1, $3);
    }
;

%%

int yyerror(const char *s)
{
	std::cerr << "Error: " << s << std::endl;
	return 0;
}

int parse_code(FILE* input, FILE* output)
{
    yyin = input;
    yyout = output;

    return yyparse();
}
