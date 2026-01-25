/**
 * Procedural Sprite Generator
 * Creates pixel-art style fighter sprites using Canvas API
 */
class SpriteGenerator {
    constructor(scene) {
        this.scene = scene;
    }

    /**
     * Generate all sprites for a fighter
     */
    generateFighterSprites(config) {
        const { name, colors } = config;

        // Generate each animation frame set
        this.generateIdleSprite(name, colors);
        this.generateWalkSprite(name, colors);
        this.generateJumpSprite(name, colors);
        this.generatePunchSprite(name, colors);
        this.generateKickSprite(name, colors);
        this.generateSpecialSprite(name, colors);
        this.generateHitSprite(name, colors);
        this.generateBlockSprite(name, colors);
        this.generateKOSprite(name, colors);
        this.generateVictorySprite(name, colors);
    }

    /**
     * Create a canvas texture and add to Phaser
     */
    createTexture(name, width, height, drawCallback) {
        const canvas = document.createElement('canvas');
        canvas.width = width;
        canvas.height = height;
        const ctx = canvas.getContext('2d');

        // Enable pixel-perfect rendering
        ctx.imageSmoothingEnabled = false;

        drawCallback(ctx, width, height);

        // Add texture to Phaser
        if (this.scene.textures.exists(name)) {
            this.scene.textures.remove(name);
        }
        this.scene.textures.addCanvas(name, canvas);
    }

    /**
     * Draw a pixel-art fighter body
     */
    drawFighterBody(ctx, x, y, colors, pose = 'idle') {
        const { skin, hair, outfit, outfitSecondary, shoes } = colors;

        ctx.save();
        ctx.translate(x, y);

        // Scale for pixel art effect
        const scale = 2;

        // Head
        ctx.fillStyle = skin;
        ctx.fillRect(12 * scale, 0, 16 * scale, 16 * scale);

        // Hair
        ctx.fillStyle = hair;
        ctx.fillRect(12 * scale, 0, 16 * scale, 6 * scale);
        ctx.fillRect(10 * scale, 2 * scale, 4 * scale, 8 * scale);
        ctx.fillRect(26 * scale, 2 * scale, 4 * scale, 8 * scale);

        // Eyes
        ctx.fillStyle = '#ffffff';
        ctx.fillRect(16 * scale, 6 * scale, 4 * scale, 4 * scale);
        ctx.fillRect(22 * scale, 6 * scale, 4 * scale, 4 * scale);
        ctx.fillStyle = '#000000';
        ctx.fillRect(18 * scale, 7 * scale, 2 * scale, 3 * scale);
        ctx.fillRect(24 * scale, 7 * scale, 2 * scale, 3 * scale);

        // Body/Torso
        ctx.fillStyle = outfit;
        ctx.fillRect(10 * scale, 16 * scale, 20 * scale, 20 * scale);

        // Belt
        ctx.fillStyle = outfitSecondary;
        ctx.fillRect(10 * scale, 32 * scale, 20 * scale, 4 * scale);

        // Arms based on pose
        ctx.fillStyle = skin;
        if (pose === 'punch') {
            // Extended punch arm
            ctx.fillRect(28 * scale, 18 * scale, 20 * scale, 8 * scale);
            ctx.fillRect(4 * scale, 20 * scale, 8 * scale, 12 * scale);
            // Fist
            ctx.fillRect(46 * scale, 16 * scale, 8 * scale, 12 * scale);
        } else if (pose === 'kick') {
            // Normal arms during kick
            ctx.fillRect(2 * scale, 18 * scale, 10 * scale, 10 * scale);
            ctx.fillRect(28 * scale, 18 * scale, 10 * scale, 10 * scale);
        } else if (pose === 'block') {
            // Arms crossed for blocking
            ctx.fillRect(6 * scale, 16 * scale, 12 * scale, 8 * scale);
            ctx.fillRect(22 * scale, 16 * scale, 12 * scale, 8 * scale);
            ctx.fillRect(14 * scale, 14 * scale, 12 * scale, 6 * scale);
        } else if (pose === 'special') {
            // Both arms forward for special
            ctx.fillRect(28 * scale, 16 * scale, 16 * scale, 10 * scale);
            ctx.fillRect(28 * scale, 24 * scale, 16 * scale, 10 * scale);
        } else {
            // Default arms
            ctx.fillRect(2 * scale, 18 * scale, 10 * scale, 14 * scale);
            ctx.fillRect(28 * scale, 18 * scale, 10 * scale, 14 * scale);
        }

        // Legs
        ctx.fillStyle = outfit;
        if (pose === 'kick') {
            // One leg extended for kick
            ctx.fillRect(12 * scale, 36 * scale, 8 * scale, 20 * scale);
            ctx.fillRect(28 * scale, 38 * scale, 24 * scale, 8 * scale);
            // Foot
            ctx.fillStyle = shoes;
            ctx.fillRect(12 * scale, 54 * scale, 10 * scale, 6 * scale);
            ctx.fillRect(48 * scale, 36 * scale, 8 * scale, 12 * scale);
        } else if (pose === 'jump') {
            // Tucked legs for jump
            ctx.fillRect(12 * scale, 36 * scale, 8 * scale, 12 * scale);
            ctx.fillRect(20 * scale, 36 * scale, 8 * scale, 12 * scale);
            ctx.fillStyle = shoes;
            ctx.fillRect(10 * scale, 46 * scale, 10 * scale, 6 * scale);
            ctx.fillRect(22 * scale, 46 * scale, 10 * scale, 6 * scale);
        } else if (pose === 'ko') {
            // Fallen pose
            ctx.fillRect(12 * scale, 36 * scale, 20 * scale, 8 * scale);
            ctx.fillStyle = shoes;
            ctx.fillRect(30 * scale, 36 * scale, 8 * scale, 8 * scale);
        } else {
            // Standing legs
            ctx.fillRect(12 * scale, 36 * scale, 8 * scale, 18 * scale);
            ctx.fillRect(20 * scale, 36 * scale, 8 * scale, 18 * scale);
            // Shoes
            ctx.fillStyle = shoes;
            ctx.fillRect(10 * scale, 52 * scale, 12 * scale, 8 * scale);
            ctx.fillRect(18 * scale, 52 * scale, 12 * scale, 8 * scale);
        }

        ctx.restore();
    }

