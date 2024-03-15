#include "intercode.hpp"

extern std::unordered_map<std::string, Entry> symbolTable;
extern Entry layersHead;

InterCode interCodes;

int tmpVarNo = 1;
int labelNo = 1;
int renameNo = 0;
std::vector<Operand> before;
std::vector<Operand> after;
bool duplicate_check = true;

void initInterCodes() {
  symbolTable.clear();
  initLayers();
}

InterCode getNullInterCode() {
  InterCode code = new InterCode_;
  code->kind = NULL_IR;
  return code;
}

InterCode findLastInterCode(InterCode codes) {
  if (codes == nullptr) return nullptr;
  //  if (codes->kind == NULL_IR) return codes;
  InterCode tmp = new InterCode_;
  tmp = codes;
  while (tmp->next != nullptr) {
    tmp = tmp->next;
  }
  return tmp;
}

void insertInterCode(InterCode codes_pre, InterCode codes_lat) {
  if (codes_pre == nullptr || codes_lat == nullptr) {
    std::cout << "Insert Code Error" << std::endl;
    return;
  }
  InterCode tail = new InterCode_;
  tail = findLastInterCode(codes_pre);
  if (tail->next == nullptr) {
    if (codes_lat->pre == nullptr) {
      codes_lat->pre = tail;
      tail->next = codes_lat;
      return;
    }
  }
}

Operand newTemp() {
  Operand tmpVar = new Operand_;
  tmpVar->kind = TEMP_OP;
  tmpVar->no = tmpVarNo;
  tmpVarNo++;
  return tmpVar;
}

Operand newLabel() {
  Operand label = new Operand_;
  label->kind = LABEL_OP;
  label->no = labelNo;
  labelNo++;
  return label;
}

Operand getVar(std::string name, Type type) {
  Operand var = new Operand_;
  var->kind = VARIABLE_OP;
  var->name = name;
  var->type = type;
  return var;
}

Operand getFunc(std::string name, Type type) {
  Operand func = new Operand_;
  func->kind = FUNCTION_OP;
  func->name = name;
  func->type = type;
  return func;
}

Operand getAddr(Operand op) {
  Operand addr = new Operand_;
  addr->kind = GET_ADDR_OP;
  addr->opr = op;
  return addr;
}

int getSize(Type type) {
  if (type->kind == BASIC && type->basic == INT_TYPE)
    return 4;
  else if (type->kind == BASIC && type->basic == CHAR_TYPE)
    return 4;
  else if (type->kind == BASIC && type->basic == FLOAT_TYPE)
    return 8;
  else if (type->kind == BASIC && type->basic == BOOL_TYPE)
    return 1;
  else if (type->kind == BASIC && type->basic == VOID_TYPE)
    return 0;
  else if (type->kind == BASIC && type->basic == STRING_TYPE)
    return 0;  // 这里应该不太对，但我还不知道应该怎么写
  else if (type->kind == ARRAY)
    return type->array.size * getSize(type->array.elem);
  else if (type->kind == STRUCT) {
    FieldList head = type->structure->head;
    int sum = 0;
    while (head != nullptr) {
      int tmp = getSize(head->type);
      if (tmp % 4 != 0) tmp = ((tmp / 4) + 1) * 4;
      sum += tmp;
      head = head->next;
    }
  }
}

void translateProgram(Node* root) {
  initInterCodes();
  duplicate_check = false;
  renameNo = 0;
  InterCode code = translateExtDefList(root->children[0]);
  duplicate_check = true;
  interCodes = code;
  printInterCodes();
}

InterCode translateExtDefList(Node* root) {
  if (root->childNum == 2) {
    InterCode code1 = translateExtDef(root->children[0]);
    InterCode code2 = translateExtDefList(root->children[1]);
    if (code2 != nullptr && code1 != nullptr) {
      insertInterCode(code1, code2);
      return code1;
    }
    if (code1 != nullptr)
      return code1;
    else if (code2 != nullptr)
      return code2;
  }
  return getNullInterCode();
}

InterCode translateExtDef(Node* root) {
  Type type = Specifier(root->children[0]);
  if (type == nullptr) return nullptr;
  if (type->kind == STRUCT && !type->structure->name.empty() && type->structure->head != nullptr) {
    Entry res = new Entry_;
    res->name = type->structure->name;
    res->type = new Type_;
    res->type->structure = type->structure;
    res->type->kind = STRUCT_DEF;
    insertSymbol(res);

    if (root->children[0]->children[0]->children[1]->name == "OptTag") {
      InterCode code1 = new InterCode_;
      code1->kind = START_STRUCT_IR;
      Operand op1 = new Operand_;
      op1->kind = CHAR_CONSTANT_OP;
      op1->name = type->structure->name;
      op1->type = type;
      code1->ops.push_back(op1);
      FieldList temp = type->structure->head;
      while (temp != nullptr) {
        InterCode code = new InterCode_;
        code->kind = DEC_IR;
        Operand op = new Operand_;
        op->kind = VARIABLE_OP;
        op->name = temp->name;
        op->type = temp->type;
        code->ops.push_back(op);
        insertInterCode(code1, code);
        temp = temp->next;
      }
      InterCode code2 = new InterCode_;
      code2->kind = END_STRUCT_IR;
      insertInterCode(code1, code2);
      return code1;
    } else {
      InterCode code1 = new InterCode_;
      code1->kind = DEC_STRUCT_IR;
      Operand op1 = new Operand_;
      op1->kind = VARIABLE_OP;
      op1->name = type->structure->name;
      op1->type = type;
      code1->ops.push_back(op1);
      return code1;
    }
  }

  if (root->children[1]->name == "FunDec") {
    Function func = FunDec(root->children[1]);
    InterCode code1 = new InterCode_;
    code1->kind = FUNCTION_IR;
    // code1->op_first = getFunc(func->name);
    code1->ops.push_back(getFunc(func->name, type));
    FieldList head = func->head;
    if (head != nullptr) {
      InterCode code2 = new InterCode_;
      code2->kind = PARAM_IR;
      while (head != nullptr) {
        code2->ops.push_back(getVar(head->name + "_" + std::to_string(renameNo + 1), head->type));
        head = head->next;
      }
      insertInterCode(code1, code2);
    }
    func->returnType = type;
    func->hasDefined = 1;
    Type newtype = new Type_;
    newtype->kind = FUNC;
    newtype->function = func;
    Entry res = new Entry_;
    res->name = func->name;
    res->type = newtype;
    insertSymbol(res);
    if (root->children[2]->name != "SEMI") {
      pushLayer();
      renameNo++;
      InterCode code3 = translateCompSt(root->children[2], func->name);
      popLayer();
      if (code3 != nullptr) insertInterCode(code1, code3);
    }
    return code1;
  } else if (root->children[1]->name == "ExtDecList") {
    InterCode code1 = getNullInterCode();
    FieldList res = translateDecList(root->children[1], type, VAR, code1);

    if (res->type->kind == ARRAY) {
      InterCode code2 = new InterCode_;
      code2->kind = DEC_ARRAY_IR;
      Operand op1 = new Operand_;
      op1->kind = VARIABLE_OP;
      op1->type = type;
      Entry temp = findSymbolAll(res->name);
      op1->name = res->name + "_" + std::to_string(temp->rename);
      code2->ops.push_back(op1);
      Operand op2 = new Operand_;
      op2->kind = INT_CONSTANT_OP;
      op2->intvalue = res->type->array.size;
      code2->ops.push_back(op2);
      Type tmp = res->type->array.elem;
      while (tmp->kind == ARRAY) {
        Operand op = new Operand_;
        op->kind = INT_CONSTANT_OP;
        op->intvalue = tmp->array.size;
        code2->ops.push_back(op);
        tmp = tmp->array.elem;
      }
      insertInterCode(code2, code1);
      return code2;
    }

    InterCode code2 = new InterCode_;
    code2->kind = DEC_IR;
    Operand op1 = new Operand_;
    op1->kind = VARIABLE_OP;
    op1->type = type;
    Entry temp = findSymbolAll(res->name);
    op1->name = res->name + "_" + std::to_string(temp->rename);
    code2->ops.push_back(op1);
    // while(res != nullptr)
    // {
    //   InterCode code = new InterCode_;
    //   code->kind = EXP_IR;
    //   Operand op = new Operand_;

    //   op->kind = VARIABLE_OP;
    //   op->type = res->type;
    //   code->ops.push_back(op);
    //   insertInterCode(code2, code);
    //   res = res->next;
    // }
    insertInterCode(code2, code1);
    return code2;
  }
  return getNullInterCode();
}

