# ECHO GRID: PARADOX RUNNER

A maze game built in C using Raylib, made as a Level 1, Term 1 project by
Waseka Mehrin(2505077) and Tasnim Tabassum Tanim(2505082).

The game features three themed missions (jungle, cosmic, cyber), a
time-echo ghost mechanic, phase-flip maze layers, dash, scan, and a
score-saving system.

---

## Requirements

- Windows (tested on Windows 10/11)
- GCC (MinGW-w64) — https://www.mingw-w64.org/
- Raylib 6.0 — included in this repository under `raylib/`

Only GCC needs to be installed separately. Raylib is bundled here.

---

## How to Build and Run

Open a terminal in the project root folder (where `main.c` is located) and
run the command for your shell.

### PowerShell / CMD

```
gcc main.c -o main -I raylib/raylib-6.0_win64_mingw-w64/include -L raylib/raylib-6.0_win64_mingw-w64/lib -lraylib -lopengl32 -lgdi32 -lwinmm
main
```

### Git Bash

```
gcc main.c -o main -I raylib/raylib-6.0_win64_mingw-w64/include -L raylib/raylib-6.0_win64_mingw-w64/lib -lraylib -lopengl32 -lgdi32 -lwinmm
./main
```

If the build succeeds, the game window opens automatically.

---

## Controls

| Key              | Action                |
| ---------------- | --------------------- |
| W A S D / Arrows | Move                  |
| SHIFT            | Dash (2 cells)        |
| F                | Scan (reveals route)  |
| E                | Phase Flip            |
| R                | Restart level         |
| M                | Mute / unmute music   |
| ESC              | Back to menu          |

---

## How to Play

1. Reach the glowing goal tile at the bottom-right before the timer ends.
2. Grab loot for points — faster pickups build a combo multiplier.
3. Avoid the echo ghost of your past self — touching it triggers a
   paradox and costs a life.
4. Flip phases with E to turn walls into paths.
5. Use F to see the route to the goal.

You have 3 lives per mission. Damage from the echo ghost, rolling
boulders, sentinels, or running out of time ends the run.

---

## Saving

The game writes two files next to the executable at runtime:

- `leaderboard.txt` — top 5 scores
- `profile.txt` — your agent callsign and best times

These are created automatically the first time you play. They are not
included in the repository.

---

## Folder Structure

```
.
├── main.c
├── README.md
├── .gitignore
├── github_link.txt
├── assets/
│   └── (all .wav files)
└── raylib/
    └── raylib-6.0_win64_mingw-w64/
        ├── include/raylib.h
        └── lib/libraylib.a
```

---

## Credits

- Sound effects— Pixabay (https://pixabay.com/sound-effects/search/game/)
- Sound effects — Mixkit (https://mixkit.co/free-sound-effects/game/)
- Graphics — all drawn at runtime using Raylib primitives

Big thanks to our supervisor, Abdur Rafi Sir, for his guidance and support.

---

## License

This project was built for educational purposes as part of the CSE 102
course. Assets are used under their respective free licenses.
