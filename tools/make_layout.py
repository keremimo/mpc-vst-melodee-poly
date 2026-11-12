#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 Kerem Kilic (Ellic Studio)
"""Melodee Poly's MPC page: vst/layout.conf from the tabs below (src/core/layout.py). Run after `make json`."""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "src"))
from core.layout import write  # noqa: E402

TABS = [
    ("OSC", [
        [("OSC A", [("osc_a_freq", "FREQ"), ("osc_a_pw", "PW"), ("osc_a_saw", "SAW"), ("osc_a_pulse", "PULSE"),
                    ("osc_a_sync", "SYNC")]),
         ("MIXER", [("mix_a", "OSC A"), ("mix_b", "OSC B"), ("mix_noise", "NOISE")])],
        [("OSC B", [("osc_b_freq", "FREQ"), ("osc_b_fine", "FINE"), ("osc_b_pw", "PW"), ("osc_b_saw", "SAW"),
                    ("osc_b_tri", "TRI"), ("osc_b_pulse", "PULSE"), ("osc_b_lo", "LO FREQ"), ("osc_b_keyb", "KEYB")])],
    ]),
    ("FILTER", [
        [("FILTER", [("cutoff", "CUTOFF"), ("resonance", "RESO"), ("filter_env", "ENV AMT"), ("key_track", "KEYBOARD"),
                     ("vel_filter", "VEL > FLT")]),
         ("AMP", [("vel_amp", "VEL > AMP")])],
        [("FILTER ENV", [("f_attack", "ATTACK"), ("f_decay", "DECAY"), ("f_sustain", "SUSTAIN"), ("f_release", "RELEASE")]),
         ("AMP ENV", [("a_attack", "ATTACK"), ("a_decay", "DECAY"), ("a_sustain", "SUSTAIN"), ("a_release", "RELEASE")])],
    ]),
    ("MOD", [
        [("LFO", [("lfo_rate", "RATE"), ("lfo_shape", "SHAPE"), ("lfo_amount", "AMOUNT"), ("lfo_freq", "> FREQ"),
                  ("lfo_pw", "> PW"), ("lfo_filter", "> FILTER")])],
        [("WHEEL", [("wheel_freq", "> FREQ"), ("wheel_filter", "> FILTER"), ("wheel_lfo", "> LFO")]),
         ("POLY MOD", [("pm_env", "FILT ENV"), ("pm_osc_b", "OSC B"), ("pm_freq_a", "> FREQ A"), ("pm_pw_a", "> PW A"),
                       ("pm_filter", "> FILTER")])],
    ]),
    ("PLAY", [
        [("PERFORM", [("glide", "GLIDE"), ("unison", "UNISON"), ("detune", "DETUNE"), ("vintage", "VINTAGE"),
                      ("bend_range", "BEND"), ("voices", "VOICES")])],
        [("TOUCH", [("at_vib", "AT > VIB"), ("at_bright", "AT > BRIGHT"), ("vib_rate", "VIB RATE")]),
         ("OUTPUT", [("volume", "VOLUME")])],
    ]),
]

if __name__ == "__main__":
    vst = os.path.join(HERE, "..", "vst")
    write(os.path.join(vst, "layout.conf"), os.path.join(vst, "params.json"), "FF7A5C", "FFA38C", TABS, "Melodee Poly")
