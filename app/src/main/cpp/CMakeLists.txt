cmake_minimum_required(VERSION 3.22.1)

project("tribex")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required libraries
find_library(log-lib log)
find_package(oboe REQUIRED CONFIG)

add_library(tribex_native SHARED
    native-lib.cpp
    AudioEngine.cpp
    Sequencer.cpp
    SampleVoice.cpp
    SamplePart.cpp
    SampleLoader.cpp
    WavetableSynth.cpp
    SynthPart.cpp
)

target_include_directories(tribex_native PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(tribex_native
    ${log-lib}
    oboe::oboe
)