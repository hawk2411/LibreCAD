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


#ifndef RS_DIMENSION_H
#define RS_DIMENSION_H

#include "rs_entitycontainer.h"
#include "rs_mtext.h"

/**
 * Holds the data that is common to all dimension entities.
 */
struct RS_DimensionData : public RS_Flags {
    /**
	 * Default constructor
     */
    RS_DimensionData();

    /**
     * Constructor with initialisation.
     *
     * @param definitionPoint Definition point.
     * @param middleOfText Middle point of dimension text.
     * @param valign Vertical alignment.
     * @param halign Horizontal alignment.
     * @param lineSpacingStyle Line spacing style.
     * @param lineSpacingFactor Line spacing factor.
     * @param text Text string entered explicitly by user or null
     *         or "<>" for the actual measurement or " " (one blank space).
     *         for suppressing the text.
     * @param style Dimension style name.
     * @param angle Rotation angle of dimension text away from
     *         default orientation.
     */
    RS_DimensionData(const RS_Vector &definitionPoint,
                     const RS_Vector &middleOfText,
                     RS_MTextData::VAlign valign,
                     RS_MTextData::HAlign halign,
                     RS_MTextData::MTextLineSpacingStyle lineSpacingStyle,
                     double lineSpacingFactor,
                     QString text,
                     QString style,
                     double angle);

    /** Definition point */
    RS_Vector definitionPoint;
    /** Middle point of dimension text */
    RS_Vector middleOfText;
    /** Vertical alignment */
    RS_MTextData::VAlign valign;
    /** Horizontal alignment */
    RS_MTextData::HAlign halign;
    /** Line spacing style */
    RS_MTextData::MTextLineSpacingStyle lineSpacingStyle;
    /** Line spacing factor */
    double lineSpacingFactor;
    /**
    * Text string entered explicitly by user or null
    * or "<>" for the actual measurement or " " (one blank space)
    * for suppressing the text.
    */
    QString text;
    /** Dimension style name */
    QString style;
    /** Rotation angle of dimension text away from default orientation */
    double angle;
};

std::ostream &operator<<(std::ostream &os,
                         const RS_DimensionData &dd);

/**
 * Abstract base class for dimension entity classes.
 *
 * @author Andrew Mustun
 */
class RS_Dimension : public RS_EntityContainer {
public:
    RS_Dimension(RS_EntityContainer *parent,
                 RS_DimensionData d);

    RS_Vector getNearestRef(const RS_Vector &coord, double *dist) const override;

    RS_Vector getNearestSelectedRef(const RS_Vector &coord, double *dist) const override;

    /** @return Copy of data that defines the dimension. */
    RS_DimensionData getData() const {
        return _data;
    }

    QString getLabel(bool resolve = true);

    void setLabel(const QString &l);

    /**
     * Needs to be implemented by the dimension class to return the
     * measurement of the dimension (e.g. 10.5 or 15'14").
     */
    virtual QString getMeasuredLabel() = 0;

    /**
     * Must be overwritten by implementing dimension entity class
     * to update the subentities which make up the dimension entity.
     */
    void update() override {
        updateDim(false);
    }

    virtual void updateDim(bool autoText) = 0;  //default = false

    void updateCreateDimensionLine(const RS_Vector &p1, const RS_Vector &p2,
                                   bool arrow1 = true, bool arrow2 = true, bool autoText = false);

    virtual RS_Vector getDefinitionPoint() const {
        return _data.definitionPoint;
    }

    RS_Vector getMiddleOfText() const {
        return _data.middleOfText;
    }

    RS_MTextData::VAlign getVAlign() const {
        return _data.valign;
    }

    RS_MTextData::HAlign getHAlign() const {
        return _data.halign;
    }

    RS_MTextData::MTextLineSpacingStyle getLineSpacingStyle() const {
        return _data.lineSpacingStyle;
    }

    double getLineSpacingFactor() const {
        return _data.lineSpacingFactor;
    }

    QString getText() const {
        return _data.text;
    }

    QString getStyle() const {
        return _data.style;
    }

    virtual double getAngle() const {
        return _data.angle;
    }

    double getGeneralFactor();

    double getGeneralScale();

    double getArrowSize();

    double getTickSize();

    double getExtensionLineExtension();

    double getExtensionLineOffset();

    double getDimensionLineGap();

    double getTextHeight();

    bool getInsideHorizontalText();

    bool getFixedLengthOn();

    double getFixedLength();

    RS2::LineWidth getExtensionLineWidth();

    RS2::LineWidth getDimensionLineWidth();

    RS_Color getDimensionLineColor();

    RS_Color getExtensionLineColor();

    RS_Color getTextColor();

    QString getTextStyle();

    double getGraphicVariable(const QString &key, double defMM, int code);

    static QString stripZerosAngle(QString angle, int zeros = 0);

    static QString stripZerosLinear(QString linear, int zeros = 1);

    void move(const RS_Vector &offset) override;

    void rotate(const RS_Vector &center, const double &angle) override;

    void rotate(const RS_Vector &center, const RS_Vector &angleVector) override;

    void scale(const RS_Vector &center, const RS_Vector &factor) override;

    void mirror(const RS_Vector &axisPoint1, const RS_Vector &axisPoint2) override;

private:
    static RS_VectorSolutions getIntersectionsLineContainer(
            const RS_Line *line, const RS_EntityContainer *entityContainer, bool infiniteLine = false);

    void updateCreateHorizontalTextDimensionLine(
            const std::array<RS_Vector, 2>& points,
            const std::array<bool, 2>& arrows, bool autoText = false);

    void updateCreateAlignedTextDimensionLine(
            const std::array<RS_Vector, 2>& points, const std::array<bool, 2>& arrows, bool autoText = false);

protected:
    /** Data common to all dimension entities. */
    RS_DimensionData _data;

    void drawArrows(const std::array<RS_Vector, 2> &points, const std::array<bool, 2> &arrows, const RS_Pen &pen,
                     const std::array<double, 2>& arrowAngle, double arrow_size_scaled);

    void drawTicks(const std::array<RS_Vector, 2> &points, const std::array<bool, 2> &arrows, const RS_Pen &pen,
                   const std::array<double, 2> &arrowAngle, double dimtsz);
};

#endif
