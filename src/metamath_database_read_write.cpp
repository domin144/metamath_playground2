/* 
 * Copyright 2013, 2016 Dominik Wójt
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

#include "metamath_database_read_write.h"
#include "tokenizer.h"

#include <adobe/forest.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <stdexcept>
#include <utility>
#include <map>
#include <set>
#include <tuple>
#include <algorithm>
#include <numeric>

namespace metamath_playground {
/*----------------------------------------------------------------------------*/
namespace {
/*----------------------------------------------------------------------------*/
struct frame_entry
{
    enum class type_t
    {
        disjoint_variable_restriction,
        essential_hypothesis,
        floating_hypothesis
    };

    type_t type;
    index index_0; /* index in its category */
};
/*----------------------------------------------------------------------------*/
using frame = std::vector<frame_entry>;
/*----------------------------------------------------------------------------*/
/* Only ordinary (not extended) frames are kept in this registry. */
struct legacy_frame_registry
{
    /* Indices in this context refer to assertion's internal arrays. */
    std::vector<frame> frames;
};
/*----------------------------------------------------------------------------*/
class scope
{
private:
    std::map<std::string, int> label_to_essential_hypothesis_index;
    std::vector<floating_hypothesis> floating_hypotheses;
    std::vector<essential_hypothesis> essential_hypotheses;
    std::vector<disjoint_variable_restriction> disjoint_variable_restrictions;
    std::vector<frame_entry> spurious_frame;

public:
    scope() = default;
    scope(scope &) = default;

    void add_floating_hypothesis(floating_hypothesis &&hypothesis);
    /* Returns -1 on failure. */
    index find_essential_hypothesis_index_by_label(
            const std::string &label) const;
    void add_essential_hypothesis(essential_hypothesis &&hypothesis);
    void add_disjoint_variable_restriction(
            disjoint_variable_restriction &&restriction);

    const std::vector<essential_hypothesis> &get_essential_hypotheses() const
    {
        return essential_hypotheses;
    }

    const std::vector<floating_hypothesis> &get_floating_hypotheses() const
    {
        return floating_hypotheses;
    }

    const std::vector<disjoint_variable_restriction>
                &get_disjoint_variable_restrictions() const
    {
        return disjoint_variable_restrictions;
    }

