#///////////////////////////////////////////////////////////////////////////////
#//
#// Copyright 2020 Alan Chambers (unicycle.bloke@gmail.com)
#// 
#// Licensed under the Apache License, Version 2.0 (the "License");
#// you may not use this file except in compliance with the License.
#// You may obtain a copy of the License at
#//
#// http://www.apache.org/licenses/LICENSE-2.0
#//
#// Unless required by applicable law or agreed to in writing, software
#// distributed under the License is distributed on an "AS IS" BASIS,
#// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#// See the License for the specific language governing permissions and
#// limitations under the License.
#//
#///////////////////////////////////////////////////////////////////////////////
cmake_minimum_required(VERSION 3.10) 

set(BUDDY_APP "buddy")

project(${BUDDY_APP})

add_executable(${BUDDY_APP} test/test.cpp)

target_include_directories(${BUDDY_APP} 
PRIVATE 
    .
    include
    catch2  
)

if (UNIX)
    # Builds on UNIX-like systems: Linux, MSYS2, Windows Subsystem for Linux, ...
    # We assume GCC is used for the build
    target_compile_options(${BUDDY_APP} PUBLIC -g -std=c++17)
else()
    # Microsoft Visual Studio 2019 (2017 didn't work so well due to some of the C++17 features in the code).
    # Code be fixed with a bit off faff. Or just install VS2019. :)
    target_compile_options(${BUDDY_APP} PUBLIC /std:c++17 /MT)
endif()
