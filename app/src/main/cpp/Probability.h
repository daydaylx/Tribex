#ifndef TRIBEX_PROBABILITY_H
#define TRIBEX_PROBABILITY_H

#include <cstdint>

namespace Tribex {

/**
 * Deterministic Hash Function for Probability
 * 
 * This is called from audio thread - NO ALLOCATIONS!
 * 
 * Based on MurmurHash3 inspired mixing (simplified for performance)
 * 
 * Input: seed (pattern), stepIndex, loopIteration
 * Output: Deterministic value 0-255
 * 
 * Guarantee: Same inputs always produce same output
 * This ensures: Live playback == Export
 */

inline uint32_t hashProbability(uint32_t seed, uint32_t stepIndex, uint32_t loopIteration) {
    // Initial mix
    uint32_t h = seed ^ 0x9747b28;
    
    // Mix in stepIndex
    h = (h ^ stepIndex) * 0x9e3779ed;
    
    // Mix in loopIteration
    h = (h ^ loopIteration) * 0xbf58476d;
    
    // Final mixing (Avalanche)
    h = (h ^ (h >> 16)) * 0x85ebca6b;
    h = (h ^ (h >> 13)) * 0xc2b2ae35;
    h = h ^ (h >> 16);
    
    return h;
}

/**
 * Check if a step should trigger based on probability
 * 
 * @param probability 0-100 (0 = never, 100 = always)
 * @param seed Pattern seed
 * @param stepIndex Current step index
 * @param loopIteration Loop counter
 * @return true if step should trigger, false otherwise
 * 
 * CRITICAL: This must be deterministic!
 * Same inputs always produce same output.
 */
inline bool shouldTrigger(uint8_t probability, uint32_t seed, uint32_t stepIndex, uint32_t loopIteration) {
    // Edge cases
    if (probability >= 100) return true;
    if (probability <= 0) return false;
    
    // Get hash value (deterministic)
    uint32_t hash = hashProbability(seed, stepIndex, loopIteration);
    
    // Extract 8-bit value (0-255)
    uint8_t hashValue = static_cast<uint8_t>(hash & 0xFF);
    
    // Map probability 0-100 to 0-255 threshold
    // Example: 50% probability -> threshold = 127
    uint8_t threshold = static_cast<uint8_t>(probability * 2.55f);
    
    return hashValue < threshold;
}

} // namespace Tribex

#endif // TRIBEX_PROBABILITY_H