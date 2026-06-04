#!/bin/bash
# Check EMF+ (-p) rendering: EMF+ records must contribute SVG output.
#
# Assertions:
#  1. On an EA dual EMF with EMF+ inline-color fills, output with -p differs
#     from output without -p (EMF+ layer contributes something).
#  2. The -p output is still DTD-valid SVG.
#  3. Output without -p is unchanged (no EMF+ leakage when disabled).
#  4. (if ../../samples/image4.emf exists) semi-transparent EMF+ fills
#     (inline ARGB with alpha 0x80) are emitted as fill-opacity attributes.
#
# Options: -n disable valgrind, -v verbose

cd "$(dirname "$0")" || exit 1
. ./colors.sh

CONV=../../build/emf2svg-conv
DTD=./svg11-flat.dtd
OUTDIR=../out/emfplus
VALGRIND="valgrind --tool=memcheck --leak-check=yes --error-exitcode=42"
VERBOSE=0
RET=0

while getopts ":hnv" opt; do
    case $opt in
    h)
        echo "usage: $0 [-n (no valgrind)] [-v (verbose)]"
        exit 0
        ;;
    n) VALGRIND="" ;;
    v) VERBOSE=1 ;;
    *) ;;
    esac
done

[ ! -x "$CONV" ] && echo -e "${RED}[ERROR]${NC} $CONV not found (build first)" && exit 1
rm -rf "$OUTDIR"
mkdir -p "$OUTDIR"

say() { [ "$VERBOSE" -eq 1 ] && echo "$@"; return 0; }
ok() { echo -e "${GREEN}[OK]${NC}    $1"; }
ko() {
    echo -e "${RED}[FAIL]${NC}  $1"
    RET=1
}

# ---------------------------------------------------------------- tracked corpus
F=./emf-ea/EA-test-file-020.emf
$VALGRIND "$CONV" -p -i "$F" -o "$OUTDIR/020-p.svg" || ko "conv -p failed on $F"
"$CONV" -i "$F" -o "$OUTDIR/020.svg" || ko "conv failed on $F"

if cmp -s "$OUTDIR/020-p.svg" "$OUTDIR/020.svg"; then
    ko "EMF+ contributes nothing: -p output identical on $F"
else
    ok "-p output differs from no-p on $F (EMF+ layer contributes)"
fi

if xmllint --dtdvalid "$DTD" --noout "$OUTDIR/020-p.svg" 2>/dev/null; then
    ok "-p output is DTD-valid on $F"
else
    ko "-p output is NOT DTD-valid on $F"
fi

# EMF+ fills on 020 are inline white rects; with -p at least one element
# carrying the comment marker (or any EMF+-emitted node) must exist.
if grep -q 'EMF+' "$OUTDIR/020-p.svg"; then
    ok "EMF+ marker comments present in -p output on $F"
else
    ko "no EMF+ marker comments in -p output on $F"
fi

# FillPath corpus file: paths from the EMF+ object table.
F=./emf-ea/EA-test-file-101.emf
$VALGRIND "$CONV" -p -i "$F" -o "$OUTDIR/101-p.svg" || ko "conv -p failed on $F"
if xmllint --dtdvalid "$DTD" --noout "$OUTDIR/101-p.svg" 2>/dev/null; then
    ok "-p output is DTD-valid on $F"
else
    ko "-p output is NOT DTD-valid on $F"
fi

