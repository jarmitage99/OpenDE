# Tabler Icons subset

Subset of the Tabler Icons webfont 3.49.0 (https://tabler.io/icons, MIT, see LICENSE) used by the editor.

Icons included:

- cube (U+FA97)
- point (U+EB0C)
- line (U+EC40)
- polygon (U+EFD0)
- arrows-move (U+F22F)
- rotate-clockwise-2 (U+EBB5)
- arrows-diagonal (U+EA27)
- arrow-bar-to-up (U+EC8B)
- folder-open (U+FAF7)
- file (U+EAA4)
- device-floppy (U+EB62)
- player-play (U+ED46)
- arrow-back-up (U+EB77)
- arrow-forward-up (U+EB78)
- copy (U+EA7A)
- trash (U+EB41)
- search (U+EB1C)
- hammer (U+EF91)
- focus-2 (U+EBD3)
- grid-dots (U+EABA)
- layout-grid (U+EDBA)
- scissors (U+EB1B)
- flip-horizontal (U+EAA7)
- eye (U+EA9A)
- maximize (U+EAEA)
- minimize (U+EAF1)
- list-tree (U+FAFA)
- hierarchy-2 (U+EE9D)
- photo (U+EB0A)
- brush (U+EBB8)
- texture (U+F51B)
- volume (U+EB51)
- music (U+EAFC)
- box-model-2 (U+EF23)
- code (U+EA77)
- tool (U+EB40)
- terminal-2 (U+EBEF)
- ruler-measure (U+F291)
- angle (U+EF20)
- perspective (U+EEBD)
- building-bridge-2 (U+EA4A)
- file (U+EAA4)
- pencil (U+EB04)
- user (U+EB4D)
- eye (U+EA9A)

Regenerate after changing the list:

```
python3 -m fontTools.subset tabler-icons.ttf --unicodes=<codes above> --output-file=subset.ttf --no-hinting --layout-features= --name-IDs='*' --notdef-outline
binary_to_compressed_c subset.ttf TablerIcons > tabler_icons.h
```

`binary_to_compressed_c` is `vendor/imgui/misc/fonts/binary_to_compressed_c.cpp`. Prepend `#pragma once` to the output.
