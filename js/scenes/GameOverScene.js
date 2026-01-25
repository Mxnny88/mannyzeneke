/**
 * Game Over Scene
 * Shows match results and options
 */
class GameOverScene extends Phaser.Scene {
    constructor() {
        super({ key: 'GameOverScene' });
    }

    init(data) {
        this.winner = data.winner || 1;
        this.p1Fighter = data.p1Fighter || 'ryu';
        this.p2Fighter = data.p2Fighter || 'ken';
        this.p1Wins = data.p1Wins || 0;
        this.p2Wins = data.p2Wins || 0;
        this.gameMode = data.mode || 'vs';
    }

    create() {
        const width = this.cameras.main.width;
        const height = this.cameras.main.height;

        // Background
        this.add.image(width / 2, height / 2, 'bg_arena').setAlpha(0.3);

        const overlay = this.add.graphics();
        overlay.fillGradientStyle(0x1a1a2e, 0x1a1a2e, 0x0a0a1a, 0x0a0a1a, 0.85);
        overlay.fillRect(0, 0, width, height);

        // Winner display
        const winnerName = this.winner === 1 ? this.p1Fighter.toUpperCase() : this.p2Fighter.toUpperCase();
        const winnerColor = this.winner === 1 ? '#00ffff' : '#ff6b8a';

        // Victory text with animation
        const victoryText = this.add.text(width / 2, 100, 'VICTORY!', {
            fontFamily: '"Press Start 2P"',
            fontSize: '48px',
            fill: '#e94560'
        }).setOrigin(0.5).setAlpha(0);

        this.tweens.add({
            targets: victoryText,
            alpha: 1,
            y: 120,
            duration: 500,
            ease: 'Power2'
        });

        // Winner name
        const winnerText = this.add.text(width / 2, 180, winnerName, {
            fontFamily: '"Press Start 2P"',
            fontSize: '36px',
            fill: winnerColor
        }).setOrigin(0.5).setAlpha(0);

        this.tweens.add({
            targets: winnerText,
            alpha: 1,
            duration: 500,
            delay: 300
        });

        // Animated glow effect on winner name
        this.tweens.add({
            targets: winnerText,
            alpha: 0.7,
            duration: 800,
            yoyo: true,
            repeat: -1,
            delay: 800
        });

        // Winner portrait
        const portraitKey = this.winner === 1 ? `${this.p1Fighter}_portrait` : `${this.p2Fighter}_portrait`;
        const portrait = this.add.image(width / 2, 320, portraitKey).setScale(1.5).setAlpha(0);

        this.tweens.add({
            targets: portrait,
            alpha: 1,
            scale: 1.6,
            duration: 500,
            delay: 500
        });

        // Score display
        const scoreText = this.add.text(width / 2, 450, `${this.p1Wins} - ${this.p2Wins}`, {
            fontFamily: '"Press Start 2P"',
            fontSize: '28px',
            fill: '#ffffff'
        }).setOrigin(0.5).setAlpha(0);

        this.tweens.add({
            targets: scoreText,
            alpha: 1,
            duration: 500,
            delay: 700
        });

        // Menu options
        this.menuOptions = [
            { text: 'REMATCH', action: () => this.rematch() },
            { text: 'CHARACTER SELECT', action: () => this.characterSelect() },
            { text: 'MAIN MENU', action: () => this.mainMenu() }
        ];

        this.selectedIndex = 0;
        this.menuTexts = [];

        this.menuOptions.forEach((option, index) => {
            const y = 500 + index * 40;

            const text = this.add.text(width / 2, y, option.text, {
                fontFamily: '"Press Start 2P"',
                fontSize: '16px',
                fill: '#ffffff'
            }).setOrigin(0.5).setAlpha(0);

            this.tweens.add({
                targets: text,
                alpha: 1,
                duration: 300,
                delay: 900 + index * 100
            });

            text.setInteractive({ useHandCursor: true });

            text.on('pointerover', () => {
                this.selectedIndex = index;
                this.updateSelection();
                this.playSound('select');
            });

            text.on('pointerdown', () => {
                this.playSound('confirm');
                option.action();
            });

            this.menuTexts.push(text);
        });

        // Setup controls after delay
        this.time.delayedCall(1000, () => {
            this.setupControls();
            this.updateSelection();
        });

        // Confetti effect for winner
        this.createConfetti(winnerColor);

        // Fade in
        this.cameras.main.fadeIn(500);
    }

