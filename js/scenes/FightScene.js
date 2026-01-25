/**
 * Fight Scene
 * Main gameplay scene where fighters battle
 */
class FightScene extends Phaser.Scene {
    constructor() {
        super({ key: 'FightScene' });
    }

    init(data) {
        this.gameMode = data.mode || 'vs';
        this.p1FighterName = data.p1Fighter || 'ryu';
        this.p2FighterName = data.p2Fighter || 'ken';
    }

    create() {
        const width = this.cameras.main.width;
        const height = this.cameras.main.height;

        // Background
        const backgrounds = ['bg_city', 'bg_dojo', 'bg_arena'];
        const bgKey = Phaser.Utils.Array.GetRandom(backgrounds);
        this.add.image(width / 2, height / 2, bgKey);

        // Ground line (visual reference)
        this.groundY = height * 0.75;

        // Create physics groups
        this.projectiles = this.physics.add.group();

        // Create fighters
        this.createFighters();

        // Setup controls
        this.setupControls();

        // Create UI
        this.createUI();

        // Round system
        this.roundNumber = 1;
        this.maxRounds = 3;
        this.p1Wins = 0;
        this.p2Wins = 0;
        this.roundTime = 99;
        this.roundActive = false;

        // Setup collision detection
        this.setupCollisions();

        // Listen for events
        this.events.on('fighterDamaged', this.onFighterDamaged, this);
        this.events.on('fighterKO', this.onFighterKO, this);

        // Pause handling
        this.escKey = this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.ESC);
        this.isPaused = false;

