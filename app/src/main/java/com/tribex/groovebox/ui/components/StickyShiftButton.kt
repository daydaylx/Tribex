package com.tribex.groovebox.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

/**
 * Sticky Shift Button
 * 
 * Toggles shift state with 3000ms timeout
 * When active, button turns red and enables shifted actions
 */
@Composable
fun StickyShiftButton(
    isShiftActive: Boolean,
    onShiftToggle: (Boolean) -> Unit,
    modifier: Modifier = Modifier
) {
    val coroutineScope = rememberCoroutineScope()
    
    val buttonColor = if (isShiftActive) {
        Color.Red
    } else {
        MaterialTheme.colorScheme.primary
    }
    
    val buttonTextColor = if (isShiftActive) {
        Color.White
    } else {
        MaterialTheme.colorScheme.onPrimary
    }
    
    Button(
        onClick = {
            onShiftToggle(!isShiftActive)
            
            // Auto-deactivate after 3000ms if still active
            if (isShiftActive) {
                // Shift is being toggled off, no timeout needed
            } else {
                // Shift is being toggled on, set timeout
                coroutineScope.launch {
                    delay(3000)  // 3000ms timeout
                    onShiftToggle(false)
                }
            }
        },
        modifier = modifier
            .height(60.dp)
            .fillMaxWidth(),
        shape = RoundedCornerShape(8.dp),
        colors = ButtonDefaults.buttonColors(
            containerColor = buttonColor,
            contentColor = buttonTextColor
        )
    ) {
        Text(
            text = if (isShiftActive) "SHIFT ACTIVE" else "SHIFT",
            style = MaterialTheme.typography.titleMedium
        )
    }
}