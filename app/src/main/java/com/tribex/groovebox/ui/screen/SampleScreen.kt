package com.tribex.groovebox.ui.screen

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

/**
 * SAMPLE Screen
 * 
 * Placeholder for M3 - will be implemented in future milestone
 * Per SPEC v3.1: Sample browser, load/save, metadata
 */
@Composable
fun SampleScreen() {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        Text(
            text = "SAMPLE",
            style = MaterialTheme.typography.headlineSmall
        )
        
        Divider()
        
        Spacer(modifier = Modifier.height(32.dp))
        
        Text(
            text = "SAMPLE Screen",
            style = MaterialTheme.typography.titleLarge
        )
        
        Text(
            text = "(Placeholder - will be implemented in future milestone)",
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )
        
        Spacer(modifier = Modifier.height(32.dp))
        
        Text(
            text = "Per SPEC v3.1:\n" +
                    "- Sample browser\n" +
                    "- Load samples from storage\n" +
                    "- Sample metadata (name, length, format)\n" +
                    "- Waveform preview\n" +
                    "- Trim/loop points",
            style = MaterialTheme.typography.bodySmall,
            color = MaterialTheme.colorScheme.secondary
        )
    }
}