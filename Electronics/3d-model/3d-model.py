import math
import cadquery as cq

# ---------------- Parameters (mm) ----------------
OUT = 150.0                 # enclosure outer size: 150 x 150
PCB = 100.0                 # PCB: 100 x 100
WALL = 2.5
CLEAR = 1.0
PCB_OFF = (OUT - PCB) / 2.0 # centered PCB: 25 mm from each outer edge
R_OUT = 8.0
FLOOR = 2.5
STANDOFF_H = 5.0
PCB_T = 1.6

RAISE = 10.0                # extra height for the whole box (base + lid move up together)
HF = 30.0 + RAISE           # front height
HB = 42.0 + RAISE           # rear height
Y_S0, Y_S1 = 16.0, 54.0     # front slope limits (rear end moved 46->54 so the 35 mm TFT board fits)
Z_SPLIT = 24.0 + RAISE      # base gets the extra depth; lid keeps its size, so lid screw lengths stay the same
TONGUE_H = 3.0

# PCB mounting holes: read from Development_pcb.kicad_pcb (board 100x100, outline
# x 180.34..280.34, y 62.23..162.23; three M3 holes of 3.2 mm; the 4th corner is empty).
# KiCad coordinates relative to the board's top-left corner (X right, Y DOWN on screen):
KICAD_HOLES = [(5.0, 95.0), (5.08, 5.08), (95.0, 5.0)]
KICAD_EMPTY_CORNER = (95.0, 95.0)       # no hole here -> plain support post
# Orientation in the box (front is y=0). The PCB is turned 180 deg so the ESP32 USB end,
# the power-input parts and the USB-C wall face the rear, and the 8-pin TFT header
# (J6) is nearest the front screen. Set False to place the board as it looks in KiCad.
PCB_ROT180 = True
def _kicad_to_box(p):
    return (PCB - p[0], p[1]) if PCB_ROT180 else (p[0], PCB - p[1])
HOLES = [_kicad_to_box(p) for p in KICAD_HOLES]
DUMMY = _kicad_to_box(KICAD_EMPTY_CORNER)

# Four enclosure screws are independent of the PCB holes.
ENC_SCREWS = [(12.0, 12.0), (OUT - 12.0, 12.0),
              (12.0, OUT - 12.0), (OUT - 12.0, OUT - 12.0)]
# Enclosure screws: M3. Lid = clearance hole; base = pilot for a self-tapping screw
# (the screw cuts its own thread in the plastic) or a hole for a heat-set insert.
ENC_SCREW_MODE = "insert"        # "insert" (heat-set, best for many openings) or "selftap"
SCREW_HOLE = 3.4                # lid clearance for M3
ENC_PILOT_D = 2.6               # base pilot, M3 self-tapping in PLA/PETG
ENC_INSERT_D = 4.0              # base hole for an M3x5.7 heat-set insert (OD 4.6)
ENC_INSERT_DEPTH = 7.0
SCREW_BOSS_R = 4.8
SCREW_COUNTERBORE_R = 3.2       # 6.4 mm recess for an M3 button head (5.7 mm)
SCREW_CB_DEPTH = 2.0            # head is 1.65 mm -> sits below the surface

# Common 1.8-inch 128x160 ST7735 RGB SPI module.
# Landscape module body is approximately 58 x 34.5 mm;
# active display is approximately 35.04 x 28.03 mm.
TFT_BODY_W = 56.0
TFT_BODY_H = 35.0
TFT_WINDOW_W = 35.04 + 1.20   # active area + 0.6 mm per side (print tolerance + position error)
TFT_WINDOW_H = 28.03 + 1.20
TFT_HOLE_INNER_D = 2.0      # PCB hole (datasheet)
TFT_HOLE_PAD_D = 4.5        # PCB pad / boss outer diameter (datasheet)
TFT_PILOT_D = 1.7           # pilot hole for M2 self-tapping screw
TFT_GLASS_H = 2.25          # glass stack height above the PCB front (3.45 - 1.20)
TFT_BOSS_H = TFT_GLASS_H + 0.05   # PCB front rests on the boss tops, glass touches the wall
TFT_PILOT_INTO_WALL = 1.0   # pilot depth beyond the boss, inside the wall (wall = 2.5)
TFT_ACTIVE_OFFSET = 2.5     # active area centre is 2.5 mm off the PCB centre toward the pin edge (re-measured: 8.0 mm from the pin edge, 13.0 mm from the other)
TFT_PIN_SIDE = +1           # +1: pin edge on the right (seen from the front), -1: left
TFT_HOLE_EDGE = 2.5

