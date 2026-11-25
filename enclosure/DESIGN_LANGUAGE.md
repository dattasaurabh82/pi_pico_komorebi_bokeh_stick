# Enclosure design language

This is the design language for the komorebi box cage: an open printed cage around a small projector, a Pico and a few controls.

It is written for agents and people who will do CAD on it. Read it before you model anything.

It stands on its own. The reference, the images, the rules and the sources are all in this folder.

> [!Important]
> The images in [assets/doscher](assets/doscher) are Jay Doscher's photos and CAD renders, copied from his posts for private reference. **Do not publish this folder.** It is in `.gitignore`, so it is on Saurabh's Mac only and the image links below do not show on GitHub.

---

## Contents

- [How to use this file](#how-to-use-this-file)
- [The language in short](#the-language-in-short)
- [Reference: Jay Doscher](#reference-jay-doscher)
  - [1. Small flat parts, joined](#1-small-flat-parts-joined)
  - [2. Repeated panels and corners](#2-repeated-panels-and-corners)
  - [3. One adapter per device](#3-one-adapter-per-device)
  - [4. Print flat, no supports](#4-print-flat-no-supports)
  - [5. Screws and threads](#5-screws-and-threads)
  - [6. Wires get a place](#6-wires-get-a-place)
  - [7. Air gaps](#7-air-gaps)
  - [8. Build it up in steps](#8-build-it-up-in-steps)
  - [9. The look](#9-the-look)
- [Where we differ](#where-we-differ)
- [Our rules](#our-rules)
  - [Structure](#structure)
  - [The look](#the-look)
  - [Printed parts](#printed-parts)
  - [Fasteners](#fasteners)
  - [In Fusion](#in-fusion)
- [Open questions](#open-questions)
- [Log](#log)
- [Sources](#sources)

---

## How to use this file

1. **Read the short version first.** It is the checklist.
2. **Look at the images of a principle before you model that kind of part.** The images carry the proportions and the details that the text does not.
3. **Follow [Our rules](#our-rules) when you model.** They are what we decided for this cage.
4. **Values marked start value are not tested.** Check them with a test print before they go into the model.
5. **Add to the [Log](#log) when something is learned or decided.** Then update the rule it changes.
6. **A new reference gets its own section** next to [Reference: Jay Doscher](#reference-jay-doscher), with its own folder under `assets/`.
7. **The state of the cage is not here.** Sizes, parts, counts and open points are in [CAGE.md](CAGE.md).

---

## The language in short

- **The cage is the outside.** No case around it. It is open, so the projector can breathe and hands can reach.
- Every device gets **its own adapter**. A new device means a new adapter, nothing else.
- Parts are **small, flat and repeated**. They are bolted together, not printed as one big piece.
- Everything **prints flat without supports**. Never a chamfer on the bed face.
- **One screw size for the cage.** Screws go into heat-set inserts, they show, and they are reachable from outside.
- **A side or a front is a row of equal truss bays.** No window made for one plug.
- **Air gaps are designed**, not left over. Nothing sits in front of a vent.
- **Wires keep their room**, even where they are not modelled yet.
- The look is **plain and honest**: open trusses, cut window corners, slot vents in rows, one dark matte colour, visible screws.

---

## Reference: Jay Doscher

Jay Doscher builds small computers into cases and printed frames. He has done it since 2015 and writes up what worked and what did not.

Each principle below says what he does, where it is seen, and what it means for this cage.

### 1. Small flat parts, joined

Big parts warp at the bed edge and take a day to print. He splits them and joins the pieces with a flat sheet, a joiner part, or long rods.

![Kuiper Deck CAD, exploded: several small printed pieces around a clear front sheet](assets/doscher/kuiper-cad-exploded-split-panel.png)

*Kuiper Deck: small printed parts bolted through a clear sheet.*

![Quick Kit Mini variant in a case, made of four printed parts](assets/doscher/qk2024-mini-variant-small-bed.jpg)

*Quick Kit Mini: the same frame as four parts, made for 180 x 180 mm beds.*

![Mil-Plastic deck: two flat printed frame halves with mounting tabs](assets/doscher/milplastic-flat-printed-parts.jpg)

*Mil-Plastic: the front frame is two halves, held together by a joiner part.*

- **Seen in:** Kuiper Deck, Quick Kit Mini, Mil-Plastic deck.
- **His reason:** his printer failed often when printing close to the bed edge. The centre of the bed is the safe area.
- **For us:** no part is larger than 120 x 120 on the 180 x 180 bed. Floor, top and rear are two parts each. A strap or a part that crosses the seam joins them.

### 2. Repeated panels and corners

One panel shape is used many times. Variants differ only in their cut-out.

![ARM Terminal Cube CAD: cube frame of six panels with X braces](assets/doscher/cube-cad-frame.png)

*ARM Terminal Cube: six panels joined at the edges.*

![ARM Terminal Cube CAD: one side panel with a rectangular opening for ports](assets/doscher/cube-cad-side-panel.png)

*One side panel. The other sides are the same size with a different opening.*

![Compute Unit: four corner pieces with matching side panels and a top plate](assets/doscher/compute-unit-corners-and-panels.jpg)

*Compute Unit: four near-identical corners and matching side panels.*

![Quick Kit 2024 unibody frame on its own, with panel connectors fitted](assets/doscher/qk2024-unibody-frame-with-ports.jpg)

*Quick Kit 2024: the frame is the structure, the ports sit in it.*

- **Seen in:** ARM Terminal, Cube, Flat Hack, Compute Unit, N100 Obelisk.
- **Detail:** on the Cube, any two sides come off without the rest falling apart.
- **Detail:** to make the Flat Hack deeper, he makes the side panels deeper. Nothing else changes.
- **For us:** the two end frames have one web, three truss bays, turned for the other side. The two plates share outline, module ports and screw lines.

### 3. One adapter per device

The frame does not know the device. A small adapter sits between them. A different device needs a different adapter and nothing else.

![Flat Hack from the back: adapter plate with a window that gives access to RAM and SSD](assets/doscher/flathack-adapter-plate-and-access-window.jpg)

*Flat Hack: the back plate has a window, so parts can be changed without taking the frame apart.*

![N100 Obelisk: a mini PC held between a top and a bottom adapter with a gap to the next one](assets/doscher/obelisk-top-bottom-adapter-pairs.jpg)

*Obelisk: each mini PC sits between a top and a bottom adapter.*

- **Seen in:** Flat Hack, Obelisk, Recovery Kit 3, Open Frame.
- **His hindsight on one build:** one adapter with all the mounts would have cut the part count.
- **For us:** Pico holder, buck tray, sensor module, terminal block clamp bar, blower bracket, Wago bracket. The cage got none of them as a cut-out for one device, only inserts.

### 4. Print flat, no supports

Every part lies flat on the bed. Overhangs are chamfers. Where a tab would need support, he prints a small block under it that snaps off.

![First Recovery Kit frame with small grey blocks broken off next to pliers](assets/doscher/rk1-sacrificial-support-blocks.jpg)

*First Recovery Kit: sacrificial blocks held up the tabs during the print.*

![Small Meshtastic radio case made of a bottom, a middle and a lid](assets/doscher/meshtastic-simple-three-part-case.jpg)

*Meshtastic case: bottom, middle, lid. No supports.*

- **His print guidelines** for one frame: at least 4 walls, 20 to 35 percent infill, every file flat, no supports.
- **Materials:** PETG for most builds, PLA for the newer desk builds.
- **His warning on heat:** printed parts in the sun or in a car get soft.
- **For us:** flat, no supports, PLA with a brim. A frame lies on its web. A part that cannot lie flat prints on the face from which every feature rises, like the blower bracket.

### 5. Screws and threads

He keeps the number of screw sizes low. How the screw holds in plastic changed over the years.

![Open Frame computer: thin inner frame with brass threaded inserts at the corners](assets/doscher/open-frame-inner-frame-with-inserts.jpg)

*Open Frame: the inner frame parts carry the threaded inserts, so the outer panels stay plain.*

![Close-up of a brass heat-set insert and a standoff in a printed corner](assets/doscher/open-frame-insert-closeup.jpg)

*A heat-set insert and a standoff, close up.*

| When | Build | What he did | Why |
| --- | --- | --- | --- |
| `2020` | ARM Terminal | Screws pressed into printed holes, then heat-set inserts | The printed holes stripped after 2 to 4 uses |
| `2020` | Open Frame | Heat-set inserts in the inner frame | Panels can come off again and again without wear |
| `2023` | Quick Kit 2024 | Ten M3 x 8 for the whole kit | No power tools needed |
| `2023` | Cube | One screw length everywhere | One length for the whole build |
| `2024` | Flat Hack | M4, M3 and M2.5 straight into plastic | He read that the strength of inserts is about the same |
| `2026` | Recovery Kit 3 | M4, M3, M2.5 | The same three sizes |

- **Tool warning from him:** a power drill strips M2.5 and M3 threads in plastic.
- **For us:** the Open Frame way. The frames carry the inserts, the plates stay plain. M3 for the cage, M2.5 at the Pico, nothing straight into plastic.

### 6. Wires get a place

He does not hide that wiring is the hard part. Later builds give wires slots, channels and a cavity.

![First Recovery Kit from the back: wires bundled with small zip ties along the frame](assets/doscher/rk1-wiring-loom-with-zip-ties.jpg)

*First Recovery Kit: wires grouped by route and tied to the frame.*

![Obelisk: side cavity for cables with its cover plate taken off](assets/doscher/obelisk-cable-cavity-and-cover.jpg)

*Obelisk: a cavity on one side gives cables room to bend.*

- **Detail:** right-angle plugs save depth, but they can block the port next to them.
- **For us:** wires are not modelled yet. The port bay is their cavity. A new part in the bay leaves a way for them, like the 20 mm under the blower.

### 7. Air gaps

Vents are part of the frame from the start.

- **Quick Kit:** a gap above and below the screen cools passively.
- **Obelisk:** a gap between each stacked unit, and an open top.
- **Cluster Deck:** boards stacked too close overheated, so he turned them to let air pass across.
- **For us:** the cage is open on purpose. Nothing sits in front of the projector's vents. The grill in front of its front vent is open truss bays. A blower helps the heat sink.

### 8. Build it up in steps

![Early white test print of the first Recovery Kit frame, sitting in the case](assets/doscher/rk1-white-test-print-in-case.jpg)

*First Recovery Kit: an early test print, used to find corner radius, layout and hole spacing.*

- **His method:** test one thing at a time and add detail in layers.
- **Each version removes parts.**
- **For us:** print, hold it against the real parts, review. The first prints showed that the real projector is deeper than its model, that M2 is too small, and that the buck tray is better off with a clamp bar.

### 9. The look

This is what the images show. He does not write it down as rules.

- **Open trusses:** X and triangle cut-outs in panels. They save plastic, let air through and keep the panel stiff.
- **Chamfered edges and corners**, not soft fillets.
- **Screws are visible** and evenly spaced. Socket heads, black.
- **One matte colour per build.**
- **Slot vents** in rows.
- **Rounded rectangle openings** for ports and cables.

---

## Where we differ

| Topic | Doscher | Us |
| --- | --- | --- |
| **Shell** | A frame inside a hard case, or a closed printed box | No case. The open cage is the outside |
| **Place** | Field kits and desk builds | Indoors, on a tripod or hung from a ceiling, used while it runs |
| **Heat** | The computer's own heat | The projector's heat. Its vents stay free, a blower helps |
| **Screws in plastic** | Straight into plastic since 2024 | Always into heat-set inserts |
| **Screw sizes** | M4, M3, M2.5 | M3 for the cage, M2.5 at the Pico |
| **Trusses** | X braces and triangles | One brace per bay, in a zigzag |
| **Openings** | Rounded rectangles | Octagons: rectangles with cut corners |
| **Outer edges** | Chamfered | Square. A chamfer on the bed face prints onto the brim |
| **Material** | PETG mostly | PLA, with a brim |

---

## Our rules

### Structure

1. **Two plates, three frames.** Floor and top are flat plates. The two ends and the rear are upright frames. The front is open at the lens, the rest of it has a grill. Floor, top and rear are two parts each.
2. **The frames carry the threads.** A frame is a web with a flange at the bottom and at the top. The inserts go into the flanges, the screws come from outside through the plates.
3. **Variants differ only in a cut-out.** The two end frames have the same web. The port one has a notch in its top flange and three more inserts. The two plates share outline, ports and screw lines.
4. **One adapter per device.** Pico holder, buck tray, sensor module, terminal block clamp bar, blower bracket, Wago bracket. The controls hold themselves with their own nuts.
5. **Anything that may move between top and bottom is a module.** It bolts to a module port. Both plates have the same ports.
6. **Nothing sits in front of a vent.** Boards stand upright against the rear frame or lie below the vent strip.
7. **What a hand must reach stays open:** the projector's buttons, the focus lever, the ports, the Pico's USB, the screws of the DC plug.
8. **A plug gets its full length plus room for the wire to turn.** The DC plug adapter set the 34 mm of the port bay.
9. **A board without mounting holes gets a tray with a clamp bar.** Rails lift it, a fixed lip holds one edge, a small bar with one M3 holds the other. Solder pads stay open. No snap arms, Saurabh's call on `2026-11-25`.
10. **Where a frame has no room for flanges, it gets lips on the outside.** The front grill has a 3 mm lip over each plate. The screws go through the lip into inserts in the plate, like on a strap.
11. **A new part in the port bay leaves the wires their way.** It stands on rails or legs, not on a closed shelf.
12. **A part that is added later bolts to a flange that is there.** The frame gets inserts for it, not a new shape.

### The look

Checked against the images in [assets/doscher](assets/doscher) on `2026-11-23` and on `2026-11-25`.

1. **Windows in the plates are octagons.** Rectangles with all four corners chamfered. No sharp inside corners, no round holes except for round parts.
2. **A truss window in a plate has one diagonal rib,** 6 wide. The two diagonals of a pair open toward the front.
3. **A side panel is a row of equal truss bays.** Posts 5 wide, one brace of 4 per bay, in a zigzag, a small gusset in each corner. Three bays on the end frames and three on the front grill. The bay width follows the length of the panel, everything else stays. The zigzag goes on around the corner.
4. **No window is shaped for one plug.** No half cut window, no step, and not just an open frame. Plugs and cables go through the bays.
5. **Both sides read the same from outside.** The web of one end frame is the other one turned, not mirrored.
6. **Members keep their width.** Bands 8 to 10, ribs 6 to 8 in the plates.
7. **Outer edges are square.** No chamfer through the thickness, no fillets. A corner may be cut in plan view, that prints clean.
8. **Slot vents come in rows.** A mullion sits behind the knob, so the knob has slots left and right of it. The front grill has truss bays. Its slot style is kept as a second part until both are printed.
9. **Screws show.** Cap heads stand proud, on a line 5 mm inside the edge, the same on floor and top.
10. **One colour.** Dark grey, matte.
11. **A later part hides where it can.** The leg of the blower bracket stands behind a post of the truss.

### Printed parts

1. **Every part fits the Prusa Mini bed with a brim.** No part is longer than 120. A long face is cut in two.
2. **Every part prints flat, without supports.** A frame lies on its web, the flanges stand up. So a frame can only lie on its outer face.
3. **A part that cannot lie flat prints on the face from which every feature rises.** Then nothing hangs over. The blower bracket is built that way.
4. **No chamfer on the bed face.** With a brim the layer above the chamfer prints onto the brim and has to be cut off. Chamfers go on edges that face up.
5. **Seams are staggered.** The plates are cut at one X, the rear frame at another. Each part then bridges the other's seam with screws it has anyway.
6. **A seam that nothing bridges gets a strap.** A flat bar on the outside with one M3 on each side of the seam, into inserts in the plates. A seam is held in two places, far apart: by a strap, or by a part that crosses it anyway, like the front grill.
7. **Wall thickness (start value):** 5 for plates, 4 for webs, 5 for flanges, so an M3 insert has its 4.2 of material.
8. **Clearance (start value):** 0.2 between printed parts, 0.5 to the projector, 0.3 around a board in a tray.
9. **No snap arms.** Saurabh's call on `2026-11-25`: a clamp bar with one screw instead.
10. **A peg on a clamp bar may sit in a hole of the device.** It locates the device, the screw holds it.
11. **No text or logos** unless asked.

### Fasteners

In the model since `2026-11-23`. Counts and places are in [CAGE.md](CAGE.md#fasteners).

1. **Printed parts are joined with machine screws into heat-set inserts, or with nuts and bolts.** No screws straight into printed plastic.
2. **M3 for the cage and the modules, M2.5 for the Pico.** Three lengths of M3: 8, 6, and 12 where a screw goes through a module and a plate. M2 was too small.
3. **An M3 heat-set insert gets a 4.0 hole** and at least 4.2 of material. An M2.5 insert gets 4.0 too. Start values.
4. **A clearance hole for M3 is 3.4.**
5. **No countersinks.** If one is ever needed, it keeps 3 mm of plastic to every edge.
6. **No extra screw under a module.** A module covers the plate around its port, so the screw lines skip that stretch. Where a frame needs a screw there, a port hole does both jobs.
7. **A screw must be reachable when the part above it is in.** The blower rests on rails, so a driver gets to the bracket's screws between them.
8. **Saurabh brings the models of inserts, screws and nuts in himself.** The agent cuts the holes, counts, then copies and places the models.
9. **The fastener copies live in `komorebi-box`,** in the component `Fasteners`. Every copy has its row in [cad/fasteners.json](cad/fasteners.json). [cad/place_fasteners.py](cad/place_fasteners.py) places them from it. A copy placed by hand gets its row in the same step.

### In Fusion

1. **The printed parts are in `komorebi-cage`,** a hybrid design. `komorebi-box` only brings parts together.
2. **Everything in `komorebi-cage` is in one rigid group.**
3. **Printed parts are dark grey.** RGB 81, 81, 81, the appearance `Print dark grey`.
4. **Holder parts are named** `<device> holder: <part>`. Cage parts are named `Cage: <part>`.
5. **Change the cage in its own tab only,** save it, then get the latest version in `komorebi-box` and save that. See [Fusion traps](CAGE.md#fusion-traps).
6. **The numbers live in [CAGE.md](CAGE.md#numbers).** Write a number back there when a face moves. [cad/build_cage.py](cad/build_cage.py) is behind the cage since `2026-11-25`, do not run it as it is.
7. **Where a device model is not exact, do not trust it.** The YG300 model is such a case, see [Numbers](CAGE.md#numbers). Its plugs are placed lump sum from a photo of the real projector.
8. **After every modelling step, look at it from several sides** before reporting.

---

## Open questions

- [ ] **PLA next to the projector's heat.** PETG is the fallback, see [Open points](CAGE.md#open-points).
- [ ] **Tie points for the wires.** None yet. They come with the wiring.
- [ ] **A second reference.** This language has one so far.

---

## Log

### 2026-11-23

- First version. Cage rules written after the layout talk with Saurabh.
- After his first review: the rules under [The look](#the-look), the plug rule, the tray rule, the fastener rules.
- After his second review: PLA with a brim, no chamfer on the bed face, split faces with staggered seams and straps. See [Printed parts](#printed-parts).
- After his third review: the front grill with lips on the outside, the place of the fastener copies.

### 2026-11-25

- After his fourth review, with the first prints in hand: clamp bar instead of snap arms, M2.5 instead of M2, side panels as truss bays, no window for one plug, a later part bolts to a flange that is there and leaves the wires their way.
- This file stands on its own now: its own reference section, its own image folder [assets/doscher](assets/doscher), its own sources. 32 images, kept out of git.

---

## Sources

All by Jay Doscher on doscher.com. Read on `2026-11-15`.

| Build | Year | Post |
| --- | --- | --- |
| Raspberry Pi Recovery Kit | 2019 | [work-recovery-kit](https://www.doscher.com/work-recovery-kit/) |
| Recovery Kit Retrospective | 2023 | [recovery-kit-retrospective](https://www.doscher.com/recovery-kit-retrospective/) |
| ARM Terminal | 2020 | [the-a-r-m-terminal](https://www.doscher.com/the-a-r-m-terminal/) |
| Open Frame computer | 2020 | [work-open-frame-computer](https://www.doscher.com/work-open-frame-computer/) |
| Mil-Plastic deck | 2020 | [the-mil-plastic-deck](https://www.doscher.com/the-mil-plastic-deck/) |
| Raspberry Pi Quick Kit | 2020 | [the-raspberry-pi-quick-kit](https://www.doscher.com/the-raspberry-pi-quick-kit/) |
| Cluster Deck | 2020 | [the-raspberry-pi-cluster-deck](https://www.doscher.com/the-raspberry-pi-cluster-deck/) |
| Kuiper Deck | 2021 | [work-kuiper-deck](https://www.doscher.com/work-kuiper-deck/) |
| Quick Kit 2024 editions | 2023 | [quick-kit-2024-editions](https://www.doscher.com/quick-kit-2024-editions/) |
| ARM Terminal 2, the Cube | 2023 | [arm-terminal-2-the-cube](https://www.doscher.com/arm-terminal-2-the-cube/) |
| ARM Terminal v4, Flat Hack | 2024 | [arm-terminal-flat-hack](https://www.doscher.com/arm-terminal-flat-hack/) |
| N100 Obelisk | 2024 | [two-spools-of-filament-the-intel-100-obelisk](https://www.doscher.com/two-spools-of-filament-the-intel-100-obelisk/) |
| Meshtastic enclosure | 2024 | [building-for-simplicity-my-take-on-a-meshtastic-enclosure](https://www.doscher.com/building-for-simplicity-my-take-on-a-meshtastic-enclosure/) |
| Compute Unit | 2026 | [the-compute-unit](https://www.doscher.com/the-compute-unit/) |
| Recovery Kit 3 | 2026 | [the-recovery-kit-3](https://www.doscher.com/the-recovery-kit-3/) |

Other:

- [Recovery Kit 2 and 2B frame enclosure, print guidelines](https://raw.githubusercontent.com/jdoscher/Recovery-Kit-2-and-2B-Open-Source-Frame-Enclosure/main/License%20and%20Print%20Guidelines.txt) (GitHub)
- Later the same day: the Wago bracket, the second part that prints on its wall face. The slicer step for such a part is written down in [CAGE.md](CAGE.md#print-again).
- The front grill went from slot rows to truss bays. One pattern for the sides and the front now.
- The slot style of the front grill is kept as a second part. Saurabh prints both and picks one.
