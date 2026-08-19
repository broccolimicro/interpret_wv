#include <interpret_wv/export.h>

#include <arithmetic/algorithm.h>
#include <common/message.h>

#include <parse/wrapper.h>

namespace parse_ucs {

ExpressionExporter::ExpressionExporter(ucs::ConstNetlist nets) : nets(nets) {
}

ExpressionExporter::~ExpressionExporter() {
}

parse_expression::operation ExpressionExporter::export_operator(int func) const {
	using OpType = arithmetic::Operation::OpType;
	using operation = parse_expression::operation;

	switch (func) {
	// VALIDITY - converted to CALL
	case OpType::WIRE_NOT: return operation("~", "", "", "");
	case OpType::WIRE_OR:  return operation("", "", "|", "");
	case OpType::WIRE_AND: return operation("", "", "&", "");
	case OpType::WIRE_XOR: return operation("", "", "^", "");
	// TRUTHINESS - converted to CALL
	case OpType::BOOLEAN_NOT: return operation("!", "", "", "");
	case OpType::BOOLEAN_OR: return operation("", "", "||", "");
	case OpType::BOOLEAN_AND: return operation("", "", "&&", "");
	case OpType::BOOLEAN_XOR: return operation("", "", "^^", "");
	case OpType::EQUAL: return operation("", "", "==", "");
	case OpType::NOT_EQUAL: return operation("", "", "!=", "");
	case OpType::LESS: return operation("", "", "<", "");
	case OpType::GREATER: return operation("", "", ">", "");
	case OpType::LESS_EQUAL: return operation("", "", "<=", "");
	case OpType::GREATER_EQUAL: return operation("", "", ">=", "");
	// NEGATIVE - converted to LESS
	case OpType::TERNARY: return operation("", "?", ":", "");
	case OpType::IDENTITY: return operation("+", "", "", "");
	case OpType::NEGATION: return operation("-", "", "", "");
	// INVERSE - converted to INTDIV
	// TODO(edward.bingham) we need type information here to determine if we are using arithmetic or logical shift
	case OpType::SHIFT_LEFT: return operation("", "", "<<", "");
	case OpType::SHIFT_RIGHT: return operation("", "", ">>", "");
	case OpType::ADD: return operation("", "", "+", "");
	case OpType::SUBTRACT: return operation("", "", "-", "");
	case OpType::MULTIPLY: return operation("", "", "*", "");
	case OpType::INTDIV: return operation("", "", "/", "");
	case OpType::INTMOD: return operation("", "", "%", "");
	case OpType::CALL: return operation("", "(", ",", ")");
	// MEMBER_CALL - converted to MEMBER and CALL
	//case OpType::CAST: return operation("", "(", "", ")");
	case OpType::ARRAY: return operation("[", "", ",", "]");
	case OpType::INDEX: return operation("", "[", ":", "]");
	//case OpType::STRUCT: return operation("'{", "", "", "}");
	case OpType::MEMBER: return operation("", ".", "", "");
	}
	return operation();
}

const parse_expression::precedence_set &ExpressionExporter::precedence() const {
	return expression_config::cfg->order;
}

parse_expression::expression::argument ExpressionExporter::export_constant(arithmetic::Value value) const {
	if (value.type == arithmetic::Value::LABEL) {
		label result;
		result.value = arithmetic::export_value(value);
		return {2, std::shared_ptr<parse::syntax>(result.clone())};
	}
	constant result;
	result.value = arithmetic::export_value(value);
	return {0, std::shared_ptr<parse::syntax>(result.clone())};
}

parse_expression::expression::argument ExpressionExporter::export_literal(size_t index) const {
	literal result;
	result.name = nets.netAt(index);
	return {1, std::shared_ptr<parse::syntax>(result.clone())};
}

parse_expression::expression export_expression(const arithmetic::Expression &expr, ucs::ConstNetlist nets) {
	return ExpressionExporter(nets).export_expression(expr);
}

parse_expression::assignment export_assignment(const arithmetic::Action &expr, ucs::ConstNetlist nets) {
	parse_expression::assignment result;
	result.valid = true;

	if (not expr.lvalue.isUndef()) {
		result.left.push_back(export_expression(expr.lvalue, nets));
	}

	// TODO(edward.bingham) we need type information about the lvalue here
	arithmetic::Operand top = expr.rvalue.top;
	if (top.isConst() and top.cnst.isNeutral()) {
		result.operation = "-";
	} else if (top.isConst() and top.cnst.isUnstable()) {
		result.operation = "~";
	} else if (top.isConst() and top.cnst.type == arithmetic::Value::WIRE and top.cnst.isValid()) {
		result.operation = "+";
	} else {
		result.right = export_expression(expr.rvalue, nets);
		result.operation = "=";
	}

	return result;
}

CompositionExporter::CompositionExporter(ucs::ConstNetlist nets) : nets(nets) {
}

CompositionExporter::~CompositionExporter() {
}

parse_expression::operation CompositionExporter::export_operator(int func) const {
	using OpType = arithmetic::Operation::OpType;
	using operation = parse_expression::operation;

	switch (func) {
	// VALIDITY - converted to CALL
	case OpType::WIRE_OR:  return operation("", "", ":", "");
	case OpType::WIRE_AND: return operation("", "", ",", "");
	}
	return operation();
}

const parse_expression::precedence_set &CompositionExporter::precedence() const {
	return composition_config::cfg->order;
}

parse_expression::expression::argument CompositionExporter::export_action(const arithmetic::Action &expr) const {
	return {1, std::shared_ptr<parse::syntax>(export_assignment(expr, nets).clone())};
}

parse_expression::expression export_composition(const arithmetic::Parallel &expr, ucs::ConstNetlist nets) {
	return CompositionExporter(nets).export_expression(expr);
}

parse_expression::expression export_composition(const arithmetic::Choice &expr, ucs::ConstNetlist nets) {
	return CompositionExporter(nets).export_expression(expr);
}


parse_ucs::function::declaration export_declaration(const weaver::Program &prgm, const weaver::Instance &inst) {
	parse_ucs::function::declaration result;
	result.valid = true;

	result.type = export_type_name(prgm, inst.type);
	parse_ucs::function::declaration::variable var;
	var.name = inst.name;
	for (int sz : inst.size) {
		var.size.push_back(export_expression(arithmetic::Expression::intOf(sz), ucs::ConstNetlist()));
	}

	result.name.push_back(var);

	return result;
}

parse_ucs::type_name export_type_name(const weaver::Program &prgm, weaver::TypeId id) {
	parse_ucs::type_name result;
	if (not prgm.typeValid(id)) {
		return result;
	}

	result.mod = prgm.modAt(id).name;
	result.name = prgm.typeAt(id).name;
	return result;
}

parse_ucs::function_decl export_decl(const weaver::Program &prgm, const weaver::Decl &decl) {
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

parse_ucs::modfile export_modfile(const weaver::Project &proj) {
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