    /**
     * Generate idle animation sprite sheet
     */
    generateIdleSprite(name, colors) {
        const frameWidth = 120;
        const frameHeight = 140;
        const frames = 4;

        this.createTexture(`${name}_idle`, frameWidth * frames, frameHeight, (ctx, w, h) => {
            for (let i = 0; i < frames; i++) {
                const bounceOffset = Math.sin(i * Math.PI / 2) * 2;
                ctx.save();
                ctx.translate(i * frameWidth, bounceOffset);
                this.drawFighterBody(ctx, 10, 10, colors, 'idle');
                ctx.restore();
            }
        });

        // Create animation config
        this.scene.anims.create({
            key: `${name}_idle`,
            frames: this.scene.anims.generateFrameNumbers(`${name}_idle`, {
                start: 0,
                end: frames - 1,
                first: 0
            }),
            frameRate: 8,
            repeat: -1
        });
    }

    /**
     * Generate walk animation
     */
    generateWalkSprite(name, colors) {
        const frameWidth = 120;
        const frameHeight = 140;
        const frames = 6;

        this.createTexture(`${name}_walk`, frameWidth * frames, frameHeight, (ctx, w, h) => {
            for (let i = 0; i < frames; i++) {
                ctx.save();
                ctx.translate(i * frameWidth, 0);
                this.drawFighterBody(ctx, 10, 10, colors, 'idle');
                ctx.restore();
            }
        });

        this.scene.anims.create({
            key: `${name}_walk`,
            frames: this.scene.anims.generateFrameNumbers(`${name}_walk`, {
                start: 0,
                end: frames - 1
            }),
            frameRate: 10,
            repeat: -1
        });
    }

    /**
     * Generate jump animation
     */
    generateJumpSprite(name, colors) {
        const frameWidth = 120;
        const frameHeight = 140;
        const frames = 3;

        this.createTexture(`${name}_jump`, frameWidth * frames, frameHeight, (ctx, w, h) => {
            for (let i = 0; i < frames; i++) {
                ctx.save();
                ctx.translate(i * frameWidth, 0);
                this.drawFighterBody(ctx, 10, 10, colors, 'jump');
                ctx.restore();
            }
        });

        this.scene.anims.create({
            key: `${name}_jump`,
            frames: this.scene.anims.generateFrameNumbers(`${name}_jump`, {
                start: 0,
                end: frames - 1
            }),
            frameRate: 10,
            repeat: 0
        });
    }

