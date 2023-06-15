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
#ifndef TYPED_INDICES_H
#define TYPED_INDICES_H

#include <cstdint>

namespace metamath_playground {

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

    typed_index &operator+=(index lhs)
    {
        index0 += lhs;
        return *this;
    }

    typed_index &operator-=(index lhs)
    {
        index0 -= lhs;
        return *this;
    }

    index &operator-=(typed_index lhs)
    {
        index0 -= lhs.index0;
        return *this;
    }

    bool operator==(const typed_index lhs) const
    {
        return index0 == lhs.index0;
    }

    bool operator!=(const typed_index lhs) const
    {
        return index0 != lhs.index0;
    }
};

} /* namespace metamath_playground */

#endif /* TYPED_INDICES_H */
