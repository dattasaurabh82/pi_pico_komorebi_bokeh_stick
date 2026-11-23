# Enclosure design language

This is the design language for the komorebi box cage.

It is written for agents and people who will do CAD on it. Read it before you model anything.

It is not a new language. It is the one from the solar server enclosure, carried over and cut down to what an open desk cage needs.

> [!Note]
> **The full text and Jay Doscher's reference images are in the private repo `orangepizero-solar-server`,** file `enclosure/DESIGN_LANGUAGE.md`, folder `enclosure/assets/doscher`. The images are not copied here. They are his, and they must not be published. Look at them before you change how a part looks.

---

## The language in short

- Every device gets **its own adapter**. A new device means a new adapter, nothing else.
- Parts are **small, flat and repeated**. They are bolted together, not printed as one big piece.
- Everything **prints flat without supports**. Chamfers, not fillets, under overhangs.
- **Few screw sizes.** Screws are visible and reachable from outside.
- **Air gaps are designed**, not left over.
- The look is **plain and honest**: open windows and trusses, chamfered corners, one dark matte colour, visible screws.

---

## Where this cage differs from the solar server

| Topic | Solar server | Komorebi box |
| --- | --- | --- |
| **Shell** | Cartridge inside a Peli case | No case. The cage is the outside |
| **Place** | Outdoors, closed, unattended | Indoors, open, on a tripod or hung from a ceiling |
| **Heat** | Sun on a closed case | The projector's own heat. Its vents must stay free |
| **Room** | Roomy is better than tight | Enough room, not too much |
| **Hands** | Serviced with the lid open | Knob, button, focus lever and the projector's buttons are used while it runs |
| **Backbone** | Floor tiles with a 20 mm grid | Two plates and three frames. No grid yet |
| **Printer** | Prusa Mini, design to 140 | Prusa Mini, 180 x 180 bed. The plates are at 170, the rear frame goes on at 45 degrees |

---

## Our rules

### Structure

1. **Two plates, three frames.** Floor and top are flat plates. The two ends and the rear are upright frames. The front is open.
2. **The frames carry the threads.** A frame is a web with a flange at the bottom and at the top. The inserts go into the flanges, the screws come from outside through the plates.
3. **Variants differ only in a cut-out.** The two end frames are the same frame. The port one has a shorter top flange and a stepped window. The two plates share outline, ports and screw lines.
4. **One adapter per device.** Pico holder, buck tray, sensor module. The controls hold themselves with their own nuts.
5. **Anything that may move between top and bottom is a module.** It bolts to a module port. Both plates have the same ports.
6. **Nothing sits in front of a vent.** Boards stand upright against the rear frame or lie below the vent strip.
7. **What a hand must reach stays open:** the projector's buttons, the focus lever, the ports, the Pico's USB, the screws of the DC plug.
8. **A plug gets its full length plus room for the wire to turn.** The DC plug adapter set the 34 mm of the port bay.
9. **A board without mounting holes gets a snap tray.** Corner posts locate it, rails lift it, two arms hook over free edges. Solder pads stay open.

### The look

Checked against the Doscher images on `2026-11-23`.

1. **Windows are octagons.** Rectangles with all four corners chamfered. No sharp inside corners, no round holes except for round parts.
2. **A truss window has one diagonal rib,** 6 wide. The two diagonals of a pair open toward the front.
3. **Members keep their width.** Bands 8 to 10, ribs 6 to 8.
4. **Outer edges are chamfered 1.5,** the two rear corners 3. No fillets.
5. **Slot vents come in rows.** A mullion sits behind the knob, so the knob has slots left and right of it.
6. **Screws show.** Cap heads stand proud, on a line 5 mm inside the edge, the same on floor and top.
7. **One colour.** Dark grey, matte.

### Printed parts

1. **Every part fits the Prusa Mini bed.** Straight if it is under 172, at 45 degrees if it is long and narrow.
2. **Every part prints flat, without supports.** A frame lies on its web, the flanges stand up.
3. **Wall thickness (start value):** 5 for plates, 4 for webs, 5 for flanges, so an M3 insert has its 4.2 of material.
4. **Clearance (start value):** 0.2 between printed parts, 0.5 to the projector, 0.3 around a board in a tray.
5. **A snap arm is at least 6 long and 1 thick, with a hook of 0.6.** Shorter arms break.
6. **No text or logos** unless asked.

### Fasteners

In the model since `2026-11-23`. Counts and places are in [CAGE.md](CAGE.md#fasteners).

1. **Printed parts are joined with machine screws into heat-set inserts, or with nuts and bolts.** No screws straight into printed plastic.
2. **M3 for the cage and the modules, M2 for the Pico.** Two lengths of M3: 8 and 6.
3. **An M3 heat-set insert gets a 4.0 hole** and at least 4.2 of material. An M2 insert gets 3.2. Start values.
4. **A clearance hole for M3 is 3.4.**
5. **No countersinks.** If one is ever needed, it keeps 3 mm of plastic to every edge.
6. **No screw under a module.** A module covers the plate around its port, so the screw lines skip that stretch.
7. **Saurabh brings the models of inserts, screws and nuts in himself.** The agent cuts the holes, counts, then copies and places the models.

### In Fusion

1. **The printed parts are in `komorebi-cage`,** a hybrid design. `komorebi-box` only brings parts together.
2. **Everything in `komorebi-cage` is in one rigid group.**
3. **Printed parts are dark grey.** RGB 81, 81, 81, the appearance `Print dark grey`.
4. **Holder parts are named** `<device> holder: <part>`. Cage parts are named `Cage: <part>`.
5. **Change the cage in its own tab only,** save it, then get the latest version in `komorebi-box` and save that. See [Fusion traps](CAGE.md#fusion-traps).
6. **The numbers live in [cad/build_cage.py](cad/build_cage.py).** Change the `P` table and run the script again. Do not push faces by hand without writing the number back.
7. **Where a device model is not exact, do not trust it.** The YG300 model is such a case, see [Numbers](CAGE.md#numbers).

---

## Log

### 2026-11-23

- First version, taken over from the solar server enclosure. Cage rules written after the layout talk with Saurabh.
- After his first review: the rules under [The look](#the-look), the plug rule, the snap tray rule, the fastener rules.
