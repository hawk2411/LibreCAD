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
    DL_Attributes() : _color(0), _width(0), _lineType("BYLAYER") {
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
    DL_Attributes(std::string layer, int color, int width, std::string lineType) :
            _layer(std::move(layer)),
            _color(color),
            _width(width),
            _lineType(std::move(lineType)) {}


    /**
     * Sets the layer. If the given pointer points to NULL, the
     *  new layer name will be an empty but valid string.
     */
    void setLayer(const std::string &layer) {
        this->_layer = layer;
    }


    /**
     * @return Layer name.
     */
    [[nodiscard]] auto getLayer() const {
        return _layer;
    }


    /**
     * Sets the color.
     *
     * @see DL_Codes, dxfColors
     */
    void setColor(int color) {
        this->_color = color;
    }


    /**
     * @return Color.
     *
     * @see DL_Codes, dxfColors
     */
    [[nodiscard]] auto getColor() const {
        return _color;
    }


    /**
     * Sets the width.
     */
    void setWidth(int width) {
        this->_width = width;
    }


    /**
     * @return Width.
     */
    [[nodiscard]] int getWidth() const {
        return _width;
    }


    /**
     * Sets the line type. This can be any string and is not
     *  checked to be a valid line type. 
     */
    void setLineType(const std::string &lineType) {
        this->_lineType = lineType;
    }


    /**
     * @return Line type.
     */
    [[nodiscard]] std::string getLineType() const {
        if (_lineType.length() == 0) {
            return "BYLAYER";
        }
        return _lineType;
    }

private:
    std::string _layer;
    int _color;
    int _width;
    std::string _lineType;
};

#endif

// EOF
