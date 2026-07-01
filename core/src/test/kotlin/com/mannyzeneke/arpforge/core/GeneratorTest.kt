package com.mannyzeneke.arpforge.core

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotEquals
import kotlin.test.assertTrue

class GeneratorTest {

    private fun allParams() = buildList {
        for (style in Style.entries)
            for (scale in ScaleType.entries)
                for (pattern in ArpPattern.entries)
                    add(
                        GenParams(
                            style = style, scale = scale, arpPattern = pattern,
                            rootPc = 4, bars = 8, seed = 1234L,
                        ),
                    )
    }

    @Test
    fun `same seed produces identical piece`() {
        val p = GenParams(seed = 99L)
        assertEquals(Generator.generate(p), Generator.generate(p))
    }

    @Test
    fun `different seeds produce different pieces`() {
        val a = Generator.generate(GenParams(seed = 1L, style = Style.HOUSE))
        val b = Generator.generate(GenParams(seed = 2L, style = Style.HOUSE))
        assertNotEquals(a, b)
    }

    @Test
    fun `chord names match bar count`() {
        for (p in allParams()) {
            val piece = Generator.generate(p)
            assertEquals(p.bars, piece.chordNames.size)
        }
    }

    @Test
    fun `all pitches are valid midi and melodic tracks stay in scale`() {
        for (p in allParams()) {
            val piece = Generator.generate(p)
            val scalePcs = p.scale.pitchClasses(p.rootPc)
            for (track in piece.tracks) {
                for (note in track.notes) {
                    assertTrue(note.pitch in 0..127, "pitch ${note.pitch} out of range")
                    assertTrue(note.velocity in 1..127, "velocity ${note.velocity} out of range")
                    assertTrue(note.startTick >= 0 && note.durationTicks > 0)
                    if (!track.isDrums) {
                        assertTrue(
                            note.pitch % 12 in scalePcs,
                            "pitch class ${note.pitch % 12} of track ${track.name} not in $scalePcs (${p.scale}, root ${p.rootPc})",
                        )
                    }
                }
            }
        }
    }

    @Test
    fun `notes fit within the piece length`() {
        for (p in allParams()) {
            val piece = Generator.generate(p)
            for (track in piece.tracks) {
                for (note in track.notes) {
                    assertTrue(note.startTick + note.durationTicks <= piece.lengthTicks + Generator.PPQ)
                }
            }
        }
    }

    @Test
    fun `expected tracks are present`() {
        val piece = Generator.generate(GenParams(arpEnabled = true, includeBass = true, includeDrums = true))
        assertEquals(listOf("Chords", "Arp", "Bass", "Drums"), piece.tracks.map { it.name })

        val minimal = Generator.generate(GenParams(arpEnabled = false, includeBass = false, includeDrums = false))
        assertEquals(listOf("Chords"), minimal.tracks.map { it.name })
    }

    @Test
    fun `arp respects octave range`() {
        val p = GenParams(arpOctaves = 3, arpEnabled = true, seed = 7L)
        val piece = Generator.generate(p)
        val arp = piece.tracks.first { it.name == "Arp" }
        val chords = piece.tracks.first { it.name == "Chords" }
        assertTrue(arp.notes.isNotEmpty())
        // The arp sits above the pad voicings.
        assertTrue(arp.notes.minOf { it.pitch } >= chords.notes.minOf { it.pitch })
    }

    @Test
    fun `chord builder names common chords`() {
        assertEquals("Am", ChordBuilder.build(ScaleType.MINOR, 9, 0, ChordExtension.TRIAD).name)
        assertEquals("Am7", ChordBuilder.build(ScaleType.MINOR, 9, 0, ChordExtension.SEVENTH).name)
        assertEquals("Am9", ChordBuilder.build(ScaleType.MINOR, 9, 0, ChordExtension.NINTH).name)
        assertEquals("Asus2", ChordBuilder.build(ScaleType.MINOR, 9, 0, ChordExtension.SUS2).name)
        assertEquals("C", ChordBuilder.build(ScaleType.MAJOR, 0, 0, ChordExtension.TRIAD).name)
        assertEquals("Cmaj7", ChordBuilder.build(ScaleType.MAJOR, 0, 0, ChordExtension.SEVENTH).name)
        assertEquals("F", ChordBuilder.build(ScaleType.MINOR, 9, 5, ChordExtension.TRIAD).name) // VI of A minor
        assertEquals("Bm7b5", ChordBuilder.build(ScaleType.MINOR, 9, 1, ChordExtension.SEVENTH).name)
        assertEquals("G7", ChordBuilder.build(ScaleType.MAJOR, 0, 4, ChordExtension.SEVENTH).name)
    }
}
