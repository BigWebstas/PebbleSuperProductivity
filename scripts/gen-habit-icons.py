#!/usr/bin/env python3
"""Render a curated set of Material icons to Pebble bitmap resources for the
per-habit icon on the Habits list.

Source font + codepoints ship with the Flutter SDK cache (Apache-2.0). We only
rasterise the names in ICON_NAMES, at SIZE px, black-on-transparent, under
resources/images/hi/. Habit rows are black-on-white / black-on-cerulean, never
inverted, so no white variant is needed.

The phone maps a habit's Material icon name to its INDEX in this list and sends
that int in HABIT_ICON; the watch just indexes a resource-id array. So the name
strings live only on the phone side (habit-icon-map.js), never on the watch.

Emits:
  - resources/images/hi/*.png        : the glyphs
  - scripts/habit-icons.media.json   : "media" entries, spliced into package.json
  - src/c/habit_icons.h              : index -> RESOURCE_ID array + count
  - src/pkjs/lib/habit-icon-map.js   : { "<name>": <index> }

Re-run + `pebble clean` whenever ICON_NAMES changes. Requires Pillow.
"""
import json
import os

from PIL import Image, ImageFont, ImageDraw

FONT = "/opt/flutter/bin/cache/artifacts/material_fonts/MaterialIcons-Regular.otf"
CODEPOINTS = "/opt/flutter/bin/cache/artifacts/material_fonts/codepoints"
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(ROOT, "resources", "images", "hi")
SIZE = 20  # box the glyph is fitted into; matches the habit-row icon slot

# Curated Material icon names (as Super Productivity stores them). Order is the
# wire index - only ever APPEND, never reorder or delete, or existing habits
# repoint. The watch stores the index in an int8, so keep this under 128.
ICON_NAMES = [
    # movement / fitness
    "directions_walk", "directions_run", "directions_bike", "fitness_center",
    "self_improvement", "pool", "sports_gymnastics", "sports_soccer",
    "sports_basketball", "sports_tennis", "sports_martial_arts", "hiking",
    "downhill_skiing", "skateboarding", "rowing", "pedal_bike",
    # food / drink
    "free_breakfast", "local_cafe", "restaurant", "local_bar", "no_drinks",
    "water_drop", "local_pizza", "lunch_dining", "dinner_dining",
    "bakery_dining", "egg", "local_drink", "coffee", "no_food", "ramen_dining",
    "icecream",
    # mind / health / sleep
    "bedtime", "medication", "psychology", "spa", "healing", "monitor_heart",
    "medical_services", "mood", "sentiment_satisfied", "air", "waves",
    # learning / creativity
    "book", "menu_book", "school", "edit", "brush", "code", "language",
    "music_note", "auto_stories", "history_edu", "science", "calculate",
    "palette", "piano", "mic", "headphones", "translate", "draw", "keyboard",
    "photo_camera", "movie",
    # chores / home
    "cleaning_services", "local_laundry_service", "wash", "checkroom", "bed",
    "kitchen", "yard", "grass", "handyman", "recycling",
    # finance / work
    "savings", "shopping_cart", "payments", "account_balance_wallet", "work",
    "business_center", "attach_money", "trending_up", "receipt_long",
    # nature / outdoors
    "local_florist", "park", "pets", "eco", "forest", "terrain", "beach_access",
    "cloud", "water",
    # social / misc
    "favorite", "star", "wb_sunny", "nightlight", "smoke_free", "phone_iphone",
    "call", "chat", "groups", "volunteer_activism", "celebration",
    "emoji_events", "flag", "alarm", "timer", "event", "today", "task_alt",
    "checklist", "do_not_disturb_on",
]


def load_codepoints():
    cp = {}
    with open(CODEPOINTS) as fh:
        for line in fh:
            parts = line.split()
            if len(parts) == 2:
                cp[parts[0]] = int(parts[1], 16)
    return cp


def render(char):
    img = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    # Fit the glyph: Material glyphs are designed on a 24px em, draw at SIZE+4
    # and trim so odd metrics still land centred.
    font = ImageFont.truetype(FONT, SIZE + 4)
    big = Image.new("RGBA", (SIZE + 8, SIZE + 8), (0, 0, 0, 0))
    ImageDraw.Draw(big).text((4, 2), char, font=font, fill=(0, 0, 0, 255))
    bbox = big.getbbox()
    if bbox:
        glyph = big.crop(bbox)
        glyph.thumbnail((SIZE, SIZE), Image.LANCZOS)
        img.paste(glyph, ((SIZE - glyph.width) // 2, (SIZE - glyph.height) // 2), glyph)
    # Keep the anti-aliased alpha: basalt/chalk/emery are 64-colour and render it
    # smoothly; diorite (1-bit) thresholds it itself. A gentle lift so faint
    # edges from the downscale don't disappear entirely.
    r, g, b, a = img.split()
    a = a.point(lambda v: 0 if v < 24 else min(255, int(v * 1.35)))
    return Image.merge("RGBA", (r, g, b, a))


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    cp = load_codepoints()
    media = []
    resids = []       # index -> RESOURCE_ID name
    name_to_index = {}
    missing = []
    for name in ICON_NAMES:
        code = cp.get(name + "_baseline") or cp.get(name)
        if not code:
            missing.append(name)
            continue
        slug = name.replace("_", "-")
        render(chr(code)).save(os.path.join(OUT_DIR, slug + ".png"))
        res = "IMAGE_HI_" + name.upper()
        media.append({
            "type": "bitmap",
            "name": res,
            "file": "images/hi/" + slug + ".png",
            # aplite draws no per-habit icon (the feature is non-aplite); keep
            # them out of its pbpack.
            "targetPlatforms": ["basalt", "chalk", "diorite", "emery"],
        })
        name_to_index[name] = len(resids)
        resids.append(res)

    assert len(resids) < 128, "watch stores the icon index in an int8"

    with open(os.path.join(ROOT, "scripts", "habit-icons.media.json"), "w") as fh:
        json.dump(media, fh, indent=2)

    with open(os.path.join(ROOT, "src", "c", "habit_icons.h"), "w") as fh:
        fh.write("// Generated by scripts/gen-habit-icons.py - do not edit.\n")
        fh.write("// Per-habit icon: the phone sends the index into this array in HABIT_ICON.\n")
        fh.write("#pragma once\n\n")
        fh.write("static const uint32_t HABIT_ICON_RES[] = {\n")
        for res in resids:
            fh.write("  RESOURCE_ID_%s,\n" % res)
        fh.write("};\n")
        fh.write("#define HABIT_ICON_COUNT %d\n" % len(resids))

    with open(os.path.join(ROOT, "src", "pkjs", "lib", "habit-icon-map.js"), "w") as fh:
        fh.write("// Generated by scripts/gen-habit-icons.py - do not edit.\n")
        fh.write("// Material icon name (as Super Productivity stores it) -> wire index\n")
        fh.write("// (index into main.c's HABIT_ICON_RES[]).\n")
        fh.write("'use strict';\n\n")
        fh.write("module.exports = " + json.dumps(name_to_index, indent=2) + ";\n")

    print("rendered %d icons" % len(resids))
    if missing:
        print("MISSING codepoints (skipped):", ", ".join(missing))


if __name__ == "__main__":
    main()
