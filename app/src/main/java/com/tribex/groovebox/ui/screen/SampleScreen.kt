package com.tribex.groovebox.ui.screen

import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp

/**
 * SAMPLE Screen
 * 
 * M9: Landscape Split-View
 * Left: Sample Browser (list of available samples)
 * Right: Sample Details (assignment, waveform preview placeholder, trim controls placeholder)
 * 
 * Per SPEC v3.1: Sample browser, load/save, metadata
 * Deferred items (M10): Waveform rendering, Trim/Loop point editing
 */
@Composable
fun SampleScreen() {
    // M9: Landscape Row layout - Split view
    Row(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        horizontalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        // Left Column - Sample Browser
        Column(
            modifier = Modifier
                .weight(1f)
                .fillMaxHeight()
                .border(
                    border = androidx.compose.foundation.BorderStroke(
                        1.dp,
                        MaterialTheme.colorScheme.outlineVariant
                    ),
                    shape = RoundedCornerShape(8.dp)
                )
                .padding(12.dp)
        ) {
            Text(
                text = "SAMPLE BROWSER",
                style = MaterialTheme.typography.titleMedium,
                color = MaterialTheme.colorScheme.secondary
            )
            
            Spacer(modifier = Modifier.height(8.dp))
            
            // Sample List
            LazyColumn(
                modifier = Modifier.fillMaxSize(),
                verticalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                items(getPlaceholderSamples()) { sample ->
                    SampleListItem(
                        name = sample.name,
                        duration = sample.duration,
                        format = sample.format,
                        isSelected = sample.name == "Kick_808.wav"
                    )
                }
            }
        }
        
        // Right Column - Sample Details
        Column(
            modifier = Modifier
                .weight(1f)
                .fillMaxHeight(),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            // Sample Details Header
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .border(
                        border = androidx.compose.foundation.BorderStroke(
                            1.dp,
                            MaterialTheme.colorScheme.outlineVariant
                        ),
                        shape = RoundedCornerShape(8.dp)
                    )
                    .padding(12.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Text(
                    text = "SAMPLE DETAILS",
                    style = MaterialTheme.typography.titleMedium,
                    color = MaterialTheme.colorScheme.secondary
                )
                
                Text(
                    text = "Kick_808.wav",
                    style = MaterialTheme.typography.titleLarge,
                    fontWeight = FontWeight.Bold
                )
                
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text(
                        text = "Duration: 0.45s",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                    Text(
                        text = "Format: WAV",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }
            
            // Assign Controls
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .border(
                        border = androidx.compose.foundation.BorderStroke(
                            1.dp,
                            MaterialTheme.colorScheme.outlineVariant
                        ),
                        shape = RoundedCornerShape(8.dp)
                    )
                    .padding(12.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Text(
                    text = "ASSIGN TO PART",
                    style = MaterialTheme.typography.titleMedium,
                    color = MaterialTheme.colorScheme.secondary
                )
                
                // Part Selection Row
                val parts = listOf("BD", "SD", "CH", "OH", "CP", "LT", "MT", "HT")
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(4.dp)
                ) {
                    parts.forEach { part ->
                        OutlinedButton(
                            onClick = { /* Assign to part */ },
                            modifier = Modifier.weight(1f)
                        ) {
                            Text(part, style = MaterialTheme.typography.labelSmall)
                        }
                    }
                }
            }
            
            // Waveform Preview Placeholder
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .weight(1f)
                    .border(
                        border = androidx.compose.foundation.BorderStroke(
                            1.dp,
                            MaterialTheme.colorScheme.outlineVariant
                        ),
                        shape = RoundedCornerShape(8.dp)
                    )
                    .padding(12.dp),
                contentAlignment = Alignment.Center
            ) {
                Column(
                    horizontalAlignment = Alignment.CenterHorizontally,
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    Text(
                        text = "WAVEFORM PREVIEW",
                        style = MaterialTheme.typography.titleMedium,
                        color = MaterialTheme.colorScheme.secondary
                    )
                    
                    Text(
                        text = "(M10: Will be implemented)",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.outline
                    )
                    
                    Text(
                        text = "Waveform rendering and playback preview",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }
            
            // Trim Controls Placeholder
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .border(
                        border = androidx.compose.foundation.BorderStroke(
                            1.dp,
                            MaterialTheme.colorScheme.outlineVariant
                        ),
                        shape = RoundedCornerShape(8.dp)
                    )
                    .padding(12.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Text(
                    text = "TRIM CONTROLS",
                    style = MaterialTheme.typography.titleMedium,
                    color = MaterialTheme.colorScheme.secondary
                )
                
                Text(
                    text = "(M10: Will be implemented)",
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.outline
                )
                
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Column {
                        Text(
                            text = "Start:",
                            style = MaterialTheme.typography.labelMedium,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                        Text(
                            text = "0.000s",
                            style = MaterialTheme.typography.bodyMedium,
                            fontWeight = FontWeight.Bold
                        )
                    }
                    
                    Column {
                        Text(
                            text = "End:",
                            style = MaterialTheme.typography.labelMedium,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                        Text(
                            text = "0.450s",
                            style = MaterialTheme.typography.bodyMedium,
                            fontWeight = FontWeight.Bold
                        )
                    }
                }
            }
        }
    }
}

/**
 * Sample List Item
 */
@Composable
fun SampleListItem(
    name: String,
    duration: String,
    format: String,
    isSelected: Boolean
) {
    val backgroundColor = if (isSelected) {
        MaterialTheme.colorScheme.primaryContainer
    } else {
        MaterialTheme.colorScheme.surface
    }
    
    val textColor = if (isSelected) {
        MaterialTheme.colorScheme.onPrimaryContainer
    } else {
        MaterialTheme.colorScheme.onSurface
    }
    
    Card(
        onClick = { /* Select sample */ },
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = backgroundColor
        )
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(8.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = name,
                style = MaterialTheme.typography.bodyMedium,
                color = textColor,
                fontWeight = if (isSelected) FontWeight.Bold else FontWeight.Normal
            )
            
            Row(
                horizontalArrangement = Arrangement.spacedBy(8.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = duration,
                    style = MaterialTheme.typography.bodySmall,
                    color = textColor
                )
                
                Text(
                    text = format,
                    style = MaterialTheme.typography.labelSmall,
                    color = MaterialTheme.colorScheme.outline
                )
            }
        }
    }
}

/**
 * Placeholder sample data
 */
data class SampleItem(
    val name: String,
    val duration: String,
    val format: String
)

fun getPlaceholderSamples(): List<SampleItem> {
    return listOf(
        SampleItem("Kick_808.wav", "0.45s", "WAV"),
        SampleItem("Kick_909.wav", "0.52s", "WAV"),
        SampleItem("Snare_808.wav", "0.38s", "WAV"),
        SampleItem("Snare_909.wav", "0.42s", "WAV"),
        SampleItem("HiHat_Closed.wav", "0.12s", "WAV"),
        SampleItem("HiHat_Open.wav", "0.35s", "WAV"),
        SampleItem("Clap_Dry.wav", "0.28s", "WAV"),
        SampleItem("Clap_Reverb.wav", "0.45s", "WAV"),
        SampleItem("Tom_Low.wav", "0.65s", "WAV"),
        SampleItem("Tom_Mid.wav", "0.58s", "WAV"),
        SampleItem("Tom_High.wav", "0.52s", "WAV"),
        SampleItem("Cymbal_Crash.wav", "1.85s", "WAV"),
        SampleItem("Cymbal_Ride.wav", "2.10s", "WAV"),
        SampleItem("Perc_Loop.wav", "1.00s", "WAV")
    )
}