//
// Created by Charles Wang on 2023/5/20.
//

#ifndef IRINTERPRETER_INTERPRET_H
#define IRINTERPRETER_INTERPRET_H
#include <bits/stdc++.h>
#include <programCall.h>
using namespace std;
enum SymbolType {
    FUNCTION,
    LABEL,
    VAR
};
enum DataType {
    INT_TYPE,
    FLOAT_TYPE,
    CHAR_TYPE,
    BOOL_TYPE,
    PTR_INT_TYPE,
    PTR_FLOAT_TYPE,
    PTR_CHAR_TYPE,
    ARRAY
};
class Array_;
class Var_;
class Function_;
class ArgList_;
class Symbol_;
class Program_;
class VarList_;
class FunctionState_;
typedef Array_* Array;
typedef Var_* Var;
typedef Function_* Function;
typedef ArgList_* ArgList;
typedef Symbol_* Symbol;
typedef Program_* Program;
typedef VarList_* VarList;
typedef FunctionState_* FunctionState;

class Array_ {
public:
    DataType type;
    vector<int> size;
    int* iarr;
    char* carr;
    float* farr;
    Array_(const Array_ &other) : type(other.type), size(other.size) {
        int len = 1;
        for (auto it: size) len *= len;
        if (other.type == INT_TYPE) iarr = other.iarr;
        else if (other.type == CHAR_TYPE) carr = other.carr;
        else if (other.type == FLOAT_TYPE) farr = other.farr;
    }
    Array_(){iarr = nullptr; carr = nullptr; farr = nullptr;}
};
class Var_ {
public:
    string name;
    DataType type;
    int size;
    union {
        int iVal;
        float fVal;
        char cVal;
        bool bVal;
        void* ptr;
        Array arr;
    };
    Var_(const Var_& other) : name(other.name), type(other.type), size(other.size) {
        switch (type) {
            case INT_TYPE:
                iVal = other.iVal;
                break;
            case CHAR_TYPE:
                cVal = other.cVal;
                break;
            case FLOAT_TYPE:
                fVal = other.fVal;
                break;
            case PTR_INT_TYPE: {
                int *tmp = new int[size];
                memcpy(tmp, other.ptr, sizeof(int) * size);
                ptr = (void *) tmp;
                break;
            }
            case PTR_CHAR_TYPE: {
                char* tmp = new char[size];
                memcpy(tmp, other.ptr, sizeof(char)*size);
                ptr = (void*) tmp;
                break;
            }
            case PTR_FLOAT_TYPE: {
                float* tmp = new float[size];
                memcpy(tmp, other.ptr, sizeof(float)*size);
                ptr = (void*) tmp;
                break;
            }
            case ARRAY:
                arr = new Array_(*other.arr);
                break;
        }
    }
    Var_(){}
};
class ArgList_ {
public:
    bool isPassing;
    Var var;
    ArgList nextVar;
    ArgList_() {var = nullptr; nextVar = nullptr;}
};
class Function_ {
public:
    string name;
    ArgList head;
    Function_() {head = nullptr;}
};
class Symbol_ {
public:
    SymbolType type;
    int lineno;
    union {
        Function func;
        int id;
        Var var;
    };

};
class Program_ {
public:
    int PC;
    vector<string> rawProgram;
    Program_(): PC(0) {}
};
class VarList_ {
public:
    Var val;
    VarList nextVar;
public:
    VarList_() : nextVar(nullptr), val(nullptr) {}
};
class FunctionState_ {
public:
    int PC;
    string funcName;
    VarList head;
    Var valToReturn;
    int index;
    FunctionState_(string name, int PC, VarList head) {
        this->funcName = std::move(name);
        this->PC = PC;
        this->head = head;
        this->valToReturn = nullptr;
        this->index = -1;
    }
    FunctionState_(string name, int PC, VarList head, Var val, int index) {
        this->funcName = std::move(name);
        this->PC = PC;
        this->head = head;
        this->valToReturn = val;
        this->index = index;
    }
    FunctionState_() {
        PC = 0;
        funcName = "";
        head = nullptr;
        index = 0;
    }
};

VarList curFuncVarList = nullptr, curFuncVarHead = nullptr;
unordered_map<string, Symbol> symbolTable;
stack<FunctionState> funcStates; /*NOLINT*/
#endif //IRINTERPRETER_INTERPRET_H
