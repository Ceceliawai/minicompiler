#include "semantic.hpp"
#include "hash.hpp"
extern bool duplicate_check;
extern int renameNo;
extern std::unordered_map<std::string, Entry> symbolTable;
extern Entry layersHead; // Imperative style
extern arrayDim getArrayDim(Type arr);
extern Type getElemType(Type b);
void semantic(Node* root) {
    initLayers();
    Program(root);
    check();
}
void check() {
    for (const auto& it: symbolTable) {
        if(it.second != nullptr) {
            Entry entry = it.second;
            while (entry) {
                if (entry->type->kind == FUNC && !entry->type->function->hasDefined) {
                    std::cout << "Error type 18 at line " << entry->type->function->lineno << ": Undefined function \""
                            << entry->name << "\"." << std::endl;
                }
                entry = entry->hashNext;
            }
        }
    }
}

void Program(Node* root) {
    ExtDefList(root->children[0]);
}

void ExtDefList(Node* root) {
    if (root->childNum==2) {
        ExtDef(root->children[0]);
        ExtDefList(root->children[1]);
    }
}

void ExtDef(Node* root) {
    Type type = Specifier(root->children[0]);
    // if type is wrong, return nullptr
    if (type == nullptr) return;
    if (type->kind == STRUCT && !type->structure->name.empty()
        && type->structure->head != nullptr) {
            auto newEntry = new Entry_;
            newEntry->name = type->structure->name;
            newEntry->type = new Type_;
            newEntry->type->structure = type->structure;
            newEntry->type->kind = STRUCT_DEF;
            insertSymbol(newEntry);
    }
    // 我们的指针不应该支持指针数组，否则最后无法到达basic类型
    if (root->children[1]->name == "ExtDecList") 
        ExtDecList(root->children[1], type);
    if (root->children[1]->name == "FunDec") {
    // 是否要考虑支持指针函数？
        Function fun = FunDec(root->children[1]);
        fun->returnType = type;
        fun->hasDefined = false;
        Type newType = new Type_;
        newType->kind = FUNC;

        newType->function = fun;
//        symbolTable["func"];
        Entry symbol = findSymbolFunc(fun->name);
        if (symbol != nullptr) 
            switch(symbol->type->function->hasDefined) {
                case false:
                    if (root->children[2]->name == "CompSt") {
                        if (!isEqualType(newType, symbol->type))
                            std::cout << "Error type 19 at line " << root->linenum << ": Conflict declaration of function \""
                                      << symbol->name << "\"." << std::endl;
                        else symbol->type->function->hasDefined = true;
                    }
                    else {
                        if (!isEqualType(newType, symbol->type))
                            std::cout << "Error type 19 at line " << root->linenum << ": Inconsistent declaration of function \""
                                      << symbol->name << "\"." << std::endl;
                    }
                    return;
                case true:
                    if (root->children[2]->name == "CompSt")
                        std::cout << "Error type 4 at line " << root->linenum << ": Redefined function \""
                                  << symbol->name << "\"." << std::endl;
                    else if (root->children[2]->name == "SEMI")
                        std::cout << "Error type 19 at line " << root->linenum << ": Conflict declaration of function \""
                                  << symbol->name << "\"." << std::endl;
                    return;
            }
        Entry newEntry = new Entry_;
        newEntry->name = fun->name;
        newEntry->type = newType;
        insertSymbol(newEntry);
        if (root->children[2]->name != "SEMI") {
            fun->hasDefined = true;
            pushLayer();
            renameNo++;
            CompSt(root->children[2], fun->name, fun->returnType);
            popLayer();
        }
    }
}

//Global variables dec
void ExtDecList(Node* root, Type type) {
    // 注意不能定义指针数组
    ExtDec(root->children[0], type, VAR);
    if (root->childNum == 1) return; 
    else ExtDecList(root->children[2], type);
}

