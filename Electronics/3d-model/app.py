#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
150 x 150 mm two-part enclosure (base + lid) for a 100 x 100 mm ESP32-S3 board.

    python3 enclosure_v15.py --check     design-rule check only (NO CadQuery needed)
    python3 enclosure_v15.py --scan      check + voxel scan of the base for < 0.8 mm walls / gaps
    python3 enclosure_v15.py             check + build + export STEP / STL

Contents
    - 100 x 100 mm custom PCB on 4 posts (3 real M3 holes + 1 dummy post)
    - 1.8" 128x160 ST7735 TFT on the sloped front face (frame pocket, 4 screws)
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

v15 - JLC "< 0.8 mm wall thickness" on the base: the solid walls were all >= 0.8 mm; the
    flagged spots were 4 hairline crevices at the speaker holder (ring dipping into the floor,
    flat legs cutting into the curved ring).  The ring now hangs from the wall only, with a
    1.2 mm real gap above the floor.  `--scan` voxel-checks the whole base for solid walls
    AND gaps under 0.8 mm (numpy + scipy, no CadQuery).

v14 - anti-warp revision (JLC3DP flagged v13 for deformation / warping)
    Resin shrinks as it cures; flat thin panels and thick lumps next to thin
    walls shrink unevenly and bend the part.  v14 removes every such cause:
    - floor 2.5 -> 3.0 mm, so floor, walls and roof are all one thickness
    - servo end walls: 8 mm solid -> 2.5 mm shells (cavity opens into the lid)
    - USB-C step: solid 20x20x7 block -> 2 mm top plate on 3 legs, open front
    - speaker holder: 6.5 mm solid skirt -> two 2.5 mm legs
    - dummy PCB post cored like the other three
    - rib grid under the floor (3 x 3), ribs tying the base walls to the floor,
      and a rib frame under the flat roof: no panel spans more than ~50 mm
    - check() verifies: no solid section thicker than MAX_SECTION, every rib
      clear of vents / slots / screws / PCB / TFT, every cavity can drain
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
FLOOR = 3.0                 # v14: = WALL (was 2.5) - one thickness everywhere
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

