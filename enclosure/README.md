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
│   ├── cage-07-buck-tray.png
│   ├── cage-08-front-grill.png
│   ├── cage-09-blower.png
│   └── doscher/
└── cad/
    ├── build_cage.py
    ├── place_fasteners.py
    └── fasteners.json
```

> [!Important]
> **`assets/doscher` is private.** It holds 32 reference images by Jay Doscher for [DESIGN_LANGUAGE.md](DESIGN_LANGUAGE.md). It is in `.gitignore` and must not be published.

## Follow in this order

1. **[CAGE.md](CAGE.md), [Steps](CAGE.md#steps):** where the work stands and what comes next.
2. **[CAGE.md](CAGE.md), [Rules for this cage](CAGE.md#rules-for-this-cage):** what Saurabh asked for.
3. **[DESIGN_LANGUAGE.md](DESIGN_LANGUAGE.md), [Our rules](DESIGN_LANGUAGE.md#our-rules):** how a part has to look and print. Read it before changing a part.
4. **[CAGE.md](CAGE.md), [Numbers](CAGE.md#numbers) and [Printed parts](CAGE.md#printed-parts):** the sizes, the seams, the front grill, the side panels, the blower.
5. **[cad/build_cage.py](cad/build_cage.py):** do not run it now. It is behind the cage since `2026-11-25`, see [Fusion traps](CAGE.md#fusion-traps). Changes go onto the bodies in `komorebi-cage`, then save.
6. **`komorebi-box` in Fusion:** get the latest version of `komorebi-cage`.
7. **[cad/fasteners.json](cad/fasteners.json), then [cad/place_fasteners.py](cad/place_fasteners.py):** add the rows for new fasteners, run the script in the tab of `komorebi-box`, save.
8. **[CAGE.md](CAGE.md), [Checks](CAGE.md#checks) and [Fusion traps](CAGE.md#fusion-traps):** run the interference check, mind the traps.
9. **[CAGE.md](CAGE.md), [Fasteners](CAGE.md#fasteners):** the counts for the shopping list. They come from [cad/fasteners.json](cad/fasteners.json).
10. **[assets](assets), then the docs:** take the nine pictures again, update the numbers and the log in [CAGE.md](CAGE.md).

## Documents

| File Name | Highlights | Why it matters |
| --- | --- | --- |
| [CAGE.md](CAGE.md) | **Steps**, the rules for this cage, the numbers, what goes inside, the printed parts with their **seams**, the **front grill**, the **side panels** and the **blower**, **fasteners** with the total count, the four reviews, open points, what to **print again**, the log | The resume point. *Read it first* |
| [DESIGN_LANGUAGE.md](DESIGN_LANGUAGE.md) | The design language of this cage, on its own: the **reference** with its images, where we differ, **our rules** for structure, the look, printed parts, fasteners and Fusion, the sources | Read it before modelling |

## CAD

| File Name | Highlights | Why it matters |
| --- | --- | --- |
| [cad/build_cage.py](cad/build_cage.py) | Builds the 13 printed parts of `2026-11-23` in the Fusion file `komorebi-cage`. All numbers in the **`P`** table at the top. The cage has 17 parts now | **Do not run it as it is.** It is behind the cage, see [Fusion traps](CAGE.md#fusion-traps). *A run would undo the work of `2026-11-25`* |
| [cad/place_fasteners.py](cad/place_fasteners.py) | Places the fastener models in `komorebi-box`, in the component **`Fasteners`**. 120 copies of six models | Run it in the tab of `komorebi-box` after the link to the cage is on its latest version. *It can run again and again* |
| [cad/fasteners.json](cad/fasteners.json) | **121 fasteners.** One row each: kind, group, seat point, the direction it goes in. `counts` at the top | Written by the build script on `2026-11-23`, the rows since then added by hand. The fastener models get placed from it |

## Pictures

Screenshots from `komorebi-box`, taken on `2026-11-25`. They get taken again when the cage changes.

All nine show the split cage with the front grill and the fasteners in place. For picture 05 the top plate and its screws are hidden, for pictures 07 and 09 the projector too. Picture 07 also hides the blower and the two plugs.