void ExtDec(Node* root, Type type, IDType class_) {
    FieldList res = VarDec(root->children[0], type, class_);
    if (class_ == FIELD && res != nullptr && root->childNum == 3) {
        std::cout << "Error type 15 at line " << root->linenum << ": Initialized field \""
                  << res->name << "\"." << std::endl;
        return;
    }
    if (class_ == VAR && res != nullptr && root->childNum == 3) {
        Type expType = Exp(root->children[2]);
        if (type->kind == PTR && type->ptr.datatype == PTR_CHAR_TYPE && expType != nullptr &&
            expType->kind == ARRAY && getElemType(expType)->basic == CHAR_TYPE) {
            type->ptr.head = getArrayDim(expType);
        } else if (expType != nullptr && !isEqualType(expType, type)) {
            std::cout << "Error type 5 at line " << root->linenum << ": Type mismatched." << std::endl;
            return;
        }
    }
}

Type Specifier(Node* root) {
    root = root->children[0];
    if (root->name == "TYPE") {
        Type newType = new Type_;
        newType->kind = BASIC;
        if (!strcmp(root->strVal, "int"))
            newType->basic = INT_TYPE;
        else if (!strcmp(root->strVal, "float"))
            newType->basic = FLOAT_TYPE;
        else if (!strcmp(root->strVal, "char"))
            newType->basic = CHAR_TYPE;
        else if (!strcmp(root->strVal, "bool"))
            newType->basic = BOOL_TYPE;
        else if (!strcmp(root->strVal, "void"))
            newType->basic = VOID_TYPE;
        else if (!strcmp(root->strVal, "string")) {
            std::cout << "Error Type 0: Type \"String\" cannot be defined externally." << std::endl;
        }
        // string value should not be BASIC, but ARRAY, processed later
        return newType;
    } else if (root->name == "TYPEPTR") {
        auto newType = new Type_;
        newType->kind = PTR;
        if (!strcmp(root->strVal, "int*")) {
            newType->ptr.datatype = PTR_INT_TYPE;
            newType->ptr.head = nullptr;
        } else if (!strcmp(root->strVal, "float*")) {
            newType->ptr.datatype = PTR_FLOAT_TYPE;
            newType->ptr.head = nullptr;
        } else if (!strcmp(root->strVal, "char*")) {
            newType->ptr.datatype = PTR_CHAR_TYPE;
            newType->ptr.head = nullptr;
        }
        return newType;
    } else if (root->name == "StructSpecifier") {
        if (duplicate_check) return StructSpecifier(root);
        else return StructSpecifier_(root);
    }
        
    return nullptr;
}

Type StructSpecifier_(Node* root) {
    Type newType = new Type_;
    newType->kind = STRUCT;
    newType->structure = new Structure_;
    for (int i = 0; i < root->childNum; i++) {
        Node* child = root->children[i];
        if (child->name == "OptTag") {
            // OptTag->ID|epsilon
            if (child->childNum == 0)
                newType->structure->name = "";
            else {
                // Entry symbol = findSymbolAll(child->children[0]->strVal);
                // if (symbol != nullptr) {
                //     std::cout << "Error type 16 at line " + std::to_string(child->linenum) + ": Duplicated name \"" + child->children[0]->strVal + "\"." << std::endl;
                //     return nullptr;
                // }
                newType->structure->name = child->children[0]->strVal;
            }
        } else if (child->name == "Tag") {
            // Tag->ID
            Entry symbol = findSymbolAll(child->children[0]->strVal);
            if (symbol == nullptr || symbol->type->kind != STRUCT_DEF) {
                std::cout << "Error type 17 at line " + std::to_string(child->linenum) + ": Undefined struct \"" +
                             child->children[0]->strVal + "\"." << std::endl;
                return nullptr;
            }
            newType->structure = symbol->type->structure;
            return newType;
        } else if (child->name == "DefList") {
            pushLayer();
            renameNo++;
            newType->structure->head = DefList(child, FIELD);
            popLayer();

            return newType;
        }
    }
    return nullptr;
}

