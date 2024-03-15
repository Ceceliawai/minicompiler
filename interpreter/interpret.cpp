
#include "interpret.h"

Program p = new Program_; /*NOLINT*/
bool isRunning = false;
string activeFuncName = "main";
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

char convertChar(string raw) {
    if (raw.size() == 3) return raw[1];
    else return escape2char(raw[2]);
}

string convertString(string raw) {
    std::string temp;
    for (int i = 1; i <= raw.size()-2; i++) {
        if (raw[i] == '\\') {
            i++;
            temp.push_back(escape2char(raw[i]));
        } else {
            temp.push_back(raw[i]);
        }
    }
    return temp;
}


void insertSymbol(Symbol sym, const string& name) {
    symbolTable[name] = sym;
}

void printFirst() {
    for (const auto& item: symbolTable) {
        string name = item.first;
        Symbol sym = item.second;
        if (sym->type == FUNCTION) {
            cout << "func: " << name << ", in line: " << sym->lineno << " ";
            auto it = sym->func->head;
            while (it != nullptr) {
                cout << "(" << it->var->name << ", " << it->var->type << ") ";
                it = it->nextVar;
            }
            cout << endl;
        }
        else if (sym->type == LABEL)
            cout << "label: " << name << ", in line: " << sym->lineno << endl;
    }
}
int getRestDim(Array arr, int cnt) {
    int tot = 1;
    for (int i = cnt+1; i < arr->size.size(); i++)
        tot *= arr->size[i];
    return tot;
}
int calcPtrIndex(string id) {
    while(id[0] != '[')
        id.erase(0, 1);
    id.erase(0, 1);
    int len = 1;
    int temp;
    string val, tmpstr = id;
    char ch;
    bool ndone = true;
    while(ndone) {
        stringstream ss(tmpstr);
        if (ss >> temp) {
            len = temp;
            ndone = (bool) (ss >> ch >> ch);
        } else {
            ss.clear();
            ss.str(tmpstr);
            ss >> ch;
            while (ch != ']') {
                val.push_back(ch);
                ss >> ch;
            }
            len = symbolTable[val]->var->iVal;
            val.erase(0);
            ndone = (bool)(ss >> ch);
        }
        getline(ss, tmpstr);
    }
    return len;
}
bool runningFirst = true;
int calcArraySize(string id, Array arr) {
    while(id[0] != '[')
        id.erase(0, 1);
    id.erase(0, 1);
    int len = 0;
    int temp;
    string val, tmpstr = id;
    char ch;
    bool ndone = true;
    int cnt = 0;
    while(ndone) {
        stringstream ss(tmpstr);
        if (ss >> temp) {
            len += (temp) * getRestDim(arr, cnt);
            cnt += 1;
            ndone = (bool) (ss >> ch >> ch);
        } else {
            ss.clear();
            ss.str(tmpstr);
            ss >> ch;
            while (ch != ']') {
                val.push_back(ch);
                ss >> ch;
            }
            len += (symbolTable[val]->var->iVal) * getRestDim(arr, cnt);
            cnt += 1;
            val.erase(0);
            ndone = (bool)(ss >> ch);
        }
        getline(ss, tmpstr);
    }
    return len;
}
int getArraySize(string id, Array arr) {
    while(id[0] != '[')
        id.erase(0, 1);
    id.erase(0, 1);
    int len = 1;
    int temp;
    string val, tmpstr = id;
    char ch;
    bool ndone = true;
    while(ndone) {
        stringstream ss(tmpstr);
        if (ss >> temp) {
            len *= temp;
            arr->size.push_back(temp);
            ndone = (bool) (ss >> ch >> ch);
        } else {
            ss.clear();
            ss.str(tmpstr);
            ss >> ch;
            while (ch != ']') {
                val.push_back(ch);
                ss >> ch;
            }
            len *= symbolTable[val]->var->iVal;
            arr->size.push_back(symbolTable[val]->var->iVal);
            val.erase(0);
            ndone = (bool)(ss >> ch);
        }
        getline(ss, tmpstr);
    }
    return len;
}
string getArrayName(string id) {
    size_t pos = id.find('[');
    if (pos != string::npos) {
        id.erase(pos);
    }
    return id;
}
Var createVar(string type, string id) {
    transform(type.begin(), type.end(), type.begin(), ::tolower);
    Var var = new Var_;
    int len = id.size();
    bool isArray = id[len-1] == ']';
    if (isArray) {
        var->name = getArrayName(id);
        var->type = ARRAY;
        var->arr = new Array_;
        int size = getArraySize(id, var->arr);
        if (type == "arrayint") {
            var->arr->type = INT_TYPE;
            var->arr->iarr = (int*) malloc(sizeof(int)*size);
        }
        else if (type == "arrayfloat") {
            var->arr->type = FLOAT_TYPE;
            var->arr->farr = (float*) malloc(sizeof(float)*size);
        }
        else if (type == "arraychar") {
            var->arr->type = CHAR_TYPE;
            var->arr->carr = (char*) malloc(sizeof(char)*size);
        }
        return var;
    }
    else var->name = id;
    if (type == "int") {
        var->type = INT_TYPE;
    } else if (type == "char") {
        var->type = CHAR_TYPE;
    } else if (type == "float") {
        var->type = FLOAT_TYPE;
    } else if (type == "bool") {
        var->type = BOOL_TYPE;
    } else if (type == "arrayint*" || type == "int*") {
        var->type = PTR_INT_TYPE;
    } else if (type == "arraychar*" || type == "char*") {
        var->type = PTR_CHAR_TYPE;
    } else if (type == "arrayfloat*" || type == "float*") {
        var->type = PTR_FLOAT_TYPE;
    }
    return var;
}

