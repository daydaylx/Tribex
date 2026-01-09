package com.tribex.groovebox.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.tribex.groovebox.audio.SampleAssetLoader
import com.tribex.groovebox.ui.viewmodel.SampleBrowserViewModel
import kotlinx.coroutines.launch

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SampleBrowserScreen(
    sampleLoader: SampleAssetLoader,
    viewModel: SampleBrowserViewModel = viewModel(
        factory = SampleBrowserViewModel.Factory(sampleLoader)
    )
) {
    val samples by viewModel.availableSamples.collectAsState()
    val loadingStates by viewModel.loadingStates.collectAsState()
    val errorMessage by viewModel.errorMessage.collectAsState()
    val selectedPart by viewModel.selectedPart.collectAsState()
    val snackbarHostState = remember { SnackbarHostState() }
    val scope = rememberCoroutineScope()

    // Show error snackbar
    LaunchedEffect(errorMessage) {
        errorMessage?.let { message ->
            scope.launch {
                snackbarHostState.showSnackbar(
                    message = message,
                    duration = SnackbarDuration.Short
                )
                viewModel.clearError()
            }
        }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Sample Browser") },
                actions = {
                    IconButton(onClick = { viewModel.refreshSamples() }) {
                        Icon(Icons.Default.Refresh, contentDescription = "Refresh")
                    }
                }
            )
        },
        snackbarHost = { SnackbarHost(snackbarHostState) }
    ) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
        ) {
            // Part Selector
            PartSelector(
                selectedPart = selectedPart,
                onPartSelected = { viewModel.selectPart(it) },
                modifier = Modifier.fillMaxWidth()
            )

            HorizontalDivider()

            // Sample List
            if (samples.isEmpty()) {
                Box(
                    modifier = Modifier.fillMaxSize(),
                    contentAlignment = Alignment.Center
                ) {
                    Text(
                        text = "No samples found",
                        style = MaterialTheme.typography.bodyLarge,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            } else {
                LazyColumn(
                    modifier = Modifier.fillMaxSize(),
                    contentPadding = PaddingValues(8.dp),
                    verticalArrangement = Arrangement.spacedBy(4.dp)
                ) {
                    itemsIndexed(samples) { _, filename ->
                        SampleItem(
                            filename = filename,
                            isLoading = loadingStates[filename] == true,
                            selectedPart = selectedPart,
                            onLoadClick = { viewModel.loadSampleToPart(filename, selectedPart) },
                            modifier = Modifier.fillMaxWidth()
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun PartSelector(
    selectedPart: Int,
    onPartSelected: (Int) -> Unit,
    modifier: Modifier = Modifier
) {
    Column(modifier = modifier.padding(16.dp)) {
        Text(
            text = "Select Part",
            style = MaterialTheme.typography.titleMedium,
            modifier = Modifier.padding(bottom = 8.dp)
        )
        
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            repeat(8) { partIndex ->
                PartButton(
                    partIndex = partIndex,
                    isSelected = selectedPart == partIndex,
                    onClick = { onPartSelected(partIndex) },
                    modifier = Modifier.weight(1f)
                )
            }
        }
    }
}

@Composable
private fun PartButton(
    partIndex: Int,
    isSelected: Boolean,
    onClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    val backgroundColor = if (isSelected) {
        MaterialTheme.colorScheme.primary
    } else {
        MaterialTheme.colorScheme.surfaceVariant
    }
    
    val contentColor = if (isSelected) {
        MaterialTheme.colorScheme.onPrimary
    } else {
        MaterialTheme.colorScheme.onSurfaceVariant
    }

    Box(
        modifier = modifier
            .aspectRatio(1f)
            .background(
                color = backgroundColor,
                shape = MaterialTheme.shapes.small
            )
            .clickable(onClick = onClick),
        contentAlignment = Alignment.Center
    ) {
        Text(
            text = (partIndex + 1).toString(),
            style = MaterialTheme.typography.titleLarge,
            color = contentColor
        )
    }
}

@Composable
private fun SampleItem(
    filename: String,
    isLoading: Boolean,
    selectedPart: Int,
    onLoadClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    Card(
        modifier = modifier,
        elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(12.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            // Sample Info
            Column(modifier = Modifier.weight(1f)) {
                Text(
                    text = filename,
                    style = MaterialTheme.typography.bodyLarge
                )
                Text(
                    text = "Load to Part ${selectedPart + 1}",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }

            // Load Button
            if (isLoading) {
                CircularProgressIndicator(
                    modifier = Modifier.size(24.dp),
                    strokeWidth = 2.dp
                )
            } else {
                IconButton(onClick = onLoadClick) {
                    Icon(
                        imageVector = Icons.Default.PlayArrow,
                        contentDescription = "Load Sample",
                        tint = MaterialTheme.colorScheme.primary
                    )
                }
            }
        }
    }
}
