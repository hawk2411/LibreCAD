/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
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

#include <iostream>
#include <cmath>
#include "rs_dimangular.h"
#include "rs_math.h"

#include "rs_constructionline.h"
#include "rs_arc.h"
#include "rs_line.h"
#include "rs_graphic.h"
#include "rs_information.h"
#include "rs_solid.h"
#include "rs_mtext.h"
#include "rs_debug.h"


/**
 * Constructor with initialisation.
 *
 * @param definitionPoint Definition point of the angular dimension.
 * @param leader Leader length.
 */
RS_DimAngularData::RS_DimAngularData(const RS_Vector &_definitionPoint1,
                                     const RS_Vector &_definitionPoint2,
                                     const RS_Vector &_definitionPoint3,
                                     const RS_Vector &_definitionPoint4) :
        definitionPoint1(_definitionPoint1),
        definitionPoint2(_definitionPoint2),
        definitionPoint3(_definitionPoint3),
        definitionPoint4(_definitionPoint4) {
}

/**
 * Constructor with initialisation.
 *
 * @param dimscale  general scale (DIMSCALE)
 * @param dimexo  distance from entities (DIMEXO)
 * @param dimexe  extension line extension (DIMEXE)
 * @param dimtxt  text height (DIMTXT)
 * @param dimgap  text distance to line (DIMGAP)
 * @param arrowSize  arrow length
 */
LC_DimAngularVars::LC_DimAngularVars(const double dimscale,
                                     const double dimexo,
                                     const double dimexe,
                                     const double dimtxt,
                                     const double dimgap,
                                     const double arrowSize) :
        _dimscale(dimscale),
        _dimexo(dimexo * dimscale),
        _dimexe(dimexe * dimscale),
        _dimtxt(dimtxt * dimscale),
        _dimgap(dimgap * dimscale),
        _arrowSize(arrowSize * dimscale) {
}

/**
 * Constructor.
 *
 * @para parent Parent Entity Container.
 * @para d Common dimension geometrical data.
 * @para ed Extended geometrical data for angular dimension.
 */
RS_DimAngular::RS_DimAngular(RS_EntityContainer *parent,
                             const RS_DimensionData &d,
                             const RS_DimAngularData &ed) :
        RS_Dimension(parent, d),
        _edata(ed) {
    calcDimension();
    calculateBordersLocal();
}

RS_Entity *RS_DimAngular::clone() const {
    RS_DimAngular *d{new RS_DimAngular(*this)};

    d->setOwner(isOwner());
    d->initId();
    d->detach();

    return d;
}

/**
 * @return Automatically created label for the default
 * measurement of this dimension.
 */
QString RS_DimAngular::getMeasuredLabel() {
    int dimaunit{getGraphicVariableInt(QString("$DIMAUNIT"), 0)};
    int dimadec{getGraphicVariableInt(QString("$DIMADEC"), 0)};
    int dimazin{getGraphicVariableInt(QString("$DIMAZIN"), 0)};
    RS2::AngleFormat format{RS_Units::numberToAngleFormat(dimaunit)};
    QString strLabel(RS_Units::formatAngle(_dimAngle, format, dimadec));

    if (RS2::DegreesMinutesSeconds != format
        && RS2::Surveyors != format) {
        strLabel = stripZerosAngle(strLabel, dimazin);
    }

    //verify if units are decimal and comma separator
    if (RS2::DegreesMinutesSeconds != dimaunit) {
        if (',' == getGraphicVariableInt(QString("$DIMDSEP"), 0)) {
            strLabel.replace(QChar('.'), QChar(','));
        }
    }

    return strLabel;
}

/**
 * @return Center of the measured dimension.
 */
RS_Vector RS_DimAngular::getCenter() const {
    return _dimCenter;
}

/**
 * @brief Add an extension line if necessary
 *
 * @param dimLine  dimension definition line including extension offset
 * @param dimPoint  point where the arc meets the definition line
 * @param dirStart  unit vector defining the lines starting point direction
 * @param dirEnd  unit vector defining the lines ending point direction
 * @param av  DXF variables with offset and extension line length
 * @param pen  pen to draw the extension line
 */
