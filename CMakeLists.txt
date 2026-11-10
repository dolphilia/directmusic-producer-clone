cmake_minimum_required(VERSION 3.22)
project(ProducerAuthoring LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT "cxx_std_20" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
  message(FATAL_ERROR "The new product requires a compiler with C++20 support.")
endif()

# Add product module and application targets with their first implementations.
# The independent Windows recovery project lives in legacy/windows/.
message(STATUS "Repository layout configured; product binaries and JUCE integration are not implemented yet.")
