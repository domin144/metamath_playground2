/*
 * Copyright 2013 Dominik Wójt
 *
 * This file is part of metamath_playground.
 *
 * SPDX-License-Identifier: MIT OR Apache-2.0
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
