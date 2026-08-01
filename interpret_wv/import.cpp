#include "import.h"

#include <parse_expression/import.h>
#include <interpret_arithmetic/import_default.h>

#include <filesystem>

namespace parse_ucs {

ExpressionImporter::ExpressionImporter(ucs::Netlist symbols, int region, bool autoDefine) : symbols(symbols) {
	this->region.push_back(region);
	this->autoDefine = autoDefine;
}

ExpressionImporter::~ExpressionImporter() {
}

arithmetic::Expression ExpressionImporter::import_term(const parse_expression::expression::argument &syntax, tokenizer *tokens) const {
	if (syntax.type < 0 or syntax.type >= (int)expression_config::cfg->literals.size() or not syntax.ptr) {
		return arithmetic::Expression::undef();
	}

	std::string type = expression_config::cfg->literals[syntax.type].first;

	if (type == "constant") {
		std::string value = syntax.ptr->get<constant>().value;
		return arithmetic::import_constant(value, tokens);
	} else if (type == "literal") {
		std::string name = syntax.ptr->get<literal>().name;
		if (region.back() != 0) {
			name += "'" + std::to_string(region.back());
		}
		return arithmetic::import_literal(name, symbols, tokens, autoDefine);
	} else if (type == "type") {
		std::string name = syntax.ptr->get<type_name>().to_string("");
		return arithmetic::Expression::typeOf(name);
	} else if (type == "term") {
		std::string name = syntax.ptr->get<type_name>().to_string("");
		return arithmetic::Expression::termOf(name);
	} else if (type == "label") {
		std::string value = syntax.ptr->get<label>().value;
		return arithmetic::import_constant(value, tokens);
	}
	internal("", "unsupported literal type '" + type + "'", __FILE__, __LINE__);
	return arithmetic::Expression::undef();
}

void ExpressionImporter::push_properties(parse_expression::operation op, const vector<parse_expression::expression::argument> &args, tokenizer *tokens) {
	if (op.is("", "'", "", "")) { // Region
		int value = -1;
		if (args.size() == 2u) {
			std::string str = args[1].ptr->to_string("");
			value = atoi(str.c_str());
		} else {
			error("", "operator ''' expects 2 arguments, found '" + ::to_string(args.size()) + "'", __FILE__, __LINE__);
		}
		this->region.push_back(value);
	}
}

void ExpressionImporter::pop_properties(parse_expression::operation op) {
	if (op.is("", "'", "", "")) { // Region
		region.pop_back();
	}
}

arithmetic::Expression ExpressionImporter::import_unary(parse_expression::operation op, arithmetic::Expression expr, tokenizer *tokens) const {
	if (op.is("!", "", "", "")) {
		return !expr;
	} else if (op.is("~", "", "", "")) {
		return ~expr;
	} else if (op.is("+", "", "", "")) {
		return expr;
	} else if (op.is("-", "", "", "")) {
		return -expr;
	}
	return expr;
}

arithmetic::Expression ExpressionImporter::import_binary(parse_expression::operation op, arithmetic::Expression left, arithmetic::Expression right, tokenizer *tokens) const {
	if (op.is("", "", "|", "")) {
		return left | right;
	} else if (op.is("", "", "&", "")) {
		return left & right;
	} else if (op.is("", "", "^", "")) {
		return left ^ right;
	} else if (op.is("", "", "||", "")) {
		return left || right;
	} else if (op.is("", "", "&&", "")) {
		return left && right;
	} else if (op.is("", "", "^^", "")) {
		return booleanXor(left, right);
	} else if (op.is("", "", "==", "")) {
		return left == right;
	} else if (op.is("", "", "!=", "")) {
		return left != right;
	} else if (op.is("", "", "<", "")) {
		return left < right;
	} else if (op.is("", "", ">", "")) {
		return left > right;
	} else if (op.is("", "", "<=", "")) {
		return left <= right;
	} else if (op.is("", "", ">=", "")) {
		return left >= right;
	} else if (op.is("", "", "<<", "")) {
		return left << right;
	} else if (op.is("", "", ">>", "")) {
		return left >> right;
	} else if (op.is("", "", "+", "")) {
		return left + right;
	} else if (op.is("", "", "-", "")) {
		return left - right;
	} else if (op.is("", "", "*", "")) {
		return left * right;
	} else if (op.is("", "", "/", "")) {
		return left / right;
	} else if (op.is("", "", "%", "")) {
		return left % right;
	}
	internal("", "unrecognized operation", __FILE__, __LINE__);
	return left;
}

/*
TODO(edward.bingham) we don't have arithmetic support for conditionals inside expressions

arithmetic::Expression ExpressionImporter::import_ternary(parse_expression::operation op, std::vector<arithmetic::Expression> args, tokenizer *tokens) const {
	if (op.is("", "?", ":", "")) {
		return left | right;
	}
	internal("", "unrecognized operation", __FILE__, __LINE__);
	return left;
}*/


arithmetic::Expression ExpressionImporter::import_group(parse_expression::operation op, vector<arithmetic::Expression> args, tokenizer *tokens) const {
	if (op.is("[", "", ",", "]")) {
		return arithmetic::array(args);
	}
	internal("", "unrecognized operation", __FILE__, __LINE__);
	return arithmetic::Expression();
}

arithmetic::Expression ExpressionImporter::import_modifier(parse_expression::operation op, vector<arithmetic::Expression> args, tokenizer *tokens) const {
	// TODO(edward.bingham) See the parser, operator :: is not yet implemented

	/*if (op.is("", "!", "", "")) {     // Channel Send
		if (not args.empty()) {
			return arithmetic::memberCall("send", args);
		} else {
			error(__FILE__, __LINE__, nullptr, nullptr, "operator '!' expects at least one operand");
			return arithmetic::memberCall("send", {arithmetic::Expression::undef()});
		}
	// TODO(edward.bingham) This operator is specific to QDI languages (CHP, HSE, PRS, COG)
	} else*/ if (op.is("", "'", "", "")) { // Region
		// only affects properties
		return args[0];
	} else if (op.is("", ".", "", "")) { // Member
		return arithmetic::Expression(arithmetic::Operation::MEMBER, args);
	// DESIGN(edward.bingham) Move "this" into the first argument of the
	// function. So "a.b.c(d, e) becomes c(a.b, d, e). This seems like a
	// reasonable way to simplify things, and follows the early style of c++
	// function names.
	} else if (op.is("", "(", ",", ")")) { // Call, Validity, Truthiness
		if (args.empty()) {
			error("", "function call expects function name", __FILE__, __LINE__);
			return arithmetic::Expression();
		}

		// Replace member calls
		if (args[0].top.isExpr() and args[0].getExpr(args[0].top.index)->func == arithmetic::Operation::MEMBER) {
			arithmetic::Operation op = *args[0].getExpr(args[0].top.index);

			arithmetic::Operand name = op.operands.back();
			op.operands.pop_back();

			if (op.operands.size() == 1u) {
				op.func = arithmetic::Operation::IDENTITY;
			}
			args[0].setExpr(op);
			args.insert(args.begin(), name);

			return arithmetic::Expression(arithmetic::Operation::MEMBER_CALL, args);
		// Replace built-in functions
		} else if (args[0].top.isConst() and args[0].top.cnst.type == arithmetic::Value::STRING and args[0].top.cnst.sval == "valid") {
			if      (args.size() == 1u) { return arithmetic::Expression::vdd(); }
			else if (args.size() == 2u) { return arithmetic::isValid(args[1]); }
			else { error("", "valid() function expects 1 argument, found " + ::to_string(args.size()-1), __FILE__, __LINE__); }
		} else if (args[0].top.isConst() and args[0].top.cnst.type == arithmetic::Value::STRING and args[0].top.cnst.sval == "true") {
			if      (args.size() == 1u) { return arithmetic::Expression::boolOf(true); }
			else if (args.size() == 2u) { return arithmetic::isTrue(args[1]); }
			else { error("", "true() function expects 1 argument, found " + ::to_string(args.size()-1), __FILE__, __LINE__); }
		} else {
			return arithmetic::Expression(arithmetic::Operation::CALL, args);
		}
	// END DESIGN
	} else if (op.is("", "[", ":", "]")) {
		return arithmetic::Expression(arithmetic::Operation::INDEX, args);
	}
	internal("", "unrecognized operation", __FILE__, __LINE__);
	return arithmetic::Expression();
}

arithmetic::Expression import_expression(const parse_expression::expression &syntax, ucs::Netlist nets, tokenizer *tokens, int region, bool auto_define) {
	return ExpressionImporter(nets, region, auto_define).import_expression(syntax, tokens);
}

CompositionImporter::CompositionImporter(ucs::Netlist symbols, int region, bool autoDefine) : symbols(symbols) {
	this->region.push_back(region);
	this->autoDefine = autoDefine;
}

CompositionImporter::~CompositionImporter() {
}

arithmetic::Action CompositionImporter::import_assignment(const assignment &syntax, tokenizer *tokens) const {
	ExpressionImporter in(symbols, region.back(), autoDefine);

	arithmetic::Action result;
	if (syntax.operation.empty()) {
		result.lvalue = arithmetic::Expression::undef();
		if (syntax.left[0].valid) {
			result.rvalue = in.import_expression(syntax.left[0], tokens);
		}
	} else if (syntax.operation == "+") {
		if (syntax.left.size() > 0) {
			result.lvalue = in.import_expression(syntax.left[0], tokens);
		}
		result.rvalue = arithmetic::Expression::vdd();
	} else if (syntax.operation == "-") {
		if (syntax.left.size() > 0) {
			result.lvalue = in.import_expression(syntax.left[0], tokens);
		}
		result.rvalue = arithmetic::Expression::gnd();
	} else if (syntax.operation == "=") {
		if (syntax.left.size() > 0) {
			result.lvalue = in.import_expression(syntax.left[0], tokens);
		}
		if (syntax.right.valid) {
			result.rvalue = in.import_expression(syntax.right, tokens);
		}
	}
	return result;
}

arithmetic::Choice CompositionImporter::import_term(const parse_expression::expression::argument &syntax, tokenizer *tokens) const {
	if (syntax.type < 0 or syntax.type >= (int)expression_config::cfg->literals.size() or not syntax.ptr) {
		return arithmetic::Choice();
	}

	return arithmetic::Choice({{import_assignment(syntax.ptr->get<assignment>(), tokens)}});
}

void CompositionImporter::push_properties(parse_expression::operation op, const vector<parse_expression::expression::argument> &args, tokenizer *tokens) {
	if (op.is("", "'", "", "")) { // Region
		int value = -1;
		if (args.size() == 2u) {
			std::string str = args[1].ptr->to_string("");
			value = atoi(str.c_str());
		} else {
			error("", "operator ''' expects 2 arguments, found '" + ::to_string(args.size()) + "'", __FILE__, __LINE__);
		}
		this->region.push_back(value);
	}
}

void CompositionImporter::pop_properties(parse_expression::operation op) {
	if (op.is("", "'", "", "")) { // Region
		region.pop_back();
	}
}

arithmetic::Choice CompositionImporter::import_modifier(parse_expression::operation op, vector<arithmetic::Choice> args, tokenizer *tokens) const {
	if (op.is("", "'", "", "")) {
		return args[0];
	}
	return parse_expression::Importer<arithmetic::Choice>::import_modifier(op, args, tokens);
}

arithmetic::Choice CompositionImporter::import_binary(parse_expression::operation op, arithmetic::Choice left, arithmetic::Choice right, tokenizer *tokens) const {
	if (op.is("", "", ":", "")) {
		return left | right;
	} else if (op.is("", "", ",", "")) {
		return left & right;
	}
	internal("", "unrecognized operation", __FILE__, __LINE__);
	return left;
}

arithmetic::Action import_assignment(const assignment &syntax, ucs::Netlist nets, tokenizer *tokens, int region, bool auto_define) {
	return CompositionImporter(nets, region, auto_define).import_assignment(syntax, tokens);
}

arithmetic::Choice import_composition(const parse_expression::expression &syntax, ucs::Netlist nets, tokenizer *tokens, int region, bool auto_define) {
	return CompositionImporter(nets, region, auto_define).import_expression(syntax, tokens);
}

/*bool Binder::define(vector<string> typeName, string name, vector<int> size, ucs::Netlist nets) {
	weaver::TypeId type = prgm.findType(currModule, typeName);
	Instance newInst(type, name, size);
	if (prgm.mods[currModule].terms[currTerm].symb.define(newInst)) {
		vector<int> i;
		i.resize(size.size(), 0);
		return true;
	}
	return false;
}

void Binder::pushScope() {
	prgm.mods[currModule].terms[currTerm].symb.pushScope();
}

void Binder::popScope() {
	prgm.mods[currModule].terms[currTerm].symb.popScope();
}*/

std::vector<int> import_arrays(const std::vector<parse_ucs::expression> &syntax, tokenizer *tokens) {
	vector<int> size;
	for (auto i = syntax.begin(); i != syntax.end(); i++) {
		string cnst = i->to_string("");
		if (cnst.find(".") != string::npos or cnst.find("e") != string::npos) {
			printf("error: constant value must be integer '%s'\n", cnst.c_str());
			size.push_back(-1);
		} else {
			size.push_back(std::stoi(cnst));
		}
	}
	return size;
}

weaver::Instance import_instance(weaver::TypeId type, const parse_ucs::variable_t<parse_ucs::expression> &syntax, tokenizer *tokens) {
	// TODO(edward.bingham) reset behavior
	return weaver::Instance(type, syntax.name, import_arrays(syntax.size, tokens));
}

bool import_declaration(vector<weaver::Instance> &result, const weaver::Program &prgm, int modIdx, const parse_ucs::function::declaration &syntax, tokenizer *tokens) {
	weaver::TypeId type = prgm.findType(syntax.type.mod, syntax.type.name, modIdx);
	if (not type.defined()) {
		printf("error: type not defined '%s'\n", syntax.type.to_string().c_str());
		return false;
	}

	for (int k = 0; k < (int)syntax.name.size(); k++) {
		result.push_back(import_instance(type, syntax.name[k], tokens));
	}
	return true;
}

weaver::Decl import_prototype(const weaver::Program &prgm, int modIdx, const parse_ucs::prototype &syntax, weaver::TypeId recvType, tokenizer *tokens) {
	weaver::TypeId retType = prgm.findType(syntax.ret.mod, syntax.ret.name, modIdx);
	vector<weaver::Instance> args;
	for (auto i = syntax.args.begin(); i != syntax.args.end(); i++) {
		import_declaration(args, prgm, modIdx, *i, tokens);
	}

	return weaver::Decl(syntax.name, args, retType, recvType);
}

void import_symbols(weaver::Program &prgm, int modIdx, const parse_ucs::source &syntax, tokenizer *tokens) {
	for (auto i = syntax.types.begin(); i != syntax.types.end(); i++) {
		//int recvType = prgm.mods[modIdx].createType(Type::typeOf(i->name));
		prgm.mods[modIdx].createType(weaver::Type::typeOf(i->name));
	}
}

weaver::Typename import_type_signature(const parse_ucs::type_signature &syntax, tokenizer *tokens) {
	weaver::Typename result;
	result.mod = syntax.type.mod;
	result.name = syntax.type.name;
	result.size = import_arrays(syntax.size, tokens);
	return result;
}

weaver::Prototype import_signature(const parse_ucs::signature &syntax, tokenizer *tokens) {
	weaver::Prototype result;
	result.mod = syntax.recv.mod;
	result.name = syntax.name;
	result.recv = syntax.recv.name;
	result.qualified = syntax.qualified;
	for (auto i = syntax.args.begin(); i != syntax.args.end(); i++) {
		result.args.push_back(import_type_signature(*i, tokens));
	}
	if (syntax.qualified) {
		result.hashArgs();
	}
	return result;
}

weaver::Decl import_decl(weaver::Program &prgm, int modIdx, const parse_ucs::function_decl &syntax, tokenizer *tokens) {
	weaver::TypeId recvType;
	if (not syntax.recv.empty()) {
		recvType = prgm.findType("", syntax.recv, modIdx);
		if (not recvType.defined()) {
			printf("error: type not defined '%s'\n", syntax.recv.c_str());
			return weaver::Decl();
		}
	}

	weaver::TypeId retType;
	if (syntax.ret.valid) {
		retType = prgm.findType(syntax.ret.mod, syntax.ret.name, modIdx);
		if (not retType.defined()) {
			printf("error: type not defined '%s'\n", syntax.ret.to_string().c_str());
			return weaver::Decl();
		}
	}

	vector<weaver::Instance> args;
	for (auto j = syntax.args.begin(); j != syntax.args.end(); j++) {
		import_declaration(args, prgm, modIdx, *j, tokens);
	}

	return weaver::Decl(syntax.name, args, retType, recvType);
}

void import_term(const weaver::Project &proj, weaver::Program &prgm, weaver::Module &mod, int modIdx, const parse_ucs::function &syntax, tokenizer *tokens) {
	weaver::Decl decl = import_decl(prgm, modIdx, syntax.decl, tokens);

	weaver::TermId id(modIdx);
	id.index = mod.createTerm(weaver::Term(decl));

	const weaver::Dialect *dialect = proj.getDialect(syntax.lang);
	if (dialect != nullptr and dialect->load != nullptr and syntax.body) {
		std::any def = dialect->load(decl.name, syntax.body.get(), tokens);

		id.var = mod.terms[id.index].createVariant(weaver::Variant(syntax.lang, def));
	}

	for (auto i = syntax.impl.begin(); i != syntax.impl.end(); i++) {
		weaver::Prototype proto = import_signature(*i, tokens);
		vector<weaver::TermId> implTerm = prgm.findTerms(proto, modIdx);
		if (implTerm.empty()) {
			printf("error: term not defined '%s'\n", proto.to_string().c_str());
		} else if (implTerm.size() != 1u) {
			printf("error: term not unique '%s'\n", proto.to_string().c_str());
		} else {
			mod.terms[id.index].impl.push_back(implTerm.back());
		}
	}
}

void import_module(const weaver::Project &proj, weaver::Program &prgm, int modIdx, const parse_ucs::source &syntax, tokenizer *tokens) {
	for (auto i = syntax.types.begin(); i != syntax.types.end(); i++) {
		weaver::TypeId recvType = prgm.findType("", i->name, modIdx);
		for (auto j = i->members.begin(); j != i->members.end(); j++) {
			import_declaration(prgm.typeAt(recvType).members, prgm, modIdx, *j, tokens);
		}

		for (auto j = i->protocols.begin(); j != i->protocols.end(); j++) {
			prgm.typeAt(recvType).methods.push_back(import_prototype(prgm, modIdx, *j, recvType, tokens));
		}
	}

	for (auto i = syntax.funcs.begin(); i != syntax.funcs.end(); i++) {
		import_term(proj, prgm, prgm.mods[modIdx], modIdx, *i, tokens);
	}
}

void import_modfile(weaver::Project &proj, const parse_ucs::modfile &syntax, tokenizer *tokens) {
	for (auto i = syntax.deps.begin(); i != syntax.deps.end(); i++) {
		for (auto j = i->path.begin(); j != i->path.end(); j++) {
			proj.depends.push_back(weaver::Depend{j->first, j->second});
		}
	}

	for (auto i = syntax.attrs.begin(); i != syntax.attrs.end(); i++) {
		if (i->name == "module") {
			proj.modName = i->value.substr(1, i->value.size()-2);
		} else if (i->name == "tech") {
			proj.setTech(i->value.substr(1, i->value.size()-2));
		}
	}
}

}