Type StructSpecifier(Node* root) {
    Type newType = new Type_;
    newType->kind = STRUCT;
    newType->structure = new Structure_;
    for (int i = 0; i < root->childNum; i++) {
        Node* child = root->children[i];
        if (child->name == "OptTag") {
            // OptTag->ID|epsilon
            if (child->childNum == 0)
                newType->structure->name = "";
            else {
                Entry symbol = findSymbolAll(child->children[0]->strVal);
                if (symbol != nullptr) {
                    std::cout << "Error type 16 at line " + std::to_string(child->linenum) + ": Duplicated name \"" + child->children[0]->strVal + "\"." << std::endl;
                    return nullptr;
                }
                newType->structure->name = child->children[0]->strVal;
            }
        } else if (child->name == "Tag") {
            // Tag->ID
            Entry symbol = findSymbolAll(child->children[0]->strVal);
            if (symbol == nullptr || symbol->type->kind != STRUCT_DEF) {
                std::cout << "Error type 17 at line " + std::to_string(child->linenum) + ": Undefined struct \"" +
                             child->children[0]->strVal + "\"." << std::endl;
                return nullptr;
            }
            newType->structure = symbol->type->structure;
            return newType;
        } else if (child->name == "DefList") {
            pushLayer();
            renameNo++;
            newType->structure->head = DefList(child, FIELD);
            popLayer();

            return newType;
        }
    }
    return nullptr;
}

FieldList VarDec(Node* root, Type type, IDType class_) {
    // VarDec->ID
    if (root->childNum == 1) {
        Entry symbolLayer = findSymbolLayer(root->children[0]->strVal);
        Entry symbolAll = findSymbolAll(root->children[0]->strVal);
        if (symbolLayer != nullptr || (symbolAll != nullptr && symbolAll->type->kind == STRUCT_DEF)) {
            switch (class_)
            {
                case FIELD:
                    std::cout << "Error type 15 at line " << root->linenum << ": Redefined field \""
                        << root->children[0]->strVal << "\"." << std::endl;
                    break;
                case VAR:
                    std::cout << "Error type 3 at line " << root->linenum << ": Redefined variable \""
                        << root->children[0]->strVal << "\"." << std::endl;
            }
            return nullptr;
        }
        auto res = new FieldList_;
        res->name = root->children[0]->strVal;
        res->type = type;
        auto newSym = new Entry_;
        newSym->name = root->children[0]->strVal;
        newSym->type = type;
        newSym->rename = renameNo;
        insertSymbol(newSym);
        return res;
    }
    // VarDec->VarDec LB INT RB
    else {
        if (type->kind == PTR) {
            std::cout << "Error type I at line " << root->linenum << ": Pointer cannot be defined as an array."
                << std::endl;
            return nullptr;
        }
        if (root->children[2]->name == "ID") {
            Entry symbolLayer = findSymbolAll(root->children[2]->strVal);
            if (symbolLayer == nullptr || symbolLayer->type->kind != BASIC || symbolLayer->type->basic != INT_TYPE) {
                std::cout << "Error type II at line " << root->linenum << ": Undefined or Non-int type variable \""
                    << symbolLayer->name << "\"." << std::endl;
                return nullptr;
            }
            Type newType = new Type_;
            newType->kind = ARRAY;
            newType->array.elem = type;
            newType->array.size = 0;//这里在中间代码IR部分要考虑修改掉！！！！
            int i;
            for(i = 0; i < 32; i++){
                if(root->children[2]->strVal[i] != '\0')newType->array.name[i] = root->children[2]->strVal[i];
                else break;
            }
            newType->array.name[i] = '\0' ;
            return VarDec(root->children[0], newType, class_);
        } else {
            Type newType  = new Type_;
            newType->kind = ARRAY;
            newType->array.elem = type;
            newType->array.size = root->children[2]->intVal;
            return VarDec(root->children[0], newType, class_);
        }
    }
}

