package com.mannyzeneke.arpforge

import android.content.Intent
import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.FilterChip
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Slider
import androidx.compose.material3.Surface
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.content.FileProvider
import androidx.lifecycle.viewmodel.compose.viewModel
import com.mannyzeneke.arpforge.core.ArpPattern
import com.mannyzeneke.arpforge.core.ArpRate
import com.mannyzeneke.arpforge.core.ChordRhythm
import com.mannyzeneke.arpforge.core.NOTE_NAMES
import com.mannyzeneke.arpforge.core.ScaleType
import com.mannyzeneke.arpforge.core.Style

private val ArpForgeColors = darkColorScheme(
    primary = Color(0xFF7C5CFF),
    onPrimary = Color.White,
    secondary = Color(0xFF00E5B0),
    background = Color(0xFF0D0F14),
    surface = Color(0xFF12141C),
    surfaceVariant = Color(0xFF1A1D28),
    onBackground = Color(0xFFE8E9F0),
    onSurface = Color(0xFFE8E9F0),
)

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            MaterialTheme(colorScheme = ArpForgeColors) {
                Surface(modifier = Modifier.fillMaxSize(), color = MaterialTheme.colorScheme.background) {
                    MainScreen()
                }
            }
        }
    }
}

@Composable
fun MainScreen(vm: MainViewModel = viewModel()) {
    val context = LocalContext.current
    val params = vm.params
    val piece = vm.piece

    val exportLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.CreateDocument("audio/midi"),
    ) { uri ->
        if (uri != null) {
            val ok = vm.exportTo(uri)
            Toast.makeText(
                context,
                if (ok) "MIDI file saved" else "Export failed",
                Toast.LENGTH_SHORT,
            ).show()
        }
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState())
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("ArpForge", fontSize = 26.sp, fontWeight = FontWeight.Bold)
            Spacer(Modifier.width(8.dp))
            Text(
                "EDM & house arps + chords",
                fontSize = 13.sp,
                color = MaterialTheme.colorScheme.secondary,
            )
        }

        // --- Result preview ---
        Card(colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface)) {
            Column(Modifier.padding(12.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                Text(
                    "${piece.keyName} · ${params.bpm} BPM",
                    fontWeight = FontWeight.SemiBold,
                )
                Row(
                    Modifier
                        .fillMaxWidth()
                        .horizontalScroll(rememberScrollState()),
                    horizontalArrangement = Arrangement.spacedBy(6.dp),
                ) {
                    piece.chordNames.forEach { name ->
                        Text(
                            name,
                            color = MaterialTheme.colorScheme.primary,
                            fontWeight = FontWeight.Bold,
                            fontSize = 16.sp,
                        )
                    }
                }
                PianoRoll(
                    piece,
                    Modifier
                        .fillMaxWidth()
                        .height(160.dp),
                )
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    Button(onClick = { vm.togglePlayback() }, modifier = Modifier.weight(1f)) {
                        Text(if (vm.isPlaying) "■ Stop" else "▶ Play")
                    }
                    Button(onClick = { vm.newIdea() }, modifier = Modifier.weight(1f)) {
                        Text("🎲 New idea")
                    }
                }
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    OutlinedButton(
                        onClick = { exportLauncher.launch(params.suggestedFileName()) },
                        modifier = Modifier.weight(1f),
                    ) {
                        Text("Save .mid")
                    }
                    OutlinedButton(
                        onClick = {
                            val file = vm.writeShareFile()
                            val uri = FileProvider.getUriForFile(
                                context,
                                context.packageName + ".fileprovider",
                                file,
                            )
                            val intent = Intent(Intent.ACTION_SEND).apply {
                                type = "audio/midi"
                                putExtra(Intent.EXTRA_STREAM, uri)
                                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                            }
                            context.startActivity(Intent.createChooser(intent, "Share MIDI"))
                        },
                        modifier = Modifier.weight(1f),
                    ) {
                        Text("Share")
                    }
                }
            }
        }

        // --- Style & key ---
        Section("Style") {
            ChipRow(Style.entries, params.style, { it.label }) { vm.update { p -> p.copy(style = it) } }
        }
        Section("Key") {
            ChipRow(NOTE_NAMES.indices.toList(), params.rootPc, { NOTE_NAMES[it] }) {
                vm.update { p -> p.copy(rootPc = it) }
            }
            ChipRow(ScaleType.entries, params.scale, { it.label }) { vm.update { p -> p.copy(scale = it) } }
        }
        Section("Groove") {
            LabeledSlider(
                label = "Tempo: ${params.bpm} BPM",
                value = params.bpm.toFloat(),
                range = 90f..150f,
                steps = 59,
            ) { vm.update { p -> p.copy(bpm = it.toInt()) } }
            ChipRow(listOf(4, 8, 16), params.bars, { "$it bars" }) { vm.update { p -> p.copy(bars = it) } }
        }
        Section("Chords") {
            ChipRow(ChordRhythm.entries, params.chordRhythm, { it.label }) {
                vm.update { p -> p.copy(chordRhythm = it) }
            }
        }
        Section("Arp") {
            ToggleRow("Arp enabled", params.arpEnabled) { vm.update { p -> p.copy(arpEnabled = it) } }
            if (params.arpEnabled) {
                ChipRow(ArpPattern.entries, params.arpPattern, { it.label }) {
                    vm.update { p -> p.copy(arpPattern = it) }
                }
                ChipRow(ArpRate.entries, params.arpRate, { it.label }) {
                    vm.update { p -> p.copy(arpRate = it) }
                }
                ChipRow(listOf(1, 2, 3), params.arpOctaves, { "$it oct" }) {
                    vm.update { p -> p.copy(arpOctaves = it) }
                }
                LabeledSlider(
                    label = "Gate: ${(params.arpGate * 100).toInt()}%",
                    value = params.arpGate,
                    range = 0.25f..1f,
                ) { vm.update { p -> p.copy(arpGate = it) } }
            }
        }
        Section("Layers") {
            ToggleRow("Bass line", params.includeBass) { vm.update { p -> p.copy(includeBass = it) } }
            ToggleRow("Drums (preview groove)", params.includeDrums) { vm.update { p -> p.copy(includeDrums = it) } }
        }
        Spacer(Modifier.height(24.dp))
    }
}

