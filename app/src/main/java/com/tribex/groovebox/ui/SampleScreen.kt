package com.tribex.groovebox.ui

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.tribex.groovebox.engine.*

/**
 * Sample Screen - M4
 * 
 * Provides UI for:
 * - Importing WAV files
 * - Assigning samples to parts
 * - Adjusting voice parameters (pitch, pan, level, decay, filter)
 */

@Composable
fun SampleScreen(
    parts: List<PartSampleState>,
    onSampleLoaded: (Int, SampleImportResult) -> Unit,
    onVoiceParamChanged: (Int, VoiceParams) -> Unit,
    onPartMuteChanged: (Int, Boolean) -> Unit,
    onPartSoloChanged: (Int, Boolean) -> Unit,
    modifier: Modifier = Modifier
) {
    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        Text(
            text = "SAMPLE",
            style = MaterialTheme.typography.headlineMedium,
            modifier = Modifier.padding(bottom = 16.dp)
        )
        
        LazyColumn(
            modifier = Modifier.weight(1f),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            items(parts) { part ->
                PartSampleCard(
                    part = part,
                    onVoiceParamChanged = { params -> onVoiceParamChanged(part.partIndex, params) },
                    onPartMuteChanged = { muted -> onPartMuteChanged(part.partIndex, muted) },
                    onPartSoloChanged = { solo -> onPartSoloChanged(part.partIndex, solo) }
                )
            }
        }
        
        Spacer(modifier = Modifier.height(16.dp))
        
        SampleBrowser(
            onSampleLoaded = onSampleLoaded,
            modifier = Modifier.fillMaxWidth()
        )
    }
}

/**
 * Part Sample Card
 */
@Composable
fun PartSampleCard(
    part: PartSampleState,
    onVoiceParamChanged: (VoiceParams) -> Unit,
    onPartMuteChanged: (Boolean) -> Unit,
    onPartSoloChanged: (Boolean) -> Unit
) {
    var params by remember { mutableStateOf(part.params) }
    var expanded by remember { mutableStateOf(false) }
    
    LaunchedEffect(part.params) {
        params = part.params
    }
    
    Card(
        modifier = Modifier.fillMaxWidth()
    ) {
        Column(
            modifier = Modifier.padding(16.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = getPartName(part.partIndex),
                    style = MaterialTheme.typography.titleMedium
                )
                
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    AssistChip(
                        onClick = { onPartMuteChanged(!part.muted) },
                        label = { Text(if (part.muted) "M ON" else "M") }
                    )
                    AssistChip(
                        onClick = { onPartSoloChanged(!part.soloed) },
                        label = { Text(if (part.soloed) "S ON" else "S") }
                    )
                }
            }
            
            if (part.hasSample && part.metadata != null) {
                Text(
                    text = "Sample: ${part.metadata!!.name}",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            } else {
                Text(
                    text = "No sample loaded",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
            
            if (part.hasSample) {
                Button(
                    onClick = { expanded = !expanded },
                    modifier = Modifier.padding(top = 8.dp)
                ) {
                    Text(if (expanded) "Hide Controls" else "Show Controls")
                }
                
                if (expanded) {
                    Divider(modifier = Modifier.padding(vertical = 8.dp))
                    
                    ParameterControls(
                        params = params,
                        onParamChanged = { newParams ->
                            params = newParams
                            onVoiceParamChanged(newParams)
                        }
                    )
                }
            }
        }
    }
}

/**
 * Parameter Controls
 */
@Composable
fun ParameterControls(
    params: VoiceParams,
    onParamChanged: (VoiceParams) -> Unit
) {
    Column(
        modifier = Modifier.fillMaxWidth(),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        ParameterSlider(
            label = "Pitch",
            value = params.pitch,
            range = VoiceParams.PITCH_RANGE,
            unit = "st",
            onValueChanged = { onParamChanged(params.copy(pitch = it)) }
        )
        
        ParameterSlider(
            label = "Pan",
            value = params.pan,
            range = VoiceParams.PAN_RANGE,
            unit = "",
            displayValue = { value ->
                when {
                    value < -0.3f -> "L"
                    value > 0.3f -> "R"
                    else -> "C"
                }
            },
            onValueChanged = { onParamChanged(params.copy(pan = it)) }
        )
        
        ParameterSlider(
            label = "Level",
            value = params.level,
            range = VoiceParams.LEVEL_RANGE,
            unit = "%",
            displayValue = { (it * 100).toInt().toString() },
            onValueChanged = { onParamChanged(params.copy(level = it)) }
        )
        
        ParameterSlider(
            label = "Decay",
            value = params.decayMs,
            range = VoiceParams.DECAY_RANGE,
            unit = "ms",
            displayValue = { it.toInt().toString() },
            onValueChanged = { onParamChanged(params.copy(decayMs = it)) }
        )
        
        FilterSelector(
            selected = params.filter,
            onFilterChanged = { onParamChanged(params.copy(filter = it)) }
        )
    }
}

/**
 * Parameter Slider
 */
@Composable
fun ParameterSlider(
    label: String,
    value: Float,
    range: ClosedFloatingPointRange<Float>,
    unit: String = "",
    displayValue: ((Float) -> String)? = null,
    onValueChanged: (Float) -> Unit
) {
    Column(modifier = Modifier.fillMaxWidth()) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(
                text = label,
                style = MaterialTheme.typography.bodyMedium
            )
            Text(
                text = "${displayValue?.invoke(value) ?: String.format("%.1f", value)} $unit",
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.primary
            )
        }
        
        Slider(
            value = value,
            onValueChange = onValueChanged,
            valueRange = range
        )
    }
}

/**
 * Filter Selector
 */
@Composable
fun FilterSelector(
    selected: FilterType,
    onFilterChanged: (FilterType) -> Unit
) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        Button(
            onClick = { onFilterChanged(FilterType.LOW_PASS) },
            colors = ButtonDefaults.buttonColors(
                containerColor = if (selected == FilterType.LOW_PASS) 
                    MaterialTheme.colorScheme.primary 
                else MaterialTheme.colorScheme.secondaryContainer
            ),
            modifier = Modifier.weight(1f)
        ) {
            Text("LP")
        }
        
        Button(
            onClick = { onFilterChanged(FilterType.HIGH_PASS) },
            colors = ButtonDefaults.buttonColors(
                containerColor = if (selected == FilterType.HIGH_PASS) 
                    MaterialTheme.colorScheme.primary 
                else MaterialTheme.colorScheme.secondaryContainer
            ),
            modifier = Modifier.weight(1f)
        ) {
            Text("HP")
        }
    }
}

/**
 * Get part name from index
 */
private fun getPartName(index: Int): String {
    return when (index) {
        0 -> "BD"
        1 -> "SD"
        2 -> "CH"
        3 -> "OH"
        4 -> "CP"
        5 -> "LT"
        6 -> "MT"
        7 -> "HT"
        8 -> "SYNTH"
        else -> "PART $index"
    }
}