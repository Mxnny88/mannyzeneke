/**
 * Boot Scene
 * Handles initial game setup and asset generation
 */
class BootScene extends Phaser.Scene {
    constructor() {
        super({ key: 'BootScene' });
    }

    preload() {
        // Display loading progress
        const width = this.cameras.main.width;
        const height = this.cameras.main.height;

        // Progress bar
        const progressBar = this.add.graphics();
        const progressBox = this.add.graphics();
        progressBox.fillStyle(0x222222, 0.8);
        progressBox.fillRect(width / 2 - 160, height / 2 - 25, 320, 50);

        // Loading text
        const loadingText = this.add.text(width / 2, height / 2 - 50, 'Loading...', {
            font: '20px "Press Start 2P"',
            fill: '#ffffff'
        }).setOrigin(0.5);

        const percentText = this.add.text(width / 2, height / 2, '0%', {
            font: '18px "Press Start 2P"',
            fill: '#ffffff'
        }).setOrigin(0.5);

        // Update progress
        this.load.on('progress', (value) => {
            progressBar.clear();
            progressBar.fillStyle(0xe94560, 1);
            progressBar.fillRect(width / 2 - 150, height / 2 - 15, 300 * value, 30);
            percentText.setText(parseInt(value * 100) + '%');
        });

        this.load.on('complete', () => {
            progressBar.destroy();
            progressBox.destroy();
            loadingText.destroy();
            percentText.destroy();
        });

        // Load any external assets here if needed
        // For now, we generate everything procedurally
    }

    create() {
        // Initialize sprite generator
        this.spriteGenerator = new SpriteGenerator(this);

        // Generate all fighter sprites
        this.generateAllFighterSprites();

        // Generate UI elements
        this.spriteGenerator.generateHealthBar();
        this.spriteGenerator.generateHitEffects();

        // Generate backgrounds
        this.generateBackgrounds();

        // Generate projectiles
        this.generateProjectiles();

        // Set up audio
        this.setupAudio();

        // Hide HTML loading screen
        const loadingScreen = document.getElementById('loading-screen');
        if (loadingScreen) {
            loadingScreen.classList.add('hidden');
            setTimeout(() => loadingScreen.remove(), 500);
        }

        // Start menu scene
        this.scene.start('MenuScene');
    }

    /**
     * Generate sprites for all fighters
     */
    generateAllFighterSprites() {
        const fighters = [
            Ryu.getInfo(),
            Ken.getInfo(),
            Blaze.getInfo(),
            Shadow.getInfo()
        ];

        fighters.forEach(fighter => {
            this.spriteGenerator.generateFighterSprites({
                name: fighter.name,
                colors: fighter.colors
            });

            // Generate portrait for character select
            this.spriteGenerator.generatePortrait(fighter.name, fighter.colors);
        });

        // Create sprite sheet frame data
        this.createFrameData();
    }

    /**
     * Create frame data for all animations
     */
    createFrameData() {
        const fighters = ['ryu', 'ken', 'blaze', 'shadow'];
        const animations = [
            { name: 'idle', frames: 4, width: 120 },
            { name: 'walk', frames: 6, width: 120 },
            { name: 'jump', frames: 3, width: 120 },
            { name: 'punch', frames: 4, width: 140 },
            { name: 'kick', frames: 4, width: 160 },
            { name: 'special', frames: 6, width: 160 },
            { name: 'hit', frames: 3, width: 120 },
            { name: 'block', frames: 2, width: 120 },
            { name: 'ko', frames: 5, width: 140 },
            { name: 'victory', frames: 4, width: 120 }
        ];

        fighters.forEach(fighter => {
            animations.forEach(anim => {
                const key = `${fighter}_${anim.name}`;
                if (this.textures.exists(key)) {
                    // Add frame data
                    const texture = this.textures.get(key);
                    const frameWidth = anim.width;
                    const frameHeight = 140;

                    // Remove existing frames if any
                    texture.getFrameNames().forEach(frameName => {
                        if (frameName !== '__BASE') {
                            texture.remove(frameName);
                        }
                    });

                    // Add numbered frames
                    for (let i = 0; i < anim.frames; i++) {
                        texture.add(i, 0, i * frameWidth, 0, frameWidth, frameHeight);
                    }
                }
            });
        });
    }

