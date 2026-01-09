cmake_minimum_required(VERSION 3.22.1)

project(tribex_native_tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TRIBEX_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/../../main/cpp)

add_executable(tribex_sequencer_tests
    SequencerTest.cpp
    ${TRIBEX_SRC_DIR}/Sequencer.cpp
)

target_include_directories(tribex_sequencer_tests PRIVATE
    ${TRIBEX_SRC_DIR}
)

enable_testing()
add_test(NAME tribex_sequencer_tests COMMAND tribex_sequencer_tests)