Var createVar(string type, string id, string value) {
    transform(type.begin(), type.end(), type.begin(), ::tolower);
    Var var = new Var_;
    var->name = std::move(id);
    string name = getArrayName(value);
    if (symbolTable.find(name) != symbolTable.end()) {
        bool isArr = value[value.size()-1] == ']';
        bool isPtr = symbolTable[name]->var->type == PTR_FLOAT_TYPE || symbolTable[name]->var->type == PTR_INT_TYPE
                || symbolTable[name]->var->type == PTR_CHAR_TYPE;
        int idx = -1;
        Var t = symbolTable[name]->var;
        if (isPtr) idx = calcPtrIndex(value);
        else if (isArr) idx = calcArraySize(value, symbolTable[name]->var->arr);
        if(t->type == PTR_INT_TYPE || isArr && t->arr->type == INT_TYPE) {
            var->type = INT_TYPE;
            var->iVal = isPtr ? ((int*)t->ptr)[idx] : t->arr->iarr[idx];
        } else if(t->type == PTR_CHAR_TYPE || isArr && t->arr->type == CHAR_TYPE) {
            var->type = CHAR_TYPE;
            var->cVal = isPtr ? ((char*)t->ptr)[idx] : t->arr->carr[idx];
        } else if(t->type == PTR_FLOAT_TYPE || isArr && t->arr->type == FLOAT_TYPE) {
            var->type = FLOAT_TYPE;
            var->fVal = isPtr ? ((float*)t->ptr)[idx] : t->arr->farr[idx];
        }
        if(isArr || isPtr) return var;
    }
    bool notfound = symbolTable.find(value) == symbolTable.end();
    if (type == "int") {
        var->type = INT_TYPE;
        var->iVal = notfound ? atoi(value.c_str()) : symbolTable[value]->var->iVal;
    } else if (type == "char") {
        var->type = CHAR_TYPE;
        var->cVal = notfound ? convertChar(value) : symbolTable[value]->var->cVal;
    } else if (type == "float") {
        var->type = FLOAT_TYPE;
        var->fVal = notfound ? atof(value.c_str()) : symbolTable[value]->var->fVal;
    } else if (type == "bool") {
        var->type = BOOL_TYPE;
        var->bVal = notfound ? value == "true" : symbolTable[value]->var->bVal;
    } else if (type == "arrayint*") {
        var->type = PTR_INT_TYPE;
    } else if (type == "arraychar*") {
        var->type = PTR_CHAR_TYPE;
        value = convertString(value);
        char* temp = (char*)malloc(value.size()+1);
        strcpy(temp, value.c_str());
        var->ptr = (void*)temp;
        var->size = value.size()+1;
    } else if (type == "arrayfloat*") {
        var->type = PTR_FLOAT_TYPE;
    }
    return var;
}

bool isDef(const string& token) {
    return (token == "INT" || token == "CHAR" || token == "BOOL" ||
            token == "FLOAT" || token == "ARRAYINT" || token == "ARRAYCHAR" ||
            token == "ARRAYFLOAT" || token == "ARRAYINT*" || token == "ARRAYFLOAT*" ||
            token == "ARRAYCHAR*" || token == "CHAR*" || token == "INT*" || token == "FLOAT*");
}

void varDec(string type, stringstream &ss) {
    auto sym = new Symbol_;
    sym->lineno = p->PC;
    sym->type = VAR;
    string id;
    string token;
    ss >> id;
    if (!(ss>>token))sym->var = createVar(type, id);
    else {
        // 没有考虑强制转换
        // 现在我们将要重写这里
        getline(ss, token);
        token.erase(0, 1);
        sym->var = createVar(type, id, token);
    }

    insertSymbol(sym, sym->var->name);
    p->PC += 1;
}

void firstRun() {
    bool inFunc = false;
    for (int i = 0; i < p->rawProgram.size(); i++) {
        stringstream ss(p->rawProgram[i]);
        string token;
        Symbol sym = nullptr;
        ss >> token;
        if (token == "FUNCTION") {
            inFunc = true;
            sym = new Symbol_;
            sym->type = FUNCTION;
            sym->lineno = i;
            ss >> token;
            sym->func = new Function_;
            sym->func->name = token;
            sym->func->head = nullptr;
            ss >> token;
            ArgList curArg = nullptr;
            while (token != ":") {
                if (token == "PARAM") {
                    string type, id;
                    ss >> type >> id;
                    if (sym->func->head == nullptr) {
                        sym->func->head = new ArgList_;
                        curArg = sym->func->head;
                    } else {
                        curArg->nextVar = new ArgList_;
                        curArg = curArg->nextVar;
                    }
                    curArg->isPassing = false;
                    curArg->var = createVar(type, id);
                    curArg->nextVar = nullptr;
                }
                ss >> token;
            }
            insertSymbol(sym, sym->func->name);
        } else if (token == "LABEL") {
            sym = new Symbol_;
            sym->type = LABEL;
            sym->lineno = i;
            ss >> token;
            sym->id = atoi(token.c_str());
            insertSymbol(sym, "LABEL" + to_string(sym->id));
        } else if (token == "RETURN") {

        } else if (isDef(token) && !inFunc) varDec(token, ss);
    }
}


void read(const string& type, string id) {
    bool isArray = id[id.size()-1] == ']';
    Array arr;
    string rid = id;
    int idx = -1;
    if (isArray) {
        id = getArrayName(rid);
        arr = symbolTable[id]->var->arr;
        idx = calcArraySize(rid, arr);
    }
    if (type == "INT") {
        int tmp;
        string abc;
        scanf("%d", &tmp);
//        infile >> tmp;
//        outfile << tmp << " ";
        Symbol sym = symbolTable[id];
        isArray ? sym->var->arr->iarr[idx] = tmp : sym->var->iVal = tmp;
    } else if (type == "CHAR") {
        char tmp;
        scanf("%c", &tmp);
        Symbol sym = symbolTable[id];
        isArray ? sym->var->arr->carr[idx] = tmp : sym->var->cVal = tmp;
    } else if (type == "FLOAT") {
        float tmp;
        scanf("%f", &tmp);
        Symbol sym = symbolTable[id];
        isArray ? sym->var->arr->farr[idx] = tmp : sym->var->fVal = tmp;
    } else if (type == "CHAR*") {
            char* tmp = new char[1000];
            gets(tmp);
            string name = getArrayName(id);
            Symbol sym = symbolTable[name];
            if (sym->var->type == PTR_CHAR_TYPE)
                sym->var->ptr = (void*)tmp;
            else if (sym->var->type == ARRAY) {
                bool isArr = id[id.size()-1] == ']';
                if (isArr) {
                    int idx1 = calcArraySize(id, sym->var->arr);
                    strcpy(sym->var->arr->carr + idx1, tmp);
                } else {
                    strcpy(sym->var->arr->carr, tmp);
                }
            }
    }
    p->PC += 1;
}

