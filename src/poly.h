// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include "core/control_step.h"
#include "core/dsp.h"
#include "core/expression.h"
#include "core/synth.h"
#include <array>

// Melodee Poly: a two-oscillator polysynth. Oscillator A (saw, pulse, hard sync to B), oscillator B (saw, triangle,
// pulse, low frequency mode), noise, a four-pole ladder with its own envelope, an amp envelope, a global LFO, wheel
// and poly modulation, glide, unison and a little vintage drift. Up to eight voices. Melodee's PROPHET engine, out
// of Melodee as a plug-in of its own.
namespace melodee {

namespace poly {
// In Melodee's order (a Melodee track's values import by place): new ones go last.
enum Param : int8_t {
    FreqA, SawA, PulseA, PwA, Sync,
    FreqB, FineB, SawB, TriB, PulseB, PwB, LoFreqB, KeybB,
    LevelA, LevelB, Noise,
    Cutoff, Resonance, EnvAmount, KeyTrack, VelFilter,
    FilterAttack, FilterDecay, FilterSustain, FilterRelease,
    AmpAttack, AmpDecay, AmpSustain, AmpRelease, VelAmp,
    LfoRate, LfoShape, LfoAmount, LfoFreq, LfoPw, LfoFilter,
    WheelFreq, WheelFilter, WheelLfo,
    PolyEnv, PolyOscB, PolyFreqA, PolyPwA, PolyFilter,
    Glide, Unison, Detune, Vintage, BendRange, Voices,
    AtVib, AtBright, VibRate,
    Count
};
constexpr unsigned legacy_count = AtVib; // what Melodee projects from before pad pressure carry
} // namespace poly

class PolySynth final : public Synth {
public:
    static constexpr unsigned max_voices = 8;
    static constexpr unsigned control_block = 16; // modulation and filter tuning rate
    static const SynthInfo& info();

    PolySynth();
    void note_on(int key, int velocity) override;
    void note_off(int key) override;
    void all_notes_off() override;
    void silence() override;
    void set_param(int id, int value) override;
    void set_wheel(float value) override { wheel = value; step.restart(); }
    void set_bend(float value) override { bend = value; step.restart(); }
    void set_mod(unsigned route, const ModRoute& value) override { if (route < mods.size()) mods[route] = value; step.restart(); }
    void set_pressure(int key, float value) override;
    void slide(int from, int to, int velocity, bool glide) override;
    void render(float* out, unsigned frames) override;
    bool sounding() const override;

private:
    struct Voice {
        int key = -1;
        bool held = false;
        uint32_t age = 0;
        float velocity = 0, pitch = 60, target = 60, drift = 0, detune = 0;
        float slide = 0; // semitones off the key (expression::slide_settle)
        dsp::Oscillator a, b;
        dsp::Ladder filter;
        dsp::Envelope amp_env, filter_env;
        float dt_a = 0, dt_b = 0, gain = 0;
        float pressure = 0, touch = 0, random = 0; // mod sources of this voice; touch follows pressure smoothly
        float noise_mod = 0;            // the matrix's noise level, per control block
        dsp::Oscillator::Mix mix_a, mix_b;
        bool live = false;              // sounding as the control step in hand began: it plays the step through
    };
    void update();        // derived values after a parameter change
    void control(Voice&); // per control block: pitch, filter, widths
    Voice& allocate(int key);
    void start(Voice& v, int key, int velocity, bool retrigger);

    std::array<int, poly::Count> p {};
    std::array<Voice, max_voices> voices {};
    std::array<ModRoute, mod_routes> mods {};
    std::array<int, 16> stack {}; // held keys for mono and unison, newest last
    unsigned stacked = 0;
    uint32_t clock = 0;
    float wheel = 0, bend = 0;
    dsp::Lfo lfo;
    expression::Vibrato vibrato; // pressure's own, apart from the LFO
    dsp::Noise noise;
    float lfo_value = 0, vibrato_value = 0;
    ControlStep step; // the modulation's own steps
    dsp::Envelope::Shape amp_shape, filter_shape;
    float glide_coefficient = 1;
    bool dirty = true;
};

} // namespace melodee
