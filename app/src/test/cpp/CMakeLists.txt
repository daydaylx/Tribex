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

# Note: AudioEngine tests are deferred due to Oboe dependency
# We'll test the non-Oboe components separately
add_executable(tribex_samplepart_tests
    SamplePartTest.cpp
    ${TRIBEX_SRC_DIR}/SamplePart.cpp
    ${TRIBEX_SRC_DIR}/SampleVoice.cpp
)

target_include_directories(tribex_samplepart_tests PRIVATE
    ${TRIBEX_SRC_DIR}
)

# Lock-free ring buffer tests (retirement queue)
# TODO: Enable when Google Test is available in build environment
# add_executable(tribex_retirement_queue_tests
#     RetirementQueueTest.cpp
#     ${TRIBEX_SRC_DIR}/SamplePart.cpp
#     ${TRIBEX_SRC_DIR}/SampleVoice.cpp
# )
# 
# target_include_directories(tribex_retirement_queue_tests PRIVATE
#     ${TRIBEX_SRC_DIR}
# )

enable_testing()
add_test(NAME tribex_sequencer_tests COMMAND tribex_sequencer_tests)
add_test(NAME tribex_samplepart_tests COMMAND tribex_samplepart_tests)
# add_test(NAME tribex_retirement_queue_tests COMMAND tribex_retirement_queue_tests)
