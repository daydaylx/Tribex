package com.tribex.groovebox.engine

import java.nio.ByteBuffer
import java.nio.ByteOrder

/**
 * Sequencer Data Models - Mirror C++ Structs
 * 
 * These Kotlin data classes mirror the C++ structures in PatternData.h
 * Used for serialization/deserialization and UI state management
 */

/**
 * Step Data - Mirror of C++ StepData
 * 
 * Represents a single step in a pattern for one part
 */
data class StepData(
    val gate: UByte,          // 0 = OFF, 1 = ON
    val velocity: UByte,      // 2-bit: 0=Ghost, 1=Normal, 2=Accent, 3=Max
    val microtiming: Byte,    // -50 to +50 ticks (sample-accurate offset)
    val probability: UByte     // 0-100% (0 = never, 100 = always)
) {
    // M4+: Locks (sparse map) - reserved for future
    
    companion object {
        const val SIZE_BYTES = 4  // gate(1) + velocity(1) + microtiming(1) + probability(1)
        
        // Default empty step
        val EMPTY = StepData(
            gate = 0u,
            velocity = 1u,  // Normal
            microtiming = 0,
            probability = 100u  // Always trigger
        )
    }
}

/**
 * Pattern - Mirror of C++ Pattern
 * 
 * Contains all steps for all parts
 */
data class Pattern(
    val id: UInt,
    val lengthSteps: UInt,   // 16, 32, 48, or 64
    val patternSeed: UInt,     // Seed for deterministic probability
    val steps: Array<Array<StepData>>  // 9 Parts x up to 64 Steps
) {
    companion object {
        const val MAX_STEPS = 64u
        const val STEPS_PER_PAGE = 16u
        const val NUM_PARTS = 9u  // 8 Drum Parts + 1 Synth Part
        
        fun createEmpty(patternId: UInt = 0u): Pattern {
            val steps = Array(NUM_PARTS.toInt()) { Array(MAX_STEPS.toInt()) { StepData.EMPTY } }
            return Pattern(
                id = patternId,
                lengthSteps = 16u,  // Default: 1 page
                patternSeed = 42u,
                steps = steps
            )
        }
    }
}

/**
 * Chain Entry - Mirror of C++ ChainEntry
 * 
 * One entry in a pattern chain
 */
data class ChainEntry(
    val patternId: UInt,
    val repeatCount: UInt  // How many times to repeat this pattern
) {
    companion object {
        fun create(patternId: UInt = 0u): ChainEntry {
            return ChainEntry(patternId, 1u)
        }
    }
}

/**
 * Chain - Mirror of C++ Chain
 * 
 * Pattern playlist with repeat counts
 */
data class Chain(
    val entries: List<ChainEntry>
) {
    companion object {
        const val MAX_CHAIN_LENGTH = 16u
        
        fun createEmpty(): Chain {
            return Chain(emptyList())
        }
    }
}

/**
 * Velocity Constants - Mirror of C++ VELOCITY_*
 */
object Velocity {
    const val GHOST: UByte = 0u   // 0.3 (40)
    const val NORMAL: UByte = 1u   // 0.6 (80)
    const val ACCENT: UByte = 2u   // 0.9 (115)
    const val MAX: UByte = 3u      // 1.0 (127)
    
    /**
     * Convert 2-bit velocity to float (0.0 - 1.0)
     */
    fun toFloat(velocity: UByte): Float {
        return when (velocity) {
            GHOST -> 0.3f
            NORMAL -> 0.6f
            ACCENT -> 0.9f
            MAX -> 1.0f
            else -> 0.6f  // Default to Normal
        }
    }
    
    /**
     * Convert float velocity to 2-bit
     */
    fun fromFloat(value: Float): UByte {
        return when {
            value < 0.45f -> GHOST
            value < 0.75f -> NORMAL
            value < 0.95f -> ACCENT
            else -> MAX
        }
    }
}

/**
 * Microtiming Constants - Mirror of C++ MICROTIMING_*
 */
object Microtiming {
    const val MIN: Byte = -50  // -50 ticks (approx -13ms @ 120bpm)
    const val MAX: Byte = 50    // +50 ticks (approx +13ms @ 120bpm)
    
    /**
     * Clamp microtiming to valid range
     */
    fun clamp(value: Byte): Byte {
        return value.coerceIn(MIN, MAX)
    }
}

/**
 * Serialization Helpers
 * 
 * Convert between C++ byte arrays and Kotlin data classes
 */
object PatternSerializer {
    /**
     * Serialize StepData to byte array (C++ compatible)
     */
    fun stepToBytes(step: StepData): ByteArray {
        return byteArrayOf(
            step.gate.toByte(),
            step.velocity.toByte(),
            step.microtiming,
            step.probability.toByte()
        )
    }
    
    /**
     * Deserialize StepData from byte array
     */
    fun stepFromBytes(bytes: ByteArray): StepData {
        require(bytes.size == StepData.SIZE_BYTES) {
            "StepData must be ${StepData.SIZE_BYTES} bytes, got ${bytes.size}"
        }
        return StepData(
            gate = bytes[0].toUByte(),
            velocity = bytes[1].toUByte(),
            microtiming = bytes[2],
            probability = bytes[3].toUByte()
        )
    }
}