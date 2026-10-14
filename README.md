# Boing Boing!

A GameTank game based on The Bouncing Ball (GBJam 2015, Game Boy).

The ball bounces on its own. Steer it left and right with the d-pad and collect every heart to clear the level. Start restarts the level.

| Tile | Effect |
| --- | --- |
| Grey block | Normal bounce |
| Orange block | Breaks after you bounce on it |
| Green block | High bounce |
| Cyan block | Launches you sideways |
| Red spikes | Restart level |

## Building

GitHub Actions builds the ROM on every push. Download `boing.gtr` from the run's artifacts and open it in the [GameTank emulator](https://github.com/clydeshaffer/GameTankEmulator).

Built on [gametank_sdk](https://github.com/clydeshaffer/gametank_sdk).
