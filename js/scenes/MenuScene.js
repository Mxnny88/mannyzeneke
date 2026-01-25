/**
 * Menu Scene
 * Main menu with game options
 */
class MenuScene extends Phaser.Scene {
    constructor() {
        super({ key: 'MenuScene' });
    }

    create() {
        const width = this.cameras.main.width;
        const height = this.cameras.main.height;

        // Background
        this.add.image(width / 2, height / 2, 'bg_arena').setAlpha(0.5);

        // Overlay gradient
        const overlay = this.add.graphics();
        overlay.fillGradientStyle(0x1a1a2e, 0x1a1a2e, 0x0a0a1a, 0x0a0a1a, 0.8);
        overlay.fillRect(0, 0, width, height);

        // Title with glow effect
        const titleShadow = this.add.text(width / 2 + 4, 104, 'STREET BRAWLER', {
            fontFamily: '"Press Start 2P"',
            fontSize: '48px',
            fill: '#000000'
        }).setOrigin(0.5);

        const title = this.add.text(width / 2, 100, 'STREET BRAWLER', {
            fontFamily: '"Press Start 2P"',
            fontSize: '48px',
            fill: '#e94560'
        }).setOrigin(0.5);

        // Animated title glow
        this.tweens.add({
            targets: title,
            alpha: 0.7,
            duration: 1000,
            yoyo: true,
            repeat: -1
        });

        // Subtitle
        this.add.text(width / 2, 160, 'THE ULTIMATE SHOWDOWN', {
            fontFamily: '"Press Start 2P"',
            fontSize: '14px',
            fill: '#ffffff'
        }).setOrigin(0.5);

        // Menu options
        this.menuOptions = [
            { text: 'VS MODE', action: () => this.startVsMode() },
            { text: 'VS CPU', action: () => this.startVsCPU() },
            { text: 'CONTROLS', action: () => this.showControls() },
            { text: 'CREDITS', action: () => this.showCredits() }
        ];

        this.selectedIndex = 0;
        this.menuTexts = [];

        this.menuOptions.forEach((option, index) => {
            const y = 280 + index * 60;

            const text = this.add.text(width / 2, y, option.text, {
                fontFamily: '"Press Start 2P"',
                fontSize: '24px',
                fill: '#ffffff'
            }).setOrigin(0.5);

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

        // Initial selection
        this.updateSelection();

        // Selector arrow
        this.selector = this.add.text(0, 0, '>', {
            fontFamily: '"Press Start 2P"',
            fontSize: '24px',
            fill: '#e94560'
        }).setOrigin(0.5);

        // Animate selector
        this.tweens.add({
            targets: this.selector,
            x: '+=10',
            duration: 300,
            yoyo: true,
            repeat: -1
        });

        this.updateSelectorPosition();

        // Instructions
        this.add.text(width / 2, height - 60, 'USE ARROW KEYS TO SELECT', {
            fontFamily: '"Press Start 2P"',
            fontSize: '12px',
            fill: '#888888'
        }).setOrigin(0.5);

        this.add.text(width / 2, height - 35, 'PRESS ENTER TO CONFIRM', {
            fontFamily: '"Press Start 2P"',
            fontSize: '12px',
            fill: '#888888'
        }).setOrigin(0.5);

        // Keyboard controls
        this.cursors = this.input.keyboard.createCursorKeys();
        this.enterKey = this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.ENTER);
        this.spaceKey = this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.SPACE);

        // Key debounce
        this.canInput = true;

        // Decorative fighters
        this.addDecorativeFighters();
    }

    /**
     * Add animated fighter silhouettes
     */
    addDecorativeFighters() {
        // Left fighter silhouette
        const leftFighter = this.add.graphics();
        leftFighter.fillStyle(0xe94560, 0.3);
        leftFighter.fillRect(50, 350, 80, 150);
        leftFighter.x = 0;

        this.tweens.add({
            targets: leftFighter,
            x: 10,
            duration: 2000,
            yoyo: true,
            repeat: -1
        });

        // Right fighter silhouette
        const rightFighter = this.add.graphics();
        rightFighter.fillStyle(0x00ffff, 0.3);
        rightFighter.fillRect(894, 350, 80, 150);
        rightFighter.x = 0;

        this.tweens.add({
            targets: rightFighter,
            x: -10,
            duration: 2000,
            yoyo: true,
            repeat: -1
        });
    }

