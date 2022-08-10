#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <sys/types.h>
#include <vector>

#include "../external/word-packing/include/word_packing.hpp"
#include "consts.hpp"
#include "grammar_tuple_coder.hpp"
#include "util.hpp"

namespace gracli {

/**
 * @brief A representation of a Grammar as a map, with rule ids as keys and vectors of unsiged integers as symbol
 * containers
 */
class Grammar {

  public:
    using Symbol = u_int32_t;
    using Rule   = word_packing::PackedIntVector<uint64_t>;

  private:
    /**
     * @brief The vector keeping the grammar's rules.
     *
     * This vector maps from the rule's id to the sequence of symbols in its right side.
     * The symbols are either the code of the character, if the symbol is a literal, or the
     * id of the rule the nonterminal belongs to offset by 256, if the symbol is a nonterminal.
     *
     * Therefore, if the value 274 is read from the symbols vector, it means that this is a nonterminal, since it is >=
     * 256. It is the nonterminal corresponding to the rule with id `274 - 256 = 18`.
     *
     * Note, that this only applies to the symbols in the innner vector and not to the indexes of the outer vector.
     */
    std::vector<Rule> m_rules;

    /**
     * @brief The id of the start rule
     */
    size_t m_start_rule_id;

  public:
    /**
     * @brief Construct an empty Grammar with a start rule of 0 and with a given capacity.
     *
     * @param capacity The number of rules the underlying vector will be setup to hold
     */
    Grammar(Symbol capacity) : m_rules{std::vector<Rule>(capacity, Rule())}, m_start_rule_id{0} {}

    Grammar(std::vector<Rule> &&rules, size_t start_rule_id) : m_rules{rules}, m_start_rule_id{start_rule_id} {}

    /**
     * @brief Reads the grammar from a file.
     *
     * @param file_path The input file
     * @return The grammar
     */
    static Grammar from_file(std::string file_path) {
        auto rules = GrammarTupleCoder::decode(file_path);
        auto n     = rules.size();
        return Grammar(std::move(rules), n - 1);
    }

    /**
     * @brief Accesses the symbols vector for the rule of the given id
     *
     * @param id The rule id whose symbols vector to access
     * @return Symbols& A reference to the rule's symbols vector
     */
    Rule &operator[](const size_t id) { return m_rules[id]; }

    /**
     * @brief Appends a terminal to the given rule's symbols vector
     *
     * @param id The id of the rule whose symbols vector should be appended to
     * @param symbol The terminal symbol to append to the vector. Note that this value should be lesser than RULE_OFFSET
     */
    void append_terminal(const size_t id, size_t symbol) { m_rules[id].push_back(symbol); }

    /**
     * @brief Appends a non terminal to the given rule's vector
     *
     * @param id The id of the rule whose symbols vector should be appended to
     * @param rule_id The id of the rule, whose non terminal should be appended to the vector. Note, that the parameter
     * should not have RULE_OFFSET added to it when passing it in.
     */
    void append_nonterminal(const size_t id, size_t rule_id) { append_terminal(id, rule_id + RULE_OFFSET); }

