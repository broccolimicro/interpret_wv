#include <interpret_wv/export.h>
#include <interpret_arithmetic/export.h>

namespace weaver {

parse_ucs::function::declaration export_declaration(const Program &prgm, const Instance &inst) {
	parse_ucs::function::declaration result;
	result.valid = true;

	result.type = export_type_name(prgm, inst.type);
	parse_ucs::function::declaration::variable var;
	var.name = inst.name;
	for (int sz : inst.size) {
		var.size.push_back(arithmetic::export_expression<parse_ucs::expression>(arithmetic::Value(sz)));
	}

	result.name.push_back(var);

	return result;
}

parse_ucs::type_name export_type_name(const Program &prgm, TypeId id) {
	parse_ucs::type_name result;
	if (not prgm.typeValid(id)) {
		return result;
	}

	result.mod = prgm.modAt(id).name;
	result.name = prgm.typeAt(id).name;
	return result;
}

parse_ucs::function_decl export_decl(const Program &prgm, const Decl &decl) {
	parse_ucs::function_decl result;
	result.valid = true;

	result.name = decl.name;
	if (prgm.typeValid(decl.recv)) {
		result.recv = prgm.typeAt(decl.recv).name;
	}

	result.ret = export_type_name(prgm, decl.ret);

	for (const auto &arg : decl.args) {
		result.args.push_back(export_declaration(prgm, arg));
	}

	return result;
}

parse_ucs::modfile export_modfile(const Project &proj) {
	parse_ucs::modfile result;
	result.valid = true;
	parse_ucs::require dep;
	dep.valid = true;

	for (auto i = proj.depends.begin(); i != proj.depends.end(); i++) {
		dep.path.push_back({i->path, i->version});
	}
	result.deps.push_back(dep);

	parse_ucs::attribute modAttr;
	modAttr.valid = true;
	modAttr.name = "module";
	modAttr.value = "\"" + proj.modName + "\"";
	result.attrs.push_back(modAttr);

	parse_ucs::attribute techAttr;
	techAttr.valid = true;
	techAttr.name = "tech";
	techAttr.value = "\"" + proj.tech.path + "\"";
	result.attrs.push_back(techAttr);

	return result;
}

}
