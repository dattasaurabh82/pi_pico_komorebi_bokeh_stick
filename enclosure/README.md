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
│   └── cage-06-port-end.png
└── cad/
    └── build_cage.py
```

## Documents

| File Name | Highlights | Why it matters |
| --- | --- | --- |
| [CAGE.md](CAGE.md) | **Steps**, the rules for this cage, the numbers, what goes inside, the printed parts, **open points**, the log | The resume point. *Read it first* |
| [DESIGN_LANGUAGE.md](DESIGN_LANGUAGE.md) | The rules for CAD on this cage, taken over from the solar server enclosure | Read it before modelling |

## CAD

| File Name | Highlights | Why it matters |
| --- | --- | --- |
| [cad/build_cage.py](cad/build_cage.py) | Builds the seven printed parts in the Fusion file `komorebi-cage`. All numbers in the **`P`** table at the top | After a change: run it in the cage's own tab, save, get the latest version in `komorebi-box`, save that. *Then update the numbers in [CAGE.md](CAGE.md#numbers)* |

## Pictures

Screenshots from `komorebi-box`, taken on `2026-11-23`. They get taken again when the cage changes.
