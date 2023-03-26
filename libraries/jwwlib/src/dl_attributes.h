/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/

#ifndef DL_ATTRIBUTES_H
#define DL_ATTRIBUTES_H

#include <string>
#include <utility>
using std::string;

#include "dl_codes.h"

/**
 * Storing and passing around attributes. Attributes
 * are the layer name, color, width and line type.
 *
 * @author Andrew Mustun
 */
class DL_Attributes {

public:

    /**
     * Default constructor.
     */
    DL_Attributes() : m_color(0), m_width(0), m_lineType("BYLAYER") {
    }



    /**
     * Constructor for DXF attributes.
     *
     * @param layer Layer name for this entity or NULL for no layer
     *              (every entity should be on a named layer!).
     * @param color Color number (0..256). 0 = BYBLOCK, 256 = BYLAYER.
     * @param width Line thickness. Defaults to zero. -1 = BYLAYER, 
     *               -2 = BYBLOCK, -3 = default width
     * @param lineType Line type name or "BYLAYER" or "BYBLOCK". Defaults
     *              to "BYLAYER"
     */
    DL_Attributes(string  layer, int color, int width, string  lineType)
    : m_layer(std::move(layer)), m_color(color), m_width(width), m_lineType(std::move(lineType)) {}



    /**
     * Sets the layer. If the given pointer points to NULL, the
     *  new layer name will be an empty but valid string.
     */
    void setLayer(const string& layer) {
        this->m_layer = layer;
    }



    /**
     * @return Layer name.
     */
    string getLayer() const {
        return m_layer;
    }



    /**
     * Sets the color.
     *
     * @see DL_Codes, dxfColors
     */
    void setColor(int color) {
        this->m_color = color;
    }



    /**
     * @return Color.
     *
     * @see DL_Codes, dxfColors
     */
    int getColor() const {
        return m_color;
    }



    /**
     * Sets the width.
     */
    void setWidth(int width) {
        this->m_width = width;
    }



    /**
     * @return Width.
     */
    int getWidth() const {
        return m_width;
    }



    /**
     * Sets the line type. This can be any string and is not
     *  checked to be a valid line type. 
     */
    void setLineType(const string& lineType) {
        this->m_lineType = lineType;
    }



    /**
     * @return Line type.
     */
    string getLineType() const {
        if (m_lineType.length() == 0) {
            return "BYLAYER";
        } else {
            return m_lineType;
        }
    }

private:
    string m_layer;
    int m_color;
    int m_width;
    string m_lineType;
};

#endif

// EOF
