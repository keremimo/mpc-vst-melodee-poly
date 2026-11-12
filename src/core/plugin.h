// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include "core/synth.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

// A Melodee synth as the Open MPC engine (mpc-vst-plugins wrapper/engine.h, mpc_engine_t): string parameters,
// MIDI, 44.1 kHz interleaved int16 stereo out. The wrapper makes it a VST2 plug-in for MPC OS (Gen1 armv7, Gen2
// aarch64), Melodee's PLUGIN tracks and the desktop.
//
// Parameters, in VST index order: the synth's own (SynthInfo::params), then the mod matrix (mod_routes routes of
// source, destination and amount), then Volume. MIDI: notes (velocity 0 is a release), pitch bend, the mod wheel
// (CC 1), poly and channel pressure, sustain (CC 64), all notes off (CC 123), all sound off (CC 120) and reset (CC
// 121); a slide or tie from a sequencer is CC 84 (portamento control) naming the key the next note goes on from,
// with the legato pedal (CC 68) down for a tie (no glide). The state is text: the tag line, then key=value lines;
// a line `melodee=` (values in the synth's own order) with `melodee_mods=` (source:destination:amount, Melodee's
// numbering) imports a Melodee track as its project saved it.
namespace melodee {

namespace plugin {

constexpr unsigned block = 128;            // the wrapper's largest render
constexpr unsigned max_params = 96;        // the synth's own, the matrix's and Volume
constexpr int volume_min = -24, volume_max = 6;

inline const char* const mod_keys[mod_routes][3] {
    {"mod1_src", "mod1_dst", "mod1_amt"}, {"mod2_src", "mod2_dst", "mod2_amt"},
    {"mod3_src", "mod3_dst", "mod3_amt"}, {"mod4_src", "mod4_dst", "mod4_amt"},
    {"mod5_src", "mod5_dst", "mod5_amt"}, {"mod6_src", "mod6_dst", "mod6_amt"},
    {"mod7_src", "mod7_dst", "mod7_amt"}, {"mod8_src", "mod8_dst", "mod8_amt"},
};
inline const char* const mod_names[mod_routes][3] {
    {"Mod 1 Source", "Mod 1 Dest", "Mod 1 Amount"}, {"Mod 2 Source", "Mod 2 Dest", "Mod 2 Amount"},
    {"Mod 3 Source", "Mod 3 Dest", "Mod 3 Amount"}, {"Mod 4 Source", "Mod 4 Dest", "Mod 4 Amount"},
    {"Mod 5 Source", "Mod 5 Dest", "Mod 5 Amount"}, {"Mod 6 Source", "Mod 6 Dest", "Mod 6 Amount"},
    {"Mod 7 Source", "Mod 7 Dest", "Mod 7 Amount"}, {"Mod 8 Source", "Mod 8 Dest", "Mod 8 Amount"},
};

// Every parameter of a synth's plug-in, in VST index order.
struct ParamList {
    std::array<ParamInfo, max_params> all {};
    unsigned count = 0, mods = 0, volume = 0; // how many; the first of the matrix's; Volume's
};

inline ParamList params_of(const SynthInfo& s) {
    ParamList l;
    for (unsigned i = 0; i < s.param_count && l.count < max_params; ++i) l.all[l.count++] = s.params[i];
    l.mods = l.count;
    for (unsigned r = 0; r < mod_routes; ++r) {
        l.all[l.count++] = {mod_keys[r][0], mod_names[r][0], 0, int16_t(unsigned(ModSource::Count) - 1), 0, Format::Choice, mod_source_names};
        l.all[l.count++] = {mod_keys[r][1], mod_names[r][1], 0, int16_t(s.dest_count - 1), 0, Format::Choice, s.dest_names};
        l.all[l.count++] = {mod_keys[r][2], mod_names[r][2], -mod_amount_max, mod_amount_max, 0, Format::Number, nullptr};
    }
    l.volume = l.count;
    l.all[l.count++] = {"volume", "Volume", volume_min, volume_max, 0, Format::Decibels, nullptr};
    return l;
}

// A value as the screen reads it: "350 ms", "2.1 Hz", "+7 st", "Saw".
inline void shown(const ParamInfo& p, int v, char* out, int size) {
    switch (p.format) {
    case Format::Percent: std::snprintf(out, size_t(size), "%d%%", v); return;
    case Format::Semitones: std::snprintf(out, size_t(size), "%+d st", v); return;
    case Format::Cents: std::snprintf(out, size_t(size), "%+d ct", v); return;
    case Format::Decibels: std::snprintf(out, size_t(size), "%+d dB", v); return;
    case Format::Time: {
        float s = time_seconds(v);
        if (s < 1) std::snprintf(out, size_t(size), "%.0f ms", s * 1000);
        else std::snprintf(out, size_t(size), s < 10 ? "%.1f s" : "%.0f s", s);
        return;
    }
    case Format::Rate: {
        float hz = rate_hertz(v);
        std::snprintf(out, size_t(size), hz < 1 ? "%.2f Hz" : hz < 10 ? "%.1f Hz" : "%.0f Hz", hz);
        return;
    }
    case Format::Switch: std::snprintf(out, size_t(size), "%s", v ? "On" : "Off"); return;
    case Format::Choice:
        std::snprintf(out, size_t(size), "%s", p.choices && v >= p.min && v <= p.max ? p.choices[v - p.min] : "");
        return;
    case Format::Number: break;
    }
    std::snprintf(out, size_t(size), "%d", v);
}

// Whether a parameter's value reads as more than its number (params.json "dynamic_display").
inline bool reads_as_text(Format f) {
    return f == Format::Time || f == Format::Rate || f == Format::Semitones || f == Format::Cents || f == Format::Decibels;
}

// The output stage: Volume, then a soft knee from -2 dBFS so a loud chord rounds off instead of clipping the
// 16-bit output. Below the knee the level is untouched.
inline float soft_knee(float x) {
    constexpr float knee = 0.8f, room = 1.0f - knee;
    float a = std::fabs(x);
    if (a <= knee) return x;
    float y = knee + room * std::tanh((a - knee) / room);
    return x < 0 ? -y : y;
}

} // namespace plugin

// One plug-in instance: the synth, the matrix and the MIDI state around it.
template <class S>
class Plugin {
public:
    static const SynthInfo& info() { return S::info(); }
    static const plugin::ParamList& list() {
        static const plugin::ParamList l = plugin::params_of(info());
        return l;
    }

