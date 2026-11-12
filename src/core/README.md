# core/

What the three Melodee synth plug-ins share, one identical copy in each repository
([mpc-vst-melodee-poly](../../../mpc-vst-melodee-poly), [-pd](../../../mpc-vst-melodee-pd),
[-chip](../../../mpc-vst-melodee-chip)), so each builds on its own the way the Open MPC toolchain expects (a port
vendors what it compiles). A change here goes into all three.

| File | What |
|---|---|
| `dsp.h` | Oscillators, the ladder filter, envelopes, LFO, noise. 44.1 kHz, the Open MPC engine contract. |
| `expression.h`, `mod_matrix.h` | The mod matrix (sources, destinations, sums), velocity curve, pressure, slides. |
| `control_step.h` | Modulation stepped on the synth's own frames, so any block cut plays the same to the sample. |
| `synth.h` | `Synth` (the voice section), `ParamInfo`, `Sound`, `SynthInfo`: one table states every parameter. |
| `plugin.h` | The synth as `mpc_engine_t`: parameters by key, MIDI, int16 out, the state and a Melodee track's import. |
| `gen_json.h` | `vst/params.json` and `vst/presets.json` from the tables (`make json`). |
| `test_synth.h` | The host checks every synth runs (`make test`). |
| `layout.py` | `vst/layout.conf`, the MPC page, from a short spec (`tools/make_layout.py`). |

The DSP came out of Melodee (melodee-sample, `src/melodee/engine/`, at commit `1fbca79`), where it ran as the
built-in PROPHET, CZ and SID engines. Taken out unchanged but for what only Melodee's host used (the sequencer's
accent, the overload guard): rendered side by side with Melodee's engines from that commit, every factory sound,
with routes, slides, pressure, wheel, bend and odd block cuts, the three plug-ins' output is identical to the bit.
