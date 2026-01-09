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
 * Performance optimizations:
 * - Uses Path batching for efficient rendering
 * - Caches waveform paths to avoid recomputation
 * - Minimizes calculations in render loop
 * - Uses pre-calculated trim indices
 * 
 * @param waveform Downsampled waveform data (max 1000 points)
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
    // Cache waveform paths to avoid recomputation
    val cachedPaths = remember(waveform, trimStartPercent, trimEndPercent) {
        if (waveform == null || waveform.points.isEmpty()) {
            null
        } else {
            val points = waveform.points
            val activePath = Path()
            val inactivePath = Path()
            
            // Pre-calculate trim indices to avoid division in loop
            val startIdx = (trimStartPercent * points.size).toInt().coerceIn(0, points.size)
            val endIdx = (trimEndPercent * points.size).toInt().coerceIn(startIdx, points.size)
            
            for (i in points.indices) {
                val amplitude = points[i]
                val inTrimRange = i >= startIdx && i <= endIdx
                
                val targetPath = if (inTrimRange) activePath else inactivePath
                
                // Add vertical bar to path (normalized coordinates 0-1)
                val xNorm = i.toFloat() / points.size
                val barHeightNorm = amplitude * 0.9f // Scale to 90% of half height
                
                // Store normalized coordinates to avoid recalculating during render
                targetPath.moveTo(xNorm, -barHeightNorm)
                targetPath.lineTo(xNorm, barHeightNorm)
            }
            
            CachedWaveformPaths(activePath, inactivePath, startIdx, endIdx)
        }
    }
    
    Canvas(
        modifier = modifier
            .fillMaxWidth()
            .height(80.dp)
    ) {
        val canvasWidth = size.width
        val canvasHeight = size.height
        val centerY = canvasHeight / 2f
        val pointWidth = if (waveform != null && waveform.points.isNotEmpty()) {
            canvasWidth / waveform.points.size
        } else {
            1f
        }
        
        if (cachedPaths == null) {
            // Draw placeholder line if no waveform
            drawLine(
                color = Color.Gray.copy(alpha = 0.3f),
                start = Offset(0f, centerY),
                end = Offset(canvasWidth, centerY),
                strokeWidth = 2f
            )
            return@Canvas
        }
        
        // Draw inactive parts (dimmed) - use cached paths directly
        drawPath(
            path = cachedPaths.inactivePath,
            color = Color.Gray.copy(alpha = 0.3f),
            style = Stroke(width = pointWidth.coerceAtLeast(1f))
        )
        
        // Draw active parts (highlighted) - use cached paths directly
        drawPath(
            path = cachedPaths.activePath,
            color = Color(0xFF2196F3),
            style = Stroke(width = pointWidth.coerceAtLeast(1f))
        )
        
        // Draw trim lines using cached indices
        val pointsSize = waveform?.points?.size ?: 1
        val startX = cachedPaths.startIdx.toFloat() / pointsSize * canvasWidth
        val endX = cachedPaths.endIdx.toFloat() / pointsSize * canvasWidth
        
        drawLine(
            color = Color.Red,
            start = Offset(startX, 0f),
            end = Offset(startX, canvasHeight),
            strokeWidth = 2f
        )
        
        drawLine(
            color = Color.Red,
            start = Offset(endX, 0f),
            end = Offset(endX, canvasHeight),
            strokeWidth = 2f
        )
    }
}

/**
 * Cached waveform paths for efficient rendering
 */
private data class CachedWaveformPaths(
    val activePath: Path,
    val inactivePath: Path,
    val startIdx: Int,
    val endIdx: Int
)