    /**
     * Set the right side of a rule directly. Note that this will overwrite the previous mapping
     *
     * @param id The id of the rule whose right side to set
     * @param symbols The container of the right side's symbols
     */
    void set_rule(const size_t id, Rule &&symbols) { m_rules[id] = symbols; }


private:
    void renumber_internal(const size_t rule_id, size_t &count, std::vector<Symbol> &renumbering) {
            Rule &symbols = m_rules[rule_id];
            for (auto symbol : symbols) {
                if (is_terminal(symbol) || renumbering[symbol - RULE_OFFSET] != invalid<Symbol>())
                    continue;
                renumber_internal(symbol - RULE_OFFSET, count, renumbering);
            }
            renumbering[rule_id] = count++;

  }
public:
    /**
     * @brief Renumbers the rules in the grammar in such a way that rules with index i only depend on rules with indices
     * lesser than i.
     *
     */
    void dependency_renumber() {
        if (m_rules.size() == 0) {
            return;
        }
        std::vector<Symbol> renumbering(m_rules.size(), invalid<Symbol>());
        size_t              count = 0;

        // Calculate a renumbering
        renumber_internal(m_start_rule_id, count, renumbering);
        // make count equal to the max. id
        count--;

        // renumber the rules and the nonterminals therein
        std::vector<Rule> new_rules(m_rules.size(), Rule());
        for (size_t old_id = 0; old_id < m_rules.size(); old_id++) {
            Rule &symbols = m_rules[old_id];
            // Renumber all the nonterminals in the symbols vector
            for (auto &symbol : symbols) {
                if (is_terminal(symbol))
                    continue;
                symbol = (renumbering[symbol - RULE_OFFSET]) + RULE_OFFSET;
            }
            // Put assign the rule to its new id
            new_rules[renumbering[old_id]] = std::move(symbols);
        }

        m_rules         = std::move(new_rules);
        m_start_rule_id = count;
    }

    /**
     * @brief Prints the grammar to an output stream.
     *
     * @param out An output stream to print the grammar to
     */
    void print(std::ostream &out = std::cout) {
        for (size_t id = 0; id < m_rules.size(); id++) {
            const Rule &symbols = m_rules[id];
            out << 'R' << id << " -> ";
            for (auto symbol : symbols) {
                if (Grammar::is_terminal(symbol)) {
                    auto c = (char) symbol;
                    switch (c) {
                        case '\n': {
                            out << "\\n";
                            break;
                        }
                        case '\r': {
                            out << "\\r";
                            break;
                        }
                        case '\t': {
                            out << "\\t";
                            break;
                        }
                        case '\0': {
                            out << "\\0";
                            break;
                        }
                        case ' ': {
                            out << '_';
                            break;
                        }
                        default: {
                            out << c;
                        }
                    }
                } else {
                    out << 'R' << symbol - RULE_OFFSET;
                }
                out << ' ';
            }
            out << std::endl;
        }
    }

  private:
    void expand(size_t rule_id, std::ostream &os) {
        for (auto &symbol : m_rules[rule_id]) {
            if (is_terminal(symbol)) {
                os << (char) symbol;
            } else {
                expand(symbol - RULE_OFFSET, os);
            }
        }
    }

  public:
    std::string reproduce() {
        std::ostringstream oss;

        if (rule_count() == 0) {
            return "";
        }
        expand(m_start_rule_id, oss);

        return oss.str();
    }
    /**
     * @brief Reproduces this grammar's source string
     *
     * @return std::string The source string
     */
    std::string reproduce_() {
        dependency_renumber();

        // This vector contains a mapping of a rule id (or rather, a nonterminal) to the string representation of the
        // rule, were it fully expanded
        std::vector<std::string> expansions;

        const auto n = rule_count();

        // After dependency_renumber is called, the rules occupy the ids 0 to n-1 (inclusive)
        for (size_t rule_id = 0; rule_id < n; rule_id++) {
            const auto symbols = m_rules[rule_id];

            // The string representation of the rule currently being processed
            std::stringstream ss;
            for (const auto symbol : symbols) {
                // If the scurrent symbol is a terminal, it can be written to this rule's string representation as is.
                // If it is a nonterminal, it can only be the nonterminal of a rule that has already been fully
                // expanded, since rules which depend on the least amount of other rules are always read first. The
                // string expansion thereof is written to the current rule's string representation
                if (is_terminal(symbol)) {
                    ss << (char) symbol;
                } else {
                    ss << expansions[symbol - RULE_OFFSET];
                }
            }

            // If we have arrived at the start rule there are no more rules to be read.
            // The string expansion of this rule is obviously the original text.
            if (rule_id == start_rule_id()) {
                return ss.str();
            }

            expansions.push_back(ss.str());
        }
        return "";
    }