    update() {
        // Navigation
        if (this.canInput) {
            if (Phaser.Input.Keyboard.JustDown(this.cursors.up)) {
                this.selectedIndex = (this.selectedIndex - 1 + this.menuOptions.length) % this.menuOptions.length;
                this.updateSelection();
                this.updateSelectorPosition();
                this.playSound('select');
                this.debounceInput();
            }

            if (Phaser.Input.Keyboard.JustDown(this.cursors.down)) {
                this.selectedIndex = (this.selectedIndex + 1) % this.menuOptions.length;
                this.updateSelection();
                this.updateSelectorPosition();
                this.playSound('select');
                this.debounceInput();
            }

            if (Phaser.Input.Keyboard.JustDown(this.enterKey) ||
                Phaser.Input.Keyboard.JustDown(this.spaceKey)) {
                this.playSound('confirm');
                this.menuOptions[this.selectedIndex].action();
                this.debounceInput();
            }
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
     * Update selector arrow position
     */
    updateSelectorPosition() {
        const selectedText = this.menuTexts[this.selectedIndex];
        this.selector.setPosition(
            selectedText.x - selectedText.width / 2 - 30,
            selectedText.y
        );
    }

    /**
     * Input debounce
     */
    debounceInput() {
        this.canInput = false;
        this.time.delayedCall(150, () => {
            this.canInput = true;
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

    /**
     * Start VS Mode (2 players)
     */
    startVsMode() {
        this.cameras.main.fadeOut(500);
        this.time.delayedCall(500, () => {
            this.scene.start('CharacterSelectScene', { mode: 'vs' });
        });
    }

    /**
     * Start VS CPU mode
     */
    startVsCPU() {
        this.cameras.main.fadeOut(500);
        this.time.delayedCall(500, () => {
            this.scene.start('CharacterSelectScene', { mode: 'cpu' });
        });
    }

    /**
     * Show controls overlay
     */
    showControls() {
        const width = this.cameras.main.width;
        const height = this.cameras.main.height;

        // Overlay
        const overlay = this.add.rectangle(width / 2, height / 2, width, height, 0x000000, 0.9);
        overlay.setInteractive();

        // Controls panel
        const panel = this.add.graphics();
        panel.fillStyle(0x1a1a2e, 1);
        panel.fillRect(width / 2 - 400, 50, 800, 476);
        panel.lineStyle(3, 0xe94560);
        panel.strokeRect(width / 2 - 400, 50, 800, 476);

        // Title
        const controlsTitle = this.add.text(width / 2, 90, 'CONTROLS', {
            fontFamily: '"Press Start 2P"',
            fontSize: '28px',
            fill: '#e94560'
        }).setOrigin(0.5);

        // Player 1 controls
        const p1Title = this.add.text(width / 2 - 180, 140, 'PLAYER 1', {
            fontFamily: '"Press Start 2P"',
            fontSize: '18px',
            fill: '#00ffff'
        }).setOrigin(0.5);

        const p1Controls = [
            'MOVE: W A S D',
            'PUNCH: F',
            'KICK: G',
            'BLOCK: H',
            'SPECIAL: R'
        ];

        p1Controls.forEach((ctrl, i) => {
            this.add.text(width / 2 - 180, 180 + i * 35, ctrl, {
                fontFamily: '"Press Start 2P"',
                fontSize: '12px',
                fill: '#ffffff'
            }).setOrigin(0.5);
        });

        // Player 2 controls
        const p2Title = this.add.text(width / 2 + 180, 140, 'PLAYER 2', {
            fontFamily: '"Press Start 2P"',
            fontSize: '18px',
            fill: '#ff6b8a'
        }).setOrigin(0.5);

        const p2Controls = [
            'MOVE: ARROWS',
            'PUNCH: J',
            'KICK: K',
            'BLOCK: L',
            'SPECIAL: U'
        ];

        p2Controls.forEach((ctrl, i) => {
            this.add.text(width / 2 + 180, 180 + i * 35, ctrl, {
                fontFamily: '"Press Start 2P"',
                fontSize: '12px',
                fill: '#ffffff'
            }).setOrigin(0.5);
        });

        // General
        this.add.text(width / 2, 400, 'PAUSE: ESC', {
            fontFamily: '"Press Start 2P"',
            fontSize: '12px',
            fill: '#888888'
        }).setOrigin(0.5);

        // Close instruction
        const closeText = this.add.text(width / 2, 480, 'PRESS ANY KEY TO CLOSE', {
            fontFamily: '"Press Start 2P"',
            fontSize: '14px',
            fill: '#e94560'
        }).setOrigin(0.5);

        this.tweens.add({
            targets: closeText,
            alpha: 0.5,
            duration: 500,
            yoyo: true,
            repeat: -1
        });

        // Close on any key
        const closePanel = () => {
            overlay.destroy();
            panel.destroy();
            controlsTitle.destroy();
            p1Title.destroy();
            p2Title.destroy();
            closeText.destroy();
            this.children.list
                .filter(c => c.type === 'Text' && p1Controls.concat(p2Controls).includes(c.text))
                .forEach(c => c.destroy());
        };

        this.input.keyboard.once('keydown', closePanel);
        overlay.once('pointerdown', closePanel);
    }

    /**
     * Show credits
     */
    showCredits() {
        const width = this.cameras.main.width;
        const height = this.cameras.main.height;

        const overlay = this.add.rectangle(width / 2, height / 2, width, height, 0x000000, 0.9);
        overlay.setInteractive();

        const credits = [
            'STREET BRAWLER',
            '',
            'A 2D FIGHTING GAME',
            '',
            'BUILT WITH:',
            'PHASER 3',
            'HOWLER.JS',
            'HTML5 CANVAS',
            '',
            'PROCEDURAL SPRITES',
            'SYNTHESIZED AUDIO',
            '',
            'PRESS ANY KEY TO CLOSE'
        ];

        const creditsTexts = credits.map((text, i) => {
            return this.add.text(width / 2, 100 + i * 35, text, {
                fontFamily: '"Press Start 2P"',
                fontSize: i === 0 ? '24px' : '14px',
                fill: i === 0 ? '#e94560' : '#ffffff'
            }).setOrigin(0.5);
        });

        const closeCredits = () => {
            overlay.destroy();
            creditsTexts.forEach(t => t.destroy());
        };

        this.input.keyboard.once('keydown', closeCredits);
        overlay.once('pointerdown', closeCredits);
    }
}

if (typeof window !== 'undefined') {
    window.MenuScene = MenuScene;
}
