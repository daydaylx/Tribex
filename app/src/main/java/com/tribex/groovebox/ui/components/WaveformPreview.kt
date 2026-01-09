package com.tribex.groovebox.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp
import com.tribex.groovebox.engine.WaveformData

/**
 * Waveform Preview Component
 * 
 * Optimized rendering using Path instead of multiple drawLine calls.
 * Displays a downsampled waveform visualization with trim range indicators.
 * The waveform is rendered as a symmetric vertical bar graph.
 * 
 * @param waveform Downsampled waveform data
 * @param trimStartPercent Start trim position (0.0 to 1.0)
 * @param trimEndPercent End trim position (0.0 to 1.0)
 * @param modifier Modifier for the component
 */
@Composable
fun WaveformPreview(
    waveform: WaveformData?,
    trimStartPercent: Float = 0f,
    trimEndPercent: Float = 1f,
    modifier: Modifier = Modifier
) {
    Canvas(
        modifier = modifier
            .fillMaxWidth()
            .height(80.dp)
    ) {
        val canvasWidth = size.width
        val canvasHeight = size.height
        val centerY = canvasHeight / 2f
        
        if (waveform == null || waveform.points.isEmpty()) {
            // Draw placeholder line if no waveform
            drawLine(
                color = Color.Gray.copy(alpha = 0.3f),
                start = Offset(0f, centerY),
                end = Offset(canvasWidth, centerY),
                strokeWidth = 2f
            )
            return@Canvas
        }
        
        val points = waveform.points
        val pointWidth = canvasWidth / points.size
        
        // Use Paths for batch rendering
        val activePath = Path()
        val inactivePath = Path()
        
        for (i in points.indices) {
            val amplitude = points[i]
            val x = i * pointWidth + pointWidth / 2f
            
            // Only draw if within trim range (dimmed outside)
            val percent = i.toFloat() / points.size
            val inTrimRange = percent >= trimStartPercent && percent <= trimEndPercent
            
            val barHeight = amplitude * centerY * 0.9f // Scale to 90% of half height
            
            val targetPath = if (inTrimRange) activePath else inactivePath
            
            // Add vertical bar to path
            targetPath.moveTo(x, centerY - barHeight)
            targetPath.lineTo(x, centerY + barHeight)
        }
        
        // Draw inactive parts (dimmed)
        drawPath(
            path = inactivePath,
            color = Color.Gray.copy(alpha = 0.3f),
            style = Stroke(width = pointWidth.coerceAtLeast(1f))
        )
        
        // Draw active parts (highlighted)
        drawPath(
            path = activePath,
            color = Color(0xFF2196F3),
            style = Stroke(width = pointWidth.coerceAtLeast(1f))
        )
        
        // Draw trim start line
        val startX = trimStartPercent * canvasWidth
        drawLine(
            color = Color.Red,
            start = Offset(startX, 0f),
            end = Offset(startX, canvasHeight),
            strokeWidth = 2f
        )
        
        // Draw trim end line
        val endX = trimEndPercent * canvasWidth
        drawLine(
            color = Color.Red,
            start = Offset(endX, 0f),
            end = Offset(endX, canvasHeight),
            strokeWidth = 2f
        )
    }
}