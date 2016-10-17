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

#include <vector>
#include <string>
#include <array>
#include <map>
#include <set>

namespace metamath_playground {

class symbol : public named
{
public:
    enum class type_t
    {
        constant,
        variable
    };

private:
    type_t type;

public:
    symbol(const type_t type_in, const std::string &name) :
        named(name),
        type(type_in)
    { }

    type_t get_type() const
    {
        return type;
    }

    void set_type(const type_t type_in)
    {
        type = type_in;
    }
};

class expression
{
private:
    std::vector<const Symbol *> symbols;

public:
    void push_back(const Symbol *symbol)
    {
        symbols.push_back(symbol);
    }

    const std::vector<const Symbol *> &get_symbols() const
    {
        return symbols;
    }
};

class disjoint_variable_restriction
{
private:
    std::array<const Symbol *, 2> variables;

public:
    disjoint_variable_restriction(
            const Symbol *variable0,
            const Symbol *variable1) :
        variables{{variable0, variable1}}
    { }

    std::array<const Symbol *, 2> get_variables() const
    {
        return variables;
    }
};

class floating_hypothesis : public named
{
private:
    const Symbol *type;
    const Symbol *variable;

public:
    floating_hypothesis(
            const std::string &hypothesis_label,
            const Symbol *type,
            const Symbol *variable_in) :
        named(hypothesis_label),
        type(type),
        variable(variable_in)
    { }

    const Symbol *get_type() const
    {
        return type;
    }

    const Symbol *get_variable() const
    {
        return variable;
    }
};

class essential_hypothesis : public named
{
private:
    expression expression0;

public:
    essential_hypothesis(
            const std::string &label,
            const expression &expression_in) :
        named(label),
        expression0(expression_in)
    { }

    const expression &get_expression() const
    {
        return expression0;
    }
};

struct proof_step
{
    enum type_t
    {
        floating_hypothesis,
        essential_hypothesis,
        assertion,
        recall,
        unknown
    };
    type_t type;
    int index;
    /* This is not necessary, but may speed up proof parsing and allow for
     * partial proof recovery, when number of assumption for some assertion
     * used in proof is changed. This value may be non-zero only for assertions.
     */
    int assumptions_count;
};

class proof
{
private:
    std::vector<disjoint_variable_restriction> disjoint_variable_restrictions;
    std::vector<floating_hypothesis> floating_hypotheses;
    std::vector<proof_step> steps;
};

class assertion : public named
{
public:
    enum class type_t
    {
        axiom,
        theorem
    };

private:
    type_t type;
    std::vector<disjoint_variable_restriction> disjoint_variable_restrictions;
    std::vector<floating_hypothesis> floating_hypotheses;
    std::vector<essential_hypothesis> essential_hypotheses;
    expression expression0;
    proof proof0;

public:
    assertion(
            const std::string &label,
            const type_t type_in,
            std::vector<disjoint_variable_restriction> &&restrictions,
            std::vector<floating_hypothesis> &&floating_hypotheses_in,
            std::vector<essential_hypothesis> &&essential_hypotheses_in,
            expression &&expression_in) :
        named(label),
        type(type_in),
        disjoint_variable_restrictions(restrictions),
        floating_hypotheses(floating_hypotheses_in),
        essential_hypotheses(essential_hypotheses_in),
        expression0(expression_in)
    { }

    const std::vector<disjoint_variable_restriction>
        &get_disjoint_variable_restrictions() const
    {
        return disjoint_variable_restrictions;
    }
    const std::vector<floating_hypothesis> &get_floating_hypotheses() const
    {
        return floating_hypotheses;
    }

    const std::vector<essential_hypothesis> &get_essential_hypotheses() const
    {
        return essential_hypotheses;
    }

    const expression &get_expression() const
    {
        return expression0;
    }
};

class Metamath_database
{
private:
    std::vector<Symbol> m_constants;
    std::vector<Symbol> m_variables;
    std::vector<assertion> m_assertions;

    std::map<std::string, const Symbol *> m_label_to_symbol;
    std::map<std::string, const assertion *> m_label_to_assertion;
    /* This is to verify if the metamath restriction of uniqueness of label and
     * math symbols is satisfied. */
    std::set<std::string> allocated_labels;

public:
    /* Returns nullptr on failure. */
    const Symbol *find_symbol_by_label(const std::string &label) const;
    /* Returns nullptr on failure. */
    const assertion *find_assertion_by_label(const std::string &label) const;
    int get_assertion_index(const assertion *assertion_in) const;
    void add_symbol(Symbol &&symbol);
    void add_assertion(assertion &&assertion);
    void reserve_label(const std::string &label);
};

} /* namespace metamath_playground */

#endif /* METAMATH_DATABASE_H */
