# DRIFT

**A random-modulation firmware for MVMNT, by Mike.**

Not a Bézier curve generator. DRIFT is a *bounded random walk* — it steps to a new
random value, then decides how to travel there: instantly, in a straight line, or on
a smooth S-curve. One knob picks which. It is modelled on the random modulator in
Kilohearts Snap Heap, and it turns MVMNT into a modulation source that goes from a
20-second drift all the way up to a 1 kHz flutter.

![MVMNT](https://dl.modulove.de/module/mvmnt/Modulove_MVMNT_Productshot_Front.jpg)

---

## The story

Mike is a sound designer working in film, television and games. He also builds
Eurorack — but not to make music with it:

> I'm doing the modular dance more for sound design (film/tv/games) than music, so
> finding something that gave me a nicer motion curve than a basic slew limiter was
> kinda annoying! But then I almost immediately was like "ahhh but what if..."

He had bought two MVMNT kits, built one, and went looking for motion. In his day job
that motion comes from software — and it comes in bulk:

> On the software side (Kilohearts Snap Heap) I usually end up with 4, 8, 11,
> sometimes 27 different random generators because hey sometimes I have to make 300
> different fight whooshes! Or space octopus tentacles. Or evil robot drones...

So he rewrote the MVMNT firmware to behave like the thing he already trusted. Every
knob got a new job, the trigger input got a new job, and the output got a resolution
bump for good measure:

> I also bumped the output up to 9bit — could go to 10bit maybe but probably not
> really that necessary (the 9 bit probably isn't either) and would start getting some
> PWM oddities I think.

Then he sent it to us with four words:

> **But yeah! Share it with the community!**

So here it is. Thanks, Mike.

---

## What changed from stock MVMNT

The stock firmware draws a Bézier curve between random points. DRIFT keeps the
hardware and throws out the engine:

| | Stock MVMNT | DRIFT |
|---|---|---|
| **Engine** | Bézier curve between random points | Bounded random walk, reflecting at the rails |
| **New values** | Always a full-range jump | A *step* from where it already is — CHAOS sets how far |
| **Transition** | Always curved | Stairstep, linear ramp, or smoothstep — your choice |
| **Rate** | Frequency + randomised timing | Straight 0.05 Hz → 1 kHz, plus a freeze position |
| **TRIG input** | Track & Hold | "New value *now*" — trigger or clock in |
| **Output** | 8-bit PWM | **9-bit PWM** (512 levels), ~31 kHz carrier |

The jacks are untouched. DRIFT drives the same output pin the same way, so **OUT**,
**INV** and **BI** behave exactly as they always have — only the shape of the voltage
coming out of them is different.

> ### ⚠️ The printed panel legend does not apply
>
> All four knobs do something different under DRIFT. The silkscreened
> ELEVATE / STRETCH / SMOOTH / FLUCTUATE labels are **wrong** for this firmware.
> See [PANEL.md](PANEL.md) for the DRIFT legend and a panel-design guide.

---

## Controls

Physical positions are unchanged — top to bottom.

| Pos | Stock label | **DRIFT** | What it does |
|-----|-------------|-----------|--------------|
| 1 | ELEVATE | **DEPTH** | Output amount. CCW = flat line, CW = full swing. |
| 2 | STRETCH | **CHAOS** | How far each new value may jump from the current one. |
| 3 | SMOOTH | **SHAPE** | *Bipolar.* Centre = stairstep. CCW = linear ramps, CW = smoothstep. |
| 4 | FLUCTUATE | **RATE** | 0.05 Hz → 1 kHz, exponential. **Hard CCW = freeze.** |
| — | TRIG in | **TRIG** | Rising edge = new value immediately. |

### CHAOS — how far it wanders

This is the knob that makes DRIFT feel different from every other random source on
the module. A new value is a *step from the current value*, not a fresh roll of the
dice. Turn it down and the voltage creeps around where it already was; turn it up and
it can leap anywhere. When a step would run past 0 V or full scale it **reflects**
back inside the range instead of clipping, so the output keeps moving at the rails.

CHAOS never reaches zero — there is a 3 % floor, so the value always drifts a little.
That is deliberate.

### SHAPE — the interesting one

SHAPE is bipolar around 12 o'clock, and it controls two things at once:

- **Distance from centre** = how much of the interval is spent moving. At the centre,
  nothing is spent moving — the value jumps and holds. At either extreme, the ramp
  fills the whole interval.
- **Side of centre** = the curve. CCW ramps are **linear**, CW ramps are
  **smoothstep**.

| Knob | Result |
|------|--------|
| Full CCW | Linear ramps, full length |
| CCW half | Linear ramps that finish early, then hold — a "pulse width" feel |
| **12 o'clock** | **Stairstep — instant jump, then hold. This is sample & hold.** |
| CW half | Smoothstep ramps that finish early, then hold |
| Full CW | Smoothstep ramps, full length — the silkiest setting |

A centre detent is worth marking on a custom panel; 12 o'clock is a genuinely
useful position rather than just a midpoint.

### RATE — and the freeze zone

Exponential sweep from about one new value every 20 seconds up to 1 kHz. Because the
sweep is exponential, the middle of the knob is still fairly slow — most of the fast
range lives in the last quarter.

At the **hard CCW end** the knob enters a **freeze** zone: the internal clock stops
and the output holds. Nothing moves until you send a trigger. There is hysteresis on
the way in and out, so it will not chatter on the boundary.

### TRIG — three different jobs

A rising edge always does the same thing — *emit a new value now* — but what that
means depends on where RATE is:

- **RATE frozen (hard CCW), SHAPE centred** → classic **sample & hold**. Clock it,
  and you get one new voltage per trigger.
- **RATE frozen, SHAPE off centre** → clocked ramps. The ramp time follows the gap
  between your last two triggers, so it re-times itself to your clock.
- **RATE running** → triggers interleave with the free-running clock. Two unrelated
  rates in one output, which is the cheapest polyrhythm in the rack.

---

## Flashing

**From your browser** (Chrome, Edge or Opera — nothing to install):

### → [dl.modulove.de/module/mvmnt](https://dl.modulove.de/module/mvmnt/)

Pick **DRIFT**, tick the option that matches your board, and hit the button.

**From source**, if you would rather build it yourself:

```bash
arduino-cli core install arduino:avr
arduino-cli compile -b arduino:avr:nano Firmware/DRIFT
```

DRIFT runs on the Arduino Nano (either bootloader) and on LGT8F328P Nano clones. The
board is detected at compile time — there is nothing to configure. On the LGT8F the
32 MHz clock puts the PWM carrier at ~62.5 kHz instead of ~31 kHz, which the output
filter likes slightly better; everything else is identical.

Going back to stock MVMNT, or over to SyncLFO, is the same button on the same page.
Nothing here is one-way.

---

## Under the hood

For anyone who wants to modify it — the sketch is short and commented, and the
constants at the top are meant to be edited.

| | |
|---|---|
| Output | 9-bit fast PWM on OC1B (pin 10), TOP = 511, ~31 kHz at 16 MHz |
| Control rate | 5 kHz (`SAMPLE_INTERVAL_US`) |
| Knob scan | 50 Hz (`KNOB_INTERVAL_US`) |
| Rate range | `FREQ_MIN_HZ` 0.05 → `FREQ_MAX_HZ` 1000, exponential |
| Trigger debounce | 500 µs |
| Flash / RAM | 5622 B (18 %) / 69 B (3 %) on the Nano |

An 8-bit fallback is documented in `writeCV()` if you want the original resolution
back. `FREQ_MAX_HZ` is bounded by `MIN_INTERVAL_US` and by the 5 kHz control rate —
at 1 kHz there are only about five control steps per value, so ramps get coarse up
there. That is the intended ceiling, not a bug.

---

## Credits & license

**DRIFT firmware by Mike**, 2026. Shared with the MVMNT community with his blessing.

Built on the MVMNT / HAGIWO SyncLFO hardware. The original Bézier engine this
replaces is by [HAGIWO](https://note.com/solder_state/n/n39aacefd73a3) and released
CC0; hardware adaptation and panel design by [Modulove](https://modulove.io) with
panel art by [bkrsmdesign](https://www.instagram.com/bkrsmdesign/).

Released under the same terms as the rest of this repository, in the spirit of the
CC0 engine it grew out of.