InterCode translateCompSt(Node* root, std::string funcName) {
  if (funcName != "") {
    Entry sym = findSymbolFunc(funcName);
    FieldList parms = sym->type->function->head;
    while (parms != nullptr) {
      Entry parm = new Entry_;
      parm->name = parms->name;
      parm->type = parms->type;
      parm->rename = renameNo;
      insertSymbol(parm);
      parms = parms->next;
    }
  }
  InterCode code = translateStmtList(root->children[1]);
  return code;
}

InterCode translateStmtList(Node* root) {
  if (root->childNum == 2 && root->children[0]->name == "Stmt") {
    InterCode code1 = translateStmt(root->children[0]);
    InterCode code2 = translateStmtList(root->children[1]);
    if (code1 != nullptr && code2 != nullptr) insertInterCode(code1, code2);
    if (code1 != nullptr)
      return code1;
    else
      return code2;
  } else if (root->childNum == 2 && root->children[0]->name == "Def") {
    // 这个我没看到例子，不太懂是对应的哪种代码，应该也不会出现就是（？
    InterCode code1 = translateDef(root->children[0], VAR);  // 这个不知道对不对 need to fill
    InterCode code2 = translateStmtList(root->children[1]);
    if (code1 != nullptr && code2 != nullptr) insertInterCode(code1, code2);
    if (code1 != nullptr)
      return code1;
    else
      return code2;
  }
  return getNullInterCode();
}

InterCode translateDefList(Node* root, IDType type) {
  if (root->childNum == 0)
    return getNullInterCode();
  else {
    InterCode code1 = translateDef(root->children[0], type);
    InterCode code2 = translateDefList(root->children[1], type);
    insertInterCode(code1, code2);
    return code1;
  }
}

InterCode translateDef(Node* root, IDType type) {
  Type cur_type = Specifier(root->children[0]);
  InterCode code1 = getNullInterCode();
  FieldList res = translateDecList(root->children[1], cur_type, type, code1);
  FieldList curr = res;
  InterCode code_head = getNullInterCode();
  if (curr->type->kind != ARRAY && curr->type->kind != STRUCT) {
    while (curr != nullptr) {
      // if (curr->type->kind == ARRAY || curr->type->kind == STRUCT) {
      InterCode code2 = getNullInterCode();
      code2->kind = DEC_IR;
      // code2->op_first = getVar(curr->name);
      Entry temp = findSymbolAll(curr->name);
      Operand op = getVar(curr->name + "_" + std::to_string(temp->rename), curr->type);
      code2->ops.push_back(op);
      code2->size = getSize(curr->type);
      insertInterCode(code_head, code2);
      // }
      curr = curr->next;
    }
    insertInterCode(code_head, code1);
    return code_head;
  } else if (curr->type->kind == ARRAY) {
    std::vector<int> intindex;
    std::vector<std::string> strindex;
    std::vector<int> sizetype;
    enum { tintindex, tstrindex };

    if (curr->type->array.size == 0) {
      sizetype.push_back(tstrindex);
      std::string tmp = "";
      for (int i = 0; i < 32; i++) {
        if (curr->type->array.name[i] != '\0')
          tmp.push_back(curr->type->array.name[i]);
        else
          break;
      }
      strindex.push_back(tmp);
    } else {
      sizetype.push_back(tintindex);
      intindex.push_back(curr->type->array.size);
    }

    auto tmp = curr->type->array.elem;
    while (tmp->kind == ARRAY) {
      if (tmp->array.size == 0) {
        sizetype.push_back(tstrindex);
        std::string temp = "";
        for (int i = 0; i < 32; i++) {
          if (tmp->array.name[i] != '\0')
            temp.push_back(tmp->array.name[i]);
          else
            break;
        }
        strindex.push_back(temp);
      } else {
        sizetype.push_back(tintindex);
        intindex.push_back(tmp->array.size);
      }
      tmp = tmp->array.elem;
    }

    code1->kind = DEC_ARRAY_IR;
    Entry temp = findSymbolAll(curr->name);
    Operand op = getVar(curr->name + "_" + std::to_string(temp->rename), curr->type);
    op->type->basic = tmp->basic;
    code1->ops.push_back(op);

    int m = 0;
    int n = 0;
    for (int i = 0; i < sizetype.size(); i++) {
      if (sizetype[i] == tintindex) {
        Operand index = new Operand_;
        index->kind = INT_CONSTANT_OP;
        index->intvalue = intindex[m];
        m++;
        code1->ops.push_back(index);
      } else {
        Operand index = new Operand_;
        index->kind = VARIABLE_OP;
        Entry t = findSymbolAll(strindex[n]);
        index->name = strindex[n] + "_" + std::to_string(t->rename);
        n++;
        code1->ops.push_back(index);
      }
    }
    return code1;
  }
}

FieldList translateDecList(Node* root, Type type, IDType idtype, InterCode code) {
  FieldList res = translateDec(root->children[0], type, idtype, code);
  if (root->childNum == 3) {
    if (res == nullptr)
      res = translateDecList(root->children[2], type, idtype, code);
    else {
      FieldList tmp = res;
      //      while (tmp != nullptr) tmp = tmp->next; //我先把这行删了，不知道有啥影响
      tmp->next = translateDecList(root->children[2], type, idtype, code);
    }
  }
  return res;
}

FieldList translateDec(Node* root, Type type, IDType idtype, InterCode code) {
  FieldList res = VarDec(root->children[0], type, idtype);
  if (idtype == VAR && res != nullptr && root->childNum == 3) {
    InterCode newcode = getNullInterCode();
    newcode->kind = ASSIGN_IR;
    InterCode left = getNullInterCode();
    left->kind = EXP_IR;
    Operand op = new Operand_;
    Entry temp = findSymbolAll(res->name);
    op->name = temp->name + "_" + std::to_string(temp->rename);
    op->kind = VARIABLE_OP;
    op->type = res->type;
    left->ops.push_back(op);
    InterCode right = translateExp(root->children[2]);
    newcode->left = left;
    newcode->right = right;
    insertInterCode(code, newcode);
  }
  return res;
}

void turnoverRELOP(InterCode code) {
  if (code->kind == EXP_IR) {
    if (code->ops[0]->kind == BOOL_CONSTANT_OP && code->ops[0]->boolvalue == true) {
      code->ops[0]->boolvalue = false;
    } else if (code->ops[0]->kind == BOOL_CONSTANT_OP && code->ops[0]->boolvalue == false) {
      code->ops[0]->boolvalue = true;
    }
    return;
  }
  if (code->kind != RELOP_IR) {
    std::cout << "turn over false!" << std::endl;
    return;
  }
  int i = 0;
  while (i < 3) {
    if (code->op[i] == '<')
      code->op[i] = '>';
    else if (code->op[i] == '>')
      code->op[i] = '<';
    else if (code->op[i] == '!') {
      code->op[i] = '=';
      break;
    } else if (code->op[i] == '=' && code->op[i + 1] == '=') {
      code->op[i] = '!';
      break;
    } else if (code->op[i] == '=' && code->op[i + 1] != '=') {
      code->op[i] = '\0';
      break;
    } else if (code->op[i] == '\0') {
      code->op[i] = '=';
      code->op[i + 1] = '\0';
      break;
    }
    i = i + 1;
  }
}

