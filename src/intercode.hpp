#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "hash.hpp"
#include "semantic.hpp"

enum Operand_kind {
  VARIABLE_OP,
  TEMP_OP,
  // CONSTANT_OP,
  LABEL_OP,
  FUNCTION_OP,
  GET_ADDR_OP,
  GET_VAR_OP,
  INT_CONSTANT_OP,
  CHAR_CONSTANT_OP,
  FLOAT_CONSTANT_OP,
  BOOL_CONSTANT_OP,
  STRING_OP  // 用于print的输出
};

enum Intercode_kind {
  LABEL_IR,
  FUNCTION_IR,
  ASSIGN_IR,
  PLUS_IR,
  SUB_IR,
  MUL_IR,
  DIV_IR,
  MOD_IR,
  GOTO_IR,
  IF_IR,
  RELOP_IR,
  RETURN_IR,
  AGR_IR,
  EXP_IR,
  CALL_IR,
  PARAM_IR,
  SCANF_IR,
  PRINTF_IR,
  NULL_IR,
  GET_ADDR_IR,
  GET_VAR_IR,
  POINTER_ASSIGN_IR,
  DEC_IR,
  DEC_ARRAY_IR,
  // DEC_STRUCT_IR,
  ARRAY_ASSIGN_IR,
  // STRUCT_ASSIGN_IR,
  SIZEOF_IR,
  REASSIGN_IR,
  MINUS_IR,
  AND_IR,
  OR_IR,
  START_STRUCT_IR,
  END_STRUCT_IR,
  DEC_STRUCT_IR
};

class Operand_;
typedef Operand_* Operand;
class InterCode_;
typedef InterCode_* InterCode;

class Operand_ {
 public:
  Operand_kind kind;
  union {
    int no;
    int intvalue;
    float floatvalue;
    char charvalue;
    bool boolvalue;
    Operand opr;
  };
  std::string name;
  Type type;

  Operand_(){};
};

class InterCode_ {
 public:
  Intercode_kind kind;
  std::vector<Operand> ops;

  union {
    int size;
    char op[3];
    InterCode exp;  // 判断条件
  };

  InterCode pre;
  InterCode next;   // next
  InterCode left;   // true
  InterCode right;  // false
  InterCode_() {
    pre = nullptr;
    next = nullptr;
    left = nullptr;
    right = nullptr;
  }
};

void initInterCodes();
void insertInterCode(InterCode codes_pre, InterCode codes_lat);
void printInterCodes();
std::string generateIntercodeString(InterCode code);
std::string printOperand(Operand op);

InterCode getNullInterCode();

void translateProgram(Node* root);
InterCode translateExtDefList(Node* root);
InterCode translateExtDef(Node* root);
InterCode translateCompSt(Node* root, std::string funcName);
InterCode translateStmtList(Node* root);
InterCode translateDefList(Node* root, IDType type);
InterCode translateDef(Node* root, IDType type);
FieldList translateDecList(Node* root, Type type, IDType idtype, InterCode code);
FieldList translateDec(Node* root, Type type, IDType idtype, InterCode code);
InterCode translateStmt(Node* root);
InterCode translateCond(Node* root, Operand labelTrue, Operand labelFalse);
InterCode translateExp(Node* root);
InterCode translateArgs(Node* root);
InterCode translateScanf(Node* root);
InterCode translatePrintf(Node* root);