# Komorebi box cage: working notes

State and decisions for the printed open cage around the YG300 projector, the Pico and the controls.

The design rules are in [DESIGN_LANGUAGE.md](DESIGN_LANGUAGE.md). This file tracks what goes in, what is decided, and where the work stands.

Fusion files, in the project `AA`, folder `smartProjector / prototyping`:

- **`komorebi-box`:** the assembly. It links every bought part and the cage.
- **`komorebi-cage`:** the printed parts only. Built by [cad/build_cage.py](cad/build_cage.py).

![Cage from the lens side](assets/cage-01-front-right.png)

![Cage from the back, controls on the rear frame](assets/cage-02-rear-left.png)

More views are in [assets](assets): the underside with the two modules, the top, the bay without the top plate, the port end.

---

## Steps

1. [x] **Read `komorebi-box` and measure the parts.** Done `2026-11-23`. Eleven linked parts, nothing missing.
2. [x] **Decide the layout.** Done `2026-11-23`, with Saurabh: rear bay, with a top plate.
3. [x] **Rough cage, parts placed on it.** Done `2026-11-23`. Seven printed parts, no fastener holes yet.
4. [ ] **Review by Saurabh.** Open. Nothing gets fine tuned before it.
5. [ ] **Fine tuning:** sizes against the real projector, window shapes, the holders.
6. [ ] **Fasteners:** heat-set inserts, screws, nuts. Saurabh brings the models in once the places are named.
7. [ ] **Test prints**, then the assembly notes.

---

## Rules for this cage

From Saurabh on `2026-11-23`:

- **No case around it.** The cage is the enclosure, and it is open so the projector can breathe.
- **The lens looks out of the front.** The DC socket, the rotary encoder and the button sit on the opposite side.
- **The Pico and the DVI sock are one rigid block.** They are soldered pin to pin.
- **The two CY HDMI plugs and their ribbon are reused.** The ribbon is 30 cm or longer and is not modelled.
- **Wires are not planned yet.** Not between button, converter, encoder and light sensor either.
- **The light sensor has its own module.** It fits on the bottom or on the top.
- **The magic arm adapter fits on the bottom or on the top too.** The cage stands on a tripod or hangs from a ceiling. Its thread is printed and tested, it must point away from the cage.
- **Fasteners come later.** No heat-set inserts, screws or nuts in this round.
- **Every part prints on the Prusa Mini bed,** 180 x 180 mm.
- **Enough room, not too much.** Tighter than the solar server cage.
- **No assembly documents yet.** Design first.

---

## Numbers

All in mm, in the coordinates of `komorebi-box`: Y is up, the lens face is +Z, the projector's ports are on the -X end. The same numbers are the `P` table at the top of [cad/build_cage.py](cad/build_cage.py).

```json
{
  "projector": {
    "x": [-86.1, 39.9],
    "y": [3.8, 50.8],
    "z": [-43.8, 36.2],
    "feet_bottom_y": 1.8,
    "own_tripod_thread": [-32.3, 20.3]
  },
  "cage": {
    "inner_x": [-103.1, 56.9],
    "inner_z_rear": -80.0,
    "front_edge_z": 40.0,
    "floor_top_y": 1.8,
    "top_plate_bottom_y": 52.8,
    "outside": [168.0, 61.0, 124.0]
  },
  "gaps": {
    "port_end": 17.0,
    "lever_end": 17.0,
    "rear_bay": 36.2,
    "above_projector": 2.0,
    "lens_face_to_front_edge": 3.8
  },
  "thickness": {
    "plates": 5.0,
    "frame_web": 4.0,
    "flange": 5.0,
    "control_field": 3.0,
    "at_button": 2.0
  },
  "module_port": {
    "opening_d": 24.0,
    "hole_square": 31.82,
    "tripod_port": [-23.1, -8.0],
    "sensor_port": [-23.1, -58.0]
  },
  "controls": {
    "height_y": 27.3,
    "jack_x": -91.0,
    "button_x": -75.0,
    "encoder_x": -45.0
  }
}
```

