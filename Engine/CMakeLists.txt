# MIT License

# Copyright (c) 2024 David Elder

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Includes
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# Definitions
add_definitions(
  -DNOMINMAX
)

# Source Files
set(SOURCES
	Engine.cpp
)

# Headers
set(HEADERS
	Engine.h
)

# Target
add_library(Engine STATIC
  ${SOURCES}
  ${HEADERS}
)

# Per-configuration definitions
target_compile_definitions(Engine PRIVATE
	$<$<CONFIG:Debug>:DEBUG=1>
	$<$<CONFIG:Debug>:RELEASE=0>
	$<$<CONFIG:Debug>:PROFILE=0>
	$<$<CONFIG:Release>:DEBUG=0>
	$<$<CONFIG:Release>:RELEASE=1>
	$<$<CONFIG:Release>:PROFILE=0>
	$<$<CONFIG:RelWithDebInfo>:PROFILE=1>
)