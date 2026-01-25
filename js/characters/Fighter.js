/**
 * Base Fighter Class
 * All playable fighters extend from this class
 */
class Fighter extends Phaser.Physics.Arcade.Sprite {
    constructor(scene, x, y, config) {
        super(scene, x, y, `${config.name}_idle`);

        this.scene = scene;
        this.config = config;
        this.fighterName = config.name;

        // Add to scene and enable physics
        scene.add.existing(this);
        scene.physics.add.existing(this);

        // Fighter stats
        this.maxHealth = config.health || 100;
        this.health = this.maxHealth;
        this.attackPower = config.attackPower || 10;
        this.defense = config.defense || 5;
        this.speed = config.speed || 200;
        this.jumpPower = config.jumpPower || -450;

        // Special move properties
        this.specialDamage = config.specialDamage || 25;
        this.specialCooldown = config.specialCooldown || 2000;
        this.lastSpecialTime = 0;

        // Combat state
        this.isAttacking = false;
        this.isBlocking = false;
        this.isHit = false;
        this.isKO = false;
        this.canMove = true;
        this.facingRight = true;
        this.comboCount = 0;
        this.lastAttackTime = 0;

        // Hitbox for attacks
        this.attackHitbox = null;

        // Physics settings
        this.setCollideWorldBounds(true);
        this.body.setGravityY(800);
        this.setSize(60, 110);
        this.setOffset(30, 20);

        // Set origin
        this.setOrigin(0.5, 1);

        // Ground position
        this.groundY = y;

        // Player number (1 or 2)
        this.playerNumber = config.playerNumber || 1;

        // Input keys (will be set by scene)
        this.keys = null;

        // Animation callbacks
        this.on('animationcomplete', this.onAnimationComplete, this);
    }

    /**
     * Set up input controls
     */
    setControls(keys) {
        this.keys = keys;
    }

    /**
     * Update fighter each frame
     */
    update(opponent) {
        if (this.isKO || !this.canMove) return;

        // Face opponent
        this.faceOpponent(opponent);

        // Handle input
        if (this.keys) {
            this.handleInput();
        }

        // Update attack hitbox position
        if (this.attackHitbox && this.attackHitbox.active) {
            const offsetX = this.facingRight ? 50 : -50;
            this.attackHitbox.setPosition(this.x + offsetX, this.y - 50);
        }
    }

    /**
     * Face toward opponent
     */
    faceOpponent(opponent) {
        if (!opponent || this.isAttacking) return;

        if (opponent.x > this.x) {
            this.facingRight = true;
            this.setFlipX(false);
        } else {
            this.facingRight = false;
            this.setFlipX(true);
        }
    }

    /**
     * Handle player input
     */
    handleInput() {
        if (this.isAttacking || this.isHit || this.isBlocking) return;

        const onGround = this.body.blocked.down || this.body.touching.down;

        // Movement
        if (this.keys.left.isDown) {
            this.setVelocityX(-this.speed);
            if (onGround && !this.isAttacking) {
                this.playAnim('walk');
            }
        } else if (this.keys.right.isDown) {
            this.setVelocityX(this.speed);
            if (onGround && !this.isAttacking) {
                this.playAnim('walk');
            }
        } else {
            this.setVelocityX(0);
            if (onGround && !this.isAttacking) {
                this.playAnim('idle');
            }
        }

        // Jump
        if (this.keys.up.isDown && onGround) {
            this.jump();
        }

        // Block
        if (this.keys.block && this.keys.block.isDown && onGround) {
            this.block();
        }

        // Attacks (using Phaser.Input.Keyboard.JustDown for single press)
        if (Phaser.Input.Keyboard.JustDown(this.keys.punch)) {
            this.punch();
        }
        if (Phaser.Input.Keyboard.JustDown(this.keys.kick)) {
            this.kick();
        }
        if (this.keys.special && Phaser.Input.Keyboard.JustDown(this.keys.special)) {
            this.special();
        }
    }

    /**
     * Play animation with fighter name prefix
     */
    playAnim(animName) {
        const fullName = `${this.fighterName}_${animName}`;
        if (this.anims.currentAnim?.key !== fullName) {
            this.play(fullName, true);
        }
    }

    /**
     * Jump action
     */
    jump() {
        this.setVelocityY(this.jumpPower);
        this.playAnim('jump');
        this.scene.sound.play('jump', { volume: 0.3 });
    }

    /**
     * Punch attack
     */
    punch() {
        if (this.isAttacking) return;

        this.isAttacking = true;
        this.setVelocityX(0);
        this.playAnim('punch');

        // Create attack hitbox
        this.createAttackHitbox(60, 40, this.attackPower);

        this.scene.sound.play('punch', { volume: 0.4 });

        // Check for combo
        const now = Date.now();
        if (now - this.lastAttackTime < 500) {
            this.comboCount++;
        } else {
            this.comboCount = 1;
        }
        this.lastAttackTime = now;
    }

    /**
     * Kick attack
     */
    kick() {
        if (this.isAttacking) return;

        this.isAttacking = true;
        this.setVelocityX(0);
        this.playAnim('kick');

        // Create attack hitbox (kick has more range)
        this.createAttackHitbox(80, 50, this.attackPower * 1.2);

        this.scene.sound.play('kick', { volume: 0.4 });

        // Check for combo
        const now = Date.now();
        if (now - this.lastAttackTime < 500) {
            this.comboCount++;
        } else {
            this.comboCount = 1;
        }
        this.lastAttackTime = now;
    }