# Regression for kakwa/libemf2svg#12 (LibreOffice tdf#107034 attachment 132406):
# an EMF+ document that produced an empty SVG before EMF+ rendering existed.
F=./emf-ea/emfplus-issue12-tdf107034.emf
if [ -f "$F" ]; then
    $VALGRIND "$CONV" -p -i "$F" -o "$OUTDIR/issue12-p.svg" || ko "conv -p failed on $F"
    "$CONV" -i "$F" -o "$OUTDIR/issue12-nop.svg"
    np=$(grep -oc '<path' "$OUTDIR/issue12-nop.svg")
    wp=$(grep -oc '<path' "$OUTDIR/issue12-p.svg")
    if [ "$wp" -gt 100 ] && grep -q 'EMF+ ' "$OUTDIR/issue12-p.svg"; then
        ok "issue#12 EMF+ document now renders content under -p ($wp paths, was $np)"
    else
        ko "issue#12 EMF+ document still renders empty under -p ($wp paths)"
    fi
    xmllint --noout "$OUTDIR/issue12-p.svg" 2>/dev/null &&
        ok "issue#12 -p output well-formed" || ko "issue#12 -p output malformed"
fi

# ---------------------------------------------------------------- crafted fixtures
# (regenerate with: python3 tests/resources/gen_emfplus_fixtures.py)

# FIX B: EMF+ fill inside an open GDI path must not corrupt the d= attribute.
F=./emf-ea/EA-emfplus-fill-in-open-path.emf
if [ -f "$F" ]; then
    $VALGRIND "$CONV" -p -i "$F" -o "$OUTDIR/fill-in-path.svg" || ko "conv -p failed on $F"
    if xmllint --noout "$OUTDIR/fill-in-path.svg" 2>/dev/null; then
        ok "EMF+ fill in open GDI path yields well-formed SVG"
    else
        ko "EMF+ fill in open GDI path corrupts the SVG (inPath guard missing)"
    fi
fi

# FIX A: oversized Elements must not cause a use-after-free / double-free.
F=./emf-corrupted/emfplus-fillrects-huge-elements.emf
if [ -f "$F" ]; then
    if $VALGRIND "$CONV" -p -i "$F" -o "$OUTDIR/huge-elements.svg" 2>/dev/null; then
        ok "FillRects with huge Elements is memory-safe under -p"
    else
        ko "FillRects with huge Elements crashes/leaks under -p (exit $?)"
    fi
fi

# FIX C: a truncated EMF+ record must not abort the whole conversion.
F=./emf-corrupted/emfplus-fillrects-truncated-size.emf
if [ -f "$F" ]; then
    "$CONV" -p -i "$F" -o "$OUTDIR/trunc.svg"
    if [ -s "$OUTDIR/trunc.svg" ] && xmllint --noout "$OUTDIR/trunc.svg" 2>/dev/null; then
        ok "truncated EMF+ record is skipped, file still rendered"
    else
        ko "truncated EMF+ record aborted the whole conversion"
    fi
fi

# ---------------------------------------------------------------- object brushes
# SolidColor object-table brush referenced by a btype=0 FillRects.
F=./emf-ea/EA-emfplus-solid-brush.emf
if [ -f "$F" ]; then
    $VALGRIND "$CONV" -p -i "$F" -o "$OUTDIR/solid-brush.svg" || ko "conv -p failed on $F"
    if grep -q 'fill="#9a8484"' "$OUTDIR/solid-brush.svg"; then
        ok "SolidColor object brush resolved to fill on $F"
    else
        ko "SolidColor object brush not resolved on $F"
    fi
    xmllint --dtdvalid "$DTD" --noout "$OUTDIR/solid-brush.svg" 2>/dev/null &&
        ok "solid-brush output DTD-valid" || ko "solid-brush output NOT DTD-valid"
fi

# LinearGradient object-table brush -> <linearGradient> def + url() reference.
F=./emf-ea/EA-emfplus-linear-gradient.emf
if [ -f "$F" ]; then
    $VALGRIND "$CONV" -p -i "$F" -o "$OUTDIR/lin-grad.svg" || ko "conv -p failed on $F"
    if grep -q '<linearGradient' "$OUTDIR/lin-grad.svg" &&
        grep -q 'fill="url(#' "$OUTDIR/lin-grad.svg"; then
        ok "LinearGradient brush emitted as linearGradient + url() on $F"
    else
        ko "LinearGradient brush not resolved on $F"
    fi
    if grep -q 'stop-color:#bfda88' "$OUTDIR/lin-grad.svg" &&
        grep -q 'stop-color:#cce2a0' "$OUTDIR/lin-grad.svg"; then
        ok "gradient start/end stop colors present on $F"
    else
        ko "gradient stop colors missing on $F"
    fi
    xmllint --dtdvalid "$DTD" --noout "$OUTDIR/lin-grad.svg" 2>/dev/null &&
        ok "linear-gradient output DTD-valid" || ko "linear-gradient output NOT DTD-valid"
