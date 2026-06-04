#!/usr/bin/env python3
"""Generate minimal EMF/EMF+ regression fixtures for the EMF+ (-p) renderer.

Run from the repository root:
    python3 tests/resources/gen_emfplus_fixtures.py

Produces three self-contained dual EMF/EMF+ files:
  emf-ea/EA-emfplus-fill-in-open-path.emf
      Well-formed. An EMF+ FillRects (inline ARGB) sits inside an EMR_COMMENT
      that appears while a GDI path is open (BEGINPATH..ENDPATH). The renderer
      must NOT emit its own element into the open "<path d=..." attribute.
  emf-corrupted/emfplus-fillrects-huge-elements.emf
      Malformed. FillRects declares a huge Elements count; the vendored
      U_PMF_VARRECTS_get frees its buffer but leaves the pointer dangling.
      The renderer must not read or double-free it.
  emf-corrupted/emfplus-fillrects-truncated-size.emf
      Malformed. The EMF+ record declares a Size that overruns the buffer.
      The renderer must skip just that record, not abort the whole file.
"""
import os
import struct
import sys

EMR_HEADER, EMR_EOF, EMR_MOVETOEX = 1, 14, 27
EMR_LINETO, EMR_BEGINPATH, EMR_ENDPATH, EMR_STROKEPATH, EMR_COMMENT = 54, 59, 60, 64, 70
EMFPLUS_IDENT = 0x2B464D45  # "EMF+"
PMR_FILLRECTS = 0x400A      # U_PMR_FILLRECTS | U_PMR_RECFLAG
PMR_OBJECT = 0x4008         # U_PMR_OBJECT | U_PMR_RECFLAG
PMR_DRAWPATH = 0x4015       # U_PMR_DRAWPATH | U_PMR_RECFLAG
PMR_DRAWRECTS = 0x400B      # U_PMR_DRAWRECTS | U_PMR_RECFLAG
PMR_DRAWLINES = 0x400D      # U_PMR_DRAWLINES | U_PMR_RECFLAG
PMR_FILLPOLYGON = 0x400C    # U_PMR_FILLPOLYGON | U_PMR_RECFLAG
PPF_B, PPF_C, PPF_D = 0x8000, 0x4000, 0x0400  # inline ARGB, int16 coords, closed
PPT_START, PPT_LINE = 0x00, 0x01
OT_BRUSH, OT_PEN, OT_PATH = 0x01, 0x02, 0x03
BT_SOLID, BT_LINEARGRADIENT = 0x00, 0x04
GVER = 0xDBC02000           # an EmfPlusGraphicsVersion value (unused by renderer)


def rec(itype, body):
    nsize = 8 + len(body)
    assert nsize % 4 == 0, (itype, nsize)
    return struct.pack("<II", itype, nsize) + body


def header(nrec):
    b = struct.pack("<iiii", 0, 0, 200, 200)        # rclBounds (device px)
    b += struct.pack("<iiii", 0, 0, 20000, 20000)   # rclFrame (.01 mm)
    b += struct.pack("<I", 0x464D4520)              # " EMF" signature
    b += struct.pack("<I", 0x00010000)              # nVersion
    b += struct.pack("<I", 0)                       # nBytes (patched below)
    b += struct.pack("<I", nrec)                    # nRecords
    b += struct.pack("<H", 1)                       # nHandles
    b += struct.pack("<H", 0)                       # sReserved
    b += struct.pack("<I", 0)                       # nDescription
    b += struct.pack("<I", 0)                       # offDescription
    b += struct.pack("<I", 0)                       # nPalEntries
    b += struct.pack("<ii", 1000, 1000)             # szlDevice
    b += struct.pack("<ii", 200, 200)               # szlMillimeters
    h = rec(EMR_HEADER, b)
    assert len(h) == 88, len(h)
    return h


def fillrects(elements=1, size_override=None, nrects=1):
    """One EMF+ FillRects record, inline ARGB (50% red), int16 rects."""
    data = struct.pack("<I", 0x80FF0000) + struct.pack("<I", elements)
    for _ in range(nrects):
        data += struct.pack("<hhhh", 20, 20, 60, 40)
    size = size_override if size_override is not None else 12 + len(data)
    return struct.pack("<HHII", PMR_FILLRECTS, PPF_B | PPF_C, size, len(data)) + data


def fillrects_brush(brush_id, nrects=1):
    """FillRects referencing an object-table brush (btype=0, int16 rects)."""
    data = struct.pack("<I", brush_id) + struct.pack("<I", nrects)
    for _ in range(nrects):
        data += struct.pack("<hhhh", 20, 20, 60, 40)
    return struct.pack("<HHII", PMR_FILLRECTS, PPF_C, 12 + len(data), len(data)) + data


def obj_record(obj_id, otype, obj_data):
    """EMF+ Object record holding a (non-continued) object of any type."""
    flags = (obj_id & 0xFF) | ((otype & 0x3F) << 8)
    return struct.pack("<HHII", PMR_OBJECT, flags, 12 + len(obj_data),
                       len(obj_data)) + obj_data