void Scanf(Node* root) {
    std::string fmt = root->children[2]->glbStr.val;
    DataType input;
    if (fmt == "%d") input = INT_TYPE;
    else if (fmt == "%f") input = FLOAT_TYPE;
    else if (fmt == "%s") input = STRING_TYPE;
    else if (fmt == "%c") input = CHAR_TYPE;
    else {
        std::cout << "Error type V at line " << root->linenum << ": Illegal format." << std::endl;
        return ;
    }
    Type curType = Exp(root->children[4]);
    if (curType->kind == PTR && curType->ptr.datatype != PTR_CHAR_TYPE) {
        std::cout << "Error type III at line " << root->linenum << ": Illegal input array." << std::endl;
    } else if (curType->kind == PTR || (curType->kind == ARRAY && curType->array.elem->kind == BASIC && curType->array.elem->basic == CHAR_TYPE)) {
        if (input != STRING_TYPE) {
            std::cout << "Error type IV(1) at line " << root->linenum << ": Input format not matched." << std::endl;
        }
    } else {
        if (input != curType->basic) {
            std::cout << "Error type IV(2) at line " << root->linenum << ": Input format not matched." << std::endl;
        }
    }
}

void Printf(Node* root) {
    std::string fmt = root->children[2]->glbStr.val;
    if (root->childNum == 5) {
        return;
    }
    DataType output;
    if (fmt == "%d") output = INT_TYPE;
    else if (fmt == "%f") output = FLOAT_TYPE;
    else if (fmt == "%s") output = STRING_TYPE;
    else if (fmt == "%c") output = CHAR_TYPE;
    else {
        std::cout << "Error type V at line " << root->linenum << ": Illegal format." << std::endl;
        return ;
    }
    Type curType = Exp(root->children[4]);
    if (curType->kind == PTR && curType->ptr.datatype != PTR_CHAR_TYPE) {
        std::cout << "Error type III at line " << root->linenum << ": Illegal input array." << std::endl;
    } else if (curType->kind == PTR || (curType->kind == ARRAY && curType->array.elem->kind == BASIC && curType->array.elem->basic == CHAR_TYPE)) {
        if (output != STRING_TYPE) {
            std::cout << "Error type IV at line " << root->linenum << ": Input format not matched." << std::endl;
        }
    } else {
        if (output != curType->basic) {
            std::cout << "Error type IV at line " << root->linenum << ": Input format not matched." << std::endl;
        }
    }
}

//function dec
Function FunDec(Node* root) {
    auto newFunc = new Function_(root->children[0]->strVal, root->linenum);
    if (root->childNum == 3)
        newFunc->head = nullptr;
    else {
        pushLayer();
        renameNo++;
        newFunc->head = VarList(root->children[2]);
        popLayer();
        FieldList temp = newFunc->head;
        while (temp != nullptr) {
            newFunc->paraNum++;
            temp = temp->next;
        }
    }
    return newFunc;
}

FieldList VarList(Node* root) {
    // VarList->ParamDec
    FieldList res = ParamDec(root->children[0]);
    // VarList->ParamDec COMMA VarList
    if (root->childNum == 3)
        res->next = VarList(root->children[2]);
    return res;
}

FieldList ParamDec(Node* root) {
    // ParamDec->Specifier VarDec
    Type spec = Specifier(root->children[0]);
    return VarDec(root->children[1], spec, FIELD);
}

void CompSt(Node* root, const std::string& funName, Type paraType) {
    Type curType = paraType;
    if (!funName.empty()) {
        Entry symbol = findSymbolFunc(funName);
        FieldList params = symbol->type->function->head;
        while (params != nullptr) {
            auto parameter = new Entry_;
            parameter->name = params->name;
            parameter->type = params->type;
            insertSymbol(parameter);
            params = params->next;
        }
        curType = symbol->type->function->returnType;
    }
//    DefList(root->children[1], VAR);
    StmtList(root->children[1], curType);
}

