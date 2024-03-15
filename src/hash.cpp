#include "hash.hpp"

std::unordered_map<std::string, Entry> symbolTable;
Entry layersHead;

void initLayers() {
    layersHead = new Entry_;
    Entry globalLayer = new Entry_;
    layersHead->hashNext = globalLayer;
}

void insertSymbol(Entry symbol) {
    bool found = (symbolTable.find(symbol->name) != symbolTable.end());
    Entry tail = found ? symbolTable[symbol->name] : nullptr;
    symbolTable[symbol->name] = symbol;
    symbol->hashNext = tail;
    
    Entry curLayer = layersHead->hashNext;
    tail = curLayer->layerNext;
    curLayer->layerNext = symbol;
    symbol->layerNext = tail;
}

Entry findSymbolAll(const std::string& name) {
    auto it = symbolTable.find(name);
    bool found = (it != symbolTable.end());
    if (found) {
        Entry curEntry = it->second;
        while (curEntry != nullptr) {
            if (curEntry->type != nullptr && curEntry->type->kind != FUNC)
                return curEntry;
            curEntry = curEntry->hashNext;
        }
    }
    return nullptr;
}

Entry findSymbolLayer(const std::string& name) {
    Entry curLayer = layersHead->hashNext;
    Entry symbol = curLayer->layerNext;
    while (symbol != nullptr) {
        if (symbol->name == name && symbol->type->kind != FUNC)
            break;
        symbol = symbol->layerNext;
    }
    return symbol;
}

Entry findSymbolFunc(const std::string& name) {
    auto it = symbolTable.find(name);
    bool found = (it != symbolTable.end());
    if (found) {
        Entry curEntry = it->second;
        while (curEntry != nullptr) {
            if (curEntry->type->kind == FUNC)
                return curEntry;
            curEntry = curEntry->hashNext;
        }
    }
    return nullptr;
}

void delSymbol(const std::string& name) {
    auto it = symbolTable.find(name);
    bool found = (it != symbolTable.end());
    if (found) {
        Entry tmp = it->second;
        Entry tmpNext = tmp->hashNext;
        if (tmpNext == nullptr) symbolTable.erase(name);
        else symbolTable[name] = tmpNext;
    }
}

void pushLayer() {
    auto newLayer = new Entry_;
    Entry tail = layersHead->hashNext;
    layersHead->hashNext = newLayer;
    newLayer->hashNext = tail;
}

void popLayer() {
    Entry curLayer = layersHead->hashNext;
    layersHead->hashNext = curLayer->hashNext;
    Entry symbol = curLayer->layerNext;
    while (symbol != nullptr) {
        delSymbol(symbol->name);
        symbol = symbol->layerNext;
    }
}

arrayDim getArrayDim(Type arr) {
    if(arr->kind != ARRAY && arr->kind != PTR)
        return nullptr;
    if (arr->kind == ARRAY) {
        auto tmp = new ptrDim_;
        tmp->curDimSize = arr->array.size;
        tmp->nextDim = arr->array.elem->kind == BASIC ? getArrayDim(arr->array.elem) : nullptr;
        return tmp;
    } else {
        return arr->ptr.head;
    }

}
int getArraySize(Type arr) {
    int tot = 1;
    if (arr->array.elem->kind != BASIC) 
        tot *= getArraySize(arr->array.elem);
    else tot *= arr->array.size;
    return tot;
}

Type getElemType(Type b) {
    if (b->array.elem->kind != BASIC)
        return getElemType(b->array.elem);
    else return b->array.elem;
}

bool checkSameDim(arrayDim aDim, arrayDim bDim) {
    if (aDim == nullptr && bDim == nullptr )
        return true;
    if (aDim->nextDim == nullptr && bDim->nextDim == nullptr &&
        aDim->curDimSize == bDim->curDimSize)
        return true;
    if (aDim->nextDim != nullptr && bDim->nextDim != nullptr &&
        aDim->curDimSize == bDim->curDimSize)
        return checkSameDim(aDim->nextDim, bDim->nextDim);
    return false;
}

bool checkSameDimType(Type a, Type b) {
    if (a->kind != ARRAY && a->kind != PTR ||
        b->kind != ARRAY && b->kind != PTR)
            return false;
    arrayDim aDim = getArrayDim(a);
    arrayDim bDim = getArrayDim(b);
    return checkSameDim(aDim, bDim);
}

bool checkPtr2Array(Type a, Type b) {
    DataType type = getElemType(b)->basic;
    switch(a->ptr.datatype) {
        case PTR_INT_TYPE: return type == INT_TYPE;
        case PTR_FLOAT_TYPE: return type == FLOAT_TYPE;
        case PTR_CHAR_TYPE: return type == CHAR_TYPE;
        default: return false;
    }
}

bool isEqualType(Type a, Type b) {
    if (a == nullptr && b == nullptr) return true;
    else if (a == nullptr || b == nullptr) return false;
    else if (a->kind == PTR || b->kind == PTR) {
        if (a->kind == PTR && b->kind == ARRAY) {
            //指针和数组类型有两种可能，一种可能是这个指针是刚刚被创建的，如int*a = b[5]和int func(int* a); int b[5]; func(b);
            //这个时候的指针虽然在初始化中被初始化为相应的指针类型，但是它对应的指针无法立刻知道对应数组长度，因此会被默认
            //赋值为-1，因此我们检查类型中，遇到-1表示这个指针是一个宝宝指针，还没有被赋值；
            //另一种可能是，这个指针之前已经被赋值了，如int* a = b[5]; 此时还会遇到检查a和b是否等价的其它情况（待补充）
            //因此需要认真考虑这个指针的长度，检查手段就是靠长度
            //正常检查手段应该是检查两个数组地址是否相等的，但是在语义分析阶段，不可能给数组分配地址。
            //这样就有一个弊端，一个指针只能表示一个数组。
            if ((a->ptr.head == nullptr || checkSameDimType(a, b)) && checkPtr2Array(a, b) )
                return true;
        } else if (a->kind == ARRAY && b->kind == PTR) {
            return isEqualType(b, a);
        } else if (a->kind == PTR && b->kind == PTR) {
            //这里为了简化，指针会给指针赋值，但是为了以后的修改方便，就留下这个位置
            return true;
        }
        return false;
    }
    else if (a->kind != b->kind) return false;
    switch (a->kind)
    {
        case BASIC: return a->basic == b->basic;
        case ARRAY: return isEqualType(a->array.elem, b->array.elem);
        case STRUCT: {
            FieldList af = a->structure->head;
            FieldList bf = b->structure->head;
            while (af != nullptr && bf != nullptr) {
                if (!isEqualType(af->type, bf->type))
                    return false;
                af = af->next;
                bf = bf->next;
            }
            if (af != nullptr || bf != nullptr)
                return false;
            return true;
        }
        case FUNC: {
            if (!isEqualType(a->function->returnType, b->function->returnType)
                || a->function->paraNum != b->function->paraNum)
                return false;
            FieldList af = a->function->head;
            FieldList bf = b->function->head;
            while (af != nullptr && bf != nullptr) {
                if (!isEqualType(af->type, bf->type))
                    return false;
                af = af->next;
                bf = bf->next;
            }
            if (af != nullptr || bf != nullptr)
                return false;
            return true;
        }
        default:
            return false;
    }
}