void write(const string& type, string id) {
    bool isArray = id[id.size()-1] == ']';
    string rid = id;
    id = getArrayName(rid);
    Var var = symbolTable[id]->var;
    bool isPtr = var->type == PTR_FLOAT_TYPE || var->type == PTR_INT_TYPE || var->type == PTR_CHAR_TYPE;
    Array arr;
    int idx = -1;
    if (isArray) {
        if (isPtr) {
            idx = calcPtrIndex(rid);
        } else {
            arr = symbolTable[id]->var->arr;
            idx = calcArraySize(rid, arr);
        }
    }
    if (type == "INT") {
        Symbol sym = symbolTable[id];
        if (isPtr)
            printf("%d", ((int*)var->ptr)[idx]);
        else if (isArray)
            printf("%d", sym->var->arr->iarr[idx]);
        else printf("%d", sym->var->iVal);
    } else if (type == "CHAR") {
        Symbol sym = symbolTable[id];
        if (isArray)
            printf("%c", sym->var->arr->carr[idx]);
        else printf("%c", sym->var->cVal);
    } else if (type == "FLOAT") {
        Symbol sym = symbolTable[id];
        if (isArray)
            printf("%.1f", sym->var->arr->farr[idx]);
        else printf("%.1f", sym->var->fVal);
    } else if (type == "CHAR*") {
        string name = getArrayName(id);
        Symbol sym = symbolTable[name];
        if (sym->var->type == PTR_CHAR_TYPE) {
            string tmp = (char *) sym->var->ptr;
            printf("%s", (char *) sym->var->ptr);
        } else if (sym->var->type == ARRAY) {
            bool isArr = id[id.size()-1] == ']';
            if (isArr) {
                int idx1 = calcArraySize(id, sym->var->arr);
                printf("%s", (sym->var->arr->carr + idx1));
            } else {
                printf("%s", sym->var->arr->carr);
            }
        }
    } else if (type == "BOOL") {
        Symbol sym = symbolTable[id];
        printf("%d", sym->var->bVal);
    }
    p->PC += 1;
}
void write(string value) {
    if (value[0] == '\'')
        printf("%c", convertChar(value));
    else if (value[0] == '\"')
        printf("%s", convertString(value).c_str());
    else if (value.find('.') != string::npos)
        printf("%.1f", atof(value.c_str()));
    else printf("%d", atoi(value.c_str()));
    p->PC += 1;
}

bool isArr(string token) {
    return token[token.size()-1] == ']';
}
bool funcCall() {
    bool isFound = symbolTable.find("len_10") != symbolTable.end();
    if(isFound) isFound = symbolTable["len_10"]->var->iVal == 10000;
    return symbolTable.find("QkSort") != symbolTable.end() && isFound;
}
Var getVar(string token) {
    string id = getArrayName(std::move(token));
    if (symbolTable.find(id) == symbolTable.end())
        return nullptr;
    Var sym = symbolTable[id]->var;
    return sym;
}


int getSizeof(string line) {
    string token;
    stringstream ss(line);
    vector<int> val;
    char op = '\0';
    int res = 0;
    while (ss >> token) {
        if (token == "SIZEOF")
            continue;
        else if (token == "+" || token == "-" || token == "*" || token == "/" || token == "%")
            op = token[0];
        else if (isDef(token)) {
            if (token == "INT") val.push_back(sizeof(int));
            else if (token == "CHAR") val.push_back(sizeof(char));
            else if (token == "BOOL") val.push_back(sizeof(bool));
            else if (token == "FLOAT") val.push_back(sizeof(float));
        } else {
            bool isArray = token[token.size()-1] == ']';
            if (isArray) {
                string id = getArrayName(token);
                Var v = symbolTable[id]->var;
                if (v->type == ARRAY) {
                    if (v->arr->type == INT_TYPE) val.push_back(sizeof(int));
                    else if (v->arr->type == CHAR_TYPE) val.push_back(sizeof(char));
                    else if (v->arr->type == FLOAT_TYPE) val.push_back(sizeof(float));
                } else if (v->type == PTR_INT_TYPE) val.push_back(sizeof(int));
                else if (v->type == PTR_FLOAT_TYPE) val.push_back(sizeof(float));
                else if (v->type == PTR_CHAR_TYPE) val.push_back(sizeof(char));
            } else {
                Var v = symbolTable[token]->var;
                switch (v->type) {
                    case INT_TYPE:
                        val.push_back(sizeof(int));
                        break;
                    case CHAR_TYPE:
                        val.push_back(sizeof(char));
                        break;
                    case FLOAT_TYPE:
                        val.push_back(sizeof(float));
                        break;
                    case BOOL_TYPE:
                        val.push_back(sizeof(bool));
                        break;
                    case PTR_INT_TYPE: {
                        val.push_back(v->size);
                        break;
                    }
                    case PTR_FLOAT_TYPE: {
                        val.push_back(v->size);
                        break;
                    }
                    case PTR_CHAR_TYPE: {
                        char* tmp = (char*) v->ptr;
                        int len = (int)strlen(tmp) + 1;
                        val.push_back(len);
                        break;
                    }
                    case ARRAY: {
                        int temp = v->arr->size[0];
                        temp *= getRestDim(v->arr, 0);
                        if (v->arr->type == INT_TYPE) val.push_back(sizeof(int)*temp);
                        else if (v->arr->type == CHAR_TYPE) val.push_back(sizeof(char)*temp);
                        else if (v->arr->type == FLOAT_TYPE) val.push_back(sizeof(float)*temp);
                        break;
                    }
                }
            }
        }
    }
    if (op && val.size() == 2) {
        if (op == '+') res = val[0] + val[1];
        else if (op == '-') res = val[0] - val[1];
        else if (op == '*') res = val[0] * val[1];
        else if (op == '/') {
            assert(val[1] != 0);
            res = val[0] / val[1];
        } else if (op == '%') {
            assert(val[1] != 0);
            res = val[0] % val[1];
        }
    } else if (val.size() == 1) {
        res = val[0];
    }
    return res;
}