# Realistic panel-mount USB-C opening, centered on rear wall.
# USB-C cable plug opening on the rear wall: a stepped "key".
#   outer pocket  = plug overmold (black plastic): 5.1 mm tall (measured by you) + clearance
#   inner slot    = metal plug tip: 8.25 x 2.40 mm (USB Type-C spec) + clearance
# For reference the spec receptacle opening is 8.34 x 2.56 mm.
USB_X = 75.0                # centre X on the rear wall
USB_Z = 14.0                # centre height (as in the original design)
USB_SLOT_W = 8.25 + 0.55    # 8.80
USB_SLOT_H = 2.40 + 0.40    # 2.80 (stadium shape, like the plug)
USB_POCKET_W = 12.8         # spec max overmold ~12.35 mm (your photo looks ~10): tighten once measured
USB_POCKET_H = 5.1 + 0.4    # 5.50
USB_POCKET_R = 1.5
USB_POCKET_DEPTH = 1.7      # wall is 2.5 -> 0.8 mm web; lets the 6.65 mm plug tip reach the connector behind the wall

# Right-wall keyed switch opening (x = OUT). Front is y=0.
SWITCH_D = 12.2            # 12 mm switch + 0.2 print tolerance
SWITCH_Y = 44.0
SWITCH_Z = 14.0

# Left-wall speaker: datasheet says 30 mm x 5.5 mm, but the real one measures larger -> holder sized for 32 mm.
SPK_D = 32.0                # measure your speaker with a caliper and set this
SPK_CLEAR = 0.4             # radial clearance around the speaker frame
SPK_RING_H = 4.0            # retaining ring height (measured from the inner wall)
SPK_RING_T = 3.0            # retaining ring thickness
SPK_Y = OUT / 2.0           # centered along the left wall
SPK_Z = 22.0 + RAISE / 2.0   # keeps the speaker at the same relative spot on the taller wall
SPK_GRILLE_D = 3.0
SPK_GRILLE = ((0.0, 1), (5.5, 6), (10.0, 12))   # (ring radius, number of holes)

# Side ventilation (left and right walls); none on front or rear.
VENT_D = 2.4
VENT_ZS = (9.0, 14.0, 19.0)
VENT_Y0 = 62.0
VENT_ROWS = 3
VENT_COLS = 11
VENT_PITCH = 5.5

# Servo (standard size, from the mechanical drawing): body 40.3 x 20, ears 53.6 long.
SERVO_L = 40.3
SERVO_W = 20.0
SERVO_CLR = 0.25            # clearance per side
SERVO_WALL = 2.5            # cradle wall thickness
SERVO_WALL_H = 16.0         # cradle wall height (ears are ~24 mm above the servo bottom, so they stay free)
SERVO_WIRE_W = 12.0         # wire channel in the rear cradle wall (JST XH-3 housing is ~9.9 x 5.75 mm)
SERVO_WIRE_H = SERVO_WALL_H + 1.0   # channel is open at the top: the cable just drops in
SERVO_HOLE_L, SERVO_HOLE_W = 32.0, 12.0   # roof cable slot behind the cradle: 2 servos, ESP-CAM supply, laser, LiDAR
SERVO_HOLE_GAP = 9.0        # distance from cradle rear wall to the slot centre
SERVO_ALIGN_SHAFT = False   # False: cradle centred on the flat roof. True: output shaft centred.
SERVO_SHAFT_FROM_WIRE_END = 26.0   # measured from the drawing (approx.)

# ---------------- Helpers ----------------
def rrect(w, d, r, x0, y0, h, z0=0.0):
    return (cq.Workplane("XY").workplane(offset=z0)
            .center(x0 + w / 2, y0 + d / 2).rect(w, d).extrude(h)
            .edges("|Z").fillet(r))

def profile_solid(y_pts_z, x0, x1):
    wp = cq.Workplane("YZ").workplane(offset=x0).polyline(y_pts_z).close()
    return wp.extrude(x1 - x0)

