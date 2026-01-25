/**
 * Character Select Scene
 * Players choose their fighters
 */
class CharacterSelectScene extends Phaser.Scene {
    constructor() {
        super({ key: 'CharacterSelectScene' });
    }

    init(data) {
        this.gameMode = data.mode || 'vs'; // 'vs' or 'cpu'
    }

    create() {
        const width = this.cameras.main.width;
        const height = this.cameras.main.height;

        // Background
        this.add.image(width / 2, height / 2, 'bg_dojo').setAlpha(0.4);

        const overlay = this.add.graphics();
        overlay.fillGradientStyle(0x1a1a2e, 0x1a1a2e, 0x0a0a1a, 0x0a0a1a, 0.7);
        overlay.fillRect(0, 0, width, height);

        // Title
        this.add.text(width / 2, 40, 'SELECT YOUR FIGHTER', {
            fontFamily: '"Press Start 2P"',
            fontSize: '28px',
            fill: '#e94560'
        }).setOrigin(0.5);

        // Fighter data
        this.fighters = [
            Ryu.getInfo(),
            Ken.getInfo(),
            Blaze.getInfo(),
            Shadow.getInfo()
        ];

        // Selection state
        this.p1Selection = 0;
        this.p2Selection = 1;
        this.p1Confirmed = false;
        this.p2Confirmed = this.gameMode === 'cpu'; // Auto-confirm for CPU

        // Create character portraits
        this.portraits = [];
        const startX = width / 2 - ((this.fighters.length - 1) * 90);

        this.fighters.forEach((fighter, index) => {
            const x = startX + index * 180;
            const y = 180;

            // Portrait background
            const bg = this.add.rectangle(x, y, 160, 210, 0x1a1a2e);
            bg.setStrokeStyle(3, 0x3d3d5c);

            // Portrait image
            const portrait = this.add.image(x, y, `${fighter.name}_portrait`);
            portrait.setScale(1);

            // Fighter name
            this.add.text(x, y + 130, fighter.displayName, {
                fontFamily: '"Press Start 2P"',
                fontSize: '14px',
                fill: '#ffffff'
            }).setOrigin(0.5);

            // Style label
            this.add.text(x, y + 155, fighter.style, {
                fontFamily: '"Press Start 2P"',
                fontSize: '10px',
                fill: '#888888'
            }).setOrigin(0.5);

            this.portraits.push({ bg, portrait, x, y });
        });

        // Player indicators
        this.p1Indicator = this.add.text(0, 0, 'P1', {
            fontFamily: '"Press Start 2P"',
            fontSize: '16px',
            fill: '#00ffff',
            backgroundColor: '#000000',
            padding: { x: 5, y: 3 }
        }).setOrigin(0.5);

        this.p2Indicator = this.add.text(0, 0, 'P2', {
            fontFamily: '"Press Start 2P"',
            fontSize: '16px',
            fill: '#ff6b8a',
            backgroundColor: '#000000',
            padding: { x: 5, y: 3 }
        }).setOrigin(0.5);

        // Fighter info panels
        this.createInfoPanels();

        // Update display
        this.updateSelection();

        // Instructions
        const p1Inst = this.gameMode === 'cpu' ? 'P1: A/D TO SELECT, F TO CONFIRM' : 'P1: A/D TO SELECT, F TO CONFIRM';
        const p2Inst = this.gameMode === 'cpu' ? 'CPU OPPONENT' : 'P2: ARROWS TO SELECT, J TO CONFIRM';

        this.add.text(width / 2, height - 70, p1Inst, {
            fontFamily: '"Press Start 2P"',
            fontSize: '11px',
            fill: '#00ffff'
        }).setOrigin(0.5);

        this.add.text(width / 2, height - 45, p2Inst, {
            fontFamily: '"Press Start 2P"',
            fontSize: '11px',
            fill: '#ff6b8a'
        }).setOrigin(0.5);

        this.add.text(width / 2, height - 20, 'ESC TO GO BACK', {
            fontFamily: '"Press Start 2P"',
            fontSize: '10px',
            fill: '#888888'
        }).setOrigin(0.5);

        // Setup controls
        this.setupControls();

        // CPU selection timer
        if (this.gameMode === 'cpu') {
            this.time.delayedCall(500, () => {
                this.p2Selection = Phaser.Math.Between(0, this.fighters.length - 1);
                this.updateSelection();
            });
        }

        // Fade in
        this.cameras.main.fadeIn(500);
    }

