#pragma once

#include <cstdio>

namespace gracli {

/**
 * @brief The value by which rule ids are offset in the symbol vectors of rules.
 * The values 0 to 255 (extended ASCII) are used for terminals, while values starting at 256 are used for rule ids.
 * The value 256 refers to rule 0, 257 refers to rule 1 et cetera.
 */
static const size_t RULE_OFFSET = 256;

} // namespace gracli
