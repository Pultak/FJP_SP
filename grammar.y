%{
#include <cstdio>
#include <iostream>

extern "C" int yylex();
int yyerror(const char *s);

extern FILE *yyin, *yyout;

#define YY_NO_UNISTD_H

%}

%union{
    char buffer[200];
}

%%

test:
    value COMPARISON value {
        printf("Hello world!");
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

