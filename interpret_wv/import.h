#pragma once

//#include <common/net.h>

#include <parse/tokenizer.h>
#include <parse_ucs/source.h>
#include <parse_ucs/expression.h>
#include <parse_ucs/prototype.h>
#include <parse_ucs/modfile.h>

#include <vector>
#include <string>

#include <weaver/program.h>
#include <weaver/project.h>

using std::vector;
using std::string;

namespace weaver {

// Managing scope
/*bool define(vector<string> typeName, string name, vector<int> size, ucs::Netlist nets);
void pushScope();
void popScope();*/

// Loading the program
bool import_declaration(vector<Instance> &result, const Program &prgm, int modIdx, const parse_ucs::function::declaration &syntax, tokenizer *tokens);
Decl import_prototype(const Program &prgm, int modIdx, const parse_ucs::prototype &syntax, TypeId recvType, tokenizer *tokens);
void import_symbols(Program &prgm, int modIdx, const parse_ucs::source &syntax, tokenizer *tokens);
void import_module(Program &prgm, int modIdx, const parse_ucs::source &syntax, tokenizer *tokens);
void import_modfile(Project &proj, const parse_ucs::modfile &syntax, tokenizer *tokens);

}