@Composable
private fun Section(title: String, content: @Composable () -> Unit) {
    Card(colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface)) {
        Column(Modifier.padding(12.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Text(
                title.uppercase(),
                fontSize = 12.sp,
                fontWeight = FontWeight.Bold,
                color = MaterialTheme.colorScheme.secondary,
            )
            content()
        }
    }
}

@Composable
private fun <T> ChipRow(
    options: List<T>,
    selected: T,
    label: (T) -> String,
    onSelect: (T) -> Unit,
) {
    Row(
        Modifier
            .fillMaxWidth()
            .horizontalScroll(rememberScrollState()),
        horizontalArrangement = Arrangement.spacedBy(6.dp),
    ) {
        options.forEach { option ->
            FilterChip(
                selected = option == selected,
                onClick = { onSelect(option) },
                label = { Text(label(option)) },
            )
        }
    }
}

@Composable
private fun LabeledSlider(
    label: String,
    value: Float,
    range: ClosedFloatingPointRange<Float>,
    steps: Int = 0,
    onChange: (Float) -> Unit,
) {
    Column {
        Text(label, fontSize = 14.sp)
        Slider(value = value, onValueChange = onChange, valueRange = range, steps = steps)
    }
}

@Composable
private fun ToggleRow(label: String, checked: Boolean, onChange: (Boolean) -> Unit) {
    Row(
        Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(label, fontSize = 14.sp)
        Switch(checked = checked, onCheckedChange = onChange)
    }
}