    /**
     * Generate background stages
     */
    generateBackgrounds() {
        // City Night stage
        this.spriteGenerator.generateBackground('bg_city', {
            skyTop: '#0a0a1a',
            skyBottom: '#1a1a3a',
            buildingColor: '#0f0f23',
            windowColor: '#ffff44',
            groundTop: '#2d2d44',
            groundBottom: '#1a1a2e',
            groundLine: '#e94560',
            groundDetail: '#3d3d5c'
        });

        // Dojo stage
        this.spriteGenerator.generateBackground('bg_dojo', {
            skyTop: '#2d1810',
            skyBottom: '#4a2820',
            buildingColor: '#1a0f0a',
            windowColor: '#ff9944',
            groundTop: '#3d2820',
            groundBottom: '#2d1810',
            groundLine: '#cc6600',
            groundDetail: '#4d3830'
        });

        // Arena stage
        this.spriteGenerator.generateBackground('bg_arena', {
            skyTop: '#1a1a2e',
            skyBottom: '#2a2a4e',
            buildingColor: '#0f0f1e',
            windowColor: '#00ffff',
            groundTop: '#2a2a4e',
            groundBottom: '#1a1a2e',
            groundLine: '#00ffff',
            groundDetail: '#3a3a5e'
        });
    }

    /**
     * Generate projectile sprites
     */
    generateProjectiles() {
        this.spriteGenerator.generateProjectile('ryu', '#00aaff');
        this.spriteGenerator.generateProjectile('ken', '#ff4400');
        this.spriteGenerator.generateProjectile('blaze', '#ff0000');
        this.spriteGenerator.generateProjectile('shadow', '#9400d3');
    }

    /**
     * Set up game audio using Howler.js
     */
    setupAudio() {
        // Create sound effects using Web Audio API synthesis
        const audioContext = new (window.AudioContext || window.webkitAudioContext)();

        // Helper to create synthetic sounds
        const createSound = (frequency, duration, type = 'sine', volume = 0.3) => {
            return () => {
                const oscillator = audioContext.createOscillator();
                const gainNode = audioContext.createGain();

                oscillator.connect(gainNode);
                gainNode.connect(audioContext.destination);

                oscillator.type = type;
                oscillator.frequency.setValueAtTime(frequency, audioContext.currentTime);

                gainNode.gain.setValueAtTime(volume, audioContext.currentTime);
                gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + duration);

                oscillator.start(audioContext.currentTime);
                oscillator.stop(audioContext.currentTime + duration);
            };
        };

        // Store sound functions
        this.game.soundEffects = {
            punch: createSound(200, 0.1, 'square', 0.4),
            kick: createSound(150, 0.15, 'square', 0.4),
            hit: createSound(100, 0.2, 'sawtooth', 0.3),
            block: createSound(400, 0.1, 'triangle', 0.2),
            special: createSound(300, 0.3, 'sine', 0.5),
            jump: createSound(500, 0.1, 'sine', 0.2),
            ko: createSound(80, 0.5, 'sawtooth', 0.6),
            select: createSound(600, 0.1, 'sine', 0.3),
            confirm: createSound(800, 0.15, 'sine', 0.3),
            victory: createSound(523.25, 0.5, 'sine', 0.4),
            round: createSound(440, 0.3, 'sine', 0.4),
            fight: createSound(349.23, 0.4, 'square', 0.5)
        };

        // Override Phaser's sound.play to use our synthetic sounds
        this.game.soundPlay = (key, config = {}) => {
            const volume = config.volume || 1;
            if (this.game.soundEffects[key]) {
                try {
                    // Resume audio context if suspended
                    if (audioContext.state === 'suspended') {
                        audioContext.resume();
                    }
                    this.game.soundEffects[key]();
                } catch (e) {
                    console.log('Audio error:', e);
                }
            }
        };
    }
}

// Export
if (typeof window !== 'undefined') {
    window.BootScene = BootScene;
}