    Plugin() {
        const auto& l = list();
        for (unsigned i = 0; i < l.count; ++i) values[i] = l.all[i].initial;
    }

    // A parameter by its index; false when there is none.
    bool set(unsigned index, int value) {
        const auto& l = list();
        if (index >= l.count) return false;
        const auto& p = l.all[index];
        value = std::clamp(value, int(p.min), int(p.max));
        values[index] = int16_t(value);
        if (index < l.mods) synth.set_param(int(index), value);
        else if (index < l.volume) {
            unsigned r = (index - l.mods) / 3;
            synth.set_mod(r, route(r));
        } else gain = std::pow(10.0f, float(value) / 20.0f);
        return true;
    }
    int get(unsigned index) const { return index < list().count ? values[index] : 0; }
    int find(const char* key) const {
        const auto& l = list();
        for (unsigned i = 0; i < l.count; ++i) if (!std::strcmp(l.all[i].key, key)) return int(i);
        return -1;
    }

    // A route as the synth takes it (the destination list's place turned into its ModDest).
    ModRoute route(unsigned r) const {
        const auto& l = list();
        unsigned base = l.mods + r * 3;
        ModRoute m;
        m.source = uint8_t(values[base]);
        unsigned d = unsigned(values[base + 1]);
        m.destination = uint8_t(d < info().dest_count ? info().dests[d] : ModDest::None);
        m.amount = int8_t(values[base + 2]);
        return m;
    }

