%{
#include <cstdio>
#include <iostream>

extern "C" int yylex();
int yyerror(const char *s);

extern FILE *yyin, *yyout;

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
}

%define parse.error verbose

%token STRING_LIT NUMBER_LIT OTHER SEMICOLON QUESTION COLON IDENTIFIER TYPE WHITESPACE LOG_OP BIN_OP COUNT_OP MULTI_OP COMPARISON
%token ASSIGN NOT
%token B_L_CURLY B_R_CURLY B_L_SQUARE B_R_SQUARE PAREN_L PAREN_R

// reserved keywords
%token STRUCT WHILE FOR IF ELSE CONST RETURN COMMA DOT TRUE FALSE

%token END 0 "end of file"

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
    	printf("Grammar root \n");
    }
;

global:
     global shrek_def {
    	printf("Struct found \n");
    }
    | global method {
    	printf("Fuction found \n");
    }
    | global variable {
    	printf("Variable found \n");
    }
    | {
    	printf("");
    }
;

declaration:
    STRUCT IDENTIFIER IDENTIFIER {
    	printf("Struct identifier found \n");
    }
    | TYPE IDENTIFIER {
    	printf("Type identifier found\n");
    }
    | TYPE IDENTIFIER B_L_SQUARE NUMBER_LIT B_R_SQUARE {
    	printf("Array definition found \n");
    }
;

variable:
    CONST declaration ASSIGN value SEMICOLON {
    	printf("Const variable declared\n");
    }
    |declaration SEMICOLON {
    	printf("Variable without assigned value\n");
    }
    | declaration ASSIGN value SEMICOLON {
    	printf("Variable with value assigned\n");
    }
;

condition:
    IF PAREN_L value PAREN_R block {
    	printf("Simple if condition\n");
    }
    | IF PAREN_L value PAREN_R block ELSE block {
    	printf("If + else condition\n");
    }
;

loop:
    FOR PAREN_L statement SEMICOLON value SEMICOLON statement PAREN_R block {
    	printf("For (smortloop) cycle\n");
    }|
    WHILE PAREN_L value PAREN_R block {
    	printf("While cycle\n");
    }
;

math:
    value COUNT_OP value {
    	printf("Value counting\n");
    }
    | value MULTI_OP value {
    	printf("Value multiplication\n");
    }
;

value:
    math {
    	printf("Sum counting found\n");
    }
    | method_call {
    	printf("Calling function\n");
    }
    | logical_statement {
    	printf("Boolean expression found\n");
    }
    | logical_statement QUESTION value COLON value {
    	printf("Ternary operator\n");
    }
    | assigning {
    	printf("Assigning value\n");
    }
    | NUMBER_LIT {
    	printf("Numba found\n");
    }
    | STRING_LIT {
    	printf("Sum charz found\n");
    }
    | IDENTIFIER {
    	printf("Value from identifier\n");
    }
    | IDENTIFIER DOT IDENTIFIER {
    	printf("Struct + its variable\n");
    }
    | IDENTIFIER B_L_SQUARE value B_R_SQUARE {
    	printf("Getting to array value\n");
    }
    | PAREN_L value PAREN_R {
    	printf("Value in parentheses\n");
    }
;

assigning:
    IDENTIFIER DOT IDENTIFIER ASSIGN value {
    	printf("Assigning value to struct atribute \n");
    }
    | IDENTIFIER ASSIGN value {
    	printf("Assigning to identifier\n");
    }
    | IDENTIFIER B_L_SQUARE value B_R_SQUARE ASSIGN value {
    	printf("Assigning value to array\n");
    }
;

statement:
    value {
    	printf("");
    }
;

command:
    variable {
    	printf("");
    }
    | statement SEMICOLON {
    	printf("");
    }
    | loop {
	printf("");
    }
    | condition {
	printf("");
    }
    | RETURN value SEMICOLON {
    	printf("");
    }

;

commands:
    commands command {
    	printf("");
    }
    | {
    	printf("");
    }
;

block:
    B_L_CURLY B_R_CURLY {
    	printf("Empty block body found\n");
    }
    | B_L_CURLY commands B_R_CURLY {
    	printf("Block body with commandas found \n");
    }
    | command {
    	printf("");
    }
;

parameters:
    declaration {
    	printf("Parameter declaration found \n");
    }
    | parameters COMMA declaration {
    	printf("Function declaration found \n");
    }
;

method:
    declaration PAREN_L PAREN_R block {
    	printf("Function without parameters definition \n");
    }
    | declaration PAREN_L PAREN_R SEMICOLON {
    	printf("Function without parameters definition \n");
    }
    | declaration PAREN_L parameters PAREN_R block {
    	printf("Function with parameters definition \n");
    }
    | declaration PAREN_L parameters PAREN_R SEMICOLON {
    	printf("");
    }
;

logical_statement:
    TRUE {
    	printf("True \n");
    }
    | FALSE {
    	printf("Truent \n");
    }
    |NOT value {
    	printf("Boolean negation\n");
    }
    | value COMPARISON value {
    	printf("Boolean comparison \n");
    }
    | PAREN_L logical_statement PAREN_R {
    	printf("Boolean in parentheses\n");
    }
    | value LOG_OP value {
    	printf("Boolean log operation \n");
    }
;

multi_declaration:
    multi_declaration declaration SEMICOLON {
    	printf("Multideclaration token \n");
    }
    | {
    	printf("");
    }
;

shrek_def:
    STRUCT IDENTIFIER B_L_CURLY multi_declaration B_R_CURLY SEMICOLON {
    	printf("Shrek declaration he stronk  \n");
    }
;

statements:
    statements COMMA value {
    	printf("function parameter token \n");
    }
    | value {
    	printf("function parameter token");
    }
;


method_call:
    IDENTIFIER PAREN_L PAREN_R {
    	printf("calling empty function \n");
    }
    | IDENTIFIER PAREN_L statements PAREN_R {
    	printf("Callin function with parameters \n");
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