    const frame &get_spurious_frame() const
    {
        return spurious_frame;
    }
};
/*----------------------------------------------------------------------------*/
void scope::add_floating_hypothesis(floating_hypothesis &&hypothesis)
{
    floating_hypotheses.push_back(std::move(hypothesis));
    spurious_frame.push_back(
                frame_entry{
                    frame_entry::type_t::floating_hypothesis,
                    static_cast<int>(floating_hypotheses.size() - 1)});
}
/*----------------------------------------------------------------------------*/
index scope::find_essential_hypothesis_index_by_label(
        const std::string &label) const
{
    for (index i = 0; i < get_essential_hypotheses().size(); ++i)
        if (get_essential_hypotheses()[i].label == label)
            return i;
    return -1;
}
/*----------------------------------------------------------------------------*/
void scope::add_essential_hypothesis(essential_hypothesis &&hypothesis)
{
    /* TODO: Put global check for name clashes at level above. */
    if(find_essential_hypothesis_index_by_label(hypothesis.label) != -1)
        throw std::runtime_error("statement name clash");

    essential_hypotheses.push_back(std::move(hypothesis));

    auto &added_hypothesis = essential_hypotheses.back();
    label_to_essential_hypothesis_index[added_hypothesis.label] =
            static_cast<int>(essential_hypotheses.size() - 1);

    spurious_frame.push_back(
                frame_entry{
                    frame_entry::type_t::essential_hypothesis,
                    static_cast<int>(essential_hypotheses.size() - 1)});
}
/*----------------------------------------------------------------------------*/
void scope::add_disjoint_variable_restriction(
        disjoint_variable_restriction &&restriction)
{
    disjoint_variable_restrictions.push_back(std::move(restriction));
    spurious_frame.push_back(
                frame_entry{
                    frame_entry::type_t::disjoint_variable_restriction,
                    static_cast<int>(
                        disjoint_variable_restrictions.size() - 1)});
}
/*----------------------------------------------------------------------------*/
void read_comment(tokenizer &tokenizer0);
/*----------------------------------------------------------------------------*/
expression read_expression(
        metamath_database &database,
        tokenizer &input_tokenizer,
        const std::string &terminating_token = "$.")
{
    expression result;
    while (input_tokenizer.peek() != terminating_token)
    {
        if (input_tokenizer.peek() == "$(")
            read_comment(input_tokenizer);
        auto symbol = database.find_symbol(input_tokenizer.get_token());
        if (!database.is_valid(symbol))
            throw std::runtime_error("symbol not found");
        result.push_back(symbol);
    }
    return result;
}
/*----------------------------------------------------------------------------*/
void read_statement(
        metamath_database &database,
        legacy_frame_registry &registry,
        scope &current_scope,
        tokenizer &input_tokenizer);
/*----------------------------------------------------------------------------*/
void read_scope(
        metamath_database &database,
        legacy_frame_registry &registry,
        scope &parent_scope,
        tokenizer &input_tokenizer)
{
    if (input_tokenizer.get_token() != "${")
        throw std::runtime_error("scope does not start with \"${\"");

    scope current_scope(parent_scope);
    while (input_tokenizer.peek() != "$}")
        read_statement(database, registry, current_scope, input_tokenizer);
    input_tokenizer.get_token(); /* consume "$}" */
}
/*----------------------------------------------------------------------------*/
void read_variables(metamath_database &database, tokenizer &input_tokenizer)
{
    if (input_tokenizer.get_token() != "$v")
        throw std::runtime_error("variables do not start with \"$v\"");

    while (input_tokenizer.peek() != "$.")
    {
        if (input_tokenizer.peek() == "$(")
            read_comment(input_tokenizer);
        const auto name = input_tokenizer.get_token();
        database.add_variable(name);
    }
    input_tokenizer.get_token(); /* consume "$." */
}
/*----------------------------------------------------------------------------*/
void read_constants(metamath_database &database, tokenizer &input_tokenizer)
{
    if (input_tokenizer.get_token() != "$c")
        throw std::runtime_error("constants do not start with \"$c\"");

    while (input_tokenizer.peek() != "$.")
    {
        if (input_tokenizer.peek() == "$(")
            read_comment(input_tokenizer);
        const auto name = input_tokenizer.get_token();
        database.add_constant(name);
    }
    input_tokenizer.get_token(); /* consume "$." */
}
/*----------------------------------------------------------------------------*/
void read_floating_hypothesis(
        metamath_database &database,
        scope &current_scope,
        tokenizer &input_tokenizer,
        const std::string &label)
{
    if (input_tokenizer.get_token() != "$f")
        throw std::runtime_error("variable assumption does not start with "
            "\"$f\"");

    const auto expression = read_expression(database, input_tokenizer);
    if (
            expression.size() != 2
            || expression[0].first != symbol::type_t::constant
            || expression[1].first != symbol::type_t::variable)
        throw std::runtime_error("invalid floating hypothesis");

    floating_hypothesis hypothesis{label, expression[0], expression[1]};
    current_scope.add_floating_hypothesis(std::move(hypothesis));

    input_tokenizer.get_token(); /* consume "$." */
}
/*----------------------------------------------------------------------------*/
void read_essential_hypothesis(
        metamath_database &database,
        scope &current_scope,
        tokenizer &input_tokenizer,
        const std::string &label)
{
    if (input_tokenizer.get_token() != "$e")
        throw std::runtime_error("assumption does not start with \"$e\"");

    auto expression0 = read_expression(database, input_tokenizer);
    essential_hypothesis hypothesis{label, expression0};
    current_scope.add_essential_hypothesis(std::move(hypothesis));

    input_tokenizer.get_token(); /* consume "$." */
}
/*----------------------------------------------------------------------------*/
void collect_variables(
        const expression &expression0,
        std::set<symbol_index> &symbols_found,
        std::vector<symbol_index> &result)
{
    /* Algorithm is designed to preserve the order of symbols in expression. */

    for(auto symbol : expression0)
    {
        if(
                symbol.first == symbol::type_t::variable
                && symbols_found.count(symbol) == 0)
        {
            symbols_found.insert(symbol);
            result.push_back(symbol);
        }
    }
}
/*----------------------------------------------------------------------------*/
/* The order of variables in frame is the order of first occurance in hypotheses
 * and expression. */
std::vector<symbol_index> collect_variables(
        const std::vector<essential_hypothesis> &hypotheses,
        const expression &expression0)
{
    std::set<symbol_index> symbols_found;
    std::vector<symbol_index> result;

    for(auto &hypothesis : hypotheses)
        collect_variables(hypothesis.expression_0, symbols_found, result);
    collect_variables(expression0, symbols_found, result);

    return result;
}
/*----------------------------------------------------------------------------*/
std::vector<disjoint_variable_restriction> filter_restrictions(
        const std::vector<disjoint_variable_restriction> &input_restrictions,
        const std::vector<symbol_index> variables)
{
    std::vector<disjoint_variable_restriction> result;
    for (auto restriction : input_restrictions)
    {
        auto v0_iterator =
                std::find(variables.begin(), variables.end(), restriction[0]);
        auto v1_iterator =
                std::find(variables.begin(), variables.end(), restriction[1]);
        if (v0_iterator != variables.end() && v1_iterator != variables.end())
            result.push_back(restriction);
    }
    return result;
}
/*----------------------------------------------------------------------------*/
std::tuple<std::vector<floating_hypothesis>, frame>
fill_legacy_frame_and_floating_hypotheses(
        const frame &spurious_frame,
        const std::vector<floating_hypothesis> &input_hypotheses,
        const std::vector<symbol_index> &variables)
{
    std::vector<floating_hypothesis> floating_hypotheses;
    frame legacy_frame;
    index essential_hypothesis_index = 0;
    index floating_hypothesis_index = 0;
    for(auto entry : spurious_frame)
    {
        switch (entry.type)
        {
        case frame_entry::type_t::disjoint_variable_restriction:
            break;
        case frame_entry::type_t::essential_hypothesis:
            legacy_frame.push_back(
                        frame_entry{entry.type, essential_hypothesis_index});
            ++essential_hypothesis_index;
            break;
        case frame_entry::type_t::floating_hypothesis: {
            auto &hypothesis = input_hypotheses[entry.index_0];
            auto iterator =
                    std::find(
                        variables.begin(),
                        variables.end(),
                        hypothesis.variable);
            if (iterator == variables.end())
                break;
            floating_hypotheses.push_back(hypothesis);
            legacy_frame.push_back(
                        frame_entry{entry.type, floating_hypothesis_index});
            ++floating_hypothesis_index;
            break; }
        }
    }
    return std::make_tuple(
                std::move(floating_hypotheses),
                std::move(legacy_frame));
}
/*----------------------------------------------------------------------------*/
class compressed_proof_code_extractor
{
private:
    tokenizer &tokenizer0;
    std::string buffer;

public:
    compressed_proof_code_extractor(tokenizer &tokenizer_in) :
        tokenizer0(tokenizer_in)
    { }
    int extract_number()
    {
        int n = 0;
        while (true)
        {
            char z = get_character();
            if ('A' <= z && z <= 'T')
            {
                n = n * 20 + z - 'A' + 1;
                return n;
            }
            else if ('U' <= z && z <= 'Y')
            {
                n = n * 5 + z - 'U' + 1;
            }
            else if (z == 'Z')
            {
                throw std::runtime_error(
                            "error: Z found in compressed proof when number "
                            "incomplete");
            }
            else
            {
                throw std::runtime_error(
                            "error: invalid character found in compressed "
                            "proof");
            }
        }
    }
    bool extract_reference_flag()
    {
        if (peek_character() == 'Z')
        {
            get_character();
            return true;
        }
        else
        {
            return false;
        }
    }
    bool is_end_of_proof()
    {
        return peek_character() == 0;
    }

private:
    void fill_buffer()
    {
        if (tokenizer0.peek() != "$.")
            buffer += tokenizer0.get_token();
    }
    char peek_character()
    {
        if (buffer.empty())
            fill_buffer();
        if (buffer.empty())
            return 0; /* end of compressed sequence */
        else
            return buffer.front();
    }
    char get_character()
    {
        char result = peek_character();
        if (result == 0)
            throw std::runtime_error(
                    "read got past end of compressed sequence");
        buffer.erase(buffer.begin());
        return result;
    }
};
/*----------------------------------------------------------------------------*/
std::vector<disjoint_variable_restriction> extract_non_mandatory_restrictions(
        const std::vector<disjoint_variable_restriction>
            &available_restrictions,
        const std::vector<floating_hypothesis> &mandatory_floating_hypotheses,
        const std::vector<floating_hypothesis> &non_mandatory_hypotheses)
{
    std::vector<disjoint_variable_restriction> non_mandatory_restrictions;
    for (auto &restriction : available_restrictions)
    {
        const auto is_variable_in_hypotheses =
                [] (
                const std::vector<floating_hypothesis> &hypotheses,
                const symbol_index symbol) -> bool
                {
                    return
                            std::find_if(
                                hypotheses.begin(),
                                hypotheses.end(),
                                [&symbol](const floating_hypothesis &hypothesis)
                                {
                                    return hypothesis.variable == symbol;
                                })
                            != hypotheses.end();
                };

        const bool non_mandatory_0 =
                is_variable_in_hypotheses(
                    non_mandatory_hypotheses,
                    restriction[0]);
        const bool non_mandatory_1 =
                is_variable_in_hypotheses(
                    non_mandatory_hypotheses,
                    restriction[1]);
        const bool mandatory_0 =
                is_variable_in_hypotheses(
                    mandatory_floating_hypotheses,
                    restriction[0]);
        const bool mandatory_1 =
                is_variable_in_hypotheses(
                    mandatory_floating_hypotheses,
                    restriction[1]);
        if (
                (non_mandatory_0 && non_mandatory_1)
                || (non_mandatory_0 && mandatory_1)
                || (mandatory_0 && non_mandatory_1))
        {
            non_mandatory_restrictions.push_back(restriction);
        }
    }
    return non_mandatory_restrictions;
}

/*----------------------------------------------------------------------------*/
proof read_compressed_proof(
        metamath_database &database,
        scope &current_scope,
        tokenizer &input_tokenizer,
        legacy_frame_registry &frame_registry,
        const std::vector<floating_hypothesis> &mandatory_floating_hypotheses)
{
    input_tokenizer.get_token(); // read "("

    std::vector<proof_step> steps;
    std::vector<floating_hypothesis> non_mandatory_hypotheses;

    const auto &essential_hypotheses = current_scope.get_essential_hypotheses();
    const auto &other_floating_hypotheses =
            current_scope.get_floating_hypotheses();
    const auto &current_legacy_frame = frame_registry.frames.back();

    const index mandatory_hypotheses_count =
            essential_hypotheses.size()
            + mandatory_floating_hypotheses.size();
    if (mandatory_hypotheses_count != current_legacy_frame.size())
        throw std::runtime_error(
                "collected mandatory hypotheses count does not match the size "
                "of the frame");

    std::vector<proof_step> referred_statements;
    /* read referred statements */
    while (input_tokenizer.peek() != ")")
    {
        while (input_tokenizer.peek() == "$(")
            read_comment(input_tokenizer);

        const auto name = input_tokenizer.get_token();

        if (name == "?")
        {
            referred_statements.push_back(
                        proof_step{proof_step::type_t::unknown, 0, 0});
            continue;
        }

        auto assertion_index = database.find_assertion(name);
        if (database.is_valid(assertion_index))
        {
            /* push with marking consumed count */
            const frame &assertions_frame =
                    frame_registry.frames[assertion_index.get_index()];
            const index consumed_count =
                    static_cast<index>(assertions_frame.size());
            referred_statements.push_back(
                        proof_step{
                            proof_step::type_t::assertion,
                            assertion_index.get_index(),
                            consumed_count});
            continue;
        }

        auto new_non_mandatory_iterator =
                std::find_if(
                    other_floating_hypotheses.begin(),
                    other_floating_hypotheses.end(),
                    [&name](const floating_hypothesis &hypothesis) -> bool
                    {
                        return hypothesis.label == name;
                    });
        if (new_non_mandatory_iterator != other_floating_hypotheses.end())
        {
            const index non_mandatory_index =
                    non_mandatory_hypotheses.size()
                    + mandatory_floating_hypotheses.size();
            non_mandatory_hypotheses.push_back(*new_non_mandatory_iterator);
            referred_statements.push_back(
                        proof_step{
                            proof_step::type_t::floating_hypothesis,
                            non_mandatory_index,
                            0});
            continue;
        }

        throw std::runtime_error("not recognized proof step");
    }
    const index referred_statements_count = referred_statements.size();

    input_tokenizer.get_token(); /* read ")" */

    compressed_proof_code_extractor extractor(input_tokenizer);
    std::vector<index> referred_expressions;
    while (!extractor.is_end_of_proof())
    {
        index number = extractor.extract_number() - 1;
        if (number < mandatory_hypotheses_count)
        {
            proof_step::type_t step_type;
            switch (current_legacy_frame[number].type)
            {
            case frame_entry::type_t::essential_hypothesis:
                step_type = proof_step::type_t::essential_hypothesis;
                break;
            case frame_entry::type_t::floating_hypothesis:
                step_type = proof_step::type_t::floating_hypothesis;
                break;
            case frame_entry::type_t::disjoint_variable_restriction:
                throw std::runtime_error("unexpected frame entry type");
            }

            steps.push_back(
                        proof_step{
                            step_type,
                            current_legacy_frame[number].index_0,
                            0});
        }
        else if (
                 (number -= mandatory_hypotheses_count)
                 < referred_statements_count)
        {
            steps.push_back(referred_statements[number]);
        }
        else if (
                 (number -= referred_statements_count)
                 < referred_expressions.size())
        {
            steps.push_back(
                        proof_step{
                            proof_step::type_t::recall,
                            referred_expressions[number],
                            0});
        }
        else
        {
            throw std::runtime_error(
                        "invalid number read in compressed proof");
        }
        if (extractor.extract_reference_flag())
        {
            referred_expressions.push_back(steps.size() - 1);
        }
    }

    std::vector<disjoint_variable_restriction> non_mandatory_restrictions =
            extract_non_mandatory_restrictions(
                current_scope.get_disjoint_variable_restrictions(),
                mandatory_floating_hypotheses,
                non_mandatory_hypotheses);

    return proof{
                std::move(non_mandatory_restrictions),
                std::move(non_mandatory_hypotheses),
                std::move(steps)};
}
//------------------------------------------------------------------------------
proof read_uncompressed_proof(
        metamath_database &database,
        scope &current_scope,
        tokenizer &input_tokenizer,
        legacy_frame_registry &frame_registry,
        const std::vector<floating_hypothesis> &mandatory_floating_hypotheses)
{
    std::vector<proof_step> steps;
    std::vector<floating_hypothesis> non_mandatory_hypotheses;

    const auto &essential_hypotheses = current_scope.get_essential_hypotheses();
    const auto &other_floating_hypotheses =
            current_scope.get_floating_hypotheses();

    while (input_tokenizer.peek() != "$.")
    {
        while (input_tokenizer.peek() == "$(")
            read_comment(input_tokenizer);

        auto name = input_tokenizer.get_token();
        if (name == "?")
        {
            steps.push_back(proof_step{proof_step::type_t::unknown, 0, 0});
            continue;
        }

        auto assertion_index = database.find_assertion(name);
        if (database.is_valid(assertion_index))
        {
            /* push with marking consumed count */
            const frame &assertions_frame =
                    frame_registry.frames[assertion_index.get_index()];
            const index consumed_count =
                    static_cast<index>(assertions_frame.size());
            steps.push_back(
                        proof_step{
                            proof_step::type_t::assertion,
                            assertion_index.get_index(),
                            consumed_count});
            continue;
        }
        auto essential_iterator =
                std::find_if(
                    essential_hypotheses.begin(),
                    essential_hypotheses.end(),
                    [&name](const essential_hypothesis &hypothesis) -> bool
                    {
                        return hypothesis.label == name;
                    });
        if (essential_iterator != essential_hypotheses.end())
        {
            /* just push with index */
            steps.push_back(
                        proof_step{
                            proof_step::type_t::essential_hypothesis,
                            essential_iterator - essential_hypotheses.begin(),
                            0});
            continue;
        }

        auto mandatory_iterator =
                std::find_if(
                    mandatory_floating_hypotheses.begin(),
                    mandatory_floating_hypotheses.end(),
                    [&name](const floating_hypothesis &hypothesis) -> bool
                    {
                        return hypothesis.label == name;
                    });
        if (mandatory_iterator != mandatory_floating_hypotheses.end())
        {
            /* just push with index */
            const index mandatory_index =
                    mandatory_iterator - mandatory_floating_hypotheses.begin();
            steps.push_back(
                        proof_step{
                            proof_step::type_t::floating_hypothesis,
                            mandatory_index,
                            0});
            continue;
        }

        auto non_mandatory_iterator =
                std::find_if(
                    non_mandatory_hypotheses.begin(),
                    non_mandatory_hypotheses.end(),
                    [&name](const floating_hypothesis &hypothesis) -> bool
                    {
                        return hypothesis.label == name;
                    });
        if (non_mandatory_iterator != non_mandatory_hypotheses.end())
        {
            /* add to proof and push with
             * index = (index in proof) + (mandatory floating hypotheses count)
             */
            const index non_mandatory_index =
                    (non_mandatory_iterator - non_mandatory_hypotheses.begin())
                    + static_cast<index>(mandatory_floating_hypotheses.size());
            steps.push_back(
                        proof_step{
                            proof_step::type_t::floating_hypothesis,
                            non_mandatory_index,
                            0});
            continue;
        }

        auto new_non_mandatory_iterator =
                std::find_if(
                    other_floating_hypotheses.begin(),
                    other_floating_hypotheses.end(),
                    [&name](const floating_hypothesis &hypothesis) -> bool
                    {
                        return hypothesis.label == name;
                    });
        if (new_non_mandatory_iterator != other_floating_hypotheses.end())
        {
            /* add to proof and push with
             * index =
             *      (non mandatory hypotheses count)
             *      + (mandatory floating hypotheses count)
             */
            const index non_mandatory_index =
                    non_mandatory_hypotheses.size()
                    + mandatory_floating_hypotheses.size();
            non_mandatory_hypotheses.push_back(*new_non_mandatory_iterator);
            steps.push_back(
                        proof_step{
                            proof_step::type_t::floating_hypothesis,
                            non_mandatory_index,
                            0});
            continue;
        }

        throw std::runtime_error("not recognized proof step");
    }

    std::vector<disjoint_variable_restriction> non_mandatory_restrictions =
            extract_non_mandatory_restrictions(
                current_scope.get_disjoint_variable_restrictions(),
                mandatory_floating_hypotheses,
                non_mandatory_hypotheses);

    return proof{
                std::move(non_mandatory_restrictions),
                std::move(non_mandatory_hypotheses),
                std::move(steps)};
}
/*----------------------------------------------------------------------------*/
std::string find_free_name(
        const std::string &base_name,
        const metamath_database &database,
        const std::set<std::string> &other_names)
{
    std::string result = base_name;
    index i = 0;
    while (
           database.is_reserved(result)
           || other_names.find(result) != other_names.end())
    {
        result = base_name + '_' + std::to_string(i);
        ++i;
    }
    return result;
}
/*----------------------------------------------------------------------------*/
std::string fix_assertion_label(
        const std::string &assertion_label,
        const metamath_database &database)
{
    std::string result = assertion_label;
    std::replace(result.begin(), result.end(), '.', '_');
    result = find_free_name(result, database, std::set<std::string>());
    return result;
}
/*----------------------------------------------------------------------------*/
std::string fix_hypothesis_label(
        const std::string &assertion_label,
        const std::string &hypothesis_label,
        const metamath_database &database,
        const std::set<std::string> &other_names)
{
    const index assertion_label_size =
            assertion_label.size();
    const index hypothesis_label_size =
            hypothesis_label.size();
    const bool correct_prefix =
            hypothesis_label_size >= assertion_label_size + 2
            && std::equal(
                assertion_label.begin(),
                assertion_label.end(),
                hypothesis_label.begin())
            && hypothesis_label[assertion_label_size] == '.';

    std::string result;

    if (correct_prefix)
    {
        result = hypothesis_label;
    }
    else
    {
        result = assertion_label + "." + hypothesis_label;
    }
    std::replace(
                result.begin() + assertion_label_size + 1,
                result.end(),
                '.',
                '_');
    return find_free_name(result, database, other_names);
}
/*----------------------------------------------------------------------------*/
void fix_labels_for_assertion(
        std::string &assertion_label,
        std::vector<floating_hypothesis> &floating_hypotheses,
        std::vector<essential_hypothesis> &essential_hypotheses,
        std::vector<floating_hypothesis> &non_mandatory_floating_hypotheses,
        const metamath_database &database)
{
    std::set<std::string> other_names;
    assertion_label =
            fix_assertion_label(assertion_label, database);
    other_names.insert(assertion_label);
    for (auto &hypothesis : floating_hypotheses)
    {
        hypothesis.label =
                fix_hypothesis_label(
                    assertion_label,
                    hypothesis.label,
                    database,
                    other_names);
        other_names.insert(hypothesis.label);
    }
    for (auto &hypothesis : essential_hypotheses)
    {
        hypothesis.label =
                fix_hypothesis_label(
                    assertion_label,
                    hypothesis.label,
                    database,
                    other_names);
        other_names.insert(hypothesis.label);
    }
    for (auto &hypothesis : non_mandatory_floating_hypotheses)
    {
        hypothesis.label =
                fix_hypothesis_label(
                    assertion_label,
                    hypothesis.label,
                    database,
                    other_names);
        other_names.insert(hypothesis.label);
    }
}
/*----------------------------------------------------------------------------*/
template <typename T>
void reorder_vector(std::vector<T> &vector, const std::vector<index> &map)
{
    auto old_vector = vector;
    for (index i = 0; i < vector.size(); ++i)
        vector[map[i]] = old_vector[i];
}
/*----------------------------------------------------------------------------*/
void reorder_proof(
        proof &proof_0,
        const legacy_frame_registry &registry)
{
    /* maps from old index to new index */
    std::vector<index> map(proof_0.steps.size());
    std::iota(map.begin(), map.end(), 0);

    std::vector<index> stack;
    std::vector<index> dependency_counts(proof_0.steps.size());

    for (index i = 0; i < proof_0.steps.size(); ++i)
    {
        auto &step = proof_0.steps[i];
        switch (step.type)
        {
        case proof_step::type_t::floating_hypothesis:
            /* no dependencies - nothing to do. */
            dependency_counts[i] = 0;
            stack.push_back(i);
            break;
        case proof_step::type_t::essential_hypothesis:
            /* Needed floating hypotheses are also hypotheses of assertion.
             * Reordering is done when processing assertion. */
            dependency_counts[i] = 0;
            stack.push_back(i);
            break;
        case proof_step::type_t::assertion: {
            /* input order:
             *    registry.frames[i->index] - each entry matches entry on the
             *    stack
             * output order:
             *    mandatory floating hypotheses
             *    essential hypotheses
             */
            auto &legacy_frame = registry.frames[step.index_0];
            index floating_offset = 0;
            index essential_offset = 0;
            index legacy_offset = 0;
            for (index j = 0; j < legacy_frame.size(); ++j)
            {
                auto l = *(stack.end() - legacy_frame.size() + j);
                if (
                        legacy_frame[j].type
                        == frame_entry::type_t::floating_hypothesis)
                {
                    essential_offset += dependency_counts[l] + 1;
                }
            }
            for (index j = 0; j < legacy_frame.size(); ++j)
            {
                auto l = *(stack.end() - legacy_frame.size() + j);
                auto l_branch_begin = l - dependency_counts[l];
                auto l_branch_end = l + 1;
                switch (legacy_frame[j].type)
                {
                case frame_entry::type_t::floating_hypothesis:
                    for (index k = l_branch_begin; k < l_branch_end; ++k)
                        map[k] += floating_offset - legacy_offset;
                    floating_offset += dependency_counts[l] + 1;
                    break;

                case frame_entry::type_t::essential_hypothesis:
                    for (index k = l_branch_begin; k < l_branch_end; ++k)
                        map[k] += essential_offset - legacy_offset;
                    essential_offset += dependency_counts[l] + 1;
                    break;

                case frame_entry::type_t::disjoint_variable_restriction:
                    throw
                            std::runtime_error(
                                "unexpected disjoint variable restriction "
                                "in legacy frame");
                }
                legacy_offset += dependency_counts[l] + 1;
            }
            dependency_counts[i] = legacy_offset;
            stack.resize(stack.size() - legacy_frame.size());
            stack.push_back(i);
            break; }
        case proof_step::type_t::recall:
            /* no dependencies - nothing to do. */
            dependency_counts[i] = 0;
            stack.push_back(i);
            break;
        case proof_step::type_t::unknown:
            /* no dependencies - nothing to do. */
            dependency_counts[i] = 0;
            stack.push_back(i);
            break;
        }
    }

    for (index i = 0; i < proof_0.steps.size(); ++i)
    {
        auto &step = proof_0.steps[i];
        if (step.type == proof_step::type_t::recall)
            step.index_0 = map[step.index_0];
    }
    reorder_vector(proof_0.steps, map);
    reorder_vector(dependency_counts, map);

    /* TODO: Now "recall" steps can refer to future steps. Need to reorder them.
     */
    throw std::runtime_error("TODO!");
    std::iota(map.begin(), map.end(), 0);
//    index push_offset = 0;
    for (index i = 0; i < proof_0.steps.size(); ++i)
    {
        auto &step = proof_0.steps[i];
        if (step.type == proof_step::type_t::recall)
        {
            if (map[step.index_0] > i)
            {
                const index branch_begin =
                        step.index_0 - dependency_counts[step.index_0];
                const index branch_end = step.index_0 + 1;
                for (index j = branch_begin; j < branch_end; ++j)
                {
                    map[j] += -branch_begin + i;
                }
            }
        }
    }

}
/*----------------------------------------------------------------------------*/
void read_assertion(
        metamath_database &database,
        scope &current_scope,
        legacy_frame_registry &registry,
        tokenizer &input_tokenizer,
        const std::string &label)
{
    assertion::type_t type;
    if (input_tokenizer.peek() == "$a")
        type = assertion::type_t::axiom;
    else if (input_tokenizer.peek() == "$p")
        type = assertion::type_t::theorem;
    else
        throw std::runtime_error(
                "assertion does not start with \"$a\" or \"$p\" ");
    input_tokenizer.get_token(); /* consume "$a" or "$p" */

    const std::string expression_terminator =
            type == assertion::type_t::axiom
            ? "$."
            : "$=";
    expression expression0 =
            read_expression(database, input_tokenizer, expression_terminator);
    auto essential_hypotheses = current_scope.get_essential_hypotheses();
    auto variables = collect_variables(essential_hypotheses, expression0);
    auto disjoint_variable_restrictions =
            filter_restrictions(
                current_scope.get_disjoint_variable_restrictions(),
                variables);

    std::vector<floating_hypothesis> floating_hypotheses;
    frame legacy_frame;
    std::tie(floating_hypotheses, legacy_frame) =
            fill_legacy_frame_and_floating_hypotheses(
                current_scope.get_spurious_frame(),
                current_scope.get_floating_hypotheses(),
                variables);

    registry.frames.push_back(legacy_frame);

    switch (type)
    {
    case assertion::type_t::axiom: {

        /* fix labels */
        std::string new_label = label;
        std::vector<floating_hypothesis> dummy;
        fix_labels_for_assertion(
                    new_label,
                    floating_hypotheses,
                    essential_hypotheses,
                    dummy,
                    database);

        assertion new_assertion{
                    label,
                    type,
                    std::move(disjoint_variable_restrictions),
                    std::move(floating_hypotheses),
                    std::move(essential_hypotheses),
                    std::move(expression0),
                    proof()};
        database.add_assertion(std::move(new_assertion));
        break; }
    case assertion::type_t::theorem: {
        input_tokenizer.get_token(); /* consume "$=" */

        proof new_proof;
        if(input_tokenizer.peek() == "(")
        {
            new_proof =
                    read_compressed_proof(
                        database,
                        current_scope,
                        input_tokenizer,
                        registry,
                        floating_hypotheses);
        }
        else
        {
            new_proof =
                    read_uncompressed_proof(
                        database,
                        current_scope,
                        input_tokenizer,
                        registry,
                        floating_hypotheses);
        }

        reorder_proof(new_proof, registry);

        /* fix labels */
        std::string new_label = label;
        fix_labels_for_assertion(
                    new_label,
                    floating_hypotheses,
                    essential_hypotheses,
                    new_proof.floating_hypotheses,
                    database);

        assertion new_assertion{
                    new_label,
                    type,
                    std::move(disjoint_variable_restrictions),
                    std::move(floating_hypotheses),
                    std::move(essential_hypotheses),
                    std::move(expression0),
                    new_proof};
        database.add_assertion(std::move(new_assertion));
        break; }
    }

    input_tokenizer.get_token(); /* consume "$." */
}
/*----------------------------------------------------------------------------*/
void read_disjoint_variable_restriction(
        metamath_database &database,
        scope &current_scope,
        tokenizer &input_tokenizer)
{
    if (input_tokenizer.get_token() != "$d")
        throw std::runtime_error(
                "disjoint variable restriction does not start with \"$d\"");

    const symbol_index index_0 =
            database.find_symbol(input_tokenizer.get_token());
    const symbol_index index_1 =
            database.find_symbol(input_tokenizer.get_token());
    if (!database.is_valid(index_0) || !database.is_valid(index_1))
        throw std::runtime_error(
                "invalid symbol in disjoint variable restriction");
    disjoint_variable_restriction restriction{{index_0, index_1}};
    current_scope.add_disjoint_variable_restriction(std::move(restriction));

    auto token = input_tokenizer.get_token(); /* consume "$." */
    if (token != "$.")
        throw std::runtime_error("invalid disjoint variable restriction");
}
/*----------------------------------------------------------------------------*/
void read_comment(tokenizer &input_tokenizer)
{
    if (input_tokenizer.get_token() != "$(")
        throw std::runtime_error("comment does not start with \"$(\"");

    while (input_tokenizer.peek() != "$)")
        input_tokenizer.get_token();
    /* consume "$)" */
    input_tokenizer.get_token();
}
/*----------------------------------------------------------------------------*/
void read_statement(
        metamath_database &database,
        legacy_frame_registry &registry,
        scope &current_scope,
        tokenizer &input_tokenizer)
{
    std::string label;
    if (input_tokenizer.peek().at(0) != '$')
        label = input_tokenizer.get_token();

    if (input_tokenizer.peek() == "$a" || input_tokenizer.peek() == "$p")
    {
        read_assertion(
                    database,
                    current_scope,
                    registry,
                    input_tokenizer,
                    label);
    }
    else if (input_tokenizer.peek() == "$v")
    {
        read_variables(database, input_tokenizer);
    }
    else if (input_tokenizer.peek() == "${")
    {
        if (!label.empty())
            throw std::runtime_error("Scope with label found.");
        read_scope(database, registry, current_scope, input_tokenizer);
    }
    else if (input_tokenizer.peek() == "$c")
    {
        read_constants(database, input_tokenizer);
    }
    else if (input_tokenizer.peek() == "$f")
    {
        read_floating_hypothesis(
                    database,
                    current_scope,
                    input_tokenizer,
                    label);
    }
    else if (input_tokenizer.peek() == "$e")
    {
        read_essential_hypothesis(
                    database,
                    current_scope,
                    input_tokenizer,
                    label);
    }
    else if (input_tokenizer.peek() == "$d")
    {
        read_disjoint_variable_restriction(
                    database, current_scope, input_tokenizer);
    }
    else if (input_tokenizer.peek() == "$(")
    {
        read_comment(input_tokenizer);
    }
    else
    {
        throw std::runtime_error("expected label or dollar statment start");
    }
}
/*----------------------------------------------------------------------------*/
void read_database_from_file(
        metamath_database &database,
        tokenizer &input_tokenizer)
{
    scope top_scope;
    legacy_frame_registry registry;
    while(!input_tokenizer.peek().empty())
    {
        read_statement(database, registry, top_scope, input_tokenizer);
    }
}
/*----------------------------------------------------------------------------*/
void write_expression_to_file(
        const metamath_database &database,
        const expression &expression_0,
        std::ostream &output_stream)
{
    for(auto symbol : expression_0)
        output_stream << database.get_symbol_label(symbol) << ' ';
}
/*----------------------------------------------------------------------------*/
void write_symbols_to_file(
        const metamath_database &database,
        std::ostream &output_stream)
{
    const auto constants_range =
            boost::make_iterator_range(
                database.constants_begin(),
                database.constants_end());
    if (constants_range.begin() != constants_range.end())
    {
        output_stream << "$c ";
        for(auto symbol_index : constants_range)
        {
            output_stream << database.get_symbol_label(symbol_index) << ' ';
        }
        output_stream << "$.\n";
    }

    const auto variables_range =
            boost::make_iterator_range(
                database.variables_begin(),
                database.variables_end());
    if (!variables_range.empty())
    {
        output_stream << "$v ";
        for(auto symbol_index : variables_range)
        {
            output_stream << database.get_symbol_label(symbol_index) << ' ';
        }
        output_stream << "$.\n";
    }
}
/*----------------------------------------------------------------------------*/
void write_floating_hypothesis(
        const metamath_database &database,
        const floating_hypothesis &hypothesis,
        std::ostream &output_stream)
{
    output_stream
            << "    " << hypothesis.label << " $f "
            << database.get_symbol_label(hypothesis.type) << ' '
            << database.get_symbol_label(hypothesis.variable) << ' '
            << "$.\n";
}
/*----------------------------------------------------------------------------*/
void write_essential_hypothesis(
        const metamath_database &database,
        const essential_hypothesis &hypothesis,
        std::ostream &output_stream)
{
    output_stream << "    " << hypothesis.label << " $e ";
    write_expression_to_file(database, hypothesis.expression_0, output_stream);
    output_stream << "$.\n";
}
/*----------------------------------------------------------------------------*/
void write_disjoint_variable_restriction(
        const metamath_database &database,
        const disjoint_variable_restriction &restriction,
        std::ostream &output_stream)
{
    output_stream
            << "    $d "
            << database.get_symbol_label(restriction[0]) << ' '
            << database.get_symbol_label(restriction[1]) << ' '
            << "$.\n";
}
/*----------------------------------------------------------------------------*/
std::string encode_compressed_number(index number)
{
    std::string result;
    number--;
    if (number < 0)
        throw std::runtime_error("n < 1");

    result.push_back('A' + number % 20);
    number /= 20;
    while (number > 0)
    {
        number--;
        result.push_back('U' + number % 5);
        number /= 5;
    }
    std::reverse(result.begin(), result.end());
    return result;
}
/*----------------------------------------------------------------------------*/
void write_assertion(
        const metamath_database &database,
        const assertion &assertion_0,
        std::ostream &output_stream)
{
    output_stream << "${\n";

    for (const auto &hypothesis : assertion_0.floating_hypotheses)
        write_floating_hypothesis(database, hypothesis, output_stream);

    for (const auto &hypothesis : assertion_0.essential_hypotheses)
        write_essential_hypothesis(database, hypothesis, output_stream);

    for (const auto &restriction : assertion_0.disjoint_variable_restrictions)
        write_disjoint_variable_restriction(
                    database,
                    restriction,
                    output_stream);

    if (assertion_0.type == assertion::type_t::theorem)
    {
        const proof &proof_0 = assertion_0.proof_0;
        for (const auto &hypothesis : proof_0.floating_hypotheses)
            write_floating_hypothesis(database, hypothesis, output_stream);

        for (const auto &restriction : proof_0.disjoint_variable_restrictions)
            write_disjoint_variable_restriction(
                        database,
                        restriction,
                        output_stream);
    }

    output_stream << "    " << assertion_0.label;
    if (assertion_0.type == assertion::type_t::axiom)
        output_stream << " $a ";
    else
        output_stream << " $p ";

    write_expression_to_file(database, assertion_0.expression_0, output_stream);

    if (assertion_0.type == assertion::type_t::theorem)
    {
        /* Saving only in compressed form is supported. */
        const proof &proof_0 = assertion_0.proof_0;

        std::vector<assertion_index> referred_assertions;
        for (const auto step : proof_0.steps)
        {
            if (step.type == proof_step::type_t::assertion)
            {
                assertion_index current_assertion_index(step.index_0);
                if (
                        std::find(
                            referred_assertions.begin(),
                            referred_assertions.end(),
                            current_assertion_index)
                        == referred_assertions.end())
                {
                    referred_assertions.push_back(current_assertion_index);
                }
            }
        }

        output_stream << "\n    $= ( ";
        for (const auto &hypothesis : proof_0.floating_hypotheses)
            output_stream << hypothesis.label << ' ';
        for (const auto &assertion_index : referred_assertions)
            output_stream
                    << database.get_assertion(assertion_index).label
                    << ' ';
        output_stream << ") ";

        for (const auto step : proof_0.steps)
        {
            switch (step.type)
            {
            case proof_step::type_t::floating_hypothesis: {
                index index_0 = step.index_0;
                if (step.index_0 >= assertion_0.floating_hypotheses.size())
                    index_0 +=
                            assertion_0.floating_hypotheses.size()
                            + assertion_0.essential_hypotheses.size();
                output_stream << encode_compressed_number(index_0 + 1);
                break; }
            case proof_step::type_t::essential_hypothesis:
                output_stream
                        << encode_compressed_number(
                               step.index_0
                               + assertion_0.floating_hypotheses.size()
                               + 1);
                break;
            case proof_step::type_t::assertion: {
                assertion_index current_assertion_index(
                            step.index_0);
                auto index_iterator =
                        std::find(
                            referred_assertions.begin(),
                            referred_assertions.end(),
                            current_assertion_index);
                if (index_iterator == referred_assertions.end())
                    throw std::runtime_error(
                            "assertion index not found on list of referred "
                            "assertions");
                output_stream
                        << encode_compressed_number(
                               (index_iterator - referred_assertions.begin())
                               + assertion_0.floating_hypotheses.size()
                               + assertion_0.essential_hypotheses.size()
                               + proof_0.floating_hypotheses.size()
                               + 1);
                break; }
            case proof_step::type_t::recall:
                output_stream
                        << encode_compressed_number(
                               step.index_0
                               + assertion_0.floating_hypotheses.size()
                               + assertion_0.essential_hypotheses.size()
                               + proof_0.floating_hypotheses.size()
                               + referred_assertions.size()
                               + 1);
                break;
            case proof_step::type_t::unknown:
                output_stream << '?';
                break;
            }
        }
    }

    output_stream << " $.\n";
    output_stream << "$}\n";
}
/*----------------------------------------------------------------------------*/
} /* anonymous namespace */
/*----------------------------------------------------------------------------*/
void read_database_from_file(
        metamath_database &database,
        std::istream &input_stream)
{
    tokenizer input_tokenizer(input_stream);
    read_database_from_file(database, input_tokenizer);
}
/*----------------------------------------------------------------------------*/
void write_database_to_file(
        const metamath_database &database,
        std::ostream &output_stream)
{
    write_symbols_to_file(database, output_stream);

    const auto assertions_range =
            boost::make_iterator_range(
                database.assertions_begin(),
                database.assertions_end());
    for (const auto assertion_index : assertions_range)
    {
        const auto &assertion_0 = database.get_assertion(assertion_index);
        write_assertion(database, assertion_0, output_stream);
    }
}
/*----------------------------------------------------------------------------*/
} /* namespace metamath_playground */
