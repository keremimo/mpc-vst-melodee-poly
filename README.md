# Melodee Poly

A two-oscillator polysynth as a native plug-in for Akai MPC OS standalone devices (MPC Live / One / X / Key,
Force; Gen1 armv7 and Gen2 aarch64) and for [Melodee](https://github.com/keremimo/melodee-sample) on the MPC
Sample, built with the [Open MPC](https://open-mpc.github.io) toolchain: a VST2 instrument with its own MPC
screen page, Q-Link pages and factory presets in MPC's PRESET menu.

It is Melodee's PROPHET engine, out of Melodee as a plug-in of its own: the same voice to the sample (see
"Where it comes from").

**Status: 1.0.0, not released.** The engine passes its host checks, and the armv7 and aarch64 builds, their skin
and the toolchain's port test (`tools/test_port.sh`) pass; the smoke test on an MPC (docs/RELEASING.md in
mpc-vst-plugins) is still to do. `tested.json` lists no device yet.

## What it is

- **Up to eight voices**, each two oscillators, noise, a four-pole ladder low pass with its own envelope, and an
  amp envelope. Poly (1-8 voices, a released voice first, then the oldest) or **unison** (every voice on the
  newest key, detuned; letting go falls back to a key still held). One voice is a mono synth with legato.
- **Oscillator A:** saw and pulse, mixed; pulse width; hard sync to B.
- **Oscillator B:** saw, triangle and pulse, mixed; pulse width; fine tune; LO FREQ (an LFO-rate mode) and
  KEYBOARD (off: a fixed pitch).
- **Poly mod:** the filter envelope and oscillator B into oscillator A's frequency, its pulse width and the cutoff.
- **LFO** (triangle, saw, square, random) into both oscillators' pitch, pulse width and cutoff; **the mod wheel**
  into pitch, cutoff and LFO depth.
- **Glide**, pitch bend (range 0-12), **vintage** (each voice drifts a little), velocity into cutoff and level.
- **Pressure** (poly and channel aftertouch): vibrato at its own rate and a brighter filter.
- **A mod matrix** of eight routes: LFO, velocity, aftertouch, wheel, key, random or the filter envelope into
  cutoff, resonance, level, pitch, either oscillator's pitch, pulse width, LFO rate or noise.
- **13 factory sounds**: Init Voice, Funk Bass II, Rubber Bass, Sync Lead, Brass Stab, Soft Pad, Glass Pluck,
  Warm Keys, Sub Bass, String Machine, Poly Sweep, Unison Lead, Noise Hat. A sound sets the voice; the mod matrix
  and Volume stay as you set them.

## The MPC page

Five tabs, each two rows of eight controls that are MPC's two banks of eight Q-Links (row 1 is Q-Links 1-8,
row 2 is 9-16): **OSC** (oscillator A, the mixer; oscillator B), **FILTER** (filter, velocity; both
envelopes), **MOD** (LFO; wheel, poly mod), **PLAY** (glide, unison, detune, vintage, bend, voices; pressure,
Volume) and **MATRIX** (eight routes as columns of source, destination and amount; Q-Link pages MOD 1-4 and MOD
5-8). Melodee's NIGHT palette with a coral accent.

## MIDI

| Message | Does |
|---|---|
| Note on / off | plays (velocity 0 is a release) |
| Pitch bend | by BEND RANGE semitones |
| CC 1 | the mod wheel |
| Poly pressure, channel pressure | a held key's pressure / every held key's |
| CC 64 | sustain pedal |
| CC 84, then a note | **slide**: the voice of the key CC 84 named goes on as the new note without a new attack, gliding (at GLIDE's rate, else a 60 ms slide) |
| CC 68 down, CC 84, then a note | **tie**: the same, without the glide |
| CC 120 / 123 / 121 | all sound off / all notes off / reset controllers |
| CC 20-35, NRPN | the first page's Q-Links / any parameter by number (the Open MPC wrapper's) |

Melodee sends CC 84 and CC 68 for its sequencer's Slide and Tie steps. The plug-in answers on every channel.