# Everything on the main PCB, as axis-aligned keep-out boxes in BOX coordinates:
# pad extents + 3 mm (or the can diameter for electrolytics), read straight from
# Development_pcb.kicad_pcb (all 31 parts).  Heights are typical body heights.
# (ref, x0, y0, x1, y1, height above the top of the PCB)
PCB_PARTS = [
    ("J7",  114.67,   42.74,  122.62,   57.94,  11.5),   # JST-XH
    ("J3",  114.64,   92.31,  122.64,  102.51,  11.5),   # JST-XH
    ("C9",  109.51,   92.34,  117.12,  102.44,   7.0),   # ceramic cap
    ("J8",  108.30,  107.36,  118.84,  115.36,  11.0),   # screw terminal (power in)
    ("C13",  105.71,   25.00,  113.31,   35.10,   7.0),   # ceramic cap
    ("C3",  103.99,   68.50,  112.99,   79.60,  12.0),   # electrolytic D8.0mm
    ("C4",  103.99,   55.80,  112.99,   66.90,  12.0),   # electrolytic D8.0mm
    ("C10",  103.42,   92.36,  111.02,  102.46,  12.0),   # electrolytic D6.3mm
    ("C14",   99.36,   24.99,  106.96,   35.09,  12.0),   # electrolytic D5.0mm
    ("D2",   99.31,   80.59,  117.67,   88.79,   3.2),   # diode
    ("F1",   95.50,  107.36,  108.60,  116.57,  10.0),   # polyfuse
    ("J2",   93.09,   69.41,  101.03,   82.11,  11.5),   # JST-XH
    ("J1",   93.09,   56.71,  101.03,   69.41,  11.5),   # JST-XH
    ("C7",   92.56,   80.89,  101.56,   91.99,  12.0),   # electrolytic D8.0mm
    ("J6",   90.71,   34.99,  115.91,   42.95,  11.5),   # JST-XH
    ("D1",   87.78,  107.16,   96.18,  125.72,  12.0),   # diode
    ("C8",   86.91,   80.89,   94.51,   90.99,   7.0),   # ceramic cap
    ("C1",   81.19,  107.56,   88.80,  117.66,   7.0),   # ceramic cap
    ("C2",   67.45,  107.11,   80.95,  120.61,  25.0),   # electrolytic D12.5mm
    ("U1",   57.65,   30.04,   90.75,   91.08,  25.0),   # ESP32-S3-DevKitC-1 in its socket
    ("R3",   50.08,   35.17,   60.22,   42.77,  11.0),   # resistor
    ("R2",   50.00,   40.25,   60.14,   47.85,  11.0),   # resistor
    ("C5",   47.58,   48.55,   55.18,   58.65,   7.0),   # ceramic cap
    ("R1",   47.54,   78.35,   55.14,   96.11,   3.5),   # resistor
    ("Q1",   45.95,   39.26,   53.45,   48.85,   8.0),   # TO-92
    ("J4",   39.36,   38.92,   47.36,   49.12,  11.5),   # JST-XH
    ("C6",   38.65,   48.57,   46.25,   58.67,  12.0),   # electrolytic D6.3mm
    ("J5",   34.83,   83.23,   45.03,   91.23,  11.5),   # JST-XH
    ("C12",   33.29,   38.97,   40.89,   49.07,   7.0),   # ceramic cap
    ("U2",   29.46,   55.44,   55.19,   80.92,  12.0),   # MP3-TF-16P player
    ("C11",   26.34,   38.97,   35.34,   50.07,  12.0),   # electrolytic D8.0mm
]
# radial electrolytics: (ref, centre x, centre y, diameter) - drawn as cylinders
PCB_ROUND = [
    ("C14",  103.16,   30.04,   5.0),
    ("C3",  108.49,   74.05,   8.0),
    ("C2",   74.20,  113.86,  12.5),
    ("C6",   42.45,   53.62,   6.3),
    ("C11",   30.84,   44.52,   8.0),
    ("C4",  108.49,   61.35,   8.0),
    ("C10",  107.22,   97.41,   6.3),
    ("C7",   97.06,   86.44,   8.0),
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
# YOUR module (photos): 8-pin GND VCC SCL SDA RST DC CS BLK on a short edge, an 8-pin
# connector on the BACK at that edge with the wires leaving straight back, and the LCD
# in a WHITE PLASTIC FRAME on the front.  The frame has two square tabs at the corners
# away from the pins, right over the two PCB holes there - so screw bosses cannot be
# used at that end.
#
# Mounting (v13): the white frame drops into a pocket in the inside face of the slope,
# sized to the frame, so the frame itself locates the screen.  Its front face rests on
# the bottom of the pocket; a window through the wall shows the lit area.  4 screws,
# no glue: M2 self-tapping screws through the module's own 4 holes into 4 bosses.
# The module lies on its side: long edge across the box, pins on the right as seen
# from the front.
TFT_BODY_W = 55.62          # PCB long side   (your caliper; drawing 55.00)
TFT_BODY_H = 34.83          # PCB short side  (your caliper; drawing 34.70)
# PCB holes: the drawing says 2.0 mm holes; your caliper outer-edge-to-outer-edge
# readings 53.6 / 32.5 then give 51.6 / 30.5 centre to centre (~2.0 mm from the edges,
# which matches your photo).
TFT_HOLE_INNER_D = 2.0
TFT_HOLE_SP_L = 53.6 - TFT_HOLE_INNER_D      # 51.6
TFT_HOLE_SP_S = 32.5 - TFT_HOLE_INNER_D      # 30.5
# Screws: 4 x M2 self-tapping through the module's own 4 holes into 4 bosses.
#  - pin end: 3.1 mm from the hole centre to the white frame -> 5.0 mm bosses.
#  - far end: you measured 4.62 mm from the end of the white frame (tabs included) to
#    the PCB edge, i.e. 2.6 mm from the hole centre -> 4.4 mm bosses.
TFT_BOSS_D = 5.0            # pin-end bosses (1.65 mm wall round the pilot)
TFT_BOSS_FAR_D = 4.4        # far-end bosses (1.35 mm wall round the pilot)
TFT_PILOT_D = 1.7 if PROCESS == "SLA" else 1.6   # M2 self-tapping pilot
TFT_PILOT_SKIN = 1.6        # plastic left between the pilot bottom and the outside (>= 1.2,
                            # with margin so the JLC thin-wall heatmap stays grey)
TFT_BOSS_PRESS = 0.15       # boss is this much short of the PCB, so the screws pull the
                            # frame face firmly onto the pocket floor
TFT_PCB_T = 1.6             # module PCB thickness (only used for the preview model)

# White frame (LCD outline) from the supplier drawing of this module:
#   PCB 56.00 x 35.00, LCD outline 45.83 x 33.00, 2.25 high above a 1.20 PCB,
#   starting 5.09 from the pin edge, centred across the PCB (1.0 each side).
TFT_FRAME_END_TO_EDGE = 4.62   # YOUR caliper: end of the white frame (tabs) -> far PCB edge
TFT_FRAME_S = 33.00         # white frame, short side
TFT_FRAME_T = 2.25          # white frame height above the front of the PCB
TFT_FRAME_FROM_PIN = 5.09   # pin edge of the PCB -> start of the white frame
TFT_FRAME_L = TFT_BODY_W - TFT_FRAME_FROM_PIN - TFT_FRAME_END_TO_EDGE   # 45.91 (drawing 45.83)
TFT_PIN_TAIL_H = 1.8        # solder joints sticking out of the PCB front (estimate)
TFT_FRAME_MEASURED = True   # supplier drawing
TFT_FRAME_CLR = 0.30        # per side around the frame (SLA +-0.2, FDM holes -0.4 -> tight)

TFT_ACTIVE_W = 35.04        # active (lit) area
TFT_ACTIVE_H = 28.03
# The lit area starts 8.0 mm from the pin edge (supplier) and ends 13.0 mm from the
# other edge: on the 55.62 PCB those disagree by 0.42 mm.  The window is centred on
# the midpoint and made 0.42 mm wider, so it shows the whole lit area either way.
TFT_LIT_FROM_PIN = (8.0 + (TFT_BODY_W - 13.0 - TFT_ACTIVE_W)) / 2.0    # 7.79
TFT_LIT_UNCERT = abs(8.0 + TFT_ACTIVE_W + 13.0 - TFT_BODY_W)           # 0.42
TFT_WINDOW_W = TFT_ACTIVE_W + 2.00 + TFT_LIT_UNCERT   # 1.0 mm margin per side
TFT_WINDOW_H = TFT_ACTIVE_H + 2.00
TFT_ACTIVE_OFFSET = TFT_BODY_W / 2.0 - TFT_LIT_FROM_PIN - TFT_ACTIVE_W / 2.0  # lit centre
                                                                            # -> pin side
TFT_PIN_SIDE = +1           # +1 pins on the right seen from the front, -1 left
TFT_POCKET_D = 1.0          # frame pocket depth into the 3 mm wall (2 mm stays)
TFT_PIN_ROW_FROM_EDGE = 2.0 # pin centres from the PCB pin edge (drawing: ~1.5..2.5)
TFT_LOCATE_RIB = 1.6        # wall kept between the pin relief and the frame pocket, so the
                            # pocket still locates the frame on the pin side (>= 1.2 for JLC)
TFT_PIN_RELIEF_TO = TFT_FRAME_FROM_PIN - TFT_FRAME_CLR - TFT_LOCATE_RIB  # pin edge -> in
TFT_PIN_RELIEF_S = 22.0     # ... and 22 mm along the pin row (8 pins x 2.54 + pads)
TFT_BEZEL = 2.0             # cosmetic recess around the window, outside
TFT_BEZEL_DEPTH = 0.5       # >= MIN_DETAIL; wall left between bezel and pocket is checked

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
USB_SHELF_H = USB_SHELF_TOP - FLOOR                       # 6.75 above the inside floor
# v14: the step is no longer a solid block. A 2 mm top plate (the glue face) stands
# on 3 legs; the bays under it are open toward the PCB so resin drains out.
USB_SHELF_SKIN = 2.0        # top plate and leg thickness

# ---- right wall key switch (mirror image of the speaker) ------------------
SWITCH_D = 12.8             # 12 mm thread + FDM hole tolerance (holes print ~0.4 small)
SWITCH_Y = OUT / 2.0        # 75.0 - centred, exactly opposite the speaker
SWITCH_Z = FLOOR + 30.0 / 2.0 + 0.3 + 1.8        # 20.1 - same height as the speaker centre
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
SPK_FLOOR_T = 1.8           # bore bottom -> floor (sets the speaker height)
# v15: JLC flagged "< 0.8 mm" on the v14 base.  Every solid wall was >= 0.8 mm; what it
# flagged were 4 hairline crevices at the speaker holder:
#   - where the ring dipped into the floor beside the lead channel it met the floor at
#     only ~16 deg, leaving a ~3 mm long wedge thinner than 0.8 mm on each side
#   - below the ring's centre, each leg's flat face met the curved ring at a sharp angle,
#     leaving a wedge from z ~11 down to the floor on each side
# Fix: the ring no longer touches the floor and has no legs.  It is fused to the left wall
# round its full circle (it starts 0.5 mm inside the wall), which holds it on its own; the
# lead channel under it is widened until the ring ends stand SPK_END_CLEAR above the floor.
SPK_LEGS = False            # v14 legs removed (they made the ring/leg crevices)
SPK_LEG_T = 2.5             # (only used if SPK_LEGS)
SPK_LEG_PROUD = 0.3
SPK_END_CLEAR = 1.2         # ring ends beside the lead channel -> floor: a real gap, >= 0.8
SPK_Z = FLOOR + SPK_D / 2.0 + SPK_BORE_CLR + SPK_FLOOR_T     # 20.1
_spk_ro = SPK_D / 2.0 + SPK_BORE_CLR + SPK_RING_T            # ring outer radius
SPK_CHANNEL_HALF = math.ceil(10.0 * math.sqrt(
    _spk_ro ** 2 - (SPK_Z - FLOOR - SPK_END_CLEAR) ** 2)) / 10.0   # 8.0: half-width of the
                                                                   # channel under the ring
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
SERVO_SHELL_T = 2.5         # v14: end walls are hollow shells of this thickness ...
SERVO_CAP_T = 2.5           # ... closed at the top (the ear rest), open into the lid below
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
# x = 0 is the centre of the window = centre of the lit area
TFT_BEZEL_DEPTH = max(TFT_BEZEL_DEPTH, MIN_DETAIL)
TFT_PCB_DX = -TFT_PIN_SIDE * TFT_ACTIVE_OFFSET        # module PCB centre
TFT_PIN_EDGE_X = TFT_PCB_DX + TFT_PIN_SIDE * TFT_BODY_W / 2.0
TFT_FRAME_CX = TFT_PIN_EDGE_X - TFT_PIN_SIDE * (TFT_FRAME_FROM_PIN + TFT_FRAME_L / 2.0)
TFT_POCKET_W = TFT_FRAME_L + 2 * TFT_FRAME_CLR
TFT_POCKET_H = TFT_FRAME_S + 2 * TFT_FRAME_CLR
TFT_RELIEF_CX = TFT_PIN_EDGE_X - TFT_PIN_SIDE * (TFT_PIN_RELIEF_TO - 0.5) / 2.0
TFT_RELIEF_W = TFT_PIN_RELIEF_TO + 0.5                # starts 0.5 mm past the PCB edge
TFT_PCB_FRONT_D = WALL - TFT_POCKET_D + TFT_FRAME_T   # outside surface -> PCB front
TFT_PCB_BACK_D = TFT_PCB_FRONT_D + TFT_PCB_T
TFT_TAIL_GAP = TFT_PCB_FRONT_D - TFT_PIN_TAIL_H - WALL   # solder joints -> inner wall face
TFT_RELIEF_D = max(0.0, 0.5 - TFT_TAIL_GAP)           # keep >= 0.5 mm in front of the joints
TFT_BOSS_H = TFT_PCB_FRONT_D - WALL - TFT_BOSS_PRESS  # inner wall face -> top of the boss
TFT_SCREW_X = TFT_PIN_EDGE_X - TFT_PIN_SIDE * (TFT_BODY_W - TFT_HOLE_SP_L) / 2.0
TFT_PTS = [(TFT_SCREW_X, -TFT_HOLE_SP_S / 2.0), (TFT_SCREW_X, TFT_HOLE_SP_S / 2.0)]
TFT_PILOT_DEPTH = TFT_BOSS_H + WALL - TFT_PILOT_SKIN  # boss + into the wall
TFT_FAR_EDGE_X = TFT_PIN_EDGE_X - TFT_PIN_SIDE * TFT_BODY_W
TFT_SCREW_FAR_X = TFT_FAR_EDGE_X + TFT_PIN_SIDE * (TFT_BODY_W - TFT_HOLE_SP_L) / 2.0
TFT_PTS_FAR = [(TFT_SCREW_FAR_X, -TFT_HOLE_SP_S / 2.0), (TFT_SCREW_FAR_X, TFT_HOLE_SP_S / 2.0)]
TFT_PTS_ALL = TFT_PTS + TFT_PTS_FAR

# ---- v14 anti-warp stiffening ---------------------------------------------
# Ribs are 0.6 x the wall: stiff enough to stop a panel bowing, thin enough not to
# leave sink marks or become thick lumps themselves.
RIB_T = 1.8                 # floor and roof ribs
WALL_RIB_T = 2.0            # base wall ribs (deliberately != RIB_T: no coplanar faces
                            # where a wall rib meets a floor rib)
WALL_RIB_D = 3.0            # how far the wall ribs stand out from the inner wall face
MAX_SECTION = 4.0           # thickest solid section allowed anywhere (the base rim
                            # flange of the rail is the one intended exception)
MAX_PANEL = 52.0            # largest unsupported span of a flat panel
# floor grid: 2.5 mm tall -> PCB underside (z 8.0) stays 2.5 mm clear of the ribs
FLOOR_RIB_H = 2.5
_SPK_CHANNEL_END = WALL + SPK_RING_H + 3.0          # 12.0: past the speaker lead channel
_USB_STEP_Y0 = OUT - WALL - USB_SHELF_D             # 127.0
# (rib runs along y at x)       (x, y0, y1)
FLOOR_RIBS_ALONG_Y = [(45.0, WALL - 0.2, OUT - WALL + 0.2),
                      (75.0, WALL - 0.2, _USB_STEP_Y0 + 0.5),   # ends in the USB step leg
                      (105.0, WALL - 0.2, OUT - WALL + 0.2)]
# (rib runs along x at y)       (y, x0, x1)
FLOOR_RIBS_ALONG_X = [(45.0, WALL - 0.2, OUT - WALL + 0.2),
                      (75.0, _SPK_CHANNEL_END, OUT - WALL + 0.2),  # leaves the speaker
                      (105.0, WALL - 0.2, OUT - WALL + 0.2)]       # lead channel open
# base wall ribs, floor -> just under the rail foot, tying the open rim to the floor
# The stepped rail foot fills z = Z_SPLIT-2 .. Z_SPLIT, inset 3..5 mm from the outside.
# v13 stopped the corner ribs 0.5 mm under it and the speaker ring 0.2 mm under it:
# hairline gaps that JLC's thin-wall check reports as "< 0.8 mm".  Everything near the
# walls now either runs UP INTO the foot (merges) or stays >= RIM_CLEAR below it.
RAIL_FOOT_Z0 = Z_SPLIT - 2.0
RAIL_FOOT_STEPS = 2                 # 2 x 1.0 mm steps (v13: 4 x 0.5 mm, under 0.8 mm)
JLC_MIN_FEATURE = 0.8               # the limit in JLC's report
RIB_TOP_IN_RIM = Z_SPLIT - 0.3      # rib tops end inside the top step of the foot
RIM_CLEAR = 1.0                     # otherwise keep at least this much under the foot
WALL_RIB_TOP = RIB_TOP_IN_RIM
CORNER_RIB_TOP = RIB_TOP_IN_RIM     # ribs from the corner screw columns to the walls
# the speaker ring top cannot drop 1 mm (it is already down at the floor), so a small
# block ties it up into the foot instead of leaving a 0.1 mm gap
SPK_TIE_W = 12.0                    # along the wall, centred on the speaker
SPK_TIE_X1 = WALL + 1.8             # 4.8: out from the outer face (inside the foot's 5.0)
WALL_RIBS = {"front": (45.0, 75.0, 105.0),        # x positions
             "rear": (45.0, 62.0, 88.0, 105.0),   # x (75 is the USB-C opening + step)
             "left": (53.0, 96.0),                # y (between vents and speaker ring)
             "right": (53.0, 96.0)}               # y (between vents and key switch)
# rib frame under the flat roof of the lid
ROOF_RIB_H = 3.0
ROOF_RIB_Y0 = 60.0          # front-back ribs start here (behind the TFT module)
ROOF_RIBS_ALONG_Y = [(54.0, ROOF_RIB_Y0, OUT - WALL + 0.2),     # (x, y0, y1)
                     (96.0, ROOF_RIB_Y0, OUT - WALL + 0.2)]
ROOF_RIBS_ALONG_X = [(64.0, WALL - 0.2, OUT - WALL + 0.2),      # (y, x0, x1)
                     (128.0, WALL - 0.2, 56.0),                 # skips the servo lead
                     (128.0, 94.0, OUT - WALL + 0.2)]           # slot + cable slot

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


def spk_ring_top():
    return SPK_Z + SPK_D / 2.0 + SPK_CLEAR + SPK_RING_T


def spk_needs_tie():
    """True if the ring top is too close under the rail foot to leave a clean gap"""
    return spk_ring_top() > RAIL_FOOT_Z0 - RIM_CLEAR


def spk_channel_tan():
    """tan of the half-angle of the V cut under the speaker ring (apex at the ring centre);
    it crosses the ring's outer surface SPK_CHANNEL_HALF either side of the centre"""
    r_o = SPK_D / 2.0 + SPK_CLEAR + SPK_RING_T
    return SPK_CHANNEL_HALF / math.sqrt(r_o ** 2 - SPK_CHANNEL_HALF ** 2)


def spk_tie_z0():
    """bottom of the tie block: just above the top of the bore, inside the ring wall"""
    return SPK_Z + SPK_D / 2.0 + SPK_CLEAR + 0.3


# ---- v14 stiffening geometry, as boxes (x0, y0, z0, x1, y1, z1) -------------
# One source for both check() and build(), so the checker tests exactly what is built.
def floor_rib_boxes():
    z0, z1 = FLOOR - 0.2, FLOOR + FLOOR_RIB_H
    out = [(x - RIB_T / 2, y0, z0, x + RIB_T / 2, y1, z1) for x, y0, y1 in FLOOR_RIBS_ALONG_Y]
    out += [(x0, y - RIB_T / 2, z0, x1, y + RIB_T / 2, z1) for y, x0, x1 in FLOOR_RIBS_ALONG_X]
    return out


def wall_rib_boxes():
    z0, z1, h, d = FLOOR - 0.2, WALL_RIB_TOP, WALL_RIB_T / 2, WALL_RIB_D
    out = []
    for x in WALL_RIBS["front"]:
        out.append(("front", x, (x - h, WALL - 0.2, z0, x + h, WALL + d, z1)))
    for x in WALL_RIBS["rear"]:
        out.append(("rear", x, (x - h, OUT - WALL - d, z0, x + h, OUT - WALL + 0.2, z1)))
    for y in WALL_RIBS["left"]:
        out.append(("left", y, (WALL - 0.2, y - h, z0, WALL + d, y + h, z1)))
    for y in WALL_RIBS["right"]:
        out.append(("right", y, (OUT - WALL - d, y - h, z0, OUT - WALL + 0.2, y + h, z1)))
    return out


def roof_rib_boxes():
    z0, z1 = HB - WALL - ROOF_RIB_H, HB - WALL + 0.2
    out = [(x - RIB_T / 2, y0, z0, x + RIB_T / 2, y1, z1) for x, y0, y1 in ROOF_RIBS_ALONG_Y]
    out += [(x0, y - RIB_T / 2, z0, x1, y + RIB_T / 2, z1) for y, x0, x1 in ROOF_RIBS_ALONG_X]
    return out


def servo_cavities():
    """hollow cores of the two servo end walls, (x0, y0, x1, y1); open into the lid"""
    s = SERVO_SHELL_T
    out = [(SERVO_CX - SRV_SIDE_X / 2 + s, SRV_OUT_Y0 + s,           # front end wall
            SERVO_CX + SRV_SIDE_X / 2 - s, SRV_POCKET_Y0 - s)]
    for k in (-1, 1):                                                 # rear wall: 2 pillars
        a = SERVO_CX + k * (SERVO_WIRE_W / 2 + s)                     # either side of the
        b = SERVO_CX + k * (SRV_SIDE_X / 2 - s)                       # lead slot
        out.append((min(a, b), SRV_POCKET_Y1 + s, max(a, b), SRV_OUT_Y1 - s))
    return out


def usb_step_parts():
    """top plate + 3 legs of the USB-C step, as boxes"""
    t = USB_SHELF_SKIN
    x0 = USB_X - USB_SHELF_W / 2
    y0, y1 = OUT - WALL - USB_SHELF_D, OUT - WALL + 0.3
    plate = (x0, y0, USB_SHELF_TOP - t, x0 + USB_SHELF_W, y1, USB_SHELF_TOP)
    legs = [(lx, y0, FLOOR - 0.2, lx + t, y1, USB_SHELF_TOP - t + 0.3)
            for lx in (x0, USB_X - t / 2, x0 + USB_SHELF_W - t)]
    return plate, legs


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
    spk_w = spk_r + SPK_RING_T + (SPK_LEG_PROUD if SPK_LEGS else 0.0)   # half-width
    walls = {
        "left": slot_boxes() + [
            ("speaker bore", SPK_Y - spk_r, SPK_Y + spk_r,
             SPK_Z - spk_r, SPK_Z + spk_r, "hole", "spk"),
            ("speaker ring", SPK_Y - spk_w, SPK_Y + spk_w,
             FLOOR, SPK_Z + spk_r + SPK_RING_T, "solid", "spk")],
        "right": slot_boxes() + [
            ("key switch hole", SWITCH_Y - SWITCH_D / 2, SWITCH_Y + SWITCH_D / 2,
             SWITCH_Z - SWITCH_D / 2, SWITCH_Z + SWITCH_D / 2, "hole", "sw"),
            ("key switch nut face", SWITCH_Y - SWITCH_NUT_D / 2, SWITCH_Y + SWITCH_NUT_D / 2,
             SWITCH_Z - SWITCH_NUT_D / 2, SWITCH_Z + SWITCH_NUT_D / 2, "solid", "sw")],
        "rear": [("USB-C pocket", USB_X - USB_POCKET_W / 2, USB_X + USB_POCKET_W / 2,
                  USB_Z - USB_POCKET_H / 2, USB_Z + USB_POCKET_H / 2, "hole", "usb"),
                 ("USB-C step", USB_X - USB_SHELF_W / 2, USB_X + USB_SHELF_W / 2,
                  FLOOR, USB_SHELF_TOP, "solid", "usb")],
        "front": [],
    }
    # v14: wall ribs are solid features on the inside of their wall
    for wname, pos, _b in wall_rib_boxes():
        walls[wname].append(("wall rib %.0f" % pos, pos - WALL_RIB_T / 2, pos + WALL_RIB_T / 2,
                             FLOOR, WALL_RIB_TOP, "solid", "rib%s%.0f" % (wname, pos)))
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
    if not TFT_FRAME_MEASURED:
        W("TFT white frame %.2f x %.2f x %.2f, %.1f mm from the pin edge, solder joints %.1f "
          "mm: ESTIMATED from photos - measure them and set TFT_FRAME_MEASURED = True"
          % (TFT_FRAME_L, TFT_FRAME_S, TFT_FRAME_T, TFT_FRAME_FROM_PIN, TFT_PIN_TAIL_H))
    if TFT_FRAME_S > TFT_BODY_H + 1.0:
        W("white frame (%.2f) is more than 1 mm wider than the PCB (%.2f) - re-measure"
          % (TFT_FRAME_S, TFT_BODY_H))
    if TFT_FRAME_L < 45.0 or TFT_FRAME_L > 46.5:
        W("white frame length works out to %.2f (drawing 45.83) - re-measure" % TFT_FRAME_L)
    # pocket must fit on the slope and keep wall behind it
    if TFT_POCKET_H / 2.0 > half - 1.5:
        E("frame pocket %.1f mm is too long for the %.1f mm slope" % (TFT_POCKET_H, SLOPE_LEN))
    if TFT_FRAME_CLR < HOLE_TOL / 2.0:
        W("frame pocket clearance %.2f per side < %s tolerance %.2f - the frame may not drop in"
          % (TFT_FRAME_CLR, PROCESS, HOLE_TOL / 2.0))
    TFT_MIN_T = JLC_THIN + 0.3          # keep 0.3 mm above JLC's limit around the screen
    if TFT_LOCATE_RIB < TFT_MIN_T and TFT_RELIEF_D > 0:
        E("TFT locating rib %.2f mm < %.1f" % (TFT_LOCATE_RIB, TFT_MIN_T))
    if TFT_PILOT_SKIN < TFT_MIN_T:
        E("TFT pilot skin %.2f mm < %.1f" % (TFT_PILOT_SKIN, TFT_MIN_T))
    left = WALL - TFT_POCKET_D - TFT_BEZEL_DEPTH
    if left < max(MIN_WALL, JLC_THIN):
        E("only %.2f mm of wall between the outside bezel and the frame pocket" % left)
    if TFT_RELIEF_D > 0 and TFT_PIN_RELIEF_TO < TFT_PIN_ROW_FROM_EDGE + 1.1:
        E("pin relief ends %.2f mm from the PCB edge but the solder joints reach %.2f mm"
          % (TFT_PIN_RELIEF_TO, TFT_PIN_ROW_FROM_EDGE + 1.1))
    left2 = WALL - TFT_RELIEF_D - TFT_BEZEL_DEPTH
    if TFT_RELIEF_D > 0 and left2 < max(MIN_WALL, JLC_THIN):
        E("only %.2f mm of wall behind the pin relief" % left2)
    N("TFT frame pocket %.2f x %.2f, %.1f deep (%.2f mm per side); wall left %.2f mm under "
      "the frame, %.2f mm under the bezel ring" % (TFT_POCKET_W, TFT_POCKET_H, TFT_POCKET_D,
                                                   TFT_FRAME_CLR, WALL - TFT_POCKET_D, left))
    # window: inside the frame, lit area inside the window
    f0 = TFT_FRAME_CX - TFT_FRAME_L / 2.0
    f1 = TFT_FRAME_CX + TFT_FRAME_L / 2.0
    if -TFT_WINDOW_W / 2 < f0 or TFT_WINDOW_W / 2 > f1 or TFT_WINDOW_H > TFT_FRAME_S:
        E("the window is larger than the white frame - the PCB would show through")
    if TFT_WINDOW_W < TFT_ACTIVE_W + TFT_LIT_UNCERT or TFT_WINDOW_H < TFT_ACTIVE_H:
        E("window is smaller than the lit area")
    N("TFT window %.2f x %.2f over the lit area; it ends %.2f / %.2f mm inside the frame "
      "ends and %.2f mm inside its sides" % (TFT_WINDOW_W, TFT_WINDOW_H,
                                             -TFT_WINDOW_W / 2 - f0, f1 - TFT_WINDOW_W / 2,
                                             (TFT_FRAME_S - TFT_WINDOW_H) / 2))
    # solder joints on the PCB front vs the inside of the wall
    N("TFT: PCB front sits %.2f mm behind the inner wall face; solder joints %.1f mm -> "
      "%s" % (TFT_PCB_FRONT_D - WALL, TFT_PIN_TAIL_H,
              "%.2f mm clear, no relief needed" % TFT_TAIL_GAP if TFT_RELIEF_D == 0 else
              "%.2f mm relief pocket over the pin row" % TFT_RELIEF_D))
    if TFT_BODY_W / 2 + abs(TFT_PCB_DX) > (OUT - 2 * WALL) / 2:
        E("TFT module is wider than the inside of the box")
    # screw bosses at the pin end
    br = TFT_BOSS_D / 2.0
    pocket_edge = TFT_PIN_EDGE_X - TFT_PIN_SIDE * (TFT_FRAME_FROM_PIN - TFT_FRAME_CLR)
    to_pocket = abs(pocket_edge - TFT_SCREW_X) - br
    if to_pocket < 0.2:
        E("TFT screw boss runs into the frame pocket (%.2f mm)" % to_pocket)
    to_relief = TFT_HOLE_SP_S / 2.0 - br - TFT_PIN_RELIEF_S / 2.0
    if TFT_RELIEF_D > 0 and to_relief < 0.5:
        E("TFT screw boss runs into the pin relief (%.2f mm)" % to_relief)
    if TFT_HOLE_SP_S / 2.0 + br > half - 1.5:
        E("TFT screw boss runs off the end of the slope")
    if (TFT_BOSS_D - TFT_PILOT_D) / 2.0 < max(MIN_WALL, JLC_THIN):
        E("TFT boss wall round the pilot is only %.2f mm" % ((TFT_BOSS_D - TFT_PILOT_D) / 2.0))
    if TFT_PILOT_SKIN < JLC_THIN:
        E("only %.2f mm of plastic outside the TFT screw pilots" % TFT_PILOT_SKIN)
    if abs(TFT_SCREW_X) - TFT_PILOT_D / 2.0 < TFT_WINDOW_W / 2.0 + TFT_BEZEL + 0.3:
        E("TFT screw pilot is under the outside bezel - too little plastic left")
    thread = TFT_PILOT_DEPTH
    N("TFT screws (pin end): 2 x M2 self-tapping, %.1f mm pitch, %.2f mm from the pin "
      "edge; boss %.1f dia x %.2f high (%.2f mm from the frame pocket), pilot %.1f x %.2f deep"
      " -> M2 x 4 (PCB 1.2-1.6 + %.1f mm of thread available)"
      % (TFT_HOLE_SP_S, (TFT_BODY_W - TFT_HOLE_SP_L) / 2.0, TFT_BOSS_D, TFT_BOSS_H, to_pocket,
         TFT_PILOT_D, TFT_PILOT_DEPTH, thread))
    if TFT_FRAME_CLR < 0.25:
        W("the 4 screws need the module to shift up to ~0.25 mm in the pocket to line up")
    # far-end bosses: must stay clear of the frame pocket (the white tabs are in it)
    brf = TFT_BOSS_FAR_D / 2.0
    pocket_far = TFT_FAR_EDGE_X + TFT_PIN_SIDE * (TFT_FRAME_END_TO_EDGE - TFT_FRAME_CLR)
    to_pocket_f = abs(pocket_far - TFT_SCREW_FAR_X) - brf
    if to_pocket_f < 0.05:
        E("TFT far-end boss runs into the frame pocket (%.2f mm)" % to_pocket_f)
    if (TFT_BOSS_FAR_D - TFT_PILOT_D) / 2.0 < max(MIN_WALL, JLC_THIN):
        E("TFT far boss wall round the pilot is only %.2f mm"
          % ((TFT_BOSS_FAR_D - TFT_PILOT_D) / 2.0))
    if abs(TFT_SCREW_FAR_X) - TFT_PILOT_D / 2.0 < TFT_WINDOW_W / 2.0 + TFT_BEZEL + 0.3:
        E("TFT far screw pilot is under the outside bezel")
    N("TFT screws (far end): 2 x M2 x 4 self-tapping, boss %.1f dia x %.2f high, %.2f mm "
      "from the frame pocket (white frame ends %.2f mm from the PCB edge, hole centre %.2f)"
      % (TFT_BOSS_FAR_D, TFT_BOSS_H, to_pocket_f, TFT_FRAME_END_TO_EDGE,
         (TFT_BODY_W - TFT_HOLE_SP_L) / 2.0))

    # ---- 3.4b TFT module back + header wires vs parts on the main PCB -----
    # module PCB back face sits WALL + boss + PCB thickness behind the outer slope;
    # allow 3 mm of parts on its back, and 12 mm behind the pin edge (7 mm wide) for
    # the back connector (~9 mm, from your side photo) + the start of the wire bend.
    z_pcb_ = FLOOR + STANDOFF_H + PCB_T
    back_d = TFT_PCB_BACK_D + 3.0
    hdr_d = TFT_PCB_BACK_D + 12.0
    mx0 = CX + TFT_PCB_DX - TFT_BODY_W / 2.0
    mx1 = CX + TFT_PCB_DX + TFT_BODY_W / 2.0
    hx_ = CX + TFT_PIN_EDGE_X - TFT_PIN_SIDE * 3.5      # connector on the back, at the pins
    worst = []
    for ref, x0, y0, x1, y1, h in PCB_PARTS:
        top = z_pcb_ + h
        for nm_, d_, xa, xb in (("module back", back_d, mx0, mx1),
                                ("connector + wires", hdr_d, hx_ - 3.5, hx_ + 3.5)):
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
        N("TFT module: the closest PCB part is %s, %.1f mm below the %s; bend the wires "
          "toward the middle of the box right after the connector" % (r_, g_, n_))

    # ---- 3.5 roof: cradle vs vents vs cable slot vs lid screw posts ------
    # (name, x0, y0, x1, y1, group, layer)  layer: above / through / below the roof.
    # Features of the same group are one design and may touch (the servo mount and
    # its slots, the rib frame); a feature above the roof cannot clash with a rib below.
    roof = [("servo mount", SERVO_CX - SRV_SIDE_X / 2, SRV_OUT_Y0,
             SERVO_CX + SRV_SIDE_X / 2, SRV_OUT_Y1, "servo", "above"),
            ("cable slot", SERVO_CX - SERVO_HOLE_L / 2, SERVO_HOLE_CY - SERVO_HOLE_W / 2,
             SERVO_CX + SERVO_HOLE_L / 2, SERVO_HOLE_CY + SERVO_HOLE_W / 2, "servo", "through"),
            ("servo lead slot", SERVO_CX - SERVO_WIRE_W / 2, SRV_POCKET_Y1,
             SERVO_CX + SERVO_WIRE_W / 2, SERVO_HOLE_CY, "servo", "through")]
    for k, (x0c, y0c, x1c, y1c) in enumerate(servo_cavities()):
        roof.append(("servo end-wall core %d" % k, x0c, y0c, x1c, y1c, "servo", "through"))
    for off in ROOF_VENT_OFFS:
        for s in (-1, 1):
            roof.append(("roof vent x=%+.1f" % (s * off),
                         CX + s * off - VENT_SLOT_W / 2, SERVO_CY - ROOF_VENT_LEN / 2,
                         CX + s * off + VENT_SLOT_W / 2, SERVO_CY + ROOF_VENT_LEN / 2,
                         "vent%d" % (s * off * 10), "through"))
    for x, y in ENC_SCREWS:
        if y > Y_S1:
            roof.append(("lid screw post (%.0f,%.0f)" % (x, y),
                         x - SCREW_BOSS_R, y - SCREW_BOSS_R, x + SCREW_BOSS_R, y + SCREW_BOSS_R,
                         "post%.0f" % x, "through"))
    for k, (x0r, y0r, _z0, x1r, y1r, _z1) in enumerate(roof_rib_boxes()):
        roof.append(("roof rib %d" % k, x0r, y0r, x1r, y1r, "rib", "below"))
    for i in range(len(roof)):
        for j in range(i + 1, len(roof)):
            n1, a0, b0, a1, b1, g1, l1 = roof[i]
            n2, c0, d0, c1, d1, g2, l2 = roof[j]
            if g1 == g2 or {l1, l2} == {"above", "below"}:
                continue
            ox, oy = _ov(a0, a1, c0, c1), _ov(b0, b1, d0, d1)
            if ox > 0 and oy > 0:
                E("roof: '%s' overlaps '%s' by %.2f x %.2f mm" % (n1, n2, ox, oy))
            elif ox > -2.0 and oy > -2.0 and (ox > 0 or oy > 0):
                W("roof: '%s' and '%s' are only %.2f mm apart" % (n1, n2, max(-ox, -oy)))
    for n1, a0, b0, a1, b1, g1, l1 in roof:
        if b0 < Y_S1 + 1.0:
            E("roof: '%s' runs off the front of the flat roof" % n1)
        if g1 == "rib":
            continue                     # ribs are meant to run into the walls
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
    if spk_ring_top() > Z_SPLIT - 1.0:
        E("speaker holder crosses the base/lid split")
    if spk_needs_tie():
        r_o = SPK_D / 2.0 + SPK_CLEAR + SPK_RING_T
        z_edge = SPK_Z + math.sqrt(max(0.0, r_o ** 2 - (SPK_TIE_W / 2.0) ** 2))
        if spk_tie_z0() > z_edge - 0.3:
            E("speaker tie block (%.0f mm wide) does not reach down into the ring at its ends"
              % SPK_TIE_W)
        # beyond the tie the ring must already be >= RIM_CLEAR below the foot
        if z_edge > RAIL_FOOT_Z0 - RIM_CLEAR + 0.01:
            E("speaker ring is still %.2f mm under the rail foot just past the tie block - "
              "widen SPK_TIE_W" % (RAIL_FOOT_Z0 - z_edge))
        N("speaker ring top (z=%.2f) would be %.2f mm under the rail foot: a %.0f mm tie "
          "block joins it up into the foot instead (no hairline gap)"
          % (spk_ring_top(), RAIL_FOOT_Z0 - spk_ring_top(), SPK_TIE_W))
    else:
        N("speaker ring top is %.1f mm below the rail foot" % (RAIL_FOOT_Z0 - spk_ring_top()))
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
      "side), %.1f mm wall, %.1f mm deep, fused to the left wall round its whole circle%s; "
      "bore bottom %.1f mm above the floor."
      % (2 * r_in_, SPK_D, SPK_CLEAR, SPK_RING_T, SPK_RING_H,
         ", on two %.1f mm legs" % SPK_LEG_T if SPK_LEGS else ", no legs", under))
    # v15: a curved ring that dips into a flat floor, or a flat leg face that cuts into the
    # curved ring, meets it at a shallow angle and leaves a long hairline crevice - JLC
    # reports those as "< 0.8 mm".  The ring ends beside the lead channel must stand a
    # REAL gap above the floor instead.
    r_out_ = r_in_ + SPK_RING_T
    z_end = SPK_Z - math.sqrt(r_out_ ** 2 - SPK_CHANNEL_HALF ** 2)
    if z_end - FLOOR < max(SPK_END_CLEAR, JLC_MIN_FEATURE) - 0.01:
        E("speaker ring ends beside the lead channel sit %.2f mm %s the floor - they meet it "
          "at a shallow angle and leave a hairline crevice (JLC flags < 0.8 mm)"
          % (abs(z_end - FLOOR), "above" if z_end > FLOOR else "into"))
    if SPK_LEGS:
        E("speaker legs: a flat leg face cutting into the curved ring leaves a hairline "
          "crevice below the ring's centre (JLC flags < 0.8 mm) - set SPK_LEGS = False")
    N("speaker lead channel: V cut, %.1f mm wide at the ring; the ring ends are radial faces and stand %.2f mm above the floor (no "
      "crevice), and the ring touches nothing but the wall and the rail tie"
      % (2 * SPK_CHANNEL_HALF, z_end - FLOOR))
    N("speaker: push it in from inside the box, face to the wall, then run a bead of hot "
      "glue around the back of the rim")
    N("speaker: from the ring to the PCB edge there is %.1f mm of free space to put it in, "
      "even with the PCB already mounted" % (PCB_OFF - WALL - SPK_RING_H))

    # ---- 3.9 v14 anti-warp rules -----------------------------------------
    # (a) one wall thickness everywhere
    if abs(FLOOR - WALL) > 0.01:
        W("floor %.1f mm differs from the walls %.1f mm - uneven shrinkage" % (FLOOR, WALL))
    # (b) ribs: thick enough to print, thin enough not to become lumps themselves
    for nm, t in (("floor/roof rib", RIB_T), ("wall rib", WALL_RIB_T)):
        if t < max(JLC_THIN, MIN_DETAIL):
            E("%s %.2f mm < JLC thin-wall limit %.1f" % (nm, t, JLC_THIN))
        if t > 0.75 * WALL:
            W("%s %.2f mm > 0.75 x wall: it can leave sink marks" % (nm, t))
    if abs(WALL_RIB_T - RIB_T) < 0.05:
        W("wall ribs and floor ribs are the same thickness: coplanar faces where they meet")
    # (c) no solid section thicker than MAX_SECTION (and none thinner than JLC allows)
    sections = [
        ("servo end-wall shell", SERVO_SHELL_T), ("servo end-wall cap", SERVO_CAP_T),
        ("servo pocket side wall", SERVO_SIDE_WALL),
        ("USB-C step plate / legs", USB_SHELF_SKIN),
        ("speaker ring", SPK_RING_T),
        ("speaker ring at the equator", SPK_RING_T + (SPK_LEG_PROUD if SPK_LEGS else 0)),
        ("PCB post round its pilot", 3.5 - 1.3),
        ("base screw column round its pilot", SCREW_BOSS_R - ENC_PILOT_D / 2.0),
        ("lid screw boss round its hole", SCREW_BOSS_R - SCREW_HOLE / 2.0),
        ("TFT boss round its pilot", (TFT_BOSS_D - TFT_PILOT_D) / 2.0),
        ("wall / floor / roof", max(WALL, FLOOR)),
    ]
    for nm, t in sections:
        if t > MAX_SECTION:
            E("%s is %.2f mm thick (> %.1f): thick lump next to thin walls -> warping"
              % (nm, t, MAX_SECTION))
        if t < JLC_THIN:
            E("%s is only %.2f mm" % (nm, t))
    N("thickest solid section: %s, %.2f mm (limit %.1f; the base rim flange of the rail "
      "is the one intended exception - it stiffens the open edge)"
      % (max(sections, key=lambda s: s[1]) + (MAX_SECTION,)))
    # (d) every hollow must drain (SLA traps liquid resin in closed pockets)
    for k, (x0c, y0c, x1c, y1c) in enumerate(servo_cavities()):
        w_ = min(x1c - x0c, y1c - y0c)
        if w_ < 1.5:
            E("servo end-wall core %d is only %.2f mm wide - resin cannot drain" % (k, w_))
    N("servo end walls: %.1f mm shells, %.1f mm cap under the ears, cores %s mm, open into "
      "the lid so resin drains" % (SERVO_SHELL_T, SERVO_CAP_T,
                                   " / ".join("%.1f x %.1f" % (c[2] - c[0], c[3] - c[1])
                                              for c in servo_cavities())))
    bay = (USB_SHELF_W - 3 * USB_SHELF_SKIN) / 2.0
    bay_h = USB_SHELF_TOP - USB_SHELF_SKIN - FLOOR
    if bay < 2.0 or bay_h < 2.0:
        E("USB-C step bays %.1f x %.1f mm are too small to drain" % (bay, bay_h))
    if bay > 10.0:
        W("USB-C step top plate spans %.1f mm between legs" % bay)
    N("USB-C step: %.1f mm plate on 3 legs, two %.1f x %.1f mm bays open toward the PCB"
      % (USB_SHELF_SKIN, bay, bay_h))
    # (e) floor ribs vs the PCB underside, the speaker lead channel, posts and step
    z_pcb_bot = FLOOR + STANDOFF_H
    if z_pcb_bot - (FLOOR + FLOOR_RIB_H) < 2.0:
        E("floor ribs leave only %.2f mm under the PCB for the through-hole leads"
          % (z_pcb_bot - FLOOR - FLOOR_RIB_H))
    ch_y0, ch_y1 = SPK_Y - SPK_CHANNEL_HALF, SPK_Y + SPK_CHANNEL_HALF
    ch_x1 = WALL + SPK_RING_H + 0.5
    for b in floor_rib_boxes():
        x0, y0, _z0, x1, y1, _z1 = b
        if _ov(x0, x1, WALL, ch_x1 + 1.5) > 0 and _ov(y0, y1, ch_y0, ch_y1) > 0:
            E("a floor rib blocks the speaker lead channel")
        for hx_, hy_ in HOLES + [DUMMY]:
            px_, py_ = hx_ + PCB_OFF, hy_ + PCB_OFF
            if _ov(x0, x1, px_ - 3.5, px_ + 3.5) > -1.0 and _ov(y0, y1, py_ - 3.5, py_ + 3.5) > -1.0:
                W("floor rib runs into / next to the PCB post at (%.0f,%.0f)" % (px_, py_))
        for sx_, sy_ in ENC_SCREWS:
            if _ov(x0, x1, sx_ - SCREW_BOSS_R, sx_ + SCREW_BOSS_R) > 0 and \
               _ov(y0, y1, sy_ - SCREW_BOSS_R, sy_ + SCREW_BOSS_R) > 0:
                E("floor rib runs into the corner screw column at (%.0f,%.0f)" % (sx_, sy_))
    N("floor: %d ribs %.1f x %.1f mm; PCB underside %.1f mm above them (trim the "
      "through-hole leads to <= 2 mm)" % (len(floor_rib_boxes()), RIB_T, FLOOR_RIB_H,
                                          z_pcb_bot - FLOOR - FLOOR_RIB_H))
    # (f) largest unsupported panel: a panel held on 4 sides bends across its SHORT side,
    #     so every grid cell is judged by min(width, depth)
    def cuts(lines, lo, hi):
        p = sorted(set([lo, hi] + [v for v in lines if lo < v < hi]))
        return list(zip(p, p[1:]))

    def worst_cell(xs, ys):
        cells = [(min(x1 - x0, y1 - y0), x1 - x0, y1 - y0) for x0, x1 in xs for y0, y1 in ys]
        return max(cells)

    f_cell = worst_cell(cuts([x for x, _a, _b in FLOOR_RIBS_ALONG_Y], WALL, OUT - WALL),
                        cuts([y for y, _a, _b in FLOOR_RIBS_ALONG_X], WALL, OUT - WALL))
    r_cell = worst_cell(cuts([x for x, _a, _b in ROOF_RIBS_ALONG_Y] +
                             [SERVO_CX - SRV_SIDE_X / 2, SERVO_CX + SRV_SIDE_X / 2],
                             WALL, OUT - WALL),
                        cuts([y for y, _a, _b in ROOF_RIBS_ALONG_X],
                             inner_slope_y(HB - WALL), OUT - WALL))
    sides = {}
    for wname, pos, _b in wall_rib_boxes():
        sides.setdefault(wname, []).append(pos)
    # base walls: the top edge is free, so the span ALONG the wall is what counts
    wmax = max(max(b - a for a, b in cuts(v, R_OUT, OUT - R_OUT)) for v in sides.values())
    for nm, s_ in (("floor panel", f_cell[0]), ("roof panel", r_cell[0]),
                   ("base wall between ribs", wmax)):
        if s_ > MAX_PANEL:
            E("%s spans %.1f mm unsupported (> %.0f)" % (nm, s_, MAX_PANEL))
    N("largest unsupported panels: floor %.0f x %.0f, roof %.0f x %.0f, base walls %.0f mm "
      "between ribs (v13: floor 144 x 144, roof 144 x 92, walls 134)"
      % (f_cell[1], f_cell[2], r_cell[1], r_cell[2], wmax))
    # (g) roof ribs vs everything hanging inside the lid
    rib_z0 = HB - WALL - ROOF_RIB_H
    top_part = max(FLOOR + STANDOFF_H + PCB_T + p[5] for p in PCB_PARTS)
    if rib_z0 - top_part < 5.0:
        E("roof ribs come within %.1f mm of the tallest PCB part" % (rib_z0 - top_part))
    if rib_z0 - (Z_SPLIT + RAIL_H) < 3.0:
        E("roof ribs reach down to the base rail")
    tft_y = max(Y_S0 + UX * sl - NX * d_
                for sl in ((SLOPE_LEN - TFT_BODY_H) / 2.0, (SLOPE_LEN + TFT_BODY_H) / 2.0)
                for d_ in (0.0, TFT_PCB_BACK_D + 3.0))
    if ROOF_RIB_Y0 - tft_y < 3.0:
        E("roof ribs start %.1f mm behind the TFT module (needs >= 3)" % (ROOF_RIB_Y0 - tft_y))
    if ROOF_RIB_Y0 < inner_slope_y(HB - WALL):
        E("roof ribs start in front of the flat roof")
    N("roof: %d ribs %.1f x %.1f mm, %.1f mm behind the TFT module, %.1f mm above the "
      "tallest PCB part" % (len(roof_rib_boxes()), RIB_T, ROOF_RIB_H, ROOF_RIB_Y0 - tft_y,
                            rib_z0 - top_part))

    # (h) nothing may stop just under the rail foot: either merge into it or keep clear.
    #     (v13 failed JLC's "< 0.8 mm wall thickness" here: 0.5 mm and 0.2 mm gaps)
    rim_feats = [("corner column ribs", CORNER_RIB_TOP), ("base wall ribs", WALL_RIB_TOP),
                 ("speaker ring (with tie)" if spk_needs_tie() else "speaker ring",
                  RIB_TOP_IN_RIM if spk_needs_tie() else spk_ring_top())]
    for nm, top in rim_feats:
        merges = RAIL_FOOT_Z0 + 1.5 <= top <= Z_SPLIT - 0.1
        clear = top <= RAIL_FOOT_Z0 - RIM_CLEAR
        if not (merges or clear):
            E("%s end at z=%.2f: %s (JLC flags < 0.8 mm)"
              % (nm, top, ("%.2f mm under the rail foot - a hairline gap" % (RAIL_FOOT_Z0 - top))
                 if top < RAIL_FOOT_Z0 else "only part-way into the rail foot - gaps under "
                 "its upper step"))
    st_ = 2.0 / RAIL_FOOT_STEPS
    if st_ < JLC_MIN_FEATURE:
        E("rail foot steps are %.2f mm - below JLC's %.1f mm minimum" % (st_, JLC_MIN_FEATURE))
    if RIB_TOP_IN_RIM < Z_SPLIT - st_ + 0.1 or SPK_TIE_X1 > WALL + 2.0 - 0.1:
        E("ribs / speaker tie do not end inside the top step of the rail foot")
    N("rim: corner ribs, wall ribs and the speaker ring all run up into the rail foot - "
      "no gaps under the base/lid joint")

    # (i) every small dimension of the BASE against JLC's 0.8 mm report limit
    g_r = [(r, n) for r, n in SPK_GRILLE if n > 0]
    grille_web = min([2 * r * math.sin(math.pi / n) - SPK_GRILLE_D for r, n in g_r if n > 1] +
                     [b[0] - a[0] - SPK_GRILLE_D for a, b in zip(SPK_GRILLE, SPK_GRILLE[1:])])
    base_dims = [
        ("side vent web", VENT_PITCH - VENT_SLOT_W),
        ("speaker grille web", grille_web),
        ("USB-C web behind the pocket", WALL - USB_POCKET_DEPTH),
        ("USB-C pocket rim round the slot", (USB_POCKET_W - USB_SLOT_W) / 2.0),
        ("wall left at the pry notches", WALL - 0.75),
        ("locating rail", RAIL_T),
        ("rail foot step", 2.0 / RAIL_FOOT_STEPS),
        ("floor rib", RIB_T), ("wall rib", WALL_RIB_T), ("corner column rib", 2.4),
        ("PCB post wall", 3.5 - 1.3), ("USB-C step plate / legs", USB_SHELF_SKIN),
        ("speaker ring", min(SPK_RING_T, SPK_LEG_T) if SPK_LEGS else SPK_RING_T),
        ("speaker ring end -> floor gap",
         SPK_Z - math.sqrt((SPK_D / 2 + SPK_CLEAR + SPK_RING_T) ** 2 - SPK_CHANNEL_HALF ** 2)
         - FLOOR),
        ("floor under the corner pilots", FLOOR - 0.1),
        ("gap wall rib - nearest vent", min(abs(p - y) - WALL_RIB_T / 2 - VENT_SLOT_W / 2
                                           for p in WALL_RIBS["left"] + WALL_RIBS["right"]
                                           for a, b in VENT_BLOCKS for y in vent_ys(a, b))),
    ]
    small = min(base_dims, key=lambda d: d[1])
    for nm, v in base_dims:
        if v < JLC_MIN_FEATURE:
            E("base: %s is %.2f mm (< %.1f)" % (nm, v, JLC_MIN_FEATURE))
    N("base: smallest dimension is the %s, %.2f mm (JLC limit %.1f)"
      % (small[0], small[1], JLC_MIN_FEATURE))

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
        N("FDM only: the top of the speaker bore is an arch and needs support. JLC adds "
          "it automatically and it is on an inside face. MJF needs no support at all.")
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
# 3b.  THIN-WALL / HAIRLINE-GAP SCAN OF THE BASE  (numpy + scipy, no CadQuery)
# ============================================================================
# check() tests the dimensions it knows about.  JLC's report also catches what nobody
# wrote down: wedges where a curved face meets a flat one at a shallow angle.  This
# voxel model of the base follows build() step by step and flags every solid region AND
# every gap narrower than 0.8 mm.  Keep it in step with build() when the base changes.
#     python3 app.py --scan          (~20 min at 0.125 mm)
def _scan_impl():
    import time
    import numpy as np
    from scipy import ndimage as ndi
    import sys as _sys
    A = _sys.modules[__name__]
    RES = SCAN_RES
    LIM = 0.8                      # JLC report limit
    R = LIM / 2.0


    def rrect(x, y, x0, y0, w, d, r):
        cx, cy = x0 + w / 2, y0 + d / 2
        qx = np.abs(x - cx) - (w / 2 - r)
        qy = np.abs(y - cy) - (d / 2 - r)
        return (np.hypot(np.maximum(qx, 0), np.maximum(qy, 0)) <= r) & (qx <= r) & (qy <= r)


    def box(X, Y, Z, b):
        x0, y0, z0, x1, y1, z1 = b
        return (X >= x0) & (X <= x1) & (Y >= y0) & (Y <= y1) & (Z >= z0) & (Z <= z1)


    def cyl_z(X, Y, Z, cx, cy, r, z0, z1):
        return ((X - cx) ** 2 + (Y - cy) ** 2 <= r * r) & (Z >= z0) & (Z <= z1)


    def cyl_x(X, Y, Z, cy, cz, r, x0, x1):
        return ((Y - cy) ** 2 + (Z - cz) ** 2 <= r * r) & (X >= x0) & (X <= x1)


    def stadium(u, v, length, width):          # long axis along u
        h = length / 2 - width / 2
        q = np.maximum(np.abs(u) - h, 0)
        return q * q + v * v <= (width / 2) ** 2


    def ring_solid(X, Y, Z, io, ii, h, z0):
        O = A.OUT
        a = rrect(X, Y, io, io, O - 2 * io, O - 2 * io, A.R_OUT - io)
        b = rrect(X, Y, ii, ii, O - 2 * ii, O - 2 * ii, A.R_OUT - ii)
        return a & ~b & (Z >= z0) & (Z <= z0 + h)


    def base(X, Y, Z):
        O, W, F, ZS = A.OUT, A.WALL, A.FLOOR, A.Z_SPLIT
        s = rrect(X, Y, 0, 0, O, O, A.R_OUT) & (Z >= 0) & (Z <= ZS)
        s &= ~(rrect(X, Y, W, W, O - 2 * W, O - 2 * W, A.R_OUT - W) & (Z >= F))
        # joint
        st = 2.0 / A.RAIL_FOOT_STEPS
        for i in range(A.RAIL_FOOT_STEPS):
            s |= ring_solid(X, Y, Z, W - A.RAIL_GAP, W + st * (i + 1), st, A.RAIL_FOOT_Z0 + st * i)
        s |= ring_solid(X, Y, Z, W, W + A.RAIL_T, A.RAIL_H + 0.1, ZS - 0.1)
        for b in A.floor_rib_boxes():
            s |= box(X, Y, Z, b)
        for _w, _p, b in A.wall_rib_boxes():
            s |= box(X, Y, Z, b)
        # PCB posts
        pts = [(p[0] + A.PCB_OFF, p[1] + A.PCB_OFF) for p in A.HOLES + [A.DUMMY]]
        for x, y in pts:
            s |= cyl_z(X, Y, Z, x, y, 3.5, F - 0.01, F + A.STANDOFF_H)
        for x, y in pts:
            s &= ~cyl_z(X, Y, Z, x, y, 1.3, F + 1.0, F + 1.0 + A.STANDOFF_H + 2)
        # corner columns
        for x, y in A.ENC_SCREWS:
            s |= cyl_z(X, Y, Z, x, y, A.SCREW_BOSS_R, F - 0.01, ZS)
            wx = W if x < A.CX else O - W
            wy = W if y < A.CX else O - W
            rh = A.CORNER_RIB_TOP - F
            s |= box(X, Y, Z, (min(x, wx), y - 1.2, F - 0.01, max(x, wx), y + 1.2, F - 0.01 + rh))
            s |= box(X, Y, Z, (x - 1.2, min(y, wy), F - 0.01, x + 1.2, max(y, wy), F - 0.01 + rh))
            s &= ~cyl_z(X, Y, Z, x, y, A.ENC_PILOT_D / 2, F - 0.1, ZS + 0.3)
        # USB-C
        u, v = X - A.USB_X, Z - A.USB_Z
        pk = rrect(X, Z, A.USB_X - A.USB_POCKET_W / 2, A.USB_Z - A.USB_POCKET_H / 2,
                   A.USB_POCKET_W, A.USB_POCKET_H, A.USB_POCKET_R)
        s &= ~(pk & (Y >= O - A.USB_POCKET_DEPTH))
        s &= ~(stadium(u, v, A.USB_SLOT_W, A.USB_SLOT_H) & (Y >= O - W - 1.0))
        plate, legs = A.usb_step_parts()
        s |= box(X, Y, Z, plate)
        for lg in legs:
            s |= box(X, Y, Z, lg)
        # key switch
        s &= ~cyl_x(X, Y, Z, A.SWITCH_Y, A.SWITCH_Z, A.SWITCH_D / 2, O - W - 1.0, O + 1)
        # speaker holder
        s |= holder(X, Y, Z)
        for rad, n in A.SPK_GRILLE:
            for i in range(n):
                a = 2 * np.pi * i / n
                s &= ~cyl_x(X, Y, Z, A.SPK_Y + rad * np.cos(a), A.SPK_Z + rad * np.sin(a),
                            A.SPK_GRILLE_D / 2, -1.0, W + 1.0)
        # side vents
        for y0, y1 in A.VENT_BLOCKS:
            for y in A.vent_ys(y0, y1):
                sl = stadium(Z - A.VENT_SLOT_Z, Y - y, A.VENT_SLOT_H, A.VENT_SLOT_W)
                s &= ~(sl & ((X <= W + 1.0) | (X >= O - W - 1.0)))
        # pry notches
        for ny0 in (-1.0, O - 0.75):
            s &= ~box(X, Y, Z, (A.CX - 8, ny0, ZS - 2.5, A.CX + 8, ny0 + 1.75, ZS + 1))
        extra = getattr(A, "base_extra_vox", None)
        if extra:
            s = extra(s, X, Y, Z)
        return s


    def holder(X, Y, Z):
        W, F = A.WALL, A.FLOOR
        r_in = A.SPK_D / 2 + A.SPK_CLEAR
        r_out = r_in + A.SPK_RING_T
        x0h, hh = W - 0.5, A.SPK_RING_H + 0.5
        rr = (Y - A.SPK_Y) ** 2 + (Z - A.SPK_Z) ** 2
        inx = (X >= x0h) & (X <= x0h + hh)
        h = inx & (rr <= r_out ** 2)
        ly = r_out + A.SPK_LEG_PROUD
        for yl0 in ((A.SPK_Y - ly, A.SPK_Y + ly - A.SPK_LEG_T) if getattr(A, "SPK_LEGS", True) else ()):
            h |= box(X, Y, Z, (x0h, yl0, F - 0.5, x0h + hh, yl0 + A.SPK_LEG_T, A.SPK_Z + 3.0))
        if A.spk_needs_tie():
            h |= box(X, Y, Z, (W - 1.0, A.SPK_Y - A.SPK_TIE_W / 2, A.spk_tie_z0(),
                               A.SPK_TIE_X1, A.SPK_Y + A.SPK_TIE_W / 2, A.RIB_TOP_IN_RIM))
        h &= ~((X >= x0h) & (X <= x0h + hh + 1.0) & (rr <= r_in ** 2))
        h &= ~((X >= W) & (X <= W + hh + 1.0) & (Z >= F) & (Z <= A.SPK_Z) &
               (np.abs(Y - A.SPK_Y) <= (A.SPK_Z - Z) * A.spk_channel_tan()))
        return h


    def scan(tile=15.0, margin=1.6):
        lo = np.array([-0.6, -0.6, -2.0])
        hi = np.array([A.OUT + 0.6, A.OUT + 0.6, A.Z_SPLIT + 4.0])
        thin_s, thin_v = [], []
        xs = np.arange(lo[0], hi[0], tile)
        ys = np.arange(lo[1], hi[1], tile)
        zz = np.arange(lo[2], hi[2] + RES / 2, RES)
        t0 = time.time()
        for tx in xs:
            for ty in ys:
                gx = np.arange(tx - margin, tx + tile + margin, RES)
                gy = np.arange(ty - margin, ty + tile + margin, RES)
                X, Y, Z = np.meshgrid(gx.astype(np.float32), gy.astype(np.float32),
                                      zz.astype(np.float32), indexing="ij")
                S = base(X, Y, Z)
                if not S.any():
                    continue
                m = int(round(margin / RES))
                for solid, out in ((S, thin_s), (~S, thin_v)):
                    core = ndi.distance_transform_edt(solid) * RES >= R
                    dcore = ndi.distance_transform_edt(~core) * RES
                    # plain 90-degree convex edges leave voxels up to ~0.57 mm from the core;
                    # anything farther is genuinely thinner than 0.8 mm
                    bad = solid & (dcore > R + 0.27)
                    bad[:m], bad[-m:], bad[:, :m], bad[:, -m:] = False, False, False, False
                    idx = np.argwhere(bad)
                    if len(idx):
                        out.append(np.c_[gx[idx[:, 0]], gy[idx[:, 1]], zz[idx[:, 2]],
                                         dcore[bad]])
            print("  x tile %.0f done (%.0fs)" % (tx, time.time() - t0), flush=True)
        cat = lambda L: np.vstack(L) if L else np.zeros((0, 4))
        return cat(thin_s), cat(thin_v)


    def clusters(P, gap=0.5):
        if not len(P):
            return []
        q = np.floor(P[:, :3] / gap).astype(int)
        q -= q.min(0)
        g = np.zeros(q.max(0) + 1, bool)
        g[tuple(q.T)] = True
        lab, n = ndi.label(g, np.ones((3, 3, 3)))
        L = lab[tuple(q.T)]
        out = []
        for k in range(1, n + 1):
            Q = P[L == k]
            out.append((len(Q), Q[:, :3].min(0), Q[:, :3].max(0)))
        return sorted(out, key=lambda c: -c[0])


    ts, tv = scan()
    bad = 0
    for nm, P in (("solid thinner than 0.8 mm", ts), ("gap narrower than 0.8 mm", tv)):
        cl = clusters(P)
        bad += len(cl)
        print("  %s: %d spot(s)" % (nm, len(cl)))
        for n, a, b in cl[:40]:
            print("    %6d vox  x %6.1f..%6.1f  y %6.1f..%6.1f  z %5.1f..%5.1f"
                  % (n, a[0], b[0], a[1], b[1], a[2], b[2]))
    return bad


