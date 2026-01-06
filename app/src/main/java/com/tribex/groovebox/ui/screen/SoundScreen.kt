package com.tribex.groovebox.ui.screen

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

/**
 * SOUND Screen
 * 
 * Placeholder for M3 - will be implemented in future milestone
 * Per SPEC v3.1: Part-level sample selection, filters, envelope
 */
@Composable
fun SoundScreen() {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        Text(
            text = "SOUND",
            style = MaterialTheme.typography.headlineSmall
        )
        
        Divider()
        
        Spacer(modifier = Modifier.height(32.dp))
        
        Text(
            text = "SOUND Screen",
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
                    "- Part-level sample selection\n" +
                    "- Filters (LP/HP/BP)\n" +
                    "- Envelope (ADSR)\n" +
                    "- Effects (reverb, delay, etc.)",
            style = MaterialTheme.typography.bodySmall,
            color = MaterialTheme.colorScheme.secondary
        )
    }
}