    // ---- MIDI ----
    void midi(const uint8_t* m, int len) {
        if (len < 1) return;
        uint8_t status = m[0] & 0xf0;
        int a = len > 1 ? m[1] & 127 : 0, b = len > 2 ? m[2] & 127 : 0;
        switch (status) {
        case 0x90: if (b) note_on(a, b); else note_off(a); break;
        case 0x80: note_off(a); break;
        case 0xa0: synth.set_pressure(a, b / 127.0f); break;
        case 0xd0: // channel pressure: every key held now
            for (int k = 0; k < 128; ++k) if (down[k]) synth.set_pressure(k, a / 127.0f);
            break;
        case 0xe0: synth.set_bend(std::clamp(float((b << 7 | a) - 8192) / 8192.0f, -1.0f, 1.0f)); break;
        case 0xb0: control(a, b); break;
        default: break;
        }
    }

    void render(int16_t* out, int frames) {
        frames = std::clamp(frames, 0, int(plugin::block));
        float mono[plugin::block] {};
        synth.render(mono, unsigned(frames));
        for (int f = 0; f < frames; ++f) {
            float x = std::clamp(plugin::soft_knee(mono[f] * gain), -1.0f, 1.0f);
            int16_t s = int16_t(std::lrintf(x * 32767.0f));
            out[2 * f] = out[2 * f + 1] = s;
        }
    }

    // ---- state: the tag line, then key=value lines ----
    int save(char* out, int size) const {
        const auto& l = list();
        int n = std::snprintf(out, size_t(size), "%s 1\n", info().tag);
        for (unsigned i = 0; i < l.count && n > 0 && n < size; ++i)
            n += std::snprintf(out + n, size_t(size - n), "%s=%d\n", l.all[i].key, values[i]);
        return n > 0 && n < size ? n : -1;
    }
    // Back to the initial values, then what the text sets; unknown keys are passed over, so an older state loads.
    void load(const char* text) {
        const auto& l = list();
        for (unsigned i = 0; i < l.count; ++i) set(i, l.all[i].initial);
        if (std::strncmp(text, info().tag, std::strlen(info().tag)) != 0) return; // not this synth's
        for (const char* line = text; line && *line; line = next_line(line)) {
            char key[32];
            const char* eq = std::strchr(line, '=');
            const char* end = std::strchr(line, '\n');
            if (!eq || (end && eq > end)) continue;
            size_t k = size_t(eq - line);
            if (k == 0 || k >= sizeof(key)) continue;
            std::memcpy(key, line, k);
            key[k] = 0;
            const char* value = eq + 1;
            if (!std::strcmp(key, "melodee")) import_values(value);
            else if (!std::strcmp(key, "melodee_mods")) import_mods(value);
            else if (int i = find(key); i >= 0) set(unsigned(i), option_or_number(list().all[unsigned(i)], value));
        }
        synth.silence();
    }

    S synth;

private:
    static const char* next_line(const char* line) {
        const char* end = std::strchr(line, '\n');
        return end ? end + 1 : nullptr;
    }
    // A value as text: a whole number (an option's place), or else the name of one of its options.
    static int option_or_number(const ParamInfo& p, const char* v) {
        size_t n = std::strcspn(v, "\r\n");
        char* end = nullptr;
        long number = std::strtol(v, &end, 10);
        if (end != v && size_t(end - v) == n) return int(number);
        if (p.choices)
            for (int o = 0; o <= p.max - p.min; ++o)
                if (std::strlen(p.choices[o]) == n && !strncasecompare(p.choices[o], v, n)) return p.min + o;
        return p.initial;
    }
    static int strncasecompare(const char* a, const char* b, size_t n) {
        for (size_t i = 0; i < n; ++i) {
            int x = std::tolower(static_cast<unsigned char>(a[i])), y = std::tolower(static_cast<unsigned char>(b[i]));
            if (x != y) return x - y;
        }
        return 0;
    }
    // `melodee=v0,v1,...`: a Melodee track's values in the synth's own order. Values a project from before pad
    // pressure did not carry stay at their initial values (it names fewer).
    void import_values(const char* v) {
        for (unsigned i = 0; i < info().param_count && *v && *v != '\n'; ++i) {
            char* end = nullptr;
            long x = std::strtol(v, &end, 10);
            if (end == v) break;
            set(i, int(x));
            v = *end == ',' ? end + 1 : end;
        }
    }
    // `melodee_mods=s:d:a,...`: Melodee's routes, ModSource and ModDest as Melodee numbers them; a destination this
    // synth does not have is dropped.
    void import_mods(const char* v) {
        const auto& l = list();
        for (unsigned r = 0; r < mod_routes && *v && *v != '\n'; ++r) {
            char* end = nullptr;
            long s = std::strtol(v, &end, 10);
            if (*end != ':') break;
            long d = std::strtol(end + 1, &end, 10);
            if (*end != ':') break;
            long a = std::strtol(end + 1, &end, 10);
            int place = 0;
            for (unsigned i = 0; i < info().dest_count; ++i) if (long(info().dests[i]) == d) place = int(i);
            bool kept = s > 0 && s < long(ModSource::Count) && place > 0;
            set(l.mods + r * 3, kept ? int(s) : 0);
            set(l.mods + r * 3 + 1, kept ? place : 0);
            set(l.mods + r * 3 + 2, kept ? int(a) : 0);
            v = *end == ',' ? end + 1 : end;
        }
    }

