"""Place the fastener models in the Fusion file `komorebi-box`, from fasteners.json.

Run it with the tab of `komorebi-box` active, after build_cage.py has run and the
link to `komorebi-cage` is on its latest version.

One model of each kind has to be in `komorebi-box` already, as a component with the
name in MODELS. The script keeps all copies in one component, `Fasteners`. On the
first run it moves the sample models in there. On every run it moves the copies it
finds to their places, adds the missing ones and deletes the ones too many. So it
can run again after every change of the cage.

Kinds without a model are left out and listed at the end.

Loader for the Fusion script runner:

    def run(_context):
        p = '/Users/saurabhdatta/Documents/Projects/pi_pico_komorebi/enclosure/cad/place_fasteners.py'
        g = {'__file__': p}
        exec(compile(open(p).read(), p, 'exec'), g)
        g['main']()
"""
import json
import os

import adsk.core
import adsk.fusion

GROUP = 'Fasteners'

# kind in fasteners.json: (component name in komorebi-box, axis of the model, length under the head)
# A screw model has its head on the + end of its axis. Its seat is the underside of the head.
# An insert model has its wide end on the + end of its axis. Its seat is that end face,
# flush with the mouth of the hole. Length None means insert.
MODELS = {
    'M3 insert': ('M3 heat-set insert, 4.0 hole', 'y', None),
    'M2 insert': ('M2 heat-set insert, 3.2 hole', 'y', None),
    'M3 x 8': ('M3 x 8 cap head', 'z', 8.0),
    'M3 x 12': ('M3 x 12 cap head', 'z', 12.0),
    'M3 x 6': ('M3 x 6 cap head', 'z', 6.0),
    'M2 x 5': ('M2 x 5 cap head', 'z', 5.0),
}

AXIS = {'x': 0, 'y': 1, 'z': 2}


def local_seat(comp, axis, length):
    """Seat point of the model in its own coordinates (cm), and the direction it goes in."""
    bb = comp.bRepBodies.item(0).boundingBox
    lo, hi = bb.minPoint.asArray(), bb.maxPoint.asArray()
    i = AXIS[axis]
    seat = [(lo[k] + hi[k]) / 2.0 for k in range(3)]
    seat[i] = hi[i] if length is None else lo[i] + length / 10.0
    into = [0.0, 0.0, 0.0]
    into[i] = -1.0
    return adsk.core.Point3D.create(*seat), adsk.core.Vector3D.create(*into)


def placement(seat_l, into_l, seat_mm, into_w):
    """Matrix that turns the model's direction onto into_w and puts its seat on seat_mm."""
    m = adsk.core.Matrix3D.create()
    to = adsk.core.Vector3D.create(*into_w)
    to.normalize()
    if into_l.isEqualTo(to):
        pass
    else:
        # for a half turn the rotation needs an axis: any one square to the model's direction
        perp = adsk.core.Vector3D.create(1, 0, 0)
        if abs(into_l.x) > 0.9:
            perp = adsk.core.Vector3D.create(0, 1, 0)
        m.setToRotateTo(into_l, to, perp)
    p = seat_l.copy()
    p.transformBy(m)
    m.translation = adsk.core.Vector3D.create(seat_mm[0] / 10.0 - p.x, seat_mm[1] / 10.0 - p.y, seat_mm[2] / 10.0 - p.z)
    return m


def main():
    app = adsk.core.Application.get()
    doc = app.activeDocument
    if not doc.name.startswith('komorebi-box'):
        raise RuntimeError('active document is %s, not komorebi-box' % doc.name)
    des = adsk.fusion.Design.cast(app.activeProduct)
    root = des.rootComponent
    with open(os.path.join(os.path.dirname(__file__), 'fasteners.json')) as fh:
        rows = json.load(fh)['fasteners']

    group_occ = None
    for o in root.occurrences:
        if o.component.name == GROUP:
            group_occ = o
            break
    if group_occ is None:
        group_occ = root.occurrences.addNewComponent(adsk.core.Matrix3D.create())
        group_occ.component.name = GROUP
    group = group_occ.component

    by_kind = {}
    for r in rows:
        by_kind.setdefault(r['kind'], []).append(r)

    # first the structure: one occurrence per row, all inside the group
    placed, left_out, todo = {}, {}, []
    for kind, items in by_kind.items():
        name = MODELS[kind][0] if kind in MODELS else None
        comp = None
        for c in des.allComponents:
            if c.name == name:
                comp = c
                break
        if comp is None:
            left_out[kind] = len(items)
            continue
        # the sample models sit in the root on the first run: move them into the group
        for o in list(root.occurrences):
            if o.component == comp:
                o.moveToComponent(group_occ)
        have = [o for o in group.occurrences if o.component == comp]
        for _ in range(len(items) - len(have)):
            group.occurrences.addExistingComponent(comp, adsk.core.Matrix3D.create())
        for o in have[len(items):]:
            o.deleteMe()
        todo.append((kind, comp, items))
        placed[kind] = len(items)
    # then the positions, all in one go. A position can only be set on the occurrence
    # as seen from the root, and a new feature in between would throw the moves away
    for kind, comp, items in todo:
        _, axis, length = MODELS[kind]
        seat_l, into_l = local_seat(comp, axis, length)
        have = [o for o in group_occ.childOccurrences if o.component == comp]
        for o, r in zip(have, items):
            o.transform2 = placement(seat_l, into_l, r['seat'], r['into'])
    if des.snapshots.hasPendingSnapshot:
        des.snapshots.add()
    print('placed', placed, 'total', sum(placed.values()))
    print('no model', left_out)