fi

# DrawPath: a Path stroked with a Pen -> SVG path with stroke + fill:none.
F=./emf-ea/EA-emfplus-drawpath.emf
if [ -f "$F" ]; then
    $VALGRIND "$CONV" -p -i "$F" -o "$OUTDIR/drawpath.svg" || ko "conv -p failed on $F"
    if grep -q 'stroke="#69738c"' "$OUTDIR/drawpath.svg" &&
        grep -q 'fill="none"' "$OUTDIR/drawpath.svg"; then
        ok "DrawPath stroked with pen color + fill:none on $F"
    else
        ko "DrawPath not stroked on $F"
    fi
    xmllint --dtdvalid "$DTD" --noout "$OUTDIR/drawpath.svg" 2>/dev/null &&
        ok "drawpath output DTD-valid" || ko "drawpath output NOT DTD-valid"
fi

# FillPolygon: an inline-color polygon -> filled, closed SVG path.
F=./emf-ea/EA-emfplus-fillpolygon.emf
if [ -f "$F" ]; then
    $VALGRIND "$CONV" -p -i "$F" -o "$OUTDIR/fillpoly.svg" || ko "conv -p failed on $F"
    if grep -q 'EMF+ FillPolygon' "$OUTDIR/fillpoly.svg" &&
        grep -q 'fill="#3366cc"' "$OUTDIR/fillpoly.svg" &&
        grep -q 'L 50.0000,90.0000 Z' "$OUTDIR/fillpoly.svg"; then
        ok "FillPolygon emitted as filled closed path on $F"
    else
        ko "FillPolygon not filled on $F"
    fi
    xmllint --dtdvalid "$DTD" --noout "$OUTDIR/fillpoly.svg" 2>/dev/null &&
        ok "fillpolygon output DTD-valid" || ko "fillpolygon output NOT DTD-valid"
fi

# DrawRects: a rectangle stroked with a Pen -> stroked path, fill:none.
F=./emf-ea/EA-emfplus-drawrects.emf
if [ -f "$F" ]; then
    $VALGRIND "$CONV" -p -i "$F" -o "$OUTDIR/drawrects.svg" || ko "conv -p failed on $F"
    if grep -q 'EMF+ DrawRects' "$OUTDIR/drawrects.svg" &&
        grep -q 'stroke="#69738c"' "$OUTDIR/drawrects.svg" &&
        grep -q 'fill="none"' "$OUTDIR/drawrects.svg"; then
        ok "DrawRects stroked with pen + fill:none on $F"
    else
        ko "DrawRects not stroked on $F"
    fi
    xmllint --dtdvalid "$DTD" --noout "$OUTDIR/drawrects.svg" 2>/dev/null &&
        ok "drawrects output DTD-valid" || ko "drawrects output NOT DTD-valid"
fi

# DrawLines: a polyline stroked with a Pen -> stroked path, fill:none.
F=./emf-ea/EA-emfplus-drawlines.emf
if [ -f "$F" ]; then
    $VALGRIND "$CONV" -p -i "$F" -o "$OUTDIR/drawlines.svg" || ko "conv -p failed on $F"
    if grep -q 'EMF+ DrawLines' "$OUTDIR/drawlines.svg" &&
        grep -q 'stroke="#69738c"' "$OUTDIR/drawlines.svg" &&
        grep -q 'L 100.0000,50.0000' "$OUTDIR/drawlines.svg"; then
        ok "DrawLines emitted as stroked polyline on $F"
    else
        ko "DrawLines not stroked on $F"
    fi
    xmllint --dtdvalid "$DTD" --noout "$OUTDIR/drawlines.svg" 2>/dev/null &&
        ok "drawlines output DTD-valid" || ko "drawlines output NOT DTD-valid"