def rounded_box_on_plane(plane, width, height, radius, depth, both=False):
    wp = cq.Workplane(plane).rect(width, height)
    if radius > 0:
        wp = wp.vertices().fillet(radius)
    return wp.extrude(depth, both=both)

# ---------------- Slope geometry ----------------
slope_len = math.hypot(Y_S1 - Y_S0, HB - HF)
ux, uz = (Y_S1 - Y_S0) / slope_len, (HB - HF) / slope_len
nx, nz = -uz, ux
ANG = math.degrees(math.atan2(HB - HF, Y_S1 - Y_S0))


def inner_slope_y(z):
    py, pz = Y_S0 - nx * WALL, HF - nz * WALL
    t = (z - pz) / uz
    return py + ux * t

# ---------------- Outer / inner solids ----------------
outer_prof = [(0, 0), (OUT, 0), (OUT, HB),
              (Y_S1, HB), (Y_S0, HF), (0, HF)]
outer = profile_solid(outer_prof, 0, OUT).intersect(
    rrect(OUT, OUT, R_OUT, 0, 0, HB + 5))
try:
    outer = outer.faces(">Z or (not <Z and not |Z)").edges().fillet(1.2)
except Exception:
    pass

zi_f, zi_b = HF - WALL, HB - WALL
inner_prof = [(WALL, FLOOR), (OUT - WALL, FLOOR), (OUT - WALL, zi_b),
              (inner_slope_y(zi_b), zi_b), (inner_slope_y(zi_f), zi_f),
              (WALL, zi_f)]
inner = profile_solid(inner_prof, WALL, OUT - WALL).intersect(
    rrect(OUT - 2 * WALL, OUT - 2 * WALL, R_OUT - WALL,
          WALL, WALL, HB + 5))
shell = outer.cut(inner)

# ---------------- Base / lid split ----------------
below = cq.Workplane("XY").box(OUT, OUT, Z_SPLIT, centered=False)
above = cq.Workplane("XY").box(OUT, OUT, HB, centered=False).translate(
    (0, 0, Z_SPLIT))
base = shell.intersect(below)
lid = shell.intersect(above)

# Tongue and groove
inner_rr = rrect(OUT - 2 * WALL, OUT - 2 * WALL, R_OUT - WALL,
                 WALL, WALL, 50, 0)
t_out = rrect(OUT - 2 * 1.25, OUT - 2 * 1.25, R_OUT - 1.25,
             1.25, 1.25, TONGUE_H, Z_SPLIT)
tongue = t_out.cut(rrect(OUT - 2 * WALL, OUT - 2 * WALL, R_OUT - WALL,
                         WALL, WALL, TONGUE_H, Z_SPLIT))
base = base.union(tongue)
g_out = rrect(OUT - 2 * 1.0, OUT - 2 * 1.0, R_OUT - 1.0,
             1.0, 1.0, TONGUE_H + 0.3, Z_SPLIT)
groove = g_out.cut(rrect(OUT - 2 * WALL, OUT - 2 * WALL, R_OUT - WALL,
                         WALL, WALL, TONGUE_H + 0.3, Z_SPLIT))
lid = lid.cut(groove)

# ---------------- PCB supports ----------------
def pcb2box(p):
    return p[0] + PCB_OFF, p[1] + PCB_OFF

z_pcb_bot = FLOOR + STANDOFF_H
z_pcb_top = z_pcb_bot + PCB_T

# Four physical support posts. The fourth is solid because the source PCB
# uses three actual mounting holes; the dummy post still supports the PCB.
for p in HOLES + [DUMMY]:
    x, y = pcb2box(p)
    so = (cq.Workplane("XY").workplane(offset=FLOOR - 0.01)
          .center(x, y).circle(3.5).extrude(STANDOFF_H + 0.01))
    base = base.union(so)
for p in HOLES:
    x, y = pcb2box(p)
    base = base.cut(
        cq.Workplane("XY").workplane(offset=FLOOR + 1.0)
        .center(x, y).circle(1.3).extrude(STANDOFF_H + 2))

