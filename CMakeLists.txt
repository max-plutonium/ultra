#
# Copyright (C) 2015 Max Plutonium <plutonium.max@gmail.com>
#
# This file is part of the ULTRA library.
#
# The ULTRA library is free software: you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# The ULTRA library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with the ULTRA library. If not, see
# <http://www.gnu.org/licenses/>.
#
project(ultra)
cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
add_definitions("-DULTRA_EXPORT=__attribute__((visibility (\"default\")))")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -fstack-check")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

include_directories(src/executors/include)

add_subdirectory(src/googletest/googlemock)

add_subdirectory(src/ultra)

add_subdirectory(src/tests)
