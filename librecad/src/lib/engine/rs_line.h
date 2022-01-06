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


#ifndef RS_LINE_H
#define RS_LINE_H

#include "rs_atomicentity.h"

class LC_Quadratic;

/**
 * Holds the data that defines a line.
 */
struct RS_LineData {
    RS_LineData() :
            _startpoint(false),
            _endpoint(false) {}

    RS_LineData(const RS_Vector &point1,
                const RS_Vector &point2) :
            _startpoint(point1),
            _endpoint(point2) {}

    RS_Vector _startpoint;
    RS_Vector _endpoint;
};

std::ostream &operator<<(std::ostream &os, const RS_LineData &ld);

/**
 * Class for a line entity.
 *
 * @author Andrew Mustun
 */
class RS_Line : public RS_AtomicEntity {
public:
    RS_Line() = default;

    RS_Line(RS_EntityContainer *parent,
            const RS_LineData &data);

    RS_Line(RS_EntityContainer *parent, const RS_Vector &pStart, const RS_Vector &pEnd);

    RS_Line(const RS_Vector &pStart, const RS_Vector &pEnd);

    RS_Entity *clone() const override;

    /** @return RS2::EntityLine */
    RS2::EntityType rtti() const override {
        return RS2::EntityLine;
    }

    /** @return true */
    bool isEdge() const override {
        return true;
    }

    /** @return Copy of data that defines the line. */
    RS_LineData getData() const {
        return _lineData;
    }

    RS_VectorSolutions getRefPoints() const override;

    /** @return Start point of the entity */
    RS_Vector getStartpoint() const override {
        return _lineData._startpoint;
    }

    /** @return End point of the entity */
    RS_Vector getEndpoint() const override {
        return _lineData._endpoint;
    }

    /** Sets the startpoint */
    void setStartpoint(RS_Vector s) {
        _lineData._startpoint = s;
        calculateBorders();
    }

    /** Sets the endpoint */
    void setEndpoint(RS_Vector e) {
        _lineData._endpoint = e;
        calculateBorders();
    }

    /**
     * @return Direction 1. The angle at which the line starts at
     * the startpoint.
     */
    double getDirection1() const override {
        return getAngle1();
    }

    /**
     * @return Direction 2. The angle at which the line starts at
     * the endpoint.
     */
    double getDirection2() const override {
        return getAngle2();
    }

    RS_Vector getTangentDirection(const RS_Vector &point) const override;

    void moveStartpoint(const RS_Vector &pos) override;

    void moveEndpoint(const RS_Vector &pos) override;

    RS2::Ending getTrimPoint(const RS_Vector &trimCoord,
                             const RS_Vector &trimPoint) override;

    RS_Vector prepareTrim(const RS_Vector &trimCoord,
                          const RS_VectorSolutions &trimSol) override;

    void reverse() override;

    bool hasEndpointsWithinWindow(const RS_Vector &v1, const RS_Vector &v2) override;

    /**
     * @return The length of the line.
     */
    double getLength() const override {
        return _lineData._startpoint.distanceTo(_lineData._endpoint);
    }

    /**
     * @return The angle of the line (from start to endpoint).
     */
    double getAngle1() const {
        return _lineData._startpoint.angleTo(_lineData._endpoint);
    }

    /**
     * @return The angle of the line (from end to startpoint).
     */
    double getAngle2() const {
        return _lineData._endpoint.angleTo(_lineData._startpoint);
    }

    bool isTangent(const RS_CircleData &circleData) const override;

    /**
     * @return a perpendicular vector
     */
    RS_Vector getNormalVector() const;

    double getProjectionValueAlongLine(const RS_Vector &coord) const;

    RS_Vector getMiddlePoint() const override;

    RS_Vector getNearestEndpoint(const RS_Vector &coord,
                                 double *dist) const override;

    RS_Vector getNearestPointOnEntity(const RS_Vector &coord,
                                      bool onEntity,
                                      double *dist,
                                      RS_Entity **entity) const override;

    //RS_Vector getPointOnEntityAlongLine(const RS_Vector& coord,const double angle,bool onEntity,double* dist,RS_Entity** entity) const;
    RS_Vector getNearestMiddle(const RS_Vector &coord,
                               double *dist,
                               int middlePoints) const override;

    RS_Vector getNearestDist(double distance,
                             const RS_Vector &coord,
                             double *dist) const override;

    RS_Vector getNearestDist(double distance,
                             bool startp) const override;

    /**
     * implementations must revert the direction of an atomic entity
     */
    void revertDirection() override;

    std::vector<RS_Entity *> offsetTwoSides(const double &distance) const override;

    /**
     * the modify offset action
     */
    bool offset(const RS_Vector &coord, const double &distance) override;

    void move(const RS_Vector &offset) override;

    void rotate(const double &angle);

    void rotate(const RS_Vector &center, const double &angle) override;

    void rotate(const RS_Vector &center, const RS_Vector &angleVector) override;

    void scale(const RS_Vector &factor) override;

    void scale(const RS_Vector &center, const RS_Vector &factor) override;

    void mirror(const RS_Vector &axisPoint1, const RS_Vector &axisPoint2) override;

    void stretch(const RS_Vector &firstCorner,
                 const RS_Vector &secondCorner,
                 const RS_Vector &offset) override;

    void moveRef(const RS_Vector &ref, const RS_Vector &offset) override;

    /** whether the entity's bounding box intersects with visible portion of graphic view */
    void draw(RS_Painter *painter, RS_GraphicView *view, double &patternOffset) override;

    friend std::ostream &operator<<(std::ostream &os, const RS_Line &l);

    void calculateBorders() override;

    /**
     * @brief getQuadratic() returns the equation of the entity
     * for quadratic,
     *
     * return a vector contains:
     * m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0
     *
     * for linear:
     * m0 x + m1 y + m2 =0
     */
    LC_Quadratic getQuadratic() const override;

    /**
     * @brief areaLineIntegral line integral for contour area calculation by Green's Theorem
     * Contour Area =\oint x dy
     * @return line integral \oint x dy along the entity
     * \oint x dy = 0.5*(x0+x1)*(y1-y0)
     */
    double areaLineIntegral() const override;

private:
    RS_LineData _lineData;

    void calcBorder();

    bool calcCoordsInConstructionMode(RS_GraphicView *view, RS_Vector &startPoint, RS_Vector &endPoint, RS_Vector &direction);
};

#endif
