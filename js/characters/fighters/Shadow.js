/**
 * Shadow - Technical Fighter
 * Tricky moves with teleportation abilities
 */
class Shadow extends Fighter {
    constructor(scene, x, y, playerNumber) {
        super(scene, x, y, {
            name: 'shadow',
            health: 85,
            attackPower: 11,
            defense: 5,
            speed: 240,
            jumpPower: -500,
            specialDamage: 22,
            specialCooldown: 2000,
            playerNumber: playerNumber
        });

        this.displayName = 'SHADOW';
        this.description = 'Mysterious ninja with teleport abilities';
        this.style = 'Technical';

        // Character colors - dark ninja theme
        this.colors = {
            skin: '#d4a574',
            hair: '#1a1a2e',
            outfit: '#2d2d44',
            outfitSecondary: '#4a4a6a',
            shoes: '#1a1a1a',
            specialColor: '#9400d3'
        };

        // Shadow clone for special
        this.shadowClone = null;
    }

    /**
     * Shadow Step - Teleport behind opponent and attack
     */
    special() {
        const now = Date.now();
        if (this.isAttacking || now - this.lastSpecialTime < this.specialCooldown) return;

        const opponent = this.scene.getOpponent(this);
        if (!opponent) return;

        this.isAttacking = true;
        this.lastSpecialTime = now;
        this.playAnim('special');

        this.scene.sound.play('special', { volume: 0.5 });

        // Create vanish effect at current position
        this.createVanishEffect(this.x, this.y);

        // Become invisible briefly
        this.setAlpha(0);

        // Teleport behind opponent
        this.scene.time.delayedCall(200, () => {
            const behindX = opponent.x + (opponent.facingRight ? -80 : 80);
            this.setPosition(
                Phaser.Math.Clamp(behindX, 100, 924),
                this.y
            );

            // Appear effect
            this.createAppearEffect(this.x, this.y);
            this.setAlpha(1);

            // Face opponent
            this.facingRight = this.x < opponent.x;
            this.setFlipX(!this.facingRight);

            // Quick strike
            this.scene.time.delayedCall(100, () => {
                this.createAttackHitbox(60, 50, this.specialDamage);
            });
        });
    }

    /**
     * Create vanish smoke effect
     */
    createVanishEffect(x, y) {
        for (let i = 0; i < 8; i++) {
            const angle = (i / 8) * Math.PI * 2;
            const smoke = this.scene.add.circle(
                x + Math.cos(angle) * 10,
                y - 50 + Math.sin(angle) * 10,
                15,
                0x4a4a6a
            );

            this.scene.tweens.add({
                targets: smoke,
                x: x + Math.cos(angle) * 50,
                y: (y - 50) + Math.sin(angle) * 50,
                alpha: 0,
                scale: 0.3,
                duration: 300,
                onComplete: () => smoke.destroy()
            });
        }
    }

    /**
     * Create appear effect
     */
    createAppearEffect(x, y) {
        // Dark energy swirl
        for (let i = 0; i < 6; i++) {
            const energy = this.scene.add.circle(
                x + Phaser.Math.Between(-30, 30),
                y - 50 + Phaser.Math.Between(-30, 30),
                Phaser.Math.Between(5, 15),
                0x9400d3
            );

            this.scene.tweens.add({
                targets: energy,
                y: energy.y - 40,
                alpha: 0,
                scale: 0,
                duration: 400,
                onComplete: () => energy.destroy()
            });
        }
    }

    /**
     * Shadow has a counter ability when blocking at perfect timing
     */
    block() {
        if (this.isAttacking) return;

        this.isBlocking = true;
        this.perfectBlockWindow = true;
        this.setVelocityX(0);
        this.playAnim('block');

        // Perfect block window is short
        this.scene.time.delayedCall(150, () => {
            this.perfectBlockWindow = false;
        });
    }

    /**
     * Override takeDamage for perfect block counter
     */
    takeDamage(amount, attacker) {
        if (this.isBlocking && this.perfectBlockWindow) {
            // Perfect block - counter attack!
            this.scene.sound.play('block', { volume: 0.5 });

            // Create counter effect
            const counterFlash = this.scene.add.circle(this.x, this.y - 50, 40, 0x9400d3);
            counterFlash.setAlpha(0.7);

            this.scene.tweens.add({
                targets: counterFlash,
                scale: 2,
                alpha: 0,
                duration: 200,
                onComplete: () => counterFlash.destroy()
            });

            // Counter damage
            attacker.takeDamage(amount * 0.5, this);

            return 0;
        }

        return super.takeDamage(amount, attacker);
    }

    /**
     * Shadow's kick has extra range due to ninja agility
     */
    kick() {
        if (this.isAttacking) return;

        this.isAttacking = true;

        // Slide kick
        const direction = this.facingRight ? 1 : -1;
        this.setVelocityX(direction * 300);
        this.playAnim('kick');

        this.createAttackHitbox(90, 40, this.attackPower * 1.1);

        this.scene.sound.play('kick', { volume: 0.4 });

        // Stop slide after short duration
        this.scene.time.delayedCall(200, () => {
            this.setVelocityX(0);
        });
    }

    static getInfo() {
        return {
            name: 'shadow',
            displayName: 'SHADOW',
            description: 'Mysterious ninja with\nteleport abilities',
            style: 'Technical',
            stats: {
                power: 6,
                speed: 8,
                defense: 5,
                special: 10
            },
            colors: {
                skin: '#d4a574',
                hair: '#1a1a2e',
                outfit: '#2d2d44',
                outfitSecondary: '#4a4a6a',
                shoes: '#1a1a1a',
                specialColor: '#9400d3'
            }
        };
    }
}

if (typeof window !== 'undefined') {
    window.Shadow = Shadow;
}
