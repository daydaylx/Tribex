package com.tribex.groovebox.ui.screen

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.material3.ExperimentalMaterial3Api
import com.tribex.groovebox.engine.AudioEngineBridge
import com.tribex.groovebox.engine.Velocity
import com.tribex.groovebox.ui.components.FPSCounter
import com.tribex.groovebox.ui.components.StepGrid
import com.tribex.groovebox.ui.components.StickyShiftButton
import kotlinx.coroutines.delay

/**
 * PATTERN Screen
 * 
 * 16-step grid with paging (16-64)
 * Running light at 60fps
 * Step edit with tap/swipe gestures
 */
@Composable
fun PatternScreen() {
    var patternState by remember { mutableStateOf(PatternState.createEmpty()) }
    var selectedPartIndexInt by remember { mutableStateOf(0) }
    val selectedPartIndex: UInt = selectedPartIndexInt.toUInt()
    var isShiftActive by remember { mutableStateOf(false) }
    var isPlaying by remember { mutableStateOf(false) }
    
    // Poll state from audio engine (~60fps)
    LaunchedEffect(Unit) {
        while (true) {
            delay(16)  // ~60fps
            
            // Poll audio engine state
            isPlaying = AudioEngineBridge.isPlaying()
            
            // Update current step from sequencer
            // For M3, we simulate running light with local state
            // M4+ will poll actual sequencer state via JNI
            val currentStep = (System.currentTimeMillis() / 250).toUInt() % 16u
            patternState = patternState.copy(currentStep = currentStep, isPlaying = isPlaying)
        }
    }
    
    // Part info for selected part
    val selectedPart = PARTS.getOrNull(selectedPartIndex.toInt())
    
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(8.dp)
    ) {
        // Header with FPS Counter
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(8.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = "PATTERN",
                style = MaterialTheme.typography.headlineSmall
            )
            
            FPSCounter()
        }
        
        Divider()
        
        // Part Selection Tabs
        LazyRow(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            items(PARTS.size) { index ->
                PartTab(
                    part = PARTS[index],
                    isSelected = index.toUInt() == selectedPartIndex,
                    onClick = { selectedPartIndexInt = index }
                )
            }
        }
        
        Spacer(modifier = Modifier.height(8.dp))
        
        // Page Controls
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            OutlinedButton(
                onClick = {
                    // Previous page
                    if (patternState.currentPage > 0u) {
                        patternState = patternState.copy(currentPage = patternState.currentPage - 1u)
                    }
                },
                enabled = patternState.currentPage > 0u
            ) {
                Text("<<")
            }
            
            Text(
                text = "Page ${(patternState.currentPage + 1u)} / 4",
                style = MaterialTheme.typography.titleMedium
            )
            
            OutlinedButton(
                onClick = {
                    // Next page
                    if (patternState.currentPage < 3u) {
                        patternState = patternState.copy(currentPage = patternState.currentPage + 1u)
                    }
                },
                enabled = patternState.currentPage < 3u
            ) {
                Text(">>")
            }
        }
        
        Spacer(modifier = Modifier.height(8.dp))
        
        // BPM Display
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.Center,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = "BPM: ${patternState.bpm.toInt()}",
                style = MaterialTheme.typography.titleLarge
            )
        }
        
        Spacer(modifier = Modifier.height(8.dp))
        
        // Step Grid
        Box(
            modifier = Modifier
                .weight(1f)
                .fillMaxWidth()
        ) {
            StepGrid(
                state = patternState,
                selectedPartIndex = selectedPartIndex,
                onPageChange = { /* Handled by page controls */ },
                onStepTap = { stepIndex ->
                    // Toggle gate
                    val partSteps = patternState.steps.getOrNull(selectedPartIndex.toInt())
                    if (partSteps != null) {
                        val newSteps = partSteps.toMutableList()
                        newSteps[stepIndex] = newSteps[stepIndex].copy(gate = !newSteps[stepIndex].gate)
                        val allParts = patternState.steps.toMutableList()
                        allParts[selectedPartIndex.toInt()] = newSteps
                        patternState = patternState.copy(steps = allParts)
                    }
                },
                onSwipeUp = { stepIndex ->
                    // Set velocity to MAX (accent)
                    val partSteps = patternState.steps.getOrNull(selectedPartIndex.toInt())
                    if (partSteps != null) {
                        val newSteps = partSteps.toMutableList()
                        newSteps[stepIndex] = newSteps[stepIndex].copy(velocity = Velocity.MAX)
                        val allParts = patternState.steps.toMutableList()
                        allParts[selectedPartIndex.toInt()] = newSteps
                        patternState = patternState.copy(steps = allParts)
                    }
                },
                onSwipeDown = { stepIndex ->
                    // Set velocity to GHOST
                    val partSteps = patternState.steps.getOrNull(selectedPartIndex.toInt())
                    if (partSteps != null) {
                        val newSteps = partSteps.toMutableList()
                        newSteps[stepIndex] = newSteps[stepIndex].copy(velocity = Velocity.GHOST)
                        val allParts = patternState.steps.toMutableList()
                        allParts[selectedPartIndex.toInt()] = newSteps
                        patternState = patternState.copy(steps = allParts)
                    }
                }
            )
        }
        
        Spacer(modifier = Modifier.height(8.dp))
        
        // Sticky Shift Button
        StickyShiftButton(
            isShiftActive = isShiftActive,
            onShiftToggle = { isActive -> isShiftActive = isActive }
        )
        
        Spacer(modifier = Modifier.height(8.dp))
        
        // Performance Controls (Mute/Solo - UI only for M3)
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            OutlinedButton(
                onClick = { /* Mute functionality - M4 */ },
                modifier = Modifier.weight(1f)
            ) {
                Text("MUTE")
            }
            
            OutlinedButton(
                onClick = { /* Solo functionality - M4 */ },
                modifier = Modifier.weight(1f)
            ) {
                Text("SOLO")
            }
        }
        
        // Sequencer Controls
        Spacer(modifier = Modifier.height(8.dp))
        
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Button(
                onClick = {
                    if (isPlaying) {
                        AudioEngineBridge.stopSequencer()
                    } else {
                        AudioEngineBridge.startSequencer()
                    }
                },
                modifier = Modifier.weight(1f)
            ) {
                Text(if (isPlaying) "STOP SEQUENCER" else "START SEQUENCER")
            }
        }
    }
}

