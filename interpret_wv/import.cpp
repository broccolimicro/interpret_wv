#include "import.h"
#include <interpret_arithmetic/import.h>

#include <filesystem>

namespace weaver {

/*bool Binder::define(vector<string> typeName, string name, vector<int> size, ucs::Netlist nets) {
	TypeId type = prgm.findType(currModule, typeName);
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
		string cnst = arithmetic::import_constant(*i, tokens);
		if (cnst.find(".") != string::npos or cnst.find("e") != string::npos) {
			printf("error: constant value must be integer '%s'\n", cnst.c_str());
			size.push_back(-1);
		} else {
			size.push_back(std::stoi(cnst));
		}
	}
	return size;
}

Instance import_instance(TypeId type, const parse_ucs::variable_t<parse_ucs::expression> &syntax, tokenizer *tokens) {
	// TODO(edward.bingham) reset behavior
	return Instance(type, syntax.name, import_arrays(syntax.size, tokens));
}

bool import_declaration(vector<Instance> &result, const Program &prgm, int modIdx, const parse_ucs::function::declaration &syntax, tokenizer *tokens) {
	TypeId type = prgm.findType(syntax.type.mod, syntax.type.name, modIdx);
	if (not type.defined()) {
		printf("error: type not defined '%s'\n", syntax.type.to_string().c_str());
		return false;
	}

	for (int k = 0; k < (int)syntax.name.size(); k++) {
		result.push_back(import_instance(type, syntax.name[k], tokens));
	}
	return true;
}

Decl import_prototype(const Program &prgm, int modIdx, const parse_ucs::prototype &syntax, TypeId recvType, tokenizer *tokens) {
	TypeId retType = prgm.findType(syntax.ret.mod, syntax.ret.name, modIdx);
	vector<Instance> args;
	for (auto i = syntax.args.begin(); i != syntax.args.end(); i++) {
		import_declaration(args, prgm, modIdx, *i, tokens);
	}

	return Decl(syntax.name, args, retType, recvType);
}

void import_symbols(Program &prgm, int modIdx, const parse_ucs::source &syntax, tokenizer *tokens) {
	for (auto i = syntax.types.begin(); i != syntax.types.end(); i++) {
		//int recvType = prgm.mods[modIdx].createType(Type::typeOf(i->name));
		prgm.mods[modIdx].createType(Type::typeOf(i->name));
	}
}

Typename import_type_signature(const parse_ucs::type_signature &syntax, tokenizer *tokens) {
	weaver::Typename result;
	result.mod = syntax.type.mod;
	result.name = syntax.type.name;
	result.size = import_arrays(syntax.size, tokens);
	return result;
}

Prototype import_signature(const parse_ucs::signature &syntax, tokenizer *tokens) {
	weaver::Prototype result;
	result.mod = syntax.recv.mod;
	result.name = syntax.name;
	result.recv = syntax.recv.name;
	result.unqualified = syntax.unqualified;
	for (auto i = syntax.args.begin(); i != syntax.args.end(); i++) {
		result.args.push_back(import_type_signature(*i, tokens));
	}
	return result;
}

void import_term(const Language &lang, Program &prgm, Module &mod, int modIdx, const parse_ucs::function &syntax, tokenizer *tokens) {
	TypeId recvType;
	if (not syntax.recv.empty()) {
		recvType = prgm.findType("", syntax.recv, modIdx);
		if (not recvType.defined()) {
			printf("error: type not defined '%s'\n", syntax.recv.c_str());
			return;
		}
	}

	TypeId retType;
	if (syntax.ret.valid) {
		retType = prgm.findType(syntax.ret.mod, syntax.ret.name, modIdx);
		if (not retType.defined()) {
			printf("error: type not defined '%s'\n", syntax.ret.to_string().c_str());
			return;
		}
	}

	vector<Instance> args;
	for (auto j = syntax.args.begin(); j != syntax.args.end(); j++) {
		import_declaration(args, prgm, modIdx, *j, tokens);
	}

	TermId id(modIdx);
	id.index = mod.createTerm(Term(syntax.name, args, retType, recvType));

	auto dialect = lang.dialects.find(syntax.lang);
	if (dialect != lang.dialects.end()) {
		std::any index = dialect->second(prgm.getLib(syntax.lang), syntax.name, syntax.body, tokens);

		id.var = mod.terms[id.index].createVariant(Variant(syntax.lang, index));
	}

	for (auto i = syntax.impl.begin(); i != syntax.impl.end(); i++) {
		Prototype proto = import_signature(*i, tokens);
		vector<TermId> implTerm = prgm.findTerms(proto, modIdx);
		if (implTerm.empty()) {
			printf("error: term not defined '%s'\n", proto.to_string().c_str());
		} else if (implTerm.size() != 1u) {
			printf("error: term not unique '%s'\n", proto.to_string().c_str());
		} else {
			mod.terms[id.index].impl.push_back(implTerm.back());
		}
	}
}

void import_module(const Language &lang, Program &prgm, int modIdx, const parse_ucs::source &syntax, tokenizer *tokens) {
	for (auto i = syntax.types.begin(); i != syntax.types.end(); i++) {
		TypeId recvType = prgm.findType("", i->name, modIdx);
		for (auto j = i->members.begin(); j != i->members.end(); j++) {
			import_declaration(prgm.typeAt(recvType).members, prgm, modIdx, *j, tokens);
		}

		for (auto j = i->protocols.begin(); j != i->protocols.end(); j++) {
			prgm.typeAt(recvType).methods.push_back(import_prototype(prgm, modIdx, *j, recvType, tokens));
		}
	}

	for (auto i = syntax.funcs.begin(); i != syntax.funcs.end(); i++) {
		import_term(lang, prgm, prgm.mods[modIdx], modIdx, *i, tokens);
	}
}

void import_modfile(Project &proj, const parse_ucs::modfile &syntax, tokenizer *tokens) {
	for (auto i = syntax.deps.begin(); i != syntax.deps.end(); i++) {
		for (auto j = i->path.begin(); j != i->path.end(); j++) {
			proj.depends.push_back(Depend{j->first, j->second});
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
