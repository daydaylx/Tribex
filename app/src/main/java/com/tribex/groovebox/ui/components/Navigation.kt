package com.tribex.groovebox.ui.components

import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.Menu
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.vector.ImageVector
import com.tribex.groovebox.ui.screen.Screen

/**
 * Navigation Bar
 * 
 * Bottom navigation bar with 3 screens (PATTERN / SOUND / SAMPLE)
 * Per SPEC v3.1: Exactly 3 screens, no sub-menus
 */
@Composable
fun NavigationBar(
    currentScreen: Screen,
    onScreenChange: (Screen) -> Unit,
    modifier: androidx.compose.ui.Modifier = androidx.compose.ui.Modifier
) {
    NavigationBar(
        containerColor = MaterialTheme.colorScheme.surface,
        modifier = modifier
    ) {
        Screen.values().forEach { screen ->
            NavigationBarItem(
                selected = currentScreen == screen,
                onClick = { onScreenChange(screen) },
                icon = {
                    Icon(
                        imageVector = getScreenIcon(screen),
                        contentDescription = screen.title
                    )
                },
                label = { Text(screen.title) }
            )
        }
    }
}

/**
 * Get icon for screen
 */
private fun getScreenIcon(screen: Screen): ImageVector {
    return when (screen) {
        Screen.PATTERN -> Icons.Default.Menu
        Screen.SOUND -> Icons.Default.Settings
        Screen.SAMPLE -> Icons.Default.Info
    }
}