fi

# A FillPolygon with a non-finite point must be fully suppressed.
F=./emf-corrupted/emfplus-fillpolygon-nan-point.emf
if [ -f "$F" ]; then
    $VALGRIND "$CONV" -p -i "$F" -o "$OUTDIR/fp-nan.svg" || ko "conv -p failed on $F"
    if grep -qiE 'nan|inf' "$OUTDIR/fp-nan.svg"; then
        ko "non-finite FillPolygon point leaked nan/inf into the SVG"
    else
        ok "non-finite FillPolygon point suppressed (no nan/inf)"
    fi
fi

# A DrawLines polyline with a non-finite point must be fully suppressed.
F=./emf-corrupted/emfplus-drawlines-nan-point.emf
if [ -f "$F" ]; then
    $VALGRIND "$CONV" -p -i "$F" -o "$OUTDIR/dl-nan.svg" || ko "conv -p failed on $F"
    if grep -qiE 'nan|inf' "$OUTDIR/dl-nan.svg"; then
        ko "non-finite DrawLines point leaked nan/inf into the SVG"
    else
        ok "non-finite DrawLines point suppressed (no nan/inf)"
    fi
fi

# A truncated Pen declaring dashed-line-data must not over-read.
F=./emf-corrupted/emfplus-pen-truncated-dldata.emf
if [ -f "$F" ]; then
    if $VALGRIND "$CONV" -p -i "$F" -o "$OUTDIR/trunc-pen.svg" 2>/dev/null; then
        ok "truncated dashed Pen is memory-safe under -p"
    else
        ko "truncated dashed Pen crashes/over-reads under -p (exit $?)"
    fi
fi

# A non-finite gradient RectF must not leak nan/inf into the SVG.
F=./emf-corrupted/emfplus-gradient-nan-rect.emf
if [ -f "$F" ]; then
    $VALGRIND "$CONV" -p -i "$F" -o "$OUTDIR/grad-nan.svg" || ko "conv -p failed on $F"
    if grep -qiE 'nan|inf' "$OUTDIR/grad-nan.svg"; then
        ko "non-finite gradient leaked nan/inf into the SVG"
    else
        ok "non-finite gradient RectF produces no nan/inf coordinates"
    fi
fi

# Dual-mode arbitration must NOT erase content the EMF+ layer cannot reproduce
# (raster images, text). These tracked dual files carry their picture in GDI
# records whose EMF+ counterparts are unimplemented; -p must keep them.
F=./emf/test-150.emf
if [ -f "$F" ]; then
    "$CONV" -p -i "$F" -o "$OUTDIR/t150-p.svg"
    n=$(grep -oc '<image' "$OUTDIR/t150-p.svg")
    if [ "$n" -ge 50 ]; then
        ok "raster images preserved under -p on test-150 ($n images)"
    else
        ko "raster images lost under -p on test-150 ($n, expected >=50)"
    fi
fi
F=./emf/test-155.emf
if [ -f "$F" ]; then
    "$CONV" -i "$F" -o "$OUTDIR/t155.svg"
    "$CONV" -p -i "$F" -o "$OUTDIR/t155-p.svg"
    np=$(wc -c < "$OUTDIR/t155.svg"); wp=$(wc -c < "$OUTDIR/t155-p.svg")
    if [ "$wp" -ge $((np / 2)) ]; then
        ok "image-only dual file not collapsed under -p on test-155 ($wp vs $np bytes)"
    else
        ko "image-only dual file collapsed under -p on test-155 ($wp vs $np bytes)"
    fi