# ---------------- Four enclosure screw columns (base boss + lid post) ----------------
for x, y in ENC_SCREWS:
    boss = (cq.Workplane("XY").workplane(offset=FLOOR - 0.01)
            .center(x, y).circle(SCREW_BOSS_R)
            .extrude(Z_SPLIT - FLOOR + 0.02))
    base = base.union(boss)
    if ENC_SCREW_MODE == "selftap":
        z0, depth, d = FLOOR - 0.1, Z_SPLIT - FLOOR + 0.4, ENC_PILOT_D
    else:
        z0, depth, d = Z_SPLIT - ENC_INSERT_DEPTH, ENC_INSERT_DEPTH + 0.4, ENC_INSERT_D
    base = base.cut(
        cq.Workplane("XY").workplane(offset=z0)
        .center(x, y).circle(d / 2.0).extrude(depth))

    lid_top = HF if y <= Y_S0 else (HF + (HB - HF) * (y - Y_S0) / (Y_S1 - Y_S0) if y < Y_S1 else HB)
    # Solid post under the lid roof so the screw head has real material to bear on.
    post = (cq.Workplane("XY").workplane(offset=Z_SPLIT)
            .center(x, y).circle(SCREW_BOSS_R)
            .extrude(lid_top - 0.2 - Z_SPLIT))
    lid = lid.union(post)
    # Clearance hole through post + roof.
    lid = lid.cut(
        cq.Workplane("XY").workplane(offset=Z_SPLIT - 0.1)
        .center(x, y).circle(SCREW_HOLE / 2).extrude(lid_top - Z_SPLIT + 1.0))
    # Head recess.
    lid = lid.cut(
        cq.Workplane("XY").workplane(offset=lid_top - SCREW_CB_DEPTH)
        .center(x, y).circle(SCREW_COUNTERBORE_R)
        .extrude(SCREW_CB_DEPTH + 0.5))

# ---------------- Bottom feet recesses ----------------
for fx, fy in [(14, 14), (OUT - 14, 14),
               (14, OUT - 14), (OUT - 14, OUT - 14)]:
    base = base.cut(
        cq.Workplane("XY").center(fx, fy).circle(4.5).extrude(1.0))

CX = OUT / 2.0

# ---------------- Rear USB-C: stepped pocket + slot ----------------
usb_plane = cq.Plane(origin=(USB_X, OUT + 1.0, USB_Z),
                     xDir=(1, 0, 0), normal=(0, -1, 0))      # normal -Y: extrudes into the box
pocket_usb = (cq.Workplane(usb_plane).rect(USB_POCKET_W, USB_POCKET_H)
              .extrude(1.0 + USB_POCKET_DEPTH))
try:
    pocket_usb = pocket_usb.edges("|Y").fillet(USB_POCKET_R)
except Exception:
    pass
base = base.cut(pocket_usb)
slot_usb = (cq.Workplane(usb_plane).slot2D(USB_SLOT_W, USB_SLOT_H, 0)
            .extrude(1.0 + WALL + 1.0))
base = base.cut(slot_usb)

# ---------------- Right-wall keyed switch hole ----------------
# x = OUT. 12 mm hole for the key switch.
sw_plane = cq.Plane(origin=(OUT + 1.0, SWITCH_Y, SWITCH_Z),
                    xDir=(0, 1, 0), normal=(-1, 0, 0))
sw = cq.Workplane(sw_plane).circle(SWITCH_D / 2.0).extrude(WALL + 2.0)
base = base.cut(sw)

# ---------------- Left-wall speaker (30 mm) ----------------
# Local plane coords: x -> world Y, y -> world Z, normal -> +X (into the box).
spk_in = cq.Plane(origin=(WALL - 0.01, SPK_Y, SPK_Z),
                  xDir=(0, 1, 0), normal=(1, 0, 0))
r_in = SPK_D / 2.0 + SPK_CLEAR
ring = (cq.Workplane(spk_in).circle(r_in + SPK_RING_T).circle(r_in)
        .extrude(SPK_RING_H))
# The ring crosses the split line, so each half gets its own part of it.
base = base.union(ring.intersect(below))
lid = lid.union(ring.intersect(above))

