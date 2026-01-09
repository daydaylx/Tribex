package com.tribex.groovebox.ui.screen

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.tribex.groovebox.engine.AudioEngineBridge

@Composable
fun SoundScreen(
    currentPartIndex: Int = 8,
    isRecording: Boolean = false,
    onRecordingToggle: () -> Unit = {},
    onPartChange: (Int) -> Unit = {}
) {
    // M9: Landscape Row layout - 2 columns
    Row(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        horizontalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        // Left Column - AMP/FILTER/PITCH
        Column(
            modifier = Modifier
                .weight(1f)
                .fillMaxHeight(),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            PartSelector(
                currentIndex = currentPartIndex,
                onPartChange = onPartChange
            )
            
            RecordingToggle(
                isRecording = isRecording,
                onToggle = onRecordingToggle
            )
            
            if (currentPartIndex == 8) {
                // Synth: Left column has Wavetable, Pitch, Amplitude, Filter
                SynthLeftColumn()
            } else {
                // Drum: Left column has Pitch, Level, Decay, Filter
                DrumLeftColumn(partIndex = currentPartIndex)
            }
        }
        
        // Right Column - ADSR/MASTER FX
        Column(
            modifier = Modifier
                .weight(1f)
                .fillMaxHeight(),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            if (currentPartIndex == 8) {
                // Synth: Right column has ADSR envelope + Master FX
                SynthRightColumn()
            } else {
                // Drum: Right column shows Master FX (limited drum parameters)
                Column(
                    modifier = Modifier
                        .fillMaxWidth()
                        .border(
                            border = BorderStroke(1.dp, MaterialTheme.colorScheme.outlineVariant),
                            shape = RoundedCornerShape(8.dp)
                        )
                        .padding(16.dp),
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    Text(
                        text = "DRUM PART",
                        style = MaterialTheme.typography.titleMedium,
                        color = MaterialTheme.colorScheme.secondary
                    )
                    
                    Text(
                        text = "Select a drum part for editing",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }
            
            // Master FX (shown for all parts)
            MasterFXSection()
        }
    }
}

@Composable
fun PartSelector(
    currentIndex: Int,
    onPartChange: (Int) -> Unit
) {
    val parts = listOf("BD", "SD", "CH", "OH", "CP", "LT", "MT", "HT", "SYNTH")
    
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.spacedBy(4.dp)
    ) {
        parts.forEachIndexed { index, name ->
            PartButton(
                name = name,
                isSelected = index == currentIndex,
                onClick = { onPartChange(index) },
                modifier = Modifier.weight(1f)
            )
        }
    }
}

@Composable
fun PartButton(
    name: String,
    isSelected: Boolean,
    onClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    val colors = ButtonDefaults.buttonColors(
        containerColor = if (isSelected) {
            MaterialTheme.colorScheme.primary
        } else {
            MaterialTheme.colorScheme.surfaceVariant
        },
        contentColor = if (isSelected) {
            MaterialTheme.colorScheme.onPrimary
        } else {
            MaterialTheme.colorScheme.onSurfaceVariant
        }
    )
    
    Button(
        onClick = onClick,
        modifier = modifier.height(48.dp),
        colors = colors,
        shape = RoundedCornerShape(8.dp)
    ) {
        Text(
            text = name,
            fontWeight = FontWeight.Bold,
            fontSize = 12.sp
        )
    }
}

@Composable
fun RecordingToggle(
    isRecording: Boolean,
    onToggle: () -> Unit
) {
    val colors = ButtonDefaults.buttonColors(
        containerColor = if (isRecording) {
            Color.Red
        } else {
            MaterialTheme.colorScheme.surfaceVariant
        },
        contentColor = if (isRecording) {
            Color.White
        } else {
            MaterialTheme.colorScheme.onSurfaceVariant
        }
    )
    
    Button(
        onClick = onToggle,
        modifier = Modifier.fillMaxWidth(),
        colors = colors,
        shape = RoundedCornerShape(8.dp)
    ) {
        Text(
            text = if (isRecording) "■ STOP REC" else "● REC",
            fontWeight = FontWeight.Bold
        )
    }
}

/**
 * Synth Left Column - AMP/FILTER/PITCH
 */
@Composable
fun SynthLeftColumn() {
    var wavetableType by remember { mutableStateOf(0) }
    var pitch by remember { mutableStateOf(0f) }
    var amplitude by remember { mutableStateOf(1.0f) }
    var cutoff by remember { mutableStateOf(0.8f) }
    var resonance by remember { mutableStateOf(0f) }
    
    val wavetableNames = listOf("SAW", "SQR", "SIN", "MAJ", "MIN", "7TH")
    
    Column(
        modifier = Modifier.fillMaxWidth(),
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        // WAVETABLE Section
        Text(
            text = "WAVETABLE",
            style = MaterialTheme.typography.labelMedium,
            color = MaterialTheme.colorScheme.secondary
        )
        
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(4.dp)
        ) {
            wavetableNames.forEachIndexed { index, name ->
                PartButton(
                    name = name,
                    isSelected = wavetableType == index,
                    onClick = {
                        wavetableType = index
                        AudioEngineBridge.setSynthWavetable(8, index)
                    },
                    modifier = Modifier.weight(1f)
                )
            }
        }
        
        HorizontalDivider(color = MaterialTheme.colorScheme.outlineVariant)
        
        // AMP Section
        Text(
            text = "AMPLITUDE",
            style = MaterialTheme.typography.labelMedium,
            color = MaterialTheme.colorScheme.secondary
        )
        
        ParameterSlider(
            name = "LEVEL",
            value = amplitude,
            range = 0f..1f,
            onValueChange = {
                amplitude = it
                AudioEngineBridge.setVoiceLevel(8, it)
            },
            unit = ""
        )
        
        HorizontalDivider(color = MaterialTheme.colorScheme.outlineVariant)
        
        // PITCH Section
        Text(
            text = "PITCH",
            style = MaterialTheme.typography.labelMedium,
            color = MaterialTheme.colorScheme.secondary
        )
        
        ParameterSlider(
            name = "PITCH",
            value = pitch,
            range = -24f..24f,
            onValueChange = {
                pitch = it
                AudioEngineBridge.setVoicePitch(8, it)
            },
            unit = "st"
        )
        
        HorizontalDivider(color = MaterialTheme.colorScheme.outlineVariant)
        
        // FILTER Section
        Text(
            text = "FILTER",
            style = MaterialTheme.typography.labelMedium,
            color = MaterialTheme.colorScheme.secondary
        )
        
        ParameterSlider(
            name = "CUTOFF",
            value = cutoff,
            range = 0f..1f,
            onValueChange = {
                cutoff = it
                AudioEngineBridge.setSynthCutoff(8, it)
            },
            unit = ""
        )
        
        ParameterSlider(
            name = "RESONANCE",
            value = resonance,
            range = 0f..1f,
            onValueChange = {
                resonance = it
                AudioEngineBridge.setSynthResonance(8, it)
            },
            unit = ""
        )
    }
}