void RS_DimAngular::extensionLine(const RS_ConstructionLine &dimLine,
                                  const RS_Vector &dimPoint,
                                  const RS_Vector &dirStart,
                                  const RS_Vector &dirEnd,
                                  const LC_DimAngularVars &av,
                                  const RS_Pen &pen) {
    double diffLine{RS_Vector::posInLine(dimLine.getStartpoint(), dimLine.getEndpoint(), dimPoint)};
    double diffCenter{RS_Vector::posInLine(dimLine.getStartpoint(), _dimCenter, dimPoint)};

    if (0.0 <= diffLine && 1.0 >= diffLine) {
        // dimension ends on entity, nothing to extend
        return;
    }

    if (0.0 > diffLine && 0.0 > diffCenter) {
        RS_Line *line{new RS_Line(this,
                                  dimLine.getStartpoint(),
                                  dimPoint - dirStart * av.exe())};

        line->setPen(pen);
        line->setLayer(nullptr);
        addEntity(line);
    } else if (1.0 < diffLine && 0.0 < diffCenter) {
        RS_Line *line{new RS_Line(this,
                                  dimLine.getEndpoint(),
                                  dimPoint - dirEnd * av.exe())};

        line->setPen(pen);
        line->setLayer(nullptr);
        addEntity(line);
    } else if (0.0 > diffLine && 1.0 < diffCenter) {
        RS_Line *line{new RS_Line(this,
                                  _dimCenter - dirStart * av.exo(),
                                  dimPoint + dirEnd * av.exe())};

        line->setPen(pen);
        line->setLayer(nullptr);
        addEntity(line);
    }
}

/**
 * @brief Add an arrow to the dimension arc
 *
 * @param point  arc endpoint, the arrow tip
 * @param angle  the angle from center to the arc endpoint
 * @param direction  this holds the sign for the arrow endpoint direction
 * @param outsideArrow  when the arc becomes too small, arrows are placed outside
 * @param av  DXF variables with offset and extension line length
 * @param pen  pen to draw the extension line
 */
void RS_DimAngular::arrow(const RS_Vector &point,
                          const double angle,
                          const double direction,
                          const bool outsideArrows,
                          const LC_DimAngularVars &av,
                          const RS_Pen &pen) {
    if (RS_TOLERANCE_ANGLE >= av.arrow()) {
        // arrow size is 0, no need to add an arrow
        return;
    }

    double arrowAngle;

    if (outsideArrows) {
        // for outside arrows use tangent angle on endpoints
        // because for small radius the arrows looked inclined
        arrowAngle = angle + std::copysign(M_PI_2, direction);
    } else {
        // compute the angle from center to the endpoint of the arrow on the arc
        double endAngle = (RS_TOLERANCE_ANGLE < _dimRadius) ? av.arrow() / _dimRadius : 0.0;

        // compute the endpoint of the arrow on the arc
        RS_Vector arrowEnd;
        arrowEnd.setPolar(_dimRadius, angle + std::copysign(endAngle, direction));
        arrowEnd += _dimCenter;
        arrowAngle = arrowEnd.angleTo(point);
    }

    RS_SolidData sd;
    RS_Solid *arrow;

    arrow = new RS_Solid(this, sd);
    arrow->shapeArrow(point, arrowAngle, av.arrow());
    arrow->setPen(pen);
    arrow->setLayer(nullptr);
    addEntity(arrow);

}

/**
 * Updates the sub entities of this dimension. Called when the
 * dimension or the position, alignment, .. changes.
 *
 * @param autoText Automatically reposition the text label
 */
void RS_DimAngular::updateDim(bool autoText /*= false*/) {
    Q_UNUSED(autoText)
    RS_DEBUG->print("RS_DimAngular::update");

    clear();

    if (isUndone()) {
        return;
    }

    if (!_dimCenter._valid) {
        return;
    }

    LC_DimAngularVars av(getGeneralScale(),
                         getExtensionLineOffset(),
                         getExtensionLineExtension(),
                         getTextHeight(),
                         getDimensionLineGap(),
                         getArrowSize());

    // create new lines with offsets for extension lines
    RS_ConstructionLine line1(nullptr,
                              RS_ConstructionLineData(_dimLine1.getStartpoint() - _dimDir1s * av.exo(),
                                                      _dimLine1.getEndpoint() - _dimDir1e * av.exo()));
    RS_ConstructionLine line2(nullptr,
                              RS_ConstructionLineData(_dimLine2.getStartpoint() - _dimDir2s * av.exo(),
                                                      _dimLine2.getEndpoint() - _dimDir2e * av.exo()));

    RS_Vector p1{_dimCenter + _dimDir1e * _dimRadius};
    RS_Vector p2{_dimCenter + _dimDir2e * _dimRadius};
    RS_Pen pen(getExtensionLineColor(), getExtensionLineWidth(), RS2::LineByBlock);

    extensionLine(line1, p1, _dimDir1s, _dimDir1e, av, pen);
    extensionLine(line2, p2, _dimDir2s, _dimDir2e, av, pen);

    // Create dimension line (arc)
    RS_Arc *arc{new RS_Arc(this, RS_ArcData(_dimCenter, _dimRadius, _dimAngleL1, _dimAngleL2, false))};
    pen.setWidth(getDimensionLineWidth());
    pen.setColor(getDimensionLineColor());
    arc->setPen(pen);
    arc->setLayer(nullptr);
    addEntity(arc);

    // do we have to put the arrows outside of the arc?
    bool outsideArrows{arc->getLength() < 3.0 * av.arrow()};

    arrow(p1, _dimAngleL1, +1.0, outsideArrows, av, pen);
    arrow(p2, _dimAngleL2, -1.0, outsideArrows, av, pen);

    // text label
    RS_MTextData textData;
    RS_Vector textPos{arc->getMiddlePoint()};

    RS_Vector distV;
    double textAngle;
    double angle1{textPos.angleTo(_dimCenter) - M_PI_2};

    // rotate text so it's readable from the bottom or right (ISO)
    // quadrant 1 & 4
    if (angle1 > M_PI_2 * 3.0 + 0.001
        || angle1 < M_PI_2 + 0.001) {
        distV.setPolar(av.gap(), angle1 + M_PI_2);
        textAngle = angle1;
    }
        // quadrant 2 & 3
    else {
        distV.setPolar(av.gap(), angle1 - M_PI_2);
        textAngle = angle1 + M_PI;
    }

    // move text away from dimension line:
    textPos += distV;

    textData = RS_MTextData(textPos,
                            av.txt(), 30.0,
                            RS_MTextData::VABottom,
                            RS_MTextData::HACenter,
                            RS_MTextData::LeftToRight,
                            RS_MTextData::Exact,
                            1.0,
                            getLabel(),
                            getTextStyle(),
                            textAngle);

    RS_MText *text{new RS_MText(this, textData)};

    // move text to the side:
    text->setPen(RS_Pen(getTextColor(), RS2::WidthByBlock, RS2::SolidLine));
    text->setLayer(nullptr);
    addEntity(text);

    calculateBorders();
}

