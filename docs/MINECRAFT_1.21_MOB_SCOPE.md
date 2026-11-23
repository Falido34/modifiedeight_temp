# Minecraft 1.21.x mob scope

This project targets the mobs introduced across the Minecraft 1.21 release line, not every mob present in Minecraft 1.21.x.

| First release | Mob | Notes |
| --- | --- | --- |
| 1.21.0 | Breeze | Hostile Trial Chambers mob; wind-charge projectile and knockback behavior |
| 1.21.0 | Bogged | Skeleton variant; poison arrows, slower attack cadence, swamp spawning |
| 1.21.4 | Creaking | Pale Garden mob linked to a Creaking Heart; movement/attack depends on player line of sight and heart state |
| 1.21.6 | Happy Ghast | Rideable flying mob; grows from Ghastling |
| 1.21.6 | Ghastling | Juvenile life stage of Happy Ghast; follows and grows into the adult |

## Scope note

The Armadillo currently present in the feature branch was introduced earlier:
- Java Edition 1.20.5
- Bedrock Edition 1.20.80

It is useful as an additional compatibility target, but it is not counted as a mob introduced in Minecraft 1.21.0. Its current implementation in this repository is only a first pass and is not feature-complete.

## Implementation checklist

For each mob, implementation is not complete until all applicable parts are integrated:
- entity class, unique entity ID, factory and save/load registration
- AI, movement, combat/defense, interactions and spawn rules
- renderer, model, texture and animation
- sounds and language keys
- drops, items, projectiles or blocks required by its behavior
- Android build and in-game validation

Official release references:
- Java 1.21: https://feedback.minecraft.net/hc/en-us/articles/27547857163917-Minecraft-Java-Edition-1-21-Tricky-Trials
- Java 1.21.4: https://www.minecraft.net/en-us/article/minecraft-java-edition-1-21-4
- Java 1.21.6: https://www.minecraft.net/en-us/article/minecraft-java-edition-1-21-6
- Java 1.20.5 Armadillo: https://feedback.minecraft.net/hc/en-us/articles/26136167989005-Minecraft-Java-Edition-1-20-5-Armored-Paws
