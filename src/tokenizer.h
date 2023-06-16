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
#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <iostream>

namespace metamath_playground {

class tokenizer
{
private:
    std::istream &input_stream;
    std::string next_token;

public:
    tokenizer(std::istream &input_stream);
    std::string get_token();
    const std::string &peek();

private:
    void extract_next_token();
};

} /* namespace metamath_playground */

#endif // TOKENIZER_H
