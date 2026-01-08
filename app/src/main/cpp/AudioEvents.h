#ifndef TRIBEX_AUDIOEVENTS_H
#define TRIBEX_AUDIOEVENTS_H

#include <cstdint>

/**
 * Audio Event Types
 * 
 * Events are sent from Control Thread (UI) to Audio Thread
 * All events are processed in render() callback
 * 
 * M1 Scope: Master controls only
 * M2+: Sequencer, Voice, FX events
 * M4: Sample Engine events
 */
enum class EventType : uint8_t {
    SET_MASTER_GAIN = 0,     // value = gain (0.0 to 2.0)
    SET_MASTER_PAN = 1,      // value = pan (-1.0 to 1.0)
    SET_TEST_TONE_FREQ = 2,   // value = frequency (Hz)
    
    // M2 Event - Sequencer
    SET_BPM = 3,            // value = bpm (20.0 to 300.0)
    
    // M4 Events - Sample Engine
    TRIGGER_VOICE = 4,      // value = velocity, reserved = partIndex
    SET_VOICE_PITCH = 5,     // value = pitch (semitones -24 to +24), reserved = partIndex
    SET_VOICE_PAN = 6,      // value = pan (-1.0 to 1.0), reserved = partIndex
    SET_VOICE_LEVEL = 7,     // value = level (0.0 to 1.0), reserved = partIndex
    SET_VOICE_DECAY = 8,     // value = decay (ms), reserved = partIndex
    SET_VOICE_FILTER = 9,     // value = filter (0.0=LP, 1.0=HP), reserved = partIndex
    LOAD_SAMPLE = 10,         // value = sampleId, reserved = partIndex
    
    // M5 Events - Synth Part (Part 8 only)
    SET_SYNTH_WAVETABLE = 11,  // value = type (0-5), reserved = partIndex
    SET_SYNTH_CUTOFF = 12,      // value = cutoff (0-1), reserved = partIndex
    SET_SYNTH_RESONANCE = 13,   // value = resonance (0-1), reserved = partIndex
    SET_SYNTH_ATTACK = 14,      // value = attack (ms), reserved = partIndex
    SET_SYNTH_DECAY = 15,       // value = decay (ms), reserved = partIndex
    SET_SYNTH_SUSTAIN = 16,     // value = sustain (0-1), reserved = partIndex
    SET_SYNTH_RELEASE = 17,     // value = release (ms), reserved = partIndex
    
    // Reserved for future milestones
    RESERVED_18 = 18,
    RESERVED_19 = 19
};

/**
 * Audio Event Structure
 * 
 * Fixed size for lock-free queue efficiency
 * POD type (Plain Old Data) - no virtual functions, no complex types
 */
struct AudioEvent {
    EventType type;
    float value;  // Gain, Pan, Frequency, BPM, velocity, or parameter value
    uint32_t reserved;  // Used for partIndex, stepIndex, etc. depending on type
    
    /**
     * Default constructor
     */
    AudioEvent() : type(EventType::SET_MASTER_GAIN), value(0.0f), reserved(0) {}
    
    /**
     * Parameterized constructor (value only)
     */
    AudioEvent(EventType t, float v) : type(t), value(v), reserved(0) {}
    
    /**
     * Parameterized constructor (value + reserved)
     */
    AudioEvent(EventType t, float v, uint32_t r) : type(t), value(v), reserved(r) {}
    
    /**
     * Check if event is valid (M1+M2+M4+M5 events)
     */
    bool isValid() const {
        return type >= EventType::SET_MASTER_GAIN && 
               type <= EventType::SET_SYNTH_RELEASE;
    }
};

// M2+: More complex event types can be defined here
// e.g., struct VoiceEvent { EventType type; uint8_t voice; float value; };

#endif // TRIBEX_AUDIOEVENTS_H