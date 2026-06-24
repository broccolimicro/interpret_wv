#pragma once

#include <parse_ucs/modfile.h>
#include <parse_ucs/type_name.h>
#include <parse_ucs/function_decl.h>
#include <parse_ucs/function.h>

#include <weaver/project.h>
#include <weaver/program.h>

namespace weaver {

parse_ucs::function::declaration export_declaration(const Program &prgm, const Instance &inst);
parse_ucs::type_name export_type_name(const Program &prgm, TypeId id);
parse_ucs::function_decl export_decl(const Program &prgm, const Decl &decl);

parse_ucs::modfile export_modfile(const Project &proj);

}
