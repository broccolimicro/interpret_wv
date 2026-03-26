#include <interpret_wv/export.h>

namespace weaver {

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