        // Fade in and start round
        this.cameras.main.fadeIn(500);
        this.time.delayedCall(500, () => this.startRound());
    }

    /**
     * Create fighter instances
     */
    createFighters() {
        const width = this.cameras.main.width;

        // Fighter class mapping
        const fighterClasses = {
            'ryu': Ryu,
            'ken': Ken,
            'blaze': Blaze,
            'shadow': Shadow
        };

        // Create Player 1
        const P1Class = fighterClasses[this.p1FighterName] || Ryu;
        this.player1 = new P1Class(this, 200, this.groundY, 1);
        this.player1.setScale(1);

        // Create Player 2
        const P2Class = fighterClasses[this.p2FighterName] || Ken;
        this.player2 = new P2Class(this, width - 200, this.groundY, 2);
        this.player2.setScale(1);
        this.player2.setFlipX(true);
        this.player2.facingRight = false;

        // Play idle animations
        this.player1.playAnim('idle');
        this.player2.playAnim('idle');

        // CPU AI setup
        if (this.gameMode === 'cpu') {
            this.player2.isCPU = true;
            this.cpuActionTimer = 0;
            this.cpuDifficulty = 0.6; // 0-1, higher = harder
        }
    }

    /**
     * Setup player controls
     */
    setupControls() {
        // Player 1 controls (WASD + FGH)
        const p1Keys = {
            up: this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.W),
            down: this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.S),
            left: this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.A),
            right: this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.D),
            punch: this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.F),
            kick: this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.G),
            block: this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.H),
            special: this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.R)
        };
        this.player1.setControls(p1Keys);

        // Player 2 controls (Arrows + JKL) - only if VS mode
        if (this.gameMode === 'vs') {
            const p2Keys = {
                up: this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.UP),
                down: this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.DOWN),
                left: this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.LEFT),
                right: this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.RIGHT),
                punch: this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.J),
                kick: this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.K),
                block: this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.L),
                special: this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.U)
            };
            this.player2.setControls(p2Keys);
        }
    }

    /**
     * Create UI elements
     */
    createUI() {
        const width = this.cameras.main.width;

        // Health bar backgrounds
        this.add.image(185, 35, 'health_bar_bg').setScale(1);
        this.add.image(width - 185, 35, 'health_bar_bg').setScale(1).setFlipX(true);

        // Health bar fills (cropped dynamically)
        this.p1HealthBar = this.add.image(13, 38, 'health_bar_fill').setOrigin(0, 0.5);
        this.p2HealthBar = this.add.image(width - 13, 38, 'health_bar_fill').setOrigin(1, 0.5);

        // Player names
        this.add.text(20, 60, this.player1.displayName || this.p1FighterName.toUpperCase(), {
            fontFamily: '"Press Start 2P"',
            fontSize: '14px',
            fill: '#00ffff'
        });

        this.add.text(width - 20, 60, this.player2.displayName || this.p2FighterName.toUpperCase(), {
            fontFamily: '"Press Start 2P"',
            fontSize: '14px',
            fill: '#ff6b8a'
        }).setOrigin(1, 0);

        // Round indicator
        this.roundText = this.add.text(width / 2, 20, `ROUND ${this.roundNumber}`, {
            fontFamily: '"Press Start 2P"',
            fontSize: '16px',
            fill: '#ffffff'
        }).setOrigin(0.5);

        // Timer
        this.timerText = this.add.text(width / 2, 50, '99', {
            fontFamily: '"Press Start 2P"',
            fontSize: '32px',
            fill: '#ffff00'
        }).setOrigin(0.5);

        // Win indicators
        this.p1WinIndicators = [];
        this.p2WinIndicators = [];

        for (let i = 0; i < this.maxRounds; i++) {
            const p1Win = this.add.circle(100 + i * 25, 85, 8, 0x333333);
            p1Win.setStrokeStyle(2, 0x00ffff);
            this.p1WinIndicators.push(p1Win);

            const p2Win = this.add.circle(width - 100 - i * 25, 85, 8, 0x333333);
            p2Win.setStrokeStyle(2, 0xff6b8a);
            this.p2WinIndicators.push(p2Win);
        }

        // Combo counter
        this.comboText = this.add.text(width / 2, 150, '', {
            fontFamily: '"Press Start 2P"',
            fontSize: '24px',
            fill: '#ffff00'
        }).setOrigin(0.5).setAlpha(0);

        // Round announcer text
        this.announceText = this.add.text(width / 2, 250, '', {
            fontFamily: '"Press Start 2P"',
            fontSize: '48px',
            fill: '#e94560'
        }).setOrigin(0.5).setAlpha(0);
    }

    /**
     * Setup collision detection
     */
    setupCollisions() {
        // Check for attack hits
        this.physics.add.overlap(
            this.player1.attackHitbox,
            this.player2,
            () => this.handleAttackHit(this.player1, this.player2),
            null,
            this
        );

        this.physics.add.overlap(
            this.player2.attackHitbox,
            this.player1,
            () => this.handleAttackHit(this.player2, this.player1),
            null,
            this
        );

        // Projectile collisions
        this.physics.add.overlap(
            this.projectiles,
            this.player1,
            this.handleProjectileHit,
            null,
            this
        );

        this.physics.add.overlap(
            this.projectiles,
            this.player2,
            this.handleProjectileHit,
            null,
            this
        );
    }

    /**
     * Handle attack hitbox collision
     */
    handleAttackHit(attacker, defender) {
        if (!attacker.attackHitbox || !attacker.attackHitbox.active) return;
        if (defender.isKO) return;

        // Apply damage
        const damage = attacker.attackHitbox.damage || attacker.attackPower;
        defender.takeDamage(damage, attacker);

        // Deactivate hitbox
        attacker.attackHitbox.setActive(false);

        // Show combo if applicable
        if (attacker.comboCount > 1) {
            this.showCombo(attacker.comboCount);
        }
    }

    /**
     * Handle projectile collision
     */
    handleProjectileHit(projectile, fighter) {
        if (!projectile.active || fighter.isKO) return;
        if (projectile.owner === fighter) return;

        // Apply damage
        fighter.takeDamage(projectile.damage, projectile.owner);

        // Destroy projectile
        projectile.destroy();
    }

    /**
     * Start a new round
     */
    startRound() {
        // Reset fighters
        this.player1.reset(200);
        this.player2.reset(this.cameras.main.width - 200);
        this.player2.setFlipX(true);
        this.player2.facingRight = false;

        // Update health bars
        this.updateHealthBars();

        // Clear projectiles
        this.projectiles.clear(true, true);

        // Update round text
        this.roundText.setText(`ROUND ${this.roundNumber}`);

        // Announce round
        this.showAnnouncement(`ROUND ${this.roundNumber}`, () => {
            this.showAnnouncement('FIGHT!', () => {
                this.roundActive = true;
                this.roundTime = 99;

                // Start timer
                this.timerEvent = this.time.addEvent({
                    delay: 1000,
                    callback: this.updateTimer,
                    callbackScope: this,
                    loop: true
                });
            });
        });
    }

    /**
     * Show announcement text
     */
    showAnnouncement(text, callback) {
        this.announceText.setText(text);
        this.announceText.setAlpha(1);
        this.announceText.setScale(0.5);

        // Play sound
        this.playSound(text === 'FIGHT!' ? 'fight' : 'round');

        this.tweens.add({
            targets: this.announceText,
            scale: 1.2,
            duration: 300,
            ease: 'Power2',
            onComplete: () => {
                this.time.delayedCall(500, () => {
                    this.tweens.add({
                        targets: this.announceText,
                        alpha: 0,
                        duration: 200,
                        onComplete: () => {
                            if (callback) callback();
                        }
                    });
                });
            }
        });
    }

    /**
     * Update round timer
     */
    updateTimer() {
        if (!this.roundActive) return;

        this.roundTime--;
        this.timerText.setText(this.roundTime.toString().padStart(2, '0'));

        // Time color warning
        if (this.roundTime <= 10) {
            this.timerText.setFill('#ff0000');
        } else if (this.roundTime <= 30) {
            this.timerText.setFill('#ffaa00');
        }

        // Time out
        if (this.roundTime <= 0) {
            this.endRoundByTimeout();
        }
    }

    /**
     * End round by timeout
     */
    endRoundByTimeout() {
        this.roundActive = false;
        this.timerEvent.remove();

        // Determine winner by health
        if (this.player1.getHealthPercent() > this.player2.getHealthPercent()) {
            this.p1Wins++;
            this.showAnnouncement('TIME! P1 WINS', () => this.checkMatchEnd());
        } else if (this.player2.getHealthPercent() > this.player1.getHealthPercent()) {
            this.p2Wins++;
            this.showAnnouncement('TIME! P2 WINS', () => this.checkMatchEnd());
        } else {
            // Draw - both get a win point
            this.showAnnouncement('DRAW!', () => this.nextRound());
        }
    }

    /**
     * Fighter damaged event handler
     */
    onFighterDamaged(fighter, damage) {
        this.updateHealthBars();

        // Screen shake for big hits
        if (damage >= 20) {
            this.cameras.main.shake(100, 0.01);
        }
    }

    /**
     * Fighter KO event handler
     */
    onFighterKO(fighter) {
        if (!this.roundActive) return;

        this.roundActive = false;
        if (this.timerEvent) this.timerEvent.remove();

        // Determine winner
        if (fighter === this.player1) {
            this.p2Wins++;
            this.player2.victory();
            this.updateWinIndicators();
            this.showAnnouncement('K.O.!', () => {
                this.time.delayedCall(1000, () => this.checkMatchEnd());
            });
        } else {
            this.p1Wins++;
            this.player1.victory();
            this.updateWinIndicators();
            this.showAnnouncement('K.O.!', () => {
                this.time.delayedCall(1000, () => this.checkMatchEnd());
            });
        }

        this.playSound('ko');
    }

    /**
     * Check if match is over
     */
    checkMatchEnd() {
        const winsNeeded = Math.ceil(this.maxRounds / 2);

        if (this.p1Wins >= winsNeeded) {
            this.endMatch(1);
        } else if (this.p2Wins >= winsNeeded) {
            this.endMatch(2);
        } else {
            this.nextRound();
        }
    }

    /**
     * Start next round
     */
    nextRound() {
        this.roundNumber++;
        this.timerText.setFill('#ffff00');
        this.startRound();
    }

    /**
     * End the match
     */
    endMatch(winner) {
        this.playSound('victory');

        const winnerName = winner === 1 ?
            (this.player1.displayName || 'PLAYER 1') :
            (this.player2.displayName || 'PLAYER 2');

        this.showAnnouncement(`${winnerName} WINS!`, () => {
            this.time.delayedCall(2000, () => {
                this.cameras.main.fadeOut(500);
                this.time.delayedCall(500, () => {
                    this.scene.start('GameOverScene', {
                        winner: winner,
                        p1Fighter: this.p1FighterName,
                        p2Fighter: this.p2FighterName,
                        p1Wins: this.p1Wins,
                        p2Wins: this.p2Wins,
                        mode: this.gameMode
                    });
                });
            });
        });
    }

    /**
     * Update health bar displays
     */
    updateHealthBars() {
        const p1Percent = this.player1.getHealthPercent();
        const p2Percent = this.player2.getHealthPercent();

        // Scale health bars
        this.p1HealthBar.setScale(p1Percent, 1);
        this.p2HealthBar.setScale(p2Percent, 1);

        // Change color based on health
        this.updateHealthBarColor(this.p1HealthBar, p1Percent);
        this.updateHealthBarColor(this.p2HealthBar, p2Percent);
    }

    /**
     * Update health bar color based on percentage
     */
    updateHealthBarColor(healthBar, percent) {
        if (percent <= 0.25) {
            healthBar.setTexture('health_bar_critical');
        } else if (percent <= 0.5) {
            healthBar.setTexture('health_bar_low');
        } else {
            healthBar.setTexture('health_bar_fill');
        }
    }

    /**
     * Update win indicator circles
     */
    updateWinIndicators() {
        for (let i = 0; i < this.p1Wins; i++) {
            this.p1WinIndicators[i].setFillStyle(0x00ffff);
        }
        for (let i = 0; i < this.p2Wins; i++) {
            this.p2WinIndicators[i].setFillStyle(0xff6b8a);
        }
    }

    /**
     * Show combo counter
     */
    showCombo(count) {
        this.comboText.setText(`${count} HIT COMBO!`);
        this.comboText.setAlpha(1);
        this.comboText.setScale(0.5);

        this.tweens.add({
            targets: this.comboText,
            scale: 1,
            duration: 200,
            onComplete: () => {
                this.time.delayedCall(500, () => {
                    this.tweens.add({
                        targets: this.comboText,
                        alpha: 0,
                        duration: 300
                    });
                });
            }
        });
    }

    /**
     * CPU AI logic
     */
    updateCPU() {
        if (!this.player2.isCPU || !this.roundActive) return;

        this.cpuActionTimer++;

        // Action frequency based on difficulty
        const actionFrequency = 30 - Math.floor(this.cpuDifficulty * 20);
        if (this.cpuActionTimer < actionFrequency) return;
        this.cpuActionTimer = 0;

        const distance = Math.abs(this.player1.x - this.player2.x);
        const cpu = this.player2;
        const player = this.player1;

        // Face player
        cpu.facingRight = cpu.x < player.x;
        cpu.setFlipX(!cpu.facingRight);

        // Decision making
        const rand = Math.random();

        // If player is attacking, try to block
        if (player.isAttacking && rand < this.cpuDifficulty) {
            cpu.block();
            return;
        }

        // If close enough, attack
        if (distance < 120) {
            if (rand < 0.4) {
                cpu.punch();
            } else if (rand < 0.7) {
                cpu.kick();
            } else if (rand < 0.85 && Date.now() - cpu.lastSpecialTime > cpu.specialCooldown) {
                cpu.special();
            } else {
                // Sometimes block
                cpu.block();
                this.time.delayedCall(300, () => cpu.stopBlock());
            }
        }
        // Move toward player
        else if (distance > 150) {
            const direction = cpu.x < player.x ? 1 : -1;
            cpu.setVelocityX(direction * cpu.speed * 0.8);
            cpu.playAnim('walk');

            // Sometimes jump while approaching
            if (rand < 0.1 && cpu.body.blocked.down) {
                cpu.jump();
            }
        }
        // In mid range, mix of approach and projectiles
        else {
            if (rand < 0.3) {
                // Use special/projectile
                if (Date.now() - cpu.lastSpecialTime > cpu.specialCooldown) {
                    cpu.special();
                }
            } else if (rand < 0.6) {
                // Approach
                const direction = cpu.x < player.x ? 1 : -1;
                cpu.setVelocityX(direction * cpu.speed);
            } else {
                // Stay defensive
                cpu.setVelocityX(0);
                cpu.playAnim('idle');
            }
        }
    }

    /**
     * Get opponent for a fighter
     */
    getOpponent(fighter) {
        return fighter === this.player1 ? this.player2 : this.player1;
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
     * Toggle pause
     */
    togglePause() {
        this.isPaused = !this.isPaused;

        if (this.isPaused) {
            this.physics.pause();
            if (this.timerEvent) this.timerEvent.paused = true;

            // Show pause menu
            this.showPauseMenu();
        } else {
            this.physics.resume();
            if (this.timerEvent) this.timerEvent.paused = false;
            this.hidePauseMenu();
        }
    }

    /**
     * Show pause menu
     */
    showPauseMenu() {
        const width = this.cameras.main.width;
        const height = this.cameras.main.height;

        this.pauseOverlay = this.add.rectangle(width / 2, height / 2, width, height, 0x000000, 0.7);

        this.pauseText = this.add.text(width / 2, height / 2 - 50, 'PAUSED', {
            fontFamily: '"Press Start 2P"',
            fontSize: '36px',
            fill: '#ffffff'
        }).setOrigin(0.5);

        this.resumeText = this.add.text(width / 2, height / 2 + 20, 'PRESS ESC TO RESUME', {
            fontFamily: '"Press Start 2P"',
            fontSize: '14px',
            fill: '#888888'
        }).setOrigin(0.5);

        this.quitText = this.add.text(width / 2, height / 2 + 60, 'PRESS Q TO QUIT', {
            fontFamily: '"Press Start 2P"',
            fontSize: '14px',
            fill: '#888888'
        }).setOrigin(0.5);

        // Quit key
        this.quitKey = this.input.keyboard.addKey(Phaser.Input.Keyboard.KeyCodes.Q);
    }

    /**
     * Hide pause menu
     */
    hidePauseMenu() {
        if (this.pauseOverlay) this.pauseOverlay.destroy();
        if (this.pauseText) this.pauseText.destroy();
        if (this.resumeText) this.resumeText.destroy();
        if (this.quitText) this.quitText.destroy();
    }

    update() {
        // Pause handling
        if (Phaser.Input.Keyboard.JustDown(this.escKey)) {
            this.togglePause();
            return;
        }

        if (this.isPaused) {
            // Check for quit
            if (this.quitKey && Phaser.Input.Keyboard.JustDown(this.quitKey)) {
                this.scene.start('MenuScene');
            }
            return;
        }

        // Update fighters
        if (this.roundActive) {
            this.player1.update(this.player2);
            this.player2.update(this.player1);

            // CPU AI
            if (this.gameMode === 'cpu') {
                this.updateCPU();
            }
        }

        // Keep fighters within bounds
        this.constrainFighters();
    }

    /**
     * Keep fighters from going off screen
     */
    constrainFighters() {
        const minX = 50;
        const maxX = this.cameras.main.width - 50;

        this.player1.x = Phaser.Math.Clamp(this.player1.x, minX, maxX);
        this.player2.x = Phaser.Math.Clamp(this.player2.x, minX, maxX);
    }
}

if (typeof window !== 'undefined') {
    window.FightScene = FightScene;
}
