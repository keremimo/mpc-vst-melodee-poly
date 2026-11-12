// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include <cstdint>

// The mod matrix's sources and destinations, the same in every Melodee synth (and numbered as Melodee's own
// model/mod_matrix.h numbers them, so a Melodee track's routes import as they are); each synth decides how far a
// destination moves and names the ones it has (Synth::mod_dests). A synth holds mod_routes routes, each three
// parameters: source, destination, amount (-100..100, 0 none).
namespace melodee {

enum class ModSource : uint8_t { None, Lfo, Velocity, Aftertouch, Wheel, KeyTrack, Random, FilterEnv, Count };
enum class ModDest : uint8_t { None, Cutoff, Resonance, AmpLevel, Pitch, OscAPitch, OscBPitch, PulseWidth, LfoRate, Noise, Count };

constexpr unsigned mod_routes = 8;
constexpr int mod_amount_max = 100;

struct ModRoute {
    uint8_t source = 0, destination = 0; // ModSource, ModDest; 0: none
    int8_t amount = 0;                   // -100..100
    bool used() const { return source && destination; }
};

// The sources as the source parameter lists them (option index = ModSource).
inline constexpr const char* mod_source_names[] {"None", "LFO", "Velocity", "Aftertouch", "Wheel", "Key track",
                                                 "Random", "Filter env"};

} // namespace melodee