InterCode translateStmt(Node* root) {
  if (root->children[0]->name == "Exp") {
    InterCode code = translateExp(root->children[0]);
    return code;
  } else if (root->children[0]->name == "CompSt") {
    pushLayer();
    renameNo++;
    InterCode code1 = translateCompSt(root->children[0], "");
    popLayer();
    return code1;
  } else if (root->children[0]->name == "Scanf") {
    return translateScanf(root->children[0]);
  } else if (root->children[0]->name == "Printf") {
    return translatePrintf(root->children[0]);
  } else if (root->children[0]->name == "RETURN") {
    InterCode code1 = translateExp(root->children[1]);
    InterCode code2 = new InterCode_;
    code2->kind = RETURN_IR;
    insertInterCode(code2, code1);
    return code2;
  } else if (root->children[0]->name == "IF" && root->childNum == 5) {
    InterCode code = getNullInterCode();
    code->kind = IF_IR;
    InterCode exp = translateExp(root->children[2]);
    if (exp->kind == AND_IR) {
      InterCode exp1 = exp->left;
      InterCode exp2 = exp->right;
      turnoverRELOP(exp1);
      turnoverRELOP(exp2);

      InterCode if1 = getNullInterCode();
      if1->kind = IF_IR;

      InterCode if2 = getNullInterCode();
      if2->kind = IF_IR;

      Operand labeln = newLabel();
      InterCode label_next = getNullInterCode();
      label_next->kind = LABEL_IR;
      label_next->ops.push_back(labeln);

      InterCode goto1 = getNullInterCode();
      goto1->kind = GOTO_IR;
      goto1->ops.push_back(labeln);

      InterCode goto2 = getNullInterCode();
      goto2->kind = GOTO_IR;
      goto2->ops.push_back(labeln);

      InterCode code_true = translateStmt(root->children[4]);

      insertInterCode(if1, exp1);
      insertInterCode(if1, goto1);
      insertInterCode(if1, if2);
      insertInterCode(if1, exp2);
      insertInterCode(if1, goto2);
      insertInterCode(if1, code_true);
      insertInterCode(if1, label_next);
      return if1;
    } else if (exp->kind == OR_IR) {
      InterCode exp1 = exp->left;
      InterCode exp2 = exp->right;

      InterCode if1 = getNullInterCode();
      if1->kind = IF_IR;

      InterCode if2 = getNullInterCode();
      if2->kind = IF_IR;

      Operand labelt = newLabel();
      InterCode label_true = getNullInterCode();
      label_true->kind = LABEL_IR;
      label_true->ops.push_back(labelt);

      Operand labeln = newLabel();
      InterCode label_next = getNullInterCode();
      label_next->kind = LABEL_IR;
      label_next->ops.push_back(labeln);

      InterCode goto1 = getNullInterCode();
      goto1->kind = GOTO_IR;
      goto1->ops.push_back(labelt);

      InterCode goto2 = getNullInterCode();
      goto2->kind = GOTO_IR;
      goto2->ops.push_back(labelt);

      InterCode goton = getNullInterCode();
      goton->kind = GOTO_IR;
      goton->ops.push_back(labeln);

      InterCode code_true = translateStmt(root->children[4]);

      insertInterCode(if1, exp1);
      insertInterCode(if1, goto1);
      insertInterCode(if1, if2);
      insertInterCode(if1, exp2);
      insertInterCode(if1, goto2);
      insertInterCode(if1, goton);
      insertInterCode(if1, label_true);
      insertInterCode(if1, code_true);
      insertInterCode(if1, label_next);
      return if1;
    } else {
      turnoverRELOP(exp);
      insertInterCode(code, exp);

      Operand label_next = newLabel();
      InterCode code_next = getNullInterCode();
      code_next->kind = LABEL_IR;
      code_next->ops.push_back(label_next);

      InterCode jump_after_if = getNullInterCode();
      jump_after_if->kind = GOTO_IR;
      jump_after_if->ops.push_back(label_next);

      InterCode code_true = translateStmt(root->children[4]);
      insertInterCode(code, jump_after_if);
      insertInterCode(code, code_true);
      insertInterCode(code, code_next);
      return code;
    }
  } else if (root->children[0]->name == "IF" && root->childNum == 7) {
    InterCode exp = translateExp(root->children[2]);
    if (exp->kind == AND_IR) {
      InterCode exp1 = exp->left;
      InterCode exp2 = exp->right;
      turnoverRELOP(exp1);
      turnoverRELOP(exp2);

      InterCode if1 = getNullInterCode();
      if1->kind = IF_IR;

      InterCode if2 = getNullInterCode();
      if2->kind = IF_IR;

      Operand labelf = newLabel();
      InterCode label_false = getNullInterCode();
      label_false->kind = LABEL_IR;
      label_false->ops.push_back(labelf);

      Operand labeln = newLabel();
      InterCode label_next = getNullInterCode();
      label_next->kind = LABEL_IR;
      label_next->ops.push_back(labeln);

      InterCode goto1 = getNullInterCode();
      goto1->kind = GOTO_IR;
      goto1->ops.push_back(labelf);

      InterCode goto2 = getNullInterCode();
      goto2->kind = GOTO_IR;
      goto2->ops.push_back(labelf);

      InterCode goton = getNullInterCode();
      goton->kind = GOTO_IR;
      goton->ops.push_back(labeln);

      InterCode code_true = translateStmt(root->children[4]);
      InterCode code_false = translateStmt(root->children[6]);

      insertInterCode(if1, exp1);
      insertInterCode(if1, goto1);
      insertInterCode(if1, if2);
      insertInterCode(if1, exp2);
      insertInterCode(if1, goto2);
      insertInterCode(if1, code_true);
      insertInterCode(if1, goton);
      insertInterCode(if1, label_false);
      insertInterCode(if1, code_false);
      insertInterCode(if1, label_next);
      return if1;
    } else if (exp->kind == OR_IR) {
      InterCode exp1 = exp->left;
      InterCode exp2 = exp->right;

      InterCode if1 = getNullInterCode();
      if1->kind = IF_IR;

      InterCode if2 = getNullInterCode();
      if2->kind = IF_IR;

      Operand labelt = newLabel();
      InterCode label_true = getNullInterCode();
      label_true->kind = LABEL_IR;
      label_true->ops.push_back(labelt);

      Operand labeln = newLabel();
      InterCode label_next = getNullInterCode();
      label_next->kind = LABEL_IR;
      label_next->ops.push_back(labeln);

      InterCode goto1 = getNullInterCode();
      goto1->kind = GOTO_IR;
      goto1->ops.push_back(labelt);

      InterCode goto2 = getNullInterCode();
      goto2->kind = GOTO_IR;
      goto2->ops.push_back(labelt);

      InterCode goton = getNullInterCode();
      goton->kind = GOTO_IR;
      goton->ops.push_back(labeln);

      InterCode code_true = translateStmt(root->children[4]);
      InterCode code_false = translateStmt(root->children[6]);

      insertInterCode(if1, exp1);
      insertInterCode(if1, goto1);
      insertInterCode(if1, if2);
      insertInterCode(if1, exp2);
      insertInterCode(if1, goto2);
      insertInterCode(if1, code_false);
      insertInterCode(if1, goton);
      insertInterCode(if1, label_true);
      insertInterCode(if1, code_true);
      insertInterCode(if1, label_next);
      return if1;
    } else {
      InterCode code = getNullInterCode();
      code->kind = IF_IR;
      turnoverRELOP(exp);
      insertInterCode(code, exp);

      Operand label_false = newLabel();
      InterCode labelf = getNullInterCode();
      labelf->kind = LABEL_IR;
      labelf->ops.push_back(label_false);

      // 原IF 后面的语句
      InterCode labeln = getNullInterCode();
      labeln->kind = LABEL_IR;
      Operand label_next = newLabel();
      labeln->ops.push_back(label_next);

      // 原IF true要执行的语句
      InterCode code_true = translateStmt(root->children[4]);
      // 原IF false要执行的语句
      InterCode code_false = translateStmt(root->children[6]);

      // true前面的那条和IF翻译在一行的GOTO
      InterCode GOTO = getNullInterCode();
      GOTO->kind = GOTO_IR;
      GOTO->ops.push_back(label_false);
      code->right = GOTO;

      // 给true后面加一个GOTO NEXT 即跳过false
      InterCode jump = getNullInterCode();
      jump->kind = GOTO_IR;
      jump->ops.push_back(label_next);

      insertInterCode(code, GOTO);
      insertInterCode(code, code_true);
      insertInterCode(code, jump);
      insertInterCode(code, labelf);
      insertInterCode(code, code_false);
      insertInterCode(code, labeln);
      return code;
    }
  } else if (root->children[0]->name == "WHILE") {
    InterCode code3 = translateExp(root->children[2]);
    if (code3->kind != AND_IR && code3->kind != OR_IR) {
      Operand label1 = newLabel();
      Operand label2 = newLabel();
      before.push_back(label1);
      after.push_back(label2);
      InterCode code1 = new InterCode_;
      code1->kind = LABEL_IR;
      code1->ops.push_back(label1);
      InterCode code2 = new InterCode_;
      code2->kind = IF_IR;
      insertInterCode(code1, code2);
      turnoverRELOP(code3);
      insertInterCode(code1, code3);
      InterCode code4 = new InterCode_;
      code4->kind = GOTO_IR;
      code4->ops.push_back(label2);
      insertInterCode(code1, code4);
      InterCode code5 = translateStmt(root->children[4]);
      insertInterCode(code1, code5);
      InterCode code6 = new InterCode_;
      code6->kind = GOTO_IR;
      code6->ops.push_back(label1);
      insertInterCode(code1, code6);
      InterCode code7 = new InterCode_;
      code7->kind = LABEL_IR;
      code7->ops.push_back(label2);
      insertInterCode(code1, code7);
      before.pop_back();
      after.pop_back();
      return code1;
    } else if (code3->kind == AND_IR) {
      std::vector<InterCode> exps;
      auto tmp = code3;
      while (tmp->kind == AND_IR) {
        InterCode exp = tmp->right;
        turnoverRELOP(exp);
        exps.push_back(exp);
        tmp = tmp->left;
      }
      InterCode exp = tmp;
      turnoverRELOP(exp);
      exps.push_back(exp);

      Operand labeln = newLabel();
      Operand labelloop = newLabel();

      InterCode code_next = getNullInterCode();
      code_next->kind = LABEL_IR;
      code_next->ops.push_back(labeln);

      InterCode code_loop = getNullInterCode();
      code_loop->kind = LABEL_IR;
      code_loop->ops.push_back(labelloop);

      before.push_back(labelloop);
      after.push_back(labeln);

      for (int i = 0; i < exps.size(); i++) {
        auto exp = exps[i];
        InterCode codeif = getNullInterCode();
        codeif->kind = IF_IR;
        insertInterCode(codeif, exp);
        InterCode codegoto = getNullInterCode();
        codegoto->ops.push_back(labeln);
        codegoto->kind = GOTO_IR;
        insertInterCode(codeif, codegoto);
        insertInterCode(code_loop, codeif);
      }

      InterCode codeture = translateStmt(root->children[4]);
      insertInterCode(code_loop, codeture);
      InterCode gotoloop = getNullInterCode();
      gotoloop->kind = GOTO_IR;
      gotoloop->ops.push_back(labelloop);
      insertInterCode(code_loop, gotoloop);
      insertInterCode(code_loop, code_next);
      before.pop_back();
      after.pop_back();
      return code_loop;
    } else if (code3->kind == OR_IR) {
      // 还没写完
      std::vector<InterCode> exps;
      auto tmp = code3;
      while (tmp->kind == AND_IR) {
        InterCode exp = tmp->right;
        // turnoverRELOP(exp);
        exps.push_back(exp);
        tmp = tmp->left;
      }
      InterCode exp = tmp;
      // turnoverRELOP(exp);
      exps.push_back(exp);

      Operand labeln = newLabel();
      Operand labelt = newLabel();
      Operand labelloop = newLabel();

      InterCode code_next = getNullInterCode();
      code_next->kind = LABEL_IR;
      code_next->ops.push_back(labeln);

      InterCode code_loop = getNullInterCode();
      code_loop->kind = LABEL_IR;
      code_loop->ops.push_back(labelloop);

      before.push_back(labelloop);
      after.push_back(labeln);

      for (int i = 0; i < exps.size(); i++) {
        auto exp = exps[i];
        InterCode codeif = getNullInterCode();
        codeif->kind = IF_IR;
        insertInterCode(codeif, exp);
        InterCode codegoto = getNullInterCode();
        codegoto->ops.push_back(labeln);
        codegoto->kind = GOTO_IR;
        insertInterCode(codeif, codegoto);
        insertInterCode(code_loop, codeif);
      }

      InterCode gotoloop = getNullInterCode();
      gotoloop->kind = GOTO_IR;
      gotoloop->ops.push_back(labelloop);
      insertInterCode(code_loop, gotoloop);
      insertInterCode(code_loop, code_next);
      before.pop_back();
      after.pop_back();
      return code_loop;
    }
  } else if (root->children[0]->name == "FOR") {
    InterCode code1 = translateExp(root->children[2]);
    Operand label1 = newLabel();
    Operand label2 = newLabel();
    Operand label3 = newLabel();
    before.push_back(label3);
    after.push_back(label2);
    InterCode code2 = new InterCode_;
    code2->kind = LABEL_IR;
    code2->ops.push_back(label1);
    insertInterCode(code1, code2);
    InterCode code3 = new InterCode_;
    code3->kind = IF_IR;
    insertInterCode(code1, code3);
    InterCode code4 = translateExp(root->children[4]);
    turnoverRELOP(code4);
    insertInterCode(code1, code4);
    InterCode code5 = new InterCode_;
    code5->kind = GOTO_IR;
    code5->ops.push_back(label2);
    insertInterCode(code1, code5);
    InterCode code6 = translateCompSt(root->children[8], "");
    insertInterCode(code1, code6);
    InterCode code10 = new InterCode_;
    code10->kind = LABEL_IR;
    code10->ops.push_back(label3);
    insertInterCode(code1, code10);
    InterCode code7 = translateExp(root->children[6]);
    insertInterCode(code1, code7);
    InterCode code8 = new InterCode_;
    code8->kind = GOTO_IR;
    code8->ops.push_back(label1);
    insertInterCode(code1, code8);
    InterCode code9 = new InterCode_;
    code9->kind = LABEL_IR;
    code9->ops.push_back(label2);
    insertInterCode(code1, code9);
    before.pop_back();
    after.pop_back();
    return code1;
  } else if (root->children[0]->name == "BREAK") {
    InterCode code1 = new InterCode_;
    code1->kind = GOTO_IR;
    code1->ops.push_back(after.back());
    return code1;
  } else if (root->children[0]->name == "CONTINUE") {
    InterCode code1 = new InterCode_;
    code1->kind = GOTO_IR;
    code1->ops.push_back(before.back());
    return code1;
  }
  return getNullInterCode();
}

