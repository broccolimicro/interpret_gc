#pragma once

#include <parse/tokenizer.h>

#include <gc/guarded_command.h>

#include <parse_gc/rule.h>
#include <parse_gc/rule_set.h>

namespace gc {

void import_rule(const parse_gc::rule &syntax, gc::GuardedCommands &rules, int default_id, tokenizer *tokens, bool auto_define);
void import_rule_set(const parse_gc::rule_set &syntax, gc::GuardedCommands &rules, int default_id, tokenizer *tokens, bool auto_define);

}
