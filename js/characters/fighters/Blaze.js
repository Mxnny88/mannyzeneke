/**
 * Blaze - Power Fighter
 * Heavy damage but slower movement
 */
class Blaze extends Fighter {
    constructor(scene, x, y, playerNumber) {
        super(scene, x, y, {
            name: 'blaze',
            health: 120,
            attackPower: 16,
            defense: 8,
            speed: 160,
            jumpPower: -400,
            specialDamage: 35,
            specialCooldown: 2500,
            playerNumber: playerNumber
        });

        this.displayName = 'BLAZE';
        this.description = 'Devastating power with crushing blows';
        this.style = 'Power';

        // Character colors - fiery theme
        this.colors = {
            skin: '#c49a6c',
            hair: '#ff4500',
            outfit: '#8b0000',
            outfitSecondary: '#ff6600',
            shoes: '#2d2d2d',
            specialColor: '#ff0000'
        };

        // Blaze has super armor during attacks
        this.hasSuperArmor = false;
    }

    /**
     * Inferno Smash - Powerful ground pound
     */
    special() {
        const now = Date.now();
        if (this.isAttacking || now - this.lastSpecialTime < this.specialCooldown) return;

        this.isAttacking = true;
        this.lastSpecialTime = now;
        this.hasSuperArmor = true;
        this.setVelocityX(0);
        this.playAnim('special');

        this.scene.sound.play('special', { volume: 0.6 });

        // Jump up first
        this.setVelocityY(-300);

        // Then slam down
        this.scene.time.delayedCall(400, () => {
            this.setVelocityY(600);

            // Create ground impact when landing
            this.scene.time.delayedCall(300, () => {
                this.createGroundImpact();
                this.hasSuperArmor = false;
            });
        });
    }

    /**
     * Create ground impact effect
     */
    createGroundImpact() {
        // Large hitbox for ground pound
        this.createAttackHitbox(150, 60, this.specialDamage);

        // Screen shake
        this.scene.cameras.main.shake(200, 0.02);

        // Fire wave effect
        for (let i = -3; i <= 3; i++) {
            const wave = this.scene.add.circle(
                this.x + i * 40,
                this.y - 10,
                20,
                0xff4400
            );

            this.scene.tweens.add({
                targets: wave,
                alpha: 0,
                scaleX: 2,
                scaleY: 0.5,
                y: wave.y - 20,
                duration: 400,
                delay: Math.abs(i) * 50,
                onComplete: () => wave.destroy()
            });
        }

        // Ground crack visual
        const crack = this.scene.add.graphics();
        crack.lineStyle(3, 0xff6600);
        crack.beginPath();
        for (let i = 0; i < 5; i++) {
            const startX = this.x + Phaser.Math.Between(-50, 50);
            crack.moveTo(startX, this.y);
            crack.lineTo(startX + Phaser.Math.Between(-30, 30), this.y + 20);
        }
        crack.strokePath();

        this.scene.tweens.add({
            targets: crack,
            alpha: 0,
            duration: 500,
            onComplete: () => crack.destroy()
        });
    }

    /**
     * Override takeDamage for super armor
     */
    takeDamage(amount, attacker) {
        if (this.hasSuperArmor) {
            // Take reduced damage but don't flinch
            this.health = Math.max(0, this.health - amount * 0.5);
            this.scene.events.emit('fighterDamaged', this, amount * 0.5);

            // Visual feedback
            this.setTint(0xff8800);
            this.scene.time.delayedCall(100, () => this.clearTint());

            if (this.health <= 0) {
                this.ko();
            }
            return amount * 0.5;
        }

        return super.takeDamage(amount, attacker);
    }

    /**
     * Override punch for heavy hit
     */
    punch() {
        if (this.isAttacking) return;

        this.isAttacking = true;
        this.setVelocityX(0);
        this.playAnim('punch');

        // Blaze's punches are slower but hit harder
        this.scene.time.delayedCall(150, () => {
            this.createAttackHitbox(70, 50, this.attackPower);
        });

        this.scene.sound.play('punch', { volume: 0.5 });
    }

    static getInfo() {
        return {
            name: 'blaze',
            displayName: 'BLAZE',
            description: 'Devastating power with\ncrushing blows',
            style: 'Power',
            stats: {
                power: 10,
                speed: 4,
                defense: 8,
                special: 9
            },
            colors: {
                skin: '#c49a6c',
                hair: '#ff4500',
                outfit: '#8b0000',
                outfitSecondary: '#ff6600',
                shoes: '#2d2d2d',
                specialColor: '#ff0000'
            }
        };
    }
}

if (typeof window !== 'undefined') {
    window.Blaze = Blaze;
}