Operand translateExpOP(Node* root) {
  if (root->childNum != 0) {
    std::cout << "Error: translateExpOP" << std::endl;
    return nullptr;
  }
  if (root->name == "ID") {
    Operand res = new Operand_;
    res->kind = VARIABLE_OP;
    res->name = root->strVal;
    Entry entry = findSymbolAll(res->name);
    if (entry != nullptr) {
      res->type = entry->type;
      res->name += "_" + std::to_string(entry->rename);
    }
    return res;
  }
  if (root->name == "INT") {
    Operand res = new Operand_;
    res->kind = INT_CONSTANT_OP;
    res->intvalue = root->intVal;
    return res;
  }
  if (root->name == "FLOAT") {
    Operand res = new Operand_;
    res->kind = FLOAT_CONSTANT_OP;
    res->floatvalue = root->floatVal;
    return res;
  }
  if (root->name == "CHAR") {
    Operand res = new Operand_;
    res->kind = CHAR_CONSTANT_OP;
    res->charvalue = root->charVal;
    return res;
  }
  if (root->name == "BOOL") {
    Operand res = new Operand_;
    res->kind = BOOL_CONSTANT_OP;
    res->boolvalue = root->boolVal;
    return res;
  }
  if (root->children[0]->name == "STRING") {
  }
  if (root->children[0]->name == "VOID") {
  }
}

