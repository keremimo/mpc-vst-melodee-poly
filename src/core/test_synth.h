// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include "core/plugin.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <set>
#include <string>
#include <vector>

// Host checks every Melodee synth plug-in runs (test/test_<synth>.cpp, `make test`), through the same engine table
// the wrapper drives: the parameter table, sound out of every factory sound, sustain, slides, the state and a
// Melodee track's import, and that a block cut anywhere plays the same to the sample (sample-accurate notes).
namespace melodee::test {

inline int failures = 0;
#define MELODEE_CHECK(cond) \
    do { if (!(cond)) { std::fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); ++melodee::test::failures; } } while (0)

template <class S>
struct Rig {
    using E = Engine<S>;
    void* p = E::create(nullptr);
    ~Rig() { E::destroy(p); }
    Plugin<S>& plugin() { return *static_cast<Plugin<S>*>(p); }
    void midi(uint8_t a, uint8_t b, uint8_t c) { uint8_t m[3] {a, b, c}; E::midi(p, m, 3); }
    void set(const char* key, const char* value) { E::set_param(p, key, value); }
    std::string get(const char* key) {
        char buf[8192] {};
        return E::get_param(p, key, buf, int(sizeof(buf))) > 0 ? std::string(buf) : std::string();
    }
    // `frames` of output (left only: both sides are the same), rendered in blocks of `block` or, with `cuts`, of
    // the sizes it lists in turn.
    std::vector<int16_t> play(unsigned frames, unsigned block = 128, const std::vector<unsigned>* cuts = nullptr) {
        std::vector<int16_t> out;
        int16_t lr[256];
        for (size_t n = 0; out.size() < frames; ++n) {
            unsigned size = cuts ? (*cuts)[n % cuts->size()] : block;
            size = std::min<unsigned>(size, unsigned(frames - out.size()));
            E::render(p, lr, int(size));
            for (unsigned f = 0; f < size; ++f) {
                out.push_back(lr[2 * f]);
                if (lr[2 * f] != lr[2 * f + 1]) ++mismatched;
            }
        }
        return out;
    }
    unsigned mismatched = 0;
};

inline double peak(const std::vector<int16_t>& x) {
    int m = 0;
    for (auto s : x) m = std::max(m, std::abs(int(s)));
    return m / 32767.0;
}
inline double rms(const std::vector<int16_t>& x) {
    double sum = 0;
    for (auto s : x) sum += double(s) * s;
    return x.empty() ? 0 : std::sqrt(sum / double(x.size())) / 32767.0;
}

// The parameter table: unique keys short enough for the wrapper, initial values in range, a name per option.
template <class S>
void table() {
    const auto& l = Plugin<S>::list();
    MELODEE_CHECK(l.count <= plugin::max_params);
    MELODEE_CHECK(l.mods == S::info().param_count);
    MELODEE_CHECK(l.volume == l.mods + 3 * mod_routes);
    std::set<std::string> keys;
    for (unsigned i = 0; i < l.count; ++i) {
        const auto& p = l.all[i];
        MELODEE_CHECK(keys.insert(p.key).second);
        MELODEE_CHECK(std::strlen(p.key) > 0 && std::strlen(p.key) < 24);
        MELODEE_CHECK(std::strlen(p.name) > 0 && std::strlen(p.name) < 24);
        MELODEE_CHECK(p.min <= p.initial && p.initial <= p.max);
        if (p.format == Format::Choice || p.format == Format::Switch) {
            MELODEE_CHECK(p.choices != nullptr);
            for (int o = 0; p.choices && o <= p.max - p.min; ++o) MELODEE_CHECK(p.choices[o] && *p.choices[o]);
        }
    }
    const auto& info = S::info();
    MELODEE_CHECK(info.dest_count >= 1 && info.dests[0] == ModDest::None);
    MELODEE_CHECK(info.sound_count >= 1);
    for (unsigned s = 0; s < info.sound_count; ++s) {
        MELODEE_CHECK(std::strlen(info.sounds[s].name) < 24);
        for (const auto& c : info.sounds[s].changes) {
            if (c[0] < 0) break;
            MELODEE_CHECK(unsigned(c[0]) < info.param_count);
            if (unsigned(c[0]) < info.param_count) MELODEE_CHECK(c[1] >= info.params[c[0]].min && c[1] <= info.params[c[0]].max);
        }
    }
}

