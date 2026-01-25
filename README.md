# Street Brawler - 2D Fighting Game

A browser-based 2D fighting game built with modern web technologies.

![Street Brawler](https://img.shields.io/badge/Game-Fighting-red)
![Phaser 3](https://img.shields.io/badge/Phaser-3.60-blue)
![JavaScript](https://img.shields.io/badge/JavaScript-ES6+-yellow)

## Features

- **4 Unique Fighters** - Each with distinct fighting styles and special moves
  - **Ryu** - Balanced warrior with energy projectiles (Hadouken)
  - **Ken** - Speed fighter with flaming uppercut (Shoryuken)
  - **Blaze** - Power fighter with devastating ground pounds
  - **Shadow** - Technical ninja with teleportation abilities

- **Game Modes**
  - VS Mode - 2-player local multiplayer
  - VS CPU - Single player against AI opponent

- **Combat System**
  - Punch, kick, and special attacks
  - Blocking mechanics
  - Combo system with hit counter
  - Health bars and round timer

- **Technical Features**
  - Procedurally generated pixel-art sprites
  - Synthesized sound effects using Web Audio API
  - Smooth animations and visual effects
  - Responsive design

## How to Play

### Quick Start

1. Open `index.html` in a modern web browser
2. Select game mode (VS or VS CPU)
3. Choose your fighter
4. Fight!

### Controls

#### Player 1
| Action | Key |
|--------|-----|
| Move Up | W |
| Move Down | S |
| Move Left | A |
| Move Right | D |
| Punch | F |
| Kick | G |
| Block | H |
| Special Move | R |

#### Player 2
| Action | Key |
|--------|-----|
| Move Up | Up Arrow |
| Move Down | Down Arrow |
| Move Left | Left Arrow |
| Move Right | Right Arrow |
| Punch | J |
| Kick | K |
| Block | L |
| Special Move | U |

#### General
| Action | Key |
|--------|-----|
| Pause | ESC |
| Confirm/Select | Enter/Space |

## Technologies Used

- **[Phaser 3](https://phaser.io/)** - HTML5 game framework
- **[Howler.js](https://howlerjs.com/)** - Audio library
- **HTML5 Canvas** - Rendering
- **Web Audio API** - Synthesized sound effects
- **CSS3** - UI styling with retro effects

## Project Structure

```
├── index.html              # Main entry point
├── css/
│   └── styles.css          # Game styling
├── js/
│   ├── game.js             # Main game configuration
│   ├── utils/
│   │   └── SpriteGenerator.js  # Procedural sprite generation
│   ├── characters/
│   │   ├── Fighter.js      # Base fighter class
│   │   └── fighters/
│   │       ├── Ryu.js      # Ryu character
│   │       ├── Ken.js      # Ken character
│   │       ├── Blaze.js    # Blaze character
│   │       └── Shadow.js   # Shadow character
│   └── scenes/
│       ├── BootScene.js    # Asset loading
│       ├── MenuScene.js    # Main menu
│       ├── CharacterSelectScene.js  # Fighter selection
│       ├── FightScene.js   # Main gameplay
│       └── GameOverScene.js # Match results
└── README.md
```

## Fighter Guide

### Ryu (Balanced)
- **Style:** All-around fighter
- **Special Move:** Hadouken - Energy projectile that travels across the screen
- **Strategy:** Good for beginners, balanced stats

### Ken (Speed)
- **Style:** Fast and aggressive
- **Special Move:** Shoryuken - Rising uppercut with flame effect
- **Strategy:** Rush down opponents, chain quick combos

### Blaze (Power)
- **Style:** Heavy hitter
- **Special Move:** Inferno Smash - Devastating ground pound with super armor
- **Strategy:** Trade hits using super armor, deal massive damage

### Shadow (Technical)
- **Style:** Tricky and evasive
- **Special Move:** Shadow Step - Teleport behind opponent and strike
- **Strategy:** Use perfect blocks to counter, outmaneuver opponents

## Browser Support

- Chrome (recommended)
- Firefox
- Safari
- Edge

## Local Development

No build process required! Simply serve the files using any local web server:

```bash
# Using Python
python -m http.server 8000

# Using Node.js
npx serve

# Using PHP
php -S localhost:8000
```

Then open `http://localhost:8000` in your browser.

## License

This project is open source and available for personal and educational use.

---

**Enjoy the fight!**
