package com.mannyzeneke.arpforge

import android.app.Application
import android.media.MediaPlayer
import android.net.Uri
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.AndroidViewModel
import com.mannyzeneke.arpforge.core.GenParams
import com.mannyzeneke.arpforge.core.GeneratedPiece
import com.mannyzeneke.arpforge.core.Generator
import com.mannyzeneke.arpforge.core.MidiWriter
import java.io.File
import kotlin.random.Random

class MainViewModel(app: Application) : AndroidViewModel(app) {

    var params by mutableStateOf(GenParams(seed = Random.nextLong()))
        private set

    var piece by mutableStateOf(Generator.generate(params))
        private set

    var isPlaying by mutableStateOf(false)
        private set

    private var player: MediaPlayer? = null

    /** Apply a parameter change and regenerate with the same seed (same idea, new settings). */
    fun update(transform: (GenParams) -> GenParams) {
        val next = transform(params)
        if (next != params) {
            params = next
            regenerate()
        }
    }

    /** Roll a fresh musical idea. */
    fun newIdea() {
        params = params.copy(seed = Random.nextLong())
        regenerate()
    }

    private fun regenerate() {
        val wasPlaying = isPlaying
        stop()
        piece = Generator.generate(params)
        if (wasPlaying) play()
    }

    fun togglePlayback() = if (isPlaying) stop() else play()

    private fun play() {
        stop()
        try {
            val file = File(getApplication<Application>().cacheDir, "preview.mid")
            file.writeBytes(MidiWriter.write(piece))
            player = MediaPlayer().apply {
                setDataSource(file.absolutePath)
                isLooping = true
                prepare()
                start()
            }
            isPlaying = true
        } catch (_: Exception) {
            player?.release()
            player = null
            isPlaying = false
        }
    }

    fun stop() {
        player?.let {
            runCatching { it.stop() }
            it.release()
        }
        player = null
        isPlaying = false
    }

    fun exportTo(uri: Uri): Boolean = runCatching {
        getApplication<Application>().contentResolver.openOutputStream(uri)?.use {
            it.write(MidiWriter.write(piece))
        } ?: error("cannot open $uri")
    }.isSuccess

    /** Writes the MIDI to a shareable cache file and returns it. */
    fun writeShareFile(): File {
        val dir = File(getApplication<Application>().cacheDir, "midi").apply { mkdirs() }
        val file = File(dir, params.suggestedFileName())
        file.writeBytes(MidiWriter.write(piece))
        return file
    }

    override fun onCleared() = stop()
}
