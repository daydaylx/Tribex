package com.tribex.groovebox.ui

import com.tribex.groovebox.ui.screen.PatternState
import com.tribex.groovebox.ui.screen.StepDisplayState
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class PatternStateTest {

    @Test
    fun createEmptyPatternHasCorrectStructure() {
        val emptyPattern = PatternState.createEmpty()
        
        // Basic structure tests
        assertEquals(9, emptyPattern.steps.size)
        emptyPattern.steps.forEach { partSteps ->
            assertEquals(16, partSteps.size)
        }
    }

    @Test
    fun basicPatternStateCreation() {
        // Simple test that PatternState can be created
        val pattern = PatternState.createEmpty()
        assertEquals(9, pattern.steps.size)
    }

    @Test
    fun basicStepDisplayStateCreation() {
        // Simple test that StepDisplayState can be created
        val step = StepDisplayState(
            stepIndex = 0u,
            partIndex = 0u,
            gate = false,
            velocity = 1u,
            probability = 100u,
            microtiming = 0
        )
        assertEquals(0u, step.stepIndex)
    }

    @Test
    fun microtimingRangeIsValid() {
        val step = StepDisplayState(
            stepIndex = 0u, partIndex = 0u, gate = true, 
            velocity = 1u, probability = 100u, microtiming = 25
        )
        
        // Microtiming should be in range -50 to 50
        assertTrue(step.microtiming >= -50 && step.microtiming <= 50)
    }

    @Test
    fun probabilityRangeIsValid() {
        val step = StepDisplayState(
            stepIndex = 0u, partIndex = 0u, gate = true, 
            velocity = 1u, probability = 75u, microtiming = 0
        )
        
        // Probability should be in range 0-100
        assertTrue(step.probability >= 0u && step.probability <= 100u)
    }

    @Test
    fun patternWithMultiplePartsHasCorrectStructure() {
        val steps = List(9) { partIndex ->
            List(16) { stepIndex ->
                StepDisplayState(
                    stepIndex = stepIndex.toUInt(),
                    partIndex = partIndex.toUInt(),
                    gate = stepIndex == 0, // First step of each part
                    velocity = 1u,
                    probability = 100u,
                    microtiming = 0
                )
            }
        }
        
        val pattern = PatternState(
            currentStep = 0u,
            bpm = 120f,
            isPlaying = false,
            currentPatternId = 0u,
            patternLengthSteps = 16u,
            currentPage = 0u,
            patternSeed = 42u,
            steps = steps
        )
        
        assertEquals(9, pattern.steps.size)
        pattern.steps.forEachIndexed { partIndex, partSteps ->
            assertEquals(16, partSteps.size)
            assertTrue(partSteps[0].gate) // First step should be active
            assertEquals(partIndex.toUInt(), partSteps[0].partIndex)
        }
    }

    @Test
    fun patternStateEquality() {
        val pattern1 = PatternState.createEmpty()
        val pattern2 = PatternState.createEmpty()
        
        // Two empty patterns should be equal
        assertEquals(pattern1, pattern2)
    }

    @Test
    fun stepDisplayStateEquality() {
        val step1 = StepDisplayState(
            stepIndex = 0u, partIndex = 0u, gate = true, 
            velocity = 1u, probability = 100u, microtiming = 0
        )
        val step2 = StepDisplayState(
            stepIndex = 0u, partIndex = 0u, gate = true, 
            velocity = 1u, probability = 100u, microtiming = 0
        )
        
        assertEquals(step1, step2)
    }
}