InterCode translateExp(Node* root) {
  if (root->childNum == 1) {
    Operand op = translateExpOP(root->children[0]);
    InterCode code = new InterCode_;
    code->kind = EXP_IR;
    code->ops.push_back(op);
    return code;
  }
  // ASSIGN_IR后面的第一条是赋值左边，第二条是赋值右边。
  // 只支持a=b+c这种，不支持a=b+c+d
  if (root->childNum == 3 && root->children[1]->name == "ASSIGNOP") {
    InterCode code1 = translateExp(root->children[0]);
    InterCode code2 = translateExp(root->children[2]);
    InterCode code = getNullInterCode();
    code->kind = ASSIGN_IR;
    code->left = code1;
    code->right = code2;
    return code;
  } else if (root->childNum == 3 && root->children[1]->name == "REASSIGNOP") {
    InterCode code1 = translateExp(root->children[0]);
    InterCode code2 = translateExp(root->children[2]);
    InterCode code = getNullInterCode();
    code->kind = REASSIGN_IR;
    int i;
    for (i = 0; i < 3; i++) {
      if (root->children[1]->strVal[i] != '=')
        code->op[i] = root->children[1]->strVal[i];
      else
        break;
    }
    code->op[i] = '\0';
    code->left = code1;
    code->right = code2;
    return code;
  } else if (root->childNum == 3 && root->children[1]->name == "AND") {
    InterCode code1 = translateExp(root->children[0]);
    InterCode code2 = translateExp(root->children[2]);
    InterCode code = getNullInterCode();
    code->kind = AND_IR;
    code->left = code1;
    code->right = code2;
    return code;
  } else if (root->childNum == 3 && root->children[1]->name == "OR") {
    InterCode code1 = translateExp(root->children[0]);
    InterCode code2 = translateExp(root->children[2]);
    InterCode code = getNullInterCode();
    code->kind = OR_IR;
    code->left = code1;
    code->right = code2;
    return code;
  } else if (root->childNum == 3 && root->children[1]->name == "RELOP") {
    InterCode code1 = getNullInterCode();
    code1->kind = RELOP_IR;
    int i;
    for (i = 0; i < 3; i++) {
      if (root->children[1]->strVal[i] != '\0')
        code1->op[i] = root->children[1]->strVal[i];
      else
        break;
    }
    code1->op[i] = '\0';
    InterCode code2 = translateExp(root->children[0]);
    InterCode code3 = translateExp(root->children[2]);
    code1->left = code2;
    code1->right = code3;
    return code1;
  } else if (root->childNum == 3 && root->children[1]->name == "PLUS") {
    InterCode code = getNullInterCode();
    code->kind = PLUS_IR;
    InterCode code1 = translateExp(root->children[0]);
    InterCode code2 = translateExp(root->children[2]);
    code->left = code1;
    code->right = code2;
    return code;
  } else if (root->childNum == 3 && root->children[1]->name == "MINUS") {
    InterCode code = getNullInterCode();
    code->kind = SUB_IR;
    InterCode code1 = translateExp(root->children[0]);
    InterCode code2 = translateExp(root->children[2]);
    code->left = code1;
    code->right = code2;
    return code;
  } else if (root->childNum == 3 && root->children[1]->name == "STAR") {
    InterCode code = getNullInterCode();
    code->kind = MUL_IR;
    InterCode code1 = translateExp(root->children[0]);
    InterCode code2 = translateExp(root->children[2]);
    code->left = code1;
    code->right = code2;
    return code;
  } else if (root->childNum == 3 && root->children[1]->name == "DIV") {
    InterCode code = getNullInterCode();
    code->kind = DIV_IR;
    InterCode code1 = translateExp(root->children[0]);
    InterCode code2 = translateExp(root->children[2]);
    code->left = code1;
    code->right = code2;
    return code;
  } else if (root->childNum == 3 && root->children[1]->name == "MOD") {
    InterCode code = getNullInterCode();
    code->kind = PLUS_IR;
    InterCode code1 = translateExp(root->children[0]);
    InterCode code2 = translateExp(root->children[2]);
    code->left = code1;
    code->right = code2;
    return code;
  } else if (root->childNum == 2 && root->children[0]->name == "MINUS") {
    InterCode code = getNullInterCode();
    code->kind = MINUS_IR;
    code->op[0] = '-';
    code->op[1] = '\0';
    Operand op = translateExpOP(root->children[1]->children[0]);
    code->ops.push_back(op);
    return code;
  } else if (root->childNum == 4 && root->children[1]->name == "LB") {
    // 数组赋值语句
    InterCode exp1 = translateExp(root->children[0]);
    InterCode exp2 = translateExp(root->children[2]);
    InterCode code = getNullInterCode();
    code->kind = ARRAY_ASSIGN_IR;
    insertInterCode(exp1, code);
    insertInterCode(exp1, exp2);
    return exp1;
  } else if (root->childNum == 4 && root->children[0]->name == "ID") {
    InterCode code = getNullInterCode();
    code->kind = CALL_IR;
    Operand op = translateExpOP(root->children[0]);
    op->kind = FUNCTION_OP;
    code->ops.push_back(op);
    InterCode codes = translateArgs(root->children[2]);
    insertInterCode(code, codes);
    return code;
  } else if (root->childNum == 3 && root->children[0]->name == "ID") {
    InterCode code = getNullInterCode();
    code->kind = CALL_IR;
    Operand op = translateExpOP(root->children[0]);
    op->kind = FUNCTION_OP;
    code->ops.push_back(op);
    return code;
  } else if (root->childNum == 4 && root->children[0]->name == "SIZEOF" &&
             root->children[2]->name == "Exp") {
    InterCode code = getNullInterCode();
    code->kind = SIZEOF_IR;
    InterCode exp = translateExp(root->children[2]);
    code->left = exp;
    return code;
  } else if (root->childNum == 4 && root->children[0]->name == "SIZEOF" &&
             root->children[2]->name == "TYPE") {
    InterCode code = getNullInterCode();
    code->kind = SIZEOF_IR;
    // InterCode exp = getNullInterCode();
    Type newType = new Type_;
    Operand op = new Operand_;
    if (!strcmp(root->children[2]->strVal, "int")) {
      newType->kind = BASIC;
      newType->basic = INT_TYPE;
      op->type = newType;
      op->kind = VARIABLE_OP;
      op->name = "INT";
    } else if (!strcmp(root->children[2]->strVal, "float")) {
      newType->kind = BASIC;
      newType->basic = FLOAT_TYPE;
      op->type = newType;
      op->kind = VARIABLE_OP;
      op->name = "FLOAT";
    } else if (!strcmp(root->children[2]->strVal, "char")) {
      newType->kind = BASIC;
      newType->basic = CHAR_TYPE;
      op->type = newType;
      op->kind = VARIABLE_OP;
      op->name = "CHAR";
    } else if (!strcmp(root->children[2]->strVal, "bool")) {
      newType->kind = BASIC;
      newType->basic = BOOL_TYPE;
      op->type = newType;
      op->kind = VARIABLE_OP;
      op->name = "BOOL";
    } else if (!strcmp(root->children[2]->strVal, "void")) {
      newType->kind = BASIC;
      newType->basic = VOID_TYPE;
      op->type = newType;
      op->kind = VARIABLE_OP;
      op->name = "VOID";
    }
    InterCode type = new InterCode_;
    type->kind = EXP_IR;
    type->ops.push_back(op);
    code->left = type;
    return code;
  } else if (root->childNum == 4 && root->children[1]->name == "TYPE") {
    InterCode code = translateExp(root->children[3]);
    return code;
  }
}

