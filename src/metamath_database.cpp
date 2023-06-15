/*
 * Copyright 2016 Dominik Wójt
 *
 * This file is part of metamath_playground.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "metamath_database.h"

namespace metamath_playground {
/*----------------------------------------------------------------------------*/
bool metamath_database::is_reserved(const std::string &label) const
{
    return allocated_labels.count(label) != 0;
}
/*----------------------------------------------------------------------------*/
symbol_index metamath_database::add_constant(
        const std::string &label)
{
    return add_symbol(label, symbol::type_t::constant);
}
/*----------------------------------------------------------------------------*/
symbol_index metamath_database::add_variable(
        const std::string &label)
{
    return add_symbol(label, symbol::type_t::variable);
}
/*----------------------------------------------------------------------------*/
symbol_index metamath_database::find_symbol(
        const std::string &label) const
{
    auto iterator = label_to_symbol.find(label);
    if (iterator != label_to_symbol.end())
        return iterator->second;
    else
        return symbol_index{symbol::type_t::constant, -1};
}
/*----------------------------------------------------------------------------*/
bool metamath_database::is_valid(const symbol_index index_in)
{
    return index_in.second != -1;
}
/*----------------------------------------------------------------------------*/
const std::string &metamath_database::get_symbol_label(
        const symbol_index index_in) const
{
    switch (index_in.first)
    {
    case symbol::type_t::constant:
        return constants[index_in.second].label;
    case symbol::type_t::variable:
        return variables[index_in.second].label;
    }
    throw std::runtime_error("invalid symbol type");
}
/*----------------------------------------------------------------------------*/
void metamath_database::remove_symbol(const symbol_index index_in)
{
    throw std::runtime_error("TODO");
}
/*----------------------------------------------------------------------------*/
metamath_database::symbol_iterator metamath_database::constants_begin() const
{
    return symbol_iterator(symbol_index(symbol::type_t::constant, 0));
}
/*----------------------------------------------------------------------------*/
metamath_database::symbol_iterator metamath_database::constants_end() const
{
    return
            symbol_iterator(
                symbol_index(symbol::type_t::constant, constants.size()));
}
/*----------------------------------------------------------------------------*/
metamath_database::symbol_iterator metamath_database::variables_begin() const
{
    return symbol_iterator(symbol_index(symbol::type_t::variable, 0));
}
/*----------------------------------------------------------------------------*/
metamath_database::symbol_iterator metamath_database::variables_end() const
{
    return
            symbol_iterator(
                symbol_index(symbol::type_t::variable, variables.size()));
}
/*----------------------------------------------------------------------------*/
assertion_index metamath_database::add_assertion(assertion &&assertion_in)
{
    std::vector<const std::string *> labels;
    labels.push_back(&assertion_in.label);
    for (auto &hypothesis : assertion_in.floating_hypotheses)
        labels.push_back(&hypothesis.label);
    for (auto &hypothesis : assertion_in.essential_hypotheses)
        labels.push_back(&hypothesis.label);
    for (auto &hypothesis : assertion_in.proof_0.floating_hypotheses)
        labels.push_back(&hypothesis.label);
    for (auto label : labels)
        reserve(*label);

    assertions.push_back(std::move(assertion_in));
    const assertion_index index0{static_cast<index>(assertions.size() - 1)};
    label_to_assertion[assertions.back().label] = index0;
    return index0;
}
/*----------------------------------------------------------------------------*/
assertion_index metamath_database::find_assertion(const std::string &label)
{
    auto iterator = label_to_assertion.find(label);
    if (iterator != label_to_assertion.end())
        return iterator->second;
    else
        return assertion_index{-1};
}
/*----------------------------------------------------------------------------*/
bool metamath_database::is_valid(const assertion_index index_in)
{
    return index_in.get_index() != -1;
}
/*----------------------------------------------------------------------------*/
const assertion &metamath_database::get_assertion(
        const assertion_index index_in) const
{
    return assertions[index_in.get_index()];
}
/*----------------------------------------------------------------------------*/
metamath_database::assertion_iterator metamath_database::assertions_begin(
        ) const
{
    return assertion_iterator(assertion_index(0));
}
/*----------------------------------------------------------------------------*/
metamath_database::assertion_iterator metamath_database::assertions_end(
        ) const
{
    return assertion_iterator(assertion_index(assertions.size()));
}
/*----------------------------------------------------------------------------*/
void metamath_database::remove_assertion(const assertion_index index_in)
{
    throw std::runtime_error("TODO");
}
/*----------------------------------------------------------------------------*/
void metamath_database::reserve(const std::string &label)
{
    if (allocated_labels.count(label) != 0)
        throw std::runtime_error("name conflict when adding a label");
    allocated_labels.insert(label);
}
/*----------------------------------------------------------------------------*/
void metamath_database::release(const std::string &label)
{
    auto iterator = allocated_labels.find(label);
    if (iterator == allocated_labels.end())
        throw std::runtime_error(
                "trying to release label which was not reserved");
    allocated_labels.erase(iterator);
}
/*----------------------------------------------------------------------------*/
symbol_index metamath_database::add_symbol(
        const std::string &label,
        symbol::type_t symbol_type)
{
    reserve(label);
    index index0;
    switch (symbol_type)
    {
    case symbol::type_t::constant:
        constants.push_back(symbol{label});
        index0 = static_cast<index>(constants.size() - 1);
        break;
    case symbol::type_t::variable:
        variables.push_back(symbol{label});
        index0 = static_cast<index>(variables.size() - 1);
        break;
    }
    const symbol_index symbol_index0{symbol_type, index0};
    label_to_symbol[label] = symbol_index0;
    return  symbol_index0;
}
/*----------------------------------------------------------------------------*/
unpacked_proof unpack_proof(const proof &proof_0)
{
    std::vector<adobe::forest<proof_step>> dangling_proofs;
    for (auto &step : proof_0.steps)
    {
        adobe::forest<proof_step> new_proof;
        new_proof.push_back(step);
        if (dangling_proofs.size() < step.assumptions_count)
            throw std::runtime_error("insufficient number of dangling proofs");
        /* leading at parent step */
        auto insert_iter = new_proof.begin();
        /* trailing at parent step */
        ++insert_iter;
        for (index i = 0; i < step.assumptions_count; ++i)
        {
            new_proof.splice(
                        insert_iter,
                        *(dangling_proofs.end() - step.assumptions_count + i));
        }
        dangling_proofs.resize(
                    dangling_proofs.size() - step.assumptions_count);
        dangling_proofs.push_back(new_proof);
    }
    if (dangling_proofs.size() != 1)
        std::runtime_error("invalid packed proof");

    unpacked_proof result;
    result.disjoint_variable_restrictions =
            proof_0.disjoint_variable_restrictions;
    result.floating_hypotheses = proof_0.floating_hypotheses;
    result.steps = std::move(dangling_proofs.back());
    return result;
}
/*----------------------------------------------------------------------------*/
proof unpack_proof(const unpacked_proof &proof_0)
{
    proof result;
    auto post_order_range =
            adobe::postorder_range(
                boost::make_iterator_range(
                    proof_0.steps.begin(),
                    proof_0.steps.end()));
    for(auto step : post_order_range)
    {
        result.steps.push_back(step);
    }
    result.disjoint_variable_restrictions =
            proof_0.disjoint_variable_restrictions;
    result.floating_hypotheses = proof_0.floating_hypotheses;
    return result;
}
/*----------------------------------------------------------------------------*/
} /* namespace metamath_playground */