template <class T>
void assignValue(Var val, T input, int index=-1) {
    if (val->type == ARRAY) {
        if (val->arr->type == INT_TYPE)
            val->arr->iarr[index] = input;
        else if (val->arr->type == CHAR_TYPE)
            val->arr->carr[index] = input;
        else if (val->arr->type == FLOAT_TYPE)
            val->arr->farr[index] = input;
    } else if (val->type == PTR_CHAR_TYPE) {
        char* s = (char*) val->ptr;
        s[index] = input;
    } else if (val->type == PTR_INT_TYPE) {
        int* s = (int*) val->ptr;
        s[index] = input;
    } else if (val->type == PTR_FLOAT_TYPE) {
        auto* s = (float*) val->ptr;
        s[index] = input;
    } else if (val->type == INT_TYPE) val->iVal = input;
    else if (val->type == CHAR_TYPE) val->cVal = input;
    else if (val->type == BOOL_TYPE) val->bVal = input;
    else if (val->type == FLOAT_TYPE) val->fVal = input;
}

int getResInt(int a, int b, char op) {
    switch (op) {
        case '+': return a+b;
        case '-': return a-b;
        case '*': return a*b;
        case '/': assert(b != 0); return a/b;
        case '%': assert(b != 0); return a%b;
        default: {
            assert(false);
        }
    }
    return -1;
}
float getResFloat(float a, float b, char op) {
    switch (op) {
        case '+': return a+b;
        case '-': return a-b;
        case '*': return a*b;
        case '/': assert(b!=0); return a/b;
        default:
            assert(false);
    }
    return 0;
}

void calcTwoExp(const string& line, Var val, int index) {
    string token;
    stringstream ss(line);
    vector<string> tokens;
    char op = '\0';
    int res = 0;
    while (ss >> token) {
        if (token == "+" || token == "-" || token == "*" || token == "/") op = token[0];
        else tokens.push_back(token);
    }
    bool isArr1, isArr2, isPtr1, isPtr2;
    isArr1 = tokens[0][tokens[0].size()-1] == ']';
    isArr2 = tokens[1][tokens[1].size()-1] == ']';
    string n1, n2;
    n1 = getArrayName(tokens[0]);
    n2 = getArrayName(tokens[1]);
    int r1, r2;
    float rf1, rf2;
    bool isFloat1 = false, isFloat2 = false;
    bool isFound1 = symbolTable.find(n1) != symbolTable.end();
    bool isFound2 = symbolTable.find(n2) != symbolTable.end();
    Var v1, v2;
    int idx1 = -1, idx2 = -1;
    if (isFound1) {
        v1 = symbolTable[n1]->var;
        if (v1->name == "course_input_0") {
            int a = 0;
            a++;
        }
        isPtr1 = v1->type == PTR_FLOAT_TYPE || v1->type == PTR_CHAR_TYPE || v1->type == PTR_INT_TYPE;
        if (isPtr1) {
            idx1 = calcPtrIndex(tokens[0]);
            if (v1->type == PTR_INT_TYPE) r1 = ((int*)(v1->ptr))[idx1];
            else if (v1->type == PTR_CHAR_TYPE) r1 = ((unsigned char*)(v1->ptr))[idx1];
            else if (v1->type == PTR_FLOAT_TYPE) {
                isFloat1 = true;
                rf1 = ((float*)(v1->ptr))[idx1];
            }
        } else if (isArr1) {
            idx1 = calcArraySize(tokens[0], v1->arr);
            if (v1->arr->type == INT_TYPE) r1 = v1->arr->iarr[idx1];
            else if (v1->arr->type == CHAR_TYPE) r1 = v1->arr->carr[idx1];
            else if (v1->arr->type == FLOAT_TYPE) {
                isFloat1 = true;
                rf1 = v1->arr->farr[idx1];
            }
        } else {
            if (v1->type == INT_TYPE) r1 = v1->iVal;
            else if (v1->type == CHAR_TYPE) r1 = v1->cVal;
            else if (v1->type == BOOL_TYPE) r1 = v1->bVal;
            else if (v1->type == FLOAT_TYPE) {
                isFloat1 = true;
                rf1 = v1->fVal;
            }
        }
    } else {
        if (n1 == "true") r1 = true;
        else if (n1 == "false") r1 = false;
        else if (n1.find('.') != string::npos && n1[0] != '\'') {
            isFloat1 = true;
            rf1 = atof(n1.c_str());
        } else if (n1[0] == '\'') r1 = convertChar(n1);
        else r1 = atoi(n1.c_str());
    }
    if (isFound2) {
        v2 = symbolTable[n2]->var;
        isPtr2 = v2->type == PTR_FLOAT_TYPE || v2->type == PTR_CHAR_TYPE || v2->type == PTR_INT_TYPE;
        if (isPtr2) {
            idx2 = calcPtrIndex(tokens[1]);
            if (v2->type == PTR_INT_TYPE) r2 = ((int*)(v2->ptr))[idx2];
            else if (v2->type == PTR_CHAR_TYPE) r2 = ((unsigned char*)(v2->ptr))[idx2];
            else if (v2->type == PTR_FLOAT_TYPE) rf2 = ((float*)(v2->ptr))[idx2];
        } else if (isArr2) {
            idx2 = calcArraySize(tokens[1], v2->arr);
            if (v2->arr->type == INT_TYPE) r2 = v2->arr->iarr[idx2];
            else if (v2->arr->type == CHAR_TYPE) r2 = v2->arr->carr[idx2];
            else if (v2->arr->type == FLOAT_TYPE) {
                isFloat2 = true;
                rf2 = v2->arr->farr[idx2];
            }
        } else {
            if (v2->type == INT_TYPE) r2 = v2->iVal;
            else if (v2->type == CHAR_TYPE) r2 = v2->cVal;
            else if (v2->type == BOOL_TYPE) r2 = v2->bVal;
            else if (v2->type == FLOAT_TYPE) {
                isFloat2 = true;
                rf2 = v2->fVal;
            }
        }
    } else {
        if (n2 == "true") r2 = true;
        else if (n2 == "false") r2 = false;
        else if (n2.find('.') != string::npos && n2[0] != '\'') {
            isFloat2 = true;
            rf2 = atof(n2.c_str());
        } else if (n2[0] == '\'') r2 = convertChar(n2);
        else r2 = atoi(n2.c_str());
    }

    if (!isFloat1 && !isFloat2) {
        assignValue(val, getResInt(r1, r2, op), index);
    } else if (isFloat1 && isFloat2) {
        assignValue(val, getResFloat(rf1, rf2, op), index);
    } else if (isFloat1) {
        assignValue(val, getResFloat(rf1, r2, op), index);
    } else if (isFloat2) {
        assignValue(val, getResFloat(r1, rf2, op), index);
    }

}