    /**
     * Generate punch animation
     */
    generatePunchSprite(name, colors) {
        const frameWidth = 140;
        const frameHeight = 140;
        const frames = 4;

        this.createTexture(`${name}_punch`, frameWidth * frames, frameHeight, (ctx, w, h) => {
            for (let i = 0; i < frames; i++) {
                ctx.save();
                ctx.translate(i * frameWidth, 0);
                const pose = i === 1 || i === 2 ? 'punch' : 'idle';
                this.drawFighterBody(ctx, 10, 10, colors, pose);
                ctx.restore();
            }
        });

        this.scene.anims.create({
            key: `${name}_punch`,
            frames: this.scene.anims.generateFrameNumbers(`${name}_punch`, {
                start: 0,
                end: frames - 1
            }),
            frameRate: 15,
            repeat: 0
        });
    }

    /**
     * Generate kick animation
     */
    generateKickSprite(name, colors) {
        const frameWidth = 160;
        const frameHeight = 140;
        const frames = 4;

        this.createTexture(`${name}_kick`, frameWidth * frames, frameHeight, (ctx, w, h) => {
            for (let i = 0; i < frames; i++) {
                ctx.save();
                ctx.translate(i * frameWidth, 0);
                const pose = i === 1 || i === 2 ? 'kick' : 'idle';
                this.drawFighterBody(ctx, 10, 10, colors, pose);
                ctx.restore();
            }
        });

        this.scene.anims.create({
            key: `${name}_kick`,
            frames: this.scene.anims.generateFrameNumbers(`${name}_kick`, {
                start: 0,
                end: frames - 1
            }),
            frameRate: 12,
            repeat: 0
        });
    }

    /**
     * Generate special move animation
     */
    generateSpecialSprite(name, colors) {
        const frameWidth = 160;
        const frameHeight = 140;
        const frames = 6;

        this.createTexture(`${name}_special`, frameWidth * frames, frameHeight, (ctx, w, h) => {
            for (let i = 0; i < frames; i++) {
                ctx.save();
                ctx.translate(i * frameWidth, 0);
                const pose = i >= 2 && i <= 4 ? 'special' : 'idle';
                this.drawFighterBody(ctx, 10, 10, colors, pose);

                // Add energy effect for special
                if (i >= 2 && i <= 4) {
                    ctx.fillStyle = colors.specialColor || '#00ffff';
                    ctx.globalAlpha = 0.7;
                    for (let j = 0; j < 5; j++) {
                        ctx.beginPath();
                        ctx.arc(90 + j * 15, 60 + Math.sin(j) * 10, 8, 0, Math.PI * 2);
                        ctx.fill();
                    }
                }
                ctx.restore();
            }
        });

        this.scene.anims.create({
            key: `${name}_special`,
            frames: this.scene.anims.generateFrameNumbers(`${name}_special`, {
                start: 0,
                end: frames - 1
            }),
            frameRate: 12,
            repeat: 0
        });
    }

    /**
     * Generate hit/hurt animation
     */
    generateHitSprite(name, colors) {
        const frameWidth = 120;
        const frameHeight = 140;
        const frames = 3;

        this.createTexture(`${name}_hit`, frameWidth * frames, frameHeight, (ctx, w, h) => {
            for (let i = 0; i < frames; i++) {
                ctx.save();
                ctx.translate(i * frameWidth + (i * 5), 0);
                this.drawFighterBody(ctx, 10, 10, colors, 'idle');
                // Add hit flash effect
                if (i === 1) {
                    ctx.fillStyle = 'rgba(255, 255, 255, 0.5)';
                    ctx.fillRect(0, 0, frameWidth, frameHeight);
                }
                ctx.restore();
            }
        });

        this.scene.anims.create({
            key: `${name}_hit`,
            frames: this.scene.anims.generateFrameNumbers(`${name}_hit`, {
                start: 0,
                end: frames - 1
            }),
            frameRate: 10,
            repeat: 0
        });
    }

    /**
     * Generate block animation
     */
    generateBlockSprite(name, colors) {
        const frameWidth = 120;
        const frameHeight = 140;
        const frames = 2;

        this.createTexture(`${name}_block`, frameWidth * frames, frameHeight, (ctx, w, h) => {
            for (let i = 0; i < frames; i++) {
                ctx.save();
                ctx.translate(i * frameWidth, 0);
                this.drawFighterBody(ctx, 10, 10, colors, 'block');
                ctx.restore();
            }
        });

        this.scene.anims.create({
            key: `${name}_block`,
            frames: this.scene.anims.generateFrameNumbers(`${name}_block`, {
                start: 0,
                end: frames - 1
            }),
            frameRate: 8,
            repeat: 0
        });
    }

