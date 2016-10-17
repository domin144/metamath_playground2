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

#include "metamath_database.h"

namespace metamath_playground {
/*----------------------------------------------------------------------------*/
const Symbol *Metamath_database::find_symbol_by_label(
        const std::string &label) const
{
    auto i = m_label_to_symbol.find(label);
    if(i != m_label_to_symbol.end())
    {
        return i->second;
    }
    else
    {
        return nullptr;
    }
}
/*----------------------------------------------------------------------------*/
const assertion *Metamath_database::find_assertion_by_label(
        const std::string &label) const
{
    auto i = m_label_to_assertion.find(label);
    if(i != m_label_to_assertion.end())
    {
        return i->second;
    }
    else
    {
        return nullptr;
    }
}
/*----------------------------------------------------------------------------*/
int Metamath_database::get_assertion_index(const assertion *assetion_in) const
{
    return static_cast<int>(assetion_in - m_assertions.data());
}
/*----------------------------------------------------------------------------*/
void Metamath_database::add_symbol(Symbol &&symbol)
{
    reserve_label(symbol.get_name());

    switch (symbol.get_type())
    {
    case Symbol::Type::constant:
        m_constants.push_back(std::move(symbol));
        m_label_to_symbol[m_constants.back().get_name()] = &m_constants.back();
        break;
    case Symbol::Type::variable:
        m_variables.push_back(std::move(symbol));
        m_label_to_symbol[m_variables.back().get_name()] = &m_variables.back();
        break;
    }
}
/*----------------------------------------------------------------------------*/
void Metamath_database::add_assertion(assertion &&assertion_in)
{
    reserve_label(assertion_in.get_name());

    m_assertions.push_back(std::move(assertion_in));
    m_label_to_assertion[m_assertions.back().get_name()] = &m_assertions.back();
}
/*----------------------------------------------------------------------------*/
void Metamath_database::reserve_label(const std::string &label)
{
    if (allocated_labels.count(label) != 0)
        throw std::runtime_error("name conflict when adding a symbol");
    allocated_labels.insert(label);
}
/*----------------------------------------------------------------------------*/
} /* namespace metamath_playground */
