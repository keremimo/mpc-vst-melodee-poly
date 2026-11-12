// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include "core/plugin.h"
#include <cstdio>
#include <string>

// The Open MPC toolchain's inputs from a synth's own tables (tools/gen_json.cpp): vst/params.json, its parameter
// list in VST index order, and vst/presets.json, the factory sounds as the wrapper's presets (MPC's PRESET menu).
// A sound sets every one of the synth's own parameters; the mod matrix and Volume stay as the player set them, as
// loading a sound on a Melodee track did. `make json` writes both; `make test` fails while they are out of date.
namespace melodee::json {

inline std::string quoted(const char* s) {
    std::string out = "\"";
    for (; *s; ++s) {
        if (*s == '"' || *s == '\\') out += '\\';
        out += *s;
    }
    return out + "\"";
}

inline const char* unit_of(Format f) {
    switch (f) {
    case Format::Percent: return "%";
    default: return "";
    }
}

inline std::string param(const ParamInfo& p) {
    std::string out = "    {\"key\": " + quoted(p.key) + ", \"name\": " + quoted(p.name);
    if (p.choices) {
        out += ", \"options\": [";
        for (int o = 0; o <= p.max - p.min; ++o) out += (o ? ", " : "") + quoted(p.choices[o]);
        out += "], \"default\": " + quoted(p.choices[p.initial - p.min]);
    } else {
        out += ", \"min\": " + std::to_string(p.min) + ", \"max\": " + std::to_string(p.max)
            + ", \"default\": " + std::to_string(p.initial) + ", \"display\": \"int\"";
        if (*unit_of(p.format)) out += ", \"unit\": " + quoted(unit_of(p.format));
        if (plugin::reads_as_text(p.format)) out += ", \"dynamic_display\": true";
    }
    return out + "}";
}

// The parameter list, with the sections an auto-layout would frame (the skin is vst/layout.conf's).
template <class S>
std::string params_json() {
    const auto& l = Plugin<S>::list();
    std::string out = "{\n  \"name\": " + quoted(S::info().name) + ",\n  \"params\": [\n";
    for (unsigned i = 0; i < l.count; ++i) out += param(l.all[i]) + (i + 1 < l.count ? ",\n" : "\n");
    return out + "  ]\n}\n";
}

// Each factory sound as every one of the synth's own values.
template <class S>
std::string presets_json() {
    const auto& info = S::info();
    std::string out = "{\n  \"presets\": [\n";
    for (unsigned s = 0; s < info.sound_count; ++s) {
        int value[plugin::max_params] {};
        for (unsigned i = 0; i < info.param_count; ++i) value[i] = info.params[i].initial;
        for (const auto& c : info.sounds[s].changes) {
            if (c[0] < 0) break;
            value[c[0]] = c[1];
        }
        out += "    {\"name\": " + quoted(info.sounds[s].name) + ", \"values\": {";
        for (unsigned i = 0; i < info.param_count; ++i) {
            const auto& p = info.params[i];
            out += (i ? ", " : "") + quoted(p.key) + ": ";
            out += p.choices ? quoted(p.choices[value[i] - p.min]) : std::to_string(value[i]);
        }
        out += s + 1 < info.sound_count ? "}},\n" : "}}\n";
    }
    return out + "  ]\n}\n";
}

inline bool write(const char* path, const std::string& text) {
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    bool ok = std::fwrite(text.data(), 1, text.size(), f) == text.size();
    return std::fclose(f) == 0 && ok;
}

// gen_json <params.json> <presets.json>: write both; gen_json --check <params.json> <presets.json>: exit 1 when
// either differs from what the tables say.
template <class S>
int main(int argc, char** argv) {
    bool check = argc == 4 && std::string(argv[1]) == "--check";
    if (argc != 3 && !check) {
        std::fprintf(stderr, "usage: gen_json [--check] <params.json> <presets.json>\n");
        return 2;
    }
    const char* paths[2] {argv[check ? 2 : 1], argv[check ? 3 : 2]};
    std::string texts[2] {params_json<S>(), presets_json<S>()};
    int status = 0;
    for (int n = 0; n < 2; ++n) {
        if (check) {
            std::string have;
            if (FILE* f = std::fopen(paths[n], "rb")) {
                char buf[4096];
                for (size_t got; (got = std::fread(buf, 1, sizeof(buf), f)) > 0;) have.append(buf, got);
                std::fclose(f);
            }
            if (have != texts[n]) { std::fprintf(stderr, "%s is out of date: run make json\n", paths[n]); status = 1; }
        } else if (!write(paths[n], texts[n])) {
            std::fprintf(stderr, "cannot write %s\n", paths[n]);
            status = 1;
        }
    }
    return status;
}

} // namespace melodee::json
