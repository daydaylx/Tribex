package com.tribex.groovebox.engine

import org.junit.Assert.assertEquals
import org.junit.Test

class SequencerModelsTest {
    @Test
    fun stepSerializationRoundTrip() {
        val step = StepData(
            gate = 1u,
            velocity = Velocity.MAX,
            microtiming = (-10).toByte(),
            probability = 64u
        )

        val bytes = PatternSerializer.stepToBytes(step)
        assertEquals(byteArrayOf(1, 3, -10, 64).toList(), bytes.toList())

        val decoded = PatternSerializer.stepFromBytes(bytes)
        assertEquals(step, decoded)
    }

    @Test(expected = IllegalArgumentException::class)
    fun stepFromBytesRejectsWrongSize() {
        PatternSerializer.stepFromBytes(byteArrayOf(1, 2, 3))
    }

    @Test
    fun velocityConversionsRespectThresholds() {
        assertEquals(0.3f, Velocity.toFloat(Velocity.GHOST), 0.0001f)
        assertEquals(0.6f, Velocity.toFloat(Velocity.NORMAL), 0.0001f)
        assertEquals(0.9f, Velocity.toFloat(Velocity.ACCENT), 0.0001f)
        assertEquals(1.0f, Velocity.toFloat(Velocity.MAX), 0.0001f)

        assertEquals(Velocity.GHOST, Velocity.fromFloat(0.2f))
        assertEquals(Velocity.NORMAL, Velocity.fromFloat(0.45f))
        assertEquals(Velocity.ACCENT, Velocity.fromFloat(0.94f))
        assertEquals(Velocity.MAX, Velocity.fromFloat(0.95f))
    }

    @Test
    fun microtimingClampEnforcesBounds() {
        assertEquals(Microtiming.MIN, Microtiming.clamp((-100).toByte()))
        assertEquals(Microtiming.MAX, Microtiming.clamp(100.toByte()))
        assertEquals(10.toByte(), Microtiming.clamp(10.toByte()))
    }

    @Test
    fun sampleMetadataTrimRangeUpdatesOffsets() {
        val metadata = SampleMetadata(
            id = 1,
            name = "Test",
            lengthMs = 1000,
            sampleRate = 1000
        )

        metadata.setTrimRange(0.1f, 0.9f)
        assertEquals(100, metadata.startOffset)
        assertEquals(900, metadata.endOffset)

        metadata.setTrimRange(0.25f, 1.0f)
        assertEquals(250, metadata.startOffset)
        assertEquals(0, metadata.endOffset)
    }

    @Test
    fun voiceParamsClampLimitsValues() {
        val params = VoiceParams(
            pitch = 99f,
            pan = -2f,
            level = 2f,
            decayMs = -100f
        ).clamp()

        assertEquals(VoiceParams.PITCH_RANGE.endInclusive, params.pitch, 0.0001f)
        assertEquals(VoiceParams.PAN_RANGE.start, params.pan, 0.0001f)
        assertEquals(VoiceParams.LEVEL_RANGE.endInclusive, params.level, 0.0001f)
        assertEquals(VoiceParams.DECAY_RANGE.start, params.decayMs, 0.0001f)
    }
}