> [!Important]
> **The projector model is 126 x 80 x 47.** Shop listings give 126 x 86 x 47.6 for the YG300. The depth may be 6 mm short in the model, or the listing counts the lens ring. The cage takes both: the bay still has 30 mm then. The lips on the floor do not. They wait for a measurement of the real projector.

---

## What goes inside

Sizes are measured in `komorebi-box`. Use `root.allOccurrences` for that, see [Fusion traps](#fusion-traps).

| Part | Fusion file | Size | Where it sits | Held by |
| --- | --- | --- | --- | --- |
| Projector | `YG300 LCD Projector` | 126 x 47 x 80, feet 2 | On its feet on the floor plate, lens to +Z | Four corner lips, later a 1/4 inch screw into its own tripod thread |
| Pico with DVI sock and HDMI plug | `PiPico2AndDVISock` | 74.3 x 22.9 x 16.1 | Upright on the rear frame, parts toward the projector, USB toward the +X end | `Pico holder: base`, four posts |
| HDMI plug at the projector | `HDMI-CY-17-UP` | 16.2 x 14.5, sticks out 9 | In the projector's port on the -X end | The port |
| Second HDMI plug | `HDMI-CY-17-DN` | same | Not used, switched off in the browser | Nothing |
| Rotary encoder with knob | `RotaryEncoderAndGuiterMetalKnob` | knob 25 across, 19.5 high | Rear frame, pins toward the Pico | Its own nut, 7.2 hole |
| Button | `Switch Assembly` | 9.6 x 27 | Rear frame | Its own nut, 7.2 hole, panel 2 thick there |
| DC socket | `Socket Panel (1)` | 21 across the nut | Rear frame, at the -X end | Its own nut, 11.2 hole |
| Buck converter | `DC-DC 4.5-28V to 0.8-20V Output 3A MP1584EN Step Down Buck Converter` | 22.4 x 17.1 x 4.6 | Flat on the bay floor under the socket | `Buck converter holder: tray` |
| Light sensor | `BH1750` | flange 28 across, dome 25.5 | In its module, now on the floor, dome down | `Light sensor module: plate` |
| Tripod adapter | `Bottom Adapter for Magic Arm Nut` | 60 x 60 x 10 | On the floor's tripod port, turned 45 degrees | Four M3 from outside, later |

The models say `Raspberry_Pi_Pico_W` and `Adafruit DVI Sock`. The real boards are the Pico 2 W and the pi3g sock. Same footprint.

---

## Printed parts

In `komorebi-cage`. Every part lies flat on the bed and needs no supports.

| Part | Component | Copies | Size | What it is |
| --- | --- | --- | --- | --- |
| Floor plate | `Cage: floor plate` | 1 | 159.6 x 119.8 x 12 | 5 thick. Four corner lips, 7 high, locate the projector. Two module ports. Three windows and an obround for the 1/4 inch screw |
| Top plate | `Cage: top plate` | 1 | 159.6 x 119.8 x 5 | Same outline and the same two ports. Open over the projector's buttons, a diagonal in each rear window |
| End frame | `Cage: end frame` | 2 | 120 x 61 x 14 | Web 4 thick with a bay window and a projector window, a flange at the bottom and at the top. The +X one is the same part turned half round |
| Rear frame | `Cage: rear frame` | 1 | 168 x 61 x 14 | Web 4 thick, two rows of slot vents, the three control holes, two flanges |
| Pico holder | `Pico holder: base` | 1 | 61.5 x 24 x 7.9 | Plate on the inside of the rear frame with four posts under the mounting holes |
| Buck tray | `Buck converter holder: tray` | 1 | 26.4 x 21.1 x 4.8 | Flat tray with four corner walls |
| Sensor module | `Light sensor module: plate` | 1 | 44 x 44 x 4 | Plate with the dome hole and a seat for the sensor's flange |

How it is built:

- **Two plates and three upright frames make the box.** The front stays open.
- **A frame is a web with two flanges.** The flanges lie on the inside of the plates. Later they carry the heat-set inserts, and the screws go in from outside through the plates. So the plates stay plain.
- **The plates sit inside the frames.** The webs cover their edges.
- **The projector is centred between the two end frames,** 17 mm of room at each end. That is where the HDMI plug, the power plug and the folded ribbon go on one end, and a finger for the focus lever on the other.
- **The rear bay is 36 mm deep.** Controls and Pico share the rear frame side by side, so air stays between the Pico and the projector's back vents: 14 mm at the HDMI plug, 24 mm along the board.
- **The control field is thinned to 3 mm on the inside,** 2 mm at the button. The models of the socket and the encoder are drawn for a 3.2 panel, and the button thread is short.
- **The bay window of the +X end frame is where a USB cable reaches the Pico.**
- **The floor is closed where the feet stand.** The feet are 14.8 square at the four corners.

### Module ports

A port is a 24 mm opening with four M3 holes on a 31.82 mm square. That is the hole pattern of the magic arm adapter, turned 45 degrees.

- **Both plates have the same two ports,** at the same place: one near the centre of gravity, one in the bay.
- **Both modules fit both ports.** So the tripod adapter and the light sensor each go on the bottom or on the top.
- **The sensor's flange is trapped** between its module plate and the cage plate. The dome looks out, the connector reaches through the opening into the bay.
- **The sensor belongs on the bay port.** On the centre port its connector would hit the projector.

![Underside with the sensor module and the tripod adapter](assets/cage-03-underside.png)

---

## Checks

Done on `2026-11-23` in `komorebi-box` with the interference tool, hidden parts left out.

- **No part cuts into another.** Two touches under 0.1 mm stay: the socket's flange on the panel, and the HDMI plug inside the projector's port.
- **Every printed part is inside 180 x 180.** The rear frame is the longest at 168.
- **The projector's buttons, focus lever, ports and vents are all open.**

---

## Open points

For the review:

- [ ] **Real size of the projector,** width, depth, height, and how far the lens ring stands out.
- [ ] **Which plug goes into the projector.** `HDMI-CY-17-UP` is plugged in now. Both CY models are 10.5 wide, which is mini HDMI size.
- [ ] **Order of the controls.** From behind, left to right: knob, button, socket.
- [ ] **Rear frame at 168 mm.** It fits the bed with 6 mm to spare on each side. I aimed for 170 at most.
- [ ] **Top plate: keep it, or lighter.** It carries the ports for hanging.
- [ ] **Floor plate is rather closed.** It could get slot vents in the bay.
- [ ] **Lips or loose stops.** The lips are printed onto the floor. Loose stops with slots would follow the real projector without a new floor.
- [ ] **A place to fold the spare ribbon.** The -X end has 17 mm.
- [ ] **How the rear frame is stiffened** between its ends. It is held by its two flanges only.

---

## Fusion traps

> [!Warning]
> **Run the build script only in the cage's own tab.** `komorebi-box` keeps its own copy of `komorebi-cage` open. `documents.item(i).activate()` can land on that copy. A change made there shows in the assembly but is never saved. Pick the document that is not modified, check `isModified` after the save, then get the latest version in `komorebi-box`.

- **Measure nested parts with `root.allOccurrences`.** Walking `occurrence.childOccurrences` gives boxes in the wrong frame. It made the buck converter look flat when it stood on edge.
- **A camera set by script keeps perspective.** The zoom is the distance from eye to target, `viewExtents` is ignored. About 45 cm shows the whole cage.
- **The cage parts are plain bodies in base features.** The script builds them from boxes and cylinders and swaps the body in place, so the links stay valid. They have no sketches. A part Saurabh wants to edit by hand gets rebuilt with sketches then.

---

## Log

### 2026-11-23

- Read `komorebi-box` version 15. Eleven linked parts, no joints.
- Asked four things. Answers: rear bay with a top plate, the sensor and the tripod adapter as modules for top or bottom, ribbon 30 cm or longer, nothing missing.
- New Fusion file `komorebi-cage`, hybrid design, saved as version 3. Seven components, eight occurrences, one rigid group.
- Cage linked into `komorebi-box`, parts placed, `HDMI-CY-17-DN` switched off. Saved as version 17.
- Found on the way: the second `HDMI-CY-17-UP` was parked beside the projector, not plugged in. The buck converter stood on edge. Both are placed now.
