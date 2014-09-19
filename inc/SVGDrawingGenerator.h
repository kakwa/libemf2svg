/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

/* vss2svg 
 * fork of RVNGSVGDrawingGenerator from librevenge
 */

// <<<<<<<<<<<<<<<<<< START ORIGINAL HEADER >>>>>>>>>>>>>>>>>>>>>>>>>>>

/* librevenge
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2006 Ariya Hidayat (ariya@kde.org)
 * Copyright (C) 2005 Fridrich Strba (fridrich.strba@bluewin.ch)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 */

// <<<<<<<<<<<<<<<<<< END ORIGINAL HEADER >>>>>>>>>>>>>>>>>>>>>>>>>>>

//#ifndef librevenge::RVNGSVGDRAWINGGENERATOR_H
//#define librevenge::RVNGSVGDRAWINGGENERATOR_H

#include <librevenge/librevenge-api.h>

#include <librevenge/RVNGDrawingInterface.h>
#include <librevenge/RVNGStringVector.h>

namespace vss2svg
{

struct SVGDrawingGeneratorPrivate;

class REVENGE_API SVGDrawingGenerator : public librevenge::RVNGDrawingInterface
{
public:
	SVGDrawingGenerator(librevenge::RVNGStringVector &vec, const librevenge::RVNGString &nmspace);
	~SVGDrawingGenerator();

	void startDocument(const librevenge::RVNGPropertyList &propList);
	void endDocument();
	void setDocumentMetaData(const librevenge::RVNGPropertyList &propList);
	void defineEmbeddedFont(const librevenge::RVNGPropertyList &propList);
	void startPage(const librevenge::RVNGPropertyList &propList);
	void endPage();
	void startMasterPage(const librevenge::RVNGPropertyList &propList);
	void endMasterPage();
	void startLayer(const librevenge::RVNGPropertyList &propList);
	void endLayer();
	void startEmbeddedGraphics(const librevenge::RVNGPropertyList &propList);
	void endEmbeddedGraphics();

	void openGroup(const librevenge::RVNGPropertyList &propList);
	void closeGroup();

	void setStyle(const librevenge::RVNGPropertyList &propList);

	void drawRectangle(const librevenge::RVNGPropertyList &propList);
	void drawEllipse(const librevenge::RVNGPropertyList &propList);
	void drawPolyline(const librevenge::RVNGPropertyList &propList);
	void drawPolygon(const librevenge::RVNGPropertyList &propList);
	void drawPath(const librevenge::RVNGPropertyList &propList);
	void drawGraphicObject(const librevenge::RVNGPropertyList &propList);
	void drawConnector(const librevenge::RVNGPropertyList &propList);
	void startTextObject(const librevenge::RVNGPropertyList &propList);
	void endTextObject();

	void startTableObject(const librevenge::RVNGPropertyList &propList);
	void openTableRow(const librevenge::RVNGPropertyList &propList);
	void closeTableRow();
	void openTableCell(const librevenge::RVNGPropertyList &propList);
	void closeTableCell();
	void insertCoveredTableCell(const librevenge::RVNGPropertyList &propList);
	void endTableObject();

	void openOrderedListLevel(const librevenge::RVNGPropertyList &propList);
	void closeOrderedListLevel();

	void openUnorderedListLevel(const librevenge::RVNGPropertyList &propList);
	void closeUnorderedListLevel();
	void openListElement(const librevenge::RVNGPropertyList &propList);
	void closeListElement();

	void defineParagraphStyle(const librevenge::RVNGPropertyList &propList);
	void openParagraph(const librevenge::RVNGPropertyList &propList);
	void closeParagraph();

	void defineCharacterStyle(const librevenge::RVNGPropertyList &propList);
	void openSpan(const librevenge::RVNGPropertyList &propList);
	void closeSpan();

	void openLink(const librevenge::RVNGPropertyList &propList);
	void closeLink();

	void insertTab();
	void insertSpace();
	void insertText(const librevenge::RVNGString &text);
	void insertLineBreak();
	void insertField(const librevenge::RVNGPropertyList &propList);

private:
	SVGDrawingGenerator(const SVGDrawingGenerator &);
	SVGDrawingGenerator &operator=(const SVGDrawingGenerator &);
	SVGDrawingGeneratorPrivate *m_pImpl;
    double textLastX;
    double textLastFontSize;
    bool   textIsParagraph;
    bool   firtLineWritten;
    bool   textNewLine;
    int    textSpaceCounter;
};

}

//#endif // librevenge::RVNGSVGDRAWINGGENERATOR_H

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