    /**
     * Create confetti particle effect
     */
    createConfetti(color) {
        const width = this.cameras.main.width;

        for (let i = 0; i < 50; i++) {
            this.time.delayedCall(i * 30, () => {
                const confetti = this.add.rectangle(
                    Phaser.Math.Between(0, width),
                    -20,
                    Phaser.Math.Between(8, 15),
                    Phaser.Math.Between(8, 15),
                    Phaser.Display.Color.HexStringToColor(
                        Phaser.Utils.Array.GetRandom(['#e94560', '#00ffff', '#ffff00', '#ff6b8a', '#ffffff'])
                    ).color
                );

                this.tweens.add({
                    targets: confetti,
                    y: 600,
                    x: confetti.x + Phaser.Math.Between(-100, 100),
                    rotation: Phaser.Math.Between(0, 10),
                    duration: Phaser.Math.Between(2000, 4000),
                    onComplete: () => confetti.destroy()
                });
            });
        }
    }

    /**
     * Setup keyboard controls
     */
    setupControls() {
        this.cursors = this.input.keyboard.createCursorKeys();
        this.enterKey = this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.ENTER);
        this.spaceKey = this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.SPACE);
        this.canInput = true;
    }

    update() {
        if (!this.canInput || !this.cursors) return;

        if (Phaser.Input.Keyboard.JustDown(this.cursors.up)) {
            this.selectedIndex = (this.selectedIndex - 1 + this.menuOptions.length) % this.menuOptions.length;
            this.updateSelection();
            this.playSound('select');
        }

        if (Phaser.Input.Keyboard.JustDown(this.cursors.down)) {
            this.selectedIndex = (this.selectedIndex + 1) % this.menuOptions.length;
            this.updateSelection();
            this.playSound('select');
        }

        if (Phaser.Input.Keyboard.JustDown(this.enterKey) ||
            Phaser.Input.Keyboard.JustDown(this.spaceKey)) {
            this.playSound('confirm');
            this.menuOptions[this.selectedIndex].action();
        }
    }

    /**
     * Update visual selection
     */
    updateSelection() {
        this.menuTexts.forEach((text, index) => {
            if (index === this.selectedIndex) {
                text.setFill('#e94560');
                text.setScale(1.1);
            } else {
                text.setFill('#ffffff');
                text.setScale(1);
            }
        });
    }

    /**
     * Rematch with same characters
     */
    rematch() {
        this.canInput = false;
        this.cameras.main.fadeOut(500);
        this.time.delayedCall(500, () => {
            this.scene.start('FightScene', {
                mode: this.gameMode,
                p1Fighter: this.p1Fighter,
                p2Fighter: this.p2Fighter
            });
        });
    }

    /**
     * Go to character select
     */
    characterSelect() {
        this.canInput = false;
        this.cameras.main.fadeOut(500);
        this.time.delayedCall(500, () => {
            this.scene.start('CharacterSelectScene', { mode: this.gameMode });
        });
    }

    /**
     * Return to main menu
     */
    mainMenu() {
        this.canInput = false;
        this.cameras.main.fadeOut(500);
        this.time.delayedCall(500, () => {
            this.scene.start('MenuScene');
        });
    }

    /**
     * Play sound effect
     */
    playSound(key) {
        if (this.game.soundPlay) {
            this.game.soundPlay(key);
        }
    }
}

if (typeof window !== 'undefined') {
    window.GameOverScene = GameOverScene;
}
