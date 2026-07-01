package com.mannyzeneke.arpforge.core

import kotlin.math.abs
import kotlin.random.Random

/**
 * Generates EDM / house chord progressions with arps, bass and drums,
 * deterministically from [GenParams.seed].
 */
object Generator {

    const val PPQ = 480
    private const val BAR = PPQ * 4

    // Scale-degree templates (0-based). Degree qualities come from the chosen scale,
    // so the same template adapts to minor, dorian, etc.
    private val PROGRESSIONS: Map<Style, List<IntArray>> = mapOf(
        Style.EDM to listOf(
            intArrayOf(0, 5, 2, 6), // i - VI - III - VII (the festival classic)
            intArrayOf(0, 6, 5, 6), // i - VII - VI - VII
            intArrayOf(0, 3, 5, 6), // i - iv - VI - VII
            intArrayOf(5, 6, 0, 0), // VI - VII - i - i
            intArrayOf(0, 5, 6, 3), // i - VI - VII - iv
        ),
        Style.HOUSE to listOf(
            intArrayOf(0, 3, 0, 4), // i - iv - i - v
            intArrayOf(0, 3, 5, 4), // i - iv - VI - v
            intArrayOf(0, 3, 0, 3), // two-chord vamp
            intArrayOf(0, 2, 3, 6), // i - III - iv - VII
            intArrayOf(0, 4, 5, 3), // i - v - VI - iv
        ),
    )

    private const val PROGRAM_SAW_LEAD = 81   // GM Lead 2 (sawtooth)
    private const val PROGRAM_WARM_PAD = 89   // GM Pad 2 (warm)
    private const val PROGRAM_EPIANO = 4      // GM Electric Piano 1
    private const val PROGRAM_SYNTH_BASS = 38 // GM Synth Bass 1

    private const val KICK = 36
    private const val CLAP = 39
    private const val CLOSED_HAT = 42
    private const val OPEN_HAT = 46

    fun generate(p: GenParams): GeneratedPiece {
        val rnd = Random(p.seed)
        val template = PROGRESSIONS.getValue(p.style).let { it[rnd.nextInt(it.size)] }
        val degreeSequence = List(p.bars) { template[it % template.size] }

        // One chord spec per distinct degree so a repeated degree keeps its flavour.
        val specByDegree = degreeSequence.distinct().associateWith { degree ->
            val extension = when (p.style) {
                Style.EDM -> if (rnd.nextFloat() < 0.25f) ChordExtension.SUS2 else ChordExtension.TRIAD
                Style.HOUSE -> if (rnd.nextFloat() < 0.45f) ChordExtension.NINTH else ChordExtension.SEVENTH
            }
            ChordBuilder.build(p.scale, p.rootPc, degree, extension)
        }
        val specs = degreeSequence.map { specByDegree.getValue(it) }

        // Voice chords around middle C with simple voice leading.
        val keyBase = 60 + if (p.rootPc > 6) p.rootPc - 12 else p.rootPc
        val voicings = mutableListOf<List<Int>>()
        for (spec in specs) {
            val raw = spec.semitones.map { keyBase + it }
            voicings += if (voicings.isEmpty()) raw else closestVoicing(raw, voicings.last())
        }

        val tracks = mutableListOf<TrackData>()
        tracks += chordTrack(p, voicings, rnd)
        if (p.arpEnabled) tracks += arpTrack(p, voicings, rnd)
        if (p.includeBass) tracks += bassTrack(p, specs, rnd)
        if (p.includeDrums) tracks += drumTrack(p, rnd)

        return GeneratedPiece(
            bpm = p.bpm,
            ppq = PPQ,
            bars = p.bars,
            keyName = p.keyName,
            chordNames = specs.map { it.name },
            tracks = tracks,
        )
    }

    /** Pick the inversion/octave of [raw] whose centre is closest to the previous chord's. */
    private fun closestVoicing(raw: List<Int>, previous: List<Int>): List<Int> {
        val prevMean = previous.average()
        var best = raw
        var bestScore = Double.MAX_VALUE
        for (octaveShift in -1..1) {
            for (inversion in raw.indices) {
                val candidate = raw.sorted().toMutableList()
                for (i in 0 until inversion) candidate[i] += 12
                val shifted = candidate.map { it + octaveShift * 12 }.sorted()
                // Small penalty keeps root position preferred when it's just as close.
                val score = abs(shifted.average() - prevMean) + inversion * 0.35
                if (score < bestScore && shifted.all { it in 24..108 }) {
                    bestScore = score
                    best = shifted
                }
            }
        }
        return best
    }

    private fun humanize(base: Int, rnd: Random, spread: Int = 6): Int =
        (base + rnd.nextInt(-spread, spread + 1)).coerceIn(1, 127)

