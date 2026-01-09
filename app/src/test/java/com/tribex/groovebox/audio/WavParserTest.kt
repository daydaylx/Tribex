package com.tribex.groovebox.audio

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlin.math.abs

class WavParserTest {
    private fun buildWav(
        numChannels: Int,
        bitsPerSample: Int,
        sampleRate: Int,
        data: ByteArray
    ): ByteArray {
        val headerSize = 44
        val totalSize = headerSize + data.size
        val buffer = ByteBuffer.allocate(totalSize).order(ByteOrder.LITTLE_ENDIAN)

        buffer.put("RIFF".toByteArray())
        buffer.putInt(36 + data.size)
        buffer.put("WAVE".toByteArray())
        buffer.put("fmt ".toByteArray())
        buffer.putInt(16)
        buffer.putShort(1)
        buffer.putShort(numChannels.toShort())
        buffer.putInt(sampleRate)
        val byteRate = sampleRate * numChannels * (bitsPerSample / 8)
        buffer.putInt(byteRate)
        val blockAlign = numChannels * (bitsPerSample / 8)
        buffer.putShort(blockAlign.toShort())
        buffer.putShort(bitsPerSample.toShort())
        buffer.put("data".toByteArray())
        buffer.putInt(data.size)
        buffer.put(data)

        return buffer.array()
    }

    private fun toFloatArray(bytes: ByteArray): FloatArray {
        val buffer = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN)
        val count = bytes.size / 4
        val out = FloatArray(count)
        for (i in 0 until count) {
            out[i] = buffer.getFloat(i * 4)
        }
        return out
    }

    @Test
    fun parseWav_validMono16() {
        val data = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN)
            .putShort(0, 0)
            .putShort(2, 32767.toShort())
            .array()
        val wavBytes = buildWav(1, 16, 44100, data)

        val result = WavParser.parseWav(wavBytes)
        assertNotNull(result)
        assertEquals(1, result!!.numChannels)
        assertEquals(16, result.bitsPerSample)
        assertEquals(44100, result.sampleRate)
    }

    @Test
    fun parseWav_rejectUnsupportedChannels() {
        val data = ByteArray(6)
        val wavBytes = buildWav(3, 16, 44100, data)
        assertNull(WavParser.parseWav(wavBytes))
    }

    @Test
    fun convertToMonoFloat32_stereoDownmix16() {
        val data = ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN)
            .putShort(0, 32767.toShort())
            .putShort(2, (-32768).toShort())
            .putShort(4, 0)
            .putShort(6, 0)
            .array()
        val wavBytes = buildWav(2, 16, 44100, data)
        val parsed = WavParser.parseWav(wavBytes)
        assertNotNull(parsed)

        val converted = WavParser.convertToMonoFloat32(wavBytes, parsed!!)
        assertNotNull(converted)
        assertEquals(8, converted!!.size)

        val floats = toFloatArray(converted)
        assertTrue(abs(floats[0]) < 1e-4f)
        assertTrue(abs(floats[1]) < 1e-4f)
    }

    @Test
    fun convertToMonoFloat32_mono32Length() {
        val data = ByteBuffer.allocate(12).order(ByteOrder.LITTLE_ENDIAN)
            .putInt(0, 2147483647)
            .putInt(4, 0)
            .putInt(8, -2147483648)
            .array()
        val wavBytes = buildWav(1, 32, 48000, data)
        val parsed = WavParser.parseWav(wavBytes)
        assertNotNull(parsed)

        val converted = WavParser.convertToMonoFloat32(wavBytes, parsed!!)
        assertNotNull(converted)
        assertEquals(12, converted!!.size)
    }
}