// Every factory sound, set the way the wrapper sets a preset (each value as text), sounds at C3 and C4 within a
// second, stays finite and under full scale, and ends after its release.
template <class S>
void sounds() {
    const auto& info = S::info();
    for (unsigned s = 0; s < info.sound_count; ++s) {
        Rig<S> r;
        int value[plugin::max_params] {};
        for (unsigned i = 0; i < info.param_count; ++i) value[i] = info.params[i].initial;
        for (const auto& c : info.sounds[s].changes) { if (c[0] < 0) break; value[c[0]] = c[1]; }
        for (unsigned i = 0; i < info.param_count; ++i) r.set(info.params[i].key, std::to_string(value[i]).c_str());
        r.midi(0x90, 48, 110);
        r.midi(0x90, 60, 100);
        auto held = r.play(44100);
        r.midi(0x80, 48, 0);
        r.midi(0x80, 60, 0);
        auto tail = r.play(44100 * 12);
        if (rms(held) < 0.003) std::fprintf(stderr, "  sound %s: rms %.4f\n", info.sounds[s].name, rms(held));
        MELODEE_CHECK(rms(held) >= 0.003);
        MELODEE_CHECK(peak(held) < 1.0);
        MELODEE_CHECK(r.mismatched == 0);
        MELODEE_CHECK(!r.plugin().synth.sounding());
    }
}

// Notes started anywhere in a block (the wrapper's SAMPLE_ACCURATE renders any 1..128 frames): the same output to
// the sample however the host cuts the blocks.
template <class S>
void block_cuts() {
    std::vector<unsigned> cuts {1, 127, 64, 3, 128, 17, 90, 5, 33, 128, 7};
    std::vector<int16_t> outs[2];
    for (int run = 0; run < 2; ++run) {
        Rig<S> r;
        r.midi(0xb0, 1, 40);
        r.midi(0x90, 57, 100);
        auto a = r.play(5000, 128, run ? &cuts : nullptr);
        r.midi(0x90, 64, 90);
        r.midi(0xe0, 0, 80);
        auto b = r.play(7000, 128, run ? &cuts : nullptr);
        r.midi(0x80, 57, 0);
        r.midi(0x80, 64, 0);
        auto c = r.play(20000, 128, run ? &cuts : nullptr);
        outs[run] = a;
        outs[run].insert(outs[run].end(), b.begin(), b.end());
        outs[run].insert(outs[run].end(), c.begin(), c.end());
    }
    MELODEE_CHECK(outs[0] == outs[1]);
}

// The sustain pedal holds a released note until it comes up; all notes off and all sound off end everything.
template <class S>
void pedal() {
    Rig<S> r;
    r.midi(0xb0, 64, 127);
    r.midi(0x90, 60, 100);
    r.play(4410);
    r.midi(0x80, 60, 0);
    auto held = r.play(44100 * 3);
    MELODEE_CHECK(rms(std::vector<int16_t>(held.end() - 4410, held.end())) > 0.002);
    r.midi(0xb0, 64, 0);
    r.play(44100 * 12);
    MELODEE_CHECK(!r.plugin().synth.sounding());
    r.midi(0x90, 62, 100);
    r.play(4410);
    r.midi(0xb0, 120, 0);
    auto after = r.play(2048);
    MELODEE_CHECK(peak(std::vector<int16_t>(after.begin() + 1024, after.end())) == 0);
}

// A slide (CC 84 then the next note): the voice of the first key goes on as the second, so letting go of the
// first does not end it and letting go of the second does.
template <class S>
void slides() {
    Rig<S> r;
    r.midi(0x90, 60, 100);
    r.play(8820);
    r.midi(0xb0, 84, 60);
    r.midi(0x90, 67, 100);
    r.midi(0x80, 60, 0);
    auto on = r.play(44100);
    MELODEE_CHECK(rms(std::vector<int16_t>(on.end() - 4410, on.end())) > 0.002);
    r.midi(0x80, 67, 0);
    r.play(44100 * 12);
    MELODEE_CHECK(!r.plugin().synth.sounding());
}