void StmtList(Node* root, Type reType) {
    // StmtList->Stmt StmtList
    if (root->childNum == 2 && root->children[0]->name == "Stmt") {
        Stmt(root->children[0], reType);
        StmtList(root->children[1], reType);
    } else if (root->childNum == 2 && root->children[0]->name == "Def") {
        Def(root->children[0], VAR);
        StmtList(root->children[1], reType);
    }
}

void Stmt(Node* root, Type reType) {
    std::string name = root->children[0]->name;
    // Stmt->Exp
    if (name == "Exp") {
        Exp(root->children[0]);
    }
    else if (name == "CompSt") { // Stmt->Compst
        pushLayer();
        renameNo++;
        CompSt(root->children[0], "", reType);
        popLayer();
    } else if (name == "Scanf"){
        Scanf(root->children[0]);
    } else if (name == "Printf"){
        Printf(root->children[0]);
    } else if (name == "RETURN") { // Stmt->RETURN
        Type resType = Exp(root->children[1]);
        if (!isEqualType(resType, reType))
            std::cout << "Error type 8 at line " << root->linenum << ": Type mismatched for return." << std::endl;
    } else if (name == "IF") { // Stmt->IF
        Exp(root->children[2]);
        Stmt(root->children[4], reType);
        if (root->childNum == 7)
            Stmt(root->children[6], reType);
    } else if (name == "WHILE") { // Stmt->WHILE
        Exp(root->children[2]);
        Stmt(root->children[4], reType);
    } else if (name == "FOR") {
        Exp(root->children[2]);
        Exp(root->children[4]);
        Exp(root->children[6]);
        pushLayer();
        renameNo++;
        CompSt(root->children[8], "", reType);
        popLayer();
    } else if (name == "BREAK") {
        return;
    } else if (name == "CONTINUE") {
        return;
    }

}

FieldList DefList(Node* root, IDType class_) {
    if (root->childNum == 0) return nullptr;
    else {
        FieldList newField = Def(root->children[0], class_);
        if (newField == nullptr) {
            newField = DefList(root->children[1], class_);
        }
        else {
            FieldList temp = newField;
            while (temp->next != nullptr)
                temp = temp->next;
            temp->next = DefList(root->children[1], class_);
        }
        return newField;
    }
}

FieldList Def(Node*root, IDType class_) {
    Type type = Specifier(root->children[0]);
    FieldList newField = DecList(root->children[1], type, class_);
    return newField;
}

FieldList DecList(Node*root, Type type, IDType class_) {
    FieldList res = Dec(root->children[0], type, class_);
    if (root->childNum == 1) return res;
    if (res == nullptr) res = DecList(root->children[2], type, class_);
    else {
        FieldList tmp = res;
        while (tmp->next != nullptr) tmp = tmp->next;
        tmp->next = DecList(root->children[2], type, class_);
    }
    return res;
}

FieldList Dec(Node* root, Type type, IDType class_) {
    FieldList res = VarDec(root->children[0], type, class_);

    if (class_ == FIELD && res != nullptr && root->childNum == 3) {
        std::cout << "Error type 15 at line " << root->linenum << ": Initialized field \""
                << res->name << "\"." << std::endl;
        return nullptr; 
    }

    if (class_ == VAR && res != nullptr && root->childNum == 3) {
        Type expType = Exp(root->children[2]);
        if (type->kind == PTR && type->ptr.datatype == PTR_CHAR_TYPE && expType != nullptr &&
        expType->kind == ARRAY && getElemType(expType)->basic == CHAR_TYPE) {
            type->ptr.head = getArrayDim(expType);
        } /*
        if (type->kind == BASIC && type->basic == STRING_TYPE) {
            type->kind = ARRAY;
            type->array.elem = new Type_;
            type->array.elem = expType->array.elem;
            type->array.size = expType->array.size;
        }*/
        else if (expType != nullptr && !isEqualType(expType, type)) {
//            std::cout << type->kind << std::endl;
            std::cout << "Error type 5 at line " << root->linenum << ": Type mismatched." << std::endl;
            return nullptr;
        }
    }
    return res;
}

