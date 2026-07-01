package com.mannyzeneke.arpforge.core

val NOTE_NAMES = arrayOf("C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B")

enum class ScaleType(val label: String, val intervals: IntArray) {
    MINOR("Minor", intArrayOf(0, 2, 3, 5, 7, 8, 10)),
    MAJOR("Major", intArrayOf(0, 2, 4, 5, 7, 9, 11)),
    DORIAN("Dorian", intArrayOf(0, 2, 3, 5, 7, 9, 10)),
    PHRYGIAN("Phrygian", intArrayOf(0, 1, 3, 5, 7, 8, 10)),
    HARMONIC_MINOR("Harm. Minor", intArrayOf(0, 2, 3, 5, 7, 8, 11));

    /** Semitones above the key root for an arbitrary scale step (can exceed one octave). */
    fun stepSemitones(step: Int): Int = intervals[step % 7] + 12 * (step / 7)

    fun pitchClasses(rootPc: Int): Set<Int> = intervals.map { (rootPc + it) % 12 }.toSet()
}

enum class ChordExtension { TRIAD, SUS2, SEVENTH, NINTH }

/**
 * A chord built diatonically on a scale degree.
 *
 * @param semitones semitones of each chord tone above the KEY root, ascending.
 * @param rootPc pitch class (0-11) of the chord root in absolute terms.
 */
data class ChordSpec(val degree: Int, val semitones: IntArray, val rootPc: Int, val name: String) {
    override fun equals(other: Any?) = other is ChordSpec &&
        degree == other.degree && semitones.contentEquals(other.semitones) &&
        rootPc == other.rootPc && name == other.name

    override fun hashCode() = 31 * (31 * (31 * degree + semitones.contentHashCode()) + rootPc) + name.hashCode()
}

object ChordBuilder {

    fun build(scale: ScaleType, keyRootPc: Int, degree: Int, extension: ChordExtension): ChordSpec {
        val steps = when (extension) {
            ChordExtension.TRIAD -> intArrayOf(0, 2, 4)
            ChordExtension.SUS2 -> intArrayOf(0, 1, 4)
            ChordExtension.SEVENTH -> intArrayOf(0, 2, 4, 6)
            // Classic deep-house voicing: drop the 5th, keep root/3rd/7th/9th.
            ChordExtension.NINTH -> intArrayOf(0, 2, 6, 8)
        }
        val semis = steps.map { scale.stepSemitones(degree + it) }.toIntArray()
        val rootPc = (keyRootPc + semis[0]) % 12
        return ChordSpec(degree, semis, rootPc, name(rootPc, semis, extension))
    }

    private fun name(rootPc: Int, semis: IntArray, extension: ChordExtension): String {
        val rel = semis.map { it - semis[0] }
        val root = NOTE_NAMES[rootPc]
        if (extension == ChordExtension.SUS2) return root + "sus2"
        val third = rel.getOrNull(1) ?: return root
        var quality = when (third % 12) {
            3 -> "m"
            4 -> ""
            else -> "?"
        }
        // Diminished / augmented fifth (triads and sevenths carry the 5th at index 2).
        val fifth = if (extension == ChordExtension.TRIAD || extension == ChordExtension.SEVENTH)
            rel.getOrNull(2)?.rem(12) else null
        val seventh = when (extension) {
            ChordExtension.SEVENTH -> rel.getOrNull(3)?.rem(12)
            ChordExtension.NINTH -> rel.getOrNull(2)?.rem(12)
            else -> null
        }
        val hasNinth = extension == ChordExtension.NINTH
        var suffix = when (seventh) {
            10 -> if (hasNinth) "9" else "7"
            11 -> if (hasNinth) "maj9" else "maj7"
            else -> ""
        }
        if (fifth == 6) {
            return if (seventh == 10) root + "m7b5" else root + "dim"
        }
        if (fifth == 8) quality = "+"
        return root + quality + suffix
    }
}
