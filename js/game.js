/**
 * Street Brawler - 2D Fighting Game
 * Main game configuration and initialization
 */

// Wait for DOM to be ready
document.addEventListener('DOMContentLoaded', () => {
    // Phaser game configuration
    const config = {
        type: Phaser.AUTO,
        width: 1024,
        height: 576,
        parent: 'game-container',
        backgroundColor: '#1a1a2e',
        pixelArt: true,
        physics: {
            default: 'arcade',
            arcade: {
                gravity: { y: 0 },
                debug: false
            }
        },
        scene: [
            BootScene,
            MenuScene,
            CharacterSelectScene,
            FightScene,
            GameOverScene
        ],
        scale: {
            mode: Phaser.Scale.FIT,
            autoCenter: Phaser.Scale.CENTER_BOTH
        },
        audio: {
            disableWebAudio: false
        }
    };

    // Create game instance
    const game = new Phaser.Game(config);

    // Extend Phaser.Scene to add custom sound play method
    Phaser.Scene.prototype.playSound = function(key, config) {
        if (this.game.soundPlay) {
            this.game.soundPlay(key, config);
        }
    };

    // Override the sound manager play method for all scenes
    const originalPlay = Phaser.Sound.BaseSoundManager.prototype.play;
    Phaser.Sound.BaseSoundManager.prototype.play = function(key, config) {
        // Use our custom sound system
        if (this.game && this.game.soundPlay) {
            this.game.soundPlay(key, config);
            return true;
        }
        return originalPlay.call(this, key, config);
    };

    // Handle window focus/blur for audio context
    window.addEventListener('click', () => {
        // Resume audio context on user interaction
        if (window.AudioContext || window.webkitAudioContext) {
            const ctx = new (window.AudioContext || window.webkitAudioContext)();
            if (ctx.state === 'suspended') {
                ctx.resume();
            }
        }
    });

    // Prevent default browser behaviors during game
    window.addEventListener('keydown', (e) => {
        // Prevent arrow keys from scrolling
        if (['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'Space'].includes(e.code)) {
            e.preventDefault();
        }
    });

    // Log game info
    console.log('%c STREET BRAWLER ', 'background: #e94560; color: white; font-size: 20px; font-weight: bold;');
    console.log('%c 2D Fighting Game ', 'background: #1a1a2e; color: #00ffff; font-size: 14px;');
    console.log('%c Built with Phaser 3 ', 'color: #888;');
});

/**
 * Game Features:
 *
 * - 4 Unique Fighters: Ryu (Balanced), Ken (Speed), Blaze (Power), Shadow (Technical)
 * - VS Mode: 2-player local multiplayer
 * - VS CPU: Single player against AI opponent
 * - Round-based combat with best of 3 rounds
 * - Special moves unique to each character
 * - Combo system
 * - Health bars and timer
 * - Procedurally generated pixel-art sprites
 * - Synthesized sound effects
 *
 * Controls:
 *
 * Player 1:
 * - Movement: W A S D
 * - Punch: F
 * - Kick: G
 * - Block: H
 * - Special: R
 *
 * Player 2:
 * - Movement: Arrow Keys
 * - Punch: J
 * - Kick: K
 * - Block: L
 * - Special: U
 *
 * General:
 * - Pause: ESC
 */