void RS_DimAngular::update() {
    calcDimension();
    RS_Dimension::update();
}

void RS_DimAngular::move(const RS_Vector &offset) {
    RS_Dimension::move(offset);

    _edata.definitionPoint1.move(offset);
    _edata.definitionPoint2.move(offset);
    _edata.definitionPoint3.move(offset);
    _edata.definitionPoint4.move(offset);
    update();
}

void RS_DimAngular::rotate(const RS_Vector &center, const double &angle) {
    rotate(center, RS_Vector(angle));
}

void RS_DimAngular::rotate(const RS_Vector &center, const RS_Vector &angleVector) {
    RS_Dimension::rotate(center, angleVector);

    _edata.definitionPoint1.rotate(center, angleVector);
    _edata.definitionPoint2.rotate(center, angleVector);
    _edata.definitionPoint3.rotate(center, angleVector);
    _edata.definitionPoint4.rotate(center, angleVector);
    update();
}

void RS_DimAngular::scale(const RS_Vector &center, const RS_Vector &factor) {
    RS_Dimension::scale(center, factor);

    _edata.definitionPoint1.scale(center, factor);
    _edata.definitionPoint2.scale(center, factor);
    _edata.definitionPoint3.scale(center, factor);
    _edata.definitionPoint4.scale(center, factor);
    update();
}

void RS_DimAngular::mirror(const RS_Vector &axisPoint1, const RS_Vector &axisPoint2) {
    RS_Dimension::mirror(axisPoint1, axisPoint2);

    _edata.definitionPoint1.mirror(axisPoint1, axisPoint2);
    _edata.definitionPoint2.mirror(axisPoint1, axisPoint2);
    _edata.definitionPoint3.mirror(axisPoint1, axisPoint2);
    _edata.definitionPoint4.mirror(axisPoint1, axisPoint2);
    update();
}

/**
 * @brief Compute all static values for dimension.
 *
 * From DXF reference the lines are P2-P1 and P-P3.
 * The dimension is drawn from line1 (P2-P1) to line2 (P-P3) in CCW direction.
 */
void RS_DimAngular::calcDimension() {
    // get unit vectors for definition points
    _dimDir1s = RS_Vector::polar(1.0, RS_Math::correctAngle(_edata.definitionPoint2.angleTo(_edata.definitionPoint1)));
    _dimDir1e = RS_Vector::polar(1.0, RS_Math::correctAngle(_edata.definitionPoint1.angleTo(_edata.definitionPoint2)));
    _dimDir2s = RS_Vector::polar(1.0, RS_Math::correctAngle(_data.definitionPoint.angleTo(_edata.definitionPoint3)));
    _dimDir2e = RS_Vector::polar(1.0, RS_Math::correctAngle(_edata.definitionPoint3.angleTo(_data.definitionPoint)));

    // create the two dimension definition lines
    _dimLine1 = RS_ConstructionLine(nullptr,
                                    RS_ConstructionLineData(_edata.definitionPoint2,
                                                            _edata.definitionPoint1));
    _dimLine2 = RS_ConstructionLine(nullptr,
                                    RS_ConstructionLineData(_data.definitionPoint,
                                                            _edata.definitionPoint3));

    RS_VectorSolutions vs{RS_Information::getIntersection(&_dimLine1, &_dimLine2, false)};
    _dimCenter = vs.get(0);
    _dimRadius = _dimCenter.distanceTo(_edata.definitionPoint4);
    _dimDirRad = RS_Vector::polar(1.0, RS_Math::correctAngle(_dimCenter.angleTo(_edata.definitionPoint4)));

    fixDimension();

    _dimAngleL1 = _dimLine1.getDirection2();
    _dimAngleL2 = _dimLine2.getDirection2();

    _dimAngle = RS_Math::correctAngle(_dimLine2.getDirection1() - _dimLine1.getDirection1());
}