/**
 * Synth Right Column - ADSR
 */
@Composable
fun SynthRightColumn() {
    var attack by remember { mutableStateOf(10f) }
    var decay by remember { mutableStateOf(200f) }
    var sustain by remember { mutableStateOf(0.7f) }
    var release by remember { mutableStateOf(300f) }
    
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .border(
                border = BorderStroke(1.dp, MaterialTheme.colorScheme.outlineVariant),
                shape = RoundedCornerShape(8.dp)
            )
            .padding(12.dp),
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        Text(
            text = "ADSR ENVELOPE",
            style = MaterialTheme.typography.titleMedium,
            color = MaterialTheme.colorScheme.secondary
        )
        
        ParameterSlider(
            name = "ATTACK",
            value = attack,
            range = 0f..5000f,
            onValueChange = {
                attack = it
                AudioEngineBridge.setSynthAttack(8, it)
            },
            unit = "ms"
        )
        
        ParameterSlider(
            name = "DECAY",
            value = decay,
            range = 0f..5000f,
            onValueChange = {
                decay = it
                AudioEngineBridge.setSynthDecay(8, it)
            },
            unit = "ms"
        )
        
        ParameterSlider(
            name = "SUSTAIN",
            value = sustain,
            range = 0f..1f,
            onValueChange = {
                sustain = it
                AudioEngineBridge.setSynthSustain(8, it)
            },
            unit = ""
        )
        
        ParameterSlider(
            name = "RELEASE",
            value = release,
            range = 0f..5000f,
            onValueChange = {
                release = it
                AudioEngineBridge.setSynthRelease(8, it)
            },
            unit = "ms"
        )
    }
}

/**
 * Drum Left Column
 */
