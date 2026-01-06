package com.tribex.groovebox.ui.screen

/**
 * Screen Enumeration
 * 
 * Defines the 3 screens of TribeX (PATTERN / SOUND / SAMPLE)
 * No sub-menus allowed per SPEC v3.1
 */
enum class Screen(val title: String) {
    PATTERN("PATTERN"),
    SOUND("SOUND"),
    SAMPLE("SAMPLE")
}