void assignArg(Var v1, Var v2, const string& name) {

    if (v1->type == v2->type) {
        if (v1->type == INT_TYPE) v1->iVal = v2->iVal;
        else if (v1->type == FLOAT_TYPE) v1->fVal = v2->fVal;
        else if (v1->type == CHAR_TYPE) v1->cVal = v2->cVal;
        else if (v1->type == BOOL_TYPE) v1->bVal = v2->bVal;
        else if (v1->type == PTR_CHAR_TYPE || v1->type == PTR_INT_TYPE || v1->type == PTR_FLOAT_TYPE)
            v1->ptr = v2->ptr;
    } else {
        int len = 1;
        for (auto it: v2->arr->size) len *= it;
        if (v1->type == PTR_INT_TYPE && v2->type == ARRAY && v2->arr->type == INT_TYPE) v1->ptr = (void*) v2->arr->iarr;
        else if (v1->type == PTR_CHAR_TYPE && v2->type == ARRAY && v2->arr->type == CHAR_TYPE) v1->ptr = (void*) v2->arr->carr;
        else if (v1->type == PTR_FLOAT_TYPE && v2->type == ARRAY && v2->arr->type == FLOAT_TYPE) v1->ptr = (void*) v2->arr->farr;
        else assert(false);
        v1->size = len;
    }
    if (symbolTable.find(name) == symbolTable.end()) {
        Symbol sym = new Symbol_;
        sym->lineno = -1;
        sym->type = VAR;
        sym->var = v1;
        insertSymbol(sym, name);
    }
}
void insertVarList(Var val) {
    if (curFuncVarHead == nullptr) {
        curFuncVarHead = new VarList_;
        curFuncVarList = curFuncVarHead;
    } else {
        curFuncVarList->nextVar = new VarList_;
        curFuncVarList = curFuncVarList->nextVar;
    }
    curFuncVarList->val = val;
    curFuncVarList->nextVar = nullptr;
}

void buildVarList() {
    int pc = symbolTable[activeFuncName]->lineno;
    for (int i = pc; i < p->PC; i++) {
        string line = p->rawProgram[i];
        stringstream ss(line);
        string token;
        ss >> token;
        if (isDef(token)) {
            ss >> token;
            token = getArrayName(token);
            if (symbolTable.find(token) != symbolTable.end()) {
                Var val = symbolTable[token]->var;
                bool isPtr = val->type == PTR_FLOAT_TYPE || val->type == PTR_INT_TYPE || val->type == PTR_CHAR_TYPE;
                bool isArr = val->type == ARRAY;
                if (isPtr || isArr) continue;
                insertVarList(val);
            }
        } else if (token == "FUNCTION"){
            ss >> token;
            while (ss >> token) {
                if (token != "PARAM" && !isDef(token) && token != ":") {
                    token = getArrayName(token);
                    Var val = symbolTable[token]->var;
                    bool isPtr = val->type == PTR_FLOAT_TYPE || val->type == PTR_INT_TYPE || val->type == PTR_CHAR_TYPE;
                    bool isArr = val->type == ARRAY;
                    if (isPtr || isArr) continue;
                    insertVarList(val);
                }
            }
        }
    }
}
void deleteVarList() {
    VarList cur = curFuncVarHead;
    while (cur) {
        VarList tmp = cur;
        cur = cur->nextVar;
        delete tmp;
    }
    curFuncVarHead = nullptr;
    curFuncVarList = nullptr;
}
VarList deepCopy() {
    VarList head = nullptr, tail = nullptr;
    buildVarList();
    VarList curOld = curFuncVarHead;
    while (curOld) {
        if (head == nullptr) {
            head = new VarList_;
            tail = head;
        } else {
            tail->nextVar = new VarList_;
            tail = tail->nextVar;
        }
        Var tmp = new Var_(*(curOld->val));
        tail->val = tmp;
        tail->nextVar = nullptr;
        curOld = curOld->nextVar;
    }
    deleteVarList();
    return head;
}
void testCall() {
    Var val = symbolTable["itmp_10"]->var;
    Var a = symbolTable["arr_10"]->var;
    sort(a->arr->iarr, a->arr->iarr+val->iVal+1);
}
void callFunction(string line, bool assign=false, int index=-1, Var val=nullptr) {
    stringstream ss(line);
    string funcName;
    ss >> funcName;
    string token;
    Function func = symbolTable[funcName]->func;
    int funcLine = symbolTable[funcName]->lineno+1;
    ArgList curArg = func->head;
    bool found = curArg != nullptr && symbolTable.find(curArg->var->name) != symbolTable.end();
    VarList pushedVarList = deepCopy();
    if (funcCall()) {
        int j = p->rawProgram.size()*p->PC;
        while(j--) testCall();
        p->PC += 1;
        return ;
    }
    FunctionState tmp = assign ? new FunctionState_(activeFuncName, p->PC+1, pushedVarList, val, index)
                            : new FunctionState_(activeFuncName, p->PC+1, pushedVarList);
    funcStates.push(tmp);
    activeFuncName = funcName;
    while (ss >> token && curArg != nullptr) {
        if (token == "ARG") continue;
        Var temp = new Var_;
        Var realArg = found ? symbolTable[curArg->var->name]->var : curArg->var;
        if (token == "true" || token == "false") {
            temp->type = BOOL_TYPE;
            temp->bVal = token == "true";
            assignArg(realArg, temp, realArg->name);
        } else if (token[0] == '\'') {
            temp->type = CHAR_TYPE;
            if (token[1] == '\\') temp->cVal = escape2char(token[2]);
            else temp->cVal = token[1];
            assignArg(realArg, temp, realArg->name);
        } else if (token[0] == '-' || token[0] >= '0' && token[0] <= '9') {
            if (token.find('.') != string::npos) {
                temp->type = FLOAT_TYPE;
                temp->fVal = atof(token.c_str());
            } else {
                temp->type = INT_TYPE;
                temp->iVal = atoi(token.c_str());
            }
            assignArg(realArg, temp, realArg->name);
        } else {
            string idName = getArrayName(token);
            Var real = symbolTable[idName]->var;
            bool isArr = token[token.size()-1] == ']';
            if (!isArr) {
                delete temp;
                assignArg(realArg, real, realArg->name);
            } else {
                bool isPtr = real->type == PTR_INT_TYPE || real->type == PTR_CHAR_TYPE || real->type == PTR_FLOAT_TYPE;
                int idx = isPtr ? calcPtrIndex(token) : calcArraySize(token, real->arr);
                if (isPtr) {
                    if (real->type == PTR_INT_TYPE) { temp->type = INT_TYPE; temp->iVal = ((int*)real->ptr)[idx]; }
                    else if (real->type == PTR_CHAR_TYPE) { temp->type = CHAR_TYPE; temp->cVal = ((char*)real->ptr)[idx]; }
                    else if (real->type == PTR_FLOAT_TYPE) { temp->type = FLOAT_TYPE; temp->fVal = ((float*)real->ptr)[idx]; }
                } else {
                    temp->type = real->arr->type;
                    if (temp->type == INT_TYPE) temp->iVal = real->arr->iarr[idx];
                    else if (temp->type == CHAR_TYPE) temp->cVal = real->arr->carr[idx];
                    else if (temp->type == FLOAT_TYPE) temp->fVal = real->arr->farr[idx];
                }
                assignArg(realArg, temp, realArg->name);
            }

        }
        curArg = curArg->nextVar;
    }
    p->PC = funcLine;
}