@Composable
fun DrumLeftColumn(partIndex: Int) {
    var pitch by remember { mutableStateOf(0f) }
    var level by remember { mutableStateOf(1.0f) }
    var decay by remember { mutableStateOf(200f) }
    var filterType by remember { mutableStateOf(0) }
    
    Column(
        modifier = Modifier.fillMaxWidth(),
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        // PITCH Section
        Text(
            text = "PITCH",
            style = MaterialTheme.typography.labelMedium,
            color = MaterialTheme.colorScheme.secondary
        )
        
        ParameterSlider(
            name = "PITCH",
            value = pitch,
            range = -24f..24f,
            onValueChange = {
                pitch = it
                AudioEngineBridge.setVoicePitch(partIndex, it)
            },
            unit = "st"
        )
        
        HorizontalDivider(color = MaterialTheme.colorScheme.outlineVariant)
        
        // AMP Section
        Text(
            text = "AMPLITUDE",
            style = MaterialTheme.typography.labelMedium,
            color = MaterialTheme.colorScheme.secondary
        )
        
        ParameterSlider(
            name = "LEVEL",
            value = level,
            range = 0f..1f,
            onValueChange = {
                level = it
                AudioEngineBridge.setVoiceLevel(partIndex, it)
            },
            unit = ""
        )
        
        ParameterSlider(
            name = "DECAY",
            value = decay,
            range = 0f..5000f,
            onValueChange = {
                decay = it
                AudioEngineBridge.setVoiceDecay(partIndex, it)
            },
            unit = "ms"
        )
        
        HorizontalDivider(color = MaterialTheme.colorScheme.outlineVariant)
        
        // FILTER Section
        Text(
            text = "FILTER TYPE",
            style = MaterialTheme.typography.labelMedium,
            color = MaterialTheme.colorScheme.secondary
        )
        
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            FilterButton(
                name = "LP",
                isSelected = filterType == 0,
                onClick = {
                    filterType = 0
                    AudioEngineBridge.setVoiceFilter(partIndex, 0)
                },
                modifier = Modifier.weight(1f)
            )
            
            FilterButton(
                name = "HP",
                isSelected = filterType == 1,
                onClick = {
                    filterType = 1
                    AudioEngineBridge.setVoiceFilter(partIndex, 1)
                },
                modifier = Modifier.weight(1f)
            )
        }
    }
}

@Composable
fun FilterButton(
    name: String,
    isSelected: Boolean,
    onClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    val colors = ButtonDefaults.buttonColors(
        containerColor = if (isSelected) {
            MaterialTheme.colorScheme.primary
        } else {
            MaterialTheme.colorScheme.surfaceVariant
        },
        contentColor = if (isSelected) {
            MaterialTheme.colorScheme.onPrimary
        } else {
            MaterialTheme.colorScheme.onSurfaceVariant
        }
    )
    
    Button(
        onClick = onClick,
        modifier = modifier.height(48.dp),
        colors = colors,
        shape = RoundedCornerShape(8.dp)
    ) {
        Text(
            text = name,
            fontWeight = FontWeight.Bold
        )
    }
}

@Composable
fun ParameterSlider(
    name: String,
    value: Float,
    range: ClosedFloatingPointRange<Float>,
    onValueChange: (Float) -> Unit,
    unit: String
) {
    Column(
        modifier = Modifier.fillMaxWidth(),
        verticalArrangement = Arrangement.spacedBy(4.dp)
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(
                text = name,
                style = MaterialTheme.typography.labelMedium,
                color = MaterialTheme.colorScheme.secondary
            )
            Text(
                text = "${String.format("%.2f", value)}$unit",
                style = MaterialTheme.typography.labelMedium,
                fontWeight = FontWeight.Bold
            )
        }
        
        Slider(
            value = value,
            onValueChange = onValueChange,
            valueRange = range,
            modifier = Modifier.fillMaxWidth()
        )
    }
}

