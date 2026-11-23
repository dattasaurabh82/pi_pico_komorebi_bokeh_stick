# Enclosure

The printed open cage for the komorebi box: the YG300 projector, the Pico with its DVI sock, the controls and the light sensor.

Start with [CAGE.md](CAGE.md). It says where the work stands.

```text
enclosure/
├── README.md
├── CAGE.md
├── DESIGN_LANGUAGE.md
├── assets/
│   ├── cage-01-front-right.png
│   ├── cage-02-rear-left.png
│   ├── cage-03-underside.png
│   ├── cage-04-top.png
│   ├── cage-05-bay-without-top-plate.png
│   ├── cage-06-port-end.png
│   ├── cage-07-buck-snap-tray.png
│   └── cage-08-front-grill.png
└── cad/
    ├── build_cage.py
    ├── place_fasteners.py
    └── fasteners.json
```

## Follow in this order

1. **[CAGE.md](CAGE.md), [Steps](CAGE.md#steps):** where the work stands and what comes next.
2. **[CAGE.md](CAGE.md), [Rules for this cage](CAGE.md#rules-for-this-cage):** what Saurabh asked for.
3. **[DESIGN_LANGUAGE.md](DESIGN_LANGUAGE.md), [Our rules](DESIGN_LANGUAGE.md#our-rules):** how a part has to look and print. Read it before changing a part.
4. **[CAGE.md](CAGE.md), [Numbers](CAGE.md#numbers) and [Printed parts](CAGE.md#printed-parts):** the sizes, the seams, the front grill, the module ports.
5. **[cad/build_cage.py](cad/build_cage.py):** change a number in the `P` table, run it in the tab of `komorebi-cage`, save.
6. **`komorebi-box` in Fusion:** get the latest version of `komorebi-cage`.
7. **[cad/place_fasteners.py](cad/place_fasteners.py):** run it in the tab of `komorebi-box`, save.
8. **[CAGE.md](CAGE.md), [Checks](CAGE.md#checks) and [Fusion traps](CAGE.md#fusion-traps):** run the interference check, mind the traps.
9. **[CAGE.md](CAGE.md), [Fasteners](CAGE.md#fasteners):** the counts for the shopping list. They come from [cad/fasteners.json](cad/fasteners.json).
10. **[assets](assets), then the docs:** take the eight pictures again, update the numbers and the log in [CAGE.md](CAGE.md).

## Documents

| File Name | Highlights | Why it matters |
| --- | --- | --- |
| [CAGE.md](CAGE.md) | **Steps**, the rules for this cage, the numbers, what goes inside, the printed parts with their **seams** and the **front grill**, **fasteners** with the total count, the three reviews, open points, the log | The resume point. *Read it first* |
| [DESIGN_LANGUAGE.md](DESIGN_LANGUAGE.md) | The rules for CAD on this cage, taken over from the solar server enclosure: structure, **the look**, printed parts, fasteners, Fusion | Read it before modelling |

## CAD

| File Name | Highlights | Why it matters |
| --- | --- | --- |
| [cad/build_cage.py](cad/build_cage.py) | Builds the 13 printed parts in the Fusion file `komorebi-cage`, 14 pieces with the second strap. All numbers in the **`P`** table at the top | After a change: run it in the cage's own tab, save, get the latest version in `komorebi-box`, save that. *Then update the numbers in [CAGE.md](CAGE.md#numbers)* |
| [cad/place_fasteners.py](cad/place_fasteners.py) | Places the fastener models in `komorebi-box`, in the component **`Fasteners`**. 108 copies of six models | Run it in the tab of `komorebi-box` after the link to the cage is on its latest version. *It can run again and again* |
| [cad/fasteners.json](cad/fasteners.json) | **109 fasteners.** One row each: kind, group, seat point, the direction it goes in. `counts` at the top | Written by the build script, *do not edit by hand*. The fastener models get placed from it |

## Pictures

Screenshots from `komorebi-box`, taken on `2026-11-23`. They get taken again when the cage changes.

All eight show the split cage with the front grill and the fasteners in place. For picture 05 the top plate is hidden, for picture 07 the projector too.
