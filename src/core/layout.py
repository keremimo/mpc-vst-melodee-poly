# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 Kerem Kilic (Ellic Studio)
"""A Melodee synth's MPC page, vst/layout.conf (mpc-vst-plugins' Force Shadow widget syntax), from a short spec.

Every tab is two rows of eight slots, as MPC's two banks of eight Q-Links: row 1 is Q-Links 1-8, row 2 is 9-16, so
a control's place on the screen is the knob that turns it. A row holds frames (a titled group of slots, left to
right); a control's kind follows its parameter (vst/params.json): a number is a knob, Off/On a toggle, up to nine
options a column of segments (every option on screen, nothing opens over a neighbour), more a pop-up list. The mod
matrix is two tabs, routes 1-4 and 5-8, a route a column pair: its amount on top, source and destination side by
side under it with nothing below, so their pop-up lists open into empty space. Colours: Melodee's NIGHT palette,
the synth's own accent.

    from core.layout import write
    write("vst/layout.conf", "vst/params.json", accent="FF7A5C", tabs=[("OSC", [[("OSC A", [("osc_a_freq", "FREQ")])], []])])
"""
import json

W, H, Y_OFF = 1280, 628, 86   # MPC's plugin area, from y=86 in the layout's (Force Shadow) coordinates
X0, Y0, SLOT, ROW = 20, Y_OFF + 14, 155, 304
FIRST = 100                    # a column's first option (its centre) under the row's top: its label 38 px higher,
                               # clear of the frame's title rule at +40


def theme(accent, accent_hi):
    return {
        "bg": "17141F", "panel": "1D1926", "line": "2E2939", "ink": "F3EEF8", "ink_dim": "9A93A8", "ink_faint": "5A5468",
        "accent": accent, "accent_hi": accent_hi, "knob_face": "262033", "knob_ring": "3A3448", "knob_dot": "F3EEF8",
        "bar": "1A1722", "lcd": "221E2C", "box": "1D1926", "btn_bg": "262033", "btn_text": "F3EEF8",
        "seg_active": accent, "seg_inactive": "221E2C", "seg_active_tx": "17141F", "tab_on": accent, "tab_on_tx": "17141F",
    }


def short(name):
    return name.upper()


def control(p, label, cx, cy, popup=False, row_top=0):
    opts = p.get("options")
    key = p["key"]
    if not opts:
        return 'knob cx=%d cy=%d r=40 label="%s" key=%s' % (cx, cy, label, key)
    if [o.lower() for o in opts] == ["off", "on"]:
        return 'toggle cx=%d cy=%d label="%s" key=%s' % (cx, cy, label, key)
    names = ",".join(short(o) for o in opts)
    if len(opts) <= 9 and not popup:
        # Every option on screen, one a row 2 px apart, as tall as fits above the frame's foot (row_top + 288). enum_h
        # with a row per option, not enum_v: enum_h puts its label a fixed 38 px over the first option, enum_v as if
        # every option were 30 px tall, which floats a long column's label up into the frame's title.
        n = len(opts)
        sh = min(34, 2 * (ROW - 14 - FIRST - 2 * n) // (2 * n - 1))   # FIRST - sh/2 + n(sh + 2) - 2 <= ROW - 16
        cy = row_top + FIRST
        return 'enum_h cx=%d cy=%d label="%s" key=%s options="%s" sw=%d sh=%d rows=%d' % (cx, cy, label, key, names, SLOT - 24, sh, n)
    return 'popup cx=%d cy=%d w=%d h=40 label="%s" key=%s options="%s"' % (cx, cy, SLOT - 18, label, key, names)


def tab_lines(name, rows, params):
    out = ["", "[tab %s]" % name]
    order = []
    for r, frames in enumerate(rows):
        col = 0
        y = Y0 + r * ROW
        for title, controls in frames:
            n = len(controls)
            out.append('frame x=%d y=%d w=%d h=%d title="%s"' % (X0 + col * SLOT + 4, y + 4, n * SLOT - 8, ROW - 12, title))
            for key, label in controls:
                if key not in params:
                    raise SystemExit("layout: no parameter %r" % key)
                cx = X0 + col * SLOT + SLOT // 2
                out.append(control(params[key], label, cx, y + ROW // 2 + 14, row_top=y))
                order.append(key)
                col += 1
            if col > 8:
                raise SystemExit("layout: tab %s row %d has %d slots (8 at most)" % (name, r + 1, col))
    if len(order) > 16:
        raise SystemExit("layout: tab %s has %d controls (16 Q-Links)" % (name, len(order)))
    out.append('qlinks "%s" = %s' % (name, ",".join(order)))
    return out


def matrix_lines(params, routes=8):
    out = []
    for first in range(1, routes + 1, 4):
        name = "MOD %d-%d" % (first, first + 3)
        out += ["", "[tab %s]" % name]
        keys = []
        for r in range(4):
            n = first + r
            x = X0 + r * 2 * SLOT
            out.append('frame x=%d y=%d w=%d h=%d title="MOD %d"' % (x + 4, Y0 + 4, 2 * SLOT - 8, 2 * ROW - 12, n))
            out.append(control(params["mod%d_amt" % n], "AMOUNT", x + SLOT, Y0 + 120))
            out.append(control(params["mod%d_src" % n], "SOURCE", x + SLOT // 2, Y0 + 290, popup=True))
            out.append(control(params["mod%d_dst" % n], "DEST", x + SLOT + SLOT // 2, Y0 + 290, popup=True))
            keys += ["mod%d_src" % n, "mod%d_dst" % n, "mod%d_amt" % n]
        out.append('qlinks "%s" = %s' % (name, ",".join(keys)))
    return out


def write(path, params_path, accent, accent_hi, tabs, synth):
    params = {p["key"]: p for p in json.load(open(params_path))["params"]}
    lines = ["# generated by tools/make_layout.py (src/core/layout.py): %s on MPC's %dx%d plugin area, Melodee's NIGHT palette" % (synth, W, H)]
    lines += ["theme_%s=%s" % kv for kv in theme(accent, accent_hi).items()]
    lines.append("art_css=skin.css")   # Melodee's Rubik for the drawn text (vst/skin.css)
    first = tabs[0]
    lines.append("qlinks_track = %s" % ",".join(k for frames in first[1] for _, cs in frames for k, _ in cs))
    for name, rows in tabs:
        lines += tab_lines(name, rows, params)
    lines += matrix_lines(params)
    open(path, "w").write("\n".join(lines) + "\n")