    /**
     * Special attack (override in subclass)
     */
    special() {
        const now = Date.now();
        if (this.isAttacking || now - this.lastSpecialTime < this.specialCooldown) return;

        this.isAttacking = true;
        this.lastSpecialTime = now;
        this.setVelocityX(0);
        this.playAnim('special');

        this.scene.sound.play('special', { volume: 0.5 });

        // Default special - powerful energy attack
        this.scene.time.delayedCall(200, () => {
            this.createAttackHitbox(100, 60, this.specialDamage);
        });
    }

    /**
     * Block stance
     */
    block() {
        if (this.isAttacking) return;

        this.isBlocking = true;
        this.setVelocityX(0);
        this.playAnim('block');
    }

    /**
     * Stop blocking
     */
    stopBlock() {
        this.isBlocking = false;
        this.playAnim('idle');
    }

    /**
     * Create temporary attack hitbox
     */
    createAttackHitbox(width, height, damage) {
        const offsetX = this.facingRight ? 50 : -50;

        // Create or reuse hitbox
        if (!this.attackHitbox) {
            this.attackHitbox = this.scene.physics.add.sprite(
                this.x + offsetX,
                this.y - 50,
                null
            );
            this.attackHitbox.setVisible(false);
            this.attackHitbox.body.setAllowGravity(false);
        }

        this.attackHitbox.setPosition(this.x + offsetX, this.y - 50);
        this.attackHitbox.body.setSize(width, height);
        this.attackHitbox.setActive(true);
        this.attackHitbox.damage = damage;
        this.attackHitbox.owner = this;

        // Deactivate after short time
        this.scene.time.delayedCall(150, () => {
            if (this.attackHitbox) {
                this.attackHitbox.setActive(false);
            }
        });
    }

    /**
     * Take damage
     */
    takeDamage(amount, attacker) {
        if (this.isKO) return;

        // Reduce damage if blocking
        let finalDamage = amount;
        if (this.isBlocking) {
            finalDamage = Math.floor(amount * 0.2);
            this.showBlockEffect();
            this.scene.sound.play('block', { volume: 0.4 });
        } else {
            // Apply hit stun
            this.isHit = true;
            this.playAnim('hit');

            // Knockback
            const knockbackDir = attacker.x < this.x ? 1 : -1;
            this.setVelocityX(knockbackDir * 150);

            this.showHitEffect();
            this.scene.sound.play('hit', { volume: 0.5 });

            this.scene.time.delayedCall(300, () => {
                this.isHit = false;
            });
        }

        // Apply damage
        this.health = Math.max(0, this.health - finalDamage);

        // Check for KO
        if (this.health <= 0) {
            this.ko();
        }

        // Emit damage event
        this.scene.events.emit('fighterDamaged', this, finalDamage);

        return finalDamage;
    }

    /**
     * Show hit effect
     */
    showHitEffect() {
        const effect = this.scene.add.sprite(this.x, this.y - 50, 'hit_effect');
        effect.setScale(1.5);
        effect.setTint(0xffff00);

        this.scene.tweens.add({
            targets: effect,
            alpha: 0,
            scale: 2,
            duration: 200,
            onComplete: () => effect.destroy()
        });
    }

    /**
     * Show block effect
     */
    showBlockEffect() {
        const effect = this.scene.add.sprite(this.x, this.y - 50, 'block_effect');
        effect.setScale(1.5);

        this.scene.tweens.add({
            targets: effect,
            alpha: 0,
            scale: 2,
            duration: 300,
            onComplete: () => effect.destroy()
        });
    }

    /**
     * Fighter is knocked out
     */
    ko() {
        this.isKO = true;
        this.canMove = false;
        this.playAnim('ko');
        this.setVelocityX(0);

        this.scene.sound.play('ko', { volume: 0.6 });

        // Emit KO event
        this.scene.events.emit('fighterKO', this);
    }

    /**
     * Reset fighter for new round
     */
    reset(x) {
        this.health = this.maxHealth;
        this.isKO = false;
        this.isAttacking = false;
        this.isBlocking = false;
        this.isHit = false;
        this.canMove = true;
        this.comboCount = 0;

        this.setPosition(x, this.groundY);
        this.setVelocity(0, 0);
        this.playAnim('idle');
    }

    /**
     * Animation complete handler
     */
    onAnimationComplete(animation) {
        const animName = animation.key.replace(`${this.fighterName}_`, '');

        switch (animName) {
            case 'punch':
            case 'kick':
            case 'special':
                this.isAttacking = false;
                this.playAnim('idle');
                break;
            case 'hit':
                this.isHit = false;
                break;
            case 'block':
                // Stay in block if still holding
                if (!this.keys?.block?.isDown) {
                    this.stopBlock();
                }
                break;
        }
    }

    /**
     * Get health percentage
     */
    getHealthPercent() {
        return this.health / this.maxHealth;
    }

    /**
     * Victory pose
     */
    victory() {
        this.canMove = false;
        this.playAnim('victory');
    }
}

// Export
if (typeof window !== 'undefined') {
    window.Fighter = Fighter;
}
