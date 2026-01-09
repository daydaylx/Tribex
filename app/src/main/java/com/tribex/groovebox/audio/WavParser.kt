package com.tribex.groovebox.audio

import java.nio.ByteBuffer
import java.nio.ByteOrder

data class WavParseResult(
    val sampleRate: Int,
    val bitsPerSample: Int,
    val numChannels: Int,
    val dataOffset: Int,
    val dataLength: Int
)

object WavParser {
    fun parseWav(data: ByteArray): WavParseResult? {
        if (data.size < 44) return null

        val buffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)

        if (String(data, 0, 4) != "RIFF") return null
        if (String(data, 8, 4) != "WAVE") return null
        if (String(data, 12, 4) != "fmt ") return null

        val audioFormat = buffer.getShort(20).toInt()
        if (audioFormat != 1) return null

        val numChannels = buffer.getShort(22).toInt()
        if (numChannels != 1 && numChannels != 2) return null

        val sampleRate = buffer.getInt(24)
        val bitsPerSample = buffer.getShort(34).toInt()

        var offset = 36
        var dataOffset = 0
        var dataLength = 0

        while (offset + 8 <= data.size) {
            val chunkId = String(data, offset, 4)
            val chunkSize = buffer.getInt(offset + 4)

            if (chunkId == "data") {
                dataOffset = offset + 8
                dataLength = chunkSize
                break
            }

            offset += 8 + chunkSize
        }

        if (dataOffset == 0) return null
        if (dataOffset + dataLength > data.size) return null

        return WavParseResult(
            sampleRate = sampleRate,
            bitsPerSample = bitsPerSample,
            numChannels = numChannels,
            dataOffset = dataOffset,
            dataLength = dataLength
        )
    }

    fun convertToMonoFloat32(data: ByteArray, wav: WavParseResult): ByteArray? {
        val buffer = ByteBuffer.wrap(data, wav.dataOffset, wav.dataLength)
            .order(ByteOrder.LITTLE_ENDIAN)

        val bytesPerSample = wav.bitsPerSample / 8
        if (bytesPerSample <= 0) return null

        val channels = wav.numChannels
        if (channels <= 0) return null

        val numSamples = wav.dataLength / bytesPerSample
        if (numSamples % channels != 0) return null

        val frameCount = numSamples / channels
        val floatBuffer = ByteBuffer.allocate(frameCount * 4).order(ByteOrder.LITTLE_ENDIAN)

        fun writeSample(frameIndex: Int, sample: Float) {
            floatBuffer.putFloat(frameIndex * 4, sample)
        }

        when (wav.bitsPerSample) {
            16 -> {
                val shortBuffer = buffer.asShortBuffer()
                for (frame in 0 until frameCount) {
                    val base = frame * channels
                    val left = shortBuffer.get(base).toFloat() / 32768.0f
                    val sample = if (channels == 1) {
                        left
                    } else {
                        val right = shortBuffer.get(base + 1).toFloat() / 32768.0f
                        (left + right) * 0.5f
                    }
                    writeSample(frame, sample)
                }
            }
            24 -> {
                fun readSample(sampleIndex: Int): Float {
                    val byteIndex = sampleIndex * 3
                    val byte0 = buffer.get(byteIndex).toInt() and 0xFF
                    val byte1 = buffer.get(byteIndex + 1).toInt() and 0xFF
                    val byte2 = buffer.get(byteIndex + 2).toInt() and 0xFF
                    var sample = byte0 or (byte1 shl 8) or (byte2 shl 16)
                    if (sample and 0x800000 != 0) {
                        sample = sample or 0xFF000000.toInt()
                    }
                    return sample.toFloat() / 8388608.0f
                }

                for (frame in 0 until frameCount) {
                    val base = frame * channels
                    val left = readSample(base)
                    val sample = if (channels == 1) {
                        left
                    } else {
                        val right = readSample(base + 1)
                        (left + right) * 0.5f
                    }
                    writeSample(frame, sample)
                }
            }
            32 -> {
                val intBuffer = buffer.asIntBuffer()
                for (frame in 0 until frameCount) {
                    val base = frame * channels
                    val left = intBuffer.get(base).toFloat() / 2147483648.0f
                    val sample = if (channels == 1) {
                        left
                    } else {
                        val right = intBuffer.get(base + 1).toFloat() / 2147483648.0f
                        (left + right) * 0.5f
                    }
                    writeSample(frame, sample)
                }
            }
            else -> return null
        }

        return floatBuffer.array()
    }
}