/**
 * @brief check the dimension and fix non conform values from foreign CAD systems
 *
 * check if the radius definition point is on the arc,
 * from line1 to line2 in counter clockwise direction
 * LibreCAD takes care on correct orientation and line order in RS_ActionDimAngular
 * but angular dimensions, created in other CAD software, may fail and must be fixed here
 */
void RS_DimAngular::fixDimension() {
    if (RS_Math::isAngleBetween(_dimDirRad.angle(), _dimDir2s.angle(), _dimDir1s.angle(), false)) {
        return;
    }
    std::array<double, 4> distance{_data.definitionPoint.distanceTo(_dimCenter),
                                   _edata.definitionPoint1.distanceTo(_dimCenter),
                                   _edata.definitionPoint2.distanceTo(_dimCenter),
                                   _edata.definitionPoint3.distanceTo(_dimCenter)};

    std::array<double, 4> angle = {0.0, 0.0, 0.0, 0.0};

    if (RS_TOLERANCE >= distance[0]) {
        angle[3] = (_edata.definitionPoint3 - _dimCenter).angle();
        angle[0] = angle[3];
    } else if (RS_TOLERANCE >= distance[3]) {
        angle[0] = (_data.definitionPoint - _dimCenter).angle();
        angle[3] = angle[0];
    } else {
        angle[0] = (_data.definitionPoint - _dimCenter).angle();
        angle[3] = (_edata.definitionPoint3 - _dimCenter).angle();
    }

    if (RS_TOLERANCE >= distance[1]) {
        angle[2] = (_edata.definitionPoint2 - _dimCenter).angle();
        angle[1] = angle[2];
    } else if (RS_TOLERANCE >= distance[2]) {
        angle[1] = (_edata.definitionPoint1 - _dimCenter).angle();
        angle[2] = angle[1];
    } else {
        angle[1] = (_edata.definitionPoint1 - _dimCenter).angle();
        angle[2] = (_edata.definitionPoint2 - _dimCenter).angle();
    }

    if (angle[2] == angle[1]
        && distance[2] < distance[1]
        && angle[0] == angle[3]
        && distance[0] < distance[3]) {
        // revert both lines
        _dimLine1 = RS_ConstructionLine(nullptr,
                                        RS_ConstructionLineData(_dimLine1.getEndpoint(),
                                                                _dimLine1.getStartpoint()));
        _dimLine2 = RS_ConstructionLine(nullptr,
                                        RS_ConstructionLineData(_dimLine2.getEndpoint(),
                                                                _dimLine2.getStartpoint()));

        // and their unit vectors
        RS_Vector swapDir{_dimDir1s};
        _dimDir1s = _dimDir1e;
        _dimDir1e = swapDir;

        swapDir = _dimDir2s;
        _dimDir2s = _dimDir2e;
        _dimDir2e = swapDir;
    }

    // check again, as the previous revert may have made this condition false
    if (!RS_Math::isAngleBetween(_dimDirRad.angle(), _dimDir2s.angle(), _dimDir1s.angle(), false)) {
        // swap the lines
        RS_ConstructionLine swapLine{_dimLine1};
        _dimLine1 = _dimLine2;
        _dimLine2 = swapLine;

        // and their unit vectors
        RS_Vector swapDir{_dimDir1s};
        _dimDir1s = _dimDir2s;
        _dimDir2s = swapDir;

        swapDir = _dimDir1e;
        _dimDir1e = _dimDir2e;
        _dimDir2e = swapDir;
    }

}

/**
 * Dumps the point's data to stdout.
 */
std::ostream &operator<<(std::ostream &os, const RS_DimAngular &d) {
    os << " DimAngular: "
       << d.getData() << std::endl
       << d.getEData() << std::endl;

    return os;
}

std::ostream &operator<<(std::ostream &os, const RS_DimAngularData &dd) {
    os << "(" << dd.definitionPoint1
       << "," << dd.definitionPoint2
       << "," << dd.definitionPoint3
       << "," << dd.definitionPoint3
       << ")";

    return os;
}