InterCode translateArgs(Node* root) {
  if (root->childNum == 1) {
    InterCode code = getNullInterCode();
    code->kind = AGR_IR;
    InterCode exp = translateExp(root->children[0]);
    insertInterCode(code, exp);
    return code;
  } else {
    InterCode code = getNullInterCode();
    code->kind = AGR_IR;
    InterCode exp = translateExp(root->children[0]);
    InterCode args = translateArgs(root->children[2]);
    insertInterCode(code, exp);
    insertInterCode(code, args);
    return code;
  }
}

InterCode translateScanf(Node* root) {
  if (root->children[4]->childNum == 1) {
    Operand op = translateExpOP(root->children[4]->children[0]);
    InterCode code = getNullInterCode();
    code->kind = SCANF_IR;
    code->ops.push_back(op);
    return code;
  } else {
    InterCode exp = translateExp(root->children[4]);
    InterCode code = getNullInterCode();
    code->kind = SCANF_IR;
    insertInterCode(code, exp);
    return code;
  }
}

InterCode translatePrintf(Node* root) {
  if (root->childNum == 5) {
    InterCode code = getNullInterCode();
    code->kind = PRINTF_IR;
    Operand op = new Operand_;
    op->kind = STRING_OP;
    std::string temp = "";
    int i = 4;
    while (root->children[2]->strVal[i] != '\0') {
      if (root->children[2]->strVal[i] == '\n') {
        temp.push_back('\\');
        temp.push_back('n');
      } else
        temp.push_back(root->children[2]->strVal[i]);
      i++;
    }
    op->name = temp;
    code->ops.push_back(op);
    return code;
  } else if (root->children[4]->childNum == 1) {
    InterCode code = getNullInterCode();
    code->kind = PRINTF_IR;
    Operand op = translateExpOP(root->children[4]->children[0]);
    code->ops.push_back(op);
    return code;
  } else {
    InterCode exp = translateExp(root->children[4]);
    InterCode code = getNullInterCode();
    code->kind = PRINTF_IR;
    insertInterCode(code, exp);
    return code;
  }
}

std::string printOperand(Operand op) {
  switch (op->kind) {
    case VARIABLE_OP:
      return op->name;
    case INT_CONSTANT_OP:
      return std::to_string(op->intvalue);
    case FLOAT_CONSTANT_OP:
      return std::to_string(op->floatvalue);
    case CHAR_CONSTANT_OP:
      if (op->charvalue == '\0')
        return "\\0";
      else if (op->charvalue == '\n')
        return "\\n";
      else {
        std::string res = "";
        res.push_back(op->charvalue);
        return res;
      }
    case FUNCTION_OP:
      return op->name;
    case LABEL_OP:
      return std::to_string(op->no);
  }
}

