package com.tribex.groovebox.persistence

import androidx.room.TypeConverter
import kotlinx.serialization.encodeToString
import kotlinx.serialization.decodeFromString
import kotlinx.serialization.json.Json
import kotlinx.serialization.Serializable
import kotlinx.serialization.modules.SerializersModule
import kotlinx.serialization.modules.contextual

/**
 * Type Converters for Room Database
 * 
 * Handles conversion between complex types and storage types (String, ByteArray)
 */

// Serializable versions of complex types for JSON serialization

@Serializable
data class StepData(
    val gate: Boolean = false,
    val velocity: Int = 80,
    val microtiming: Int = 0,
    val probability: Int = 100,
    val locks: Map<String, Float> = emptyMap()
)

@Serializable
data class PatternData(
    val steps: List<Map<Int, StepData>>
) {
    /**
     * Get gate state for a specific part and step
     * @param partIndex Part index (0-8)
     * @param stepIndex Step index
     * @return Gate state (true = active, false = inactive)
     */
    fun getGate(partIndex: Int, stepIndex: Int): Boolean {
        if (stepIndex >= steps.size) return false
        val stepMap = steps[stepIndex]
        val stepData = stepMap[partIndex]
        return stepData?.gate ?: false
    }
}

/**
 * JSON Converter for pattern steps
 */
object PatternStepsConverter {
    
    private val json = Json {
        ignoreUnknownKeys = true
        encodeDefaults = true
    }
    
    @TypeConverter
    fun fromPatternData(patternData: PatternData): String {
        return json.encodeToString(patternData)
    }
    
    @TypeConverter
    fun toPatternData(jsonString: String): PatternData {
        return json.decodeFromString(jsonString)
    }
}

/**
 * ByteArray converter for waveform data
 */
object ByteArrayConverter {
    
    @TypeConverter
    fun toByteArray(bytes: ByteArray?): ByteArray {
        return bytes ?: byteArrayOf()
    }
    
    @TypeConverter
    fun fromByteArray(bytes: ByteArray): ByteArray {
        return bytes
    }
}

/**
 * Unified Converters class for Room @TypeConverters annotation
 */
class Converters {
    
    @TypeConverter
    fun fromPatternData(patternData: PatternData): String {
        return PatternStepsConverter.fromPatternData(patternData)
    }
    
    @TypeConverter
    fun toPatternData(jsonString: String): PatternData {
        return PatternStepsConverter.toPatternData(jsonString)
    }
    
    @TypeConverter
    fun toByteArray(bytes: ByteArray?): ByteArray {
        return ByteArrayConverter.toByteArray(bytes)
    }
    
    @TypeConverter
    fun fromByteArray(bytes: ByteArray): ByteArray {
        return ByteArrayConverter.fromByteArray(bytes)
    }
}