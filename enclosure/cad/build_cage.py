"""Build the printed parts of the komorebi cage in the Fusion file `komorebi-cage`.

Run it with `komorebi-cage` as the active tab. It builds every part again from the
numbers in P and swaps the body inside each component, so the links in
`komorebi-box` stay valid. All numbers are mm, in the coordinates of `komorebi-box`
(projector where it stands there, Y up, lens face +Z, ports on the -X end).

Loader for the Fusion script runner:

    def run(_context):
        p = '/Users/saurabhdatta/Documents/Projects/pi_pico_komorebi/enclosure/cad/build_cage.py'
        g = {}
        exec(compile(open(p).read(), p, 'exec'), g)
        g['main']()
"""
import math

import adsk.core
import adsk.fusion

P = {
    # projector, as measured in komorebi-box
    'proj_x0': -86.1, 'proj_x1': 39.9, 'proj_z0': -43.8, 'proj_z1': 36.2,
    'proj_tripod_x': -32.3, 'proj_tripod_z': 20.3,
    # cage envelope
    'half_inner_x': 80.0,      # inner half width, centred on the projector
    'z_rear': -80.0,           # inner face of the rear frame
    'z_front': 40.0,           # front edge of the plates
    'web': 4.0,                # thickness of the upright frames
    'plate_t': 5.0,
    'floor_top': 1.8,          # the projector feet stand here
    'top_bottom': 52.8,        # 2 above the projector top
    'flange_w': 10.0, 'flange_t': 5.0,
    'gap': 0.2,                # between printed parts
    'proj_clear': 0.5,         # lips to projector
    'lip_h': 7.0,
    # module ports, same on the floor and on the top
    'port_tripod_z': -8.0, 'port_sensor_z': -58.0,
    'port_pitch': 31.82,       # square of the four M3 holes, the magic arm adapter turned 45 degrees
    'port_open_d': 24.0, 'insert_d': 4.0, 'm3_clear_d': 3.4,
    # controls on the rear frame
    'ctrl_y': 27.3, 'jack_x': -91.0, 'button_x': -75.0, 'encoder_x': -45.0,
    'jack_d': 11.2, 'button_d': 7.2, 'encoder_d': 7.2,
    'ctrl_panel_t': 3.0,       # panel left behind the jack and the encoder, their models are drawn for 3.2
    # pico group, see CAGE.md
    'pico_holes': [(49.7, 21.6, -72.1), (49.7, 33.0, -72.1), (-2.3, 18.4, -73.8), (-2.3, 36.2, -73.8)],
    # buck converter footprint on the floor
    'buck_x0': -90.6, 'buck_x1': -68.2, 'buck_z0': -67.5, 'buck_z1': -50.4,
    'colour': (81, 81, 81),
}

AX = {
    # normal axis: (a axis, b axis, n axis) as indices into x, y, z
    'y': (0, 2, 1),
    'x': (2, 1, 0),
    'z': (0, 1, 2),
}

_tm = None


def tm():
    global _tm
    if _tm is None:
        _tm = adsk.fusion.TemporaryBRepManager.get()
    return _tm


def _w(axis, a, b, n):
    ia, ib, i_n = AX[axis]
    v = [0.0, 0.0, 0.0]
    v[ia], v[ib], v[i_n] = a, b, n
    return v


def _pt(axis, a, b, n):
    v = _w(axis, a, b, n)
    return adsk.core.Point3D.create(v[0] / 10.0, v[1] / 10.0, v[2] / 10.0)


def _vec(axis, a, b, n):
    v = _w(axis, a, b, n)
    return adsk.core.Vector3D.create(v[0], v[1], v[2])


def rot_rect(axis, ca, cb, ang_deg, length, width, n0, n1):
    """Box with its length at ang_deg in the a, b plane, centred on ca, cb."""
    t = math.radians(ang_deg)
    ld = _vec(axis, math.cos(t), math.sin(t), 0)
    wd = _vec(axis, -math.sin(t), math.cos(t), 0)
    obb = adsk.core.OrientedBoundingBox3D.create(
        _pt(axis, ca, cb, (n0 + n1) / 2.0), ld, wd, length / 10.0, width / 10.0, abs(n1 - n0) / 10.0)
    return tm().createBox(obb)


def rect(axis, a0, a1, b0, b1, n0, n1):
    return rot_rect(axis, (a0 + a1) / 2.0, (b0 + b1) / 2.0, 0, abs(a1 - a0), abs(b1 - b0), n0, n1)


