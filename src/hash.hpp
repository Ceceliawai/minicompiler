#pragma once
#include "semantic.hpp"
// hash table function decs

void initLayers();
void insertSymbol(Entry symbol);
Entry findSymbolAll(const std::string& name);
Entry findSymbolLayer(const std::string& name);
Entry findSymbolFunc(const std::string& name);
void delSymbol(const std::string& name);
void pushLayer();
void popLayer();
bool isEqualType(Type a, Type b);