package com.tribex.groovebox.ui.components

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.RectF
import android.view.View
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import com.tribex.groovebox.engine.Velocity
import com.tribex.groovebox.ui.screen.PatternState

/**
 * Step Grid Composable
 * 
 * Renders 16-step grid using low-level Canvas for 60fps performance
 * Avoids Compose recomposition per step - critical for running light
 */
@Composable
fun StepGrid(
    state: PatternState,
    selectedPartIndex: UInt,
    onStepTap: (Int) -> Unit,
    onSwipeUp: (Int) -> Unit,
    onSwipeDown: (Int) -> Unit,
    modifier: Modifier = Modifier
) {
    AndroidView(
        factory = { ctx ->
            StepGridView(ctx, state, selectedPartIndex, onStepTap, onSwipeUp, onSwipeDown)
        },
        update = { view ->
            view.updateState(state, selectedPartIndex, onStepTap, onSwipeUp, onSwipeDown)
        },
        modifier = modifier
            .fillMaxSize()
            .background(Color.Black)
    )
}

/**
 * Custom View for Step Grid Rendering
 * 
 * Uses low-level Canvas for 60fps performance
 * Polls state every 16ms (~60fps)
 */
private class StepGridView(
    context: Context,
    private var state: PatternState,
    private var selectedPartIndex: UInt,
    private var onStepTap: (Int) -> Unit,
    private var onSwipeUp: (Int) -> Unit,
    private var onSwipeDown: (Int) -> Unit
) : View(context) {
    
    private val gridPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val activePaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val currentPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val textPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    
    private var lastTapX = 0f
    private var lastTapY = 0f
    private var lastTapTime = 0L
    
    init {
        // Setup paints
        gridPaint.color = Color.Gray.toArgb()
        gridPaint.style = Paint.Style.STROKE
        gridPaint.strokeWidth = 2f
        
        activePaint.color = Color.Cyan.toArgb()
        activePaint.style = Paint.Style.FILL
        
        currentPaint.color = Color.Yellow.toArgb()
        currentPaint.style = Paint.Style.FILL
        
        textPaint.color = Color.White.toArgb()
        textPaint.textSize = 40f
        textPaint.textAlign = Paint.Align.CENTER
        
        // Start polling loop for state updates (~60fps)
        postPollingLoop()
    }
    
    fun updateState(
        newState: PatternState,
        newSelectedPartIndex: UInt,
        newOnStepTap: (Int) -> Unit,
        newOnSwipeUp: (Int) -> Unit,
        newOnSwipeDown: (Int) -> Unit
    ) {
        state = newState
        selectedPartIndex = newSelectedPartIndex
        onStepTap = newOnStepTap
        onSwipeUp = newOnSwipeUp
        onSwipeDown = newOnSwipeDown
        // Invalidate to trigger redraw
        invalidate()
    }
    
    private fun postPollingLoop() {
        postDelayed({
            // Check if we need to poll state from audio engine
            // For now, we use Compose state updates
            invalidate()  // Trigger redraw
            postPollingLoop()  // Continue polling
        }, 16)  // ~60fps
    }
    
    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        
        val width = width.toFloat()
        val height = height.toFloat()
        
        // Calculate cell size (4x4 grid)
        val padding = 8f
        val availableWidth = width - (padding * 2)
        val availableHeight = height - (padding * 2)
        val cellWidth = availableWidth / 4f
        val cellHeight = availableHeight / 4f
        
        // Draw grid background
        canvas.drawRect(padding, padding, width - padding, height - padding, gridPaint)
        
        // Draw 16 steps
        val partSteps = state.steps.getOrNull(selectedPartIndex.toInt()) ?: emptyList()
        
        for (row in 0..3) {
            for (col in 0..3) {
                val stepIndex = row * 4 + col
                val stepState = partSteps.getOrNull(stepIndex) ?: continue
                
                val left = padding + col * cellWidth + 4f
                val top = padding + row * cellHeight + 4f
                val right = left + cellWidth - 8f
                val bottom = top + cellHeight - 8f
                
                val rect = RectF(left, top, right, bottom)
                
                // Determine step color based on state
                val isCurrentStep = (stepIndex.toUInt() == state.currentStep)
                val isActiveStep = stepState.gate
                
                if (isCurrentStep) {
                    // Currently playing step (running light)
                    currentPaint.color = when {
                        isActiveStep -> Color.Yellow.toArgb()
                        else -> Color.DarkGray.toArgb()
                    }
                    canvas.drawRect(rect, currentPaint)
                    
                    // Draw step number
                    canvas.drawText((stepIndex + 1).toString(), left + cellWidth / 2, top + cellHeight / 2, textPaint)
                } else if (isActiveStep) {
                    // Active gate (non-current)
                    val velocityColor = when (stepState.velocity) {
                        Velocity.GHOST -> Color.DarkGray.toArgb()
                        Velocity.NORMAL -> Color.Cyan.toArgb()
                        Velocity.ACCENT -> Color.Green.toArgb()
                        Velocity.MAX -> Color.Red.toArgb()
                        else -> Color.Cyan.toArgb()
                    }
                    activePaint.color = velocityColor
                    canvas.drawRect(rect, activePaint)
                    
                    // Draw step number
                    canvas.drawText((stepIndex + 1).toString(), left + cellWidth / 2, top + cellHeight / 2, textPaint)
                } else {
                    // Inactive gate
                    canvas.drawRect(rect, gridPaint)
                    
                    // Draw step number dimmed
                    textPaint.color = Color.DarkGray.toArgb()
                    canvas.drawText((stepIndex + 1).toString(), left + cellWidth / 2, top + cellHeight / 2, textPaint)
                    textPaint.color = Color.White.toArgb()
                }
            }
        }
    }
    
    override fun onTouchEvent(event: android.view.MotionEvent): Boolean {
        when (event.action) {
            android.view.MotionEvent.ACTION_DOWN -> {
                lastTapX = event.x
                lastTapY = event.y
                lastTapTime = System.currentTimeMillis()
                return true
            }
            
            android.view.MotionEvent.ACTION_UP -> {
                val deltaX = event.x - lastTapX
                val deltaY = event.y - lastTapY
                val elapsed = System.currentTimeMillis() - lastTapTime
                
                // Detect swipe (short time + significant vertical movement)
                if (elapsed < 200) {
                    val stepIndex = getStepIndex(event.x, event.y)
                    if (stepIndex != -1) {
                        if (Math.abs(deltaY) > Math.abs(deltaX) && Math.abs(deltaY) > 50f) {
                            // Vertical swipe detected
                            if (deltaY > 0) {
                                onSwipeDown(stepIndex)
                            } else {
                                onSwipeUp(stepIndex)
                            }
                        } else {
                            // Tap detected
                            onStepTap(stepIndex)
                        }
                    }
                }
                return true
            }
        }
        return super.onTouchEvent(event)
    }
    
    private fun getStepIndex(x: Float, y: Float): Int {
        val width = width.toFloat()
        val height = height.toFloat()
        val padding = 8f
        
        if (x < padding || x > width - padding || y < padding || y > height - padding) {
            return -1
        }
        
        val availableWidth = width - (padding * 2)
        val availableHeight = height - (padding * 2)
        val cellWidth = availableWidth / 4f
        val cellHeight = availableHeight / 4f
        
        val col = ((x - padding) / cellWidth).toInt()
        val row = ((y - padding) / cellHeight).toInt()
        
        if (col < 0 || col > 3 || row < 0 || row > 3) {
            return -1
        }
        
        return row * 4 + col
    }
}