    /**
     * @brief Returns the id of this grammar's start rule
     *
     * @return const size_t The id of the start rule
     */
    const inline size_t start_rule_id() const { return m_start_rule_id; }

    /**
     * @brief Set the start rule id
     *
     * @param i The id the start rule id should be set to
     */
    inline void set_start_rule_id(size_t i) { m_start_rule_id = i; }

    /**
     * @brief Calculates the size of the grammar
     *
     * The size is defined as the count of symbols in all right sides of this grammar's rules
     *
     * @return const size_t The size of the grammar
     */
    const size_t grammar_size() const {
        auto count = 0;
        for (size_t rule_id = 0; rule_id < m_rules.size(); rule_id++) {
            count += m_rules[rule_id].size();
        }
        return count;
    }

    /**
     * @brief Counts the rules in this grammar
     *
     * @return const size_t The rule count
     */
    const inline size_t rule_count() const { return m_rules.size(); }

    /**
     * @brief Checks whether this grammar contains the rule with the given id
     *
     * @param id The id to search for
     * @return true If the grammar contains the rule with the given id
     * @return false If the grammar does not contain the rule with the given id
     */
    const inline bool contains_rule(size_t id) const { return m_rules.size() < id && !m_rules[id].empty(); }

    /**
     * @brief Checks whether the grammar is empty
     *
     * @return true If the grammar contains no rules
     * @return false If the grammar contains rules
     */
    const inline bool empty() const { return rule_count() == 0; }

  private:
    const inline size_t source_length(size_t id, std::vector<size_t> lookup) const {
        std::vector<size_t> rule_lengths;
        constexpr auto      MAX = std::numeric_limits<size_t>().max();
        rule_lengths.resize(rule_count(), MAX);

        size_t count = 0;

        for (auto &symbol : m_rules[id]) {
            if (is_terminal(symbol)) {
                count++;
            } else if (lookup[symbol - RULE_OFFSET] < MAX) {
                count += lookup[symbol - RULE_OFFSET];
            } else {
                auto symb_length             = source_length();
                lookup[symbol - RULE_OFFSET] = symb_length;
                count += symb_length;
            }
        }
        return count;
    }

  public:
    const inline size_t source_length() const {
        std::vector<size_t> lookup;
        constexpr auto      MAX = std::numeric_limits<size_t>().max();
        lookup.resize(rule_count(), MAX);

        return source_length(m_start_rule_id, lookup);
    }

    /**
     * @brief Checks whether a symbol is a terminal.
     *
     * A symbol is considered a terminal if its value falls into the extended ASCII range, i.e. it is between 0 and 255.
     *
     * @see RULE_OFFSET
     *
     * @param symbol The symbol to check
     * @return true If the symbol is in the extended ASCII range
     * @return false If the symbol is not in the extended ASCII range
     */
    static const inline bool is_terminal(size_t symbol) { return symbol < RULE_OFFSET; }

    /**
     * @brief Checks whether a symbol is a non-terminal.
     *
     * A symbol is considered a non-terminal if its value is outside the extended ASCII range, i.e. it is greater or
     * equal to than 256.
     *
     * @see RULE_OFFSET
     *
     * @param symbol The symbol to check
     * @return true If the symbol outside the extended ASCII range
     * @return false If the symbol is in the extended ASCII range
     */
    static const inline bool is_non_terminal(size_t symbol) { return !is_terminal(symbol); }

    static inline std::vector<Rule> consume(Grammar &&gr) {
        std::vector<Rule> new_vec(0);
        std::swap(gr.m_rules, new_vec);
        return new_vec;
    }

    auto begin() const { return m_rules.cbegin(); }

    auto end() const { return m_rules.cend(); }

    const Rule &get_rule_const(size_t id) { return m_rules[id]; }
};

} // namespace gracli
