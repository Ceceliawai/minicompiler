#pragma once

#include <bits/stdc++.h>

#include <utility>

#include "Tree.hpp"

class ptrDim_;
class Type_;
class FieldList_;
class Structure_;
class Function_;
class Entry_;
typedef ptrDim_* ptrDim;
typedef ptrDim_* arrayDim;
typedef Type_* Type;
typedef FieldList_* FieldList;
typedef Structure_* Structure;
typedef Function_* Function;
typedef Entry_* Entry;

// 数据类型
enum DataType {
    INT_TYPE = 0,
    FLOAT_TYPE,
    CHAR_TYPE,
    BOOL_TYPE,
    VOID_TYPE,
    STRING_TYPE // should not be used externally!!
};

enum PtrDataType {
    PTR_INT_TYPE,
    PTR_FLOAT_TYPE,
    PTR_CHAR_TYPE
};

enum Kind {
    BASIC,
    ARRAY,
    STRUCT,
    STRUCT_DEF,
    FUNC,
    PTR
};

enum IDType {
    VAR,
    FIELD
};

// 指针维度
class ptrDim_ {
public: 
    int curDimSize;
    ptrDim nextDim;
    ptrDim_(): curDimSize(0), nextDim(nullptr) {}
};

// 类型类
class Type_ {
public:
    Kind kind;
    union {
        // basic datatype
        DataType basic;
        // array, with datatype of elem and size
        struct {
            Type elem;
            int size;
            char name[32];
        } array;
        struct {
            // not decided yet
            PtrDataType datatype;
            // void* addr; //应该加上，但是怎样获取数组地址？
            //这里的head是从右往左边存的，eg. a[3][4][5], head=5->next=4->next=3->next=nullptr
            ptrDim head = nullptr;
        } ptr;
        Structure structure;
        Function function;
    };
    Type_(){}
};

// 域列表
class FieldList_ {
public:
    // field name
    std::string name;
    Type type;
    FieldList next;
    FieldList_() :  type(nullptr), next(nullptr){}
};

// 结构体
class Structure_ {
public:
    std::string name;
    FieldList head;
    Structure_(): head(nullptr){}
};

// 函数类定义
class Function_ {
 public:
  // function name
  std::string name;
  // datatype of return value
  Type returnType;
  // num of parameters
  int paraNum;
  // param linked list
  FieldList head;
  // defined before?
  bool hasDefined;
  // line num
  int lineno;

  Function_() : hasDefined(false), returnType(nullptr), head(nullptr) {}
  Function_(std::string name, int lineno) {
    this->name = std::move(name);
    this->paraNum = 0;
    this->lineno = lineno;
  }
};

// entry definition
class Entry_ {
 public:
  std::string name;
  int rename;
  Type type;
  // suppose that the hash table is listed vertically
  //  same slot, next item, horizontally
  Entry hashNext;
  // same layer, next item, vertically
  Entry layerNext;
  Entry_() : hashNext(nullptr), layerNext(nullptr), type(nullptr) {}
};

// semantic analysis function decs
void semantic(Node* root);
void Program(Node* root);
void check();


void ExtDefList(Node* root);
void ExtDef(Node* root);
void ExtDecList(Node* root, Type type);
void ExtDec(Node* root, Type type, IDType class_);
Type Specifier(Node* root);
Type StructSpecifier(Node* root);
Type StructSpecifier_(Node* root);
FieldList VarDec(Node* root, Type type, IDType class_);
void Scanf(Node* root);
void Printf(Node* root);
Function FunDec(Node* root);
FieldList VarList(Node* root);
FieldList ParamDec(Node* root);
void CompSt(Node* root, const std::string& funName, Type paraType);
void StmtList(Node* root, Type reType);
void Stmt(Node* root, Type reType);
FieldList DefList(Node* root, IDType class_);
FieldList Def(Node* root, IDType class_);
FieldList DecList(Node* root, Type type, IDType class_);
FieldList Dec(Node* root, Type type, IDType class_);
Type Exp(Node* root);
FieldList Args(Node* root);
void printArgs(FieldList head);
void printType(Type type);