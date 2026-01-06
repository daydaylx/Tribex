#ifndef TRIBEX_PATTERNDATA_H
#define TRIBEX_PATTERNDATA_H

#include <cstdint>

namespace Tribex {

// Step Data Structure
// Represents a single step in a pattern for one part
struct StepData {
    uint8_t gate;          // 0 = OFF, 1 = ON
    uint8_t velocity;      // 2-bit: 0=Ghost, 1=Normal, 2=Accent, 3=Max
    int8_t microtiming;     // -50 to +50 ticks (sample-accurate offset)
    int8_t probability;    // 0-100% (0 = never, 100 = always)
    
    // M4+: Locks (sparse map) - reserved for future
    
    StepData()
        : gate(0)
        , velocity(1)  // Default: Normal
        , microtiming(0)
        , probability(100)  // Default: Always trigger
    {}
};

// Pattern Configuration
constexpr uint32_t MAX_STEPS = 64;        // Maximum steps per pattern (4 pages)
constexpr uint32_t STEPS_PER_PAGE = 16;    // Steps per page
constexpr uint32_t NUM_PARTS = 9;         // 8 Drum Parts + 1 Synth Part

// Pattern Structure
// Contains all steps for all parts
struct Pattern {
    uint32_t id;
    uint32_t lengthSteps;   // 16, 32, 48, or 64
    uint32_t patternSeed;   // Seed for deterministic probability
    StepData steps[NUM_PARTS][MAX_STEPS];  // 9 Parts x 64 Steps
    
    Pattern()
        : id(0)
        , lengthSteps(16)  // Default: 1 page
        , patternSeed(42)
    {
        // Initialize all steps to empty
        for (uint32_t part = 0; part < NUM_PARTS; part++) {
            for (uint32_t step = 0; step < MAX_STEPS; step++) {
                steps[part][step] = StepData();
            }
        }
    }
};

// Chain Entry
// One entry in a pattern chain
struct ChainEntry {
    uint32_t patternId;
    uint32_t repeatCount;  // How many times to repeat this pattern
    
    ChainEntry()
        : patternId(0)
        , repeatCount(1)
    {}
};

// Chain Configuration
constexpr uint32_t MAX_CHAIN_LENGTH = 16;

// Chain Structure
// Pattern playlist with repeat counts
struct Chain {
    ChainEntry entries[MAX_CHAIN_LENGTH];
    uint32_t length;
    
    Chain()
        : length(0)
    {
        for (uint32_t i = 0; i < MAX_CHAIN_LENGTH; i++) {
            entries[i] = ChainEntry();
        }
    }
};

// Velocity Constants
constexpr uint8_t VELOCITY_GHOST = 0;   // 0.3 (40)
constexpr uint8_t VELOCITY_NORMAL = 1;   // 0.6 (80)
constexpr uint8_t VELOCITY_ACCENT = 2;   // 0.9 (115)
constexpr uint8_t VELOCITY_MAX = 3;      // 1.0 (127)

// Velocity to float (0.0 - 1.0)
inline float velocityToFloat(uint8_t velocity) {
    switch (velocity) {
        case VELOCITY_GHOST: return 0.3f;
        case VELOCITY_NORMAL: return 0.6f;
        case VELOCITY_ACCENT: return 0.9f;
        case VELOCITY_MAX: return 1.0f;
        default: return 0.6f;
    }
}

// Microtiming constants
constexpr int8_t MICROTIMING_MIN = -50;  // -50 ticks (approx -13ms @ 120bpm)
constexpr int8_t MICROTIMING_MAX = 50;    // +50 ticks (approx +13ms @ 120bpm)

} // namespace Tribex

#endif // TRIBEX_PATTERNDATA_H