# Wire notch at the bottom of the ring (follows the speaker height).
r_ring_out = r_in + SPK_RING_T
notch = (cq.Workplane("XY").box(SPK_RING_H + 0.5, 8.0, SPK_RING_T + 4.0, centered=False)
         .translate((WALL, SPK_Y - 4.0, SPK_Z - r_ring_out - 0.2)))
base = base.cut(notch)

# Grille: 1 + 6 + 12 holes through the wall.
spk_out = cq.Plane(origin=(-1.0, SPK_Y, SPK_Z),
                   xDir=(0, 1, 0), normal=(1, 0, 0))
pts = []
for rad, n in SPK_GRILLE:
    for i in range(n):
        a = 2 * math.pi * i / n
        pts.append((rad * math.cos(a), rad * math.sin(a)))
grille = (cq.Workplane(spk_out).pushPoints(pts)
          .circle(SPK_GRILLE_D / 2.0).extrude(WALL + 2.0))
base = base.cut(grille)
lid = lid.cut(grille)

# ---------------- Side ventilation (left + right walls) ----------------
# Each hole gets its own plane (origin at the hole centre), so the right-wall
# plane orientation (where local Y would be flipped) cannot mirror the holes.
# Left wall: two blocks either side of the speaker. Right wall: block behind
# the key switch. Front/rear walls have no vents (USB-C is on the rear).
def vent_ys(y_start, y_end, row):
    ys, y = [], y_start + (VENT_PITCH / 2.0 if row % 2 else 0.0)
    while y <= y_end:
        ys.append(y)
        y += VENT_PITCH
    return ys

VENT_BLOCKS = {
    "left":  [(18.0, 48.0), (100.0, OUT - 14.0)],
    "right": [(VENT_Y0, OUT - 14.0)],
}
for wall, blocks in VENT_BLOCKS.items():
    x_off, normal = (-1.0, (1, 0, 0)) if wall == "left" else (OUT + 1.0, (-1, 0, 0))
    for y0, y1 in blocks:
        for row, z in enumerate(VENT_ZS):
            for y in vent_ys(y0, y1, row):
                pl = cq.Plane(origin=(x_off, y, z), xDir=(0, 1, 0), normal=normal)
                base = base.cut(cq.Workplane(pl).circle(VENT_D / 2.0)
                                .extrude(WALL + 2.0))

# ---------------- TFT 1.8-inch ST7735: window + 4 screw bosses on front slope ----------------
# Mounting: the PCB sits on 4 bosses (4.5 mm pads, as on the PCB) with the glass
# touching the inside of the wall; M2 self-tapping screws (~4 mm) go from the PCB
# back into the bosses. The board is shifted so the ACTIVE area is centred in the window.
mid = slope_len / 2.0
cy = Y_S0 + ux * mid
cz = HF + uz * mid
slope_plane = cq.Plane(origin=(CX, cy, cz),
                       xDir=(1, 0, 0), normal=(0, nx, nz))

window = cq.Workplane(slope_plane).rect(TFT_WINDOW_W, TFT_WINDOW_H).extrude(8, both=True)
lid = lid.cut(window)

pcb_dx = -TFT_PIN_SIDE * TFT_ACTIVE_OFFSET
hole_x = (-(TFT_BODY_W / 2.0 - TFT_HOLE_EDGE) + pcb_dx,
           (TFT_BODY_W / 2.0 - TFT_HOLE_EDGE) + pcb_dx)
hole_y = (-(TFT_BODY_H / 2.0 - TFT_HOLE_EDGE),
           (TFT_BODY_H / 2.0 - TFT_HOLE_EDGE))
tft_pts = [(hx, hy) for hx in hole_x for hy in hole_y]

# Bosses grow from the inner wall face into the cavity (normal = -n).
# Start 0.3 mm inside the wall so the union is solid.
boss_plane = cq.Plane(origin=(CX, cy - nx * (WALL - 0.3), cz - nz * (WALL - 0.3)),
                      xDir=(1, 0, 0), normal=(0, -nx, -nz))
bosses = (cq.Workplane(boss_plane).pushPoints(tft_pts)
          .circle(TFT_HOLE_PAD_D / 2.0).extrude(TFT_BOSS_H + 0.3))
lid = lid.union(bosses)

