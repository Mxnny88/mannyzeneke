import { Compressor } from './compressor.js';

/**
 * Compressor Plugin Application
 */
class CompressorApp {
    constructor() {
        this.audioContext = null;
        this.compressor = null;
        this.sourceNode = null;
        this.mediaStream = null;
        this.audioBuffer = null;
        this.isPlaying = false;
        this.isMicActive = false;
        this.animationFrameId = null;

        // Knob configuration (parameter ranges)
        this.knobConfig = {
            threshold: { min: -60, max: 0, default: -24, unit: 'dB', decimals: 0 },
            ratio: { min: 1, max: 20, default: 4, unit: ':1', decimals: 1 },
            attack: { min: 0.001, max: 0.5, default: 0.01, unit: 'ms', multiplier: 1000, decimals: 0 },
            release: { min: 0.01, max: 1, default: 0.1, unit: 'ms', multiplier: 1000, decimals: 0 },
            knee: { min: 0, max: 30, default: 6, unit: 'dB', decimals: 0 },
            makeup: { min: -12, max: 24, default: 0, unit: 'dB', decimals: 1 },
            mix: { min: 0, max: 100, default: 100, unit: '%', decimals: 0 }
        };

        this.knobValues = {};
        this.activeKnob = null;
        this.lastY = 0;

        this.init();
    }

    async init() {
        // Initialize knob values
        for (const [param, config] of Object.entries(this.knobConfig)) {
            this.knobValues[param] = config.default;
        }

        this.setupEventListeners();
        this.updateAllKnobDisplays();
        this.drawTransferCurve();
    }

    async initAudio() {
        if (this.audioContext) return;

        this.audioContext = new (window.AudioContext || window.webkitAudioContext)();
        this.compressor = new Compressor(this.audioContext);
        this.compressor.output.connect(this.audioContext.destination);

        // Apply current knob values
        this.syncCompressorParams();

        // Start metering
        this.startMetering();
    }

    syncCompressorParams() {
        if (!this.compressor) return;

        this.compressor.setParams({
            threshold: this.knobValues.threshold,
            ratio: this.knobValues.ratio,
            attack: this.knobValues.attack / 1000, // Convert ms to seconds
            release: this.knobValues.release / 1000,
            knee: this.knobValues.knee,
            makeup: this.knobValues.makeup,
            mix: this.knobValues.mix / 100 // Convert percentage to 0-1
        });
    }

    setupEventListeners() {
        // Bypass button
        document.getElementById('bypass-btn').addEventListener('click', () => this.toggleBypass());

        // Audio file input
        document.getElementById('load-audio-btn').addEventListener('click', () => {
            document.getElementById('audio-file-input').click();
        });

        document.getElementById('audio-file-input').addEventListener('change', (e) => {
            this.loadAudioFile(e.target.files[0]);
        });

        // Microphone button
        document.getElementById('mic-btn').addEventListener('click', () => this.toggleMicrophone());

        // Transport controls
        document.getElementById('play-btn').addEventListener('click', () => this.togglePlayback());
        document.getElementById('stop-btn').addEventListener('click', () => this.stop());

        // Knob interactions
        document.querySelectorAll('.knob').forEach(knob => {
            knob.addEventListener('mousedown', (e) => this.startKnobDrag(e));
            knob.addEventListener('dblclick', (e) => this.resetKnob(e));
        });

        // Global mouse events for knob dragging
        document.addEventListener('mousemove', (e) => this.handleKnobDrag(e));
        document.addEventListener('mouseup', () => this.stopKnobDrag());

        // Touch support for knobs
        document.querySelectorAll('.knob').forEach(knob => {
            knob.addEventListener('touchstart', (e) => this.startKnobDrag(e), { passive: false });
        });
        document.addEventListener('touchmove', (e) => this.handleKnobDrag(e), { passive: false });
        document.addEventListener('touchend', () => this.stopKnobDrag());
    }

