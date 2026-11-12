// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
// Host checks for Melodee Poly (core/test_synth.h), plus what is its own: unison stacking every voice on one key,
// and the time and rate values reading as the screen shows them.
#include "core/test_synth.h"
#include "poly.h"

using namespace melodee;
using namespace melodee::test;

namespace {
void unison() {
    Rig<PolySynth> r;
    r.set("unison", "1");
    r.midi(0x90, 48, 100);
    auto a = r.play(8820);
    MELODEE_CHECK(rms(a) > 0.01);
    r.midi(0x90, 55, 100); // legato: every voice moves to the newest key
    r.midi(0x80, 55, 0);   // and back to the one still held
    auto b = r.play(8820);
    MELODEE_CHECK(rms(b) > 0.01);
    r.midi(0x80, 48, 0);
    r.play(44100 * 12);
    MELODEE_CHECK(!r.plugin().synth.sounding());
}

void readouts() {
    Rig<PolySynth> r;
    r.set("a_release", "35");
    MELODEE_CHECK(r.get("a_release_display") == "25 ms");
    r.set("a_release", "100");
    MELODEE_CHECK(r.get("a_release_display") == "10 s");
    MELODEE_CHECK(r.get("lfo_rate_display") == "1.7 Hz");
    MELODEE_CHECK(r.get("osc_b_fine_display") == "+7 ct");
    MELODEE_CHECK(r.get("osc_a_freq_display") == "+0 st");
    r.set("lfo_shape", "2");
    MELODEE_CHECK(r.get("lfo_shape") == "2");
}
} // namespace

int main() {
    unison();
    readouts();
    return run_all<PolySynth>("Melodee Poly");
}
