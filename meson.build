# Copyright 2023 Dominik Wójt
#
# This file is part of metamath_playground.
#
# SPDX-License-Identifier: MIT OR Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

project(
  'metamath_playgroud',
  'cpp',
  default_options: ['cpp_std=c++20'],
  license: 'Apache-2.0',
  license_files: 'LICENSE'
)

cmake = import('cmake')

adobe_source_libraries_options = cmake.subproject_options()
adobe_source_libraries_options.add_cmake_defines({'USE_SYSTEM_BOOST': true})
adobe_source_libraries_options.set_install(false)
adobe_source_libraries_subproject = cmake.subproject(
  'adobe_source_libraries',
  options: adobe_source_libraries_options)
adobe_source_libraries_dependency = \
  adobe_source_libraries_subproject.dependency('asl')

subdir('src')