    // Knob interaction methods
    startKnobDrag(e) {
        e.preventDefault();
        this.activeKnob = e.target.closest('.knob');
        if (e.touches) {
            this.lastY = e.touches[0].clientY;
        } else {
            this.lastY = e.clientY;
        }
        this.activeKnob.classList.add('dragging');
    }

    handleKnobDrag(e) {
        if (!this.activeKnob) return;
        e.preventDefault();

        const currentY = e.touches ? e.touches[0].clientY : e.clientY;
        const delta = this.lastY - currentY;
        this.lastY = currentY;

        const param = this.activeKnob.dataset.param;
        const config = this.knobConfig[param];

        // Calculate new value
        const range = config.max - config.min;
        const sensitivity = range / 200; // Full range over 200 pixels
        let newValue = this.knobValues[param] + delta * sensitivity;

        // Clamp to range
        newValue = Math.max(config.min, Math.min(config.max, newValue));
        this.knobValues[param] = newValue;

        // Update display and compressor
        this.updateKnobDisplay(param);
        this.updateKnobRotation(param);

        // Update compressor parameter
        if (this.compressor) {
            this.updateCompressorParam(param);
        }

        // Redraw transfer curve for threshold, ratio, and knee changes
        if (['threshold', 'ratio', 'knee'].includes(param)) {
            this.drawTransferCurve();
        }
    }

    stopKnobDrag() {
        if (this.activeKnob) {
            this.activeKnob.classList.remove('dragging');
            this.activeKnob = null;
        }
    }

    resetKnob(e) {
        const knob = e.target.closest('.knob');
        const param = knob.dataset.param;
        const config = this.knobConfig[param];

        this.knobValues[param] = config.default;
        this.updateKnobDisplay(param);
        this.updateKnobRotation(param);

        if (this.compressor) {
            this.updateCompressorParam(param);
        }

        if (['threshold', 'ratio', 'knee'].includes(param)) {
            this.drawTransferCurve();
        }
    }

    updateCompressorParam(param) {
        const value = this.knobValues[param];

        switch (param) {
            case 'threshold':
                this.compressor.setThreshold(value);
                break;
            case 'ratio':
                this.compressor.setRatio(value);
                break;
            case 'attack':
                this.compressor.setAttack(value / 1000);
                break;
            case 'release':
                this.compressor.setRelease(value / 1000);
                break;
            case 'knee':
                this.compressor.setKnee(value);
                break;
            case 'makeup':
                this.compressor.setMakeup(value);
                break;
            case 'mix':
                this.compressor.setMix(value / 100);
                break;
        }
    }

    updateKnobDisplay(param) {
        const config = this.knobConfig[param];
        const value = this.knobValues[param];
        const displayEl = document.getElementById(`${param}-value`);

        let displayValue = value;
        if (config.multiplier) {
            displayValue = value;
        }

        const formatted = displayValue.toFixed(config.decimals);
        displayEl.textContent = `${formatted} ${config.unit}`;
    }

    updateKnobRotation(param) {
        const config = this.knobConfig[param];
        const value = this.knobValues[param];
        const normalized = (value - config.min) / (config.max - config.min);

        // Rotation from -135 to 135 degrees
        const rotation = -135 + normalized * 270;

        const knob = document.getElementById(`${param}-knob`);
        knob.style.transform = `rotate(${rotation}deg)`;
    }

    updateAllKnobDisplays() {
        for (const param of Object.keys(this.knobConfig)) {
            this.updateKnobDisplay(param);
            this.updateKnobRotation(param);
        }
    }

    // Bypass
    toggleBypass() {
        const btn = document.getElementById('bypass-btn');
        btn.classList.toggle('active');

        if (this.compressor) {
            this.compressor.toggleBypass();
        }
    }

    // Audio loading
    async loadAudioFile(file) {
        if (!file) return;

        await this.initAudio();

        try {
            const arrayBuffer = await file.arrayBuffer();
            this.audioBuffer = await this.audioContext.decodeAudioData(arrayBuffer);

            document.getElementById('file-info').textContent = file.name;
            document.getElementById('play-btn').disabled = false;
            document.getElementById('stop-btn').disabled = false;
            document.getElementById('mic-btn').classList.remove('active');

            // Stop microphone if active
            if (this.isMicActive) {
                this.stopMicrophone();
            }

            this.stop();
        } catch (err) {
            console.error('Error loading audio file:', err);
            document.getElementById('file-info').textContent = 'Error loading file';
        }
    }

