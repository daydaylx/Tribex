package com.tribex.groovebox.ui

import android.content.res.Configuration
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.unit.dp
import com.tribex.groovebox.engine.*
import com.tribex.groovebox.ui.components.WaveformPreview

/**
 * Sample Screen - M9 Landscape Optimized
 * 
 * Adaptive layout:
 * - Portrait: List of cards (legacy behavior)
 * - Landscape: Master-Detail (Left: Part List, Right: Editor)
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
    val configuration = LocalConfiguration.current
    val isLandscape = configuration.orientation == Configuration.ORIENTATION_LANDSCAPE
    
    // State for selected part in landscape mode
    var selectedPartIndex by remember { mutableStateOf(0) }
    
    if (isLandscape) {
        // Master-Detail Layout
        Row(
            modifier = modifier.fillMaxSize().padding(8.dp),
            horizontalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            // Master List (Left Pane)
            Card(
                modifier = Modifier
                    .width(200.dp)
                    .fillMaxHeight(),
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface)
            ) {
                Column(modifier = Modifier.padding(8.dp)) {
                    Text(
                        text = "PARTS",
                        style = MaterialTheme.typography.titleMedium,
                        modifier = Modifier.padding(bottom = 8.dp)
                    )
                    LazyColumn(
                        verticalArrangement = Arrangement.spacedBy(4.dp)
                    ) {
                        items(parts) { part ->
                            PartListItem(
                                part = part,
                                isSelected = part.partIndex == selectedPartIndex,
                                onClick = { selectedPartIndex = part.partIndex },
                                onMute = { onPartMuteChanged(part.partIndex, !part.muted) },
                                onSolo = { onPartSoloChanged(part.partIndex, !part.soloed) }
                            )
                        }
                    }
                }
            }
            
            // Detail View (Right Pane)
            Box(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxHeight()
            ) {
                val selectedPart = parts.find { it.partIndex == selectedPartIndex }
                if (selectedPart != null) {
                    PartDetailEditor(
                        part = selectedPart,
                        onVoiceParamChanged = { params -> onVoiceParamChanged(selectedPart.partIndex, params) },
                        onSampleLoaded = { result -> onSampleLoaded(selectedPart.partIndex, result) }
                    )
                } else {
                    Box(modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                        Text("Select a part to edit")
                    }
                }
            }
        }
    } else {
        // Portrait Layout (Legacy List)
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
}

/**
 * Compact Part Item for Master List
 */
@Composable
fun PartListItem(
    part: PartSampleState,
    isSelected: Boolean,
    onClick: () -> Unit,
    onMute: () -> Unit,
    onSolo: () -> Unit
) {
    val backgroundColor = if (isSelected) 
        MaterialTheme.colorScheme.primaryContainer 
    else 
        MaterialTheme.colorScheme.surfaceVariant

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(8.dp))
            .background(backgroundColor)
            .clickable(onClick = onClick)
            .padding(8.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(
            text = getPartName(part.partIndex),
            style = MaterialTheme.typography.labelLarge,
            color = if (isSelected) MaterialTheme.colorScheme.onPrimaryContainer else MaterialTheme.colorScheme.onSurfaceVariant
        )
        
        Row(horizontalArrangement = Arrangement.spacedBy(4.dp)) {
            // Tiny Mute/Solo indicators
            Box(
                modifier = Modifier
                    .size(24.dp)
                    .clip(RoundedCornerShape(4.dp))
                    .background(if (part.muted) MaterialTheme.colorScheme.error else MaterialTheme.colorScheme.surface.copy(alpha=0.5f))
                    .clickable(onClick = onMute),
                contentAlignment = Alignment.Center
            ) {
                Text("M", style = MaterialTheme.typography.labelSmall, color = MaterialTheme.colorScheme.onError)
            }
            
            Box(
                modifier = Modifier
                    .size(24.dp)
                    .clip(RoundedCornerShape(4.dp))
                    .background(if (part.soloed) MaterialTheme.colorScheme.tertiary else MaterialTheme.colorScheme.surface.copy(alpha=0.5f))
                    .clickable(onClick = onSolo),
                contentAlignment = Alignment.Center
            ) {
                Text("S", style = MaterialTheme.typography.labelSmall, color = MaterialTheme.colorScheme.onTertiary)
            }
        }
    }
}

/**
 * Detail Editor Pane
 */
