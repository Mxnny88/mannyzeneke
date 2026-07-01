package com.mannyzeneke.arpforge.core

data class NoteEvent(
    val startTick: Long,
    val durationTicks: Long,
    val pitch: Int,
    val velocity: Int,
)

data class TrackData(
    val name: String,
    val channel: Int,
    val program: Int,
    val notes: List<NoteEvent>,
    val isDrums: Boolean = false,
)

data class GeneratedPiece(
    val bpm: Int,
    val ppq: Int,
    val bars: Int,
    val keyName: String,
    val chordNames: List<String>,
    val tracks: List<TrackData>,
) {
    val lengthTicks: Long get() = bars.toLong() * ppq * 4
}
