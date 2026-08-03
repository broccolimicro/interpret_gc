#include "import.h"

#include <common/standard.h>

#include <interpret_arithmetic/import_default.h>

namespace parse_gc {

const bool debug = false;

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
	} else if (type == "label") {
		std::string name = syntax.ptr->get<label>().value;
		return arithmetic::Expression::labelOf(name);
	} else if (type == "ident") {
		std::string value = syntax.ptr->get<ident>().value;
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

}

namespace gc {

void import_rule(const parse_gc::rule &syntax, gc::GuardedCommands &rules, int default_id, tokenizer *tokens, bool auto_define) {
	gc::GuardedCommand result;

	/*if (syntax.weak) {
		attr.weak = true;
	}
	if (syntax.force) {
		attr.force = true;
	}
	if (syntax.pass) {
		attr.pass = true;
	}
	// TODO(edward.bingham) I need after for the minimum delay and within for maximum delay
	if (syntax.after != std::numeric_limits<uint64_t>::max()) {
		attr.delay_max = syntax.after;
	}*/
	if (syntax.assume.valid) {
		result.assume = import_expression(syntax.assume, rules, tokens, default_id, auto_define);
	}

	if (syntax.implicant.valid) {
		result.guard = import_expression(syntax.implicant, rules, tokens, default_id, auto_define);
	}

	if (syntax.action.valid) {
		result.action = import_composition(syntax.action, rules, tokens, default_id, auto_define);
	}

	//pr.nets[uid].keep = syntax.keep;
	rules.rules.push_back(result);
}

void import_rule_set(const parse_gc::rule_set &syntax, gc::GuardedCommands &rules, int default_id, tokenizer *tokens, bool auto_define) {
	if (syntax.region != "") {
		default_id = atoi(syntax.region.c_str());
	}

	/*for (auto i = syntax.assume.begin(); i != syntax.assume.end(); i++) {
		if (*i == "nobackflow") {
			pr.assume_nobackflow = true;
		} else if (*i == "static") {
			pr.assume_static = true;
		}
	}

	for (auto i = syntax.require.begin(); i != syntax.require.end(); i++) {
		if (*i == "driven") {
			pr.require_driven = true;
		} else if (*i == "stable") {
			pr.require_stable = true;
		} else if (*i == "noninterfering") {
			pr.require_noninterfering = true;
		} else if (*i == "adiabatic") {
			pr.require_adiabatic = true;
		}
	}*/

	for (int i = 0; i < (int)syntax.rules.size(); i++) {
		import_rule(syntax.rules[i], rules, default_id, tokens, auto_define);
	}

	for (int i = 0; i < (int)syntax.regions.size(); i++) {
		import_rule_set(syntax.regions[i], rules, default_id, tokens, auto_define);
	}
}

}