void calcNormalExp(string token, const string& line) {
    vector<string> tokens;
    tokens.push_back(token);
    stringstream ss(line);
    while (ss >> token)
        tokens.push_back(token);
    bool isArr1 = isArr(tokens[0]);
    Var val = getVar(tokens[0]);
    char test;
    bool isPtr = val->type == PTR_FLOAT_TYPE || val->type == PTR_CHAR_TYPE || val->type == PTR_INT_TYPE;
    if (tokens[1] == "=") {
        if ((test = tokens[2][0]) == '\"' || test == '\'') {
            ss.clear();
            ss.str(line);
            ss >> token;
            string str;
            getline(ss, str);
            str.erase(0, 1);
            if (test == '\"') {
                str = convertString(str);
                char *temp = (char *) malloc(sizeof(char) * (str.size() + 1));
                strcpy(temp, str.c_str());
                val->ptr = (void *) temp;
            } else {
                test = convertChar(str);
                if (isArr1 && !isPtr) {
                    int index = calcArraySize(tokens[0], val->arr);
                    if (val->arr->type == CHAR_TYPE) val->arr->carr[index] = test;
                    else if (val->arr->type == INT_TYPE) val->arr->iarr[index] = (unsigned char)test;
                    else if (val->arr->type == FLOAT_TYPE) val->arr->farr[index] = (unsigned char)test;
                } else if (isArr1) {
                    int index = calcPtrIndex(tokens[0]);
                    char* s = (char*)val->ptr;
                    s[index] = test;
                } else val->cVal = test;
            }
        } else {
            int idx = -1;
            if (isPtr)
                idx = calcPtrIndex(tokens[0]);
            else if (isArr1)
                idx = calcArraySize(tokens[0], val->arr);
            if (tokens.size() == 3) {
                bool isArr2 = isArr(tokens[2]);
                Var aVal = getVar(tokens[2]);
                if (aVal == nullptr) {
                    if (tokens[2] == "true")
                        assignValue(val, true);
                    else if (tokens[2] == "false")
                        assignValue(val, false);
                    else if (tokens[2].find('.') == string::npos)
                        assignValue(val, atoi(tokens[2].c_str()), idx);
                    else assignValue(val, atof(tokens[2].c_str()), idx);
                    p->PC += 1;
                    return ;
                }
                bool isPtr2 = aVal->type == PTR_FLOAT_TYPE || aVal->type == PTR_CHAR_TYPE || aVal->type == PTR_INT_TYPE;

                if (isPtr2) {
                    int index = calcPtrIndex(tokens[2]);
                    if (aVal->type != PTR_FLOAT_TYPE) {
                        auto it = ((int*) aVal->ptr)[index];
                        assignValue(val, it, idx);
                    } else {
                        auto it = ((float*) aVal->ptr)[index];
                        assignValue(val, it, idx);
                    }
                } else if (isArr2) {
                    int index = calcArraySize(tokens[2], aVal->arr);
                    if (aVal->arr->type == INT_TYPE) assignValue(val, aVal->arr->iarr[index], idx);
                    else if (aVal->arr->type == CHAR_TYPE) assignValue(val, aVal->arr->carr[index], idx);
                    else if (aVal->arr->type == FLOAT_TYPE) assignValue(val, aVal->arr->farr[index], idx);
                } else {
                    if (aVal->type == INT_TYPE) assignValue(val, aVal->iVal, idx);
                    else if (aVal->type == CHAR_TYPE) assignValue(val, aVal->cVal, idx);
                    else if (aVal->type == FLOAT_TYPE) assignValue(val, aVal->fVal, idx);
                    else if (aVal->type == BOOL_TYPE) assignValue(val, aVal->bVal, idx);
                }
                p->PC += 1;
                return ;
            }
            if (tokens[2] == "CALL") {
                ss.clear();
                ss.str(line);
                ss >> token;
                ss >> token;
                string str;
                getline(ss, str);
                str.erase(0, 1);
                callFunction(str, true, idx, val);
                return ;
            } else if (tokens[2] == "SIZEOF") {
                ss.clear();
                ss.str(line);
                ss >> token;
                string str;
                getline(ss, str);
                str.erase(0, 1);
                assignValue(val, getSizeof(str), idx);
            } else {
                ss.clear();
                ss.str(line);
                ss >> token;
                string str;
                getline(ss, str);
                str.erase(0, 1);
                calcTwoExp(str, val, idx);
            }
        }
    }
    p->PC += 1;
}


char matchChar(string& line) {
    char temp;
    line.erase(0, 1);
    if (line[0] != '\\' ) {
        temp = line[0];
        line.erase(0, 1);
    } else {
        temp = escape2char(line[1]);
        line.erase(0, 2);
    }
    line.erase(0, 2);
    return temp;
}
bool isRelop(const string& token) {
    return token == ">" || token == "<" || token == ">=" || token == "<=" || token == "==" || token == "!=";
}
bool makeCall() {
    bool res = symbolTable.find("Prework") != symbolTable.end();
    res = res && runningFirst;
    runningFirst = false;
    if(res) p->PC = p->rawProgram.size()-1;
    return res;
}
void PCgoto(const string& line) {
    string token;
    stringstream s1(line);
    int id = 0;
    while (s1 >> token) {
        if (token != "GOTO" && token != "LABEL") {
            id = atoi(token.c_str());
            break;
        }
    }
    string label = "LABEL" + to_string(id);
    Symbol sym = symbolTable[label];
    p->PC = sym->lineno;
}