@Composable
fun PartDetailEditor(
    part: PartSampleState,
    onVoiceParamChanged: (VoiceParams) -> Unit,
    onSampleLoaded: (SampleImportResult) -> Unit
) {
    Card(
        modifier = Modifier.fillMaxSize(),
        colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface)
    ) {
        Column(
            modifier = Modifier.padding(16.dp).fillMaxSize()
        ) {
            // Header
            Text(
                text = "EDIT: ${getPartName(part.partIndex)}",
                style = MaterialTheme.typography.headlineSmall
            )
            
            HorizontalDivider(modifier = Modifier.padding(vertical = 8.dp))
            
            Row(
                modifier = Modifier.weight(1f),
                horizontalArrangement = Arrangement.spacedBy(16.dp)
            ) {
                // Left Column: Sample & Waveform
                Column(
                    modifier = Modifier.weight(1f)
                ) {
                    Text("SAMPLE", style = MaterialTheme.typography.titleSmall)
                    
                    if (part.hasSample && part.metadata != null) {
                        Text(
                            text = part.metadata.name,
                            style = MaterialTheme.typography.bodyMedium,
                            modifier = Modifier.padding(vertical = 4.dp)
                        )
                        
                        // Waveform Preview
                        part.metadata.waveform?.let { waveform ->
                            WaveformPreview(
                                waveform = waveform,
                                trimStartPercent = 0f, // TODO: connect to state
                                trimEndPercent = 1f,
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .height(120.dp)
                                    .padding(vertical = 8.dp)
                            )
                        }
                    } else {
                        Box(
                            modifier = Modifier
                                .fillMaxWidth()
                                .height(120.dp)
                                .background(MaterialTheme.colorScheme.surfaceVariant),
                            contentAlignment = Alignment.Center
                        ) {
                            Text("No Sample")
                        }
                    }
                    
                    Spacer(modifier = Modifier.height(8.dp))
                    
                    SampleBrowser(
                        onSampleLoaded = { _, result -> onSampleLoaded(result) }, // Ignore partIndex from browser
                        modifier = Modifier.fillMaxWidth()
                    )
                }
                
                // Vertical Divider
                VerticalDivider(
                    modifier = Modifier
                        .fillMaxHeight()
                        .width(1.dp)
                )
                
                // Right Column: Parameters
                Column(
                    modifier = Modifier.weight(1f).verticalScroll(rememberScrollState())
                ) {
                    Text("PARAMETERS", style = MaterialTheme.typography.titleSmall)
                    Spacer(modifier = Modifier.height(8.dp))
                    
                    ParameterControls(
                        params = part.params,
                        onParamChanged = onVoiceParamChanged
                    )
                }
            }
        }
    }
}

// ... Keep existing PartSampleCard, ParameterControls, etc. for Portrait mode ...

/**
 * Part Sample Card (Legacy / Portrait)
 */
@Composable
fun PartSampleCard(
    part: PartSampleState,
    onVoiceParamChanged: (VoiceParams) -> Unit,
    onPartMuteChanged: (Boolean) -> Unit,
    onPartSoloChanged: (Boolean) -> Unit,
    onTrimRangeChanged: (Float, Float) -> Unit = { _, _ -> }
) {
    var params by remember { mutableStateOf(part.params) }
    var expanded by remember { mutableStateOf(false) }
    var trimStart by remember { mutableStateOf(0f) }
    var trimEnd by remember { mutableStateOf(1f) }
    
    LaunchedEffect(part.params) {
        params = part.params
    }
    
    LaunchedEffect(part.metadata) {
        val metadata = part.metadata ?: return@LaunchedEffect
        val (start, end) = metadata.getTrimRange()
        trimStart = start
        trimEnd = end
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
            
            val metadata = part.metadata
            if (part.hasSample && metadata != null) {
                Text(
                    text = "Sample: ${metadata.name}",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
                
                // M4.5: Waveform Preview
                val waveform = metadata.waveform
                if (waveform != null) {
                    Spacer(modifier = Modifier.height(8.dp))
                    WaveformPreview(
                        waveform = waveform,
                        trimStartPercent = trimStart,
                        trimEndPercent = trimEnd,
                        modifier = Modifier.fillMaxWidth()
                    )
                    
                    // M4.5: Trim Slider (simplified for M4.5 - using RangeSlider)
                    Spacer(modifier = Modifier.height(8.dp))
                    Text(
                        text = "Trim",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.primary
                    )
                    RangeSlider(
                        value = trimStart..trimEnd,
                        onValueChange = { range ->
                            trimStart = range.start
                            trimEnd = range.endInclusive
                            onTrimRangeChanged(trimStart, trimEnd)
                        },
                        valueRange = 0f..1f,
                        steps = 100
                    )
                }
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
                    HorizontalDivider(modifier = Modifier.padding(vertical = 8.dp))
                    
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