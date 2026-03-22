# Diablo 1 Clone

## Project Overview
A Diablo 1 game clone written in C. Uses SDL2 for rendering, input, and audio.

## Language & Build
- **Language**: C (C11 standard)
- **Build**: Makefile with gcc/clang
- **Dependencies**: SDL2, SDL2_image, SDL2_mixer, SDL2_ttf

## Project Structure
```
src/           - C source files
include/       - Header files
assets/        - Game assets (sprites, tiles, audio, fonts)
build/         - Build output (gitignored)
```

## Skills
This project has 4 Claude Code skills installed:

- **gemini-image**: Generate game assets (sprites, tiles, icons, UI elements) using Gemini AI
- **gemini-cli**: Get second-opinion code reviews and web search via Gemini CLI
- **codex-cli**: Cross-verify code with OpenAI Codex for independent analysis
- **agent-teams**: Coordinate parallel work with multiple AI teammates

## Asset Generation
Use the `gemini-image` skill for generating game assets:
- Sprites: `python3 .claude/skills/gemini-image/scripts/generate_image.py "pixel art dark fantasy warrior sprite sheet, 32x32 grid" -o assets/sprites/warrior.png --aspect 1:1`
- Tiles: `python3 .claude/skills/gemini-image/scripts/generate_image.py "dark dungeon floor tile, seamless, pixel art style" -o assets/tiles/floor.png --aspect 1:1`
- UI: `python3 .claude/skills/gemini-image/scripts/generate_image.py "dark fantasy game UI health bar, pixel art" -o assets/ui/healthbar.png --aspect 16:9`

## Conventions
- Use snake_case for functions and variables
- Use UPPER_SNAKE_CASE for constants and macros
- Header guards: `#ifndef DIABLO_MODULE_NAME_H`
- Keep functions short and focused
- Comment non-obvious game logic