SCAN_RES = 0.125


def scan_base():
    return _scan_impl()


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
    # 2 x 1.0 mm steps = a 45 deg underside (v13 used 0.5 mm steps: below JLC's 0.8 mm
    # minimum feature size)
    for i in range(RAIL_FOOT_STEPS):
        st = 2.0 / RAIL_FOOT_STEPS
        s = ring_solid(WALL - RAIL_GAP, WALL + st * (i + 1), st, RAIL_FOOT_Z0 + st * i)
        rail_foot = s if rail_foot is None else rail_foot.union(s)
    rail_top = ring_solid(WALL, WALL + RAIL_T, RAIL_H + 0.1, Z_SPLIT - 0.1)
    base = base.union(rail_foot).union(rail_top)
    lid = lid.cut(ring_solid(WALL - RAIL_GAP, WALL, RAIL_H + 0.5, Z_SPLIT - 0.1))

    checkpoint("joint + rail")

    # ---- v14: rib grid on the floor + ribs tying the base walls to the floor
    def box6(b):
        x0, y0, z0, x1, y1, z1 = b
        return cq.Workplane("XY").box(x1 - x0, y1 - y0, z1 - z0, centered=False) \
                 .translate((x0, y0, z0))

    for b in floor_rib_boxes():
        base = base.union(box6(b))
    for _w, _p, b in wall_rib_boxes():
        base = base.union(box6(b))

    checkpoint("floor + wall ribs")

    # ---- PCB posts ------------------------------------------------------
    def pcb2box(p):
        return p[0] + PCB_OFF, p[1] + PCB_OFF

    for p in HOLES + [DUMMY]:
        x, y = pcb2box(p)
        base = base.union(cq.Workplane("XY").workplane(offset=FLOOR - 0.01)
                          .center(x, y).circle(3.5).extrude(STANDOFF_H + 0.01))
    for p in HOLES + [DUMMY]:           # v14: the dummy post is cored too (no 7 mm lump)
        x, y = pcb2box(p)
        base = base.cut(cq.Workplane("XY").workplane(offset=FLOOR + 1.0)
                        .center(x, y).circle(1.3).extrude(STANDOFF_H + 2))

    checkpoint("PCB posts")

    # ---- corner screw columns ------------------------------------------
    for x, y in ENC_SCREWS:
        base = base.union(cq.Workplane("XY").workplane(offset=FLOOR - 0.01)
                          .center(x, y).circle(SCREW_BOSS_R).extrude(Z_SPLIT - FLOOR + 0.01))
        wx = WALL if x < CX else OUT - WALL
        wy = WALL if y < CX else OUT - WALL
        rh = CORNER_RIB_TOP - FLOOR      # v14: runs up into the rail foot (v13: 0.5 mm gap)
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
    # v14: 2 mm top plate on 3 legs (built as one solid, joined in one union),
    # bays open toward the PCB so resin drains and nothing is thicker than the wall
    plate, legs = usb_step_parts()
    step = box6(plate)
    for lg in legs:
        step = step.union(box6(lg))
    base = base.union(step)

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
    # v14: two 2.5 mm legs, one each side - no solid lower part.  Their outer faces
    # stand 0.3 mm proud of the ring and their tops 3 mm above its equator, so no face
    # is ever tangent to the ring cylinder (tangent faces break OpenCascade booleans).
    holder = ring
    ly = r_out + SPK_LEG_PROUD
    for yl0 in ((SPK_Y - ly, SPK_Y + ly - SPK_LEG_T) if SPK_LEGS else ()):   # v15: no legs
        holder = holder.union(cq.Workplane("XY")
                              .box(hh, SPK_LEG_T, SPK_Z + 3.0 - FLOOR + 0.5, centered=False)
                              .translate((x0h, yl0, FLOOR - 0.5)))
    # v14: tie the top of the ring up into the rail foot (v13 left a 0.2 mm gap there).
    # It starts inside the wall and stops 0.3 under the split, inside the foot's top step.
    if spk_needs_tie():
        holder = holder.union(cq.Workplane("XY")
                              .box(SPK_TIE_X1 - (WALL - 1.0), SPK_TIE_W,
                                   RIB_TOP_IN_RIM - spk_tie_z0(), centered=False)
                              .translate((WALL - 1.0, SPK_Y - SPK_TIE_W / 2.0, spk_tie_z0())))
    holder = holder.cut(bore)
    # lead channel under the speaker (cut from the holder only, never from the floor)
    # v15: a V-shaped cut aimed at the ring centre, so each ring end is a RADIAL face,
    # square to both ring surfaces (a flat vertical cut left a 63 deg knife edge).  It is
    # wide enough that the ring ends stand SPK_END_CLEAR above the floor (no crevice).
    # The part of the ring below the floor top stays inside the floor.
    vw = (SPK_Z - FLOOR) * spk_channel_tan()
    holder = holder.cut(cq.Workplane("YZ").workplane(offset=WALL)
                        .polyline([(SPK_Y, SPK_Z), (SPK_Y + vw, FLOOR), (SPK_Y - vw, FLOOR)])
                        .close().extrude(hh + 1.0))
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

    # 4 screw bosses (5.0 at the pin end, 4.4 at the far end) + their M2 pilots
    for pts_, d_ in ((TFT_PTS, TFT_BOSS_D), (TFT_PTS_FAR, TFT_BOSS_FAR_D)):
        lid = lid.union(cq.Workplane(slope_plane(WALL - 0.3)).pushPoints(pts_)
                        .circle(d_ / 2.0).extrude(TFT_BOSS_H + 0.3))
    lid = lid.cut(cq.Workplane(cq.Plane(origin=(CX, cy - NX * (WALL + TFT_BOSS_H),
                                                cz - NZ * (WALL + TFT_BOSS_H)),
                                        xDir=(1, 0, 0), normal=(0, NX, NZ)))
                  .pushPoints(TFT_PTS_ALL).circle(TFT_PILOT_D / 2.0).extrude(TFT_PILOT_DEPTH))
    # pocket for the white frame, cut AFTER the bosses so nothing can ever stand in it
    lid = lid.cut(cq.Workplane(slope_plane(WALL - TFT_POCKET_D)).center(TFT_FRAME_CX, 0)
                  .rect(TFT_POCKET_W, TFT_POCKET_H).extrude(TFT_POCKET_D + TFT_FRAME_T + 0.5))
    # shallow relief over the solder joints of the pin row (only if they need it)
    if TFT_RELIEF_D > 0:
        lid = lid.cut(cq.Workplane(slope_plane(WALL - TFT_RELIEF_D)).center(TFT_RELIEF_CX, 0)
                      .rect(TFT_RELIEF_W, TFT_PIN_RELIEF_S).extrude(TFT_RELIEF_D + 0.3))

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
    # v14: hollow the two 8 mm end walls into 2.5 mm shells.  The cores are cut up
    # from inside the lid through the roof, so they open into the lid and drain; the
    # top 2.5 mm stays solid as the rest for the ears.
    z_cav0 = HB - WALL - 1.0
    for x0c, y0c, x1c, y1c in servo_cavities():
        lid = lid.cut(cq.Workplane("XY").box(x1c - x0c, y1c - y0c,
                                             SRV_LEDGE_Z - SERVO_CAP_T - z_cav0,
                                             centered=False).translate((x0c, y0c, z_cav0)))

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

    # ---- v14: rib frame under the flat roof ------------------------------
    for b in roof_rib_boxes():
        x0, y0, z0, x1, y1, z1 = b
        lid = lid.union(cq.Workplane("XY").box(x1 - x0, y1 - y0, z1 - z0, centered=False)
                        .translate((x0, y0, z0)))

    checkpoint("roof ribs")

    # ---- pry notches in the base top edge -------------------------------
    for ny0 in (-1.0, OUT - 0.75):
        base = base.cut(cq.Workplane("XY").box(16.0, 1.75, 2.5, centered=(True, False, False))
                        .translate((CX, ny0, Z_SPLIT - 2.5)))

    checkpoint("pry notches")

    return base, lid


