package com.tribex.groovebox.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
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
            val path = Path()
            path.moveTo(0f, centerY)
            path.lineTo(canvasWidth, centerY)
            drawPath(
                path = path,
                color = Color.Gray.copy(alpha = 0.3f),
                style = Stroke(width = 2f)
            )
            return@Canvas
        }
        
        val points = waveform.points
        val pointWidth = canvasWidth / points.size
        
        // Draw waveform bars
        for (i in points.indices) {
            val amplitude = points[i]
            val x = i * pointWidth + pointWidth / 2f
            
            // Only draw if within trim range (dimmed outside)
            val percent = i.toFloat() / points.size
            val inTrimRange = percent >= trimStartPercent && percent <= trimEndPercent
            
            val barColor = if (inTrimRange) {
                Color(0xFF2196F3) // Blue for active range
            } else {
                Color.Gray.copy(alpha = 0.3f) // Dimmed for trim range
            }
            
            // Draw symmetric vertical bar (up and down from center)
            val barHeight = amplitude * centerY * 0.9f // Scale to 90% of half height
            
            // Top half
            drawLine(
                color = barColor,
                start = Offset(x, centerY),
                end = Offset(x, centerY - barHeight),
                strokeWidth = pointWidth.coerceAtLeast(1f)
            )
            
            // Bottom half
            drawLine(
                color = barColor,
                start = Offset(x, centerY),
                end = Offset(x, centerY + barHeight),
                strokeWidth = pointWidth.coerceAtLeast(1f)
            )
        }
        
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

/**
 * Trim Slider Component
 * 
 * Dual-range slider for setting sample trim points.
 * 
 * @param trimStartPercent Start trim position (0.0 to 1.0)
 * @param trimEndPercent End trim position (0.0 to 1.0)
 * @param onTrimRangeChanged Callback when trim range changes
 * @param modifier Modifier for the component
 */
@Composable
fun TrimSlider(
    trimStartPercent: Float,
    trimEndPercent: Float,
    onTrimRangeChanged: (Float, Float) -> Unit,
    modifier: Modifier = Modifier
) {
    Canvas(
        modifier = modifier.fillMaxWidth()
    ) {
        val canvasWidth = size.width
        val canvasHeight = size.height
        val centerY = canvasHeight / 2f
        
        // Draw background track
        drawRect(
            color = Color.Gray.copy(alpha = 0.2f),
            size = size
        )
        
        // Draw active trim range
        val startX = trimStartPercent * canvasWidth
        val endX = trimEndPercent * canvasWidth
        val rangeWidth = (endX - startX).coerceAtLeast(0f)
        
        drawRect(
            color = Color(0xFF2196F3).copy(alpha = 0.5f),
            topLeft = Offset(startX, 0f),
            size = androidx.compose.ui.geometry.Size(rangeWidth, canvasHeight)
        )
        
        // Draw center line
        drawLine(
            color = Color.Gray.copy(alpha = 0.5f),
            start = Offset(canvasWidth / 2f, 0f),
            end = Offset(canvasWidth / 2f, canvasHeight),
            strokeWidth = 1f
        )
        
        // Note: Actual touch handling would be done via Box with pointer events
        // This is a visual-only component for M4.5
        // Full touch implementation can be added in future milestone
    }
}