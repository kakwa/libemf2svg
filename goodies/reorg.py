#!/usr/bin/env python

import sys
import re

fname='./src/lib/emf2svg.c'

org_func = {
'Bitmap': [
'EMR_ALPHABLEND',
'EMR_BITBLT',
'EMR_MASKBLT',
'EMR_PLGBLT',
'EMR_SETDIBITSTODEVICE',
'EMR_STRETCHBLT',
'EMR_STRETCHDIBITS',
'EMR_TRANSPARENTBLT',
],

'Clipping': [
'EMR_EXCLUDECLIPRECT',
'EMR_EXTSELECTCLIPRGN',
'EMR_INTERSECTCLIPRECT',
'EMR_OFFSETCLIPRGN',
'EMR_SELECTCLIPPATH',
],

'Comment': [
'EMR_COMMENT',
'EMR_COMMENT_EMFPLUS',
'EMR_COMMENT_EMFSPOOL',
'EMR_COMMENT_PUBLIC',
'EMR_COMMENT_BEGINGROUP',
'EMR_COMMENT_ENDGROUP',
'EMR_COMMENT_MULTIFORMATS',
'EMR_COMMENT_WINDOWS_METAFILE',
],

'Control': [
'EMR_EOF',
'EMR_HEADER',
],

'Path': [
'EMR_BEGINPATH',
'EMR_ENDPATH',
'EMR_FLATTENPATH',
'EMR_ABORTPATH',
'EMR_WIDENPATH',
'',
],

'Drawing': [
'EMR_ANGLEARC',
'EMR_ARC',
'EMR_ARCTO',
'EMR_CHORD',
'EMR_CLOSEFIGURE',
'EMR_ELLIPSE',
'EMR_EXTFLOODFILL',
'EMR_EXTTEXTOUTA',
'EMR_EXTTEXTOUTW',
'EMR_FILLPATH',
'EMR_FILLRGN',
'EMR_FRAMERGN',
'EMR_GRADIENTFILL',
'EMR_LINETO',
'EMR_PAINTRGN',
'EMR_PIE',
'EMR_POLYBEZIER',
'EMR_POLYBEZIER16',
'EMR_POLYBEZIERTO',
'EMR_POLYBEZIERTO16',
'EMR_POLYDRAW',
'EMR_POLYDRAW16',
'EMR_POLYGON',
'EMR_POLYGON16',
'EMR_POLYLINE',
'EMR_POLYLINE16',
'EMR_POLYLINETO',
'EMR_POLYLINETO16',
'EMR_POLYPOLYGON',
'EMR_POLYPOLYGON16',
'EMR_POLYPOLYLINE',
'EMR_POLYPOLYLINE16',
'EMR_POLYTEXTOUTA',
'EMR_POLYTEXTOUTW',
'EMR_RECTANGLE',
'EMR_ROUNDRECT',
'EMR_SETPIXELV',
'EMR_SMALLTEXTOUT',
'EMR_STROKEANDFILLPATH',
'EMR_STROKEPATH',
],

'Escape': [
'EMR_DRAWESCAPE',
'EMR_EXTESCAPE',
'EMR_NAMEDESCAPE',
],

'Object Creation': [
'EMR_CREATEBRUSHINDIRECT',
'EMR_CREATECOLORSPACE',
'EMR_CREATECOLORSPACEW',
'EMR_CREATEDIBPATTERNBRUSHPT',
'EMR_CREATEMONOBRUSH',
'EMR_CREATEPALETTE',
'EMR_CREATEPEN',
'EMR_EXTCREATEFONTINDIRECTW',
'EMR_EXTCREATEPEN',
],

'Object Manipulation': [
'EMR_COLORCORRECTPALETTE',
'EMR_DELETECOLORSPACE',
'EMR_DELETEOBJECT',
'EMR_REALIZEPALETTE',
'EMR_RESIZEPALETTE',
'EMR_SELECTOBJECT',
'EMR_SELECTPALETTE',
'EMR_SETCOLORSPACE',
'EMR_SETPALETTEENTRIES',
],
'OpenGL': [
'EMR_GLSBOUNDEDRECORD',
'EMR_GLSRECORD',
],

'Path Bracket': [
],

'State Record': [
'EMR_COLORMATCHTOTARGETW',
'EMR_FORCEUFIMAPPING',
'EMR_INVERTRGN',
'EMR_MOVETOEX',
'EMR_PIXELFORMAT',
'EMR_RESTOREDC',
'EMR_SAVEDC',
'EMR_SCALEVIEWPORTEXTEX',
'EMR_SCALEWINDOWEXTEX',
'EMR_SETARCDIRECTION',
'EMR_SETBKCOLOR',
'EMR_SETBKMODE',
'EMR_SETBRUSHORGEX',
'EMR_SETCOLORADJUSTMENT',
'EMR_SETICMMODE',
'EMR_SETICMPROFILEA',
'EMR_SETICMPROFILEW',
'EMR_SETLAYOUT',
'EMR_SETLINKEDUFIS',
'EMR_SETMAPMODE',
'EMR_SETMAPPERFLAGS',
'EMR_SETMETARGN',
'EMR_SETMITERLIMIT',
'EMR_SETPOLYFILLMODE',
'EMR_SETROP2',
'EMR_SETSTRETCHBLTMODE',
'EMR_SETTEXTALIGN',
'EMR_SETTEXTCOLOR',
'EMR_SETTEXTJUSTIFICATION',
'EMR_SETVIEWPORTEXTEX',
'EMR_SETVIEWPORTORGEX',
'EMR_SETWINDOWEXTEX',
'EMR_SETWINDOWORGEX',
],

'Transform': [
'EMR_MODIFYWORLDTRANSFORM',
'EMR_SETWORLDTRANSFORM',
],
}

functions = {}

header = """#ifdef __cplusplus
extern "C" {
#endif

#ifndef DARWIN
#define _POSIX_C_SOURCE 200809L
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h> /* for offsetof() macro */
#include <string.h>
#include <math.h>
#include "uemf.h"
#include "emf2svg.h"
#include "emf2svg_private.h"
#include "emf2svg_print.h"
#include "pmf2svg.h"
#include "pmf2svg_print.h"

"""

footer = """

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
"""

with open(fname) as f:
    content = f.readlines()

func_name=""
in_func=False
for line in content:
    if re.match(r'^[^\t^ ^#^/].*\(', line):
        in_func=True
        func_name = re.search(r'.*\ +\*?([A-Za-z_0-9]+)\(.*', line).group(1)
        print func_name
    if in_func:
        if func_name not in functions:
            functions[func_name] = ''
        functions[func_name] = functions[func_name] + line
    if re.match(r'^}$',line):
        in_func=False
    #print(line)
f.close()

for rtype in org_func:
    filename = './src/lib/emf2svg_rec_' + re.sub(' ', '_', rtype.lower()) + '.c'
    #print filename
    f = open(filename, 'w')
    f.write(header)
    for record in org_func[rtype]:
        func = 'U_' +  re.sub('_', '', record) + '_draw'
        if func in functions:
            f.write(functions[func])
            del functions[func]
    f.write(footer)
    f.close()


f = open('./src/lib/emf2svg.c', 'w')
f.write(header)
for func in sorted(['U_emf_onerec_analyse', 'U_emf_onerec_draw', 'emf2svg']):
    f.write(functions[func])
    del functions[func]
f.write(footer)
f.close()

f = open('./src/lib/emf2svg_utils.c', 'w')
f.write(header)
for func in sorted(functions):
    f.write(functions[func])
f.write(footer)
f.close()
