#pragma once
#include <iostream>
#include <string>
typedef enum {
    SYN_NOT_NULL = 1,
    SYN_NULL,
    LEX_ID, 
    LEX_TYPE,
    LEX_INT,
    LEX_FLOAT,
    LEX_CHAR,
    LEX_BOOL,
    LEX_PTR,
    LEX_STRING,
    LEX_OTHER
} NodeType;
typedef struct TreeNode Node;
struct TreeNode {
public:
    // Node name
    std::string name;
    // Node type, define it is whether a leaf node or internal node
    NodeType nodeType;
    // program line num of this node
    int linenum;
    union {
        char strVal[32];// used for id, type, idptr
        int intVal; // intVal of this node
        float floatVal; // floatVal of this node
        char charVal;// charVal of this node
        bool boolVal;// boolVal of this node
        struct { int size; char val[64]; } glbStr; // const char* value
    };
    // number of children
    int childNum;
    // children ptrs
    struct TreeNode** children;
    // construction function
    TreeNode(std::string name, NodeType nodeType, int linenum, int childNum, TreeNode** children) {
        this->name = name;
        this->nodeType = nodeType;
        this->linenum = linenum;
        this->childNum = childNum;
        this->children = children;
        bool nullNode = true;
        for (int i = 0; i < this->childNum; i++)
            if ((this->children)[i]->nodeType != SYN_NULL)
                nullNode = false;
        if (nodeType == SYN_NOT_NULL && nullNode)
            this->nodeType = SYN_NULL;
    }
};

Node* createNode(std::string name, NodeType nodeType, int linenum, int childNum, Node** children);
// void printTree(Node* root, int depth);  