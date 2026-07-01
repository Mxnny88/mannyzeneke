package com.mannyzeneke.arpforge.core

enum class Style(val label: String) {
    EDM("EDM"),
    HOUSE("House");
}

enum class ChordRhythm(val label: String) {
    SUSTAINED("Sustained pad"),
    PUMPING("Pumping 8ths"),
    OFFBEAT_STABS("Offbeat stabs");
}

enum class ArpPattern(val label: String) {
    UP("Up"),
    DOWN("Down"),
    UP_DOWN("Up-Down"),
    DOWN_UP("Down-Up"),
    CONVERGE("Converge"),
    ROOT_BOUNCE("Root bounce"),
    RANDOM("Random");
}

enum class ArpRate(val label: String, val stepsPerBeat: Int) {
    EIGHTH("1/8", 2),
    EIGHTH_TRIPLET("1/8T", 3),
    SIXTEENTH("1/16", 4),
    SIXTEENTH_TRIPLET("1/16T", 6);
}

data class GenParams(
    val style: Style = Style.HOUSE,
    val rootPc: Int = 9, // A
    val scale: ScaleType = ScaleType.MINOR,
    val bpm: Int = 124,
    val bars: Int = 4,
    val chordRhythm: ChordRhythm = ChordRhythm.SUSTAINED,
    val arpEnabled: Boolean = true,
    val arpPattern: ArpPattern = ArpPattern.UP,
    val arpRate: ArpRate = ArpRate.SIXTEENTH,
    val arpOctaves: Int = 2,
    val arpGate: Float = 0.8f,
    val includeBass: Boolean = true,
    val includeDrums: Boolean = true,
    val seed: Long = 42L,
) {
    val keyName: String get() = NOTE_NAMES[rootPc] + " " + scale.label

    fun suggestedFileName(): String =
        "arpforge_${style.label.lowercase()}_${NOTE_NAMES[rootPc].replace('#', 's')}_${scale.label.lowercase().replace(". ", "").replace(' ', '_')}_${bpm}bpm.mid"
}
