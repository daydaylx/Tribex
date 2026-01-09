package com.tribex.groovebox.ui.screen

import com.tribex.groovebox.engine.Velocity

/**
 * Pattern Display State
 * 
 * UI state for PATTERN screen - represents one page (16 steps)
 */
data class PatternState(
    val currentStep: UInt = 0u,          // Currently playing step (running light)
    val bpm: Float = 120f,                // BPM value
    val isPlaying: Boolean = false,         // Audio engine playing state
    val currentPatternId: UInt = 0u,       // Active pattern ID
    val patternLengthSteps: UInt = 16u,     // Pattern length (16/32/48/64)
    val currentPage: UInt = 0u,             // Current page (0-3 for 16/32/48/64)
    val patternSeed: UInt = 0u,             // Pattern seed for deterministic probability
    val steps: List<List<StepDisplayState>>  // 9 Parts x 16 Steps (visible page)
) {
    companion object {
        fun createEmpty(): PatternState {
            val steps = List(9) { part ->
                List(16) { stepIndex ->
                    StepDisplayState(
                        stepIndex = stepIndex.toUInt(),
                        partIndex = part.toUInt(),
                        gate = false,
                        velocity = Velocity.NORMAL,
                        probability = 100u,
                        microtiming = 0
                    )
                }
            }
            return PatternState(steps = steps)
        }
    }
}

/**
 * Step Display State
 * 
 * Represents a single step in the UI grid
 */
data class StepDisplayState(
    val stepIndex: UInt,       // Step index (0-15 for current page)
    val partIndex: UInt,       // Part index (0-8)
    val gate: Boolean,          // Gate state (ON/OFF)
    val velocity: UByte,       // Velocity (2-bit)
    val probability: UByte,     // Probability (0-100%)
    val microtiming: Byte = 0   // Microtiming (-50 to +50 ticks)
)

/**
 * Part Info
 * 
 * Information about a part for display
 */
data class PartInfo(
    val index: UInt,           // Part index (0-8)
    val name: String,           // Part name (e.g., "BD", "SD", "SYNTH")
    val isMuted: Boolean = false,
    val isSolo: Boolean = false
)

/**
 * All Parts Info
 * 
 * 9 Parts: 8 Drum Parts + 1 Synth Part
 */
val PARTS = listOf(
    PartInfo(0u, "BD"),
    PartInfo(1u, "SD"),
    PartInfo(2u, "CH"),
    PartInfo(3u, "OH"),
    PartInfo(4u, "CP"),
    PartInfo(5u, "LT"),
    PartInfo(6u, "MT"),
    PartInfo(7u, "HT"),
    PartInfo(8u, "SYNTH")
)
