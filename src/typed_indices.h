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

#ifndef TYPED_INDICES_H
#define TYPED_INDICES_H

#include <cstdint>

using index = std::intptr_t;

/* This class template allows for encoding indexed type and type of container in
 * which the indexed objects are contained into the index type. It is designed
 * to prevent mixing indices of different type and, more importantly, express
 * intended use for each index. */

template<typename Indexed, typename Container>
class typed_index
{
public:
    using indexed = Indexed;
    using container = Container;

private:
    index index0 = 0;

public:
    typed_index() = default;

    explicit typed_index(index index_in) :
        index0(index_in)
    { }

    index get_index() const
    {
        return index0;
    }

    typed_index &operator+(index lhs)
    {
        index0 + lhs;
        return *this;
    }

    typed_index &operator-(index lhs)
    {
        index0 - lhs;
        return *this;
    }

    index &operator-(typed_index lhs)
    {
        index0 - lhs.index0;
        return *this;
    }
};

#endif /* TYPED_INDICES_H */