    private fun chordTrack(p: GenParams, voicings: List<List<Int>>, rnd: Random): TrackData {
        val notes = mutableListOf<NoteEvent>()
        val eighth = PPQ / 2
        voicings.forEachIndexed { bar, chord ->
            val barStart = bar.toLong() * BAR
            when (p.chordRhythm) {
                ChordRhythm.SUSTAINED -> chord.forEach {
                    notes += NoteEvent(barStart, (BAR * 0.98).toLong(), it, humanize(78, rnd, 4))
                }
                ChordRhythm.PUMPING -> for (step in 0 until 8) {
                    val start = barStart + step.toLong() * eighth
                    val accent = if (step % 2 == 0) 8 else 0
                    chord.forEach {
                        notes += NoteEvent(start, (eighth * 0.6).toLong(), it, humanize(82 + accent, rnd, 4))
                    }
                }
                ChordRhythm.OFFBEAT_STABS -> for (step in intArrayOf(1, 3, 5, 7)) {
                    val start = barStart + step.toLong() * eighth
                    chord.forEach {
                        notes += NoteEvent(start, (eighth * 0.45).toLong(), it, humanize(92, rnd, 5))
                    }
                }
            }
        }
        val program = if (p.style == Style.HOUSE) PROGRAM_EPIANO else PROGRAM_WARM_PAD
        return TrackData("Chords", channel = 0, program = program, notes = notes)
    }

    private fun arpTrack(p: GenParams, voicings: List<List<Int>>, rnd: Random): TrackData {
        val notes = mutableListOf<NoteEvent>()
        val stepTicks = (PPQ / p.arpRate.stepsPerBeat).toLong()
        val stepsPerBar = p.arpRate.stepsPerBeat * 4
        val gateTicks = (stepTicks * p.arpGate).toLong().coerceAtLeast(10)

        voicings.forEachIndexed { bar, chord ->
            val barStart = bar.toLong() * BAR
            // Arp sits an octave above the pad, spread over the octave range.
            val pool = buildList {
                for (oct in 0 until p.arpOctaves) {
                    chord.sorted().forEach { add(it + 12 + oct * 12) }
                }
            }.distinct().sorted().filter { it <= 120 }
            if (pool.isEmpty()) return@forEachIndexed

            val sequence = arpSequence(pool, p.arpPattern, stepsPerBar, rnd)
            for (step in 0 until stepsPerBar) {
                val start = barStart + step * stepTicks
                val onBeat = step % p.arpRate.stepsPerBeat == 0
                val velocity = humanize(if (onBeat) 106 else 92, rnd)
                notes += NoteEvent(start, gateTicks, sequence[step % sequence.size], velocity)
            }
        }
        return TrackData("Arp", channel = 1, program = PROGRAM_SAW_LEAD, notes = notes)
    }

    private fun arpSequence(pool: List<Int>, pattern: ArpPattern, steps: Int, rnd: Random): List<Int> =
        when (pattern) {
            ArpPattern.UP -> pool
            ArpPattern.DOWN -> pool.reversed()
            ArpPattern.UP_DOWN ->
                if (pool.size <= 2) pool else pool + pool.subList(1, pool.size - 1).reversed()
            ArpPattern.DOWN_UP ->
                if (pool.size <= 2) pool.reversed()
                else pool.reversed() + pool.subList(1, pool.size - 1)
            ArpPattern.CONVERGE -> buildList {
                var lo = 0
                var hi = pool.size - 1
                while (lo <= hi) {
                    add(pool[lo])
                    if (lo != hi) add(pool[hi])
                    lo++; hi--
                }
            }
            ArpPattern.ROOT_BOUNCE -> buildList {
                val root = pool.first()
                for (other in pool.drop(1)) {
                    add(root)
                    add(other)
                }
                if (isEmpty()) add(root)
            }
            ArpPattern.RANDOM -> List(steps) { pool[rnd.nextInt(pool.size)] }
        }

    private fun bassTrack(p: GenParams, specs: List<ChordSpec>, rnd: Random): TrackData {
        val notes = mutableListOf<NoteEvent>()
        val eighth = PPQ / 2
        specs.forEachIndexed { bar, spec ->
            val barStart = bar.toLong() * BAR
            val pitch = 36 + spec.rootPc // C2..B2
            when (p.style) {
                Style.EDM -> for (step in 0 until 8) {
                    val accent = if (step % 2 == 0) 10 else 0
                    notes += NoteEvent(
                        barStart + step.toLong() * eighth,
                        (eighth * 0.7).toLong(), pitch, humanize(96 + accent, rnd, 4),
                    )
                }
                Style.HOUSE -> for (step in intArrayOf(1, 3, 5, 7)) {
                    notes += NoteEvent(
                        barStart + step.toLong() * eighth,
                        (eighth * 0.55).toLong(), pitch, humanize(104, rnd, 4),
                    )
                }
            }
        }
        return TrackData("Bass", channel = 2, program = PROGRAM_SYNTH_BASS, notes = notes)
    }

    private fun drumTrack(p: GenParams, rnd: Random): TrackData {
        val notes = mutableListOf<NoteEvent>()
        val eighth = PPQ / 2
        for (bar in 0 until p.bars) {
            val barStart = bar.toLong() * BAR
            for (beat in 0 until 4) {
                notes += NoteEvent(barStart + beat.toLong() * PPQ, eighth.toLong(), KICK, 120)
            }
            for (beat in intArrayOf(1, 3)) {
                notes += NoteEvent(barStart + beat.toLong() * PPQ, eighth.toLong(), CLAP, humanize(100, rnd, 4))
            }
            val hat = if (p.style == Style.HOUSE) OPEN_HAT else CLOSED_HAT
            for (step in intArrayOf(1, 3, 5, 7)) {
                notes += NoteEvent(
                    barStart + step.toLong() * eighth,
                    (eighth * 0.5).toLong(), hat, humanize(88, rnd),
                )
            }
        }
        return TrackData("Drums", channel = 9, program = 0, notes = notes, isDrums = true)
    }
}