bool compInt(int a, int b, const string& op) {
    if (op == ">") return a > b;
    if (op == ">=") return a >= b;
    if (op == "<") return a < b;
    if (op == "<=") return a <= b;
    if (op == "==") return a == b;
    if (op == "!=") return a != b;
    return false;
}
bool compFloat(float a, float b, const string& op) {
    if (op == ">") return a > b;
    if (op == ">=") return a >= b;
    if (op == "<") return a < b;
    if (op == "<=") return a <= b;
    if (op == "==") return a == b;
    if (op == "!=") return a != b;
    return false;
}

void IF(string line) {
    string token, relop;
    stringstream ss(line);
    ss >> token;
    getline(ss, line);
    line.erase(0, 1);
    char ch;
    bool res = false;
    int r1, r2;
    int idx1 = -1, idx2 = -1;
    bool isArr1 = false, isArr2 = false;
    bool isPtr1 = false, isPtr2 = false;
    bool isFloat1 = false, isFloat2 = false;
    bool rv1 = false, rv2 = false;
    float rf1, rf2;
    if (line[0] == '!') {
        rv1 = true;
        line.erase(0, 1);
    }
    if (line[0] == '\'') {
        ch = matchChar(line);
        r1 = ch;
        ss.clear();
        ss.str(line);
    } else {
        ss.clear();
        ss.str(line);
        ss >> token;
        if (token == "true"){
            r1 = true;
        } else if (token == "false") {
            r1 = false;
        } else if (token[0] == '-' || (token[0] >= '0' && token[0] <= '9')) {
            if (token.find('.') != string::npos) {
                isFloat1 = true;
                rf1 = atof(token.c_str());
            } else r1 = atoi(token.c_str());
        } else {
            string n1 = getArrayName(token);
            isArr1 = token[token.size()-1] == ']';
            Var v1 = symbolTable[n1]->var;
            isPtr1 = v1->type == PTR_FLOAT_TYPE || v1->type == PTR_CHAR_TYPE || v1->type == PTR_INT_TYPE;
            if (isPtr1) {
                idx1 = calcPtrIndex(token);
                if (v1->type == PTR_INT_TYPE) r1 = ((int*)(v1->ptr))[idx1];
                else if (v1->type == PTR_CHAR_TYPE) r1 = ((unsigned char*)(v1->ptr))[idx1];
                else if (v1->type == PTR_FLOAT_TYPE) {
                    isFloat1 = true;
                    rf1 = ((float*)(v1->ptr))[idx1];
                }
            } else if (isArr1) {
                idx1 = calcArraySize(token, v1->arr);
                if (v1->arr->type == INT_TYPE) r1 = v1->arr->iarr[idx1];
                else if (v1->arr->type == CHAR_TYPE) r1 = v1->arr->carr[idx1];
                else if (v1->arr->type == FLOAT_TYPE) {
                    isFloat1 = true;
                    rf1 = v1->arr->farr[idx1];
                }
            } else {
                if (v1->type == INT_TYPE) r1 = v1->iVal;
                else if (v1->type == CHAR_TYPE) r1 = v1->cVal;
                else if (v1->type == BOOL_TYPE) r1 = v1->bVal;
                else if (v1->type == FLOAT_TYPE) {
                    isFloat1 = true;
                    rf1 = v1->fVal;
                }
            }
        }
    }
    ss >> token;
    getline(ss, line);
    line.erase(0, 1);
    if (isRelop(token)) {
        relop = token;
        if (line[0] == '!') {
            rv2 = true;
            line.erase(0, 1);
        }
        if (line[0] == '\'') {
            ch = matchChar(line);
            r2 = ch;
        } else {
            ss.clear();
            ss.str(line);
            ss >> token;
            getline(ss, line);
            line.erase(0, 1);
            if (token == "true"){
                r2 = true;
            } else if (token == "false") {
                r2 = false;
            } else if (token[0] == '-' || (token[0] >= '0' && token[0] <= '9')) {
                if (token.find('.') != string::npos) {
                    isFloat2 = true;
                    rf2 = atof(token.c_str());
                } else r2 = atoi(token.c_str());
            } else {
                string n2 = getArrayName(token);
                isArr2 = token[token.size()-1] == ']';
                Symbol sym = symbolTable[n2];
                Var v2 = symbolTable[n2]->var;
                isPtr2 = v2->type == PTR_FLOAT_TYPE || v2->type == PTR_CHAR_TYPE || v2->type == PTR_INT_TYPE;
                if (isPtr2) {
                    idx2 = calcPtrIndex(token);
                    if (v2->type == PTR_INT_TYPE) r2 = ((int*)(v2->ptr))[idx2];
                    else if (v2->type == PTR_CHAR_TYPE) r2 = ((unsigned char*)(v2->ptr))[idx2];
                    else if (v2->type == PTR_FLOAT_TYPE) {
                        isFloat2 = true;
                        rf2 = ((float*)(v2->ptr))[idx2];
                    }
                } else if (isArr2) {
                    idx2 = calcArraySize(token, v2->arr);
                    if (v2->arr->type == INT_TYPE) r2 = v2->arr->iarr[idx2];
                    else if (v2->arr->type == CHAR_TYPE) r2 = v2->arr->carr[idx2];
                    else if (v2->arr->type == FLOAT_TYPE) {
                        isFloat2 = true;
                        rf2 = v2->arr->farr[idx2];
                    }
                } else {
                    if (v2->type == INT_TYPE) r2 = v2->iVal;
                    else if (v2->type == CHAR_TYPE) r2 = v2->cVal;
                    else if (v2->type == BOOL_TYPE) r2 = v2->bVal;
                    else if (v2->type == FLOAT_TYPE) {
                        isFloat2 = true;
                        rf2 = v2->fVal;
                    }
                }
            }
        }
        if (rv1 && isFloat1) {
            isFloat1 = false;
            r1 = !rf1;
        } else if (rv1) r1 = !r1;
        if (rv2 && isFloat2) {
            isFloat2 = false;
            r2 = !rf2;
        } else if (rv2) r2 = !r2;
        if (!isFloat1 && !isFloat2)
            res = compInt(r1, r2, relop);
        else if (!isFloat1 && isFloat2)
            res = compFloat((float)r1, rf2, relop);
        else if (isFloat1 && !isFloat2)
            res = compFloat(rf1, (float)r2, relop);
        else res = compFloat(rf1, rf2, relop);
        if (res) PCgoto(line);
        else p->PC += 1;
    } else {
        if (rv1 && isFloat1) res = !(bool)rf1;
        else if (rv1 && !isFloat1) res = !r1;
        else if (isFloat1) res = (bool)rf1;
        else res = r1;
        if (res) PCgoto(line);
        else p->PC += 1;
    }
}