def octa(axis, a0, a1, b0, b1, n0, n1, c):
    """Rectangle with all four corners chamfered by c."""
    body = rect(axis, a0, a1, b0, b1, n0, n1)
    half = (abs(a1 - a0) / 2.0 + abs(b1 - b0) / 2.0 - c) / math.sqrt(2.0)
    clip = rot_rect(axis, (a0 + a1) / 2.0, (b0 + b1) / 2.0, 45, 2 * half, 2 * half, n0, n1)
    tm().booleanOperation(body, clip, adsk.fusion.BooleanTypes.IntersectionBooleanType)
    return body


def cyl(axis, a, b, r, n0, n1):
    return tm().createCylinderOrCone(_pt(axis, a, b, n0), r / 10.0, _pt(axis, a, b, n1), r / 10.0)


def cut(target, tool):
    tm().booleanOperation(target, tool, adsk.fusion.BooleanTypes.DifferenceBooleanType)


def join(target, tool):
    tm().booleanOperation(target, tool, adsk.fusion.BooleanTypes.UnionBooleanType)


def derived():
    d = dict(P)
    d['xc'] = (P['proj_x0'] + P['proj_x1']) / 2.0
    d['xil'] = d['xc'] - P['half_inner_x']
    d['xir'] = d['xc'] + P['half_inner_x']
    d['xol'] = d['xil'] - P['web']
    d['xor'] = d['xir'] + P['web']
    d['zro'] = P['z_rear'] - P['web']
    d['yb'] = P['floor_top'] - P['plate_t']
    d['yt'] = P['top_bottom'] + P['plate_t']
    d['proj_half'] = (P['proj_x1'] - P['proj_x0']) / 2.0
    return d


def cut_ports(body, d, y0, y1, hole_d):
    h = d['port_pitch'] / 2.0
    for z in (d['port_tripod_z'], d['port_sensor_z']):
        cut(body, cyl('y', d['xc'], z, d['port_open_d'] / 2.0, y0 - 1, y1 + 1))
        for sx in (-1, 1):
            for sz in (-1, 1):
                cut(body, cyl('y', d['xc'] + sx * h, z + sz * h, hole_d / 2.0, y0 - 1, y1 + 1))


def floor_plate(d):
    g, xc = d['gap'], d['xc']
    y0, y1 = d['yb'], d['floor_top']
    b = rect('y', d['xil'] + g, d['xir'] - g, d['z_rear'] + g, d['z_front'], y0, y1)
    cut_ports(b, d, y0, y1, d['insert_d'])
    # obround for a 1/4 inch screw into the projector's own tripod thread
    px, pz = d['proj_tripod_x'], d['proj_tripod_z']
    slot = rect('y', px - 4, px + 4, pz - 3.5, pz + 3.5, y0 - 1, y1 + 1)
    join(slot, cyl('y', px - 4, pz, 3.5, y0 - 1, y1 + 1))
    join(slot, cyl('y', px + 4, pz, 3.5, y0 - 1, y1 + 1))
    cut(b, slot)
    # window under the projector's bottom vent slot
    cut(b, octa('y', xc - 27, xc + 27, -37.0, -29.5, y0 - 1, y1 + 1, 2))
    c = d['proj_clear']
    ph = d['proj_half']
    lip_top = y1 + d['lip_h']
    for s in (-1, 1):
        a0, a1 = sorted((xc + s * 25, xc + s * 62))
        cut(b, octa('y', a0, a1, -18.0, 10.0, y0 - 1, y1 + 1, 5))
        # corner lips, front and rear, each with a return along the end face
        u0, u1 = sorted((xc + s * (ph - 13.5), xc + s * (ph + c + 3)))
        r0, r1 = sorted((xc + s * (ph + c), xc + s * (ph + c + 3)))
        zf = d['proj_z1'] + c
        zr = d['proj_z0'] - c
        join(b, rect('y', u0, u1, zf, d['z_front'], y1 - 0.5, lip_top))
        join(b, rect('y', r0, r1, zf - 10.5, d['z_front'], y1 - 0.5, lip_top))
        join(b, rect('y', u0, u1, zr - 3, zr, y1 - 0.5, lip_top))
        join(b, rect('y', r0, r1, zr - 3, zr + 14, y1 - 0.5, lip_top))
    return b


