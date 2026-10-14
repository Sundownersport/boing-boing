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

Locally, with git, make, a C compiler and Node.js:

```
make prereqs
make import
make
```

`make prereqs` builds cc65 and zopfli into `tools/` once. The ROM is written to `bin/boing.gtr`.

On GitHub, run the Build workflow from the Actions tab and download `boing-rom` from the run's artifacts, or run `scripts/ci_build.sh`, which starts the workflow, waits for it and downloads the ROM to `bin/`.

In VS Code, both are under Terminal > Run Build Task (Ctrl+Shift+B).

Open the ROM in the [GameTank emulator](https://github.com/clydeshaffer/GameTankEmulator).

Built on [gametank_sdk](https://github.com/clydeshaffer/gametank_sdk).
