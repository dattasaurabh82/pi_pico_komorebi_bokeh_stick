"""Build the printed parts of the komorebi cage in the Fusion file `komorebi-cage`.

Run it with the cage's own tab active. It builds every part again from the
numbers in P and swaps the body inside each component, so the links in
`komorebi-box` stay valid. All numbers are mm, in the coordinates of `komorebi-box`
(projector where it stands there, Y up, lens face +Z, ports on the -X end).

It also writes fasteners.json next to this file: one row per insert and screw,
with its seat point and the direction it goes in.

Loader for the Fusion script runner:

    def run(_context):
        p = '/Users/saurabhdatta/Documents/Projects/pi_pico_komorebi/enclosure/cad/build_cage.py'
        g = {'__file__': p}
        exec(compile(open(p).read(), p, 'exec'), g)
        g['main']()
"""
import json
import math
import os

import adsk.core
import adsk.fusion

P = {
    # ---- projector, as measured in komorebi-box
    'proj_x0': -86.1, 'proj_x1': 39.9, 'proj_z0': -43.8, 'proj_z1': 36.2,
    'proj_tripod_x': -32.3, 'proj_tripod_z': 20.3,
    # ---- cage envelope
    'gap_port': 34.0,          # projector to the -X frame: HDMI plug, DC plug adapter, folded ribbon
    'gap_lever': 10.5,         # projector to the +X frame: the flange there is the end stop
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
    # ---- seams: the plates and the rear frame are each two parts, the seams are staggered
    'plate_seam_x': -60.0,     # between the projector's -X feet and the module ports
    'rear_seam_x': -23.1,      # on the projector centre, between the knob and the pico
    'strap_dx': 5.5,           # strap screws left and right of the plate seam
    'strap_z_floor': [35.5, -4.0],
    'strap_z_top': [35.5, -26.0],
    # ---- module ports, same on the floor and on the top
    'port_tripod_z': -8.0, 'port_sensor_z': -58.0,
    'port_pitch': 31.82,       # square of the four M3 holes, the magic arm adapter turned 45 degrees
    'port_open_d': 24.0,
    # ---- controls on the rear frame
    'ctrl_y': 27.3, 'jack_x': -98.0, 'button_x': -76.0, 'encoder_x': -50.0,
    'jack_d': 11.2, 'button_d': 7.2, 'encoder_d': 7.2,
    'ctrl_panel_t': 3.0,       # panel left behind the jack and the encoder, their models are drawn for 3.2
    # ---- pico group: its four mounting holes as x, y, z of the board underside
    'pico_holes': [(43.2, 21.6, -72.1), (43.2, 33.0, -72.1), (-8.8, 18.4, -73.8), (-8.8, 36.2, -73.8)],
    'pico_boss_x': [-8.8, 37.0],
    # ---- buck converter footprint on the floor, held by a snap tray
    'buck_x0': -93.6, 'buck_x1': -71.2, 'buck_z0': -67.5, 'buck_z1': -50.4,
    'buck_lift': 5.0,          # board underside above the tray base, room for solder joints and for the snap arms
    'buck_pcb_t': 1.6,
    # ---- the DC plug adapter in the projector's jack, top of it
    'dc_plug_z0': 21.8, 'dc_plug_top': 49.8,
    # ---- fasteners
    'm3_insert_d': 4.0, 'm3_clear_d': 3.4, 'm2_insert_d': 3.2,
    'end_screw_z': [-66.0, -16.0, 32.0],
    'rear_screw_u': [-77.0, -42.9, -29.0, 36.6, 57.0],   # from the projector centre, two of them flank the plate seam
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


def chamfer_edge(body, axis, a, b, sa, sb, c, n0, n1):
    """45 degree chamfer, legs c, on the edge that runs along the normal axis through a, b.

    sa, sb say on which side of the edge there is no material: +1 or -1 along a and b.
    """
    ang = math.degrees(math.atan2(sb, sa)) + 90.0
    cut(body, rot_rect(axis, a, b, ang, 6 * c + 10, c * math.sqrt(2.0), n0, n1))


def truss_window(axis, a0, a1, b0, b1, n0, n1, c, rib, rising):
    """Octagon window with one diagonal rib left standing. rising: the rib goes from a0, b0 to a1, b1."""
    w = octa(axis, a0, a1, b0, b1, n0, n1, c)
    da, db = (a1 - a0), (b1 - b0)
    ang = math.degrees(math.atan2(db, da if rising else -da))
    cut(w, rot_rect(axis, (a0 + a1) / 2.0, (b0 + b1) / 2.0, ang, math.hypot(da, db) + 20, rib, n0 - 1, n1 + 1))
    return w


def derived():
    d = dict(P)
    d['xc'] = (P['proj_x0'] + P['proj_x1']) / 2.0
    d['proj_half'] = (P['proj_x1'] - P['proj_x0']) / 2.0
    d['xil'] = P['proj_x0'] - P['gap_port']
    d['xir'] = P['proj_x1'] + P['gap_lever']
    d['xol'] = d['xil'] - P['web']
    d['xor'] = d['xir'] + P['web']
    d['zro'] = P['z_rear'] - P['web']
    d['yb'] = P['floor_top'] - P['plate_t']
    d['yt'] = P['top_bottom'] + P['plate_t']
    d['x_end_port'] = d['xil'] + P['flange_w'] / 2.0     # screw line of the -X frame
    d['x_end_lever'] = d['xir'] - P['flange_w'] / 2.0    # screw line of the +X frame
    d['z_rear_screw'] = P['z_rear'] + P['flange_w'] / 2.0
    d['rear_screw_x'] = [d['xc'] + u for u in P['rear_screw_u']]
    # the two rear holes of the bay port sit over the rear flange: they are frame screws too
    h = P['port_pitch'] / 2.0
    d['rear_port_screws'] = [(d['xc'] - h, P['port_sensor_z'] - h), (d['xc'] + h, P['port_sensor_z'] - h)]
    d['port_top_flange_z1'] = P['dc_plug_z0'] - 9.8      # the top flange of the -X frame stops before the DC plug
    d['buck_pcb_y0'] = P['floor_top'] + 2.0 + P['buck_lift']
    d['buck_zc'] = (P['buck_z0'] + P['buck_z1']) / 2.0
    d['buck_ear_x'] = [P['buck_x0'] - 6.0, P['buck_x1'] + 6.0]
    return d


def frame_screws(d, top):
    """x, z of every screw that ties a frame flange to a plate."""
    pts = []
    for z in d['end_screw_z']:
        if not (top and z > d['port_top_flange_z1'] - 4):
            pts.append((d['x_end_port'], z))
        pts.append((d['x_end_lever'], z))
    for x in d['rear_screw_x']:
        pts.append((x, d['z_rear_screw']))
    pts.extend(d['rear_port_screws'])
    return pts


def port_holes(d):
    h = d['port_pitch'] / 2.0
    pts = []
    for z in (d['port_tripod_z'], d['port_sensor_z']):
        for sx in (-1, 1):
            for sz in (-1, 1):
                if z == d['port_sensor_z'] and sz < 0:
                    continue    # frame screws, see rear_port_screws
                pts.append((d['xc'] + sx * h, z + sz * h))
    return pts


def strap_holes(d, top):
    zs = d['strap_z_top'] if top else d['strap_z_floor']
    return [(d['plate_seam_x'] + s * d['strap_dx'], z) for z in zs for s in (-1, 1)]


def split_x(body, x, gap):
    """Cut a body in two at x. Returns the -X part and the +X part."""
    left = tm().copy(body)
    right = tm().copy(body)
    tm().booleanOperation(left, rect('y', -600, x - gap / 2.0, -600, 600, -600, 600),
                          adsk.fusion.BooleanTypes.IntersectionBooleanType)
    tm().booleanOperation(right, rect('y', x + gap / 2.0, 600, -600, 600, -600, 600),
                          adsk.fusion.BooleanTypes.IntersectionBooleanType)
    return left, right


def cut_ports(body, d, y0, y1):
    for z in (d['port_tripod_z'], d['port_sensor_z']):
        cut(body, cyl('y', d['xc'], z, d['port_open_d'] / 2.0, y0 - 1, y1 + 1))
    for (x, z) in port_holes(d):
        cut(body, cyl('y', x, z, d['m3_insert_d'] / 2.0, y0 - 1, y1 + 1))


def plate_blank(d, y0, y1, top):
    g = d['gap']
    b = rect('y', d['xil'] + g, d['xir'] - g, d['z_rear'] + g, d['z_front'], y0, y1)
    cut_ports(b, d, y0, y1)
    for (x, z) in frame_screws(d, top):
        cut(b, cyl('y', x, z, d['m3_clear_d'] / 2.0, y0 - 1, y1 + 1))
    for (x, z) in strap_holes(d, top):
        cut(b, cyl('y', x, z, d['m3_insert_d'] / 2.0, y0 - 1, y1 + 1))
    return b


def floor_plate(d):
    xc = d['xc']
    y0, y1 = d['yb'], d['floor_top']
    n0, n1 = y0 - 1, y1 + 1
    b = plate_blank(d, y0, y1, False)
    # obround for a 1/4 inch screw into the projector's own tripod thread
    px, pz = d['proj_tripod_x'], d['proj_tripod_z']
    slot = rect('y', px - 4, px + 4, pz - 3.5, pz + 3.5, n0, n1)
    join(slot, cyl('y', px - 4, pz, 3.5, n0, n1))
    join(slot, cyl('y', px + 4, pz, 3.5, n0, n1))
    cut(b, slot)
    # window under the projector's bottom vent slot
    cut(b, octa('y', xc - 27, xc + 27, -37.0, -29.5, n0, n1, 2))
    # under the projector, between the feet
    cut(b, octa('y', d['proj_x0'] - 6, d['plate_seam_x'] - 10, -18.0, 10.0, n0, n1, 5))
    cut(b, octa('y', xc + 25, d['xir'] - 12, -18.0, 10.0, n0, n1, 5))
    # row of three under the port bay, the buck tray sits behind it
    sx0, sx1 = d['xil'] + d['flange_w'], d['proj_x0'] - 11
    cell = 20.5
    for i in range(3):
        z0 = -41.5 + i * (cell + 6.0)
        cut(b, octa('y', sx0, sx1, z0, z0 + cell, n0, n1, 3))
    # under the pico
    cut(b, octa('y', xc + 25, d['xir'] - 12, -68.0, -52.0, n0, n1, 3))
    # inserts for the buck tray
    for x in d['buck_ear_x']:
        cut(b, cyl('y', x, d['buck_zc'], d['m3_insert_d'] / 2.0, n0, n1))
    # corner lips. The +X pair has no return: the flange of the end frame is the stop there
    c = d['proj_clear']
    lip_top = y1 + d['lip_h']
    zf = d['proj_z1'] + c
    zr = d['proj_z0'] - c
    lx0, lx1 = d['proj_x0'] - c - 3, d['proj_x0'] + 13.5
    join(b, rect('y', lx0, lx1, zf, d['z_front'], y1 - 0.5, lip_top))
    join(b, rect('y', lx0, lx0 + 3, zf - 10.5, d['z_front'], y1 - 0.5, lip_top))
    join(b, rect('y', lx0, lx1, zr - 3, zr, y1 - 0.5, lip_top))
    join(b, rect('y', lx0, lx0 + 3, zr - 3, zr + 14, y1 - 0.5, lip_top))
    rx0, rx1 = d['proj_x1'] - 13.5, d['xir'] - d['flange_w'] - d['gap']
    join(b, rect('y', rx0, rx1, zf, d['z_front'], y1 - 0.5, lip_top))
    join(b, rect('y', rx0, rx1, zr - 3, zr, y1 - 0.5, lip_top))
    return b


def top_plate(d):
    xc = d['xc']
    y0, y1 = d['top_bottom'], d['yt']
    n0, n1 = y0 - 1, y1 + 1
    b = plate_blank(d, y0, y1, True)
    lx0, lx1 = d['xil'] + d['flange_w'], xc - 21
    rx0, rx1 = xc + 21, d['xir'] - d['flange_w']
    # front row: over the port bay, over the projector's buttons, over the lens end
    rib0, rib1 = d['proj_x0'] - 4, d['proj_x0'] + 4
    cut(b, octa('y', lx0, rib0, -22.0, 32.0, n0, n1, 4))
    sm = d['plate_seam_x']
    cut(b, octa('y', rib1, sm - 4, -22.0, 32.0, n0, n1, 6))
    cut(b, octa('y', sm + 4, lx1 - 2, -22.0, 32.0, n0, n1, 3))
    cut(b, octa('y', rx0, rx1, -22.0, 32.0, n0, n1, 6))
    # rear row: one diagonal each, they open toward the front
    cut(b, truss_window('y', lx0, sm - 4, -70.0, -30.0, n0, n1, 4, 6, False))
    cut(b, octa('y', sm + 4, lx1 - 2, -70.0, -30.0, n0, n1, 3))
    cut(b, truss_window('y', rx0, rx1, -70.0, -30.0, n0, n1, 4, 6, True))
    return b


def end_frame(d, port):
    """Upright frame on one end. port: the -X end, where the top flange stops before the DC plug."""
    if port:
        x_out, x_in, s = d['xol'], d['xil'], -1
    else:
        x_out, x_in, s = d['xor'], d['xir'], 1
    x0, x1 = min(x_out, x_in), max(x_out, x_in)
    b = rect('x', d['z_rear'], d['z_front'], d['yb'], d['yt'], x0, x1)
    cut(b, octa('x', -74.0, -50.0, 8.8, 45.8, x0 - 1, x1 + 1, 5))
    cut(b, octa('x', -42.0, 32.0, 8.8, 45.8, x0 - 1, x1 + 1, 8))
    if port:
        # the window steps up in front of the DC plug, so its wires and its screws can be reached
        cut(b, octa('x', d['dc_plug_z0'] - 8, d['dc_plug_z0'] + 11.5, 30.0, d['dc_plug_top'] + 0.5, x0 - 1, x1 + 1, 3))
    ft, fw = d['flange_t'], d['flange_w']
    fa, fb = sorted((x_in + s * 0.5, x_in - s * fw))
    ztop = d['port_top_flange_z1'] if port else d['z_front']
    join(b, rect('x', d['z_rear'], d['z_front'], d['floor_top'], d['floor_top'] + ft, fa, fb))
    join(b, rect('x', d['z_rear'], ztop, d['top_bottom'] - ft, d['top_bottom'], fa, fb))
    xs = d['x_end_port'] if port else d['x_end_lever']
    for z in d['end_screw_z']:
        cut(b, cyl('y', xs, z, d['m3_insert_d'] / 2.0, d['floor_top'] - 1, d['floor_top'] + ft + 1))
        if z < ztop - 4:
            cut(b, cyl('y', xs, z, d['m3_insert_d'] / 2.0, d['top_bottom'] - ft - 1, d['top_bottom'] + 1))
    return b


def rear_slots(d):
    """x ranges of the slot vents. One mullion is centred on the encoder, one on the seam."""
    s0, s1 = d['xil'] + 4, d['xir'] - 4
    m = 3.0
    e = d['encoder_x']
    q = d['rear_seam_x']
    a_mid = (s0 + e - m) / 2.0
    c_mid = (q + m + s1) / 2.0
    return [(s0, a_mid - m), (a_mid + m, e - m), (e + m, q - m), (q + m, c_mid - m), (c_mid + m, s1)]


def rear_frame(d):
    z0, z1 = d['zro'], d['z_rear']
    n0, n1 = z0 - 1, z1 + 1
    b = rect('z', d['xol'], d['xor'], d['yb'], d['yt'], z0, z1)
    for (y0, y1) in ((8.0, 14.5), (40.1, 46.6)):
        for (a0, a1) in rear_slots(d):
            cut(b, octa('z', a0, a1, y0, y1, n0, n1, 2))
    cy = d['ctrl_y']
    # the nuts of the jack and the encoder need a thinner panel: thin the field on the inside
    cut(b, rect('z', d['jack_x'] - 11.5, d['encoder_x'] + 7.5, cy - 11.3, cy + 11.3, z0 + d['ctrl_panel_t'], n1))
    cut(b, cyl('z', d['jack_x'], cy, d['jack_d'] / 2.0, n0, n1))
    cut(b, cyl('z', d['button_x'], cy, d['button_d'] / 2.0, n0, n1))
    # the button thread is short: 2 of panel left, recess on the inside
    cut(b, cyl('z', d['button_x'], cy, 6.25, z0 + 2, n1))
    cut(b, cyl('z', d['encoder_x'], cy, d['encoder_d'] / 2.0, n0, n1))
    # behind the pico: a window, and the two screws of the pico holder
    bx = d['pico_boss_x']
    cut(b, octa('z', bx[0] + 7, bx[1] - 7, cy - 6.5, cy + 6.5, n0, n1, 3))
    for x in bx:
        cut(b, cyl('z', x, cy, d['m3_clear_d'] / 2.0, n0, n1))
    ft, fw, g = d['flange_t'], d['flange_w'], d['gap']
    fx0, fx1 = d['xil'] + fw + g, d['xir'] - fw - g
    join(b, rect('z', fx0, fx1, d['floor_top'], d['floor_top'] + ft, z1 - 0.5, z1 + fw))
    join(b, rect('z', fx0, fx1, d['top_bottom'] - ft, d['top_bottom'], z1 - 0.5, z1 + fw))
    # the flange grows a tab where it takes the two rear screws of the bay port
    for (x, z) in d['rear_port_screws']:
        join(b, rect('z', x - 5, x + 5, d['floor_top'], d['floor_top'] + ft, z1 + fw - 0.5, z1 + fw + 3))
        join(b, rect('z', x - 5, x + 5, d['top_bottom'] - ft, d['top_bottom'], z1 + fw - 0.5, z1 + fw + 3))
    pts = [(x, d['z_rear_screw']) for x in d['rear_screw_x']] + d['rear_port_screws']
    for (x, z) in pts:
        cut(b, cyl('y', x, z, d['m3_insert_d'] / 2.0, d['floor_top'] - 1, d['floor_top'] + ft + 1))
        cut(b, cyl('y', x, z, d['m3_insert_d'] / 2.0, d['top_bottom'] - ft - 1, d['top_bottom'] + 1))
    return b


def seam_strap(d):
    """Flat bar across the plate seam, on the outside. Built at the front of the floor, copied to the other three places."""
    x, z = d['plate_seam_x'], d['strap_z_floor'][0]
    y1 = d['yb']
    y0 = y1 - 3.0
    b = octa('y', x - 9.5, x + 9.5, z - 4.0, z + 4.0, y0, y1, 2)
    for s in (-1, 1):
        cut(b, cyl('y', x + s * d['strap_dx'], z, d['m3_clear_d'] / 2.0, y0 - 1, y1 + 1))
    return b


def pico_holder(d):
    z0 = d['z_rear']
    cy = d['ctrl_y']
    bx = d['pico_boss_x']
    xs = [h[0] for h in d['pico_holes']]
    b = rect('z', min(xs) - 5.7, max(xs) + 3.8, cy - 12.0, cy + 12.0, z0, z0 + 3)
    cut(b, octa('z', bx[0] + 7, bx[1] - 7, cy - 6.5, cy + 6.5, z0 - 1, z0 + 4, 3))
    for (x, y, ztop) in d['pico_holes']:
        join(b, cyl('z', x, y, 3.0, z0 + 2.5, ztop))
    for x in bx:
        join(b, cyl('z', x, cy, 4.0, z0 + 2.5, z0 + 5))
    # M2 inserts in the posts, M3 inserts in the two bosses
    for (x, y, ztop) in d['pico_holes']:
        cut(b, cyl('z', x, y, d['m2_insert_d'] / 2.0, z0 - 1, ztop + 1))
    for x in bx:
        cut(b, cyl('z', x, cy, d['m3_insert_d'] / 2.0, z0 - 1, z0 + 6))
    return b


def buck_tray(d):
    """Snap tray. The board drops between four corner posts onto two rails, two arms hook over its long edges."""
    yb0 = d['floor_top']
    yb1 = yb0 + 2.0
    py0 = d['buck_pcb_y0']
    py1 = py0 + d['buck_pcb_t']
    bx0, bx1, bz0, bz1 = d['buck_x0'], d['buck_x1'], d['buck_z0'], d['buck_z1']
    xm, zc = (bx0 + bx1) / 2.0, d['buck_zc']
    x0, x1, z0, z1 = bx0 - 2.0, bx1 + 2.0, bz0 - 2.0, bz1 + 2.0
    b = rect('y', x0, x1, z0, z1, yb0, yb1)
    # ears with the two screws
    for x in d['buck_ear_x']:
        join(b, octa('y', x - 4.5, x + 4.5, zc - 4.5, zc + 4.5, yb0, yb1, 2))
    join(b, rect('y', d['buck_ear_x'][0], d['buck_ear_x'][1], zc - 4.5, zc + 4.5, yb0, yb1))
    for x in d['buck_ear_x']:
        cut(b, cyl('y', x, zc, d['m3_clear_d'] / 2.0, yb0 - 1, yb1 + 1))
    # two rails carry the board, clear of the solder pads on its short ends
    for sx in (-1, 1):
        join(b, rect('y', xm + sx * 4.0 - 1.5, xm + sx * 4.0 + 1.5, bz0 + 1.0, bz1 - 1.0, yb1 - 0.5, py0))
    # corner posts up to the top of the board
    posts = rect('y', x0, x1, z0, z1, yb1 - 0.5, py1)
    cut(posts, rect('y', bx0 - 0.3, bx1 + 0.3, bz0 - 0.3, bz1 + 0.3, yb0, py1 + 1))
    keep = None
    for cx in (x0, x1):
        for cz in (z0, z1):
            k = rect('y', cx - 4.8, cx + 4.8, cz - 4.8, cz + 4.8, yb0, py1 + 1)
            if keep is None:
                keep = k
            else:
                join(keep, k)
    tm().booleanOperation(posts, keep, adsk.fusion.BooleanTypes.IntersectionBooleanType)
    join(b, posts)
    # snap arms on the two long sides, each set off to the side where the board edge is free:
    # the trimmer sits 0.4 from one edge, the diode 0.7 from the other
    hook = 0.6
    for (ze, s, xo) in ((bz0, -1, 3.0), (bz1, 1, -3.0)):
        ax0, ax1 = xm + xo - 3.0, xm + xo + 3.0
        a0, a1 = sorted((ze + s * 0.3, ze + s * 1.3))
        join(b, rect('y', ax0, ax1, a0, a1, yb1 - 0.5, py1 + 0.1))
        h0, h1 = sorted((ze + s * 1.3, ze - s * hook))
        hk = rect('y', ax0, ax1, h0, h1, py1 + 0.1, py1 + 1.6)
        chamfer_edge(hk, 'x', ze - s * hook, py1 + 1.6, -s, 1, 1.2, ax0 - 1, ax1 + 1)
        join(b, hk)
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


def fasteners(d):
    """Every insert and screw: kind, where it belongs, seat point, direction it goes in."""
    rows = []

    def add(kind, group, x, y, z, dx, dy, dz):
        rows.append({'kind': kind, 'group': group, 'seat': [round(x, 2), round(y, 2), round(z, 2)], 'into': [dx, dy, dz]})

    h = d['port_pitch'] / 2.0
    for top in (False, True):
        y_face = d['top_bottom'] if top else d['floor_top']
        y_out = d['yt'] if top else d['yb']
        s = -1 if top else 1
        name = 'top' if top else 'floor'
        for (x, z) in frame_screws(d, top):
            add('M3 insert', 'frame flange, ' + name, x, y_face, z, 0, s, 0)
            if not top and (x, z) in d['rear_port_screws']:
                # the sensor module sits here: its screw goes through module and plate into the flange
                add('M3 x 12', 'sensor module, into the rear flange', x, y_out - 4.0, z, 0, s, 0)
            else:
                add('M3 x 8', 'plate to frame, ' + name, x, y_out, z, 0, s, 0)
        for (x, z) in port_holes(d):
            add('M3 insert', 'module port, ' + name, x, y_out, z, 0, s, 0)
        for (x, z) in strap_holes(d, top):
            add('M3 insert', 'seam strap, ' + name, x, y_out, z, 0, s, 0)
            add('M3 x 8', 'seam strap, ' + name, x, y_out - s * 3.0, z, 0, s, 0)
    for sx in (-1, 1):
        for sz in (-1, 1):
            add('M3 x 6', 'tripod adapter', d['xc'] + sx * h, d['yb'] - 2.2, d['port_tripod_z'] + sz * h, 0, 1, 0)
        add('M3 x 8', 'sensor module', d['xc'] + sx * h, d['yb'] - 4.0, d['port_sensor_z'] + h, 0, 1, 0)
    for x in d['pico_boss_x']:
        add('M3 insert', 'pico holder', x, d['ctrl_y'], d['z_rear'], 0, 0, 1)
        add('M3 x 8', 'pico holder', x, d['ctrl_y'], d['zro'], 0, 0, 1)
    for (x, y, ztop) in d['pico_holes']:
        add('M2 insert', 'pico post', x, y, ztop, 0, 0, -1)
        add('M2 x 5', 'pico', x, y, ztop + 1.6, 0, 0, -1)
    for x in d['buck_ear_x']:
        add('M3 insert', 'buck tray, in the floor', x, d['floor_top'], d['buck_zc'], 0, -1, 0)
        add('M3 x 6', 'buck tray', x, d['floor_top'] + 2.0, d['buck_zc'], 0, -1, 0)
    add('1/4-20 x 10', 'projector', d['proj_tripod_x'], d['yb'], d['proj_tripod_z'], 0, 1, 0)
    return rows


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
    return body


def main():
    app = adsk.core.Application.get()
    doc = app.activeDocument
    if not doc.name.startswith('komorebi-cage'):
        raise RuntimeError('active document is %s, not komorebi-cage' % doc.name)
    # raises on the copy that komorebi-box keeps open: a change there is never saved
    doc.isUpToDate
    des = adsk.fusion.Design.cast(app.activeProduct)
    root = des.rootComponent
    d = derived()
    ap = _appearance(app, des, d['colour'])
    while root.rigidGroups.count:
        root.rigidGroups.item(0).deleteMe()
    # each end frame is its own component now: drop the turned copy of the first draft
    seen = set()
    for o in list(root.occurrences):
        if o.component.name in seen:
            o.deleteMe()
        else:
            seen.add(o.component.name)
    g = d['gap']
    floor_l, floor_r = split_x(floor_plate(d), d['plate_seam_x'], g)
    top_l, top_r = split_x(top_plate(d), d['plate_seam_x'], g)
    rear_l, rear_r = split_x(rear_frame(d), d['rear_seam_x'], g)
    parts = [
        ('Cage: floor plate', lambda: floor_r),
        ('Cage: floor plate, port', lambda: floor_l),
        ('Cage: top plate', lambda: top_r),
        ('Cage: top plate, port', lambda: top_l),
        ('Cage: end frame', lambda: end_frame(d, False)),
        ('Cage: end frame, port', lambda: end_frame(d, True)),
        ('Cage: rear frame', lambda: rear_r),
        ('Cage: rear frame, port', lambda: rear_l),
        ('Cage: seam strap', lambda: seam_strap(d)),
        ('Pico holder: base', lambda: pico_holder(d)),
        ('Buck converter holder: tray', lambda: buck_tray(d)),
        ('Light sensor module: plate', lambda: sensor_module(d)),
    ]
    for name, fn in parts:
        body = put(root, name, fn(), ap)
        bb = body.boundingBox
        print('%-30s %6.1f x %5.1f x %6.1f  lumps %d' % (
            name, (bb.maxPoint.x - bb.minPoint.x) * 10, (bb.maxPoint.y - bb.minPoint.y) * 10,
            (bb.maxPoint.z - bb.minPoint.z) * 10, body.lumps.count))
    # three more straps: floor middle, top front, top middle
    strap = [o for o in root.occurrences if o.component.name == 'Cage: seam strap'][0].component
    z0 = d['strap_z_floor'][0]
    up = d['yt'] - (d['yb'] - 3.0)
    for (dy, dz) in ((0.0, d['strap_z_floor'][1] - z0), (up, d['strap_z_top'][0] - z0), (up, d['strap_z_top'][1] - z0)):
        m = adsk.core.Matrix3D.create()
        m.translation = adsk.core.Vector3D.create(0, dy / 10.0, dz / 10.0)
        root.occurrences.addExistingComponent(strap, m)
    coll = adsk.core.ObjectCollection.create()
    for o in root.occurrences:
        coll.add(o)
    root.rigidGroups.add(coll, True)
    rows = fasteners(d)
    counts = {}
    for r in rows:
        counts[r['kind']] = counts.get(r['kind'], 0) + 1
    out = os.path.join(os.path.dirname(__file__), 'fasteners.json')
    with open(out, 'w') as fh:
        json.dump({'counts': counts, 'fasteners': rows}, fh, indent=2)
    print('occurrences', root.occurrences.count, 'rigid groups', root.rigidGroups.count)
    print('fasteners', counts)
