/* 
 * Copyright 2013 Dominik Wójt
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