    void note_on(int key, int velocity) {
        down[key] = true;
        sustained[key] = false;
        if (slide_from >= 0) { // CC 84 named the key this note goes on from
            int from = slide_from;
            slide_from = -1;
            synth.slide(from, key, velocity, !legato);
            return;
        }
        synth.note_on(key, velocity);
    }
    void note_off(int key) {
        down[key] = false;
        if (sustain) { sustained[key] = true; return; }
        synth.note_off(key);
    }
    void control(int cc, int v) {
        switch (cc) {
        case 1: synth.set_wheel(v / 127.0f); break;
        case 64: {
            bool on = v >= 64;
            if (sustain && !on)
                for (int k = 0; k < 128; ++k)
                    if (sustained[k]) { sustained[k] = false; if (!down[k]) synth.note_off(k); }
            sustain = on;
            break;
        }
        case 68: legato = v >= 64; break;
        case 84: slide_from = v; break;
        case 120: synth.silence(); clear(); break;
        case 121:
            synth.set_wheel(0); synth.set_bend(0);
            control(64, 0);
            legato = false; slide_from = -1;
            break;
        case 123: synth.all_notes_off(); clear(); break;
        default: break;
        }
    }
    void clear() {
        for (int k = 0; k < 128; ++k) { down[k] = false; sustained[k] = false; }
        slide_from = -1;
    }

    std::array<int16_t, plugin::max_params> values {};
    float gain = 1;
    bool down[128] {};
    bool sustained[128] {}; // let go while the pedal is down: released when it comes up
    bool sustain = false, legato = false;
    int slide_from = -1;
};

// The engine table for the wrapper: plugin.cpp hands it over as mpc_engine().
template <class S>
struct Engine {
    using P = Plugin<S>;
    static void* create(const char*) { return new (std::nothrow) P(); }
    static void destroy(void* p) { delete static_cast<P*>(p); }
    static void midi(void* p, const uint8_t* m, int len) { static_cast<P*>(p)->midi(m, len); }
    static void set_param(void* p, const char* key, const char* value) {
        auto* x = static_cast<P*>(p);
        if (!std::strcmp(key, "state")) { x->load(value); return; }
        int i = x->find(key);
        if (i >= 0) x->set(unsigned(i), int(std::lround(std::strtod(value, nullptr))));
    }
    static int get_param(void* p, const char* key, char* buf, int len) {
        auto* x = static_cast<P*>(p);
        if (!std::strcmp(key, "state")) return x->save(buf, len);
        int i = x->find(key);
        if (i >= 0) return std::snprintf(buf, size_t(len), "%d", x->get(unsigned(i)));
        size_t n = std::strlen(key);
        if (n > 8 && !std::strcmp(key + n - 8, "_display")) { // "<key>_display": the value as the screen reads it
            char base[32];
            if (n - 8 >= sizeof(base)) return 0;
            std::memcpy(base, key, n - 8);
            base[n - 8] = 0;
            int j = x->find(base);
            if (j < 0) return 0;
            plugin::shown(P::list().all[unsigned(j)], x->get(unsigned(j)), buf, len);
            return int(std::strlen(buf)) + 1;
        }
        return 0;
    }
    static void render(void* p, int16_t* out, int frames) { static_cast<P*>(p)->render(out, frames); }
};

} // namespace melodee
