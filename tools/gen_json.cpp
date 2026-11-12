// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
// vst/params.json and vst/presets.json from Melodee Poly's own tables (core/gen_json.h).
#include "core/gen_json.h"
#include "poly.h"

int main(int argc, char** argv) { return melodee::json::main<melodee::PolySynth>(argc, argv); }
