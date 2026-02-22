#pragma once

#include <gc/guarded_command.h>

#include <parse_gc/rule.h>
#include <parse_gc/rule_set.h>

namespace gc {

parse_gc::rule export_rule(const gc::GuardedCommand &gc);
parse_gc::rule_set export_rule_set(const gc::GuardedCommands &pr);

}