// The state: every value back as it was saved, an unknown or older text loads what it names over initial values,
// and option values may be given by name.
template <class S>
void state() {
    const auto& l = Plugin<S>::list();
    std::mt19937 random(7);
    Rig<S> a;
    for (unsigned i = 0; i < l.count; ++i) {
        const auto& p = l.all[i];
        int v = p.min + int(random() % unsigned(p.max - p.min + 1));
        a.set(p.key, std::to_string(v).c_str());
    }
    std::string saved = a.get("state");
    MELODEE_CHECK(saved.size() > 0 && saved.size() < 8192);
    MELODEE_CHECK(saved.compare(0, std::strlen(S::info().tag), S::info().tag) == 0);
    Rig<S> b;
    b.set("state", saved.c_str());
    for (unsigned i = 0; i < l.count; ++i) MELODEE_CHECK(a.get(l.all[i].key) == b.get(l.all[i].key));
    // Another synth's state is not this one's: everything back to initial values.
    Rig<S> c;
    c.set(l.all[0].key, std::to_string(l.all[0].max).c_str());
    c.set("state", "someone-else 1\nvolume=-6\n");
    MELODEE_CHECK(c.get(l.all[0].key) == std::to_string(l.all[0].initial));
    MELODEE_CHECK(c.get("volume") == "0");
    // Names for options; unknown keys passed over.
    std::string text = std::string(S::info().tag) + " 1\nnot_a_key=4\nmod1_src=wheel\nmod1_dst=" + S::info().dest_names[1] + "\nmod1_amt=-40\n";
    Rig<S> d;
    d.set("state", text.c_str());
    MELODEE_CHECK(d.get("mod1_src") == "4");
    MELODEE_CHECK(d.get("mod1_dst") == "1");
    MELODEE_CHECK(d.get("mod1_amt") == "-40");
    auto route = d.plugin().route(0);
    MELODEE_CHECK(route.source == uint8_t(ModSource::Wheel) && route.destination == uint8_t(S::info().dests[1]) && route.amount == -40);
}

// A Melodee track's values (`melodee=`, the synth's own order) and routes (`melodee_mods=`, Melodee's numbers);
// a project from before pad pressure names fewer values, and the rest load at their initial values.
template <class S>
void melodee_import() {
    const auto& info = S::info();
    std::string values;
    for (unsigned i = 0; i < info.param_count; ++i) values += (i ? "," : "") + std::to_string(info.params[i].max);
    unsigned dest = unsigned(info.dests[info.dest_count - 1]);
    std::string text = std::string(info.tag) + " 1\nmelodee=" + values + "\nmelodee_mods=1:" + std::to_string(dest)
        + ":55,0:0:0,6:9999:20\n";
    Rig<S> r;
    r.set("state", text.c_str());
    for (unsigned i = 0; i < info.param_count; ++i) MELODEE_CHECK(r.get(info.params[i].key) == std::to_string(info.params[i].max));
    MELODEE_CHECK(r.get("mod1_src") == "1");
    MELODEE_CHECK(r.get("mod1_dst") == std::to_string(info.dest_count - 1));
    MELODEE_CHECK(r.get("mod1_amt") == "55");
    MELODEE_CHECK(r.get("mod3_src") == "0"); // a destination this synth has not: dropped
    if (info.legacy_count) {
        std::string few;
        for (unsigned i = 0; i < info.legacy_count; ++i) few += (i ? "," : "") + std::to_string(info.params[i].min);
        Rig<S> old;
        old.set("state", (std::string(info.tag) + " 1\nmelodee=" + few + "\n").c_str());
        for (unsigned i = info.legacy_count; i < info.param_count; ++i)
            MELODEE_CHECK(old.get(info.params[i].key) == std::to_string(info.params[i].initial));
    }
}

// Volume: -24 dB is about a sixteenth of the level; value text reads as the screen shows it.
template <class S>
void volume() {
    std::vector<int16_t> outs[2];
    for (int n = 0; n < 2; ++n) {
        Rig<S> r;
        if (n) r.set("volume", "-24");
        r.midi(0x90, 60, 100);
        outs[n] = r.play(22050);
    }
    MELODEE_CHECK(rms(outs[1]) < rms(outs[0]) * 0.08 && rms(outs[1]) > rms(outs[0]) * 0.04);
    Rig<S> r;
    MELODEE_CHECK(r.get("volume_display") == "+0 dB");
    MELODEE_CHECK(r.get("no_such_key").empty());
}

template <class S>
int run_all(const char* name) {
    table<S>();
    sounds<S>();
    block_cuts<S>();
    pedal<S>();
    slides<S>();
    state<S>();
    melodee_import<S>();
    volume<S>();
    if (failures) std::fprintf(stderr, "%s: %d check(s) failed\n", name, failures);
    else std::printf("%s: all checks passed\n", name);
    return failures ? 1 : 0;
}

} // namespace melodee::test