    /**
     * Create info panels for selected fighters
     */
    createInfoPanels() {
        const width = this.cameras.main.width;
        const height = this.cameras.main.height;

        // P1 info panel (left)
        this.p1Panel = this.add.container(130, height - 180);

        const p1Bg = this.add.rectangle(0, 0, 220, 120, 0x1a1a2e, 0.9);
        p1Bg.setStrokeStyle(2, 0x00ffff);

        this.p1NameText = this.add.text(0, -40, '', {
            fontFamily: '"Press Start 2P"',
            fontSize: '16px',
            fill: '#00ffff'
        }).setOrigin(0.5);

        this.p1DescText = this.add.text(0, -10, '', {
            fontFamily: '"Press Start 2P"',
            fontSize: '8px',
            fill: '#ffffff',
            align: 'center'
        }).setOrigin(0.5);

        this.p1StatsText = this.add.text(0, 30, '', {
            fontFamily: '"Press Start 2P"',
            fontSize: '8px',
            fill: '#aaaaaa'
        }).setOrigin(0.5);

        this.p1Panel.add([p1Bg, this.p1NameText, this.p1DescText, this.p1StatsText]);

        // P2 info panel (right)
        this.p2Panel = this.add.container(width - 130, height - 180);

        const p2Bg = this.add.rectangle(0, 0, 220, 120, 0x1a1a2e, 0.9);
        p2Bg.setStrokeStyle(2, 0xff6b8a);

        this.p2NameText = this.add.text(0, -40, '', {
            fontFamily: '"Press Start 2P"',
            fontSize: '16px',
            fill: '#ff6b8a'
        }).setOrigin(0.5);

        this.p2DescText = this.add.text(0, -10, '', {
            fontFamily: '"Press Start 2P"',
            fontSize: '8px',
            fill: '#ffffff',
            align: 'center'
        }).setOrigin(0.5);

        this.p2StatsText = this.add.text(0, 30, '', {
            fontFamily: '"Press Start 2P"',
            fontSize: '8px',
            fill: '#aaaaaa'
        }).setOrigin(0.5);

        this.p2Panel.add([p2Bg, this.p2NameText, this.p2DescText, this.p2StatsText]);
    }

