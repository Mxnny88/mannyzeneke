package com.mannyzeneke.arpforge

import androidx.compose.foundation.Canvas
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import com.mannyzeneke.arpforge.core.GeneratedPiece

private val TRACK_COLORS = mapOf(
    "Chords" to Color(0xFF7C5CFF),
    "Arp" to Color(0xFF00E5B0),
    "Bass" to Color(0xFFFF8A5C),
)

@Composable
fun PianoRoll(piece: GeneratedPiece, modifier: Modifier = Modifier) {
    Canvas(modifier) {
        drawRoundRect(Color(0xFF161923), cornerRadius = CornerRadius(16f, 16f))

        val melodic = piece.tracks.filter { !it.isDrums }
        val notes = melodic.flatMap { t -> t.notes.map { t.name to it } }
        if (notes.isEmpty()) return@Canvas

        val minPitch = notes.minOf { it.second.pitch } - 1
        val maxPitch = notes.maxOf { it.second.pitch } + 1
        val range = (maxPitch - minPitch).coerceAtLeast(1)
        val totalTicks = piece.lengthTicks.toFloat()
        val rowH = size.height / range

        // Beat grid, heavier line on bar boundaries.
        val beats = piece.bars * 4
        for (beat in 0..beats) {
            val x = size.width * beat / beats
            drawLine(
                color = if (beat % 4 == 0) Color(0xFF2A2E3D) else Color(0xFF1E2230),
                start = Offset(x, 0f),
                end = Offset(x, size.height),
                strokeWidth = if (beat % 4 == 0) 2f else 1f,
            )
        }

        for ((trackName, note) in notes) {
            val x = size.width * note.startTick / totalTicks
            val w = (size.width * note.durationTicks / totalTicks).coerceAtLeast(2f)
            val y = size.height * (maxPitch - note.pitch) / range
            val color = TRACK_COLORS[trackName] ?: Color.Gray
            drawRoundRect(
                color = color.copy(alpha = 0.45f + 0.55f * (note.velocity / 127f)),
                topLeft = Offset(x, y),
                size = Size(w - 1f, (rowH - 1f).coerceAtLeast(2f)),
                cornerRadius = CornerRadius(3f, 3f),
            )
        }
    }
}