Type Exp(Node* root) {
    std::string name1 = root->children[0]->name;
    std::string name2 = root->childNum>=2 ? root->children[1]->name : "";
    if (name1 == "Exp") {
        // ASSIGNOP
        if (name2 == "ASSIGNOP" || name2 == "REASSIGNOP") {
            // three confirmed situations
            // 1.var 2.field 3.arrayelem
            // 1.string
            Node* left = root->children[0];
            Node* right = root->children[2];
            Type leftType = nullptr;
            Type rightType = Exp(right);
            std::string leftName0 = left->children[0]->name;
            std::string leftName1 = left->childNum >= 2 ? left->children[1]->name:"";
            if ((left->childNum == 1 && leftName0 == "ID") ||
                (left->childNum == 3 && leftName1 == "DOT") ||
                (left->childNum == 4 && leftName1 == "LB"))
                leftType = Exp(left);
            else {
                std::cout << "Error type 6 at line " << root->linenum << ": The left-hand side of an assignment must be a variable." << std::endl;
                return nullptr;
            }
            if (leftType != nullptr && rightType != nullptr && !isEqualType(leftType, rightType)) {
                std::cout << "Error type 5 at line " << root->linenum << ": Type mismatched for assignment." << std::endl;
                return nullptr;
            }
            return leftType;

        } else if (name2 == "LB") { // ARRAY
            Type base = Exp(root->children[0]);
            if (base != nullptr) {
                if ((base->kind != ARRAY && base->kind != PTR)) {
                    std::cout << "Error type 10 at line " << root->linenum << ": Expect an array before [...]." << std::endl;
                    return nullptr;
                }
                Type index = Exp(root->children[2]);
                if (index == nullptr || index->kind != BASIC || index->basic != INT_TYPE) {
                    std::cout << "Error type 12 at line " << root->linenum << ": Expect an integer in [...]." << std::endl;
                    return nullptr;
                }
                Type tmp = new Type_;
                tmp->kind = PTR;
                tmp->ptr.datatype = base->ptr.datatype;
                tmp->ptr.head = getArrayDim(base);
                if (tmp->ptr.head == nullptr) {
                    DataType dtype;
                    if (tmp->ptr.datatype == PTR_INT_TYPE) dtype = INT_TYPE;
                    else if (tmp->ptr.datatype == PTR_CHAR_TYPE) dtype = CHAR_TYPE;
                    else dtype = FLOAT_TYPE;
                    tmp->kind = BASIC;
                    tmp->basic = dtype;
                }
                return base->kind == ARRAY ? base->array.elem : tmp;
            }
            return nullptr;
        } else if (name2 == "DOT") { // STRUCT
            Type res = Exp(root->children[0]);
            if (res != nullptr) {
                if (res->kind != STRUCT) {
                    std::cout << "Error type 13 at line " << root->linenum << ": Illegal use of \".\"." << std::endl;
                    return nullptr;
                }
                std::string field = root->children[2]->strVal;
                FieldList head = res->structure->head;
                Type found = nullptr;
                while (head != nullptr) {
                    if (field == head->name) {
                        found = head->type;
                        break;
                    }
                    head = head->next;
                }
                if (found == nullptr) {
                    std::cout << "Error type 14 at line " << root->linenum << ": Non-existed field \"" << field << "\"." << std::endl;
                    return nullptr;
                }
                return found;
            }
            return nullptr;
        } else { // CALC
            Type exp1Type = Exp(root->children[0]);
            Type exp2Type = Exp(root->children[2]);
            if (exp1Type == nullptr || exp2Type == nullptr) return nullptr;
            if (!isEqualType(exp1Type, exp2Type)) {
                std::cout << "Error type 7 at line " << root->linenum << ": Type mismatched for operands." << std::endl;
                return nullptr;
            }
            if (root->children[1]->name == "RELOP") {
                Type cmp = new Type_;
                cmp->kind = BASIC;
                cmp->basic = BOOL_TYPE;
                return cmp;
            }
            return exp1Type;
        }

    } else if (name1 == "SIZEOF") {
        Type res = new Type_;
        res->kind = BASIC;
        res->basic = INT_TYPE;
        if (root->children[2]->name == "Exp") {
            Type exp = Exp(root->children[2]);
            return res;
        } else if (root->children[2]->name == "TYPE") {
            return res;
        } else return nullptr;
    } else if (name1 == "LP" && name2 == "Exp") {
        return Exp(root->children[1]);
    } else if (name1 == "MINUS") {
        Type res = Exp(root->children[1]);
        // null error is left for exp
        if (res != nullptr) {
            if (res->kind == BASIC &&
                (res->basic == INT_TYPE ||
                 res->basic == FLOAT_TYPE))
                return res;
            else std::cout << "Error type 7 at line " << root->linenum << ": Type mismatched for operands." << std::endl;
        }
        return nullptr;
    } else if (name1 == "NOT") { // !Exp
        Type res = Exp(root->children[1]);
        if (res != nullptr)
            if (res->kind != BASIC || res->basic != INT_TYPE || res->basic != BOOL_TYPE) {
                std::cout << "Error type 7 at line " << root->linenum << ": Type mismatched for operands." << std::endl;
                return nullptr;
            }
        return res;
    } else if (name1 == "ID") { // ID
        // var
        if (root->childNum == 1) {
            Entry symbol = findSymbolAll(root->children[0]->strVal);
            if (symbol == nullptr) {
                std::cout << "Error type 1 at line " << root->linenum << ": Undefined variable \""
                          << root->children[0]->strVal << "\"." << std::endl;
                return nullptr;
            }
            return symbol->type;
        } else { // function
            Entry symbol = findSymbolFunc(root->children[0]->strVal);
            // not a func var
            if (symbol == nullptr) {
                symbol = findSymbolAll(root->children[0]->strVal);
                // normal var
                if (symbol != nullptr)
                    std::cout << "Error type 11 at line " << root->linenum << ": \"" << symbol->name
                              << "\" is not a function." << std::endl;
                    // not a var
                else
                    std::cout << "Error type 2 at line " << root->linenum << ": Undefined function \""
                              << root->children[0]->strVal << "\"." << std::endl;
                return nullptr;
            }
            // not defined
            if (!symbol->type->function->hasDefined) {
                std::cout << "Error type 2 at line " << root->linenum << ": Undefined function \""
                          << root->children[0]->strVal << "\"." << std::endl;
                return nullptr;
            }
            FieldList args = nullptr;
            FieldList argsT = nullptr;
            FieldList defArgs = symbol->type->function->head;
            // func (args)
            if (root->childNum == 4) {
                args = Args(root->children[2]);
                argsT = args;
            }
            bool equalArgs = true;
            while (args != nullptr && defArgs != nullptr && equalArgs) {
                equalArgs = isEqualType(args->type, defArgs->type);
                args = args->next;
                defArgs = defArgs->next;
            }
            if (args != nullptr || defArgs != nullptr) equalArgs = false;
            if (!equalArgs) {
                std::cout << "Error type 9 at line " << root->linenum << ": The method \""
                          << symbol->name << "(";
                printArgs(symbol->type->function->head);
                std::cout << ")\" is not applicable for the arguments \"(";
                printArgs(argsT);
                std::cout << ")\"." << std::endl;
                return nullptr;
            }
            return symbol->type->function->returnType;
        }
    } else if (name1 == "INT") { // INT
        Type res = new Type_;
        res->kind = BASIC;
        res->basic = INT_TYPE;
        return res;
    } else if (name1 == "FLOAT") { // FLOAT
        Type res = new Type_;
        res->kind = BASIC;
        res->basic = FLOAT_TYPE;
        return res;
    } else if (name1 == "CHAR") { // CHAR
        Type res = new Type_;
        res->kind = BASIC;
        res->basic = CHAR_TYPE;
        return res;
    } else if (name1 == "BOOL") { // BOOL
        Type res = new Type_;
        res->kind = BASIC;
        res->basic = BOOL_TYPE;
        return res;
    } else if (name1 == "STRING") {
        Type res = new Type_;
        res->kind = ARRAY;
        res->array.size = root->children[0]->glbStr.size+1;
        res->array.elem = new Type_;
        res->array.elem->kind = BASIC;
        res->array.elem->basic = CHAR_TYPE;
        return res;
    } else if (name1 == "VOID") { // VOID
        Type res = new Type_;
        res->kind = BASIC;
        res->basic = VOID_TYPE;
        return res;
    } else if (name2 == "TYPE") {
        if (!strcmp(root->children[1]->strVal, "int")) {
            Type type = Exp(root->children[3]);
            if (type->kind != BASIC || type->basic == STRING_TYPE || type->basic == VOID_TYPE) {
                std::cout << "Error Type VI: Type \"";
                printType(type);
                std::cout << "\" cannot be converted into type \"int\"." << std::endl;
                return nullptr;
            }
            Type res = new Type_;
            res->kind = BASIC;
            res->basic = INT_TYPE;
            return res;
        } else if (!strcmp(root->children[1]->strVal, "float")) {
            Type type = Exp(root->children[3]);
            if (type->kind != BASIC || type->basic == STRING_TYPE || type->basic == VOID_TYPE) {
                std::cout << "Error Type VI: Type \"";
                printType(type);
                std::cout << "\" cannot be converted into type \"float\"." << std::endl;
                return nullptr;
            }
            Type res = new Type_;
            res->kind = BASIC;
            res->basic = FLOAT_TYPE;
            return res;
        } else if (!strcmp(root->children[1]->strVal, "char") ||
                   !strcmp(root->children[1]->strVal, "bool") ) {
            Type type = Exp(root->children[3]);
            if (type->kind != BASIC && type->basic != INT_TYPE && type->basic != CHAR_TYPE) {
                std::cout << "Error Type VI: Type \"";
                printType(type);
                std::cout << "\" cannot be converted into type \"char/bool\"." << std::endl;
                return nullptr;
            }
            Type res = new Type_;
            res->kind = BASIC;
            res->basic = CHAR_TYPE;
            if (!strcmp(root->children[1]->strVal, "bool"))
                res->basic = BOOL_TYPE;
            return res;
        } else {
            std::cout << "Error Type VII: One cannot convert an expression into type\""
                << root->children[1]->strVal << "\"." << std::endl;
            return nullptr;
        }
    }
    return nullptr;
}

