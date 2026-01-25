/**
 * Ken - Speed Fighter
 * Fast attacks and movement, lower defense
 */
class Ken extends Fighter {
    constructor(scene, x, y, playerNumber) {
        super(scene, x, y, {
            name: 'ken',
            health: 90,
            attackPower: 10,
            defense: 4,
            speed: 260,
            jumpPower: -480,
            specialDamage: 25,
            specialCooldown: 1800,
            playerNumber: playerNumber
        });

        this.displayName = 'KEN';
        this.description = 'Fast and furious with flaming attacks';
        this.style = 'Speed';

        // Character colors
        this.colors = {
            skin: '#e8b89d',
            hair: '#ffd700',
            outfit: '#cc0000',
            outfitSecondary: '#8b0000',
            shoes: '#1a1a1a',
            specialColor: '#ff4400'
        };
    }

    /**
     * Shoryuken - Rising uppercut with flames
     */
    special() {
        const now = Date.now();
        if (this.isAttacking || now - this.lastSpecialTime < this.specialCooldown) return;

        this.isAttacking = true;
        this.lastSpecialTime = now;
        this.setVelocityX(0);
        this.playAnim('special');

        this.scene.sound.play('special', { volume: 0.5 });

        // Rising uppercut motion
        const direction = this.facingRight ? 1 : -1;
        this.setVelocity(direction * 100, -400);

        // Create fire effect
        this.createFireEffect();

        // Multiple hit windows
        for (let i = 0; i < 3; i++) {
            this.scene.time.delayedCall(100 + i * 100, () => {
                this.createAttackHitbox(70, 80, this.specialDamage / 3);
            });
        }
    }

    /**
     * Create fire trail effect
     */
    createFireEffect() {
        for (let i = 0; i < 5; i++) {
            this.scene.time.delayedCall(i * 50, () => {
                const fire = this.scene.add.circle(
                    this.x + Phaser.Math.Between(-20, 20),
                    this.y - 40 + Phaser.Math.Between(-20, 20),
                    Phaser.Math.Between(10, 25),
                    0xff4400
                );

                this.scene.tweens.add({
                    targets: fire,
                    alpha: 0,
                    scale: 0,
                    y: fire.y - 30,
                    duration: 300,
                    onComplete: () => fire.destroy()
                });
            });
        }
    }

    /**
     * Override punch for faster combo
     */
    punch() {
        if (this.isAttacking) return;

        this.isAttacking = true;
        this.setVelocityX(0);
        this.playAnim('punch');

        // Ken's punches are faster
        this.createAttackHitbox(55, 35, this.attackPower * 0.8);

        this.scene.sound.play('punch', { volume: 0.4 });

        // Faster recovery for combos
        this.scene.time.delayedCall(150, () => {
            this.isAttacking = false;
        });
    }

    static getInfo() {
        return {
            name: 'ken',
            displayName: 'KEN',
            description: 'Fast and furious with\nflaming attacks',
            style: 'Speed',
            stats: {
                power: 6,
                speed: 9,
                defense: 4,
                special: 8
            },
            colors: {
                skin: '#e8b89d',
                hair: '#ffd700',
                outfit: '#cc0000',
                outfitSecondary: '#8b0000',
                shoes: '#1a1a1a',
                specialColor: '#ff4400'
            }
        };
    }
}

if (typeof window !== 'undefined') {
    window.Ken = Ken;
}