def build_tft_model():
    """A model of YOUR TFT module + its 4 screws, sitting where they go in
    the closed box (preview only - never printed).  Returns [(name, workplane, rgba)]."""
    import cadquery as cq
    mid = SLOPE_LEN / 2.0
    cy, cz = Y_S0 + UX * mid, HF + UZ * mid

    def pl(depth):
        """plane parallel to the slope, 'depth' mm in from the outside, x across the box,
        y up the slope, normal pointing OUT of the box (extrude(-t) goes inward)"""
        return cq.Plane(origin=(CX, cy - NX * depth, cz - NZ * depth),
                        xDir=(1, 0, 0), normal=(0, NX, NZ))

    def slab(depth, cx_, cy_, w, h, t):
        return cq.Workplane(pl(depth)).center(cx_, cy_).rect(w, h).extrude(-t)

    s_ = TFT_PIN_SIDE
    frame_front = WALL - TFT_POCKET_D
    parts = []

    # white frame: a ring (1.0 mm sides, 1.5 mm at the pin end, 7 mm at the far end)
    frame = slab(frame_front, TFT_FRAME_CX, 0, TFT_FRAME_L, TFT_FRAME_S, TFT_FRAME_T)
    op_pin, op_far, op_side = 1.5, 7.0, 1.0
    op_l = TFT_FRAME_L - op_pin - op_far
    op_cx = (TFT_PIN_EDGE_X - s_ * (TFT_FRAME_FROM_PIN + op_pin + op_l / 2.0))
    frame = frame.cut(slab(frame_front + 0.01, op_cx, 0, op_l, TFT_FRAME_S - 2 * op_side,
                           TFT_FRAME_T + 0.1))
    parts.append(("tft_white_frame", frame, (0.96, 0.96, 0.94, 1.0)))
    # glass (black) filling the frame opening, and the lit area just in front of it
    parts.append(("tft_glass", slab(frame_front + 0.02, op_cx, 0, op_l - 0.1,
                                    TFT_FRAME_S - 2 * op_side - 0.1, 1.9),
                  (0.05, 0.05, 0.06, 1.0)))
    parts.append(("tft_lit_area", slab(frame_front - 0.05, 0, 0, TFT_ACTIVE_W, TFT_ACTIVE_H,
                                       0.06), (0.10, 0.45, 0.85, 1.0)))
    # module PCB with its 4 holes
    pcb = slab(TFT_PCB_FRONT_D, TFT_PCB_DX, 0, TFT_BODY_W, TFT_BODY_H, TFT_PCB_T)
    holes = TFT_PTS_ALL
    pcb = pcb.cut(cq.Workplane(pl(TFT_PCB_FRONT_D - 0.5)).pushPoints(holes)
                  .circle(TFT_HOLE_INNER_D / 2.0).extrude(-(TFT_PCB_T + 1.0)))
    parts.append(("tft_pcb", pcb, (0.08, 0.08, 0.10, 1.0)))
    # 8 solder joints on the front, at the pin edge
    pin_x = TFT_PIN_EDGE_X - s_ * TFT_PIN_ROW_FROM_EDGE
    pins = [(pin_x, (i - 3.5) * 2.54) for i in range(8)]
    parts.append(("tft_solder_joints",
                  cq.Workplane(pl(TFT_PCB_FRONT_D)).pushPoints(pins).circle(0.6)
                  .extrude(TFT_PIN_TAIL_H if TFT_RELIEF_D > 0
                           else min(TFT_PIN_TAIL_H, TFT_PCB_FRONT_D - WALL - 0.1)),
                  (0.75, 0.75, 0.75, 1.0)))
    # 8-pin connector on the back, wires leaving straight back (stub)
    con_cx = TFT_PIN_EDGE_X - s_ * 3.5
    parts.append(("tft_connector", slab(TFT_PCB_BACK_D, con_cx, 0, 6.0, 20.0, 9.0),
                  (0.95, 0.93, 0.85, 1.0)))
    parts.append(("tft_wires", slab(TFT_PCB_BACK_D + 9.0, con_cx, 0, 3.0, 16.0, 3.0),
                  (0.85, 0.20, 0.15, 1.0)))
    # 4 M2 screw heads (3.8 dia x 1.3) on the back of the module
    heads = cq.Workplane(pl(TFT_PCB_BACK_D)).pushPoints(TFT_PTS_ALL).circle(1.9).extrude(-1.3)
    parts.append(("tft_screws", heads, (0.70, 0.70, 0.72, 1.0)))
    for nm, wp, _ in parts:
        sol = wp.solids().vals()
        if not sol or not all(x.isValid() for x in sol):
            raise RuntimeError("TFT model part '%s' did not build" % nm)
    print("  ok  TFT model             %d parts (preview only)" % len(parts))
    return parts