FieldList Args(Node* root) {
    auto res = new FieldList_;
    res->type = Exp(root->children[0]);
    // Args->Exp COMMA Args
    if (root->childNum == 3)
        res->next = Args(root->children[2]);
    return res;
}

void printArgs(FieldList head) {
    if (head == nullptr) return;
    printType(head->type);
    if (head->next == nullptr) return;
    std::cout << ", ";
    printArgs(head->next);
}
void printType(Type type) {
    switch (type->kind) {
        case BASIC:
            switch (type->basic) {
                case INT_TYPE:
                    std::cout << "int"; break;
                case FLOAT_TYPE:
                    std::cout << "float"; break;
                case CHAR_TYPE:
                    std::cout << "char"; break;
                case BOOL_TYPE:
                    std::cout << "bool"; break;
            }
            break;
        case PTR:
            switch (type->ptr.datatype) {
                case PTR_CHAR_TYPE:
                    std::cout << "char*"; break;
                case PTR_INT_TYPE:
                    std::cout << "int*"; break;
                case PTR_FLOAT_TYPE:
                    std::cout << "float*"; break;
            }
            break;
        case STRUCT:
            std::cout << "struct " << type->structure->name;
            break;
        case ARRAY:
            printType(type->array.elem);
            std::cout << "[]";
            break;
        case FUNC:
            std::cout << "function " << type->function->name;
            break;
        case STRUCT_DEF:
            std::cout << "struct def " << type->structure->name;
    }
}