# Pilot holes drilled from the boss tops into the wall (blind: outer face stays closed).
top_plane = cq.Plane(origin=(CX, cy - nx * (WALL + TFT_BOSS_H), cz - nz * (WALL + TFT_BOSS_H)),
                     xDir=(1, 0, 0), normal=(0, nx, nz))
pilots = (cq.Workplane(top_plane).pushPoints(tft_pts)
          .circle(TFT_PILOT_D / 2.0).extrude(TFT_BOSS_H + TFT_PILOT_INTO_WALL))
lid = lid.cut(pilots)

# ---------------- Servo stand on the flat roof ----------------
flat_cy = (Y_S1 + OUT) / 2.0
sy = flat_cy + ((SERVO_SHAFT_FROM_WIRE_END - SERVO_L / 2.0) if SERVO_ALIGN_SHAFT else 0.0)
sx = CX
in_x, in_y = SERVO_W + 2 * SERVO_CLR, SERVO_L + 2 * SERVO_CLR
out_x, out_y = in_x + 2 * SERVO_WALL, in_y + 2 * SERVO_WALL

cradle = (cq.Workplane("XY").workplane(offset=HB - 0.3).center(sx, sy)
          .rect(out_x, out_y).extrude(SERVO_WALL_H + 0.3))
try:
    cradle = cradle.edges("|Z").fillet(2.0)
except Exception:
    pass
pocket = (cq.Workplane("XY").workplane(offset=HB).center(sx, sy)
          .rect(in_x, in_y).extrude(SERVO_WALL_H + 1.0))
cradle = cradle.cut(pocket)
lid = lid.union(cradle)

# Wire end of the servo faces the rear (+Y): notch in the rear wall ...
notch = (cq.Workplane("XY").workplane(offset=HB).center(sx, sy + out_y / 2.0 - SERVO_WALL / 2.0)
         .rect(SERVO_WIRE_W, SERVO_WALL + 1.0).extrude(SERVO_WIRE_H))
lid = lid.cut(notch)
# ... and a slot through the roof right behind the cradle for the wires.
slot = (cq.Workplane("XY").workplane(offset=HB - WALL - 1.0)
        .center(sx, sy + out_y / 2.0 + SERVO_HOLE_GAP)
        .slot2D(SERVO_HOLE_L, SERVO_HOLE_W, 0).extrude(WALL + 2.0))
lid = lid.cut(slot)

# ---------------- Pry notches (front + rear centre) ----------------
# Cut only into the BASE top edge (1.0 mm deep, 2.5 mm high): the lid has a thin 1.0 mm lip
# over the tongue that must stay intact. A flat screwdriver goes in below the lid lip.
for ny0 in (-1.0, OUT - 1.0):
    notch_p = (cq.Workplane("XY").box(16.0, 2.0, 2.5, centered=(True, False, False))
               .translate((CX, ny0, Z_SPLIT - 2.5)))
    base = base.cut(notch_p)

# ---------------- Export ----------------
import os
out_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "outputs")
os.makedirs(out_dir, exist_ok=True)

cq.exporters.export(base, f"{out_dir}/enclosure_base_150mm_corrected.step")
cq.exporters.export(lid, f"{out_dir}/enclosure_lid_150mm_corrected.step")
asm = cq.Assembly().add(base, name="base").add(lid, name="lid")
asm.save(f"{out_dir}/enclosure_assembly_150mm_corrected.step")
cq.exporters.export(base, f"{out_dir}/enclosure_base_150mm_corrected.stl")
cq.exporters.export(lid, f"{out_dir}/enclosure_lid_150mm_corrected.stl")

# Save this exact generator beside the CAD outputs.
import shutil
shutil.copy(__file__, f"{out_dir}/enclosure_generator_150mm_v3.py")

for n, s in (("base", base), ("lid", lid)):
    bb = s.val().BoundingBox()
    print(n, "bbox", round(bb.xlen, 2), round(bb.ylen, 2), round(bb.zlen, 2),
          "valid", s.val().isValid(), "volume", round(s.val().Volume(), 1))
print("OUT", OUT, "PCB_OFF", PCB_OFF, "TFT 56x35 bosses", "window", round(TFT_WINDOW_W,2), "x", round(TFT_WINDOW_H,2), "slope angle", round(ANG, 2), "switch right y", SWITCH_Y, "servo centre", (sx, round(sy,1)))