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
#include "tokenizer.h"
#include <stdexcept>

namespace metamath_playground {
/*----------------------------------------------------------------------------*/
tokenizer::tokenizer(std::istream &input_stream_in) :
    input_stream(input_stream_in)
{
    input_stream >> next_token;
}
/*----------------------------------------------------------------------------*/
std::string tokenizer::get_token()
{
    if (next_token.empty())
    {
        throw std::runtime_error("requested a token from past the end of the"
            "stream");
    }
    std::string result(next_token);
    extract_next_token();
    return result;
}
/*----------------------------------------------------------------------------*/
const std::string &tokenizer::peek()
{
    return next_token;
}
/*----------------------------------------------------------------------------*/
void tokenizer::extract_next_token()
{
    input_stream >> next_token;
    if (!input_stream)
        next_token.clear();
}
/*----------------------------------------------------------------------------*/
} /* namespace metamath_playground */
