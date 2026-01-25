/**
 * Ryu - Balanced Fighter
 * Well-rounded stats with a powerful Hadouken special
 */
class Ryu extends Fighter {
    constructor(scene, x, y, playerNumber) {
        super(scene, x, y, {
            name: 'ryu',
            health: 100,
            attackPower: 12,
            defense: 6,
            speed: 200,
            jumpPower: -450,
            specialDamage: 20,
            specialCooldown: 1500,
            playerNumber: playerNumber
        });

        this.displayName = 'RYU';
        this.description = 'Balanced warrior with powerful energy attacks';
        this.style = 'Balanced';

        // Character colors for sprite generation
        this.colors = {
            skin: '#e8b89d',
            hair: '#2d1810',
            outfit: '#ffffff',
            outfitSecondary: '#1a1a1a',
            shoes: '#8b4513',
            specialColor: '#00aaff'
        };
    }

    /**
     * Hadouken - Energy projectile
     */
    special() {
        const now = Date.now();
        if (this.isAttacking || now - this.lastSpecialTime < this.specialCooldown) return;

        this.isAttacking = true;
        this.lastSpecialTime = now;
        this.setVelocityX(0);
        this.playAnim('special');

        this.scene.sound.play('special', { volume: 0.5 });

        // Create hadouken projectile after wind-up
        this.scene.time.delayedCall(300, () => {
            this.fireHadouken();
        });
    }

    /**
     * Fire the hadouken projectile
     */
    fireHadouken() {
        const direction = this.facingRight ? 1 : -1;
        const offsetX = this.facingRight ? 60 : -60;

        const hadouken = this.scene.physics.add.sprite(
            this.x + offsetX,
            this.y - 50,
            'ryu_projectile'
        );

        hadouken.setFlipX(!this.facingRight);
        hadouken.body.setAllowGravity(false);
        hadouken.setVelocityX(direction * 400);
        hadouken.damage = this.specialDamage;
        hadouken.owner = this;

        // Add glow effect
        hadouken.setTint(0x00aaff);

        // Add to projectiles group
        if (this.scene.projectiles) {
            this.scene.projectiles.add(hadouken);
        }

        // Destroy after traveling
        this.scene.time.delayedCall(1500, () => {
            if (hadouken.active) {
                hadouken.destroy();
            }
        });

        // Check world bounds
        hadouken.setCollideWorldBounds(true);
        hadouken.body.onWorldBounds = true;
        hadouken.body.world.on('worldbounds', (body) => {
            if (body.gameObject === hadouken) {
                hadouken.destroy();
            }
        });
    }

    /**
     * Static method to get character info for selection screen
     */
    static getInfo() {
        return {
            name: 'ryu',
            displayName: 'RYU',
            description: 'Balanced warrior with\npowerful energy attacks',
            style: 'Balanced',
            stats: {
                power: 7,
                speed: 6,
                defense: 6,
                special: 8
            },
            colors: {
                skin: '#e8b89d',
                hair: '#2d1810',
                outfit: '#ffffff',
                outfitSecondary: '#1a1a1a',
                shoes: '#8b4513',
                specialColor: '#00aaff'
            }
        };
    }
}

// Export
if (typeof window !== 'undefined') {
    window.Ryu = Ryu;
}