std::string generateIntercodeString(InterCode code) {
  if (code == nullptr) return "";
  if (code->kind == EXP_IR) {
    if (code->pre != nullptr && code->pre->kind == IF_IR) {
      if (code->ops[0]->kind == VARIABLE_OP)
        return code->ops[0]->name + " ";
      else if (code->ops[0]->kind == INT_CONSTANT_OP)
        return std::to_string(code->ops[0]->intvalue) + " ";
      else if (code->ops[0]->kind == CHAR_CONSTANT_OP) {
        if (code->ops[0]->charvalue == '\0') {
          std::string value = "\\0";
          return value + " ";
        } else if (code->ops[0]->charvalue == '\n') {
          std::string value = "\\n";
          return value + " ";
        }
        std::string value = "";
        value.push_back(code->ops[0]->charvalue);
        return value + " ";
      } else if (code->ops[0]->kind == FLOAT_CONSTANT_OP) {
        return std::to_string(code->ops[0]->floatvalue) + " ";
      } else if (code->ops[0]->kind == BOOL_CONSTANT_OP) {
        if (code->ops[0]->boolvalue == true)
          return "true ";
        else
          return "false ";
      }
    }
    if (code->pre != nullptr && code->pre->kind == PRINTF_IR && code->ops[0]->type != nullptr &&
        code->ops[0]->type->kind == ARRAY) {
      if (code->ops[0]->kind == VARIABLE_OP) {
        if (code->ops[0]->type->kind == ARRAY) {
          if (code->ops[0]->type->basic == INT_TYPE)
            return " INT " + code->ops[0]->name;
          else if (code->ops[0]->type->basic == CHAR_TYPE)
            return " CHAR " + code->ops[0]->name;
          else if (code->ops[0]->type->basic == FLOAT_TYPE)
            return " FLOAT " + code->ops[0]->name;
          else if (code->ops[0]->type->basic == BOOL_TYPE)
            return " BOOL " + code->ops[0]->name;
        }
      } else if (code->ops[0]->kind == INT_CONSTANT_OP)
        return std::to_string(code->ops[0]->intvalue);
      else if (code->ops[0]->kind == CHAR_CONSTANT_OP) {
        if (code->ops[0]->charvalue == '\0') {
          std::string value = "\'\\0\'";
          return value + " ";
        } else if (code->ops[0]->charvalue == '\n') {
          std::string value = "\'\\n\'";
          return value + " ";
        }
        std::string value = "\'";
        value.push_back(code->ops[0]->charvalue);
        return value + "\'";
      } else if (code->ops[0]->kind == FLOAT_CONSTANT_OP) {
        return std::to_string(code->ops[0]->floatvalue);
      } else if (code->ops[0]->kind == BOOL_CONSTANT_OP) {
        if (code->ops[0]->boolvalue == true)
          return "true";
        else
          return "false";
      }
    }
    if (code->ops[0]->kind == VARIABLE_OP)
      return code->ops[0]->name;
    else if (code->ops[0]->kind == INT_CONSTANT_OP)
      return std::to_string(code->ops[0]->intvalue);
    else if (code->ops[0]->kind == CHAR_CONSTANT_OP) {
      if (code->ops[0]->charvalue == '\0') {
        std::string value = "\'\\0\'";
        return value + " ";
      } else if (code->ops[0]->charvalue == '\n') {
        std::string value = "\'\\n\'";
        return value + " ";
      }
      std::string value = "\'";
      value.push_back(code->ops[0]->charvalue);
      return value + "\'";
    } else if (code->ops[0]->kind == FLOAT_CONSTANT_OP) {
      return std::to_string(code->ops[0]->floatvalue);
    } else if (code->ops[0]->kind == BOOL_CONSTANT_OP) {
      if (code->ops[0]->boolvalue == true)
        return "true";
      else
        return "false";
    }
  }
  if (code->kind == PLUS_IR) {
    std::string output;
    InterCode left = code->left;
    while (left != nullptr) {
      output = output + generateIntercodeString(left);
      if (left->kind == ARRAY_ASSIGN_IR) left = left->next;
      left = left->next;
    }
    output = output + " +";
    InterCode right = code->right;
    while (right != nullptr) {
      if (right->kind != ARRAY_ASSIGN_IR) output = output + " ";
      output = output + generateIntercodeString(right);
      if (right->kind == ARRAY_ASSIGN_IR) right = right->next;
      right = right->next;
    }
    output = output + "\n";
    return output;
  }
  if (code->kind == SUB_IR) {
    std::string output;
    InterCode left = code->left;
    while (left != nullptr) {
      output = output + generateIntercodeString(left);
      if (left->kind == ARRAY_ASSIGN_IR) left = left->next;
      left = left->next;
    }
    output = output + " -";
    InterCode right = code->right;
    while (right != nullptr) {
      if (right->kind != ARRAY_ASSIGN_IR) output = output + " ";
      output = output + generateIntercodeString(right);
      if (right->kind == ARRAY_ASSIGN_IR) right = right->next;
      right = right->next;
    }
    output = output + "\n";
    return output;
  }
  if (code->kind == MUL_IR) {
    std::string output;
    InterCode left = code->left;
    while (left != nullptr) {
      output = output + generateIntercodeString(left);
      if (left->kind == ARRAY_ASSIGN_IR) left = left->next;
      left = left->next;
    }
    output = output + " *";
    InterCode right = code->right;
    while (right != nullptr) {
      if (right->kind != ARRAY_ASSIGN_IR) output = output + " ";
      output = output + generateIntercodeString(right);
      if (right->kind == ARRAY_ASSIGN_IR) right = right->next;
      right = right->next;
    }
    output = output + "\n";
    return output;
  }
  if (code->kind == DIV_IR) {
    std::string output;
    InterCode left = code->left;
    while (left != nullptr) {
      output = output + generateIntercodeString(left);
      if (left->kind == ARRAY_ASSIGN_IR) left = left->next;
      left = left->next;
    }
    output = output + " /";
    InterCode right = code->right;
    while (right != nullptr) {
      if (right->kind != ARRAY_ASSIGN_IR) output = output + " ";
      output = output + generateIntercodeString(right);
      if (right->kind == ARRAY_ASSIGN_IR) right = right->next;
      right = right->next;
    }
    output = output + "\n";
    return output;
  }
  if (code->kind == MOD_IR) {
    std::string output;
    InterCode left = code->left;
    while (left != nullptr) {
      output = output + generateIntercodeString(left);
      if (left->kind == ARRAY_ASSIGN_IR) left = left->next;
      left = left->next;
    }
    output = output + " %";
    InterCode right = code->right;
    while (right != nullptr) {
      if (right->kind != ARRAY_ASSIGN_IR) output = output + " ";
      output = output + generateIntercodeString(right);
      if (right->kind == ARRAY_ASSIGN_IR) right = right->next;
      right = right->next;
    }
    output = output + "\n";
    return output;
  }
  if (code->kind == RELOP_IR) {
    std::string output;
    InterCode left = code->left;
    while (left != nullptr) {
      output = output + generateIntercodeString(left);
      if (left->kind == ARRAY_ASSIGN_IR) left = left->next;
      left = left->next;
    }
    int i;
    for (i = 0; i < 3; i++) {
      if (code->op[i] == '\0') break;
    }
    std::string relop(&code->op[0], &code->op[i]);
    output = output + " " + relop;
    InterCode right = code->right;
    while (right != nullptr) {
      if (right->kind != ARRAY_ASSIGN_IR) output = output + " ";
      output = output + generateIntercodeString(right);
      if (right->kind == ARRAY_ASSIGN_IR) right = right->next;
      right = right->next;
    }
    output = output + " ";
    return output;
  }
  if (code->kind == ARRAY_ASSIGN_IR) {
    std::string output;
    auto next = code->next;
    if (next && next->ops.size() && next->ops[0]->kind == VARIABLE_OP)
      output = "[" + next->ops[0]->name + "]";
    else if (next && next->ops.size())
      output = "[" + std::to_string(next->ops[0]->intvalue) + "]";
    if (code->next->next != nullptr && code->next->next->kind != ARRAY_ASSIGN_IR) {
      auto tmp = code->pre->pre;
      while (tmp->kind == ARRAY_ASSIGN_IR) tmp = tmp->pre->pre;
      if (tmp->pre->kind == SCANF_IR || tmp->pre->kind == PRINTF_IR) output = output + "\n";
    }
    return output;
  }
  if (code->kind == ASSIGN_IR) {
    std::string output;
    InterCode left = code->left;
    while (left != nullptr) {
      output = output + generateIntercodeString(left);
      if (left->kind == ARRAY_ASSIGN_IR) left = left->next;
      left = left->next;
    }
    output = output + " =";
    InterCode right = code->right;
    while (right != nullptr) {
      if (right->kind != ARRAY_ASSIGN_IR) output = output + " ";
      output = output + generateIntercodeString(right);
      if (right->kind == ARRAY_ASSIGN_IR) right = right->next;
      if (right->kind == CALL_IR) break;
      right = right->next;
    }
    output = output + "\n";
    return output;
  }
  if (code->kind == REASSIGN_IR) {
    std::string output;
    InterCode left = code->left;
    while (left != nullptr) {
      output = output + generateIntercodeString(left);
      if (left->kind == ARRAY_ASSIGN_IR) left = left->next;
      left = left->next;
    }
    output = output + " = " + output;

    int i;
    for (i = 0; i < 3; i++) {
      if (code->op[i] == '\0') break;
    }
    std::string relop(&code->op[0], &code->op[i]);
    output = output + " " + relop + " ";
    InterCode right = code->right;
    while (right != nullptr) {
      if (right->kind != ARRAY_ASSIGN_IR) output = output + " ";
      output = output + generateIntercodeString(right);
      if (right->kind == ARRAY_ASSIGN_IR) right = right->next;
      right = right->next;
    }
    output = output + "\n";
    return output;
  }
  if (code->kind == LABEL_IR) {
    std::string output = "";
    auto pre = code->pre;
    while (pre != nullptr && pre->kind == NULL_IR) pre = pre->pre;
    if (pre != nullptr && pre->kind == EXP_IR && pre->pre->kind == ARRAY_ASSIGN_IR) output = "\n";
    output += "LABEL ";
    output = output + std::to_string(code->ops[0]->no) + " :\n";
    return output;
  }
  if (code->kind == GOTO_IR) {
    auto tmp = code->pre;
    while (tmp->kind == NULL_IR) {
      tmp = tmp->pre;
    }
    if (tmp->kind == EXP_IR) tmp = tmp->pre;
    if (tmp->kind == ARRAY_ASSIGN_IR) std::cout << "\n";
    std::string output;
    output = "GOTO LABEL ";
    Operand label = code->ops[0];
    output = output + printOperand(label) + "\n";
    return output;
  }
  if (code->kind == IF_IR) {
    std::string output = "IF ";
    output = output + generateIntercodeString(code->exp);
    output = output + generateIntercodeString(code->right);
    std::cout << output;
    InterCode iter = code->right->next;
    while (iter != nullptr) {
      output = generateIntercodeString(iter);
      if (output != "") std::cout << output;
      iter = iter->next;
    }
    iter = code->left;
    while (iter != nullptr) {
      if (iter->kind != NULL_IR) {
        output = generateIntercodeString(iter);
        std::cout << output;
      }
      iter = iter->next;
    }
    output = "";
    return output;
  }
  if (code->kind == MINUS_IR) {
    std::string output;
    output = "-";
    output = output + printOperand(code->ops[0]);
    return output;
  }
  if (code->kind == CALL_IR) {
    std::string output;
    output = "CALL ";
    output = output + code->ops[0]->name;
    InterCode arg = code->next;
    InterCode exp;
    if (arg != nullptr) exp = arg->next;
    while (arg != nullptr && arg->kind == AGR_IR) {
      output = output + " ARG";
      output = output + " ";
      output = output + generateIntercodeString(exp);
      InterCode right = exp->next;
      while (right != nullptr && right->kind == ARRAY_ASSIGN_IR) {
        if (right->kind != ARRAY_ASSIGN_IR) output = output + " ";
        output = output + generateIntercodeString(right);
        if (right->kind == ARRAY_ASSIGN_IR) right = right->next;
        right = right->next;
      }

      arg = exp->next;
      if (arg != nullptr) exp = arg->next;
    }
    output = output + "\n";
    return output;
  }
  if (code->kind == SIZEOF_IR) {
    std::string output = "SIZEOF ";
    output = output + generateIntercodeString(code->left);
    return output;
  }
}

