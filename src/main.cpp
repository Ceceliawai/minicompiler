#include "Tree.hpp"
#include "intercode.hpp"
#include "semantic.hpp"

// #include "semantic.hpp"
extern int yyrestart(FILE* f);
extern int yyparse();
extern void printTree(Node* root, int depth);
extern Node* root;
extern int lexError;
extern int synError;

int main(int argc, char** argv) {
  if (argc <= 1) return 1;
  FILE* f = fopen(argv[1], "r");
  if (!f) {
    perror(argv[1]);
    return 1;
  }
  yyrestart(f);
  yyparse();
  if (root != NULL && lexError == 0 && synError == 0) {
    semantic(root);
//        printTree(root, 0);
    translateProgram(root);
  }
  return 0;
}