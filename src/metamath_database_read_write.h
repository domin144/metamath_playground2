/* 
 * Copyright 2013, 2016 Dominik Wójt
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
#ifndef METAMATH_DATABASE_READ_H
#define METAMATH_DATABASE_READ_H

#include "metamath_database.h"

#include <iostream>

namespace metamath_playground {

void read_database_from_file(
        Metamath_database &db,
        std::istream &input_stream);

void write_database_to_file(
        const Metamath_database &db,
        std::ostream &output_stream);

} /* namespace metamath_playground */

#endif // METAMATH_DATABASE_READ_H
