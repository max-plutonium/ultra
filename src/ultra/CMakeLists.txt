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
set(VERSION 0.0.0)
cmake_minimum_required(VERSION 3.0)

add_definitions("-DULTRA_VERSION=${VERSION}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} \
    -fvisibility=hidden -fvisibility-inlines-hidden")

set(PUBLIC_HEADERS
)

set(HEADERS
)

set(SOURCES
)

#add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})
#set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${VERSION}" SOVERSION "0")
#
#set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "-debug")
#
#install(TARGETS ${PROJECT_NAME} DESTINATION lib)
#install(FILES ${PUBLIC_HEADERS} DESTINATION ultra)