    // Microphone
    async toggleMicrophone() {
        if (this.isMicActive) {
            this.stopMicrophone();
        } else {
            await this.startMicrophone();
        }
    }

    async startMicrophone() {
        await this.initAudio();

        try {
            this.mediaStream = await navigator.mediaDevices.getUserMedia({ audio: true });
            this.sourceNode = this.audioContext.createMediaStreamSource(this.mediaStream);
            this.sourceNode.connect(this.compressor.input);

            this.isMicActive = true;
            document.getElementById('mic-btn').classList.add('active');
            document.getElementById('file-info').textContent = 'Microphone active';
            document.getElementById('play-btn').disabled = true;
            document.getElementById('stop-btn').disabled = true;

        } catch (err) {
            console.error('Error accessing microphone:', err);
            document.getElementById('file-info').textContent = 'Microphone access denied';
        }
    }

    stopMicrophone() {
        if (this.sourceNode) {
            this.sourceNode.disconnect();
            this.sourceNode = null;
        }

        if (this.mediaStream) {
            this.mediaStream.getTracks().forEach(track => track.stop());
            this.mediaStream = null;
        }

        this.isMicActive = false;
        document.getElementById('mic-btn').classList.remove('active');

        if (this.audioBuffer) {
            document.getElementById('file-info').textContent = 'Audio file loaded';
            document.getElementById('play-btn').disabled = false;
            document.getElementById('stop-btn').disabled = false;
        } else {
            document.getElementById('file-info').textContent = 'No audio loaded';
        }
    }

    // Playback
    togglePlayback() {
        if (this.isPlaying) {
            this.pause();
        } else {
            this.play();
        }
    }

    async play() {
        if (!this.audioBuffer || this.isPlaying) return;

        await this.initAudio();

        if (this.audioContext.state === 'suspended') {
            await this.audioContext.resume();
        }

        this.sourceNode = this.audioContext.createBufferSource();
        this.sourceNode.buffer = this.audioBuffer;
        this.sourceNode.connect(this.compressor.input);

        this.sourceNode.onended = () => {
            if (this.isPlaying) {
                this.stop();
            }
        };

        this.sourceNode.start(0);
        this.isPlaying = true;

        document.getElementById('play-btn').classList.add('playing');
        document.getElementById('play-btn').querySelector('.play-icon').textContent = '⏸';
    }

    pause() {
        // Web Audio API doesn't support true pause, so we stop
        this.stop();
    }

    stop() {
        if (this.sourceNode && !this.isMicActive) {
            try {
                this.sourceNode.stop();
            } catch (e) {
                // Ignore if already stopped
            }
            this.sourceNode.disconnect();
            this.sourceNode = null;
        }

        this.isPlaying = false;
        document.getElementById('play-btn').classList.remove('playing');
        document.getElementById('play-btn').querySelector('.play-icon').textContent = '▶';
    }

    // Metering
    startMetering() {
        const updateMeters = () => {
            if (!this.compressor) return;

            // Input meters
            const inputLevels = this.compressor.getInputLevels();
            document.getElementById('input-meter-l').style.height = `${inputLevels.left * 100}%`;
            document.getElementById('input-meter-r').style.height = `${inputLevels.right * 100}%`;

            // Output meters
            const outputLevels = this.compressor.getOutputLevels();
            document.getElementById('output-meter-l').style.height = `${outputLevels.left * 100}%`;
            document.getElementById('output-meter-r').style.height = `${outputLevels.right * 100}%`;

            // Gain reduction meter
            const gr = this.compressor.getGainReduction();
            // GR is negative, normalize to 0-1 range (assuming max GR of -30dB)
            const grNormalized = Math.min(1, Math.abs(gr) / 30);
            document.getElementById('gr-meter').style.width = `${grNormalized * 100}%`;

            this.animationFrameId = requestAnimationFrame(updateMeters);
        };

        updateMeters();
    }

