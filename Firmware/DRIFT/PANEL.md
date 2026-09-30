# DRIFT — panel legend & design guide

DRIFT reassigns all four knobs, so the stock MVMNT silkscreen
(ELEVATE / STRETCH / SMOOTH / FLUCTUATE) **does not describe what the module does**
under this firmware. This is the legend it should have, and what a custom panel needs
to get right.

The four holes do not move — only the artwork changes.

---

## The legend

| Pos | Pin | Stock label | **DRIFT** | Sub-label |
|-----|-----|-------------|-----------|-----------|
| 1 | A0 | ELEVATE | **DEPTH** | output amount |
| 2 | A1 | STRETCH | **CHAOS** | step size |
| 3 | A3 | SMOOTH | **SHAPE** | linear ◄ step ► smooth |
| 4 | A5 | FLUCTUATE | **RATE** | freeze ◄ 0.05 Hz — 1 kHz |
| — | D3 | ↘ (trig) | **TRIG** | new value / clock |
| — | D10 | OUT / INV / BI | unchanged | 9-bit CV |

Jack positions and functions are unchanged from stock: **TRIG** top-left,
**INV** top-right, **OUT** bottom-left, **BI** bottom-right, LED in the middle.

---

## What the artwork has to communicate

Three of the four knobs are not plain unipolar sweeps, and a panel that ignores that
makes the firmware much harder to use.

### 1 · DEPTH — plain

Linear, unipolar attenuator. CCW = 0 V flat, CW = full swing. Nothing special;
a simple thin→thick wedge reads fine. No detent.

### 2 · CHAOS — plain, but never zero

Linear, unipolar. CCW = tiny steps, CW = full-range leaps. There is a 3 % floor, so
it never fully stops — **do not print a "0" at the CCW end**, it would be a lie.
Small-ripple icon → jagged-leap icon works well. No detent.

### 3 · SHAPE — **bipolar, needs a centre detent** ★

The most important graphic on the panel. Distance from centre sets how much of the
interval is spent ramping; the side of centre sets the curve.

```
LINEAR  ◄————————  STEP / S&H  ————————►  SMOOTH
 full CCW            12 o'clock             full CW
```

- Mark **12 o'clock** clearly — a notch, a tick, a detent dot. It is the sample &
  hold position and users need to find it by feel.
- Split arc: left half `LINEAR`, right half `SMOOTH`, centre tick `STEP`.
- Mini waveform icons at the extremes read instantly: a ramp on the left, an S-curve
  on the right, a staircase in the middle.

### 4 · RATE — **exponential, with a freeze zone at hard CCW** ★

```
[FREEZE] 0.05 Hz ——————— 7 Hz ———————►  1 kHz
  hard CCW      (12 o'clock)      full CW
```

- Freeze is **not a band, it is the hard stop**. The firmware enters freeze below ADC
  8/1023 and leaves it above 24/1023 — under 1 % of rotation — so draw it as an
  **end-cap marker** at the CCW limit (snowflake or pause glyph) with a leader to the
  word FREEZE, not as a wide zone. A wide band would be a lie about where it bites.
- Useful scale marks, if you want numbers: **0.05 Hz** at the CCW end, **~7 Hz** at
  12 o'clock, **1 kHz** at full CW.
- The sweep is exponential, so most of the fast range lives in the last quarter of
  the rotation. A wedge that grows sharply toward CW is more honest than an even one.

---

## Hole geometry (measured from the stock panel template)

6 HP × 3 U — **30.0 × 128.5 mm**. Origin = top-left corner of the panel.

| Feature | X (mm) | Y (mm) | Ø (mm) |
|---|---|---|---|
| Pot 1 — DEPTH | 15.00 | 26.27 | 7.2 |
| Pot 2 — CHAOS | 15.00 | 44.68 | 7.2 |
| Pot 3 — SHAPE | 15.00 | 63.10 | 7.2 |
| Pot 4 — RATE | 15.00 | 81.51 | 7.2 |
| Jack — TRIG | 6.75 | 98.54 | 6.2 |
| Jack — INV | 23.25 | 98.54 | 6.2 |
| Jack — OUT | 6.75 | 109.97 | 6.2 |
| Jack — BI | 23.25 | 109.97 | 6.2 |
| LED | 15.00 | 104.25 | 3.0 |

Pot spacing is 18.415 mm. Jacks sit on a 16.51 × 11.43 mm grid; allow **9.2 mm** clear
for the jack nut/washer, which is why label text has to sit outside that circle.

Mounting slots: centre **X = 7.45 mm**, at **Y = 3.0 mm** and **Y = 125.5 mm** —
5.9 mm long × 3.2 mm high with 1.6 mm rounded ends.

Source templates (stock SyncLFO front): `HAGIWO_Sync_LFO_Front.svg`,
`HAGIWO Sync LFO Front.DXF`, `HAGIWO Sync LFO Front.STEP`.

---

## Style notes

The stock MVMNT panel is by [bkrsmdesign](https://www.instagram.com/bkrsmdesign/):
light labels in rounded pills, dotted tick arcs around each knob, flowing organic
line art in gold on charcoal, wide-tracked `MVMNT` header with a waveform under it.

A DRIFT panel should stay in that family and change the accent:

- **Identity colour: amber / orange.** Charcoal panel, warm accent line art.
- **DEPTH** neutral/white · **CHAOS** amber · **SHAPE** split (cool = linear,
  warm = smooth) · **RATE** warm with a cold frozen tip at CCW.
- Header: `MVMNT // DRIFT`, with a small `by Mike` credit line.
- Only the legend art changes — keep all four holes and the LED exactly where they are.
