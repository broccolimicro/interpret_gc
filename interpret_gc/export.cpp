#include "export.h"
#include <common/standard.h>
#include <common/text.h>

#include <interpret_arithmetic/export.h>

namespace gc {

const bool debug = false;

parse_gc::rule export_rule(const gc::GuardedCommand &rule, const gc::GuardedCommands &rules) {
	parse_gc::setup_expressions();
	
	parse_gc::rule result;
	result.valid = true;
	/*result.keep = rules.nets[net].keep;
	result.weak = attr.weak;
	result.force = attr.force;
	result.pass = attr.pass;*/
	if (not rule.assume.isValid()) {
		result.assume = arithmetic::export_expression<parse_gc::expression>(rule.assume, rules);
	}
	/*if (attr.delay_max != attributes().delay_max) {
		result.after = attr.delay_max;
	}*/
	if (not rule.guard.isValid()) {
		result.implicant = arithmetic::export_expression<parse_gc::expression>(rule.guard, rules);
	}
	result.action = arithmetic::export_composition<parse_gc::simple_composition>(rule.action, rules);
	if (debug) cout << result.to_string() << endl;
	return result;
}

parse_gc::rule_set export_rule_set(const gc::GuardedCommands &rules) {
	parse_gc::setup_expressions();
	parse_gc::rule_set result;
	result.valid = true;

	/*if (pr.assume_nobackflow) {
		result.assume.push_back("nobackflow");
	}
	if (pr.assume_static) {
		result.assume.push_back("static");
	}

	if (pr.require_driven) {
		result.require.push_back("driven");
	}
	if (pr.require_stable) {
		result.require.push_back("stable");
	}
	if (pr.require_noninterfering) {
		result.require.push_back("noninterfering");
	}
	if (pr.require_adiabatic) {
		result.require.push_back("adiabatic");
	}*/

	for (int i = 0; i < (int)rules.rules.size(); i++) {
		result.rules.push_back(export_rule(rules.rules[i], rules));
	}

	return result;
}

}