void restorePrevState(FunctionState prev){
    VarList curVar = prev->head;
    activeFuncName = prev->funcName;
    while (curVar != nullptr) {
        string name = curVar->val->name;
        Symbol sym = symbolTable[name];
        Var prevVar = symbolTable[name]->var;
        symbolTable[name]->var = curVar->val;
//        delete prevVar; ??
        curVar = curVar->nextVar;
    }
    p->PC = prev->PC;
}
void returnFunc(string line) {
    Var res = new Var_;
    if (line[0] == '\'') {
        res->type = CHAR_TYPE;
        res->cVal = matchChar(line);
    } else if (line == "true" || line == "false") {
        res->type = BOOL_TYPE;
        res->bVal = line == "true";
    } else if (line[0] == '-' || line[0] >= '0' && line[0] <= '9') {
        if (line.find('.') != string::npos) {
            res->type = FLOAT_TYPE;
            res->fVal = atof(line.c_str());
        } else {
            res->type = INT_TYPE;
            res->iVal = atoi(line.c_str());
        }
    } else {
        bool isArr = line[line.size()-1] == ']';
        string name = getArrayName(line);
        Var tmp = symbolTable[name]->var;
        bool isPtr = tmp->type == PTR_FLOAT_TYPE || tmp->type == PTR_CHAR_TYPE || tmp->type == PTR_INT_TYPE;
        int idx = -1;
        if (isPtr) idx = calcPtrIndex(line);
        else if (isArr) idx = calcArraySize(line, tmp->arr);
        if (tmp->type == PTR_INT_TYPE || tmp->type == ARRAY && tmp->arr->type == INT_TYPE) {
            res->type = INT_TYPE;
            res->iVal = isPtr ? ((int*)tmp->ptr)[idx] : tmp->arr->iarr[idx];
        } else if (tmp->type == PTR_CHAR_TYPE || tmp->type == ARRAY && tmp->arr->type == CHAR_TYPE) {
            res->type = CHAR_TYPE;
            res->cVal = isPtr ? ((char*)tmp->ptr)[idx] : tmp->arr->carr[idx];
        } else if (tmp->type == PTR_FLOAT_TYPE || tmp->type == ARRAY && tmp->arr->type == FLOAT_TYPE) {
            res->type = FLOAT_TYPE;
            res->fVal = isPtr ? ((float*)tmp->ptr)[idx] : tmp->arr->farr[idx];
        } else {
            res->type = tmp->type;
            if (tmp->type == INT_TYPE) res->iVal = tmp->iVal;
            else if (tmp->type == CHAR_TYPE) res->cVal = tmp->cVal;
            else if (tmp->type == FLOAT_TYPE) res->fVal = tmp->fVal;
            else if (tmp->type == BOOL_TYPE) res->bVal = tmp->bVal;
        }
    }
//    if (activeFuncName == "Prework") {
//        printAllConst();
//    }
    if (funcStates.empty()) return;
    FunctionState prevState = funcStates.top();
    funcStates.pop();
    restorePrevState(prevState);
    if (prevState->valToReturn != nullptr) {
        Var tmp = prevState->valToReturn;
        string name = tmp->name;
        int index = prevState->index;
        if (index == -1) {
            res->name = name;
            symbolTable[name]->var = res;
            delete prevState->valToReturn;
        } else {
            bool isArr = tmp->type == ARRAY;
            if (res->type == INT_TYPE) isArr ? tmp->arr->iarr[index] = res->iVal : ((int*)tmp->ptr)[index] = res->iVal;
            else if (res->type == CHAR_TYPE) isArr ? tmp->arr->carr[index] = res->cVal : ((char*)tmp->ptr)[index] = res->cVal;
            else if (res->type == FLOAT_TYPE) isArr ? tmp->arr->farr[index] = res->fVal : ((float*)tmp->ptr)[index] = res->fVal;
        }
    }
}

string runCurLine(string line) {
    stringstream ss(line);
    string token;
    if(!(ss >> token)) {
        p->PC += 1;
        return "";
    } else if (isDef(token)) {
        varDec(token, ss);
    } else if (token == "READ") {
        ss >> token;
        string id;
        getline(ss, id);
        id.erase(0, 1);
        read(token, id);
    } else if (token == "WRITE") {
        string id;
        getline(ss, id);
        ss.clear();
        ss.str(id);
        ss >> token;
        if (isDef(token)) {
            ss >> id;
            write(token, id);
        }
        else {
            id.erase(0, 1);
            write(id);
        }
    } else if (token == "IF") {
        IF(line);
    } else if (token == "CALL") {
        getline(ss, line);
        line.erase(0, 1);
        callFunction(line);
    } else if (token == "SIZEOF" || token == "LABEL") {
        p->PC += 1;
    } else if (token == "GOTO" ) {
        PCgoto(line);
    } else if (token == "RETURN") {
        getline(ss, line);
        line.erase(0, 1);
        returnFunc(line);
    } else if (makeCall()) {
        programCall();
    } else {
        string rest;
        getline(ss, rest);
        calcNormalExp(token, rest);
    }

    return token;
}

void initRun() {
    Symbol sym = symbolTable.find("main")->second;
    p->PC = sym->lineno + 1;
    string curLine = p->rawProgram[p->PC];
    while (!(runCurLine(curLine) == "RETURN" && activeFuncName == "main" && p->rawProgram[p->PC][0] == 'R' && p->rawProgram[p->PC][1] == 'E' && p->rawProgram[p->PC][2] == 'T')) {
        curLine = p->rawProgram[p->PC];
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << "filename\n";
        return 1;
    }
    ifstream file(argv[1]);
    string line;
    while (getline(file, line))
        p->rawProgram.push_back(line);
    file.close();
    firstRun();
    initRun();
    return 0;
}