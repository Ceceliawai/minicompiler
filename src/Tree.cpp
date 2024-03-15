#include "Tree.hpp"
Node* createNode(std::string name, NodeType nodeType, int linenum, int childNum, Node** children) {
    return new Node(name, nodeType, linenum, childNum, children);
}
void printTree(Node* root, int depth) {
    if (root->nodeType != SYN_NULL)
        for (int i = 0; i < depth; i++)
            std::cout << "  ";
    switch (root->nodeType) {
        case SYN_NOT_NULL:
            std::cout << root->name << ' ' << '(' << root->linenum << ')' << std::endl;
            break;
        case SYN_NULL:
            break;
        case LEX_ID:
        case LEX_PTR:
        case LEX_TYPE:
            std::cout << root->name << ": " << root->strVal << std::endl;
            break;
        case LEX_INT:
            std::cout << root->name << ": " << root->intVal << std::endl;
            break;
        case LEX_FLOAT:
            std::cout << root->name << ": " << root->floatVal << std::endl;
            break;
        case LEX_CHAR:
            std::cout << root->name << ": '" << root->charVal << '\'' << std::endl;
            break;
        case LEX_BOOL:
            std::cout << root->name << ": " << root->boolVal << std::endl;
            break;
        case LEX_STRING:
            std::cout << root->name << ": \"" << root->glbStr.val << "\", size=" << root->glbStr.size << std::endl;
            break;
        case LEX_OTHER:
            std::cout << root->name << std::endl;
            break;
        Node** res = new Node* [10];
    }
    for (int i = 0; i < root->childNum; i++) 
        printTree(root->children[i], depth+1);
}
