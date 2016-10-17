/* 
 * Copyright 2013 Dominik Wójt
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
#ifndef NAMED_H
#define NAMED_H

#include <string>

namespace metamath_playground {
/*----------------------------------------------------------------------------*/
class named
{
private:
    std::string name;

public:
    named(const std::string &name_in) :
        name(name_in)
    { }

    const std::string &get_name() const
    {
        return name;
    }

    void set_name(const std::string &name_in)
    {
        name = name_in;
    }
};
/*----------------------------------------------------------------------------*/
class less_by_name
{
public:
    bool operator()(const named &lhs, const named &rhs) const;
};
/*----------------------------------------------------------------------------*/
} /* namespace metamath_playground */

#endif // NAMED_H
