// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include "core/mod_matrix.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// What a Melodee synth plug-in is made of: its parameter table (ranges, initial values, how a value reads), the
// factory sounds, the mod matrix destinations it names, and the voice itself (Synth), driven on the audio thread
// only. core/plugin.h turns these into the Open MPC engine (mpc_engine_t) and tools/gen_json.cpp into the
// toolchain's vst/params.json and vst/presets.json, so the table here is the one place a parameter is stated.
namespace melodee {

enum class Format : uint8_t {
    Number,    // plain integer
    Percent,   // 0..100 "%"
    Semitones, // signed, "st"
    Cents,     // signed, "ct"
    Time,      // 0..100 on an exponential 1 ms .. 10 s scale
    Rate,      // 0..100 on an exponential 0.05 .. 30 Hz scale
    Switch,    // Off / On
    Choice,    // one of `choices`
    Decibels,  // signed dB
};

struct ParamInfo {
    const char* key;  // params.json key: stable once released (projects, automation and skins bind by index)
    const char* name; // as a host lists it
    int16_t min, max, initial;
    Format format = Format::Percent;
    const char* const* choices = nullptr; // Format::Choice: max - min + 1 names
};

// A factory sound: the values that differ from the initial ones, (id, value) pairs ended by id -1.
constexpr unsigned max_sound_changes = 40;
struct Sound {
    const char* name;
    std::array<std::array<int16_t, 2>, max_sound_changes> changes;
};

// Exponential scales of the Time and Rate formats.
inline float time_seconds(int value) { return 0.001f * std::pow(10000.0f, std::clamp(value, 0, 100) / 100.0f); }
inline float rate_hertz(int value) { return 0.05f * std::pow(600.0f, std::clamp(value, 0, 100) / 100.0f); }

struct SynthInfo {
    const char* name;              // the plug-in's name: "Melodee Poly"
    const char* tag;               // the first word of its state: "melodee-poly"
    const ParamInfo* params;       // the voice's own parameters, in VST index order (the mod matrix and Volume follow)
    unsigned param_count;
    // How many of them a Melodee project saved before pad pressure (Melodee's params_version 0) carries; the rest
    // import at their initial values. 0: all.
    unsigned legacy_count;
    // The matrix destinations it has, None first, in the order its destination list names them.
    const ModDest* dests;
    const char* const* dest_names;
    unsigned dest_count;
    const Sound* sounds;
    unsigned sound_count;
};

// One synth voice section as the plug-in plays it. Created up front and never allocating; render() adds the mono
// output into `out`. Audio thread only (the Open MPC wrapper never calls two of these at once).
class Synth {
public:
    virtual ~Synth() = default;
    virtual void note_on(int key, int velocity) = 0;
    virtual void note_off(int key) = 0;
    virtual void all_notes_off() = 0; // release held notes
    virtual void silence() = 0;       // stop every voice at once
    virtual void set_param(int id, int value) = 0; // one of SynthInfo::params, already in range
    virtual void set_wheel(float /*0..1*/) {}
    virtual void set_bend(float /*-1..1*/) {}
    virtual void set_mod(unsigned /*route*/, const ModRoute&) {}
    virtual void set_pressure(int /*key*/, float /*0..1*/) {}
    // A slide or tie (MIDI CC 84, core/plugin.h): `from`, still held, carries on as `to` without a new attack, its
    // pitch gliding there when `glide`. A synth with no held `from` plays `to` as ever.
    virtual void slide(int from, int to, int velocity, bool /*glide*/) {
        if (from == to) return;
        note_on(to, velocity);
        note_off(from);
    }
    virtual void render(float* out, unsigned frames) = 0;
    virtual bool sounding() const = 0;
};

} // namespace melodee
