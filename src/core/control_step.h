// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once

// A synth's modulation (its LFOs, pitch, filter tuning and levels) worked out once a step of `frames` of its own,
// counted from its last change (a note, a value, the wheel), never from where a render call starts: so however the
// host cuts a block (sample-accurate notes split it anywhere), the synth plays the same, to the sample. At a step's
// start what moves with time moves on by the frames played since the last one began (`since`). From Melodee's
// engine/engine.h.
namespace melodee {

struct ControlStep {
    unsigned left = 0;  // frames of the step in hand still to play; 0: the next frame starts one
    unsigned since = 0; // frames played since the step in hand began
    void restart() { left = 0; } // a change: the next frame starts a step
    // At a frame to play: true when a step starts here (and `frames` is its length); then run() with each run's frames.
    bool starts(unsigned frames) {
        if (left) return false;
        left = frames;
        return true;
    }
    unsigned run(unsigned wanted) const { return wanted < left ? wanted : left; } // the frames before the step ends
    // After a run: true when it ended the step (what is checked once a step, such as a voice let go, comes now).
    bool played(unsigned frames) { left -= frames; since += frames; return left == 0; }
    unsigned take() { unsigned s = since; since = 0; return s; } // at a step's start: the frames since the last one's
};

} // namespace melodee
