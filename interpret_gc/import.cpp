#include "import.h"

#include <common/standard.h>

#include <interpret_arithmetic/import.h>

namespace gc {

const bool debug = false;

void import_rule(const parse_gc::rule &syntax, gc::GuardedCommands &rules, int default_id, tokenizer *tokens, bool auto_define) {
	GuardedCommand result;

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
		result.assume = arithmetic::import_expression(syntax.assume, rules, default_id, tokens, auto_define);
	}

	if (syntax.implicant.valid) {
		result.guard = arithmetic::import_expression(syntax.implicant, rules, default_id, tokens, auto_define);
	}

	if (syntax.action.valid) {
		result.action = arithmetic::import_choice(syntax.action, rules, default_id, tokens, auto_define);
	}

	//pr.nets[uid].keep = syntax.keep;
	rules.rules.push_back(result);
}

void import_rule_set(const parse_gc::rule_set &syntax, gc::GuardedCommands &rules, int default_id, tokenizer *tokens, bool auto_define)
{
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
