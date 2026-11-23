# Enclosure design language

This is the design language for the komorebi box cage.

It is written for agents and people who will do CAD on it. Read it before you model anything.

It is not a new language. It is the one from the solar server enclosure, carried over and cut down to what an open desk cage needs.

> [!Note]
> **The full text and Jay Doscher's reference images are in the private repo `orangepizero-solar-server`,** file `enclosure/DESIGN_LANGUAGE.md`, folder `enclosure/assets/doscher`. The images are not copied here. They are his, and they must not be published.

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
| **Printer** | Prusa Mini, design to 140 | Prusa Mini, 180 x 180 bed, design to 170 |

---

## Our rules

### Structure

1. **Two plates, three frames.** Floor and top are flat plates. The two ends and the rear are upright frames. The front is open.
2. **The frames carry the threads.** A frame is a web with a flange at the bottom and at the top. The inserts go into the flanges, the screws come from outside through the plates.
3. **Repeat parts.** The two end frames are one part. The two plates share outline and ports.
4. **One adapter per device.** Pico holder, buck tray, sensor module. The controls hold themselves with their own nuts.
5. **Anything that may move between top and bottom is a module.** It bolts to a module port. Both plates have the same ports.
6. **Nothing sits in front of a vent.** Boards stand upright against the rear frame or lie below the vent strip.
7. **What a hand must reach stays open:** the projector's buttons, the focus lever, the ports, the Pico's USB.

### Printed parts

1. **Every part fits the Prusa Mini bed.** Design to 170 x 170.
2. **Every part prints flat, without supports.** A frame lies on its web, the flanges stand up.
3. **Corners of windows are chamfered.**
4. **Wall thickness (start value):** 5 for plates, 4 for webs, 5 for flanges, so an M3 insert has its 4.2 of material.
5. **Clearance (start value):** 0.2 between printed parts, 0.5 to the projector.
6. **No text or logos** unless asked.

### Fasteners

Not in the model yet. When they come:

1. **Printed parts are joined with machine screws into heat-set inserts, or with nuts and bolts.** No screws straight into printed plastic.
2. **M3 for the cage and the modules, M2 for the Pico.**
3. **An M3 heat-set insert gets a 4.0 hole** and at least 4.2 of material. Start value.
4. **A countersink keeps 3 mm of plastic to every edge.**
5. **Saurabh brings the models of inserts, screws and nuts in himself.** The agent names the places.

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
