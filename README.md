# ArpForge 🎹

An Android app that generates **EDM and house style arpeggios, chord progressions, bass lines and drum grooves**, previews them in-app, and **exports the result as a standard MIDI file** you can drop straight into FL Studio, Ableton, or any other DAW.

## Features

- **Two styles** — *EDM* (festival triads, sawtooth arps, pumping 8th chords) and *House* (7th/9th chords, e-piano offbeat stabs, offbeat bass).
- **Chord progression generator** — style-appropriate progressions (i–VI–III–VII, i–iv–i–v vamps, …) built diatonically in any of 12 keys and 5 scales (minor, major, dorian, phrygian, harmonic minor), with automatic voice leading between chords.
- **Arpeggiator** — 7 patterns (up, down, up-down, down-up, converge, root bounce, random), 4 rates (1/8, 1/8T, 1/16, 1/16T), 1–3 octave range, and gate control.
- **Bass & drums** — optional root-note bass line and a four-on-the-floor groove so previews sound like the real thing.
- **Piano-roll preview** — see the generated chords, arp, and bass on a mini piano roll.
- **In-app playback** — looped MIDI playback so you can audition ideas instantly.
- **MIDI export** — save a format-1 `.mid` file (separate tracks for chords, arp, bass, drums, with tempo and program changes) anywhere on your device, or share it directly to another app.
- **New idea button** — every roll of the dice is a new seeded idea; the same seed always regenerates the same result.

## Download the APK

Every push builds an installable debug APK on GitHub Actions:

1. Open the repo's **Actions** tab and pick the latest **Build APK** run.
2. Download the **ArpForge-debug-apk** artifact and unzip it.
3. Copy `app-debug.apk` to your phone and install it (you may need to allow "install unknown apps" for your browser/file manager).

Requires Android 8.0 (API 26) or newer.

## Project layout

| Module | What it is |
| --- | --- |
| `core/` | Pure Kotlin (JVM) music engine: scales, diatonic chord building, progression + arp/bass/drum generation, and a dependency-free Standard MIDI File writer. Fully unit tested. |
| `app/` | Android app (Jetpack Compose, Material 3): parameter UI, piano-roll canvas, MediaPlayer preview, SAF export and share. |

## Building locally

```bash
./gradlew :core:test          # run the engine tests
./gradlew :app:assembleDebug  # build the APK (needs the Android SDK)
```

The APK lands in `app/build/outputs/apk/debug/app-debug.apk`.