def build_parts_preview():
    """Models of the key switch, speaker, USB-C module and main PCB (all 31 KiCad parts)
    where they sit in the closed box - preview only.  Returns [(name, workplane, rgba)]."""
    import cadquery as cq

    def comp(solids):
        return cq.Workplane("XY").newObject([cq.Compound.makeCompound(solids)])

    def box(cx, cy, z0, w, d, h):
        return cq.Workplane("XY").box(w, d, h, centered=(True, True, False)) \
                 .translate((cx, cy, z0)).val()

    def cyl_z(cx, cy, z0, dia, h):
        return cq.Workplane("XY").workplane(offset=z0).center(cx, cy).circle(dia / 2.0) \
                 .extrude(h).val()

    def cyl_x(x0, y, z, dia, length):          # axis along +x, starting at x0
        return cq.Workplane("YZ").workplane(offset=x0).center(y, z).circle(dia / 2.0) \
                 .extrude(length).val()

    out = []
    # ---- key switch on the right wall (panel = the wall, inner face at OUT - WALL)
    wi = OUT - WALL
    chrome, dark = (0.80, 0.81, 0.83, 1.0), (0.15, 0.15, 0.16, 1.0)
    sw = [cyl_x(OUT, SWITCH_Y, SWITCH_Z, 15.0, 4.0),                     # front cap (outside)
          cyl_x(wi - 12.0, SWITCH_Y, SWITCH_Z, 12.0, 12.0 + WALL),        # M12 thread
          cyl_x(wi - 12.0 - 7.0, SWITCH_Y, SWITCH_Z, 12.5, 7.0)]          # body
    sw.append(cq.Workplane("YZ").workplane(offset=wi - 2.5).center(SWITCH_Y, SWITCH_Z)
              .polygon(6, 16.0).extrude(2.5).val())                       # nut, inside
    out.append(("key_switch", comp(sw), chrome))
    out.append(("key_switch_slot", comp([cq.Workplane("YZ").workplane(offset=OUT + 3.9)
                                         .center(SWITCH_Y, SWITCH_Z).rect(1.6, 6.0)
                                         .extrude(0.2).val()]), dark))
    out.append(("key_switch_base", comp([cyl_x(wi - 21.0, SWITCH_Y, SWITCH_Z, 11.0, 2.0)]),
                (0.10, 0.60, 0.25, 1.0)))
    out.append(("key_switch_lugs", comp([box(wi - 23.5, SWITCH_Y + dy, SWITCH_Z - 1.5, 5.0, 0.5,
                                             3.0) for dy in (-3.0, 3.0)]), chrome))
    # ---- speaker in its ring on the left wall, face to the wall
    sx0 = WALL
    out.append(("speaker_frame", comp([cyl_x(sx0, SPK_Y, SPK_Z, SPK_D, 1.5)]),
                (0.12, 0.12, 0.13, 1.0)))
    out.append(("speaker_cone", comp([cyl_x(sx0 + 0.6, SPK_Y, SPK_Z, SPK_D - 3.0, 1.2)]),
                (0.22, 0.22, 0.24, 1.0)))
    out.append(("speaker_back", comp([cyl_x(sx0 + 1.5, SPK_Y, SPK_Z, 22.0, 4.0)]),
                (0.70, 0.70, 0.72, 1.0)))
    out.append(("speaker_tabs", comp([box(sx0 + 3.5, SPK_Y + dy, SPK_Z - SPK_D / 2.0 + 0.5,
                                          3.0, 1.5, 3.0) for dy in (-2.0, 2.0)]),
                (0.10, 0.55, 0.25, 1.0)))
    # ---- USB-C trigger module on its step at the rear wall
    usb_pcb_z = USB_SHELF_TOP + USB_GLUE_GAP
    sock_l, sock_w = 7.35, 8.94
    sock_y1 = OUT - WALL - 0.2                       # socket mouth just behind the wall
    pcb_l, pcb_w = 14.0, 11.0
    pcb_y1 = sock_y1 - 1.5                           # the socket overhangs the PCB by 1.5
    out.append(("usb_c_module_pcb", comp([box(USB_X, pcb_y1 - pcb_l / 2.0, usb_pcb_z, pcb_w,
                                              pcb_l, USB_MOD_PCB_T)]),
                (0.45, 0.15, 0.60, 1.0)))
    out.append(("usb_c_socket", comp([box(USB_X, sock_y1 - sock_l / 2.0,
                                          usb_pcb_z + USB_MOD_PCB_T, sock_w, sock_l,
                                          USB_SOCKET_H)]), chrome))
    out.append(("usb_c_chip", comp([box(USB_X - 1.5, pcb_y1 - 8.0, usb_pcb_z + USB_MOD_PCB_T,
                                        3.0, 3.0, 0.9)]), dark))
    # ---- main PCB on its posts, with every part from the KiCad file
    zb = FLOOR + STANDOFF_H
    zt = zb + PCB_T
    board = cq.Workplane("XY").box(PCB, PCB, PCB_T, centered=False).translate((PCB_OFF, PCB_OFF, zb))
    for hx, hy in HOLES:
        board = board.cut(cq.Workplane("XY").workplane(offset=zb - 0.5)
                          .center(PCB_OFF + hx, PCB_OFF + hy).circle(1.6).extrude(PCB_T + 1.0))
    out.append(("main_pcb", board, (0.05, 0.35, 0.18, 1.0)))
    rounds = {r: (cx, cy, d) for r, cx, cy, d in PCB_ROUND}
    groups = {}

    def add(g, solid):
        groups.setdefault(g, []).append(solid)

    for ref, x0, y0, x1, y1, h in PCB_PARTS:
        cx, cy = (x0 + x1) / 2.0, (y0 + y1) / 2.0
        w, d = max(2.0, x1 - x0 - 4.0), max(2.0, y1 - y0 - 4.0)   # body, not the keep-out
        if ref in rounds:
            rx, ry, dia = rounds[ref]
            add("electrolytics", cyl_z(rx, ry, zt, dia, h))
        elif ref == "U1":                                          # DevKit in its socket
            for sx_ in (-1, 1):
                add("headers", box(cx + sx_ * 12.7, cy, zt, 2.5, y1 - y0 - 6.0, 8.5))
            add("devkit", box(cx, cy, zt + 8.5, 28.0, y1 - y0 - 4.0, 1.6))
            add("rf_module", box(cx, y0 + 2.0 + 12.75, zt + 10.1, 18.0, 25.5, 3.2))
        elif ref == "U2":                                          # MP3 player module
            add("headers", box(cx, cy, zt, w, d, 8.5))
            add("mp3_module", box(cx, cy, zt + 8.5, 20.5, 20.5, 1.6))
            add("rf_module", box(cx, cy + 4.0, zt + 10.1, 11.0, 12.0, 1.9))
        elif ref.startswith("J8"):
            add("terminal", box(cx, cy, zt, w, d, h))
        elif ref.startswith("J"):
            add("jst", box(cx, cy, zt, w, d, 7.0))
        elif ref.startswith("C"):
            add("ceramics", box(cx, cy, zt, min(w, 5.0), min(d, 5.0) if d < w else d * 0.6, h))
        elif ref.startswith("R"):
            add("resistors", box(cx, cy, zt, min(w, 2.5) if h > 5 else w, min(d, 2.5) if h > 5
                                 else 2.5, h))
        elif ref.startswith("F"):
            add("fuse", box(cx, cy, zt, w, min(d, 3.5), h))
        elif ref.startswith("D"):                                  # axial diodes, lying flat
            long_ = max(w, d) * 0.6
            add("black_parts", box(cx, cy, zt, 3.5 if w < d else long_, long_ if w < d else 3.5,
                                   3.5))
        else:                                                      # TO-92
            add("black_parts", box(cx, cy, zt, 4.5, 3.6, 5.0))
    colours = {"electrolytics": (0.10, 0.12, 0.35, 1.0), "headers": (0.08, 0.08, 0.08, 1.0),
               "devkit": (0.05, 0.05, 0.08, 1.0), "rf_module": (0.78, 0.78, 0.80, 1.0),
               "mp3_module": (0.10, 0.25, 0.60, 1.0), "terminal": (0.10, 0.45, 0.20, 1.0),
               "jst": (0.95, 0.93, 0.86, 1.0), "ceramics": (0.90, 0.60, 0.15, 1.0),
               "resistors": (0.85, 0.75, 0.55, 1.0), "fuse": (0.95, 0.80, 0.10, 1.0),
               "black_parts": (0.10, 0.10, 0.10, 1.0)}
    for g, sols in groups.items():
        out.append(("pcb_" + g, comp(sols), colours[g]))
    for nm, wp, _ in out:
        sol = wp.solids().vals()
        if not sol or not all(x.isValid() for x in sol):
            raise RuntimeError("preview part '%s' did not build" % nm)
    print("  ok  parts preview         key switch, speaker, USB-C, main PCB (%d parts)"
          % len(PCB_PARTS))
    return out


