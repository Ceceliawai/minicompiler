%locations
%error-verbose
%{
    #include <stdarg.h>
    #include "lexical.cpp"
    Node* root = NULL;
    Node** package(int childNum, Node* child1, ...);
    void yyerror(const char* msg);
    int synError = 0;
%}

%token INT FLOAT CHAR BOOL STRING VOID ID SEMI COMMA ASSIGNOP RELOP 
%token PLUS MINUS STAR DIV MOD AND OR DOT NOT TYPE LP RP LB RB LC RC STRUCT RETURN IF ELSE WHILE
%token REASSIGNOP PTR FOR BREAK CONTINUE SIZEOF TYPEPTR SCANF PRINTF

%right ASSIGNOP REASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV MOD
%right NOT SIZEOF
%left LP RP LB RB DOT

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
%output "syntax.cpp"

%%
Program : ExtDefList                            { $$ = createNode("Program", SYN_NOT_NULL, @$.first_line, 
                                                  1, package(1, $1));
                                                  root = $$; }
    ;
ExtDefList : ExtDef ExtDefList                  { $$ = createNode("ExtDefList", SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | /* empty */                               { $$ = createNode("ExtDefList", SYN_NULL, @$.first_line
                                                  , 0, NULL);}
    ;
ExtDef : Specifier ExtDecList SEMI              { $$ = createNode("ExtDef", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Specifier SEMI                            { $$ = createNode("ExtDef", SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | Specifier FunDec SEMI                     { $$ = createNode("ExtDef", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Specifier FunDec CompSt                   { $$ = createNode("ExtDef", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Specifier error SEMI                      { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | error SEMI                                { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Specifier error                           { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    ;
// modified to support global assignment
ExtDecList : ExtDec                             { $$ = createNode("ExtDecList", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | ExtDec COMMA ExtDecList                   { $$ = createNode("ExtDecList", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | ExtDec error COMMA ExtDecList             { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    ;
// newly added
ExtDec : VarDec					                { $$ = createNode("ExtDec", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | VarDec ASSIGNOP Exp			            { $$ = createNode("ExtDec", SYN_NOT_NULL, @$.first_line
						                          , 3, package(3, $1, $2, $3)); }
    | error ASSIGNOP Exp			            { $$ = createNode("Error", SYN_NULL, @$.first_line
						                          , 0, NULL); yyerrok; }
    ;
Specifier : TYPE                                { $$ = createNode("Specifier", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | TYPEPTR                                   { $$ = createNode("Specifier", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | StructSpecifier                           { $$ = createNode("Specifier", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    ;
StructSpecifier : STRUCT OptTag LC DefList RC   { $$ = createNode("StructSpecifier", SYN_NOT_NULL, @$.first_line
                                                  , 5, package(5, $1, $2, $3, $4, $5)); }
    | STRUCT Tag                                { $$ = createNode("StructSpecifier", SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | STRUCT error LC DefList RC                { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | STRUCT OptTag LC error RC                 { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | STRUCT OptTag LC error                    { $$ = createNode("Error", SYN_NULL, @$.first_line
                              	                    , 0, NULL); yyerrok; }
    | STRUCT error                              { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    ;
OptTag : ID                                     { $$ = createNode("OptTag", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | /* empty */                               { $$ = createNode("OptTag", SYN_NULL, @$.first_line
                                                  , 0, NULL); }
    ;
Tag : ID                                        { $$ = createNode("Tag", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    ;

VarDec : ID                                     { $$ = createNode("VarDec", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | VarDec LB INT RB                          { $$ = createNode("VarDec", SYN_NOT_NULL, @$.first_line
                                                  , 4, package(4, $1, $2, $3, $4));  }
    | VarDec LB ID RB                           { $$ = createNode("VarDec", SYN_NOT_NULL, @$.first_line
                                                  , 4, package(4, $1, $2, $3, $4)); }
    | VarDec LB error RB                        { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | VarDec LB error                           { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    ;
Scanf : SCANF LP STRING COMMA Exp RP SEMI       { $$ = createNode("Scanf", SYN_NOT_NULL, @$.first_line
                                                  , 7, package(7, $1, $2, $3, $4, $5, $6, $7)); }
    ;
Printf : PRINTF LP STRING COMMA Exp RP SEMI     { $$ = createNode("Printf", SYN_NOT_NULL, @$.first_line
                                                  , 7, package(7, $1, $2, $3, $4, $5, $6, $7)); }
    | PRINTF LP STRING RP SEMI                  { $$ = createNode("Printf", SYN_NOT_NULL, @$.first_line
                                                  , 5, package(5, $1, $2, $3, $4, $5)); }
    ;
FunDec : ID LP VarList RP                       { $$ = createNode("FunDec", SYN_NOT_NULL, @$.first_line
                                                  , 4, package(4, $1, $2, $3, $4)); }
    | ID LP RP                                  { $$ = createNode("FunDec", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | ID LP error RP                            { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | ID LP error                               { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    ;
VarList : ParamDec COMMA VarList                { $$ = createNode("VarList", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | ParamDec                                  { $$ = createNode("VarList", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    ;
ParamDec : Specifier VarDec                     { $$ = createNode("ParamDec", SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    ;

CompSt : LC StmtList RC                         { $$ = createNode("CompSt", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    ;
StmtList : Stmt StmtList                        { $$ = createNode("StmtList", SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | Def StmtList                              { $$ = createNode("StmtList", SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | /* empty */                               { $$ = createNode("StmtList", SYN_NOT_NULL, @$.first_line
                                                  , 0, NULL); }
    ;
Stmt : Exp SEMI                                 { $$ = createNode("Stmt", SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | CompSt                                    { $$ = createNode("Stmt", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | Scanf                                     { $$ = createNode("Stmt", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | Printf                                    { $$ = createNode("Stmt", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | RETURN Exp SEMI                           { $$ = createNode("Stmt", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE   { $$ = createNode("Stmt", SYN_NOT_NULL, @$.first_line
                                                  , 5, package(5, $1, $2, $3, $4, $5)); }
    | IF LP Exp RP Stmt ELSE Stmt               { $$ = createNode("Stmt", SYN_NOT_NULL, @$.first_line
                                                  , 7, package(7, $1, $2, $3, $4, $5, $6, $7)); }
    | WHILE LP Exp RP Stmt                      { $$ = createNode("Stmt", SYN_NOT_NULL, @$.first_line
                                                  , 5, package(5, $1, $2, $3, $4, $5)); }
    | FOR LP Exp SEMI Exp SEMI Exp RP CompSt    { $$ = createNode("Stmt", SYN_NOT_NULL, @$.first_line
                                                  , 9, package(9, $1, $2, $3, $4, $5, $6, $7, $8, $9)); }
    | BREAK SEMI                                { $$ = createNode("Stmt", SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | CONTINUE SEMI                             { $$ = createNode("Stmt", SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | error SEMI                                { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | IF LP error RP Stmt %prec LOWER_THAN_ELSE { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | IF LP Exp RP error ELSE Stmt              { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | IF LP error RP ELSE Stmt                  { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | error LP Exp RP Stmt                      { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    ;
DefList : Def DefList                           { $$ = createNode("DefList", SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | /* empty */                               { $$ = createNode("Stmt", SYN_NULL, @$.first_line
                                                  , 0, NULL); }
    ;
Def : Specifier DecList SEMI                    { $$ = createNode("Def", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    ;
DecList : Dec                                   { $$ = createNode("DecList", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | Dec COMMA DecList                         { $$ = createNode("DecList", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Dec error DecList                         { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    ;
Dec : VarDec                                    { $$ = createNode("Dec", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | VarDec ASSIGNOP Exp                       { $$ = createNode("Dec", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | error ASSIGNOP Exp                        { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    ;
Exp : Exp ASSIGNOP Exp                          { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp REASSIGNOP Exp                        { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp AND Exp                               { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp OR Exp                                { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp RELOP Exp                             { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp PLUS Exp                              { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp MINUS Exp                             { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp STAR Exp                              { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp DIV Exp                               { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp MOD Exp                               { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | SIZEOF LP Exp RP                          { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 4, package(4, $1, $2, $3, $4));}
    | SIZEOF LP TYPE RP                         { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 4, package(4, $1, $2, $3, $4));}
    | LP Exp RP                                 { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | MINUS Exp                                 { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | NOT Exp                                   { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | ID LP Args RP                             { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 4, package(4, $1, $2, $3, $4)); }
    | ID LP RP                                  { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp LB Exp RB                             { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 4, package(4, $1, $2, $3, $4)); }
    | Exp DOT ID                                { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | LP TYPE RP Exp                            { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 4, package(4, $1, $2, $3, $4)); }
    | ID                                        { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | PTR ID                                    { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | INT                                       { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | FLOAT                                     { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | CHAR                                      { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | BOOL                                      { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | STRING                                    { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | VOID                                      { $$ = createNode("Exp", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | Exp ASSIGNOP error                        { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Exp REASSIGNOP error                      { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Exp AND error                             { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Exp OR error                              { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Exp RELOP error                           { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Exp PLUS error                            { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Exp MINUS error                           { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Exp STAR error                            { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Exp DIV error                             { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Exp MOD error                             { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }                                              
    | ID LP error RP                            { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Exp LB error RB                           { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | LP error RP Exp                           { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | LP TYPE RP error                          { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | LP error RP error                         { $$ = createNode("Error", SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    ;
Args : Exp COMMA Args                           { $$ = createNode("Args", SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp                                       { $$ = createNode("Args", SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    ;
%%
Node** package(int childNum, Node* child1, ...) {
    va_list ap;
    va_start(ap, child1);
    Node** res = new Node* [childNum];
    res[0] = child1;
    for (int i = 1; i < childNum; i++)
        res[i] = va_arg(ap, Node*);
    return res;
}

void yyerror(const char* msg) { 
    synError++;
    printf("Error type B at Line %d: %s\n", yylineno, msg);
}