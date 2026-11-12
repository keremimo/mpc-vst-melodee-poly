// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
// Melodee Poly as mpc-vst-plugins' engine (wrapper/engine.h): the wrapper (vst2_wrap.c) makes it a VST2 plug-in.
#include "core/plugin.h"
#include "poly.h"

extern "C" {
#include "engine.h"
}

namespace {
using E = melodee::Engine<melodee::PolySynth>;
const mpc_engine_t api {E::create, E::destroy, E::midi, E::set_param, E::get_param, E::render, nullptr, nullptr};
} // namespace

extern "C" const mpc_engine_t* mpc_engine(void) { return &api; }
