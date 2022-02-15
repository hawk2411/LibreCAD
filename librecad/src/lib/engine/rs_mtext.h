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


#ifndef RS_MTEXT_H
#define RS_MTEXT_H

#include "rs_entitycontainer.h"

/**
 * Holds the data that defines a text entity.
 */
struct RS_MTextData {
    /**
     * Vertical alignments.
     */
    enum VAlign {
        VATop,      /**< Top. */
        VAMiddle,   /**< Middle */
        VABottom    /**< Bottom */
    };

    /**
     * Horizontal alignments.
     */
    enum HAlign {
        HALeft,     /**< Left */
        HACenter,   /**< Centered */
        HARight     /**< Right */
    };

    /**
     * MText drawing direction.
     */
    enum MTextDrawingDirection {
        LeftToRight,     /**< Left to right */
        TopToBottom,     /**< Top to bottom */
        ByStyle          /**< Inherited from associated text style */
    };

    /**
     * Line spacing style for MTexts.
     */
    enum MTextLineSpacingStyle {
        AtLeast,        /**< Taller characters will override */
        Exact           /**< Taller characters will not override */
    };

    /**
     * Default constructor. Leaves the data object uninitialized.
     */
    RS_MTextData() = default;

    /**
     * Constructor with initialisation.
     *
     * @param insertionPoint Insertion point
     * @param height Nominal (initial) text height
     * @param width Reference rectangle width
     * @param valign Vertical alignment
     * @param halign Horizontal alignment
     * @param drawingDirection Drawing direction
     * @param lineSpacingStyle Line spacing style
     * @param lineSpacingFactor Line spacing factor
     * @param text Text string
     * @param style Text style name
     * @param angle Rotation angle
     * @param updateMode RS2::Update will update the text entity instantly
     *    RS2::NoUpdate will not update the entity. You can update
     *    it later manually using the update() method. This is
     *    often the case since you might want to adjust attributes
     *    after creating a text entity.
     */
    RS_MTextData(const RS_Vector &insertionPoint,
                 double height,
                 double width,
                 VAlign valign,
                 HAlign halign,
                 MTextDrawingDirection drawingDirection,
                 MTextLineSpacingStyle lineSpacingStyle,
                 double lineSpacingFactor,
                 QString text,
                 QString style,
                 double angle,
                 RS2::UpdateMode updateMode = RS2::Update);

    /** Insertion point */
    RS_Vector insertionPoint;
    /** Nominal (initial) text height */
    double height{0.0};
    /** Reference rectangle width */
    double width{0.0};
    /** Vertical alignment */
    VAlign valign{VATop};
    /** Horizontal alignment */
    HAlign halign{HALeft};
    /** Drawing direction */
    MTextDrawingDirection drawingDirection{LeftToRight};
    /** Line spacing style */
    MTextLineSpacingStyle lineSpacingStyle{AtLeast};
    /** Line spacing factor */
    double lineSpacingFactor{0.0};
    /** Text string */
    QString text;
    /** Text style name */
    QString style;
    /** Rotation angle */
    double angle{0.0};
    /** Update mode */
    RS2::UpdateMode updateMode{RS2::UpdateMode::NoUpdate};
};

std::ostream &operator<<(std::ostream &os, const RS_MTextData &td);


/**
 * Class for a text entity.
 * Please note that text strings can contain special
 * characters such as %%c for a diameter sign as well as unicode
 * characters. Line feeds are stored as real line feeds in the string.
 *
 * @author Andrew Mustun
 */
class RS_MText : public RS_EntityContainer {
public:
    RS_MText(RS_EntityContainer *parent,
             RS_MTextData d);

    ~RS_MText() override = default;

    RS_Entity *clone() const override;

    /**	@return RS2::EntityText */
    RS2::EntityType rtti() const override {
        return RS2::EntityMText;
    }

    /** @return Copy of data that defines the text. */
    RS_MTextData getData() const {
        return _data;
    }

    void update() override;

    int getNumberOfLines() const;


    RS_Vector getInsertionPoint() const {
        return _data.insertionPoint;
    }

    double getHeight() const {
        return _data.height;
    }

    void setHeight(double h) {
        _data.height = h;
    }

    double getWidth() const {
        return _data.width;
    }

    void setAlignment(int a);

    int getAlignment() const;

    RS_MTextData::VAlign getVAlign() const {
        return _data.valign;
    }

    void setVAlign(RS_MTextData::VAlign va) {
        _data.valign = va;
    }

    RS_MTextData::HAlign getHAlign() const {
        return _data.halign;
    }

    RS_MTextData::MTextDrawingDirection getDrawingDirection() const {
        return _data.drawingDirection;
    }

    RS_MTextData::MTextLineSpacingStyle getLineSpacingStyle() const {
        return _data.lineSpacingStyle;
    }

    void setLineSpacingFactor(double f) {
        _data.lineSpacingFactor = f;
    }

    double getLineSpacingFactor() const {
        return _data.lineSpacingFactor;
    }

    void setText(const QString &t);

    QString getText() const {
        return _data.text;
    }

    void setStyle(const QString &s) {
        _data.style = s;
    }

    QString getStyle() const {
        return _data.style;
    }

    void setAngle(double a) {
        _data.angle = a;
    }

    double getAngle() const {
        return _data.angle;
    }

    double getUsedTextWidth() const {
        return _usedTextWidth;
    }

    double getUsedTextHeight() const {
        return _usedTextHeight;
    }

    /**
     * @return The insertion point as endpoint.
     */
    RS_Vector getNearestEndpoint(const RS_Vector &coord, double *dist) const override;

    RS_VectorSolutions getRefPoints() const override;

    void move(const RS_Vector &offset) override;

    void rotate(const RS_Vector &center, const double &angle) override;

    void rotate(const RS_Vector &center, const RS_Vector &angleVector) override;

    void scale(const RS_Vector &center, const RS_Vector &factor) override;

    void mirror(const RS_Vector &axisPoint1, const RS_Vector &axisPoint2) override;

    bool hasEndpointsWithinWindow(const RS_Vector &v1, const RS_Vector &v2) override;

    void stretch(const RS_Vector &firstCorner,
                         const RS_Vector &secondCorner,
                         const RS_Vector &offset) override;

    friend std::ostream &operator<<(std::ostream &os, const RS_Text &p);

    void draw(RS_Painter *painter, RS_GraphicView *view, double &patternOffset) override;

private:
    double updateAddLine(RS_EntityContainer *textLine, int lineCounter);

private:
    RS_MTextData _data;

    /**
     * Text width used by the current contents of this text entity.
     * This property is updated by the update method.
     * @see update
     */
    double _usedTextWidth;
    /**
     * Text height used by the current contents of this text entity.
     * This property is updated by the update method.
     * @see update
     */
    double _usedTextHeight;
};

#endif
