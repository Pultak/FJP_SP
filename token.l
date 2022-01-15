%option noyywrap
%{
#pragma once

#include <cstdio>
#include <cstring>
#include "../file.h"
#include "grammar.tab.h"


#define yyterminate() return END
#define YY_DECL extern "C" int yylex (void)

#define TYPE_INT    1
#define TYPE_CHAR   2
#define TYPE_STRING 3
#define TYPE_BOOL   4


void showError(const char* token);

int convert_to_type_enum(char *in) {
    if (strcmp(in, "numba") == 0) {
        return TYPE_INT;
    } else if (strcmp(in, "char") == 0) {
        return TYPE_CHAR;
    } else if (strcmp(in, "charz") == 0) {
        return TYPE_STRING;
    } else if (strcmp(in, "iz") == 0) {
        return TYPE_BOOL;
    }
    return -1;
}
%}

type_regex        (numba|charz|char|iz)
identifier_regex  ([a-z_][a-zA-Z0-9_]*)
number_regex      ([0-9])+
string_regex      \"(\\.|[^"\\])*\"
log_op_regex      (\|\||\&\&)
bin_op_regex      (\||\&|\~)
count_op_regex    (\+|\-)
multi_op_regex    (\*|\/)
comparison_regex  (>|<|==|>=|<=|!=)

%%
shrek               {return STRUCT;}
lööp                {return WHILE;}
smortlööp           {return FOR;}
despite             {return IF;}
unless              {return ELSE;}
E                   {return CONST;}
gimme               {return RETURN;}
tru                 {return TRUE;}
trunt               {return FALSE;}
","                 {return COMMA;}
"."                 {return DOT;}
"="                 {return ASSIGN;}
"!"                 {return NOT;}
"?"                 {return QUESTION;}

{type_regex}		    {yylval.type = convert_to_type_enum(yytext); return TYPE;}
{identifier_regex}		{sscanf(yytext, "%s", yylval.identifier); return IDENTIFIER;}
{string_regex}          {
                        int length = yyleng;
                        if (length > sizeof(yylval.string_lit)) {
                            length = sizeof(yylval.string_lit) - 1;
                        }
                        memcpy(yylval.string_lit, yytext, length);
                        yylval.string_lit[length] = 0;
                        return STRING_LIT;
                        }
{number_regex}          {yylval.number_lit = atoi(yytext); return NUMBER_LIT;}
{log_op_regex}          {sscanf(yytext, "%s", yylval.log_op); return LOG_OP;}
{bin_op_regex}          {sscanf(yytext, "%s", yylval.bin_op); return BIN_OP;}
{count_op_regex}        {sscanf(yytext, "%s", yylval.math_op); return COUNT_OP;}
{multi_op_regex}        {sscanf(yytext, "%s", yylval.math_op); return MULTI_OP;}
{comparison_regex}      {sscanf(yytext, "%s", yylval.comparison); return COMPARISON;}

"{"      		    {yylval.bracket = yytext[0]; return B_L_CURLY;}
"}"      		    {yylval.bracket = yytext[0]; return B_R_CURLY;}
"["      		    {yylval.bracket = yytext[0]; return B_L_SQUARE;}
"]"      		    {yylval.bracket = yytext[0]; return B_R_SQUARE;}
"("      		    {yylval.parenthesis = yytext[0]; return PAREN_L;}
")"      		    {yylval.parenthesis = yytext[0]; return PAREN_R;}
";"      		    {return SEMICOLON;}
":"                 {return COLON;}
" "      		    ;
"\t"                ;
"\r"                ;
"\n"                ;
.      		        {showError(yytext); return OTHER;}

%%

void showError(const char* token){
    printf("Unexpected character or characters: %s\n", token);
}
