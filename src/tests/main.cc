/*
 * Copyright (C) 2015 Max Plutonium <plutonium.max@gmail.com>
 *
 * This file is part of the test suite of the ULTRA library.
 *
 * The ULTRA library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * The ULTRA library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the ULTRA library. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstdio>
#include <gmock/gmock.h>

int main(int argc, const char **argv)
{
    std::printf("ultra gtest\n");
    testing::InitGoogleMock(&argc, const_cast<char **>(argv));

    return RUN_ALL_TESTS();
}