void printInterCodes() {
  InterCode cur = interCodes;
  while (cur != nullptr) {
    std::string output = "";
    if (cur->kind == FUNCTION_IR) {
      if (cur->pre != nullptr) std::cout << "\n";
      output = "FUNCTION ";
      output = output + printOperand(cur->ops[0]);
      if (cur->next->kind != PARAM_IR)
        output = output + " :\n";
      else {
        cur = cur->next;
        int i = 0;
        for (i; i < cur->ops.size(); i++) {
          output = output + " PARAM";
          output = output + " ";
          Type type = cur->ops[i]->type;
          if (cur->ops[i]->kind == VARIABLE_OP && type->kind == BASIC && type->basic == INT_TYPE) {
            output = output + "INT " + cur->ops[i]->name;
          } else if (cur->ops[i]->kind == VARIABLE_OP && type->kind == BASIC &&
                     type->basic == FLOAT_TYPE) {
            output = output + "FLOAT " + cur->ops[i]->name;
          } else if (cur->ops[i]->kind == VARIABLE_OP && type->kind == BASIC &&
                     type->basic == CHAR_TYPE) {
            output = output + "CHAR " + cur->ops[i]->name;
          } else if (cur->ops[i]->kind == VARIABLE_OP && type->kind == BASIC &&
                     type->basic == BOOL_TYPE) {
            output = output + "BOOL " + cur->ops[i]->name;
          } else if (cur->ops[i]->kind == VARIABLE_OP && type->kind == PTR &&
                     type->ptr.datatype == PTR_INT_TYPE) {
            output = output + "INT* " + cur->ops[i]->name;
          } else if (cur->ops[i]->kind == VARIABLE_OP && type->kind == PTR &&
                     type->ptr.datatype == PTR_FLOAT_TYPE) {
            output = output + "FLOAT* " + cur->ops[i]->name;
          } else if (cur->ops[i]->kind == VARIABLE_OP && type->kind == PTR &&
                     type->ptr.datatype == PTR_CHAR_TYPE) {
            output = output + "CHAR* " + cur->ops[i]->name;
          }
        }
        output = output + " :\n";
      }
      std::cout << output;
    } else if (cur->kind == DEC_IR) {
      if (cur->ops[0]->type->kind == BASIC && cur->ops[0]->type->basic == INT_TYPE) output = "INT ";
      if (cur->ops[0]->type->kind == BASIC && cur->ops[0]->type->basic == FLOAT_TYPE)
        output = "FLOAT ";
      if (cur->ops[0]->type->kind == BASIC && cur->ops[0]->type->basic == CHAR_TYPE)
        output = "CHAR ";
      if (cur->ops[0]->type->kind == BASIC && cur->ops[0]->type->basic == BOOL_TYPE)
        output = "BOOL ";
      if (cur->ops[0]->type->kind == PTR && cur->ops[0]->type->ptr.datatype == PTR_INT_TYPE)
        output = "INT* ";
      if (cur->ops[0]->type->kind == PTR && cur->ops[0]->type->ptr.datatype == PTR_FLOAT_TYPE)
        output = "FLOAT* ";
      if (cur->ops[0]->type->kind == PTR && cur->ops[0]->type->ptr.datatype == PTR_CHAR_TYPE)
        output = "CHAR* ";
      output = output + cur->ops[0]->name + "\n";
      std::cout << output;
    } else if (cur->kind == SCANF_IR) {
      std::string output;
      output = "READ";
      if (cur->ops.size() == 1) {
        Operand op = cur->ops[0];
        if (op->type->kind == BASIC && op->type->basic == INT_TYPE)
          output = output + " INT ";
        else if (op->type->kind == BASIC && op->type->basic == FLOAT_TYPE)
          output = output + " FLOAT ";
        else if (op->type->kind == BASIC && op->type->basic == CHAR_TYPE)
          output = output + " CHAR ";
        else if (op->type->kind == BASIC && op->type->basic == BOOL_TYPE)
          output = output + " BOOL ";
        else if (op->type->kind == PTR && op->type->ptr.datatype == PTR_CHAR_TYPE)
          output = output + " CHAR* ";
        else if (op->type->kind == ARRAY && op->type->basic == CHAR_TYPE)
          output = output + " CHAR* ";
        output = output + op->name + "\n";
        std::cout << output;
      } else {
        std::cout << output;
        if (cur->next->ops[0]->type->basic == INT_TYPE) std::cout << " INT ";
        if (cur->next->ops[0]->type->basic == FLOAT_TYPE) std::cout << " FLOAT ";
        if (cur->next->ops[0]->type->basic == CHAR_TYPE) std::cout << " CHAR ";
        if (cur->next->ops[0]->type->basic == BOOL_TYPE) std::cout << " BOOL ";
      }
    } else if (cur->kind == PRINTF_IR) {
      auto tmp = cur->pre;
      while (tmp->kind == EXP_IR && tmp->pre->kind == ARRAY_ASSIGN_IR) {
        tmp = tmp->pre;
        tmp = tmp->pre;
      }
      tmp = tmp->pre;
      if (tmp->kind == PRINTF_IR) std::cout << "\n";

      std::string output;
      output = "WRITE";
      if (cur->ops.size() == 1) {
        Operand op = cur->ops[0];
        if (op->kind != STRING_OP) {
          if (op->type->kind == BASIC && op->type->basic == INT_TYPE)
            output = output + " INT ";
          else if (op->type->kind == BASIC && op->type->basic == FLOAT_TYPE)
            output = output + " FLOAT ";
          else if (op->type->kind == BASIC && op->type->basic == CHAR_TYPE)
            output = output + " CHAR ";
          else if (op->type->kind == BASIC && op->type->basic == BOOL_TYPE)
            output = output + " BOOL ";
          else if (op->type->kind == PTR && op->type->ptr.datatype == PTR_CHAR_TYPE)
            output = output + " CHAR* ";
          else if (op->type->kind == ARRAY && op->type->basic == CHAR_TYPE)
            output = output + " CHAR* ";
          output = output + op->name + "\n";
        } else
          output = output + " \"" + op->name + "\"" + "\n";
        std::cout << output;
      } else {
        std::cout << output;
      }
    } else if (cur->kind == ARRAY_ASSIGN_IR) {
      output = generateIntercodeString(cur);
      std::cout << output;
      cur = cur->next;
    } else if (cur->kind == RETURN_IR) {
      std::string output = "RETURN ";
      output = output + generateIntercodeString(cur->next) + "\n";
      cur = cur->next;
      std::cout << output;
    } else if (cur->kind == IF_IR) {
      std::cout << "IF ";
    } else if (cur->kind == DEC_ARRAY_IR) {
      output = "ARRAY";
      if (cur->ops[0]->type->basic == INT_TYPE)
        output = output + "INT";
      else if (cur->ops[0]->type->basic == FLOAT_TYPE)
        output = output + "FLOAT";
      else if (cur->ops[0]->type->basic == CHAR_TYPE)
        output = output + "CHAR";
      else if (cur->ops[0]->type->basic == BOOL_TYPE)
        output = output + "BOOL";
      output = output + " ";
      output = output + cur->ops[0]->name;
      for (int i = 1; i < cur->ops.size(); i++) {
        if (cur->ops[i]->kind == INT_CONSTANT_OP)
          output = output + "[" + std::to_string(cur->ops[i]->intvalue) + "]";
        else
          output = output + "[" + cur->ops[i]->name + "]";
      }
      output = output + "\n";
      std::cout << output;
    } else if (cur->kind == START_STRUCT_IR) {
      std::string output;
      output = "STRUCT" + cur->ops[0]->name + " :" + "\n";
      std::cout << output;
    } else if (cur->kind == END_STRUCT_IR) {
      std::cout << "ENDSTRUCT" << std::endl;
    } else if (cur->kind == CALL_IR) {
      std::string output = generateIntercodeString(cur);
      auto tmp = cur->next;
      while (tmp != nullptr && tmp->kind == AGR_IR) {
        tmp = tmp->next->next;
        cur = tmp;
      }
      cur = tmp->pre;
      std::cout << output;
    } else if (cur->kind != NULL_IR) {
      output = generateIntercodeString(cur);
      std::cout << output;
    }
    cur = cur->next;
  }
}