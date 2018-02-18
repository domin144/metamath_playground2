/*
 * Copyright 2016 Dominik Wójt
 *
 * This file is part of metamath_playground.
 *
 * metamath_playground is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * metamath_playground is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with metamath_playground.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef METAMATH_DATABASE_H
#define METAMATH_DATABASE_H

#include "named.h"
#include "typed_indices.h"

#include <adobe/forest.hpp>

#include <vector>
#include <string>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <iterator>

namespace metamath_playground {

struct symbol
{
    enum class type_t
    {
        constant,
        variable
    };

    std::string label;
};

using symbol_index = std::pair<symbol::type_t, index>;

using expression = std::vector<symbol_index>;

using disjoint_variable_restriction = std::array<symbol_index, 2>;

struct floating_hypothesis
{
    std::string label;
    symbol_index type;
    symbol_index variable;
};

struct essential_hypothesis
{
    std::string label;
    expression expression_0;
};

struct proof_step
{
    enum class type_t
    {
        floating_hypothesis,
        essential_hypothesis,
        assertion,
        recall,
        unknown
    };

    type_t type;
    /* index_0 has different meaning depending on type:
     * - floating_hypothesis: index in (assertion's floating hypotheses +
     *   proof's floating hypotheses),
     * - essential hypothesis: index in (assertion's essential hypotheses),
     * - assertion: index in database,
     * - recall: index in vector of steps */
    index index_0;
    /* This is not necessary, but may speed up proof parsing and allow for
     * partial proof recovery, when number of assumption for some assertion
     * used in proof is changed. This value may be non-zero only for
     * assertions. */
    index assumptions_count;
};

struct proof
{
    std::vector<disjoint_variable_restriction>
        disjoint_variable_restrictions;
    std::vector<floating_hypothesis> floating_hypotheses;
    std::vector<proof_step> steps;
};

struct assertion
{
    enum class type_t
    {
        axiom,
        theorem
    };

    std::string label;
    type_t type;
    std::vector<disjoint_variable_restriction>
        disjoint_variable_restrictions;
    std::vector<floating_hypothesis> floating_hypotheses;
    std::vector<essential_hypothesis> essential_hypotheses;
    expression expression_0;
    proof proof_0;
};

class metamath_database;

using assertion_index = typed_index<assertion, metamath_database>;

class metamath_database
{
public:
    class symbol_iterator
    {
    public:
        using difference_type = index;
        using value_type = symbol_index;
        using pointer = symbol_index *;
        using reference = symbol_index &;
        using iterator_category = std::forward_iterator_tag;

    private:
        symbol_index index_0;

    public:
        symbol_iterator() :
            index_0(symbol::type_t::constant, -1)
        { }

        symbol_iterator(const symbol_index index_in) :
            index_0(index_in)
        { }

        symbol_index operator*() const
        {
            return index_0;
        }

        symbol_iterator &operator++()
        {
            ++index_0.second;
            return *this;
        }

        bool operator!=(const symbol_iterator other) const
        {
            return this->index_0 != other.index_0;
        }

        bool operator==(const symbol_iterator other) const
        {
            return this->index_0 == other.index_0;
        }
    };

    class assertion_iterator
    {
    public:
        using difference_type = index;
        using value_type = assertion_index;
        using pointer = assertion_index *;
        using reference = assertion_index &;
        using iterator_category = std::forward_iterator_tag;

    private:
        assertion_index index_0;

    public:
        assertion_iterator() :
            index_0(-1)
        { }

        assertion_iterator(const assertion_index index_in) :
            index_0(index_in)
        { }

        assertion_index operator*() const
        {
            return index_0;
        }

        assertion_iterator &operator++()
        {
            index_0 += 1;
            return *this;
        }

        bool operator!=(const assertion_iterator other) const
        {
            return this->index_0 != other.index_0;
        }

        bool operator==(const assertion_iterator other) const
        {
            return this->index_0 == other.index_0;
        }
    };

private:
    /* members */
    std::vector<symbol> constants;
    std::vector<symbol> variables;
    std::vector<assertion> assertions;

    std::unordered_map<std::string, symbol_index> label_to_symbol;
    std::unordered_map<std::string, assertion_index> label_to_assertion;
    /* This is to verify if the metamath restriction of uniqueness of label and
     * math symbols is satisfied. */
    std::unordered_set<std::string> allocated_labels;

public:
    /* public methods */
    metamath_database() = default;

    bool is_reserved(const std::string &label) const;

    /* add/remove symbols */
    symbol_index add_constant(const std::string &label);
    symbol_index add_variable(const std::string &label);
    /* use is_valid to check if symbol was found */
    symbol_index find_symbol(const std::string &label) const;
    static bool is_valid(symbol_index index_in);
    const std::string &get_symbol_label(symbol_index index_in) const;
    /* warning: this is a complex operation: needs updating all expressions!
     * Also note, that any symbol indices and expressions kept outside database
     * may be invalidated. */
    void remove_symbol(symbol_index index_in);
    symbol_iterator constants_begin() const;
    symbol_iterator constants_end() const;
    symbol_iterator variables_begin() const;
    symbol_iterator variables_end() const;

    /* add/remove assertion */
    assertion_index add_assertion(assertion &&assertion_in);
    assertion_index find_assertion(const std::string &label);
    static bool is_valid(assertion_index index_in);
    const assertion &get_assertion(assertion_index index_in) const;
    assertion_iterator assertions_begin() const;
    assertion_iterator assertions_end() const;
    /* warning: this is a complex operation: needs updating all proofs!
     * Also note, that any assertion indices kept outside database may be
     * invalidated. */
    void remove_assertion(assertion_index index_in);

private:
    /* private methods */
    void reserve(const std::string &label);
    void release(const std::string &label);
    symbol_index add_symbol(
            const std::string &label,
            symbol::type_t symbol_type);
};

struct unpacked_proof
{
    std::vector<disjoint_variable_restriction>
        disjoint_variable_restrictions;
    std::vector<floating_hypothesis> floating_hypotheses;
    adobe::forest<proof_step> steps;
};

unpacked_proof unpack_proof(const proof &proof_0);
proof unpack_proof(const unpacked_proof &proof_0);

} /* namespace metamath_playground */

#endif /* METAMATH_DATABASE_H */
