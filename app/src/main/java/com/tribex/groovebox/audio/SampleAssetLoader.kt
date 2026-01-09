package com.tribex.groovebox.audio

import android.content.Context
import android.content.res.AssetManager
import android.util.Log
import com.tribex.groovebox.engine.AudioEngineBridge
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File
import java.io.FileOutputStream
import java.io.IOException

/**
 * SampleAssetLoader - Loads drum samples from Android Assets
 * 
 * Loads WAV samples from app/src/main/assets/samples/
 * and passes them to the C++ Audio Engine via JNI.
 * 
 * M9: Sample Loading Integration
 */
class SampleAssetLoader(private val context: Context) {
    
    private val TAG = "SampleAssetLoader"
    
    // Default 909 kit mapping (8 drum parts)
    private val defaultKit = mapOf(
        0 to "909_kick.wav",
        1 to "909_snare.wav",
        2 to "909_clap.wav",
        3 to "909_clhat.wav",
        4 to "909_ohhat.wav",
        5 to "909_lotom.wav",
        6 to "909_hitom.wav",
        7 to "909_crash.wav"
    )
    
    /**
     * Load default 909 kit into all 8 drum parts
     * Call this on app startup
     */
    suspend fun loadDefaultKit(): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            Log.i(TAG, "Loading default 909 kit...")
            
            var successCount = 0
            for ((partIndex, filename) in defaultKit) {
                val result = loadSampleToPart(partIndex, filename)
                if (result.isSuccess) {
                    successCount++
                } else {
                    Log.e(TAG, "Failed to load $filename: ${result.exceptionOrNull()?.message}")
                }
            }
            
            if (successCount == defaultKit.size) {
                Log.i(TAG, "✅ All 8 samples loaded successfully")
                Result.success(Unit)
            } else {
                Log.w(TAG, "⚠️  Loaded $successCount/${defaultKit.size} samples")
                Result.failure(Exception("Only $successCount samples loaded"))
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to load default kit", e)
            Result.failure(e)
        }
    }
    
    /**
     * Load a single WAV sample into a specific part
     * 
     * @param partIndex Part index (0-7 for drums, 8 for synth)
     * @param filename Sample filename in assets/samples/
     */
    suspend fun loadSampleToPart(partIndex: Int, filename: String): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            // Validate part index
            if (partIndex < 0 || partIndex >= 8) {
                return@withContext Result.failure(IllegalArgumentException("Invalid part index: $partIndex"))
            }
            
            // Load WAV file from assets
            val assetPath = "samples/$filename"
            val wavData = loadWavFromAssets(assetPath)
                ?: return@withContext Result.failure(IOException("Failed to load $assetPath"))
            
            // Parse WAV file using WavParser
            val wavResult = WavParser.parseWav(wavData)
                ?: return@withContext Result.failure(Exception("Failed to parse WAV file: $filename"))
            
            // Convert to mono float32
            val audioData = WavParser.convertToMonoFloat32(wavData, wavResult)
                ?: return@withContext Result.failure(Exception("Failed to convert audio data: $filename"))
            
            val numSamples = audioData.size / 4 // float32 = 4 bytes per sample
            
            Log.d(TAG, "Loaded $filename: $numSamples samples, ${wavResult.sampleRate}Hz, ${wavResult.numChannels}ch")
            
            // Load into audio engine
            AudioEngineBridge.loadSample(
                partIndex = partIndex,
                sampleData = audioData,
                length = audioData.size,
                sampleRate = wavResult.sampleRate,
                sampleId = partIndex, // Use part index as sample ID
                startOffset = 0,
                endOffset = 0 // 0 = use full sample
            )
            
            Log.i(TAG, "✅ Loaded $filename to Part $partIndex")
            Result.success(Unit)
            
        } catch (e: Exception) {
            Log.e(TAG, "Failed to load sample $filename to part $partIndex", e)
            Result.failure(e)
        }
    }
    
    /**
     * Load WAV file from assets as byte array
     */
    private fun loadWavFromAssets(assetPath: String): ByteArray? {
        return try {
            context.assets.open(assetPath).use { inputStream ->
                inputStream.readBytes()
            }
        } catch (e: IOException) {
            Log.e(TAG, "Failed to read asset: $assetPath", e)
            null
        }
    }
    
    /**
     * List all available samples in assets/samples/
     */
    suspend fun listAvailableSamples(): Result<List<String>> = withContext(Dispatchers.IO) {
        try {
            val samples = context.assets.list("samples")?.toList() ?: emptyList()
            Log.d(TAG, "Found ${samples.size} samples in assets")
            Result.success(samples.filter { it.endsWith(".wav", ignoreCase = true) })
        } catch (e: Exception) {
            Log.e(TAG, "Failed to list samples", e)
            Result.failure(e)
        }
    }
    
    /**
     * Load a custom sample from external storage
     * 
     * @param partIndex Part index (0-7)
     * @param filePath Absolute file path to WAV file
     */
    suspend fun loadCustomSample(partIndex: Int, filePath: String): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            if (partIndex < 0 || partIndex >= 8) {
                return@withContext Result.failure(IllegalArgumentException("Invalid part index: $partIndex"))
            }
            
            val file = File(filePath)
            if (!file.exists() || !file.canRead()) {
                return@withContext Result.failure(IOException("Cannot read file: $filePath"))
            }
            
            // Read WAV file
            val wavData = file.readBytes()
            val wavResult = WavParser.parseWav(wavData)
                ?: return@withContext Result.failure(Exception("Failed to parse WAV file"))
            
            // Convert to mono float32
            val audioData = WavParser.convertToMonoFloat32(wavData, wavResult)
                ?: return@withContext Result.failure(Exception("Failed to convert audio data"))
            
            val numSamples = audioData.size / 4
            
            Log.d(TAG, "Loaded custom sample: $numSamples samples, ${wavResult.sampleRate}Hz")
            
            // Load into audio engine
            AudioEngineBridge.loadSample(
                partIndex = partIndex,
                sampleData = audioData,
                length = audioData.size,
                sampleRate = wavResult.sampleRate,
                sampleId = partIndex,
                startOffset = 0,
                endOffset = 0
            )
            
            Log.i(TAG, "✅ Loaded custom sample to Part $partIndex")
            Result.success(Unit)
            
        } catch (e: Exception) {
            Log.e(TAG, "Failed to load custom sample from $filePath", e)
            Result.failure(e)
        }
    }
    
    /**
     * Unload sample from a specific part
     */
    fun unloadSample(partIndex: Int) {
        if (partIndex < 0 || partIndex >= 8) {
            Log.e(TAG, "Invalid part index: $partIndex")
            return
        }
        
        AudioEngineBridge.unloadSample(partIndex)
        Log.i(TAG, "Unloaded sample from Part $partIndex")
    }
    
    /**
     * Reload default kit (useful after audio engine restart)
     */
    suspend fun reloadDefaultKit(): Result<Unit> {
        Log.i(TAG, "Reloading default kit...")
        return loadDefaultKit()
    }
}