# ============================================================================
# 5.  EXPORT
# ============================================================================
def export(base, lid, tft=None):
    """Writes these files:
         1_enclosure_base_plus_lid_v15  - the closed box WITH the TFT, key switch, speaker,
                                          USB-C module and main PCB inside
                                          in place (preview only - do not order it)
         2_enclosure_lid_v15            - the lid, lying open side down as printed
         3_enclosure_base_v15           - the base
       each as STEP and STL, plus
         4_preview_in_colour_v15.step   - the same as file 1, with colours (open it in
                                          Fusion 360 / FreeCAD / an online STEP viewer)"""
    import os
    import cadquery as cq

    d = os.path.join(os.path.dirname(os.path.abspath(__file__)), "outputs")
    os.makedirs(d, exist_ok=True)
    lid_flat = lid.translate((0, 0, -Z_SPLIT))
    extra = []
    for _, wp, _c in (tft or []):
        extra += wp.solids().vals()
    both = cq.Workplane("XY").newObject(base.solids().vals() + lid.solids().vals() + extra)

    written = []
    for name, shape in (("1_enclosure_base_plus_lid_v15", both),
                        ("2_enclosure_lid_v15", lid_flat),
                        ("3_enclosure_base_v15", base)):
        for ext, kw in (("step", {}), ("stl", dict(tolerance=0.01, angularTolerance=0.05))):
            p = os.path.join(d, "%s.%s" % (name, ext))
            cq.exporters.export(shape, p, **kw)
            written.append(p)

    if tft:
        try:
            assy = cq.Assembly(name="enclosure_v15")
            assy.add(base, name="base", color=cq.Color(0.92, 0.92, 0.90, 1.0))
            assy.add(lid, name="lid", color=cq.Color(0.80, 0.82, 0.85, 1.0))
            for nm, wp, rgba in tft:
                assy.add(wp, name=nm, color=cq.Color(*rgba))
            p = os.path.join(d, "4_preview_in_colour_v15.step")
            if hasattr(assy, "export"):
                assy.export(p)
            else:
                assy.save(p)
            written.append(p)
        except Exception as ex:                      # preview only - never block the parts
            print("  ..  colour preview skipped: %s" % ex)

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
    if "--scan" in sys.argv:
        print("\nscanning the base for walls / gaps under 0.8 mm ...")
        sys.exit(1 if scan_base() else 0)
    print("\nbuilding ...")
    b, l = build()
    t = []
    for fn in (build_tft_model, build_parts_preview):
        try:
            t += fn()
        except Exception as ex:                      # preview only - never block the parts
            print("  ..  %s skipped: %s" % (fn.__name__, ex))
    export(b, l, t)