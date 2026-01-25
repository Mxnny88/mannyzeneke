# Compressor Plugin

A web-based audio dynamics compressor plugin built with the Web Audio API. Features a professional-looking UI with real-time metering and visualization.

## Features

- **Full Compressor Controls**
  - Threshold (-60dB to 0dB)
  - Ratio (1:1 to 20:1)
  - Attack (1ms to 500ms)
  - Release (10ms to 1000ms)
  - Knee (0dB to 30dB soft knee)
  - Makeup Gain (-12dB to +24dB)
  - Mix (0-100% for parallel compression)

- **Real-time Visualization**
  - Stereo input level meters
  - Stereo output level meters
  - Gain reduction meter
  - Transfer curve display

- **Audio Sources**
  - Load audio files (WAV, MP3, etc.)
  - Live microphone input

- **Additional Features**
  - Bypass toggle
  - Interactive knob controls (drag to adjust, double-click to reset)
  - Responsive design

## Getting Started

### Running Locally

1. Clone the repository
2. Start a local web server:

```bash
# Using npm
npm install
npm start

# Or using Python
python -m http.server 8000

# Or using PHP
php -S localhost:8000
```

3. Open your browser to `http://localhost:8000` (or the appropriate port)

### Usage

1. **Load Audio**: Click "Load Audio File" to select an audio file, or click "Use Microphone" for live input
2. **Play**: Use the transport controls to play/stop the audio
3. **Adjust Parameters**: Drag the knobs up/down to adjust compressor settings
4. **Reset**: Double-click any knob to reset it to its default value
5. **Bypass**: Click the power button to bypass the compressor

## Controls

| Control | Range | Description |
|---------|-------|-------------|
| Threshold | -60 to 0 dB | Level above which compression begins |
| Ratio | 1:1 to 20:1 | Amount of gain reduction applied |
| Attack | 1-500 ms | How quickly compression engages |
| Release | 10-1000 ms | How quickly compression releases |
| Knee | 0-30 dB | Soft knee width for gradual compression onset |
| Makeup | -12 to +24 dB | Output gain to compensate for compression |
| Mix | 0-100% | Dry/wet blend for parallel compression |

## Technical Details

Built using:
- **Web Audio API** - Native browser audio processing
- **DynamicsCompressorNode** - Hardware-accelerated dynamics processing
- **AnalyserNode** - Real-time audio analysis for metering
- **Canvas API** - Transfer curve visualization

## Browser Support

Works in all modern browsers that support the Web Audio API:
- Chrome
- Firefox
- Safari
- Edge

## License

MIT License
