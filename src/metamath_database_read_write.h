/*
 * Copyright 2013, 2016 Dominik Wójt
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
#ifndef METAMATH_DATABASE_READ_H
#define METAMATH_DATABASE_READ_H

#include "metamath_database.h"

#include <iostream>

namespace metamath_playground {

void read_database_from_file(
        metamath_database &db,
        std::istream &input_stream);

void write_database_to_file(
        const metamath_database &db,
        std::ostream &output_stream);

} /* namespace metamath_playground */

#endif /* METAMATH_DATABASE_READ_H */