    /**
     * Setup keyboard controls
     */
    setupControls() {
        // Player 1 controls
        this.p1Left = this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.A);
        this.p1Right = this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.D);
        this.p1Confirm = this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.F);

        // Player 2 controls
        this.p2Left = this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.LEFT);
        this.p2Right = this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.RIGHT);
        this.p2Confirm = this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.J);

        // Back
        this.escKey = this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.ESC);

        this.canInput = true;
    }

    update() {
        if (!this.canInput) return;

        // Player 1 selection
        if (!this.p1Confirmed) {
            if (Phaser.Input.Keyboard.JustDown(this.p1Left)) {
                this.p1Selection = (this.p1Selection - 1 + this.fighters.length) % this.fighters.length;
                this.updateSelection();
                this.playSound('select');
            }
            if (Phaser.Input.Keyboard.JustDown(this.p1Right)) {
                this.p1Selection = (this.p1Selection + 1) % this.fighters.length;
                this.updateSelection();
                this.playSound('select');
            }
            if (Phaser.Input.Keyboard.JustDown(this.p1Confirm)) {
                this.p1Confirmed = true;
                this.playSound('confirm');
                this.updateSelection();
                this.checkStartFight();
            }
        }

        // Player 2 selection (only in VS mode)
        if (!this.p2Confirmed && this.gameMode === 'vs') {
            if (Phaser.Input.Keyboard.JustDown(this.p2Left)) {
                this.p2Selection = (this.p2Selection - 1 + this.fighters.length) % this.fighters.length;
                this.updateSelection();
                this.playSound('select');
            }
            if (Phaser.Input.Keyboard.JustDown(this.p2Right)) {
                this.p2Selection = (this.p2Selection + 1) % this.fighters.length;
                this.updateSelection();
                this.playSound('select');
            }
            if (Phaser.Input.Keyboard.JustDown(this.p2Confirm)) {
                this.p2Confirmed = true;
                this.playSound('confirm');
                this.updateSelection();
                this.checkStartFight();
            }
        }

        // Back to menu
        if (Phaser.Input.Keyboard.JustDown(this.escKey)) {
            this.cameras.main.fadeOut(500);
            this.time.delayedCall(500, () => {
                this.scene.start('MenuScene');
            });
        }
    }

    /**
     * Update selection display
     */
    updateSelection() {
        // Update portrait highlights
        this.portraits.forEach((p, index) => {
            let strokeColor = 0x3d3d5c;
            let strokeWidth = 3;

            if (index === this.p1Selection && index === this.p2Selection) {
                strokeColor = 0xffff00; // Both selecting same
                strokeWidth = 5;
            } else if (index === this.p1Selection) {
                strokeColor = this.p1Confirmed ? 0x00ffff : 0x008888;
                strokeWidth = this.p1Confirmed ? 5 : 4;
            } else if (index === this.p2Selection) {
                strokeColor = this.p2Confirmed ? 0xff6b8a : 0x884444;
                strokeWidth = this.p2Confirmed ? 5 : 4;
            }

            p.bg.setStrokeStyle(strokeWidth, strokeColor);
        });

        // Update indicators
        const p1Portrait = this.portraits[this.p1Selection];
        const p2Portrait = this.portraits[this.p2Selection];

        this.p1Indicator.setPosition(p1Portrait.x - 60, p1Portrait.y - 90);
        this.p2Indicator.setPosition(p2Portrait.x + 60, p2Portrait.y - 90);

        // Animate confirmed indicators
        if (this.p1Confirmed) {
            this.p1Indicator.setScale(1.2);
        }
        if (this.p2Confirmed) {
            this.p2Indicator.setScale(1.2);
        }

        // Update info panels
        const p1Fighter = this.fighters[this.p1Selection];
        this.p1NameText.setText(p1Fighter.displayName);
        this.p1DescText.setText(p1Fighter.description);
        this.p1StatsText.setText(this.formatStats(p1Fighter.stats));

        const p2Fighter = this.fighters[this.p2Selection];
        this.p2NameText.setText(p2Fighter.displayName);
        this.p2DescText.setText(p2Fighter.description);
        this.p2StatsText.setText(this.formatStats(p2Fighter.stats));
    }

    /**
     * Format stats for display
     */
    formatStats(stats) {
        return `POW:${'*'.repeat(stats.power)} SPD:${'*'.repeat(stats.speed)}\nDEF:${'*'.repeat(stats.defense)} SPC:${'*'.repeat(stats.special)}`;
    }

    /**
     * Check if both players confirmed
     */
    checkStartFight() {
        if (this.p1Confirmed && this.p2Confirmed) {
            this.canInput = false;

            // Show VS screen briefly
            this.showVsScreen();
        }
    }

    /**
     * Show VS screen before fight
     */
    showVsScreen() {
        const width = this.cameras.main.width;
        const height = this.cameras.main.height;

        const overlay = this.add.rectangle(width / 2, height / 2, width, height, 0x000000, 0.9);

        const p1Fighter = this.fighters[this.p1Selection];
        const p2Fighter = this.fighters[this.p2Selection];

        // P1 name slides in from left
        const p1Name = this.add.text(-200, height / 2 - 50, p1Fighter.displayName, {
            fontFamily: '"Press Start 2P"',
            fontSize: '36px',
            fill: '#00ffff'
        }).setOrigin(0.5);

        this.tweens.add({
            targets: p1Name,
            x: width / 2 - 150,
            duration: 500,
            ease: 'Power2'
        });

        // VS text
        const vsText = this.add.text(width / 2, height / 2, 'VS', {
            fontFamily: '"Press Start 2P"',
            fontSize: '48px',
            fill: '#e94560'
        }).setOrigin(0.5).setAlpha(0);

        this.tweens.add({
            targets: vsText,
            alpha: 1,
            scale: 1.2,
            duration: 300,
            delay: 300
        });

        // P2 name slides in from right
        const p2Name = this.add.text(width + 200, height / 2 + 50, p2Fighter.displayName, {
            fontFamily: '"Press Start 2P"',
            fontSize: '36px',
            fill: '#ff6b8a'
        }).setOrigin(0.5);

        this.tweens.add({
            targets: p2Name,
            x: width / 2 + 150,
            duration: 500,
            ease: 'Power2'
        });

        // Start fight after delay
        this.time.delayedCall(2000, () => {
            this.cameras.main.fadeOut(500);
            this.time.delayedCall(500, () => {
                this.scene.start('FightScene', {
                    mode: this.gameMode,
                    p1Fighter: p1Fighter.name,
                    p2Fighter: p2Fighter.name
                });
            });
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
    window.CharacterSelectScene = CharacterSelectScene;
}
