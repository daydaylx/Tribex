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
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        PartSelector(
            currentIndex = currentPartIndex,
            onPartChange = onPartChange
        )
        
        Spacer(modifier = Modifier.height(16.dp))
        
        RecordingToggle(
            isRecording = isRecording,
            onToggle = onRecordingToggle
        )
        
        Spacer(modifier = Modifier.height(16.dp))
        
        if (currentPartIndex == 8) {
            SynthParameterSection()
        } else {
            DrumParameterSection(partIndex = currentPartIndex)
        }
        
        Spacer(modifier = Modifier.height(16.dp))
        
        MasterFXSection()
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

@Composable
fun SynthParameterSection() {
    var wavetableType by remember { mutableStateOf(0) }
    var pitch by remember { mutableStateOf(0f) }
    var cutoff by remember { mutableStateOf(0.8f) }
    var resonance by remember { mutableStateOf(0f) }
    var attack by remember { mutableStateOf(10f) }
    var decay by remember { mutableStateOf(200f) }
    var sustain by remember { mutableStateOf(0.7f) }
    var release by remember { mutableStateOf(300f) }
    var amplitude by remember { mutableStateOf(1.0f) }
    
    val wavetableNames = listOf("SAW", "SQUARE", "SINE", "MAJ", "MIN", "7TH")
    
    Column(
        modifier = Modifier.fillMaxWidth(),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
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
        
        ParameterSlider(
            name = "AMPLITUDE",
            value = amplitude,
            range = 0f..1f,
            onValueChange = {
                amplitude = it
                AudioEngineBridge.setVoiceLevel(8, it)
            },
            unit = ""
        )
        
        ParameterSlider(
            name = "FILTER CUTOFF",
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
        
        Divider(color = MaterialTheme.colorScheme.outlineVariant)
        
        Text(
            text = "ADSR ENVELOPE",
            style = MaterialTheme.typography.labelMedium,
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

@Composable
fun DrumParameterSection(partIndex: Int) {
    var pitch by remember { mutableStateOf(0f) }
    var level by remember { mutableStateOf(1.0f) }
    var decay by remember { mutableStateOf(200f) }
    var filterType by remember { mutableStateOf(0) }
    
    Column(
        modifier = Modifier.fillMaxWidth(),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
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
        modifier = Modifier.fillMaxWidth()
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
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .border(
                border = BorderStroke(1.dp, MaterialTheme.colorScheme.outlineVariant),
                shape = RoundedCornerShape(8.dp)
            )
            .padding(16.dp)
    ) {
        Text(
            text = "MASTER FX",
            style = MaterialTheme.typography.titleMedium,
            color = MaterialTheme.colorScheme.secondary
        )
        
        Spacer(modifier = Modifier.height(8.dp))
        
        Text(
            text = "Valve Saturation, Limiter, Delay, Reverb",
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )
        
        Text(
            text = "(M5: Placeholder - will be implemented in future milestone)",
            style = MaterialTheme.typography.bodySmall,
            color = MaterialTheme.colorScheme.outline
        )
    }
}