fi

# ---------------------------------------------------------------- no -p: unchanged
F=./emf-ea/EA-test-file-020.emf
"$CONV" -i "$F" -o "$OUTDIR/020-again.svg"
if cmp -s "$OUTDIR/020.svg" "$OUTDIR/020-again.svg"; then
    ok "no-p output is deterministic on $F"
else
    ko "no-p output is not deterministic on $F"
fi
if grep -q 'EMF+' "$OUTDIR/020.svg"; then
    ko "EMF+ leakage: marker present without -p on $F"
else
    ok "no EMF+ leakage without -p on $F"
fi

# ---------------------------------------------------------------- alpha shadows (local samples)
F=../../samples/image4.emf
if [ -f "$F" ]; then
    $VALGRIND "$CONV" -p -i "$F" -o "$OUTDIR/image4-p.svg" || ko "conv -p failed on $F"
    NSEMI=$(grep -oE 'fill-opacity="0\.50[0-9]*"' "$OUTDIR/image4-p.svg" | wc -l)
    say "semi-transparent fills found: $NSEMI"
    # image4 has 2 FillRects + 2 FillPath shadows with inline RGBA{A3,A3,A3,80}
    if [ "$NSEMI" -ge 2 ]; then
        ok "FillRects shadows emitted with fill-opacity=0.50 on image4 ($NSEMI)"
    else
        ko "expected >=2 semi-transparent fills on image4, got $NSEMI"
    fi
    if [ "$NSEMI" -ge 4 ]; then
        ok "FillPath shadows emitted with fill-opacity=0.50 on image4 ($NSEMI)"
    else
        ko "expected >=4 semi-transparent fills (FillRects+FillPath) on image4, got $NSEMI"
    fi
    if grep -q 'fill="#a3a3a3" *fill-opacity' "$OUTDIR/image4-p.svg" ||
        grep -qE 'fill-opacity="0\.50[0-9]*"[^>]*fill="#a3a3a3"|fill="#a3a3a3"[^>]*fill-opacity="0\.50' "$OUTDIR/image4-p.svg"; then
        ok "shadow color #a3a3a3 carries the alpha on image4"
    else
        ko "no #a3a3a3 + fill-opacity pairing found on image4"
    fi
    # Phase 5: the duplicate opaque GDI shadow squares must be suppressed
    # (EMF+ active, outside a GetDC window) while the EMF+ soft shadows and the
    # GetDC-played text survive.
    if [ "$(grep -c 'fill:#a3a3a3"' "$OUTDIR/image4-p.svg")" -eq 0 ]; then
        ok "opaque GDI fallback shadows suppressed on image4 (dark squares gone)"
    else
        ko "opaque GDI fallback shadows still present on image4"
    fi
    if grep -qE '<text|<!\[CDATA' "$OUTDIR/image4-p.svg"; then
        ok "GetDC-played text preserved on image4"
    else
        ko "text lost on image4 (over-suppressed GDI)"
    fi

    # The first EMF+ shadow rect (world UL{500,-406} WH{309,119} through
    # SetWorldTransform {1,0,0,-1,-196,-23}) must land exactly on the
    # GDI-fallback geometry: device rect [304,613]x[264,383].
    if grep -q 'M 304.0000,383.0000 L 613.0000,383.0000 L 613.0000,264.0000 L 304.0000,264.0000 Z' \
        "$OUTDIR/image4-p.svg"; then
        ok "EMF+ shadow rect matches GDI-fallback device geometry on image4"
    else
        ko "EMF+ shadow rect geometry wrong or missing on image4"
    fi
    if xmllint --dtdvalid "$DTD" --noout "$OUTDIR/image4-p.svg" 2>/dev/null; then
        ok "-p output is DTD-valid on image4"
    else
        ko "-p output is NOT DTD-valid on image4"
    fi
else
    echo "[SKIP] $F not present, skipping alpha-shadow assertions"
fi

exit $RET
