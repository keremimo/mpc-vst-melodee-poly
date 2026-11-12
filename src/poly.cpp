// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#include "poly.h"
#include <algorithm>
#include <cmath>

namespace melodee {
using namespace poly;

namespace {
const char* const key_track[] {"Off", "Half", "Full"};
const char* const lfo_shapes[] {"Triangle", "Saw", "Square", "Random"};
const char* const off_on[] {"Off", "On"};

constexpr ParamInfo P(const char* key, const char* name, int min, int max, int initial, Format format = Format::Percent,
                      const char* const* choices = nullptr) {
    return {key, name, int16_t(min), int16_t(max), int16_t(initial), format, choices};
}
constexpr ParamInfo S(const char* key, const char* name, int initial) { return P(key, name, 0, 1, initial, Format::Switch, off_on); }

const ParamInfo params[Count] {
    P("osc_a_freq", "Osc A Freq", -24, 24, 0, Format::Semitones), S("osc_a_saw", "Osc A Saw", 1),
    S("osc_a_pulse", "Osc A Pulse", 0), P("osc_a_pw", "Osc A PW", 5, 95, 50), S("osc_a_sync", "Osc A Sync", 0),
    P("osc_b_freq", "Osc B Freq", -24, 24, 0, Format::Semitones), P("osc_b_fine", "Osc B Fine", -50, 50, 7, Format::Cents),
    S("osc_b_saw", "Osc B Saw", 1), S("osc_b_tri", "Osc B Tri", 0), S("osc_b_pulse", "Osc B Pulse", 0),
    P("osc_b_pw", "Osc B PW", 5, 95, 50), S("osc_b_lo", "Osc B Lo Freq", 0), S("osc_b_keyb", "Osc B Keyboard", 1),
    P("mix_a", "Mix Osc A", 0, 100, 80), P("mix_b", "Mix Osc B", 0, 100, 60), P("mix_noise", "Mix Noise", 0, 100, 0),
    P("cutoff", "Cutoff", 0, 100, 62), P("resonance", "Resonance", 0, 100, 28), P("filter_env", "Filter Env Amount", 0, 100, 45),
    P("key_track", "Key Track", 0, 2, 1, Format::Choice, key_track), P("vel_filter", "Vel > Filter", 0, 100, 40),
    P("f_attack", "Filter Attack", 0, 100, 0, Format::Time), P("f_decay", "Filter Decay", 0, 100, 45, Format::Time),
    P("f_sustain", "Filter Sustain", 0, 100, 30), P("f_release", "Filter Release", 0, 100, 35, Format::Time),
    P("a_attack", "Amp Attack", 0, 100, 0, Format::Time), P("a_decay", "Amp Decay", 0, 100, 50, Format::Time),
    P("a_sustain", "Amp Sustain", 0, 100, 80), P("a_release", "Amp Release", 0, 100, 35, Format::Time),
    P("vel_amp", "Vel > Amp", 0, 100, 65),
    P("lfo_rate", "LFO Rate", 0, 100, 55, Format::Rate), P("lfo_shape", "LFO Shape", 0, 3, 0, Format::Choice, lfo_shapes),
    P("lfo_amount", "LFO Amount", 0, 100, 0), P("lfo_freq", "LFO > Freq", 0, 100, 10), P("lfo_pw", "LFO > PW", 0, 100, 0),
    P("lfo_filter", "LFO > Filter", 0, 100, 0),
    P("wheel_freq", "Wheel > Freq", 0, 100, 15), P("wheel_filter", "Wheel > Filter", 0, 100, 30),
    P("wheel_lfo", "Wheel > LFO", 0, 100, 60),
    P("pm_env", "Poly Mod Env", 0, 100, 0), P("pm_osc_b", "Poly Mod Osc B", 0, 100, 0), S("pm_freq_a", "Poly Mod > Freq A", 0),
    S("pm_pw_a", "Poly Mod > PW A", 0), S("pm_filter", "Poly Mod > Filter", 0),
    P("glide", "Glide", 0, 100, 0), S("unison", "Unison", 0), P("detune", "Unison Detune", 0, 100, 20),
    P("vintage", "Vintage", 0, 100, 15), P("bend_range", "Bend Range", 0, 12, 2, Format::Semitones),
    P("voices", "Voices", 1, 8, 8, Format::Number),
    P("at_vib", "AT > Vibrato", 0, 100, expression::default_at_vib), P("at_bright", "AT > Bright", 0, 100, expression::default_at_bright),
    P("vib_rate", "Vibrato Rate", 0, 100, expression::default_vib_rate, Format::Rate),
};

// Every destination of the matrix, under its common name.
const ModDest dests[] {ModDest::None, ModDest::Cutoff, ModDest::Resonance, ModDest::AmpLevel, ModDest::Pitch,
                       ModDest::OscAPitch, ModDest::OscBPitch, ModDest::PulseWidth, ModDest::LfoRate, ModDest::Noise};
const char* const dest_names[] {"None", "Cutoff", "Resonance", "Amp level", "Pitch", "Osc A pitch", "Osc B pitch",
                                "Pulse width", "LFO rate", "Noise"};

// The factory sounds, made for this synth (not copied from any instrument's programs): what differs from the
// initial values.
const Sound sounds[] {
    {"Init Voice", {{{-1, 0}}}},
    {"Funk Bass II", {{{FreqA, -12}, {FreqB, -12}, {FineB, 5}, {PulseB, 1}, {SawB, 0}, {LevelB, 70},
                       {Cutoff, 38}, {Resonance, 35}, {EnvAmount, 62}, {FilterDecay, 38}, {FilterSustain, 10},
                       {AmpDecay, 45}, {AmpSustain, 60}, {AmpRelease, 18}, {Voices, 1}, {Glide, 8}, {-1, 0}}}},
    {"Rubber Bass", {{{FreqA, -12}, {SawA, 0}, {PulseA, 1}, {PwA, 30}, {FreqB, -24}, {SawB, 0}, {TriB, 1},
                      {Cutoff, 30}, {Resonance, 55}, {EnvAmount, 55}, {FilterDecay, 42}, {FilterSustain, 0},
                      {AmpSustain, 70}, {AmpRelease, 20}, {Voices, 1}, {-1, 0}}}},
    {"Sync Lead", {{{Sync, 1}, {FreqA, 7}, {LevelB, 30}, {Cutoff, 70}, {Resonance, 20}, {EnvAmount, 30},
                    {PolyEnv, 55}, {PolyFreqA, 1}, {FilterDecay, 55}, {FilterSustain, 20}, {Voices, 1}, {Glide, 15},
                    {LfoAmount, 15}, {-1, 0}}}},
    {"Brass Stab", {{{FineB, 9}, {Cutoff, 42}, {Resonance, 15}, {EnvAmount, 60}, {FilterAttack, 30}, {FilterDecay, 45},
                     {FilterSustain, 45}, {AmpAttack, 18}, {AmpSustain, 85}, {AmpRelease, 30}, {-1, 0}}}},
    {"Soft Pad", {{{FineB, 12}, {PulseA, 1}, {PwA, 35}, {Cutoff, 45}, {Resonance, 10}, {EnvAmount, 25},
                   {FilterAttack, 70}, {FilterDecay, 70}, {FilterSustain, 60}, {FilterRelease, 70},
                   {AmpAttack, 65}, {AmpSustain, 90}, {AmpRelease, 72}, {LfoAmount, 20}, {LfoPw, 40}, {LfoFreq, 0},
                   {Vintage, 35}, {-1, 0}}}},
    {"Glass Pluck", {{{SawA, 0}, {PulseA, 1}, {PwA, 18}, {FreqB, 12}, {SawB, 0}, {TriB, 1}, {LevelB, 45},
                      {Cutoff, 48}, {Resonance, 40}, {EnvAmount, 55}, {FilterDecay, 32}, {FilterSustain, 0},
                      {AmpDecay, 40}, {AmpSustain, 0}, {AmpRelease, 40}, {-1, 0}}}},
    {"Warm Keys", {{{SawA, 0}, {PulseA, 1}, {PwA, 45}, {SawB, 0}, {TriB, 1}, {FineB, 4}, {Cutoff, 40},
                    {EnvAmount, 35}, {KeyTrack, 2}, {FilterDecay, 50}, {FilterSustain, 25}, {AmpDecay, 58},
                    {AmpSustain, 35}, {AmpRelease, 40}, {VelFilter, 45}, {-1, 0}}}},
    {"Sub Bass", {{{FreqA, -12}, {SawA, 0}, {PulseA, 0}, {FreqB, -12}, {SawB, 0}, {TriB, 1}, {LevelA, 0},
                   {LevelB, 100}, {Cutoff, 35}, {Resonance, 0}, {EnvAmount, 10}, {Voices, 1}, {-1, 0}}}},
    {"String Machine", {{{PulseA, 1}, {SawA, 1}, {PwA, 25}, {FineB, 14}, {Cutoff, 55}, {Resonance, 5}, {EnvAmount, 10},
                         {AmpAttack, 55}, {AmpSustain, 95}, {AmpRelease, 60}, {LfoAmount, 25}, {LfoPw, 60},
                         {LfoFreq, 0}, {LfoRate, 40}, {Vintage, 30}, {-1, 0}}}},
    {"Poly Sweep", {{{Cutoff, 20}, {Resonance, 65}, {EnvAmount, 70}, {FilterAttack, 72}, {FilterDecay, 75},
                     {FilterSustain, 30}, {FilterRelease, 70}, {AmpAttack, 40}, {AmpSustain, 90}, {AmpRelease, 70},
                     {PolyOscB, 25}, {PolyFilter, 1}, {-1, 0}}}},
    {"Unison Lead", {{{Unison, 1}, {Detune, 35}, {Cutoff, 64}, {Resonance, 22}, {EnvAmount, 35}, {FilterDecay, 50},
                      {FilterSustain, 40}, {Glide, 20}, {-1, 0}}}},
    {"Noise Hat", {{{LevelA, 0}, {LevelB, 0}, {Noise, 100}, {Cutoff, 88}, {Resonance, 30}, {KeyTrack, 0},
                    {EnvAmount, 0}, {AmpDecay, 22}, {AmpSustain, 0}, {AmpRelease, 20}, {-1, 0}}}},
};

// The voice's level, staged as Melodee staged it: a held note at full velocity about -15 LUFS (+5.3 dB).
constexpr float output_trim = 1.841f;
} // namespace

const SynthInfo& PolySynth::info() {
    static const SynthInfo synth {"Melodee Poly", "melodee-poly", params, Count, legacy_count,
                                  dests, dest_names, sizeof(dests) / sizeof(dests[0]),
                                  sounds, sizeof(sounds) / sizeof(sounds[0])};
    return synth;
}

PolySynth::PolySynth() {
    for (unsigned n = 0; n < Count; ++n) p[n] = params[n].initial;
    for (unsigned n = 0; n < max_voices; ++n) {
        voices[n].a.reset(0.13f * n); voices[n].b.reset(0.29f * n); // free-running: voices start apart
    }
    update();
}

void PolySynth::set_param(int id, int value) {
    if (id < 0 || id >= Count) return;
    step.restart();
    p[size_t(id)] = std::clamp<int>(value, params[id].min, params[id].max);
    dirty = true;
}

void PolySynth::update() {
    dirty = false;
    using dsp::Envelope;
    amp_shape = {Envelope::coefficient(time_seconds(p[AmpAttack])), Envelope::coefficient(time_seconds(p[AmpDecay])),
                 Envelope::coefficient(time_seconds(p[AmpRelease])), p[AmpSustain] / 100.0f};
    filter_shape = {Envelope::coefficient(time_seconds(p[FilterAttack])), Envelope::coefficient(time_seconds(p[FilterDecay])),
                    Envelope::coefficient(time_seconds(p[FilterRelease])), p[FilterSustain] / 100.0f};
    // Glide: about 2 s at the top, per control block.
    float glide = p[Glide] ? 0.002f * std::pow(1000.0f, p[Glide] / 100.0f) : 0;
    glide_coefficient = glide > 0 ? 1.0f - std::exp(-float(control_block) / (glide * dsp::sample_rate()) * 3.0f) : 1.0f;
}

bool PolySynth::sounding() const {
    for (auto& v : voices) if (v.amp_env.active()) return true;
    return false;
}

PolySynth::Voice& PolySynth::allocate(int key) {
    unsigned limit = std::clamp(p[Voices], 1, int(max_voices));
    for (unsigned n = 0; n < limit; ++n) if (voices[n].key == key) return voices[n];
    for (unsigned n = 0; n < limit; ++n) if (!voices[n].amp_env.active()) return voices[n];
    Voice* oldest = nullptr; // a released voice first, else the oldest
    for (unsigned n = 0; n < limit; ++n)
        if (!voices[n].held && (!oldest || voices[n].age < oldest->age)) oldest = &voices[n];
    if (oldest) return *oldest;
    oldest = &voices[0];
    for (unsigned n = 1; n < limit; ++n) if (voices[n].age < oldest->age) oldest = &voices[n];
    return *oldest;
}

void PolySynth::set_pressure(int key, float value) {
    step.restart();
    for (auto& v : voices) if (v.key == key && v.held) v.pressure = value;
}

void PolySynth::start(Voice& v, int key, int velocity, bool retrigger) {
    bool was = v.amp_env.active();
    v.pressure = 0; v.touch = 0;
    v.random = noise.next();
    v.key = key; v.held = true; v.age = ++clock;
    v.velocity = velocity / 127.0f;
    v.slide = 0;
    v.target = float(key);
    if (!was || glide_coefficient >= 1.0f) v.pitch = float(key);
    if (retrigger || !was) { v.amp_env.gate(true); v.filter_env.gate(true); }
    if (!was) v.filter.reset();
}

void PolySynth::note_on(int key, int velocity) {
    step.restart();
    if (dirty) update();
    // Remember the held keys for mono and unison legato.
    if (stacked < stack.size()) {
        unsigned w = 0;
        for (unsigned n = 0; n < stacked; ++n) if (stack[n] != key) stack[w++] = stack[n];
        stacked = w;
        stack[stacked++] = key;
    }
    bool unison = p[Unison] != 0;
    unsigned limit = std::clamp(p[Voices], 1, int(max_voices));
    if (unison || limit == 1) {
        bool legato = false;
        for (unsigned n = 0; n < (unison ? limit : 1u); ++n) legato |= voices[n].held;
        for (unsigned n = 0; n < (unison ? limit : 1u); ++n) {
            auto& v = voices[n];
            start(v, key, velocity, !legato);
            float spread = limit > 1 ? (float(n) - (limit - 1) / 2.0f) / ((limit - 1) / 2.0f) : 0;
            v.detune = unison ? spread * p[Detune] / 100.0f * 0.35f : 0;
        }
        return;
    }
    auto& v = allocate(key);
    v.detune = 0;
    start(v, key, velocity, true);
}

// A slide or tie (Synth::slide): the held voices of `from` go on as `to`. With the GLIDE knob up they glide at its
// rate, as a played legato does; else a slide takes its own 60 ms.
void PolySynth::slide(int from, int to, int velocity, bool glide) {
    step.restart();
    if (dirty) update();
    bool found = false;
    for (auto& v : voices)
        if (v.key == from && v.held) {
            v.slide = glide && glide_coefficient >= 1.0f ? v.pitch - float(to) : 0;
            v.key = to; v.target = float(to); v.age = ++clock;
            if (!glide || glide_coefficient >= 1.0f) v.pitch = float(to) + v.slide;
            found = true;
        }
    if (!found) { note_on(to, velocity); return; }
    for (unsigned n = 0; n < stacked; ++n) if (stack[n] == from) stack[n] = to; // mono and unison: `to` is held now
}

void PolySynth::note_off(int key) {
    step.restart();
    unsigned w = 0;
    for (unsigned n = 0; n < stacked; ++n) if (stack[n] != key) stack[w++] = stack[n];
    stacked = w;
    unsigned limit = std::clamp(p[Voices], 1, int(max_voices));
    if (p[Unison] || limit == 1) {
        // Back to the previous held key, legato; or release.
        for (unsigned n = 0; n < (p[Unison] ? limit : 1u); ++n) {
            auto& v = voices[n];
            if (v.key != key) continue;
            if (stacked) { v.key = stack[stacked - 1]; v.target = float(v.key); if (glide_coefficient >= 1) v.pitch = v.target; }
            else { v.held = false; v.amp_env.gate(false); v.filter_env.gate(false); }
        }
        return;
    }
    for (auto& v : voices)
        if (v.key == key && v.held) { v.held = false; v.amp_env.gate(false); v.filter_env.gate(false); }
}

void PolySynth::all_notes_off() {
    step.restart();
    stacked = 0;
    for (auto& v : voices) if (v.held) { v.held = false; v.amp_env.gate(false); v.filter_env.gate(false); }
}

void PolySynth::silence() {
    step.restart();
    stacked = 0;
    for (auto& v : voices) {
        v.key = -1; v.held = false; v.amp_env = {}; v.filter_env = {}; v.filter.reset();
    }
}

void PolySynth::control(Voice& v) {
    v.pitch += (v.target - v.pitch) * glide_coefficient;
    if (v.slide != 0) { // a sequenced slide's own glide, over the GLIDE knob's (none: at once)
        v.slide = expression::slide_settle(v.slide, control_block);
        v.pitch = v.target + v.slide;
    }
    // Vintage: each voice wanders a little in pitch.
    if (p[Vintage]) {
        float r = noise.next();
        v.drift = std::clamp(v.drift + r * 0.004f, -1.0f, 1.0f) * 0.999f;
    }
    float lfo_depth = std::min(1.0f, p[LfoAmount] / 100.0f + wheel * p[WheelLfo] / 100.0f);
    float lfo_now = lfo_value * lfo_depth;
    float poly_env = v.filter_env.level * p[PolyEnv] / 100.0f;
    float bend_st = bend * p[BendRange];
    float drift = v.drift * p[Vintage] / 100.0f * 0.15f;
    // The mod matrix: full amount moves the cutoff 5 octaves, resonance and
    // level fully, pitch 12 semitones, pulse width 0.45, noise fully.
    expression::follow(v.touch, v.pressure, control_block);
    using expression::at;
    auto m = expression::sums(mods, {lfo_value, v.velocity, v.touch, wheel, (v.pitch - 60.0f) / 60.0f, v.random, v.filter_env.level});
    v.noise_mod = at(m, ModDest::Noise);
    // Pressure, as aftertouch on a polysynth: vibrato and a brighter filter.
    float vibrato_st = vibrato_value * expression::vibrato_semitones(v.touch, p[AtVib]);
    float pitch_mod = at(m, ModDest::Pitch) * 12 + vibrato_st;

    float key_a = v.pitch + p[FreqA] + bend_st + drift + v.detune
        + lfo_now * p[LfoFreq] / 100.0f * 2.0f + wheel * p[WheelFreq] / 100.0f * 2.0f
        + (p[PolyFreqA] ? poly_env * 24.0f : 0) + pitch_mod + at(m, ModDest::OscAPitch) * 12;
    float key_b = (p[KeybB] ? v.pitch : 60.0f) + p[FreqB] + p[FineB] / 100.0f + bend_st + drift * 0.7f + v.detune
        + lfo_now * p[LfoFreq] / 100.0f * 2.0f + wheel * p[WheelFreq] / 100.0f * 2.0f + pitch_mod + at(m, ModDest::OscBPitch) * 12;
    v.dt_a = std::min(dsp::midi_hz(key_a) / dsp::sample_rate(), 0.45f);
    float hz_b = dsp::midi_hz(key_b);
    if (p[LoFreqB]) hz_b /= 64.0f;
    v.dt_b = std::min(hz_b / dsp::sample_rate(), 0.45f);

    float width_a = p[PwA] / 100.0f + lfo_now * p[LfoPw] / 100.0f * 0.4f + (p[PolyPwA] ? poly_env * 0.4f : 0) + at(m, ModDest::PulseWidth) * 0.45f;
    v.mix_a = {p[SawA] ? 1.0f : 0.0f, p[PulseA] ? 1.0f : 0.0f, 0, std::clamp(width_a, 0.05f, 0.95f)};
    float width_b = p[PwB] / 100.0f + lfo_now * p[LfoPw] / 100.0f * 0.4f;
    v.mix_b = {p[SawB] ? 1.0f : 0.0f, p[PulseB] ? 1.0f : 0.0f, p[TriB] ? 1.0f : 0.0f, std::clamp(width_b, 0.05f, 0.95f)};

    // Filter: octaves above 20 Hz, then every modulation in octaves.
    float octaves = p[Cutoff] / 100.0f * 9.97f;
    octaves += v.filter_env.level * p[EnvAmount] / 100.0f * 7.0f;
    octaves += (v.pitch - 60.0f) / 12.0f * (p[KeyTrack] * 0.5f);
    octaves += (v.velocity - 0.5f) * p[VelFilter] / 100.0f * 4.0f;
    octaves += lfo_now * p[LfoFilter] / 100.0f * 3.0f + wheel * p[WheelFilter] / 100.0f * 4.0f;
    // Poly mod into the filter: the envelope, and oscillator B where it is in its cycle.
    if (p[PolyFilter]) octaves += poly_env * 4.0f + (2.0f * v.b.phase - 1.0f) * p[PolyOscB] / 100.0f * 2.0f;
    octaves += at(m, ModDest::Cutoff) * 5 + v.touch * p[AtBright] / 100.0f * 4.0f;
    v.filter.tune(20.0f * std::exp2(octaves), std::clamp(p[Resonance] / 100.0f + at(m, ModDest::Resonance), 0.0f, 1.0f));
    v.gain = std::max(0.0f, expression::velocity_gain(v.velocity, p[VelAmp] / 100.0f) * (1.0f + at(m, ModDest::AmpLevel)));
}

void PolySynth::render(float* out, unsigned frames) {
    if (dirty) update();
    // Unison stacks every voice on one key: their detuned sum, as loud as one voice.
    float output_gain = 0.7f * output_trim / (p[Unison] ? std::sqrt(float(std::clamp(p[Voices], 1, int(max_voices)))) : 1.0f);
    float level_a = p[LevelA] / 100.0f, level_b = p[LevelB] / 100.0f, level_n = p[Noise] / 100.0f;
    float poly_b_fm = p[PolyFreqA] ? p[PolyOscB] / 100.0f * 2.0f : 0;
    float poly_b_pw = p[PolyPwA] ? p[PolyOscB] / 100.0f * 0.4f : 0;
    bool sync = p[Sync] != 0;
    for (unsigned start = 0; start < frames;) {
        // A control step on its own frames (core/control_step.h), not from this call's start.
        bool stepping = step.starts(control_block);
        if (stepping) {
            unsigned moved = step.take();
            float rate_octaves = expression::lfo_rate_octaves(mods, wheel, lfo_value);
            lfo_value = lfo.next(rate_hertz(p[LfoRate]) * std::exp2(rate_octaves), unsigned(p[LfoShape]), float(moved) / dsp::sample_rate());
            vibrato_value = vibrato.next(rate_hertz(p[VibRate]), moved);
        }
        unsigned n = step.run(frames - start);
        for (auto& v : voices) {
            if (stepping) { // the voices sounding as the step starts play it through
                v.live = v.amp_env.active();
                if (!v.live) { if (v.key >= 0 && !v.held) v.key = -1; continue; }
                control(v);
            } else if (!v.live) {
                continue;
            }
            for (unsigned f = 0; f < n; ++f) {
                bool wrap_b, wrap_a;
                float b = v.b.next(v.dt_b, v.mix_b, wrap_b);
                auto mix_a = v.mix_a;
                if (poly_b_pw != 0) mix_a.width = std::clamp(mix_a.width + b * poly_b_pw, 0.05f, 0.95f);
                float a = v.a.next(v.dt_a * (1.0f + b * poly_b_fm), mix_a, wrap_a);
                if (sync && wrap_b) v.a.reset(v.b.phase * v.dt_a / std::max(v.dt_b, 1e-6f));
                float ln = std::clamp(level_n + v.noise_mod, 0.0f, 1.0f);
                float x = a * level_a + b * level_b + (ln != 0 ? noise.next() * ln : 0);
                x *= 0.5f;
                float y = v.filter.process(x);
                v.filter_env.next(filter_shape);
                out[start + f] += y * v.amp_env.next(amp_shape) * v.gain * output_gain;
            }
        }
        step.played(n);
        start += n;
    }
}

} // namespace melodee