@Composable
fun MasterFXSection() {
    // P1.1: Master FX Parameter State
    var delayTimeMs by remember { mutableStateOf(250f) }
    var delayFeedback by remember { mutableStateOf(0.3f) }
    var delayMix by remember { mutableStateOf(0.2f) }
    var reverbSize by remember { mutableStateOf(0.5f) }
    var reverbDensity by remember { mutableStateOf(0.5f) }
    var reverbMix by remember { mutableStateOf(0.2f) }
    var valveAmount by remember { mutableStateOf(0.0f) }
    var limiterThresholdDb by remember { mutableStateOf(-0.3f) }
    var limiterReleaseMs by remember { mutableStateOf(50f) }
    
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .border(
                border = BorderStroke(1.dp, MaterialTheme.colorScheme.outlineVariant),
                shape = RoundedCornerShape(8.dp)
            )
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Text(
            text = "MASTER FX",
            style = MaterialTheme.typography.titleMedium,
            color = MaterialTheme.colorScheme.secondary
        )
        
        HorizontalDivider(color = MaterialTheme.colorScheme.outlineVariant)
        
        Text(
            text = "DELAY",
            style = MaterialTheme.typography.labelMedium,
            color = MaterialTheme.colorScheme.secondary
        )
        
        ParameterSlider(
            name = "TIME",
            value = delayTimeMs,
            range = 0f..1000f,
            onValueChange = {
                delayTimeMs = it
                AudioEngineBridge.setDelayTimeMs(it)
            },
            unit = "ms"
        )
        
        ParameterSlider(
            name = "FEEDBACK",
            value = delayFeedback,
            range = 0f..0.95f,
            onValueChange = {
                delayFeedback = it
                AudioEngineBridge.setDelayFeedback(it)
            },
            unit = ""
        )
        
        ParameterSlider(
            name = "MIX",
            value = delayMix,
            range = 0f..1f,
            onValueChange = {
                delayMix = it
                AudioEngineBridge.setDelayMix(it)
            },
            unit = ""
        )
        
        HorizontalDivider(color = MaterialTheme.colorScheme.outlineVariant)
        
        Text(
            text = "REVERB",
            style = MaterialTheme.typography.labelMedium,
            color = MaterialTheme.colorScheme.secondary
        )
        
        ParameterSlider(
            name = "SIZE",
            value = reverbSize,
            range = 0f..1f,
            onValueChange = {
                reverbSize = it
                AudioEngineBridge.setReverbSize(it)
            },
            unit = ""
        )
        
        ParameterSlider(
            name = "DENSITY",
            value = reverbDensity,
            range = 0f..1f,
            onValueChange = {
                reverbDensity = it
                AudioEngineBridge.setReverbDensity(it)
            },
            unit = ""
        )
        
        ParameterSlider(
            name = "MIX",
            value = reverbMix,
            range = 0f..1f,
            onValueChange = {
                reverbMix = it
                AudioEngineBridge.setReverbMix(it)
            },
            unit = ""
        )
        
        HorizontalDivider(color = MaterialTheme.colorScheme.outlineVariant)
        
        Text(
            text = "VALVE SATURATION",
            style = MaterialTheme.typography.labelMedium,
            color = MaterialTheme.colorScheme.secondary
        )
        
        ParameterSlider(
            name = "AMOUNT",
            value = valveAmount,
            range = 0f..1f,
            onValueChange = {
                valveAmount = it
                AudioEngineBridge.setValveAmount(it)
            },
            unit = ""
        )
        
        HorizontalDivider(color = MaterialTheme.colorScheme.outlineVariant)
        
        Text(
            text = "LIMITER",
            style = MaterialTheme.typography.labelMedium,
            color = MaterialTheme.colorScheme.secondary
        )
        
        ParameterSlider(
            name = "THRESHOLD",
            value = limiterThresholdDb,
            range = -12f..-0.3f,
            onValueChange = {
                limiterThresholdDb = it
                AudioEngineBridge.setLimiterThresholdDb(it)
            },
            unit = "dB"
        )
        
        ParameterSlider(
            name = "RELEASE",
            value = limiterReleaseMs,
            range = 10f..1000f,
            onValueChange = {
                limiterReleaseMs = it
                AudioEngineBridge.setLimiterReleaseMs(it)
            },
            unit = "ms"
        )
    }
}

// Legacy functions kept for compatibility
@Composable
fun SynthParameterSection() {
    // This function is now split into SynthLeftColumn and SynthRightColumn
    // Kept for any potential external references
    Column {
        SynthLeftColumn()
        Spacer(modifier = Modifier.height(16.dp))
        SynthRightColumn()
    }
}

@Composable
fun DrumParameterSection(partIndex: Int) {
    // This function is now split into DrumLeftColumn
    // Kept for any potential external references
    DrumLeftColumn(partIndex = partIndex)
}
