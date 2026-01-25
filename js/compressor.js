/**
 * Compressor - A dynamics compressor using Web Audio API
 * Wraps the native DynamicsCompressorNode with additional features
 */
export class Compressor {
    constructor(audioContext) {
        this.audioContext = audioContext;
        this.bypassed = false;

        // Create nodes
        this.inputGain = audioContext.createGain();
        this.outputGain = audioContext.createGain();
        this.makeupGain = audioContext.createGain();
        this.dryGain = audioContext.createGain();
        this.wetGain = audioContext.createGain();
        this.compressor = audioContext.createDynamicsCompressor();

        // Analyzers for metering
        this.inputAnalyserL = audioContext.createAnalyser();
        this.inputAnalyserR = audioContext.createAnalyser();
        this.outputAnalyserL = audioContext.createAnalyser();
        this.outputAnalyserR = audioContext.createAnalyser();

        // Channel splitters/mergers for stereo metering
        this.inputSplitter = audioContext.createChannelSplitter(2);
        this.outputSplitter = audioContext.createChannelSplitter(2);

        // Configure analyzers
        [this.inputAnalyserL, this.inputAnalyserR, this.outputAnalyserL, this.outputAnalyserR].forEach(analyser => {
            analyser.fftSize = 256;
            analyser.smoothingTimeConstant = 0.8;
        });

        // Default parameter values
        this.params = {
            threshold: -24,  // dB
            ratio: 4,        // :1
            attack: 0.01,    // seconds
            release: 0.1,    // seconds
            knee: 6,         // dB
            makeup: 0,       // dB
            mix: 1           // 0-1 (dry/wet)
        };

        // Connect the signal flow
        this.setupRouting();

        // Apply default parameters
        this.updateAllParams();
    }

    setupRouting() {
        // Input -> splitter for input metering
        this.inputGain.connect(this.inputSplitter);
        this.inputSplitter.connect(this.inputAnalyserL, 0);
        this.inputSplitter.connect(this.inputAnalyserR, 1);

        // Dry path: input -> dryGain -> output
        this.inputGain.connect(this.dryGain);
        this.dryGain.connect(this.outputGain);

        // Wet path: input -> compressor -> makeup -> wetGain -> output
        this.inputGain.connect(this.compressor);
        this.compressor.connect(this.makeupGain);
        this.makeupGain.connect(this.wetGain);
        this.wetGain.connect(this.outputGain);

        // Output -> splitter for output metering
        this.outputGain.connect(this.outputSplitter);
        this.outputSplitter.connect(this.outputAnalyserL, 0);
        this.outputSplitter.connect(this.outputAnalyserR, 1);
    }

    get input() {
        return this.inputGain;
    }

    get output() {
        return this.outputGain;
    }

    updateAllParams() {
        this.setThreshold(this.params.threshold);
        this.setRatio(this.params.ratio);
        this.setAttack(this.params.attack);
        this.setRelease(this.params.release);
        this.setKnee(this.params.knee);
        this.setMakeup(this.params.makeup);
        this.setMix(this.params.mix);
    }

    setThreshold(value) {
        // Range: -100 to 0 dB
        this.params.threshold = Math.max(-100, Math.min(0, value));
        this.compressor.threshold.setValueAtTime(this.params.threshold, this.audioContext.currentTime);
    }

    setRatio(value) {
        // Range: 1 to 20
        this.params.ratio = Math.max(1, Math.min(20, value));
        this.compressor.ratio.setValueAtTime(this.params.ratio, this.audioContext.currentTime);
    }

    setAttack(value) {
        // Range: 0 to 1 second
        this.params.attack = Math.max(0, Math.min(1, value));
        this.compressor.attack.setValueAtTime(this.params.attack, this.audioContext.currentTime);
    }

    setRelease(value) {
        // Range: 0 to 1 second
        this.params.release = Math.max(0, Math.min(1, value));
        this.compressor.release.setValueAtTime(this.params.release, this.audioContext.currentTime);
    }

    setKnee(value) {
        // Range: 0 to 40 dB
        this.params.knee = Math.max(0, Math.min(40, value));
        this.compressor.knee.setValueAtTime(this.params.knee, this.audioContext.currentTime);
    }

    setMakeup(value) {
        // Range: -12 to 24 dB
        this.params.makeup = Math.max(-12, Math.min(24, value));
        const gainLinear = Math.pow(10, this.params.makeup / 20);
        this.makeupGain.gain.setValueAtTime(gainLinear, this.audioContext.currentTime);
    }

    setMix(value) {
        // Range: 0 to 1 (parallel compression)
        this.params.mix = Math.max(0, Math.min(1, value));
        this.wetGain.gain.setValueAtTime(this.params.mix, this.audioContext.currentTime);
        this.dryGain.gain.setValueAtTime(1 - this.params.mix, this.audioContext.currentTime);
    }

    setBypass(bypass) {
        this.bypassed = bypass;
        if (bypass) {
            // Full dry signal
            this.dryGain.gain.setValueAtTime(1, this.audioContext.currentTime);
            this.wetGain.gain.setValueAtTime(0, this.audioContext.currentTime);
        } else {
            // Restore mix setting
            this.setMix(this.params.mix);
        }
    }

    toggleBypass() {
        this.setBypass(!this.bypassed);
        return this.bypassed;
    }

    // Get current gain reduction in dB (negative value)
    getGainReduction() {
        return this.compressor.reduction;
    }

    // Get input level (0-1 range for meter display)
    getInputLevels() {
        const dataL = new Float32Array(this.inputAnalyserL.fftSize);
        const dataR = new Float32Array(this.inputAnalyserR.fftSize);

        this.inputAnalyserL.getFloatTimeDomainData(dataL);
        this.inputAnalyserR.getFloatTimeDomainData(dataR);

        return {
            left: this.calculateRMS(dataL),
            right: this.calculateRMS(dataR)
        };
    }

    // Get output level (0-1 range for meter display)
    getOutputLevels() {
        const dataL = new Float32Array(this.outputAnalyserL.fftSize);
        const dataR = new Float32Array(this.outputAnalyserR.fftSize);

        this.outputAnalyserL.getFloatTimeDomainData(dataL);
        this.outputAnalyserR.getFloatTimeDomainData(dataR);

        return {
            left: this.calculateRMS(dataL),
            right: this.calculateRMS(dataR)
        };
    }

    calculateRMS(data) {
        let sum = 0;
        for (let i = 0; i < data.length; i++) {
            sum += data[i] * data[i];
        }
        const rms = Math.sqrt(sum / data.length);
        // Convert to 0-1 range with some headroom
        return Math.min(1, rms * 2);
    }

    // Get all current parameters
    getParams() {
        return { ...this.params };
    }

    // Set all parameters at once
    setParams(params) {
        if (params.threshold !== undefined) this.setThreshold(params.threshold);
        if (params.ratio !== undefined) this.setRatio(params.ratio);
        if (params.attack !== undefined) this.setAttack(params.attack);
        if (params.release !== undefined) this.setRelease(params.release);
        if (params.knee !== undefined) this.setKnee(params.knee);
        if (params.makeup !== undefined) this.setMakeup(params.makeup);
        if (params.mix !== undefined) this.setMix(params.mix);
    }

    // Disconnect all nodes
    disconnect() {
        this.inputGain.disconnect();
        this.outputGain.disconnect();
        this.compressor.disconnect();
        this.makeupGain.disconnect();
        this.dryGain.disconnect();
        this.wetGain.disconnect();
        this.inputSplitter.disconnect();
        this.outputSplitter.disconnect();
    }
}
