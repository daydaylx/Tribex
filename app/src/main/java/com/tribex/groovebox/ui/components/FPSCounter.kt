package com.tribex.groovebox.ui.components

import android.view.Choreographer
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.delay

/**
 * FPS Counter
 * 
 * Displays real-time frame rate using Choreographer callbacks
 * Goal: Stable >55fps for smooth running light
 */
@Composable
fun FPSCounter(modifier: Modifier = Modifier) {
    var frameCount by remember { mutableIntStateOf(0) }
    var lastTime by remember { mutableLongStateOf(System.currentTimeMillis()) }
    var fps by remember { mutableFloatStateOf(0f) }
    
    // Update FPS every second
    LaunchedEffect(Unit) {
        while (true) {
            delay(1000)  // Update every second
            val now = System.currentTimeMillis()
            val elapsed = (now - lastTime).toFloat()
            fps = if (elapsed > 0f) frameCount * 1000f / elapsed else 0f
            frameCount = 0
            lastTime = now
        }
    }
    
    // Track frames using Choreographer
    DisposableEffect(Unit) {
        val choreographer = Choreographer.getInstance()
        
        val frameCallback = object : Choreographer.FrameCallback {
            override fun doFrame(frameTimeNanos: Long) {
                frameCount++
                choreographer.postFrameCallback(this)
            }
        }
        
        choreographer.postFrameCallback(frameCallback)
        
        onDispose {
            choreographer.removeFrameCallback(frameCallback)
        }
    }
    
    // Color based on performance
    val fpsColor = when {
        fps >= 55f -> Color.Green
        fps >= 45f -> Color.Yellow
        else -> Color.Red
    }
    
    Text(
        text = "FPS: ${fps.toInt()}",
        modifier = modifier,
        color = fpsColor,
        style = MaterialTheme.typography.bodySmall
    )
}