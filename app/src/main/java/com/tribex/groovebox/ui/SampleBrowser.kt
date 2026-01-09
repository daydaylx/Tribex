package com.tribex.groovebox.ui

import android.net.Uri
import android.provider.OpenableColumns
import android.util.Log
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.tribex.groovebox.audio.WavParser
import com.tribex.groovebox.engine.SampleImportResult
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.ByteArrayOutputStream

/**
 * Sample Browser Component - M4
 * 
 * Provides UI for importing WAV files via Storage Access Framework.
 * 
 * Features:
 * - File picker for WAV files
 * - WAV parsing to extract metadata
 * - Sample loading into audio engine
 * - Error handling
 */

@Composable
fun SampleBrowser(
    onSampleLoaded: (Int, SampleImportResult) -> Unit,
    modifier: Modifier = Modifier
) {
    val context = LocalContext.current
    var selectedUri by remember { mutableStateOf<Uri?>(null) }
    var isLoading by remember { mutableStateOf(false) }
    var error by remember { mutableStateOf<String?>(null) }
    var nextSampleId by remember { mutableIntStateOf(1) }
    val scope = rememberCoroutineScope()
    
    // File picker launcher
    val filePickerLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.GetContent()
    ) { uri: Uri? ->
        uri?.let {
            selectedUri = it
            error = null
        }
    }
    
    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        Text(
            text = "Sample Browser",
            style = MaterialTheme.typography.headlineSmall
        )
        
        Text(
            text = "Import WAV files (16/24/32-bit PCM)",
            style = MaterialTheme.typography.bodySmall,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )
        
        // Import button
        Button(
            onClick = { filePickerLauncher.launch("audio/wav") },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Select WAV File")
        }
        
        // Selected file info
        selectedUri?.let { uri ->
            val fileName = getFileName(context, uri)
            Text(
                text = "Selected: $fileName",
                style = MaterialTheme.typography.bodyMedium,
                modifier = Modifier.padding(vertical = 8.dp)
            )
            
            HorizontalDivider()
            
            // Load button
            Button(
                onClick = {
                    isLoading = true
                    scope.launch {
                        val result = loadSample(context, uri, nextSampleId)
                        isLoading = false
                        if (result.success) {
                            onSampleLoaded(nextSampleId, result)
                            nextSampleId++
                            selectedUri = null
                        } else {
                            error = result.error
                        }
                    }
                },
                enabled = !isLoading,
                modifier = Modifier.fillMaxWidth()
            ) {
                if (isLoading) {
                    CircularProgressIndicator(
                        modifier = Modifier.size(16.dp),
                        color = MaterialTheme.colorScheme.onPrimary
                    )
                    Spacer(Modifier.width(8.dp))
                    Text("Loading...")
                } else {
                    Text("Load Sample")
                }
            }
        }
        
        // Error message
        error?.let { errorMsg ->
            Card(
                colors = CardDefaults.cardColors(
                    containerColor = MaterialTheme.colorScheme.errorContainer
                ),
                modifier = Modifier.fillMaxWidth()
            ) {
                Text(
                    text = errorMsg,
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onErrorContainer,
                    modifier = Modifier.padding(16.dp)
                )
            }
        }
    }
}

/**
 * Get file name from URI
 */
private fun getFileName(context: android.content.Context, uri: Uri): String {
    var result: String? = null
    context.contentResolver.query(uri, null, null, null, null)?.use { cursor ->
        if (cursor.moveToFirst()) {
            val index = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
            if (index >= 0) {
                result = cursor.getString(index)
            }
        }
    }
    return result ?: "Unknown File"
}

/**
 * Load sample from URI
 * 
 * Parses WAV file and loads it into audio engine.
 * 
 * @param context Android context
 * @param uri File URI
 * @param sampleId Unique sample ID
 * @return SampleImportResult with metadata or error
 */
private suspend fun loadSample(
    context: android.content.Context,
    uri: Uri,
    sampleId: Int
): SampleImportResult = withContext(Dispatchers.IO) {
    try {
        // Read file into memory
        val inputStream = context.contentResolver.openInputStream(uri)
            ?: return@withContext SampleImportResult(error = "Failed to open file")
        
        val outputStream = ByteArrayOutputStream()
        val buffer = ByteArray(8192)
        var bytesRead: Int
        var totalBytes = 0
        
        while (inputStream.read(buffer).also { bytesRead = it } != -1) {
            outputStream.write(buffer, 0, bytesRead)
            totalBytes += bytesRead
            
            // Limit file size to 100MB
            if (totalBytes > 100 * 1024 * 1024) {
                inputStream.close()
                return@withContext SampleImportResult(error = "File too large (max 100MB)")
            }
        }
        
        inputStream.close()
        val fileData = outputStream.toByteArray()
        
        // Parse WAV header
        val wavResult = WavParser.parseWav(fileData)
        
        if (wavResult == null) {
            return@withContext SampleImportResult(error = "Invalid WAV file format")
        }
        
        // Convert audio data to float32
        val floatData = WavParser.convertToMonoFloat32(fileData, wavResult)
        
        if (floatData == null) {
            return@withContext SampleImportResult(error = "Failed to convert audio data")
        }
        
        // Calculate duration in ms (float32 mono samples)
        val sampleCount = floatData.size / 4
        val durationMs = (sampleCount * 1000L) / wavResult.sampleRate
        
        // Create metadata
        val metadata = com.tribex.groovebox.engine.SampleMetadata(
            id = sampleId,
            name = getFileName(context, uri),
            lengthMs = durationMs,
            sampleRate = wavResult.sampleRate,
            startOffset = 0,
            endOffset = sampleCount
        )
        
        SampleImportResult(
            success = true,
            sampleId = sampleId,
            metadata = metadata,
            sampleData = floatData,
            error = null
        )
        
    } catch (e: Exception) {
        Log.e("SampleBrowser", "Error loading sample", e)
        SampleImportResult(error = "Failed to load sample: ${e.message}")
    }
}
