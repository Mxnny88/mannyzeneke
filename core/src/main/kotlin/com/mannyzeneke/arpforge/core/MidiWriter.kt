package com.mannyzeneke.arpforge.core

import java.io.ByteArrayOutputStream

/**
 * Writes a [GeneratedPiece] as a Standard MIDI File (format 1).
 */
object MidiWriter {

    fun write(piece: GeneratedPiece): ByteArray {
        val out = ByteArrayOutputStream()
        val trackChunks = buildList {
            add(tempoTrack(piece))
            piece.tracks.forEach { add(noteTrack(it)) }
        }

        out.writeAscii("MThd")
        out.writeInt32(6)
        out.writeInt16(1) // format 1
        out.writeInt16(trackChunks.size)
        out.writeInt16(piece.ppq)
        trackChunks.forEach { chunk ->
            out.writeAscii("MTrk")
            out.writeInt32(chunk.size)
            out.write(chunk)
        }
        return out.toByteArray()
    }

    private fun tempoTrack(piece: GeneratedPiece): ByteArray {
        val t = ByteArrayOutputStream()
        t.writeMeta(0, 0x03, "ArpForge ${piece.keyName}".toByteArray(Charsets.US_ASCII))
        val microsPerQuarter = 60_000_000 / piece.bpm
        t.writeMeta(
            0, 0x51,
            byteArrayOf(
                (microsPerQuarter ushr 16).toByte(),
                (microsPerQuarter ushr 8).toByte(),
                microsPerQuarter.toByte(),
            ),
        )
        t.writeMeta(0, 0x58, byteArrayOf(4, 2, 24, 8)) // 4/4
        t.writeMeta(0, 0x2F, ByteArray(0)) // end of track
        return t.toByteArray()
    }

    private fun noteTrack(track: TrackData): ByteArray {
        val t = ByteArrayOutputStream()
        t.writeMeta(0, 0x03, track.name.toByteArray(Charsets.US_ASCII))
        t.writeVarLen(0)
        t.write(0xC0 or track.channel)
        t.write(track.program and 0x7F)

        // (tick, isNoteOn, pitch, velocity); note-offs sort before note-ons at
        // the same tick so immediate retriggers don't get swallowed.
        data class Event(val tick: Long, val on: Boolean, val pitch: Int, val velocity: Int)

        val events = buildList {
            for (n in track.notes) {
                add(Event(n.startTick, true, n.pitch, n.velocity))
                add(Event(n.startTick + n.durationTicks, false, n.pitch, 0))
            }
        }.sortedWith(compareBy({ it.tick }, { it.on }))

        var lastTick = 0L
        for (e in events) {
            t.writeVarLen(e.tick - lastTick)
            lastTick = e.tick
            t.write((if (e.on) 0x90 else 0x80) or track.channel)
            t.write(e.pitch and 0x7F)
            t.write(e.velocity and 0x7F)
        }
        t.writeMeta(0, 0x2F, ByteArray(0))
        return t.toByteArray()
    }

    private fun ByteArrayOutputStream.writeMeta(delta: Long, type: Int, data: ByteArray) {
        writeVarLen(delta)
        write(0xFF)
        write(type)
        writeVarLen(data.size.toLong())
        write(data)
    }

    internal fun ByteArrayOutputStream.writeVarLen(value: Long) {
        require(value >= 0) { "negative delta time" }
        var buffer = value and 0x7F
        var v = value shr 7
        while (v > 0) {
            buffer = (buffer shl 8) or 0x80 or (v and 0x7F)
            v = v shr 7
        }
        while (true) {
            write((buffer and 0xFF).toInt())
            if (buffer and 0x80 != 0L) buffer = buffer shr 8 else break
        }
    }

    private fun ByteArrayOutputStream.writeAscii(s: String) = write(s.toByteArray(Charsets.US_ASCII))

    private fun ByteArrayOutputStream.writeInt16(v: Int) {
        write((v ushr 8) and 0xFF)
        write(v and 0xFF)
    }

    private fun ByteArrayOutputStream.writeInt32(v: Int) {
        write((v ushr 24) and 0xFF)
        write((v ushr 16) and 0xFF)
        write((v ushr 8) and 0xFF)
        write(v and 0xFF)
    }
}