def obj_brush(obj_id, brush_data):
    return obj_record(obj_id, OT_BRUSH, brush_data)


def pen_solid(bgra, width=1.0):
    """A Pen object embedding a SolidColor brush (no optional pen data)."""
    pendata = struct.pack("<IIf", 0, 0, width)            # Flags=0, Unit=0, Width
    brush = brush_solid(bgra)
    return struct.pack("<II", GVER, 0) + pendata + brush  # Version, Type=0


def path_line(p0, p1):
    """A Path object: two int16 points (Start, Line)."""
    d = struct.pack("<IIHH", GVER, 2, PPF_C, 0)           # Version, Count, Flags, reserved
    d += struct.pack("<hh", *p0) + struct.pack("<hh", *p1)
    d += struct.pack("<BB", PPT_START, PPT_LINE)
    return d


def drawpath(path_id, pen_id):
    """DrawPath record: PathID in flags, PenID in data."""
    data = struct.pack("<I", pen_id)
    flags = path_id & 0xFF
    return struct.pack("<HHII", PMR_DRAWPATH, flags, 12 + len(data),
                       len(data)) + data


def drawrects(pen_id, rects):
    """DrawRects record: PenID in flags, int16 rects (x,y,w,h)."""
    data = struct.pack("<I", len(rects))
    for rc in rects:
        data += struct.pack("<hhhh", *rc)
    flags = (pen_id & 0xFF) | PPF_C
    return struct.pack("<HHII", PMR_DRAWRECTS, flags, 12 + len(data),
                       len(data)) + data


def fillpolygon_inline(argb, points):
    """FillPolygon with an inline ARGB color (btype=1), absolute float points."""
    data = struct.pack("<I", argb) + struct.pack("<I", len(points))
    for p in points:
        data += struct.pack("<ff", *p)
    return struct.pack("<HHII", PMR_FILLPOLYGON, PPF_B, 12 + len(data),
                       len(data)) + data


def drawlines(pen_id, points, closed=False):
    """DrawLines record: PenID in flags, absolute float points."""
    data = struct.pack("<I", len(points))
    for p in points:
        data += struct.pack("<ff", *p)
    flags = (pen_id & 0xFF) | (PPF_D if closed else 0)
    return struct.pack("<HHII", PMR_DRAWLINES, flags, 12 + len(data),
                       len(data)) + data


def brush_solid(bgra):
    return struct.pack("<II", GVER, BT_SOLID) + bytes(bgra)


def brush_lineargradient(rect, start_bgra, end_bgra):
    d = struct.pack("<II", GVER, BT_LINEARGRADIENT)
    d += struct.pack("<I", 0)              # BrushData Flags (U_BD_None)
    d += struct.pack("<i", 0)              # WrapMode
    d += struct.pack("<ffff", *rect)       # RectF: X, Y, Width, Height
    d += bytes(start_bgra) + bytes(end_bgra)
    d += struct.pack("<II", 0, 0)          # Reserved1, Reserved2
    return d


def emfplus_comment(pmf_bytes):
    cbData = 4 + len(pmf_bytes)  # cIdent + EMF+ data
    body = struct.pack("<I", cbData) + struct.pack("<I", EMFPLUS_IDENT) + pmf_bytes
    while (8 + len(body)) % 4:
        body += b"\x00"
    return rec(EMR_COMMENT, body)


def eof():
    return rec(EMR_EOF, struct.pack("<III", 0, 0, 0))


def assemble(body_records):
    blob = header(1 + len(body_records)) + b"".join(body_records)
    return blob[:48] + struct.pack("<I", len(blob)) + blob[52:]