    stopMetering() {
        if (this.animationFrameId) {
            cancelAnimationFrame(this.animationFrameId);
            this.animationFrameId = null;
        }
    }

    // Transfer curve visualization
    drawTransferCurve() {
        const canvas = document.getElementById('transfer-curve');
        const ctx = canvas.getContext('2d');
        const width = canvas.width;
        const height = canvas.height;

        // Clear canvas
        ctx.fillStyle = '#1a1a2e';
        ctx.fillRect(0, 0, width, height);

        // Draw grid
        ctx.strokeStyle = '#2d2d4a';
        ctx.lineWidth = 1;

        // Grid lines
        for (let i = 0; i <= 4; i++) {
            const pos = (i / 4) * width;
            ctx.beginPath();
            ctx.moveTo(pos, 0);
            ctx.lineTo(pos, height);
            ctx.stroke();
            ctx.beginPath();
            ctx.moveTo(0, pos);
            ctx.lineTo(width, pos);
            ctx.stroke();
        }

        // Draw 1:1 reference line
        ctx.strokeStyle = '#3a3a5c';
        ctx.beginPath();
        ctx.moveTo(0, height);
        ctx.lineTo(width, 0);
        ctx.stroke();

        // Get compressor parameters
        const threshold = this.knobValues.threshold;
        const ratio = this.knobValues.ratio;
        const knee = this.knobValues.knee;

        // Draw transfer curve
        ctx.strokeStyle = '#00d4ff';
        ctx.lineWidth = 2;
        ctx.beginPath();

        const dbMin = -60;
        const dbMax = 0;
        const dbRange = dbMax - dbMin;

        for (let x = 0; x <= width; x++) {
            // Input level in dB
            const inputDb = dbMin + (x / width) * dbRange;

            // Calculate output level based on compressor curve
            let outputDb;

            if (knee > 0) {
                // Soft knee calculation
                const kneeStart = threshold - knee / 2;
                const kneeEnd = threshold + knee / 2;

                if (inputDb < kneeStart) {
                    // Below knee - no compression
                    outputDb = inputDb;
                } else if (inputDb > kneeEnd) {
                    // Above knee - full compression
                    outputDb = threshold + (inputDb - threshold) / ratio;
                } else {
                    // In knee region - gradual compression
                    const kneeRatio = (inputDb - kneeStart) / knee;
                    const currentRatio = 1 + (ratio - 1) * kneeRatio;
                    outputDb = kneeStart + (inputDb - kneeStart) / currentRatio;
                }
            } else {
                // Hard knee
                if (inputDb < threshold) {
                    outputDb = inputDb;
                } else {
                    outputDb = threshold + (inputDb - threshold) / ratio;
                }
            }

            // Convert to canvas coordinates
            const y = height - ((outputDb - dbMin) / dbRange) * height;

            if (x === 0) {
                ctx.moveTo(x, y);
            } else {
                ctx.lineTo(x, y);
            }
        }

        ctx.stroke();

        // Draw threshold marker
        const thresholdX = ((threshold - dbMin) / dbRange) * width;
        ctx.strokeStyle = '#ff6b35';
        ctx.lineWidth = 1;
        ctx.setLineDash([4, 4]);
        ctx.beginPath();
        ctx.moveTo(thresholdX, 0);
        ctx.lineTo(thresholdX, height);
        ctx.stroke();
        ctx.setLineDash([]);

        // Draw labels
        ctx.fillStyle = '#6a6a80';
        ctx.font = '10px sans-serif';
        ctx.fillText('0dB', width - 25, 12);
        ctx.fillText('-60dB', 2, height - 4);
        ctx.fillText('IN', width - 15, height - 4);

        // Rotated label for output
        ctx.save();
        ctx.translate(12, 25);
        ctx.rotate(-Math.PI / 2);
        ctx.fillText('OUT', 0, 0);
        ctx.restore();
    }
}

// Initialize the app when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    window.compressorApp = new CompressorApp();
});
