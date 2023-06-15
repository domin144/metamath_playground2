/*
 * Copyright 2017 Dominik Wójt
 *
 * This file is part of metamath_playground.
 
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
#include "metamath_database_read_write.h"

#include <fstream>

int main(const int argc, const char *const *const argv) try
{
    if (argc != 3)
        throw std::runtime_error(
                "usage: metamath_playgroud input.mm output.mm");

    using namespace metamath_playground;

    std::ifstream input_stream(argv[1]);
    std::ofstream output_stream(argv[2]);

    metamath_database database;
    read_database_from_file(database, input_stream);
    write_database_to_file(database, output_stream);

    return 0;
}
catch (const std::runtime_error &error)
{
    std::cerr << "std::runtime_error caught: " << error.what() << std::endl;
}
catch (...)
{
    std::cerr << "unknown exception caught" << std::endl;
}