    /**
     * Generate KO/death animation
     */
    generateKOSprite(name, colors) {
        const frameWidth = 140;
        const frameHeight = 140;
        const frames = 5;

        this.createTexture(`${name}_ko`, frameWidth * frames, frameHeight, (ctx, w, h) => {
            for (let i = 0; i < frames; i++) {
                ctx.save();
                ctx.translate(i * frameWidth, 0);
                // Gradually fall down
                const rotation = (i / frames) * (Math.PI / 2);
                ctx.translate(60, 100);
                ctx.rotate(rotation);
                ctx.translate(-60, -100);
                this.drawFighterBody(ctx, 10, 10, colors, i < 3 ? 'hit' : 'ko');
                ctx.restore();
            }
        });

        this.scene.anims.create({
            key: `${name}_ko`,
            frames: this.scene.anims.generateFrameNumbers(`${name}_ko`, {
                start: 0,
                end: frames - 1
            }),
            frameRate: 8,
            repeat: 0
        });
    }

    /**
     * Generate victory animation
     */
    generateVictorySprite(name, colors) {
        const frameWidth = 120;
        const frameHeight = 140;
        const frames = 4;

        this.createTexture(`${name}_victory`, frameWidth * frames, frameHeight, (ctx, w, h) => {
            for (let i = 0; i < frames; i++) {
                ctx.save();
                ctx.translate(i * frameWidth, Math.abs(Math.sin(i * Math.PI / 2)) * -10);
                this.drawFighterBody(ctx, 10, 10, colors, 'idle');
                ctx.restore();
            }
        });

        this.scene.anims.create({
            key: `${name}_victory`,
            frames: this.scene.anims.generateFrameNumbers(`${name}_victory`, {
                start: 0,
                end: frames - 1
            }),
            frameRate: 6,
            repeat: -1
        });
    }

    /**
     * Generate projectile sprite
     */
    generateProjectile(name, color) {
        this.createTexture(`${name}_projectile`, 60, 30, (ctx, w, h) => {
            // Energy ball
            const gradient = ctx.createRadialGradient(30, 15, 0, 30, 15, 15);
            gradient.addColorStop(0, '#ffffff');
            gradient.addColorStop(0.3, color);
            gradient.addColorStop(1, 'transparent');

            ctx.fillStyle = gradient;
            ctx.beginPath();
            ctx.arc(30, 15, 15, 0, Math.PI * 2);
            ctx.fill();

            // Trail
            ctx.fillStyle = color;
            ctx.globalAlpha = 0.5;
            ctx.beginPath();
            ctx.ellipse(15, 15, 15, 8, 0, 0, Math.PI * 2);
            ctx.fill();
        });
    }

    /**
     * Generate background for the fighting stage
     */
    generateBackground(name, config) {
        const width = 1024;
        const height = 576;

        this.createTexture(name, width, height, (ctx, w, h) => {
            // Sky gradient
            const skyGradient = ctx.createLinearGradient(0, 0, 0, h * 0.6);
            skyGradient.addColorStop(0, config.skyTop || '#1a1a2e');
            skyGradient.addColorStop(1, config.skyBottom || '#16213e');
            ctx.fillStyle = skyGradient;
            ctx.fillRect(0, 0, w, h * 0.6);

            // City silhouette / background elements
            ctx.fillStyle = config.buildingColor || '#0f0f23';
            for (let i = 0; i < 15; i++) {
                const bw = 50 + Math.random() * 80;
                const bh = 100 + Math.random() * 200;
                const bx = i * 70 + Math.random() * 20;
                ctx.fillRect(bx, h * 0.6 - bh, bw, bh);

                // Windows
                ctx.fillStyle = config.windowColor || '#ffff00';
                for (let wy = h * 0.6 - bh + 20; wy < h * 0.6 - 20; wy += 30) {
                    for (let wx = bx + 10; wx < bx + bw - 10; wx += 20) {
                        if (Math.random() > 0.3) {
                            ctx.fillRect(wx, wy, 8, 12);
                        }
                    }
                }
                ctx.fillStyle = config.buildingColor || '#0f0f23';
            }

            // Ground
            const groundGradient = ctx.createLinearGradient(0, h * 0.6, 0, h);
            groundGradient.addColorStop(0, config.groundTop || '#2d2d44');
            groundGradient.addColorStop(1, config.groundBottom || '#1a1a2e');
            ctx.fillStyle = groundGradient;
            ctx.fillRect(0, h * 0.6, w, h * 0.4);

            // Ground line
            ctx.strokeStyle = config.groundLine || '#e94560';
            ctx.lineWidth = 4;
            ctx.beginPath();
            ctx.moveTo(0, h * 0.75);
            ctx.lineTo(w, h * 0.75);
            ctx.stroke();

            // Ground details
            ctx.fillStyle = config.groundDetail || '#3d3d5c';
            for (let i = 0; i < 20; i++) {
                ctx.fillRect(i * 55, h * 0.75, 50, 5);
            }
        });
    }

