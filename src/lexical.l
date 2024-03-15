%option yylineno
%{
    #include <stdlib.h>
    #include <iostream>
    #include <ctype.h>
    #include <string>
    #include "Tree.hpp"
    #define YYSTYPE Node*
    #include "syntax.hpp"
    int yycolumn = 1;
    #define YY_USER_ACTION \
        yylloc.first_line = yylloc.last_line = yylineno; \
        yylloc.first_column = yycolumn; \
        yylloc.last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;
    int lexError = 0;
    int charToi(char ch);
    char escape2char(char ch);
    bool str2bool(char* text);
    int hexstrToi(char* text);
    int octstrToi(char* text);
%}
%option outfile="lexical.cpp"
digit       [0-9]
letter      [_a-zA-Z]
BIN         (0b|0B)[01]+
OCT         0[0-7]+
DEC         0|[1-9]{digit}*
HEX         (0x|0X)[0-9a-fA-F]+
INT         {DEC}|{OCT}|{HEX}
FLOAT       (({digit}*\.{digit}+|{digit}+\.)[eE][+-]?{digit}+)|({digit}+\.{digit}+)
ID          {letter}+({digit}|{letter})*
RELOP       ">"|"<"|">="|"<="|"=="|"!="
REASSIGNOP  "+="|"-="|"*="|"/="|"%="
TYPE        "int"|"float"|"char"|"bool"|"void"
TYPEPTR     "int*"|"float*"|"char*"|"bool*"
SPACE       [ \t\r]+
CHAR1       "\'"\\."\'"
CHAR2       "\'"[^\\']"\'"
BOOL        "true"|"false"
STRING      "\""(\\.|[^"\\])*"\""

%%
"\n"        { yycolumn = 1; }
{SPACE}     { }
","         { yylval = createNode("COMMA", LEX_OTHER, yylineno, 0, NULL);
              return COMMA; }
"."         { yylval = createNode("DOT", LEX_OTHER, yylineno, 0, NULL);
              return DOT; }
";"         { yylval = createNode("SEMI", LEX_OTHER, yylineno, 0, NULL);
              return SEMI; }
"("         { yylval = createNode("LP", LEX_OTHER, yylineno, 0, NULL);
              return LP; }
")"         { yylval = createNode("RP", LEX_OTHER, yylineno, 0, NULL);
              return RP; }
"["         { yylval = createNode("LB", LEX_OTHER, yylineno, 0, NULL);
              return LB; }
"]"         { yylval = createNode("RB", LEX_OTHER, yylineno, 0, NULL);
              return RB; }
"{"         { yylval = createNode("LC", LEX_OTHER, yylineno, 0, NULL);
              return LC; }
"}"         { yylval = createNode("RC", LEX_OTHER, yylineno, 0, NULL);
              return RC; }
{RELOP}     { yylval = createNode("RELOP", LEX_OTHER, yylineno, 0, NULL);
              strcpy(yylval->strVal, yytext);
              return RELOP; }
"="         { yylval = createNode("ASSIGNOP", LEX_OTHER, yylineno, 0, NULL);
              return ASSIGNOP; }
"!"         { yylval = createNode("NOT", LEX_OTHER, yylineno, 0, NULL);
              return NOT; }
"&&"        { yylval = createNode("AND", LEX_OTHER, yylineno, 0, NULL);
              return AND; }
"||"        { yylval = createNode("OR", LEX_OTHER, yylineno, 0, NULL);
              return OR; }
{REASSIGNOP} { yylval = createNode("REASSIGNOP", LEX_OTHER, yylineno, 0, NULL);
               strcpy(yylval->strVal, yytext);
               return REASSIGNOP; }
"+"         { yylval = createNode("PLUS", LEX_OTHER, yylineno, 0, NULL);
              return PLUS; }
"-"         { yylval = createNode("MINUS", LEX_OTHER, yylineno, 0, NULL);
              return MINUS; }
"*"         { yylval = createNode("STAR", LEX_OTHER, yylineno, 0, NULL);
              return STAR; }
"/"         { yylval = createNode("DIV", LEX_OTHER, yylineno, 0, NULL);
              return DIV; }
"%"         { yylval = createNode("MOD", LEX_OTHER, yylineno, 0, NULL);
              return MOD;}
"struct"    { yylval = createNode("STRUCT", LEX_OTHER, yylineno, 0, NULL);
              return STRUCT; }
"return"    { yylval = createNode("RETURN", LEX_OTHER, yylineno, 0, NULL);
              return RETURN; }
"if"        { yylval = createNode("IF", LEX_OTHER, yylineno, 0, NULL); 
              return IF; }
"else"      { yylval = createNode("ELSE", LEX_OTHER, yylineno, 0, NULL);
              return ELSE; }
"for"       { yylval = createNode("FOR", LEX_OTHER, yylineno, 0, NULL);
              return FOR; }
"while"     { yylval = createNode("WHILE", LEX_OTHER, yylineno, 0, NULL);
              return WHILE; }
"break"     { yylval = createNode("BREAK", LEX_OTHER, yylineno, 0, NULL);
              return BREAK; }
"continue"  { yylval = createNode("CONTINUE", LEX_OTHER, yylineno, 0, NULL);
              return CONTINUE; }
"sizeof"    { yylval = createNode("SIZEOF", LEX_OTHER, yylineno, 0, NULL);
              return SIZEOF; }
"ptr"       { yylval = createNode("PTR", LEX_OTHER, yylineno, 0, NULL);
              return PTR; }
"scanf"     { yylval = createNode("SCANF", LEX_OTHER, yylineno, 0, NULL);
              return SCANF; }
"printf"    { yylval = createNode("PRINTF", LEX_OTHER, yylineno, 0, NULL);
              return PRINTF;}
{TYPE}      { yylval = createNode("TYPE", LEX_TYPE, yylineno, 0, NULL);
              strcpy(yylval->strVal, yytext);
              return TYPE; }
{TYPEPTR}   { yylval = createNode("TYPEPTR", LEX_PTR, yylineno, 0, NULL);
              strcpy(yylval->strVal, yytext);
              return TYPEPTR; }
{HEX}       { yylval = createNode("INT", LEX_INT, yylineno, 0, NULL);
              yylval->intVal = hexstrToi(yytext);
              return INT; }
{OCT}       { yylval = createNode("INT", LEX_INT, yylineno, 0, NULL);
              yylval->intVal = octstrToi(yytext);
              return INT; }
{DEC}       { yylval = createNode("INT", LEX_INT, yylineno, 0, NULL);
              yylval->intVal = atoi(yytext);
              return INT; }
{FLOAT}     { yylval = createNode("FLOAT", LEX_FLOAT, yylineno, 0, NULL);
              yylval->floatVal = atof(yytext);
              return FLOAT; }
{CHAR1}     { yylval = createNode("CHAR", LEX_CHAR, yylineno, 0, NULL);
              yylval->charVal = escape2char(yytext[2]); 
              return CHAR; }
{CHAR2}     { yylval = createNode("CHAR", LEX_CHAR, yylineno, 0, NULL);
              yylval->charVal = yytext[1];
              return CHAR; }
{BOOL}      { yylval = createNode("BOOL", LEX_BOOL, yylineno, 0, NULL);
              yylval->boolVal = str2bool(yytext);
              return BOOL; }
{STRING}    { yylval = createNode("STRING", LEX_STRING, yylineno, 0, NULL);
              std::string temp = "";
              for (int i = 1; i <= yyleng-2; i++) {
                if (yytext[i] == '\\') {
                    i++;
                    temp.push_back(escape2char(yytext[i]));
                } else {
                    temp.push_back(yytext[i]);
                }
              }
              yylval->glbStr.size = temp.size();
              strcpy(yylval->glbStr.val, temp.c_str());
              return STRING; }
{ID}        { yylval = createNode("ID", LEX_ID, yylineno, 0, NULL);
              strcpy(yylval->strVal, yytext);
              return ID; }
"//"        { char c = yyinput(); 
              while (c != '\n') c = yyinput(); }
"/*"        { char a = yyinput(); char b = yyinput();
              while (!(a == '*' && b == '/')) { a = b; b = yyinput(); } }
.           { printf("Error type A at Line %d: Mysterious characters \'%s\'\n", yylineno, yytext); 
              lexError++; }
%%
int charToi(char ch) {   
    if(isdigit(ch))
        return ch - 48;
    if( ch < 'A' || (ch > 'F' && ch < 'a') || ch > 'z' )
        return -1;
    if(isalpha(ch))
        return isupper(ch) ? ch - 55 : ch - 87;
    return -1;
}

int hexstrToi(char* text) {
    int len = strlen(text);
    int res = 0;
    for (int i = 2; i < len; i++) {
        res *= 16;
        res += charToi(text[i]);
    }
    return res;
}

int octstrToi(char* text) {
    int len = strlen(text);
    int res = 0;
    for (int i = 0; i < len; i++) {
        res *= 8;
        res += charToi(text[i]);
    }
    return res;
}
char escape2char(char ch){
	switch(ch){
	case 'a': return '\a';
	case 'b': return '\b';
	case 'f': return '\f';
	case 'n': return '\n';
	case 'r': return '\r';
	case 't': return '\t';
	case 'v': return '\v';
	case '\\': return '\\';
	case '\'': return '\'';
	case '\"': return '\"';
	default:
		if ('0'<=ch && ch<='9')
			return (char)(ch-'0');
		else
			return ch;
	}
}
bool str2bool(char* text){
    return strcmp(text, "true") == 0;
}
int yywrap() {
    return 1;
}
