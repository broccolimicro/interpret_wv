#pragma once

#include <parse/tokenizer.h>
#include <parse_ucs/source.h>
#include <parse_ucs/expression.h>
#include <parse_ucs/prototype.h>
#include <parse_ucs/modfile.h>

#include <parse_expression/expression.h>
#include <parse_expression/assignment.h>
#include <parse_expression/import.h>

#include <vector>
#include <string>

#include <arithmetic/expression.h>
#include <arithmetic/action.h>
#include <arithmetic/state.h>
#include <weaver/program.h>
#include <weaver/project.h>

using std::vector;
using std::string;

namespace parse_ucs {

struct ExpressionImporter : parse_expression::Importer<arithmetic::Expression> {
	ucs::Netlist symbols;
	vector<int> region;
	bool autoDefine;

	ExpressionImporter(ucs::Netlist symbols, int region = 0, bool autoDefine = false);
	~ExpressionImporter();

	arithmetic::Expression import_term(const parse_expression::expression::argument &syntax, tokenizer *tokens) const override;
	void push_properties(parse_expression::operation op, const vector<parse_expression::expression::argument> &args, tokenizer *tokens) override;
	void pop_properties(parse_expression::operation op) override;
	arithmetic::Expression import_unary(parse_expression::operation op, arithmetic::Expression expr, tokenizer *tokens) const override;
	arithmetic::Expression import_binary(parse_expression::operation op, arithmetic::Expression left, arithmetic::Expression right, tokenizer *tokens) const override;
	arithmetic::Expression import_group(parse_expression::operation op, vector<arithmetic::Expression> args, tokenizer *tokens) const override;
	arithmetic::Expression import_modifier(parse_expression::operation op, vector<arithmetic::Expression> args, tokenizer *tokens) const override;
};

arithmetic::Expression import_expression(const parse_expression::expression &syntax, ucs::Netlist nets, tokenizer *tokens, int region = 0, bool auto_define = false);

struct CompositionImporter : parse_expression::Importer<arithmetic::Choice> {
	ucs::Netlist symbols;
	vector<int> region;
	bool autoDefine;

	CompositionImporter(ucs::Netlist symbols, int region = 0, bool autoDefine = false);
	~CompositionImporter();

	arithmetic::Action import_assignment(const assignment &syntax, tokenizer *tokens) const;
	arithmetic::Choice import_term(const parse_expression::expression::argument &syntax, tokenizer *tokens) const override;
	void push_properties(parse_expression::operation op, const vector<parse_expression::expression::argument> &args, tokenizer *tokens) override;
	void pop_properties(parse_expression::operation op) override;
	arithmetic::Choice import_binary(parse_expression::operation op, arithmetic::Choice left, arithmetic::Choice right, tokenizer *tokens) const override;
	arithmetic::Choice import_modifier(parse_expression::operation op, vector<arithmetic::Choice> args, tokenizer *tokens) const override;
};

arithmetic::Action import_assignment(const assignment &syntax, ucs::Netlist nets, tokenizer *tokens, int region = 0, bool auto_define = false);
arithmetic::Choice import_composition(const parse_expression::expression &syntax, ucs::Netlist nets, tokenizer *tokens, int region = 0, bool auto_define = false);


// Managing scope
/*bool define(vector<string> typeName, string name, vector<int> size, ucs::Netlist nets);
void pushScope();
void popScope();*/

// Loading the program
bool import_declaration(vector<weaver::Instance> &result, const weaver::Program &prgm, int modIdx, const parse_ucs::function::declaration &syntax, tokenizer *tokens);
void import_type_signature(vector<weaver::Typename> &result, const parse_ucs::function::declaration &syntax, tokenizer *tokens);
weaver::Decl import_prototype(const weaver::Program &prgm, int modIdx, const parse_ucs::prototype &syntax, weaver::TypeId recvType, tokenizer *tokens);
void import_symbols(weaver::Program &prgm, int modIdx, const parse_ucs::source &syntax, tokenizer *tokens);
weaver::Typename import_type_signature(const parse_ucs::type_signature &syntax, tokenizer *tokens);
weaver::Prototype import_signature(const parse_ucs::signature &syntax, tokenizer *tokens);
weaver::Decl import_decl(const weaver::Program &prgm, int modIdx, const parse_ucs::function_decl &syntax, tokenizer *tokens);
weaver::TermId import_term(const weaver::Project &proj, weaver::Program &prgm, weaver::Module &mod, int modIdx, const parse_ucs::function &syntax, tokenizer *tokens);
std::vector<weaver::TermId> import_module(const weaver::Project &proj, weaver::Program &prgm, int modIdx, const parse_ucs::source &syntax, tokenizer *tokens);
void import_modfile(weaver::Project &proj, const parse_ucs::modfile &syntax, tokenizer *tokens);

}