## The state

What a project saves (VST2 chunk) is text: `melodee-poly 1`, then one `key=value` line per parameter (keys as in
`vst/params.json`; an option may be given by name). Keys it does not know are passed over and the ones it does
not find stay at their initial values, so older states load. Two more lines import a Melodee track as Melodee's
project file kept it: `melodee=<values in the order of src/poly.h's Param>` (a project from before pad pressure
names fewer: the rest stay at their initial values) and `melodee_mods=<source>:<destination>:<amount>,...` (the
eight routes, sources and destinations numbered as in `src/core/mod_matrix.h`). Melodee writes these when it opens
a project made while PROPHET was built in.

## Building

The device builds use [mpc-vst-plugins](https://github.com/sd88me/mpc-vst-plugins) (MIT), checked out beside
this repository (or named by `MPC_VST`), and Docker:

```sh
vst/build.sh            # armv7 and aarch64: vst/build/melodee_poly.so, vst/build/aarch64/melodee_poly.so, the skin
```

`vst/build.sh` calls the toolchain's `tools/build_port.sh` with `vst/vst.json`: the skin from `vst/layout.conf`, its
artwork drawn in a browser (`"art": "html"`, the toolchain's `mpc-vst-html-art` image) in Melodee's Rubik
(`vst/skin.css`, `vst/fonts/`),
`params.h` from `vst/params.json`, the presets from `vst/presets.json` and the `.so` linked with the toolchain's
VST2 wrapper (44.1 kHz, sample-accurate notes). Then `tools/test_port.sh vst/vst.json` (the wrapper's own host
test), a CPU bench and `tools/release.py` make the `-mpc-armv7.zip` and `-mpc-aarch64.zip` (the toolchain's
docs/RELEASING.md).

On the host, with a C++17 compiler:

```sh
make test               # the synth's checks under ASan and UBSan, and that the JSON files are current
make json               # vst/params.json and vst/presets.json from the tables in src/poly.cpp
python3 tools/make_layout.py    # vst/layout.conf from the tab spec in that script
```

The parameter table in `src/poly.cpp` is the one place a parameter is stated: the JSON files and the layout are
made from it. Parameters only ever go on the end (projects and skins bind by index; the toolchain's
`vst/params.lock`, written by the first build, holds them to it).

## Installing

- **MPC OS (Gen1, Gen2):** the release zips carry their own `install.sh` and `INSTALL.md` (root SSH; the
  toolchain's installer app, Plugin Manager VST or `mpc-store.sh` install them too).
- **Melodee on the MPC Sample:** copy the plug-in folder (`Ellic Studio - VST - Melodee Poly`, from the
  aarch64 zip's `portable/`) into `Plugins/` at the top of the SD card. Melodee plays it on a PLUGIN track, and a
  project saved with Melodee's built-in PROPHET engine (Melodee 0.1.0-alpha.6 and before) opens with that track
  on Melodee Poly, as it sounded.

## Where it comes from

Melodee Poly is Melodee's PROPHET engine (melodee-sample, `src/melodee/engine/prophet.cpp`, commit `1fbca79`),
written from scratch for Melodee. Its DSP is unchanged: rendered side by side with Melodee's engine, every
factory sound with mod routes, slides, pressure, wheel, bend and odd block cuts is identical to the bit. Only
what Melodee's own host used is gone (the sequencer's accent, which Melodee now sends as velocity, and its
overload guard). It is an independent design, not a model or copy of any instrument; the factory sounds were
made for it.

## Licence and credit

AGPL-3.0-only (`LICENSE`). Melodee Poly is the work of Kerem Kiliç (Ellic Studio), from Melodee. The Open MPC
toolchain and its VST2 wrapper, which the build links, are sd88me's (MIT). Rubik (`vst/fonts/Rubik-VF.ttf`, the
skin's typeface) is The Rubik Project Authors', under the SIL Open Font License 1.1 (`vst/fonts/OFL.txt`).