def write(path, blob):
    with open(path, "wb") as f:
        f.write(blob)
    print("wrote %s (%d bytes)" % (path, len(blob)))


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(__file__)

    # FIX B: fill emitted while a GDI path is open.
    write(os.path.join(root, "emf-ea", "EA-emfplus-fill-in-open-path.emf"),
          assemble([
              rec(EMR_BEGINPATH, b""),
              rec(EMR_MOVETOEX, struct.pack("<ii", 10, 10)),
              rec(EMR_LINETO, struct.pack("<ii", 150, 150)),
              emfplus_comment(fillrects()),
              rec(EMR_ENDPATH, b""),
              rec(EMR_STROKEPATH, struct.pack("<iiii", 0, 0, 200, 200)),
              eof(),
          ]))

    # FIX A: declared Elements far exceeds the record's data.
    write(os.path.join(root, "emf-corrupted", "emfplus-fillrects-huge-elements.emf"),
          assemble([emfplus_comment(fillrects(elements=100000, nrects=1)), eof()]))

    # FIX C: declared Size overruns the buffer.
    write(os.path.join(root, "emf-corrupted", "emfplus-fillrects-truncated-size.emf"),
          assemble([emfplus_comment(fillrects(size_override=0xFFFF)), eof()]))

    # Phase 2: a SolidColor object-table brush referenced by FillRects (btype=0).
    # Brush #2 = opaque #9a8484 (BGRA bytes 0x84,0x84,0x9a,0xff).
    write(os.path.join(root, "emf-ea", "EA-emfplus-solid-brush.emf"),
          assemble([
              emfplus_comment(obj_brush(2, brush_solid((0x84, 0x84, 0x9A, 0xFF))) +
                              fillrects_brush(2)),
              eof(),
          ]))

    # Phase 2: FillPolygon with an inline color -> filled, closed SVG path.
    write(os.path.join(root, "emf-ea", "EA-emfplus-fillpolygon.emf"),
          assemble([
              emfplus_comment(
                  fillpolygon_inline(0xFF3366CC,
                                     [(10, 10), (100, 20), (50, 90)])),
              eof(),
          ]))

    # Phase 2 (strokes): DrawRects -> stroked rectangle outline.
    write(os.path.join(root, "emf-ea", "EA-emfplus-drawrects.emf"),
          assemble([
              emfplus_comment(
                  obj_record(0, OT_PEN, pen_solid((0x8C, 0x73, 0x69, 0xFF))) +
                  drawrects(0, [(20, 20, 60, 40)])),
              eof(),
          ]))

    # Phase 2 (strokes): DrawLines -> stroked polyline.
    write(os.path.join(root, "emf-ea", "EA-emfplus-drawlines.emf"),
          assemble([
              emfplus_comment(
                  obj_record(0, OT_PEN, pen_solid((0x8C, 0x73, 0x69, 0xFF))) +
                  drawlines(0, [(10, 10), (100, 50), (50, 100)])),
              eof(),
          ]))

    # Phase 2: a FillPolygon with a non-finite point must be suppressed.
    write(os.path.join(root, "emf-corrupted", "emfplus-fillpolygon-nan-point.emf"),
          assemble([
              emfplus_comment(
                  fillpolygon_inline(0xFF3366CC,
                                     [(10, 10), (float("nan"), 20), (50, 90)])),
              eof(),
          ]))

    # Phase 2 (strokes): a DrawLines polyline containing a non-finite point
    # must be suppressed entirely (no "nan"/"inf" in the SVG).
    nan_pt = float("nan")
    write(os.path.join(root, "emf-corrupted", "emfplus-drawlines-nan-point.emf"),
          assemble([
              emfplus_comment(
                  obj_record(0, OT_PEN, pen_solid((0x8C, 0x73, 0x69, 0xFF))) +
                  drawlines(0, [(10, 10), (nan_pt, 50), (50, 100)])),
              eof(),
          ]))

    # Phase 2 (strokes): a truncated Pen that declares dashed-line-data but
    # contains none must not over-read (the vendored U_PMF_LEN_PENDATA walk is
    # unbounded). 12-byte pen data: Version + Type + PenData Flags=U_PD_DLData.
    U_PD_DLDATA = 0x0100
    write(os.path.join(root, "emf-corrupted", "emfplus-pen-truncated-dldata.emf"),
          assemble([
              emfplus_comment(
                  obj_record(0, OT_PEN, struct.pack("<III", GVER, 0, U_PD_DLDATA)) +
                  obj_record(1, OT_PATH, path_line((10, 10), (100, 100))) +
                  drawpath(1, 0)),
              eof(),
          ]))

    # Phase 2 (strokes): a Pen + Path + DrawPath -> stroked SVG path outline.
    # Pen #0 = #69738c solid, width 1; Path #1 = line (10,10)-(100,100).
    write(os.path.join(root, "emf-ea", "EA-emfplus-drawpath.emf"),
          assemble([
              emfplus_comment(
                  obj_record(0, OT_PEN, pen_solid((0x8C, 0x73, 0x69, 0xFF))) +
                  obj_record(1, OT_PATH, path_line((10, 10), (100, 100))) +
                  drawpath(1, 0)),
              eof(),
          ]))

    # Phase 2: a LinearGradient brush with a non-finite RectF must not leak
    # "nan"/"inf" into the SVG gradient coordinates (DTD validation, being
    # CDATA, would not catch it).
    nan = float("nan")
    write(os.path.join(root, "emf-corrupted", "emfplus-gradient-nan-rect.emf"),
          assemble([
              emfplus_comment(
                  obj_brush(3, brush_lineargradient(
                      (nan, nan, nan, nan),
                      (0x88, 0xDA, 0xBF, 0xFF), (0xA0, 0xE2, 0xCC, 0xFF))) +
                  fillrects_brush(3)),
              eof(),
          ]))

    # Phase 2: a LinearGradient object-table brush referenced by FillRects.
    # Brush #3 = #bfda88 -> #cce2a0 over rect (20,20)-(80,60).
    write(os.path.join(root, "emf-ea", "EA-emfplus-linear-gradient.emf"),
          assemble([
              emfplus_comment(
                  obj_brush(3, brush_lineargradient(
                      (20.0, 20.0, 60.0, 40.0),
                      (0x88, 0xDA, 0xBF, 0xFF), (0xA0, 0xE2, 0xCC, 0xFF))) +
                  fillrects_brush(3)),
              eof(),
          ]))


if __name__ == "__main__":
    main()
