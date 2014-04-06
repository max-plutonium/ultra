#
# Copyright (C) 2015 Max Plutonium <plutonium.max@gmail.com>
#
# This file is part of the test suite of the ULTRA library.
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
project(ultra-tests)
cmake_minimum_required(VERSION 3.0)

include_directories(SYSTEM ../googletest/googletest/include)
include_directories(SYSTEM ../googletest/googlemock/include)

set(SOURCES
    main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} gmock)

set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "-debug")

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