    /**
     * Generate UI elements
     */
    generateHealthBar() {
        // Health bar background
        this.createTexture('health_bar_bg', 350, 40, (ctx, w, h) => {
            ctx.fillStyle = '#1a1a2e';
            ctx.fillRect(0, 0, w, h);
            ctx.strokeStyle = '#ffffff';
            ctx.lineWidth = 3;
            ctx.strokeRect(0, 0, w, h);
        });

        // Health bar fill
        this.createTexture('health_bar_fill', 344, 34, (ctx, w, h) => {
            const gradient = ctx.createLinearGradient(0, 0, 0, h);
            gradient.addColorStop(0, '#4ade80');
            gradient.addColorStop(0.5, '#22c55e');
            gradient.addColorStop(1, '#16a34a');
            ctx.fillStyle = gradient;
            ctx.fillRect(0, 0, w, h);
        });

        // Low health
        this.createTexture('health_bar_low', 344, 34, (ctx, w, h) => {
            const gradient = ctx.createLinearGradient(0, 0, 0, h);
            gradient.addColorStop(0, '#fbbf24');
            gradient.addColorStop(0.5, '#f59e0b');
            gradient.addColorStop(1, '#d97706');
            ctx.fillStyle = gradient;
            ctx.fillRect(0, 0, w, h);
        });

        // Critical health
        this.createTexture('health_bar_critical', 344, 34, (ctx, w, h) => {
            const gradient = ctx.createLinearGradient(0, 0, 0, h);
            gradient.addColorStop(0, '#ef4444');
            gradient.addColorStop(0.5, '#dc2626');
            gradient.addColorStop(1, '#b91c1c');
            ctx.fillStyle = gradient;
            ctx.fillRect(0, 0, w, h);
        });
    }

    /**
     * Generate hit effect sprites
     */
    generateHitEffects() {
        // Impact effect
        this.createTexture('hit_effect', 64, 64, (ctx, w, h) => {
            ctx.fillStyle = '#ffffff';
            // Star burst
            for (let i = 0; i < 8; i++) {
                ctx.save();
                ctx.translate(w / 2, h / 2);
                ctx.rotate((i * Math.PI) / 4);
                ctx.fillRect(-4, -30, 8, 25);
                ctx.restore();
            }
            // Center
            ctx.beginPath();
            ctx.arc(w / 2, h / 2, 10, 0, Math.PI * 2);
            ctx.fill();
        });

        // Block effect
        this.createTexture('block_effect', 48, 48, (ctx, w, h) => {
            ctx.strokeStyle = '#00ffff';
            ctx.lineWidth = 4;
            ctx.beginPath();
            ctx.arc(w / 2, h / 2, 20, 0, Math.PI * 2);
            ctx.stroke();
            ctx.beginPath();
            ctx.arc(w / 2, h / 2, 12, 0, Math.PI * 2);
            ctx.stroke();
        });
    }

    /**
     * Generate character portrait for selection screen
     */
    generatePortrait(name, colors) {
        this.createTexture(`${name}_portrait`, 150, 200, (ctx, w, h) => {
            // Background
            const gradient = ctx.createLinearGradient(0, 0, w, h);
            gradient.addColorStop(0, colors.outfit);
            gradient.addColorStop(1, colors.outfitSecondary);
            ctx.fillStyle = gradient;
            ctx.fillRect(0, 0, w, h);

            // Draw larger fighter head/bust
            ctx.fillStyle = colors.skin;
            ctx.fillRect(45, 60, 60, 60);

            // Hair
            ctx.fillStyle = colors.hair;
            ctx.fillRect(45, 50, 60, 25);
            ctx.fillRect(35, 60, 15, 35);
            ctx.fillRect(100, 60, 15, 35);

            // Eyes
            ctx.fillStyle = '#ffffff';
            ctx.fillRect(55, 80, 15, 15);
            ctx.fillRect(80, 80, 15, 15);
            ctx.fillStyle = '#000000';
            ctx.fillRect(62, 85, 8, 10);
            ctx.fillRect(87, 85, 8, 10);

            // Body hint
            ctx.fillStyle = colors.outfit;
            ctx.fillRect(35, 120, 80, 80);

            // Border
            ctx.strokeStyle = '#ffffff';
            ctx.lineWidth = 4;
            ctx.strokeRect(0, 0, w, h);
        });
    }
}

// Export for use in other modules
if (typeof window !== 'undefined') {
    window.SpriteGenerator = SpriteGenerator;
}