def top_plate(d):
    g, xc = d['gap'], d['xc']
    y0, y1 = d['top_bottom'], d['yt']
    b = rect('y', d['xil'] + g, d['xir'] - g, d['z_rear'] + g, d['z_front'], y0, y1)
    cut_ports(b, d, y0, y1, d['insert_d'])
    for s in (-1, 1):
        a0, a1 = sorted((xc + s * 21, xc + s * 70))
        # front window, over the projector buttons
        cut(b, octa('y', a0, a1, -22.0, 32.0, y0 - 1, y1 + 1, 6))
        # rear window with one diagonal
        w = octa('y', a0, a1, -70.0, -30.0, y0 - 1, y1 + 1, 4)
        ang = math.degrees(math.atan2(40.0, s * 49.0))
        cut(w, rot_rect('y', (a0 + a1) / 2.0, -50.0, ang, 90, 6, y0 - 2, y1 + 2))
        cut(b, w)
    return b


def end_frame(d):
    """The frame on the -X end. The +X end is the same part turned half round."""
    x0, x1 = d['xol'], d['xil']
    b = rect('x', d['z_rear'], d['z_front'], d['yb'], d['yt'], x0, x1)
    cut(b, octa('x', -74.0, -50.0, 8.8, 45.8, x0 - 1, x1 + 1, 5))
    cut(b, octa('x', -42.0, 32.0, 8.8, 45.8, x0 - 1, x1 + 1, 6))
    ft, fw = d['flange_t'], d['flange_w']
    join(b, rect('x', d['z_rear'], d['z_front'], d['floor_top'], d['floor_top'] + ft, x1 - 0.5, x1 + fw))
    join(b, rect('x', d['z_rear'], d['z_front'], d['top_bottom'] - ft, d['top_bottom'], x1 - 0.5, x1 + fw))
    return b


def rear_frame(d):
    z0, z1 = d['zro'], d['z_rear']
    b = rect('z', d['xol'], d['xor'], d['yb'], d['yt'], z0, z1)
    # two rows of slot vents
    span0, span1 = d['xil'] + 4, d['xir'] - 4
    mull = 6.0
    seg = (span1 - span0 - 2 * mull) / 3.0
    for (y0, y1) in ((8.0, 14.5), (40.1, 46.6)):
        for i in range(3):
            a0 = span0 + i * (seg + mull)
            cut(b, octa('z', a0, a0 + seg, y0, y1, z0 - 1, z1 + 1, 2))
    cy = d['ctrl_y']
    # the nuts of the jack and the encoder need a thinner panel: thin the field on the inside
    cut(b, rect('z', d['jack_x'] - 11.5, d['encoder_x'] + 7.5, cy - 11.3, cy + 11.3, z0 + d['ctrl_panel_t'], z1 + 1))
    cut(b, cyl('z', d['jack_x'], cy, d['jack_d'] / 2.0, z0 - 1, z1 + 1))
    cut(b, cyl('z', d['button_x'], cy, d['button_d'] / 2.0, z0 - 1, z1 + 1))
    # the button thread is short: 2 of panel left, recess on the inside
    cut(b, cyl('z', d['button_x'], cy, 6.25, z0 + 2, z1 + 1))
    cut(b, cyl('z', d['encoder_x'], cy, d['encoder_d'] / 2.0, z0 - 1, z1 + 1))
    ft, fw, g = d['flange_t'], d['flange_w'], d['gap']
    fx0, fx1 = d['xil'] + fw + g, d['xir'] - fw - g
    join(b, rect('z', fx0, fx1, d['floor_top'], d['floor_top'] + ft, z1 - 0.5, z1 + fw))
    join(b, rect('z', fx0, fx1, d['top_bottom'] - ft, d['top_bottom'], z1 - 0.5, z1 + fw))
    return b


def pico_holder(d):
    z0 = d['z_rear']
    b = rect('z', -8.0, 53.5, 15.3, 39.3, z0, z0 + 3)
    cut(b, octa('z', 5.0, 41.0, 20.8, 33.8, z0 - 1, z0 + 4, 3))
    for (x, y, ztop) in d['pico_holes']:
        join(b, cyl('z', x, y, 3.0, z0 + 2.5, ztop))
    for (x, y, ztop) in d['pico_holes']:
        cut(b, cyl('z', x, y, 0.9, z0 - 1, ztop + 1))
    return b


def buck_tray(d):
    y0 = d['floor_top']
    x0, x1 = d['buck_x0'] - 2.0, d['buck_x1'] + 2.0
    z0, z1 = d['buck_z0'] - 2.0, d['buck_z1'] + 2.0
    b = rect('y', x0, x1, z0, z1, y0, y0 + 2)
    rim = rect('y', x0, x1, z0, z1, y0 + 1.5, y0 + 4.8)
    cut(rim, rect('y', d['buck_x0'] - 0.3, d['buck_x1'] + 0.3, d['buck_z0'] - 0.3, d['buck_z1'] + 0.3, y0, y0 + 6))
    corners = None
    for cx in (x0, x1):
        for cz in (z0, z1):
            k = rect('y', cx - 7, cx + 7, cz - 7, cz + 7, y0, y0 + 6)
            if corners is None:
                corners = k
            else:
                join(corners, k)
    tm().booleanOperation(rim, corners, adsk.fusion.BooleanTypes.IntersectionBooleanType)
    join(b, rim)
    return b


