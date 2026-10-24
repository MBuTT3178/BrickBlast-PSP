# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [v1.1.0] - 2026-10-24

### Added
- **3-Move Streak Grace Window:** Mirroring the official mobile mechanics, active streaks now grant a 3-move grace counter. Placing a non-clearing piece decrements the counter without breaking the streak; clearing a line resets the counter back to 3 and increments the streak tier.
- **HUD Streak Pips:** Real-time visual pip counters (`●●●` $\rightarrow$ `●●○` $\rightarrow$ `●○○`) rendered on the top-left HUD showing exact moves remaining before streak expiration. Turns red on the final warning move.
- **Streak Lost Notification:** Immediate visual feedback when the 3-move window expires and streak resets.
- **Hardware Pause Engine:** Pressing **START** toggles an instantaneous modal pause overlay with options to resume (**START**) or restart the run (**SELECT**).
- **Circle ($\bigcirc$) Deck Control:** Pressing Circle returns the selected piece to the deck and cycles placement selection.
- **Dynamic Revive Price Escalation:** Revive pricing now dynamically scales per run to reward skillful play:
  $$\text{Cost} = 100 + (\text{revives\_used} \times 50)$$
  ($100 \rightarrow 150 \rightarrow 200 \rightarrow 250\dots$).
- **Revive Counter:** Added real-time tracking of `revive_count`, displayed in both the Second Chance Revive modal and the Game Over breakdown.

### Changed
- **De-collided Two-Tier HUD:** Completely overhauled header layout into a symmetrical two-row dashboard. Player score is dynamically measured and centered at $X = 240$ with $> 35\text{px}$ clearance on either side, completely eliminating text overlap between Score, Coins, and Streak.
- **Official Title Branding:** Renamed project to **Brick Blast (PSP Edition)** across `PARAM.SFO` executable metadata, `Makefile`, in-game header, and distribution packaging.
- **Persistent Save Migration:** Save subsystem automatically migrates legacy high scores and coin balances to `ms0:/PSP/SAVEDATA/BRICKBLAST/score.dat`.

---

## [v1.0.0] - 2026-10-24

### Added
- **Engine Core:** Native C99 game engine engineered specifically for 32MB PSP-1000 hardware and PPSSPP.
- **Direct VRAM Pipeline:** 60 FPS double-buffered rendering across `0x44000000` and `0x44088000` with zero frame drops or tearing.
- **34 Piece Geometries:** Authentic block shapes with 8×8 grid collision testing, matrix rotation, and ghost preview projection.
- **Adaptive Spawning Engine:** Anti-lockout solvability guarantees with 75% bias toward line-clearing candidate pieces.
- **10-Note Harmonic Streak Audio:** 22,050 Hz PCM scale linearly interpolated to 44,100 Hz hardware audio output, ascending with consecutive streak tiers.
- **Persistent Coin Economy:** 200 starter coins, $+5$ per line clear, $+8$ multi-clear bonus, and $+2 \times \text{streak}$ multiplier saved to Memory Stick.
- **Second Chance Revive Mechanic:** Interactive modal detonating a $4\times 4$ center blast and restocking 3 guaranteed-fitting pieces on board lockout.
