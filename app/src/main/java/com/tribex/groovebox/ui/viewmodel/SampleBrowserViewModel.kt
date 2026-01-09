package com.tribex.groovebox.ui.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.tribex.groovebox.audio.SampleAssetLoader
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class SampleBrowserViewModel(
    private val sampleLoader: SampleAssetLoader
) : ViewModel() {

    private val _availableSamples = MutableStateFlow<List<String>>(emptyList())
    val availableSamples: StateFlow<List<String>> = _availableSamples.asStateFlow()

    private val _loadingStates = MutableStateFlow<Map<String, Boolean>>(emptyMap())
    val loadingStates: StateFlow<Map<String, Boolean>> = _loadingStates.asStateFlow()

    private val _selectedPart = MutableStateFlow(0)
    val selectedPart: StateFlow<Int> = _selectedPart.asStateFlow()

    private val _errorMessage = MutableStateFlow<String?>(null)
    val errorMessage: StateFlow<String?> = _errorMessage.asStateFlow()

    init {
        refreshSamples()
    }

    fun refreshSamples() {
        viewModelScope.launch {
            withContext(Dispatchers.IO) {
                sampleLoader.listAvailableSamples()
                    .onSuccess { samples ->
                        _availableSamples.value = samples
                    }
                    .onFailure { error ->
                        _errorMessage.value = "Failed to load samples: ${error.message}"
                    }
            }
        }
    }

    fun selectPart(partIndex: Int) {
        if (partIndex in 0..7) {
            _selectedPart.value = partIndex
        }
    }

    fun loadSampleToPart(filename: String, partIndex: Int) {
        viewModelScope.launch {
            // Set loading state
            _loadingStates.value = _loadingStates.value + (filename to true)

            withContext(Dispatchers.IO) {
                sampleLoader.loadSampleToPart(partIndex, filename)
                    .onSuccess {
                        _errorMessage.value = "✅ Loaded $filename to Part ${partIndex + 1}"
                    }
                    .onFailure { error ->
                        _errorMessage.value = "❌ Failed to load $filename: ${error.message}"
                    }
            }

            // Clear loading state
            _loadingStates.value = _loadingStates.value - filename
        }
    }

    fun clearError() {
        _errorMessage.value = null
    }

    class Factory(
        private val sampleLoader: SampleAssetLoader
    ) : ViewModelProvider.Factory {
        @Suppress("UNCHECKED_CAST")
        override fun <T : ViewModel> create(modelClass: Class<T>): T {
            if (modelClass.isAssignableFrom(SampleBrowserViewModel::class.java)) {
                return SampleBrowserViewModel(sampleLoader) as T
            }
            throw IllegalArgumentException("Unknown ViewModel class")
        }
    }
}
