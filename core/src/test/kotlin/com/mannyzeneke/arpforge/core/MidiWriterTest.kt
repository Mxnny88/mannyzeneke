package com.mannyzeneke.arpforge.core

import com.mannyzeneke.arpforge.core.MidiWriter.writeVarLen
import java.io.ByteArrayOutputStream
import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertTrue

class MidiWriterTest {

    private fun varLen(value: Long): ByteArray {
        val out = ByteArrayOutputStream()
        out.writeVarLen(value)
        return out.toByteArray()
    }

    @Test
    fun `variable length quantities match the SMF spec examples`() {
        assertContentEquals(byteArrayOf(0x00), varLen(0))
        assertContentEquals(byteArrayOf(0x7F), varLen(0x7F))
        assertContentEquals(byteArrayOf(0x81.toByte(), 0x00), varLen(0x80))
        assertContentEquals(byteArrayOf(0xFF.toByte(), 0x7F), varLen(0x3FFF))
        assertContentEquals(byteArrayOf(0x81.toByte(), 0x80.toByte(), 0x00), varLen(0x4000))
        assertContentEquals(byteArrayOf(0xFF.toByte(), 0xFF.toByte(), 0xFF.toByte(), 0x7F), varLen(0x0FFFFFFF))
    }

    @Test
    fun `header chunk is well formed`() {
        val piece = Generator.generate(GenParams(seed = 3L))
        val bytes = MidiWriter.write(piece)
        assertEquals("MThd", String(bytes, 0, 4, Charsets.US_ASCII))
        assertEquals(6, readInt32(bytes, 4))
        assertEquals(1, readInt16(bytes, 8)) // format 1
        assertEquals(piece.tracks.size + 1, readInt16(bytes, 10)) // + tempo track
        assertEquals(piece.ppq, readInt16(bytes, 12))
    }

    @Test
    fun `written file round-trips through a parser`() {
        for (style in Style.entries) {
            val params = GenParams(style = style, bpm = 128, bars = 8, seed = 11L)
            val piece = Generator.generate(params)
            val parsed = parse(MidiWriter.write(piece))

            assertEquals(piece.tracks.size + 1, parsed.tracks.size)
            assertEquals(60_000_000 / 128, parsed.microsPerQuarter)

            piece.tracks.forEachIndexed { i, track ->
                val parsedTrack = parsed.tracks[i + 1]
                assertEquals(track.notes.size, parsedTrack.noteOns.size, "note count of ${track.name}")
                assertEquals(track.notes.size, parsedTrack.noteOffs, "note-off count of ${track.name}")
                val expected = track.notes.map { it.startTick to it.pitch }.sortedWith(compareBy({ it.first }, { it.second }))
                val actual = parsedTrack.noteOns.map { it.first to it.second }.sortedWith(compareBy({ it.first }, { it.second }))
                assertEquals(expected, actual, "note-on ticks/pitches of ${track.name}")
            }
        }
    }

    @Test
    fun `every note on has a matching note off`() {
        val piece = Generator.generate(GenParams(style = Style.EDM, seed = 21L))
        for (track in parse(MidiWriter.write(piece)).tracks) {
            assertEquals(track.noteOns.size, track.noteOffs)
        }
    }

    @Test
    fun `file is non trivial`() {
        val bytes = MidiWriter.write(Generator.generate(GenParams()))
        assertTrue(bytes.size > 500)
    }

    // --- minimal SMF parser used only for verification ---

    private class ParsedTrack {
        val noteOns = mutableListOf<Pair<Long, Int>>() // (absolute tick, pitch)
        var noteOffs = 0
    }

    private class ParsedFile(val microsPerQuarter: Int, val tracks: List<ParsedTrack>)

    private fun readInt16(b: ByteArray, at: Int) =
        ((b[at].toInt() and 0xFF) shl 8) or (b[at + 1].toInt() and 0xFF)

    private fun readInt32(b: ByteArray, at: Int) =
        ((b[at].toInt() and 0xFF) shl 24) or ((b[at + 1].toInt() and 0xFF) shl 16) or
            ((b[at + 2].toInt() and 0xFF) shl 8) or (b[at + 3].toInt() and 0xFF)

    private fun parse(bytes: ByteArray): ParsedFile {
        require(String(bytes, 0, 4, Charsets.US_ASCII) == "MThd")
        val trackCount = readInt16(bytes, 10)
        var pos = 8 + readInt32(bytes, 4)
        var tempo = -1
        val tracks = mutableListOf<ParsedTrack>()
        repeat(trackCount) {
            require(String(bytes, pos, 4, Charsets.US_ASCII) == "MTrk") { "bad track header at $pos" }
            val len = readInt32(bytes, pos + 4)
            var p = pos + 8
            val end = p + len
            val track = ParsedTrack()
            var tick = 0L
            var running = -1
            while (p < end) {
                var delta = 0L
                while (true) {
                    val byte = bytes[p++].toInt() and 0xFF
                    delta = (delta shl 7) or (byte and 0x7F).toLong()
                    if (byte and 0x80 == 0) break
                }
                tick += delta
                var status = bytes[p].toInt() and 0xFF
                if (status >= 0x80) p++ else status = running
                running = status
                when {
                    status == 0xFF -> {
                        val type = bytes[p++].toInt() and 0xFF
                        var metaLen = 0
                        while (true) {
                            val byte = bytes[p++].toInt() and 0xFF
                            metaLen = (metaLen shl 7) or (byte and 0x7F)
                            if (byte and 0x80 == 0) break
                        }
                        if (type == 0x51) {
                            tempo = ((bytes[p].toInt() and 0xFF) shl 16) or
                                ((bytes[p + 1].toInt() and 0xFF) shl 8) or (bytes[p + 2].toInt() and 0xFF)
                        }
                        p += metaLen
                    }
                    status in 0x90..0x9F -> {
                        val pitch = bytes[p].toInt() and 0xFF
                        val velocity = bytes[p + 1].toInt() and 0xFF
                        p += 2
                        if (velocity > 0) track.noteOns += tick to pitch else track.noteOffs++
                    }
                    status in 0x80..0x8F -> {
                        p += 2
                        track.noteOffs++
                    }
                    status in 0xC0..0xDF -> p += 1
                    else -> p += 2
                }
            }
            require(p == end) { "track over/under-run: $p vs $end" }
            tracks += track
            pos = end
        }
        return ParsedFile(tempo, tracks)
    }
}