def sensor_module(d):
    """Plate that carries the BH1750 dome. Fits either port, on the floor or on the top."""
    xc, z = d['xc'], d['port_sensor_z']
    y1 = d['yb']          # against the outside of the floor plate
    y0 = y1 - 4.0
    b = octa('y', xc - 22, xc + 22, z - 22, z + 22, y0, y1, 5)
    cut(b, cyl('y', xc, z, 13.0, y0 - 1, y1 + 1))
    cut(b, cyl('y', xc, z, 14.3, y1 - 2.2, y1 + 1))
    h = d['port_pitch'] / 2.0
    for sx in (-1, 1):
        for sz in (-1, 1):
            cut(b, cyl('y', xc + sx * h, z + sz * h, d['m3_clear_d'] / 2.0, y0 - 1, y1 + 1))
    return b


def _appearance(app, des, rgb):
    name = 'Print dark grey'
    ap = des.appearances.itemByName(name)
    if ap is None:
        src = None
        for lib in app.materialLibraries:
            if 'Appearance' not in lib.name:
                continue
            for a in lib.appearances:
                if a.name == 'Plastic - Matte (Gray)':
                    src = a
                    break
            if src:
                break
        ap = des.appearances.addByCopy(src, name)
        prop = adsk.core.ColorProperty.cast(ap.appearanceProperties.itemByName('Color'))
        prop.value = adsk.core.Color.create(rgb[0], rgb[1], rgb[2], 255)
    return ap


def put(root, name, temp_body, ap):
    """Create the component, or swap the body inside the one that is there."""
    occ = None
    for o in root.occurrences:
        if o.component.name == name:
            occ = o
            break
    if occ is None:
        occ = root.occurrences.addNewComponent(adsk.core.Matrix3D.create())
        occ.component.name = name
    comp = occ.component
    bfs = comp.features.baseFeatures
    bf = bfs.item(0) if bfs.count else bfs.add()
    bf.startEdit()
    for i in range(bf.bodies.count - 1, -1, -1):
        bf.bodies.item(i).deleteMe()
    comp.bRepBodies.add(temp_body, bf)
    bf.finishEdit()
    body = comp.bRepBodies.item(0)
    body.name = name.split(': ')[-1]
    body.appearance = ap
    return occ


def main():
    app = adsk.core.Application.get()
    doc = app.activeDocument
    if not doc.name.startswith('komorebi-cage'):
        raise RuntimeError('active document is %s, not komorebi-cage' % doc.name)
    des = adsk.fusion.Design.cast(app.activeProduct)
    root = des.rootComponent
    d = derived()
    ap = _appearance(app, des, d['colour'])
    parts = [
        ('Cage: floor plate', floor_plate),
        ('Cage: top plate', top_plate),
        ('Cage: end frame', end_frame),
        ('Cage: rear frame', rear_frame),
        ('Pico holder: base', pico_holder),
        ('Buck converter holder: tray', buck_tray),
        ('Light sensor module: plate', sensor_module),
    ]
    for name, fn in parts:
        occ = put(root, name, fn(d), ap)
        bb = occ.boundingBox
        print('%s  %.1f x %.1f x %.1f' % (
            name, (bb.maxPoint.x - bb.minPoint.x) * 10, (bb.maxPoint.y - bb.minPoint.y) * 10,
            (bb.maxPoint.z - bb.minPoint.z) * 10))
    # second end frame: the same component, turned half round about the cage's long axis
    ends = [o for o in root.occurrences if o.component.name == 'Cage: end frame']
    if len(ends) < 2:
        m = adsk.core.Matrix3D.create()
        m.setToRotation(math.pi, adsk.core.Vector3D.create(0, 0, 1),
                        adsk.core.Point3D.create(d['xc'] / 10.0, (d['yb'] + d['yt']) / 20.0, 0))
        root.occurrences.addExistingComponent(ends[0].component, m)
    if root.rigidGroups.count == 0:
        coll = adsk.core.ObjectCollection.create()
        for o in root.occurrences:
            coll.add(o)
        root.rigidGroups.add(coll, True)
    print('occurrences', root.occurrences.count, 'rigid groups', root.rigidGroups.count)