/**
 * Part Tab
 * 
 * Part selection button (one of 9 parts)
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun PartTab(
    part: PartInfo,
    isSelected: Boolean,
    onClick: () -> Unit
) {
    val backgroundColor = if (isSelected) {
        MaterialTheme.colorScheme.primary
    } else {
        MaterialTheme.colorScheme.surfaceVariant
    }
    
    val textColor = if (isSelected) {
        MaterialTheme.colorScheme.onPrimary
    } else {
        MaterialTheme.colorScheme.onSurfaceVariant
    }
    
    FilterChip(
        selected = isSelected,
        onClick = onClick,
        label = { Text(part.name) },
        colors = FilterChipDefaults.filterChipColors(
            selectedContainerColor = backgroundColor,
            selectedLabelColor = textColor,
            containerColor = backgroundColor,
            labelColor = textColor
        )
    )
}

/**
 * Toggle step gate
 */
private fun toggleStepGate(partIndex: UInt, stepIndex: Int) {
    // This will be called from PatternScreen's patternState
    // For now, gate toggling is handled inline in the Composable
}

/**
 * Set step velocity
 */
private fun setStepVelocity(partIndex: UInt, stepIndex: Int, velocity: UByte) {
    // This will be called from PatternScreen's patternState
    // For now, velocity setting is handled inline in the Composable
}