#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
150 x 150 mm two-part enclosure (base + lid) for a 100 x 100 mm ESP32-S3 board.

    python3 enclosure_v12.py --check     design-rule check only (NO CadQuery needed)
    python3 enclosure_v12.py             check + build + export STEP / STL

Contents
    - 100 x 100 mm custom PCB on 4 posts (3 real M3 holes + 1 dummy post)
    - 1.8" 128x160 ST7735 TFT on the sloped front face
    - MG996R servo in a glued pocket on the flat roof (body on the roof, ears on the walls)
    - 30 mm speaker in the left wall
    - 12 mm key switch in the right wall  (mirror image of the speaker)
    - USB-C key opening in the rear wall

Everything above the Helpers marker is plain data + arithmetic,
so make_drawings.py can import the same numbers that the CAD is built from.

Why a design-rule check is built in
    v8..v10 each shipped a geometry fault that no one saw until the drawing was
    printed (TFT hole pitch, then the key switch sitting on top of the vent
    slots).  Every feature is now declared once, as data, and check() verifies
    every feature against every other feature and against the JLC3DP rules
    BEFORE any CAD is built.  A clash aborts the script instead of quietly
    exporting a broken STL.
"""

import math

# ============================================================================
# 1.  PROCESS  --  pick the JLC3DP process; every clearance follows from it
# ============================================================================
# JLC3DP published rules (2024/25):
#   FDM   min wall 2.0-2.5   assembly clearance 0.5   detail >=1.0   hole tol +-0.4
#   MJF   min wall 1.0       assembly clearance 0.4   detail >=0.5   hole tol +-0.3
#   SLA   min wall 1.0       assembly clearance 0.2   detail >=0.4   hole tol +-0.3
# FDM is the strictest, so a part designed for FDM also prints in MJF and SLA.
PROCESS = "SLA"                      # "FDM" | "MJF" | "SLA"   (ordered: SLA, 9600 resin)

_RULES = {                           # clear, min_wall, min_detail, hole_tol
    "FDM": (0.50, 2.00, 1.00, 0.40),
    "MJF": (0.40, 1.00, 0.50, 0.30),
    "SLA": (0.20, 1.00, 0.40, 0.30),
}
FIT_CLEAR, MIN_WALL, MIN_DETAIL, HOLE_TOL = _RULES[PROCESS]
BUILD_MAX = (250.0, 250.0, 300.0)    # JLC3DP FDM build volume (PLA / ASA)
JLC_THIN = 1.2                       # JLC thin-wall heatmap: walls below this show yellow / red

# ============================================================================
# 2.  PARAMETERS
# ============================================================================
# ---- shell ----------------------------------------------------------------
OUT = 150.0                 # outer footprint 150 x 150
PCB = 100.0                 # main PCB 100 x 100
WALL = 3.0
CLEAR = 1.0
PCB_OFF = (OUT - PCB) / 2.0 # PCB centred -> 25 mm from each outer edge
R_OUT = 8.0                 # corner radius
FLOOR = 2.5
STANDOFF_H = 5.0
PCB_T = 1.6

RAISE = 16.0                # the whole box is 16 mm taller so the speaker fits in the base
HF = 30.0 + RAISE           # 46.0  front height
HB = 42.0 + RAISE           # 58.0  rear height
Y_S0, Y_S1 = 16.0, 54.0     # the sloped front face runs between these two y values
Z_SPLIT = 24.0 + RAISE      # 40.0  base / lid joint

# ---- base / lid joint: flat butt joint + inner locating rail ---------------
RAIL_T = 2.0
RAIL_H = 2.0
RAIL_GAP = max(FIT_CLEAR + 0.2, 0.5)   # >= 0.5 per side: JLC SLA is +-0.3% over 100 mm
                                       # (+-0.45 at 150 mm), FDM shrinks / warps a little

# ---- main PCB mounting ----------------------------------------------------
# From Development_pcb.kicad_pcb: board outline x 180.34..280.34, y 62.23..162.23,
# three M3 holes (3.2 mm); the fourth corner has no hole.
KICAD_HOLES = [(5.0, 95.0), (5.08, 5.08), (95.0, 5.0)]
KICAD_EMPTY_CORNER = (95.0, 95.0)
PCB_ROT180 = True           # USB end of the DevKit and the power parts face the rear


def _kicad_to_box(p):
    return (PCB - p[0], p[1]) if PCB_ROT180 else (p[0], PCB - p[1])


HOLES = [_kicad_to_box(p) for p in KICAD_HOLES]
DUMMY = _kicad_to_box(KICAD_EMPTY_CORNER)

# Everything on the main PCB, as axis-aligned keep-out boxes in BOX coordinates,
# already grown by 3 mm around the pad extents to stand in for the component body.
# (ref, x0, y0, x1, y1, height above the top of the PCB)
PCB_PARTS = [
    ("J7",  115.64,  43.59, 121.64,  57.09, 11.5),   # JST-XH 4
    ("J3",  115.64,  93.16, 121.64, 101.66, 11.5),   # JST-XH 2
    ("J8",  109.30, 108.36, 117.84, 114.36, 11.0),   # screw terminal (power in)
    ("D2",  100.41,  81.69, 116.57,  87.69,  3.2),
    ("C9",  110.31,  93.14, 116.31, 101.64,  7.0),
    ("J6",   91.56,  35.97, 115.06,  41.97, 11.5),   # JST-XH 8 -> TFT
    ("C13", 106.51,  25.80, 112.51,  34.30,  7.0),
    ("C3",  105.49,  71.84, 111.49,  81.34, 12.0),
    ("C4",  105.49,  59.14, 111.49,  68.64, 12.0),
    ("C10", 104.31,  93.16, 110.31, 101.66, 12.0),
    ("F1",   96.50, 108.36, 107.60, 115.56, 10.0),
    ("C14", 100.16,  25.79, 106.16,  34.29, 12.0),
    ("J2",   94.06,  70.26, 100.06,  81.26, 11.5),
    ("J1",   94.06,  57.56, 100.06,  68.56, 11.5),
    ("C7",   94.06,  81.69, 100.06,  91.19, 12.0),
    ("D1",   88.98, 108.36,  94.98, 124.52, 12.0),
    ("C8",   87.71,  81.69,  93.71,  90.19,  7.0),
    ("U1",   58.50,  30.89,  89.90,  90.23, 25.0),   # ESP32-S3-DevKitC-1 in its socket
    ("C1",   82.00, 108.36,  88.00, 116.86,  7.0),
]

# ---- enclosure screws (4 corners, independent of the PCB) -----------------
ENC_SCREWS = [(12.0, 12.0), (OUT - 12.0, 12.0),
              (12.0, OUT - 12.0), (OUT - 12.0, OUT - 12.0)]
# No foot recesses: the underside of the base is one flat surface.
FEET = []
FOOT_D = 9.0
FOOT_DEPTH = 1.0
# Heat-set inserts need a plastic that melts (PLA / PETG / nylon). SLA resin is a thermoset
# that softens at ~56 C and scorches or cracks instead, so resin parts use self-tapping M3.
ENC_SCREW_MODE = "selftap" if PROCESS == "SLA" else "insert"
SCREW_HOLE = 3.5                # M3 clearance in the lid
ENC_PILOT_D = 2.6               # M3 self-tapping pilot in the base
ENC_INSERT_D = 4.1              # M3 x 5.7 heat-set insert (OD 4.6)
ENC_INSERT_DEPTH = 7.0
SCREW_BOSS_R = 4.8
SCREW_COUNTERBORE_R = 3.3
SCREW_CB_DEPTH = 2.0

# ---- 1.8" 128x160 ST7735 TFT ---------------------------------------------
# Caliper (yours) + the supplier mechanical drawing.  These modules differ
# between suppliers, so these are YOUR module's numbers, not a generic datasheet.
# Vendor PCB drawing of YOUR module (1.8'128X160 RGB_TFT, 8-pin GND VDD SCL SDA RST DC CS BLK):
#   PCB 55.00 x 34.70, hole centres 50.93 x 31.00, header on a short edge, in line with the holes.
TFT_BODY_W = 55.00          # PCB long side   (drawing 55.00, your caliper 55.62)
TFT_BODY_H = 34.70          # PCB short side  (drawing 34.70, your caliper 34.83)
TFT_HOLE_SP_L = 50.93       # hole centre to centre, long side   (drawing)
TFT_HOLE_SP_S = 31.00       # hole centre to centre, short side  (drawing)
TFT_HOLES_OUTER_L = 53.6    # your caliper, outer edge to outer edge - cross-check only
TFT_HOLES_OUTER_S = 32.5
TFT_HOLE_INNER_D = 2.5      # hole in the module PCB (M2 screw)
# Short side: the drawing says 31.00, your caliper (32.5 outer - 2.5 hole) says 30.0.
# The pilots are placed at the midpoint.  An M2 screw in the 2.5 mm module hole has
# 0.25 mm of play per side, so it passes whichever number is right, and a self-tapping
# screw bites the plastic fine 0.25 mm off the pilot centre.
TFT_HOLE_SP_S_CAL = TFT_HOLES_OUTER_S - TFT_HOLE_INNER_D           # 30.0
TFT_HOLE_SP_S_USED = (TFT_HOLE_SP_S + TFT_HOLE_SP_S_CAL) / 2.0      # 30.5
TFT_PIN_ROW = (TFT_BODY_W - TFT_HOLE_SP_L) / 2.0   # header row sits in line with the holes
TFT_HOLE_PAD_D = 5.2        # boss outer diameter (1.8 mm ring around the pilot)
TFT_ACTIVE_W = 35.04        # active (lit) area
TFT_ACTIVE_H = 28.03
TFT_GLASS_W = 45.83         # glass footprint, long side   (supplier drawing)
TFT_GLASS_H = 33.00         # glass footprint, short side
TFT_GLASS_T = 2.25          # glass stack height above the front of the module PCB
TFT_GLASS_CLR = 0.45        # clearance kept around the glass (also sets the boss flats).
                            # The pilot centre is only 2.55 mm from the glass edge, so this
                            # is the trade-off: 0.45 leaves 1.30 mm of boss wall (JLC flags
                            # < 1.2). If the glass rubs a boss flat, shave the flat with a knife.
TFT_LIT_UNCERT = 8.0 + TFT_ACTIVE_W + 13.0 - TFT_BODY_W   # 1.04: the 8 / 13 mm edge numbers
                                                         # overshoot the 55 mm PCB by this much
TFT_WINDOW_W = TFT_ACTIVE_W + 2.00 + TFT_LIT_UNCERT   # 1.0 mm margin per side + the uncertainty
TFT_WINDOW_H = TFT_ACTIVE_H + 2.00
TFT_PILOT_D = 1.7 if PROCESS == "SLA" else 1.6   # M2 self-tap pilot: resin is stiffer
                                                # than PLA, so 0.1 mm more to avoid cracking
TFT_BOSS_H = TFT_GLASS_T + 0.25      # 0.25 mm so a tall print can never crush the glass
TFT_PILOT_INTO_WALL = 1.0   # pilot continues this far into the wall (wall stays closed)
# The lit area is 8.0 mm from the pin edge and 13.0 mm from the other edge, so it is
# not centred on the module PCB.  8 + 35.04 + 13 = 56.04, i.e. 1.04 mm more than the
# 55.00 PCB, so its exact position is uncertain by ~1 mm.  The lit area is placed at
# the midpoint of the two readings ((13 - 8) / 2 = 2.5 mm off centre) and the window
# is made 1.04 mm wider, so it shows the whole lit area whichever reading is right.
TFT_ACTIVE_OFFSET = (13.0 - 8.0) / 2.0      # 2.50
TFT_PIN_SIDE = +1           # +1 pin header on the right seen from the front, -1 left
TFT_BEZEL = 2.0             # cosmetic recess around the window
TFT_BEZEL_DEPTH = 1.0       # >= MIN_DETAIL

# ---- rear wall USB-C ------------------------------------------------------
USB_X = 75.0
USB_Z = 14.0
USB_SLOT_W = 8.25 + 1.0     # metal plug tip 8.25 x 2.40 + 0.5 per side
USB_SLOT_H = 2.40 + 1.0
USB_POCKET_W = 13.0         # plug overmold
USB_POCKET_H = 5.1 + 1.0
USB_POCKET_R = 1.5
USB_POCKET_DEPTH = WALL - 1.5
# Step inside the rear wall for the small PCB with the USB-C socket (+ / - wires).
# The PCB is hot-glued onto it. The step is deliberately LOWER than a dry fit by
# USB_GLUE_GAP: the hot glue fills that gap, so a thick glue layer can never push the
# socket above the opening. Plug a cable in through the wall first, then glue -
# the plug holds the socket exactly in line while the glue sets.
USB_MOD_PCB_T = 1.6         # thickness of the small USB-C PCB
USB_SOCKET_H = 3.3          # height of a top-mount USB-C socket (3.2-3.5)
USB_MOD_CENTER_H = USB_MOD_PCB_T + USB_SOCKET_H / 2.0     # 3.25: PCB underside -> socket centre
USB_GLUE_GAP = 1.0          # room for the hot glue under the PCB
USB_SHELF_W = 20.0          # across (x)
USB_SHELF_D = 20.0          # out from the inner rear wall (y) - long enough for the wire pads
USB_SHELF_TOP = USB_Z - USB_MOD_CENTER_H - USB_GLUE_GAP  # 9.75
USB_SHELF_H = USB_SHELF_TOP - FLOOR                       # 7.25 above the inside floor

# ---- right wall key switch (mirror image of the speaker) ------------------
SWITCH_D = 12.8             # 12 mm thread + FDM hole tolerance (holes print ~0.4 small)
SWITCH_Y = OUT / 2.0        # 75.0 - centred, exactly opposite the speaker
SWITCH_Z = FLOOR + 30.0 / 2.0 + 1.0 + 1.5        # 20.0 - same height as the speaker centre
                                                 # (kept equal to SPK_Z by the checker)
SWITCH_BODY_D = 18.0        # keep-out diameter of the body behind the panel
SWITCH_BODY_L = 26.0        # body length behind the panel (yours: 25-26)
SWITCH_NUT_D = 18.0         # flat area needed on the inside face for the nut

# ---- left wall speaker ----------------------------------------------------
# A full round ring with a solid lower part.  The bore itself is the fit: 0.3 mm
# clearance per side centres the 30 mm speaker, and a bead of hot glue around the
# back of the rim holds it.  No ribs, no thin features - nothing for the thin-wall
# check to flag and no fragile booleans in the CAD.
SPK_D = 30.0                # caliper-verified
SPK_BORE_CLR = 0.3          # per side: SLA +-0.2 -> 0.1..0.5, FDM -0.4 still slides in
SPK_CLEAR = SPK_BORE_CLR
SPK_RING_H = 6.0            # bore depth: the speaker sits fully inside it
SPK_RING_T = 2.5
SPK_Y = OUT / 2.0
SPK_FLOOR_T = 2.2           # solid material between the bore and the floor
SPK_Z = FLOOR + SPK_D / 2.0 + SPK_BORE_CLR + SPK_FLOOR_T     # 20.0
SPK_GRILLE_D = 3.0
SPK_GRILLE = ((0.0, 1), (5.5, 6), (10.0, 12))

# ---- side ventilation -----------------------------------------------------
# Left and right walls carry the SAME two blocks, so the box is symmetric and
# neither the speaker (left) nor the key switch (right) sits on a slot.
VENT_SLOT_W = 2.4
VENT_SLOT_H = 14.0
VENT_SLOT_Z = 14.0
VENT_PITCH = 5.5
VENT_Y0 = 18.0
VENT_BLOCKS = [(18.0, 48.0), (100.0, OUT - 14.0)]

# ---- MG996R servo mount on the flat roof -------------------------------
# Simple glued mount: the servo body stands on the solid roof inside a pocket, the
# ears rest on top of the two end walls, and a few dabs of hot glue (ears + body)
# hold it.  No screws, no clamp bars, no ear holes needed.
#   MG996R drawing: 40.3 body length, 53.6 overall with the ears, 20 body width,
#   36.6 case height, 47.6 overall height, 26.6 from the base to the ear underside.
SERVO_BODY_L = 40.7         # drawing 40.3, text spec 40.7 -> use the larger
SERVO_BODY_W = 20.0         # drawing 20, your caliper 20.0
SERVO_BODY_H = 36.6         # case height
SERVO_TOTAL_L = 53.6        # ear tip to ear tip
SERVO_TOTAL_H = 47.6        # base to the top of the output spline
SERVO_EAR_Z = 26.6          # base of the case -> underside of the ears
SERVO_EAR_T = 3.5           # ear thickness
SERVO_EAR_PROJ = (SERVO_TOTAL_L - SERVO_BODY_L) / 2.0        # 6.45 per end

SERVO_FIT = 1.20            # total slack at the rib tips across the body (FDM +-0.4, glued)
SERVO_RIB_H = 1.20          # guide ribs on the two long walls
SERVO_SIDE_WALL = 2.5
SERVO_END_WALL = 8.0        # the ears rest on top of these
SERVO_LEDGE_H = 26.0        # wall height above the roof. LOWER than SERVO_EAR_Z, so the
                            # body always stands on the roof; the small gap under the
                            # ears is filled with hot glue.
SERVO_WIRE_W = 13.0         # lead slot: FULL HEIGHT through the rear end wall, open at the
                            # top and through the roof - the plug never has to be threaded
SERVO_PLUG_W = 8.5          # 3-pin servo plug (JR/Futaba) incl. latch, with margin
SERVO_PLUG_T = 4.5
SERVO_CX = OUT / 2.0
SERVO_CY = 95.0
# roof cable slot right behind the mount (servo lead, 2nd servo, camera, laser, LiDAR)
SERVO_HOLE_L, SERVO_HOLE_W = 32.0, 15.0
SERVO_HOLE_CY = 135.5

# ---- roof ventilation -----------------------------------------------------
ROOF_VENT_LEN = 40.0
ROOF_VENT_OFFS = tuple(28.0 + 5.5 * k for k in range(6))

# ---- derived servo geometry (used by the checker and the drawings) -------
SRV_POCKET_X = SERVO_BODY_W + SERVO_FIT + 2 * SERVO_RIB_H
SRV_POCKET_Y = SERVO_BODY_L + 1.4             # 0.7 per end: FDM +-0.4 and servo variation
SRV_POCKET_Y0 = SERVO_CY - SRV_POCKET_Y / 2.0
SRV_POCKET_Y1 = SERVO_CY + SRV_POCKET_Y / 2.0
SRV_SIDE_X = SRV_POCKET_X + 2 * SERVO_SIDE_WALL
SRV_OUT_Y0 = SRV_POCKET_Y0 - SERVO_END_WALL
SRV_OUT_Y1 = SRV_POCKET_Y1 + SERVO_END_WALL
SRV_LEDGE_Z = HB + SERVO_LEDGE_H            # top of the pocket walls
SRV_TOP_Z = SRV_LEDGE_Z                     # top of the mount
SRV_BODY_Z0 = HB                            # the servo body stands on the roof
SRV_EAR_GAP = SERVO_EAR_Z - SERVO_LEDGE_H   # glue gap under the ears
SRV_SHAFT_Z = SRV_BODY_Z0 + SERVO_TOTAL_H   # top of the output spline

# ---- derived slope geometry ----------------------------------------------
SLOPE_LEN = math.hypot(Y_S1 - Y_S0, HB - HF)
UX, UZ = (Y_S1 - Y_S0) / SLOPE_LEN, (HB - HF) / SLOPE_LEN
NX, NZ = -UZ, UX
ANG = math.degrees(math.atan2(HB - HF, Y_S1 - Y_S0))

# ---- derived TFT geometry (local slope coords, x across, y up the slope) --
TFT_PCB_DX = -TFT_PIN_SIDE * TFT_ACTIVE_OFFSET        # module centre, window centred on the lit area
TFT_HOLE_X = (-TFT_HOLE_SP_L / 2.0 + TFT_PCB_DX, TFT_HOLE_SP_L / 2.0 + TFT_PCB_DX)
TFT_HOLE_Y = (-TFT_HOLE_SP_S_USED / 2.0, TFT_HOLE_SP_S_USED / 2.0)
TFT_PTS = [(hx, hy) for hx in TFT_HOLE_X for hy in TFT_HOLE_Y]
TFT_KO_W = TFT_GLASS_W + 2 * TFT_GLASS_CLR            # glass keep-out, also cuts the boss flats
TFT_KO_H = TFT_GLASS_H + 2 * TFT_GLASS_CLR

# ---------------- Helpers ----------------
# (make_drawings.py reads everything above this line)

CX = OUT / 2.0


def vent_ys(y0, y1):
    ys, y = [], y0
    while y <= y1 + 1e-9:
        ys.append(y)
        y += VENT_PITCH
    return ys


def inner_slope_y(z):
    py, pz = Y_S0 - NX * WALL, HF - NZ * WALL
    return py + UX * ((z - pz) / UZ)


def _ov(a0, a1, b0, b1):
    """overlap of two 1-D intervals; negative = gap"""
    return min(a1, b1) - max(a0, b0)


# ============================================================================
# 3.  DESIGN RULE CHECK  (pure Python - runs without CadQuery)
# ============================================================================
def check(verbose=True):
    err, warn, note = [], [], []

    def E(m): err.append(m)

    def W(m): warn.append(m)

    def N(m): note.append(m)

    # ---- 3.1 JLC3DP build rules -----------------------------------------
    base_bb = (OUT, OUT, Z_SPLIT)
    lid_bb = (OUT, OUT, SRV_TOP_Z - Z_SPLIT)   # includes the servo mount
    for nm, bb in (("base", base_bb), ("lid", lid_bb)):
        if any(d > m for d, m in zip(sorted(bb), sorted(BUILD_MAX))):
            E("%s %s mm exceeds the %s build volume %s" % (nm, bb, PROCESS, BUILD_MAX))
    if WALL < MIN_WALL:
        E("wall %.2f < %s minimum %.2f" % (WALL, PROCESS, MIN_WALL))
    if FLOOR < MIN_WALL:
        E("floor %.2f < %s minimum %.2f" % (FLOOR, PROCESS, MIN_WALL))
    for nm, v in (("speaker ring wall", SPK_RING_T), ("servo rib tip", 1.2),
                  ("lid wall at the rail groove", WALL - RAIL_GAP),
                  ("USB-C pocket web", WALL - USB_POCKET_DEPTH)):
        if v < JLC_THIN:
            E("%s %.2f mm < JLC thin-wall limit %.1f" % (nm, v, JLC_THIN))
    if WALL - RAIL_GAP < MIN_WALL:
        E("lid wall at the rail groove is only %.2f mm" % (WALL - RAIL_GAP))
    N("base/lid rail: %.1f mm clearance per side, lid wall %.1f mm at the groove"
      % (RAIL_GAP, WALL - RAIL_GAP))
    if WALL - USB_POCKET_DEPTH < 1.5:
        E("USB-C pocket leaves a %.2f mm web (needs >= 1.5)" % (WALL - USB_POCKET_DEPTH))
    for nm, v in (("rail", RAIL_T), ("bezel depth", TFT_BEZEL_DEPTH), ("servo rib", SERVO_RIB_H),
                  ("vent slot", VENT_SLOT_W)):
        if v < MIN_DETAIL:
            E("%s %.2f < %s minimum detail %.2f" % (nm, v, PROCESS, MIN_DETAIL))

    # ---- 3.1b base and lid screw positions -------------------------------
    # (both parts are built from the same ENC_SCREWS list, and the lid is exported
    #  moved in z only - never mirrored - so the holes coincide by construction)
    for (fx, fy), (sx_, sy_) in zip(FEET, ENC_SCREWS):
        if math.hypot(fx - sx_, fy - sy_) > 0.01:
            W("foot recess at (%.1f,%.1f) is not concentric with screw (%.1f,%.1f)"
              % (fx, fy, sx_, sy_))
    if FOOT_DEPTH > FLOOR - 1.2:
        E("foot recess %.1f deep leaves less than 1.2 mm of floor" % FOOT_DEPTH)
    N("corner screws: base %s and lid hole share the same 4 centres %s"
      % ("pilot" if ENC_SCREW_MODE == "selftap" else "insert",
         ", ".join("(%.0f,%.0f)" % p for p in ENC_SCREWS)))
    if not FEET:
        N("base underside: one flat surface, no recesses")

    # ---- 3.2 side walls: every feature against every other feature -------
    # (name, y0, y1, z0, z1, kind)   kind: "hole" = cut through the wall
    #                                      "solid" = material / keep-out area
    def slot_boxes():
        out = []
        for a, b in VENT_BLOCKS:
            for y in vent_ys(a, b):
                out.append(("vent y=%.1f" % y, y - VENT_SLOT_W / 2, y + VENT_SLOT_W / 2,
                            VENT_SLOT_Z - VENT_SLOT_H / 2, VENT_SLOT_Z + VENT_SLOT_H / 2,
                            "hole", "vent%.1f" % y))
        return out

    spk_r = SPK_D / 2 + SPK_CLEAR
    walls = {
        "left": slot_boxes() + [
            ("speaker bore", SPK_Y - spk_r, SPK_Y + spk_r,
             SPK_Z - spk_r, SPK_Z + spk_r, "hole", "spk"),
            ("speaker ring", SPK_Y - spk_r - SPK_RING_T, SPK_Y + spk_r + SPK_RING_T,
             SPK_Z - spk_r - SPK_RING_T, SPK_Z + spk_r + SPK_RING_T, "solid", "spk")],
        "right": slot_boxes() + [
            ("key switch hole", SWITCH_Y - SWITCH_D / 2, SWITCH_Y + SWITCH_D / 2,
             SWITCH_Z - SWITCH_D / 2, SWITCH_Z + SWITCH_D / 2, "hole", "sw"),
            ("key switch nut face", SWITCH_Y - SWITCH_NUT_D / 2, SWITCH_Y + SWITCH_NUT_D / 2,
             SWITCH_Z - SWITCH_NUT_D / 2, SWITCH_Z + SWITCH_NUT_D / 2, "solid", "sw")],
        "rear": [("USB-C pocket", USB_X - USB_POCKET_W / 2, USB_X + USB_POCKET_W / 2,
                  USB_Z - USB_POCKET_H / 2, USB_Z + USB_POCKET_H / 2, "hole", "usb")],
    }
    for wall, feats in walls.items():
        for i in range(len(feats)):
            for j in range(i + 1, len(feats)):
                n1, a0, a1, b0, b1, k1, g1 = feats[i]
                n2, c0, c1, d0, d1, k2, g2 = feats[j]
                if g1 == g2:
                    continue                      # a hole inside its own solid feature
                oy, oz = _ov(a0, a1, c0, c1), _ov(b0, b1, d0, d1)
                if oy > 0 and oz > 0:
                    E("%s wall: '%s' overlaps '%s' by %.2f x %.2f mm"
                      % (wall, n1, n2, oy, oz))
                elif oy > -1.5 and oz > -1.5 and (oy > 0 or oz > 0):
                    W("%s wall: '%s' and '%s' are only %.2f mm apart"
                      % (wall, n1, n2, max(-oy, -oz)))
        # holes must stay in the base, clear of the split line and the floor
        for n1, a0, a1, b0, b1, k1, g1 in feats:
            if k1 != "hole":
                continue
            if b1 > Z_SPLIT - 1.5:
                E("%s wall: '%s' reaches z=%.2f, within 1.5 mm of the split at %.1f"
                  % (wall, n1, b1, Z_SPLIT))
            if b0 < FLOOR:
                E("%s wall: '%s' reaches z=%.2f, below the floor at %.1f"
                  % (wall, n1, b0, FLOOR))
            if a0 < R_OUT + 1.0 or a1 > OUT - R_OUT - 1.0:
                W("%s wall: '%s' runs into the corner radius" % (wall, n1))

    # the switch hole itself must sit inside the flat area kept for the nut
    if SWITCH_D + 2.0 > SWITCH_NUT_D:
        W("key switch: only %.1f mm of flat wall around the hole" % (SWITCH_NUT_D - SWITCH_D))

    # ---- 3.3 things that reach INTO the box vs the PCB -------------------
    z_pcb = FLOOR + STANDOFF_H + PCB_T
    # key switch body: a cylinder on the x axis
    sw_x0 = OUT - WALL - SWITCH_BODY_L
    for ref, x0, y0, x1, y1, h in PCB_PARTS:
        if _ov(x0, x1, sw_x0, OUT) > 0 and \
           _ov(y0, y1, SWITCH_Y - SWITCH_BODY_D / 2, SWITCH_Y + SWITCH_BODY_D / 2) > 0 and \
           _ov(z_pcb, z_pcb + h, SWITCH_Z - SWITCH_BODY_D / 2, SWITCH_Z + SWITCH_BODY_D / 2) > 0:
            E("key switch body hits %s on the PCB" % ref)
    # how close did it get?
    cand = [(abs(SWITCH_Y - (y1 if y1 < SWITCH_Y else y0)), ref)
            for ref, x0, y0, x1, y1, h in PCB_PARTS
            if _ov(x0, x1, sw_x0, OUT) > 0 and z_pcb + h > SWITCH_Z - SWITCH_BODY_D / 2]
    if cand:
        d_, r_ = min(cand)
        N("key switch body: nearest PCB part is %s, %.1f mm away in y" % (r_, d_))
    # how big may the key switch body be? (distance from its axis to everything near it)
    free = []
    for ref, x0, y0, x1, y1, h in PCB_PARTS:
        if _ov(x0, x1, sw_x0, OUT) > 0:
            dy = max(0.0, y0 - SWITCH_Y, SWITCH_Y - y1)
            dz = max(0.0, z_pcb - SWITCH_Z, SWITCH_Z - (z_pcb + h))
            free.append((math.hypot(dy, dz), ref))
    if _ov(PCB_OFF, PCB_OFF + PCB, sw_x0, OUT) > 0:       # the board itself under the body
        free.append((SWITCH_Z - z_pcb, "the PCB surface"))
    free.append((SWITCH_Z - FLOOR, "the floor"))
    free.append((Z_SPLIT - RAIL_H - SWITCH_Z, "the lid rail"))
    r_free, what = min(free)
    if r_free < SWITCH_BODY_D / 2.0:
        E("key switch body (dia %.1f) hits %s" % (SWITCH_BODY_D, what))
    N("key switch: the body may be up to dia %.1f mm and %.1f mm long - the limit is %s"
      % (2 * r_free, SWITCH_BODY_L, what))
    tz = 6.0                     # terminal / wire zone behind the body: dia 12 around the axis
    behind = [(sw_x0 - x1, ref) for ref, x0, y0, x1, y1, h in PCB_PARTS
              if x1 < sw_x0 and _ov(y0, y1, SWITCH_Y - tz, SWITCH_Y + tz) > 0
              and z_pcb + h > SWITCH_Z - tz]
    if behind:
        d_b, r_b = min(behind)
        N("key switch: %.1f mm free behind the body for its terminals, then %s (top z=%.1f); "
          "bend the wires UP after soldering" % (d_b, r_b,
                                                 z_pcb + [p for p in PCB_PARTS if p[0] == r_b][0][5]))
    # speaker holder
    spk_x1 = WALL + SPK_RING_H
    for ref, x0, y0, x1, y1, h in PCB_PARTS:
        if x0 < spk_x1:
            E("speaker holder hits %s on the PCB" % ref)
    # PCB must fit between the walls
    if PCB_OFF < WALL + 1.0:
        E("PCB has only %.1f mm to the wall" % PCB_OFF)

    # ---- 3.3b USB-C module step ---------------------------------------
    sh_x0, sh_x1 = USB_X - USB_SHELF_W / 2, USB_X + USB_SHELF_W / 2
    sh_y0, sh_y1 = OUT - WALL - USB_SHELF_D, OUT - WALL
    if sh_y0 < PCB_OFF + PCB + 1.0:
        E("USB-C step reaches the main PCB (y=%.1f vs PCB edge %.1f)" % (sh_y0, PCB_OFF + PCB))
    for ref, x0, y0, x1, y1, h in PCB_PARTS:
        if _ov(x0, x1, sh_x0, sh_x1) > 0 and _ov(y0, y1, sh_y0, sh_y1) > 0:
            E("USB-C step hits %s" % ref)
    for hx_, hy_ in HOLES + [DUMMY]:
        px_, py_ = hx_ + PCB_OFF, hy_ + PCB_OFF
        if _ov(px_ - 3.5, px_ + 3.5, sh_x0, sh_x1) > 0 and _ov(py_ - 3.5, py_ + 3.5, sh_y0, sh_y1) > 0:
            E("USB-C step hits the PCB post at (%.1f,%.1f)" % (px_, py_))
    if USB_SHELF_TOP > USB_Z - USB_SLOT_H / 2:
        E("USB-C step top is above the bottom of the opening")
    if USB_SHELF_TOP < FLOOR + 1.0:
        E("USB-C step is too low")
    if USB_GLUE_GAP < 0.5 or USB_GLUE_GAP > 2.0:
        W("USB-C glue gap %.1f mm: hot glue wants ~0.5-2.0 mm" % USB_GLUE_GAP)
    N("USB-C step: %.0f x %.0f mm, %.2f mm above the inside floor (top z=%.2f). PCB %.1f + "
      "socket %.1f puts the socket centre at z=%.1f with %.1f mm of hot glue under the PCB; "
      "%.1f mm clear of the main PCB"
      % (USB_SHELF_W, USB_SHELF_D, USB_SHELF_H, USB_SHELF_TOP, USB_MOD_PCB_T, USB_SOCKET_H,
         USB_SHELF_TOP + USB_GLUE_GAP + USB_MOD_CENTER_H, USB_GLUE_GAP, sh_y0 - (PCB_OFF + PCB)))

    # ---- 3.4 TFT on the slope -------------------------------------------
    half = SLOPE_LEN / 2.0
    if TFT_BODY_H / 2.0 > half - 2.0:
        E("TFT module is %.1f mm across the slope; the slope is only %.1f mm"
          % (TFT_BODY_H, SLOPE_LEN))
    N("TFT module leaves %.1f mm at each end of the slope" % (half - TFT_BODY_H / 2.0))
    # window must stay inside the glass
    g0, g1 = TFT_PCB_DX - TFT_GLASS_W / 2, TFT_PCB_DX + TFT_GLASS_W / 2
    if -TFT_WINDOW_W / 2 < g0 or TFT_WINDOW_W / 2 > g1:
        E("window is wider than the glass")
    if TFT_WINDOW_H / 2 > TFT_GLASS_H / 2:
        E("window is taller than the glass")
    N("window margin on the glass: %.2f / %.2f mm long side, %.2f mm short side"
      % (-TFT_WINDOW_W / 2 - g0, g1 - TFT_WINDOW_W / 2,
         (TFT_GLASS_H - TFT_WINDOW_H) / 2))
    # window must stay inside the lit area + margin, and the lit area inside the window
    if TFT_WINDOW_W < TFT_ACTIVE_W or TFT_WINDOW_H < TFT_ACTIVE_H:
        E("window is smaller than the lit area")
    # bosses: cut back by the glass keep-out, so check what is left
    ko0, ko1 = TFT_PCB_DX - TFT_KO_W / 2, TFT_PCB_DX + TFT_KO_W / 2
    for hx in TFT_HOLE_X:
        b0, b1 = hx - TFT_HOLE_PAD_D / 2, hx + TFT_HOLE_PAD_D / 2
        if hx < TFT_PCB_DX:                      # relief cuts this boss on its right
            left, thin = ko0 - b0, ko0 - (hx + TFT_PILOT_D / 2)
        else:                                    # ... and this one on its left
            left, thin = b1 - ko1, (hx - TFT_PILOT_D / 2) - ko1
        if thin < max(MIN_DETAIL, JLC_THIN):
            E("TFT boss at x=%+.2f leaves only %.2f mm of wall beside the pilot "
              "(needs >= %.1f)" % (hx, thin, max(MIN_DETAIL, JLC_THIN)))
        N("TFT boss at x=%+.2f: %.2f mm wide after the glass relief, %.2f mm of wall "
          "on the glass side" % (hx, left, thin))
    # the module must not be wider than the slope face
    if TFT_BODY_W / 2 + abs(TFT_PCB_DX) > (OUT - 2 * WALL) / 2:
        E("TFT module is wider than the inside of the box")
    # holes must land on the module, not off its edge
    m_edge = TFT_BODY_W / 2 - TFT_HOLE_SP_L / 2
    if m_edge < TFT_HOLE_INNER_D / 2 + 0.5:
        E("TFT screw holes sit only %.2f mm from the module edge" % m_edge)
    N("TFT screw holes sit %.2f mm (long) / %.2f mm (short) from the module edge"
      % (m_edge, TFT_BODY_H / 2 - TFT_HOLE_SP_S / 2))
    play = (TFT_HOLE_INNER_D - 2.0) / 2.0          # M2 screw in the module hole
    miss = abs(TFT_HOLE_SP_S_USED - TFT_HOLE_SP_S) / 2.0
    miss = max(miss, abs(TFT_HOLE_SP_S_USED - TFT_HOLE_SP_S_CAL) / 2.0)
    if miss > play + 1e-9:
        E("TFT short-side pilots are %.2f mm off one of the two readings; an M2 screw only "
          "has %.2f mm of play" % (miss, play))
    N("TFT short-side pilots at %.2f mm pitch (drawing %.2f, caliper %.2f): each screw is "
      "at most %.2f mm off, inside the %.2f mm play of an M2 in a %.1f mm hole"
      % (TFT_HOLE_SP_S_USED, TFT_HOLE_SP_S, TFT_HOLE_SP_S_CAL, miss, play, TFT_HOLE_INNER_D))
    for side, outer, sp in (("long", TFT_HOLES_OUTER_L, TFT_HOLE_SP_L),):
        if abs((outer - sp) - TFT_HOLE_INNER_D) > 0.4:
            W("TFT %s side: caliper %.1f outer-to-outer vs drawing pitch %.2f implies a "
              "%.2f mm hole (expected ~%.1f) - check with the printed template"
              % (side, outer, sp, outer - sp, TFT_HOLE_INNER_D))

    # ---- 3.4b TFT module back + header wires vs parts on the main PCB -----
    # module PCB back face sits WALL + boss + PCB thickness behind the outer slope;
    # allow 3 mm of parts on its back and 20 mm of header pins + straight Dupont
    # housings behind the header row (on the pin side, 2.54 mm wide).
    z_pcb_ = FLOOR + STANDOFF_H + PCB_T
    back_d = WALL + TFT_BOSS_H + 1.6 + 3.0
    hdr_d = WALL + TFT_BOSS_H + 1.6 + 20.0
    mx0 = CX + TFT_PCB_DX - TFT_BODY_W / 2.0
    mx1 = CX + TFT_PCB_DX + TFT_BODY_W / 2.0
    hx_ = CX + TFT_PCB_DX + TFT_PIN_SIDE * (TFT_BODY_W / 2.0 - TFT_PIN_ROW)
    worst = []
    for ref, x0, y0, x1, y1, h in PCB_PARTS:
        top = z_pcb_ + h
        for nm_, d_, xa, xb in (("module back", back_d, mx0, mx1),
                                ("header wires", hdr_d, hx_ - 1.5, hx_ + 1.5)):   # 2.54 mm housing
            if _ov(x0, x1, xa, xb) <= 0:
                continue
            for k in range(41):                       # sample along the slope
                sl = (SLOPE_LEN - TFT_BODY_H) / 2.0 + TFT_BODY_H * k / 40.0
                yy = Y_S0 + UX * sl - NX * d_
                zz = HF + UZ * sl - NZ * d_
                if y0 <= yy <= y1:
                    worst.append((zz - top, ref, nm_))
    if worst:
        g_, r_, n_ = min(worst)
        if g_ < 2.0:
            E("TFT %s comes within %.1f mm of %s on the PCB" % (n_, g_, r_))
        N("TFT module: the closest PCB part is %s, %.1f mm below the %s" % (r_, g_, n_))

    # ---- 3.5 roof: cradle vs vents vs cable slot vs lid screw posts ------
    roof = [("servo mount", SERVO_CX - SRV_SIDE_X / 2, SRV_OUT_Y0,
             SERVO_CX + SRV_SIDE_X / 2, SRV_OUT_Y1),
            ("cable slot", SERVO_CX - SERVO_HOLE_L / 2, SERVO_HOLE_CY - SERVO_HOLE_W / 2,
             SERVO_CX + SERVO_HOLE_L / 2, SERVO_HOLE_CY + SERVO_HOLE_W / 2)]
    for off in ROOF_VENT_OFFS:
        for s in (-1, 1):
            roof.append(("roof vent x=%+.1f" % (s * off),
                         CX + s * off - VENT_SLOT_W / 2, SERVO_CY - ROOF_VENT_LEN / 2,
                         CX + s * off + VENT_SLOT_W / 2, SERVO_CY + ROOF_VENT_LEN / 2))
    for x, y in ENC_SCREWS:
        if y > Y_S1:
            roof.append(("lid screw post (%.0f,%.0f)" % (x, y),
                         x - SCREW_BOSS_R, y - SCREW_BOSS_R, x + SCREW_BOSS_R, y + SCREW_BOSS_R))
    for i in range(len(roof)):
        for j in range(i + 1, len(roof)):
            n1, a0, b0, a1, b1 = roof[i]
            n2, c0, d0, c1, d1 = roof[j]
            ox, oy = _ov(a0, a1, c0, c1), _ov(b0, b1, d0, d1)
            if ox > 0 and oy > 0:
                E("roof: '%s' overlaps '%s' by %.2f x %.2f mm" % (n1, n2, ox, oy))
            elif ox > -2.0 and oy > -2.0 and (ox > 0 or oy > 0):
                W("roof: '%s' and '%s' are only %.2f mm apart" % (n1, n2, max(-ox, -oy)))
    for n1, a0, b0, a1, b1 in roof:
        if b0 < Y_S1 + 1.0:
            E("roof: '%s' runs off the front of the flat roof" % n1)
        if b1 > OUT - WALL - 1.0:
            E("roof: '%s' runs into the rear wall" % n1)
        if a0 < WALL + 1.0 or a1 > OUT - WALL - 1.0:
            E("roof: '%s' runs into a side wall" % n1)

    # ---- 3.6 servo mount (glued) ----------------------------------------
    tip_gap = SRV_POCKET_X - 2 * SERVO_RIB_H
    if tip_gap < SERVO_BODY_W:
        E("servo pocket grips %.2f mm across a %.2f mm body" % (tip_gap, SERVO_BODY_W))
    N("servo: %.2f mm at the rib tips for a %.1f mm body (%.2f mm per side); "
      "pocket %.1f x %.1f mm" % (tip_gap, SERVO_BODY_W, (tip_gap - SERVO_BODY_W) / 2,
                                 SRV_POCKET_X, SRV_POCKET_Y))
    # the body must stand on the roof; the ears must sit just above the wall tops
    if SRV_EAR_GAP < 0.2:
        E("pocket walls are %.1f mm high but the ears sit %.1f mm above the servo base: "
          "the ears would land first and the body would hang" % (SERVO_LEDGE_H, SERVO_EAR_Z))
    if SRV_EAR_GAP > 1.5:
        W("%.1f mm gap under the ears - a lot for hot glue" % SRV_EAR_GAP)
    if SERVO_END_WALL < SERVO_EAR_PROJ + 1.0:
        E("end wall %.1f mm is shorter than the %.2f mm ear" % (SERVO_END_WALL, SERVO_EAR_PROJ))
    N("servo body stands on the roof; ears sit %.1f mm above the wall tops (glue gap). "
      "The pocket encloses the lower %.1f mm of the %.1f mm case."
      % (SRV_EAR_GAP, SERVO_LEDGE_H, SERVO_BODY_H))
    N("top of the output spline: z=%.1f (%.1f mm above the roof)"
      % (SRV_SHAFT_Z, SRV_SHAFT_Z - HB))
    N("the pocket floor is the solid %.1f mm roof: there is no hole under the servo" % WALL)
    if SERVO_WIRE_W < SERVO_PLUG_W + 3.0:
        E("lead slot %.1f mm is too narrow for a %.1f mm servo plug" % (SERVO_WIRE_W, SERVO_PLUG_W))
    strip = (SERVO_BODY_W - SERVO_WIRE_W) / 2.0
    if strip < 3.0:
        E("the lead slot leaves only %.2f mm of wall under each side of the rear ear" % strip)
    N("servo lead: %.0f mm slot, full height through the rear end wall and the roof; a %.1f mm "
      "plug drops through with %.1f mm to spare. The rear ear rests on %.1f mm each side."
      % (SERVO_WIRE_W, SERVO_PLUG_W, SERVO_WIRE_W - SERVO_PLUG_W, strip))

    # ---- 3.7 speaker -----------------------------------------------------
    if SPK_Z + SPK_D / 2 + SPK_CLEAR + SPK_RING_T > Z_SPLIT - 1.0:
        E("speaker holder crosses the base/lid split")
    N("speaker holder top is %.1f mm below the split" %
      (Z_SPLIT - (SPK_Z + SPK_D / 2 + SPK_CLEAR + SPK_RING_T)))
    r_in_ = SPK_D / 2.0 + SPK_CLEAR
    if SPK_CLEAR < HOLE_TOL / 2.0 + 0.05:
        E("speaker bore clearance %.2f mm per side is too tight for %s (+-%.1f)"
          % (SPK_CLEAR, PROCESS, HOLE_TOL))
    if SPK_CLEAR > 0.6:
        W("speaker bore clearance %.2f mm per side - the speaker will not be centred" % SPK_CLEAR)
    if abs(SWITCH_Z - SPK_Z) > 0.01:
        W("key switch (z=%.2f) is no longer level with the speaker (z=%.2f)" % (SWITCH_Z, SPK_Z))
    under = SPK_Z - r_in_ - FLOOR
    if under < 1.5:
        E("only %.2f mm of material under the speaker bore (needs >= 1.5)" % under)
    N("speaker holder: full round ring (bore dia %.1f for the %.1f mm speaker, %.1f mm per "
      "side), %.1f mm wall, %.1f mm deep, solid lower part, %.1f mm under the bore. No ribs."
      % (2 * r_in_, SPK_D, SPK_CLEAR, SPK_RING_T, SPK_RING_H, under))
    N("speaker: push it in from inside the box, face to the wall, then run a bead of hot "
      "glue around the back of the rim")
    N("speaker: from the ring to the PCB edge there is %.1f mm of free space to put it in, "
      "even with the PCB already mounted" % (PCB_OFF - WALL - SPK_RING_H))

    # ---- 3.8 printing notes ---------------------------------------------
    if PROCESS == "SLA":
        N("SLA: JLC orients the parts and adds supports; they can touch outside faces and leave "
          "small nubs (the sanding finish removes them). No inserts: all screws self-tap.")
        cb_front = HF - SCREW_CB_DEPTH - Z_SPLIT
        cb_rear = HB - SCREW_CB_DEPTH - Z_SPLIT
        N("corner screws (self-tapping M3, %.1f mm pilot): front pair M3 x %d, rear pair "
          "M3 x %d (about 8 mm of thread in the base)"
          % (ENC_PILOT_D, round(cb_front + 8), round(cb_rear + 8)))
        if ENC_SCREW_MODE == "insert":
            E("heat-set inserts do not work in SLA resin - set ENC_SCREW_MODE = 'selftap'")
    if PROCESS == "FDM":
        N("FDM only: the lid's 4 TFT bosses (%.1f mm) hang from the sloped face and "
          "the top of the speaker bore is an arch - both need support. JLC adds it "
          "automatically and it is all on inside faces. MJF needs no support at all."
          % TFT_BOSS_H)
        N("FDM only: print the base open-side-up and the lid open-side-down "
          "(the file already has the lid lying flat).")

    if verbose:
        print("=" * 74)
        print(" DESIGN RULE CHECK      process = %s   clearance = %.2f mm" % (PROCESS, FIT_CLEAR))
        print("=" * 74)
        for m in note:
            print("  note   " + m)
        for m in warn:
            print("  WARN   " + m)
        for m in err:
            print("  ERROR  " + m)
        print("-" * 74)
        print("  %d error(s), %d warning(s)" % (len(err), len(warn)))
        print("  base %.0f x %.0f x %.0f mm      lid %.0f x %.0f x %.0f mm"
              % (base_bb + lid_bb))
        print("=" * 74)
    return err, warn


# ============================================================================
# 4.  CAD
# ============================================================================
def build():
    import cadquery as cq

    def rrect(w, d, r, x0, y0, h, z0=0.0):
        return (cq.Workplane("XY").workplane(offset=z0)
                .center(x0 + w / 2, y0 + d / 2).rect(w, d).extrude(h)
                .edges("|Z").fillet(r))

    def profile_solid(pts, x0, x1):
        return cq.Workplane("YZ").workplane(offset=x0).polyline(pts).close().extrude(x1 - x0)

    # ---- shell ----------------------------------------------------------
    outer_prof = [(0, 0), (OUT, 0), (OUT, HB), (Y_S1, HB), (Y_S0, HF), (0, HF)]
    outer = profile_solid(outer_prof, 0, OUT).intersect(rrect(OUT, OUT, R_OUT, 0, 0, HB + 5))
    for sel, rad in ((">Z or (not <Z and not |Z)", 1.2), ("<Z", 1.0)):
        try:
            outer = outer.faces(sel).edges().fillet(rad)
        except Exception:
            pass

    zi_f, zi_b = HF - WALL, HB - WALL
    inner_prof = [(WALL, FLOOR), (OUT - WALL, FLOOR), (OUT - WALL, zi_b),
                  (inner_slope_y(zi_b), zi_b), (inner_slope_y(zi_f), zi_f), (WALL, zi_f)]
    inner = profile_solid(inner_prof, WALL, OUT - WALL).intersect(
        rrect(OUT - 2 * WALL, OUT - 2 * WALL, R_OUT - WALL, WALL, WALL, HB + 5))
    shell = outer.cut(inner)

    below = cq.Workplane("XY").box(OUT, OUT, Z_SPLIT, centered=False)
    above = cq.Workplane("XY").box(OUT, OUT, HB + 60, centered=False).translate((0, 0, Z_SPLIT))
    base = shell.intersect(below)
    lid = shell.intersect(above)

    def _solids(w):
        return w.solids().vals()

    vol = {"base": sum(x.Volume() for x in _solids(base)),
           "lid": sum(x.Volume() for x in _solids(lid))}

    def checkpoint(stage):
        """Stops the build the moment a boolean goes wrong, naming the stage."""
        for nm, w in (("base", base), ("lid", lid)):
            sol = _solids(w)
            v = sum(x.Volume() for x in sol)
            if len(sol) != 1 or v < 0.8 * vol[nm]:
                raise RuntimeError("CAD step failed at '%s': %s is now %d solid(s), volume "
                                   "%.1f -> %.1f cm3. Nothing was exported."
                                   % (stage, nm, len(sol), vol[nm] / 1000.0, v / 1000.0))
            vol[nm] = v
        print("  ok  %-22s base %6.1f cm3   lid %6.1f cm3"
              % (stage, vol["base"] / 1000.0, vol["lid"] / 1000.0))

    checkpoint("shell split")

    # ---- joint ----------------------------------------------------------
    def ring_solid(inset_out, inset_in, h, z0):
        return rrect(OUT - 2 * inset_out, OUT - 2 * inset_out, R_OUT - inset_out,
                     inset_out, inset_out, h, z0).cut(
            rrect(OUT - 2 * inset_in, OUT - 2 * inset_in, R_OUT - inset_in,
                  inset_in, inset_in, h + 1.0, z0 - 0.5))

    rail_foot = None
    for i in range(4):          # 4 x 0.5 mm steps = a 45 deg underside, prints unsupported
        s = ring_solid(WALL - RAIL_GAP, WALL + 0.5 * (i + 1), 0.5, Z_SPLIT - 2.0 + 0.5 * i)
        rail_foot = s if rail_foot is None else rail_foot.union(s)
    rail_top = ring_solid(WALL, WALL + RAIL_T, RAIL_H + 0.1, Z_SPLIT - 0.1)
    base = base.union(rail_foot).union(rail_top)
    lid = lid.cut(ring_solid(WALL - RAIL_GAP, WALL, RAIL_H + 0.5, Z_SPLIT - 0.1))

    checkpoint("joint + rail")

    # ---- PCB posts ------------------------------------------------------
    def pcb2box(p):
        return p[0] + PCB_OFF, p[1] + PCB_OFF

    for p in HOLES + [DUMMY]:
        x, y = pcb2box(p)
        base = base.union(cq.Workplane("XY").workplane(offset=FLOOR - 0.01)
                          .center(x, y).circle(3.5).extrude(STANDOFF_H + 0.01))
    for p in HOLES:
        x, y = pcb2box(p)
        base = base.cut(cq.Workplane("XY").workplane(offset=FLOOR + 1.0)
                        .center(x, y).circle(1.3).extrude(STANDOFF_H + 2))

    checkpoint("PCB posts")

    # ---- corner screw columns ------------------------------------------
    for x, y in ENC_SCREWS:
        base = base.union(cq.Workplane("XY").workplane(offset=FLOOR - 0.01)
                          .center(x, y).circle(SCREW_BOSS_R).extrude(Z_SPLIT - FLOOR + 0.02))
        wx = WALL if x < CX else OUT - WALL
        wy = WALL if y < CX else OUT - WALL
        rh = Z_SPLIT - 2.5 - FLOOR
        base = base.union(cq.Workplane("XY").box(abs(x - wx), 2.4, rh, centered=False)
                          .translate((min(x, wx), y - 1.2, FLOOR - 0.01)))
        base = base.union(cq.Workplane("XY").box(2.4, abs(y - wy), rh, centered=False)
                          .translate((x - 1.2, min(y, wy), FLOOR - 0.01)))
        if ENC_SCREW_MODE == "selftap":
            z0, dep, d = FLOOR - 0.1, Z_SPLIT - FLOOR + 0.4, ENC_PILOT_D
        else:
            z0, dep, d = Z_SPLIT - ENC_INSERT_DEPTH, ENC_INSERT_DEPTH + 0.4, ENC_INSERT_D
        base = base.cut(cq.Workplane("XY").workplane(offset=z0)
                        .center(x, y).circle(d / 2.0).extrude(dep))

        top = HF if y <= Y_S0 else (HF + (HB - HF) * (y - Y_S0) / (Y_S1 - Y_S0) if y < Y_S1 else HB)
        lid = lid.union(cq.Workplane("XY").workplane(offset=Z_SPLIT)
                        .center(x, y).circle(SCREW_BOSS_R).extrude(top - 0.2 - Z_SPLIT))
        lid = lid.cut(cq.Workplane("XY").workplane(offset=Z_SPLIT - 0.1)
                      .center(x, y).circle(SCREW_HOLE / 2).extrude(top - Z_SPLIT + 1.0))
        lid = lid.cut(cq.Workplane("XY").workplane(offset=top - SCREW_CB_DEPTH)
                      .center(x, y).circle(SCREW_COUNTERBORE_R).extrude(SCREW_CB_DEPTH + 0.5))

    checkpoint("corner screws")

    # ---- feet -----------------------------------------------------------
    for fx, fy in FEET:
        base = base.cut(cq.Workplane("XY").center(fx, fy).circle(FOOT_D / 2.0).extrude(FOOT_DEPTH))

    checkpoint("feet")

    # ---- rear USB-C -----------------------------------------------------
    usb = cq.Plane(origin=(USB_X, OUT + 1.0, USB_Z), xDir=(1, 0, 0), normal=(0, -1, 0))
    pk = cq.Workplane(usb).rect(USB_POCKET_W, USB_POCKET_H).extrude(1.0 + USB_POCKET_DEPTH)
    try:
        pk = pk.edges("|Y").fillet(USB_POCKET_R)
    except Exception:
        pass
    base = base.cut(pk)
    base = base.cut(cq.Workplane(usb).slot2D(USB_SLOT_W, USB_SLOT_H, 0).extrude(1.0 + WALL + 1.0))

    checkpoint("USB-C opening")

    # ---- step for the USB-C power module ----------------------------------
    base = base.union(cq.Workplane("XY")
                      .box(USB_SHELF_W, USB_SHELF_D + 0.3, USB_SHELF_TOP - FLOOR + 0.01,
                           centered=False)
                      .translate((USB_X - USB_SHELF_W / 2.0, OUT - WALL - USB_SHELF_D,
                                  FLOOR - 0.01)))

    checkpoint("USB-C step")

    # ---- right wall key switch -----------------------------------------
    sw = cq.Plane(origin=(OUT + 1.0, SWITCH_Y, SWITCH_Z), xDir=(0, 1, 0), normal=(-1, 0, 0))
    base = base.cut(cq.Workplane(sw).circle(SWITCH_D / 2.0).extrude(WALL + 2.0))

    checkpoint("key switch")

    # ---- left wall speaker ----------------------------------------------
    spk_in = cq.Plane(origin=(WALL - 0.01, SPK_Y, SPK_Z), xDir=(0, 1, 0), normal=(1, 0, 0))
    r_in = SPK_D / 2.0 + SPK_CLEAR
    r_out = r_in + SPK_RING_T
    # The whole holder is built as ONE solid first and joined to the base in ONE union.
    # It starts 0.5 mm inside the wall: a real overlap, never two faces touching.
    x0h = WALL - 0.5
    hh = SPK_RING_H + 0.5
    hold_pl = cq.Plane(origin=(x0h, SPK_Y, SPK_Z), xDir=(0, 1, 0), normal=(1, 0, 0))
    ring = cq.Workplane(hold_pl).circle(r_out).circle(r_in).extrude(hh)        # full round ring
    bore = cq.Workplane(hold_pl).circle(r_in).extrude(hh + 1.0)
    skirt = (cq.Workplane("XY").box(hh, 2 * r_out, SPK_Z - FLOOR + 0.5, centered=False)
             .translate((x0h, SPK_Y - r_out, FLOOR - 0.5)).cut(bore))           # solid lower part
    holder = ring.union(skirt)
    # lead channel under the speaker (cut from the holder only, never from the floor)
    holder = holder.cut(cq.Workplane("XY").box(hh + 1.0, 8.0, 4.0 + 1.0, centered=False)
                        .translate((WALL, SPK_Y - 4.0, FLOOR)))
    hs = holder.solids().vals()
    if len(hs) != 1 or not hs[0].isValid():
        raise RuntimeError("speaker holder is %d solid(s), valid=%s - not joined"
                           % (len(hs), [h.isValid() for h in hs]))
    base = base.union(holder)
    spk_out = cq.Plane(origin=(-1.0, SPK_Y, SPK_Z), xDir=(0, 1, 0), normal=(1, 0, 0))
    pts = []
    for rad, n in SPK_GRILLE:
        for i in range(n):
            a = 2 * math.pi * i / n
            pts.append((rad * math.cos(a), rad * math.sin(a)))

    def snap_z(zc, r=SPK_GRILLE_D / 2.0, web=1.2):
        lo, hi = Z_SPLIT - r - web, Z_SPLIT + r + web
        return zc if (zc <= lo or zc >= hi) else (lo if (zc - lo) < (hi - zc) else hi)

    pts = [(px, snap_z(SPK_Z + py) - SPK_Z) for px, py in pts]
    grille = cq.Workplane(spk_out).pushPoints(pts).circle(SPK_GRILLE_D / 2.0).extrude(WALL + 2.0)
    base = base.cut(grille)
    lid = lid.cut(grille)

    checkpoint("speaker")

    # ---- side vents (identical on both walls) ---------------------------
    for x_off, nrm in ((-1.0, (1, 0, 0)), (OUT + 1.0, (-1, 0, 0))):
        for y0, y1 in VENT_BLOCKS:
            for y in vent_ys(y0, y1):
                pl = cq.Plane(origin=(x_off, y, VENT_SLOT_Z), xDir=(0, 1, 0), normal=nrm)
                base = base.cut(cq.Workplane(pl).slot2D(VENT_SLOT_H, VENT_SLOT_W, 90)
                                .extrude(WALL + 2.0))

    checkpoint("side vents")

    # ---- TFT on the slope ----------------------------------------------
    mid = SLOPE_LEN / 2.0
    cy, cz = Y_S0 + UX * mid, HF + UZ * mid
    slope = cq.Plane(origin=(CX, cy, cz), xDir=(1, 0, 0), normal=(0, NX, NZ))
    lid = lid.cut(cq.Workplane(slope).rect(TFT_WINDOW_W, TFT_WINDOW_H).extrude(8, both=True))
    if TFT_BEZEL_DEPTH > 0:
        lid = lid.cut(cq.Workplane(slope)
                      .rect(TFT_WINDOW_W + 2 * TFT_BEZEL, TFT_WINDOW_H + 2 * TFT_BEZEL)
                      .extrude(-TFT_BEZEL_DEPTH))

    def slope_plane(depth):
        """a plane parallel to the slope, 'depth' mm inside the wall, facing the cavity"""
        return cq.Plane(origin=(CX, cy - NX * depth, cz - NZ * depth),
                        xDir=(1, 0, 0), normal=(0, -NX, -NZ))

    lid = lid.union(cq.Workplane(slope_plane(WALL - 0.3)).pushPoints(TFT_PTS)
                    .circle(TFT_HOLE_PAD_D / 2.0).extrude(TFT_BOSS_H + 0.3))
    # glass relief: guarantees TFT_GLASS_CLR around the glass and flattens the bosses
    lid = lid.cut(cq.Workplane(slope_plane(WALL - 0.05)).center(TFT_PCB_DX, 0)
                  .rect(TFT_KO_W, TFT_KO_H).extrude(TFT_BOSS_H + 1.05))
    # blind pilots for the M2 screws
    lid = lid.cut(cq.Workplane(cq.Plane(origin=(CX, cy - NX * (WALL + TFT_BOSS_H),
                                                cz - NZ * (WALL + TFT_BOSS_H)),
                                        xDir=(1, 0, 0), normal=(0, NX, NZ)))
                  .pushPoints(TFT_PTS).circle(TFT_PILOT_D / 2.0)
                  .extrude(TFT_BOSS_H + TFT_PILOT_INTO_WALL))

    checkpoint("TFT")

    # ---- servo mount on the roof (glued, no screws) -------------------
    # one plain block; the body stands on the roof inside the pocket and the ears
    # rest on top of the two end walls
    tower = (cq.Workplane("XY").workplane(offset=HB - 0.3)
             .center(SERVO_CX, (SRV_OUT_Y0 + SRV_OUT_Y1) / 2.0)
             .rect(SRV_SIDE_X, SRV_OUT_Y1 - SRV_OUT_Y0)
             .extrude(SERVO_LEDGE_H + 0.3))
    try:
        tower = tower.edges("|Z").fillet(2.5)
    except Exception:
        pass
    lid = lid.union(tower)

    # body pocket: closed floor, the roof stays solid under the servo
    lid = lid.cut(cq.Workplane("XY").workplane(offset=HB)
                  .center(SERVO_CX, SERVO_CY).rect(SRV_POCKET_X, SRV_POCKET_Y)
                  .extrude(SERVO_LEDGE_H + 2.0))
    # guide ribs on the two long pocket walls
    def rib(fx, fy, ix, iy):
        ax, ay = -iy, ix
        prof = [(-0.5, -1.5), (0.0, -1.5), (SERVO_RIB_H, -0.6),
                (SERVO_RIB_H, 0.6), (0.0, 1.5), (-0.5, 1.5)]       # 1.2 mm flat tip
        pts2 = [(fx + d * ix + a * ax, fy + d * iy + a * ay) for d, a in prof]
        return (cq.Workplane("XY").workplane(offset=HB - 0.1).polyline(pts2).close()
                .extrude(SERVO_LEDGE_H - 2.0 + 0.1))

    for dy in (-12.0, 0.0, 12.0):
        lid = lid.union(rib(SERVO_CX - SRV_POCKET_X / 2, SERVO_CY + dy, 1, 0))
        lid = lid.union(rib(SERVO_CX + SRV_POCKET_X / 2, SERVO_CY + dy, -1, 0))
    # lead slot: full height through the rear end wall, open at the top, and on through
    # the roof into the cable slot - the lead and its plug simply drop in
    lid = lid.cut(cq.Workplane("XY").workplane(offset=HB - WALL - 1.0)
                  .center(SERVO_CX, (SRV_POCKET_Y1 + SERVO_HOLE_CY) / 2.0)
                  .rect(SERVO_WIRE_W, SERVO_HOLE_CY - SRV_POCKET_Y1)
                  .extrude(WALL + SERVO_LEDGE_H + 3.0))
    # roof cable slot right behind the mount
    lid = lid.cut(cq.Workplane("XY").workplane(offset=HB - WALL - 1.0)
                  .center(SERVO_CX, SERVO_HOLE_CY)
                  .slot2D(SERVO_HOLE_L, SERVO_HOLE_W, 0).extrude(WALL + 2.0))

    checkpoint("servo mount")

    # ---- roof vents -----------------------------------------------------
    for off in ROOF_VENT_OFFS:
        for s in (-1, 1):
            lid = lid.cut(cq.Workplane("XY").workplane(offset=HB - WALL - 1.0)
                          .center(CX + s * off, SERVO_CY)
                          .slot2D(ROOF_VENT_LEN, VENT_SLOT_W, 90).extrude(WALL + 2.0))

    checkpoint("roof vents")

    # ---- pry notches in the base top edge -------------------------------
    for ny0 in (-1.0, OUT - 0.75):
        base = base.cut(cq.Workplane("XY").box(16.0, 1.75, 2.5, centered=(True, False, False))
                        .translate((CX, ny0, Z_SPLIT - 2.5)))

    checkpoint("pry notches")

    return base, lid


# ============================================================================
# 5.  EXPORT
# ============================================================================
def export(base, lid):
    """Writes 6 files, in this order:
         1_enclosure_base_plus_lid_v12  - the lid sitting on the base
         2_enclosure_lid_v12            - the lid, lying open side down as printed
         3_enclosure_base_v12           - the base
       each as STEP and STL."""
    import os
    import cadquery as cq

    d = os.path.join(os.path.dirname(os.path.abspath(__file__)), "outputs")
    os.makedirs(d, exist_ok=True)
    lid_flat = lid.translate((0, 0, -Z_SPLIT))
    both = cq.Workplane("XY").newObject(base.solids().vals() + lid.solids().vals())

    written = []
    for name, shape in (("1_enclosure_base_plus_lid_v12", both),
                        ("2_enclosure_lid_v12", lid_flat),
                        ("3_enclosure_base_v12", base)):
        for ext, kw in (("step", {}), ("stl", dict(tolerance=0.01, angularTolerance=0.05))):
            p = os.path.join(d, "%s.%s" % (name, ext))
            cq.exporters.export(shape, p, **kw)
            written.append(p)

    for name, shape in (("base", base), ("lid", lid_flat)):
        bb = shape.val().BoundingBox()
        print("  %-5s %6.1f x %6.1f x %6.1f mm   valid=%s   volume=%.1f cm3"
              % (name, bb.xlen, bb.ylen, bb.zlen, shape.val().isValid(),
                 shape.val().Volume() / 1000.0))
    print()
    for p in written:
        print("  " + os.path.basename(p))
    print("\n  %d files in %s" % (len(written), d))
    return written


if __name__ == "__main__":
    import sys

    errors, warnings = check()
    if errors:
        print("\nBUILD STOPPED: fix the errors above.")
        sys.exit(1)
    if "--check" in sys.argv:
        sys.exit(0)
    print("\nbuilding ...")
    b, l = build()
    export(b, l)