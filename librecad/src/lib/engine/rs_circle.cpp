/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2011-2012 Dongxu Li (dongxuli2011@gmail.com)
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

#include <cfloat>
#include <QPolygonF>
#include <optional>

#include "rs_circle.h"

#include "rs_arc.h"
#include "rs_line.h"
#include "rs_information.h"
#include "rs_graphicview.h"
#include "rs_painter.h"
#include "rs_linetypepattern.h"
#include "rs_math.h"
#include "lc_quadratic.h"
#include "rs_debug.h"

RS_CircleData::RS_CircleData(RS_Vector const &center, double radius) :
        center(center), radius(radius) {
}

bool RS_CircleData::isValid() const {
    return (center.valid && radius > RS_TOLERANCE);
}

bool RS_CircleData::operator==(RS_CircleData const &rhs) const {
    if (!(center.valid && rhs.center.valid)) return false;
    if (center.squaredTo(rhs.center) > RS_TOLERANCE2) return false;
    return fabs(radius - rhs.radius) < RS_TOLERANCE;
}

std::ostream &operator<<(std::ostream &os, const RS_CircleData &ad) {
    os << "(" << ad.center <<
       "/" << ad.radius <<
       ")";
    return os;
}


/**
 * constructor.
 */
RS_Circle::RS_Circle(RS_EntityContainer *parent, const RS_CircleData &d)
        : RS_AtomicEntity(parent), _data(d) {

    RS_Vector r(_data.radius, _data.radius);
    _minV = _data.center - r;
    _maxV = _data.center + r;

}

RS_Entity *RS_Circle::clone() const {
    auto *c = new RS_Circle(*this);
    c->initId();
    return c;
}


void RS_Circle::calculateBorders() {
    RS_Vector r(_data.radius, _data.radius);
    _minV = _data.center - r;
    _maxV = _data.center + r;
}


/** @return The center point (x) of this arc */
RS_Vector RS_Circle::getCenter() const {
    return _data.center;
}

/** Sets new center. */
void RS_Circle::setCenter(const RS_Vector &c) {
    _data.center = c;
}

/** @return The radius of this arc */
double RS_Circle::getRadius() const {
    return _data.radius;
}

/** Sets new radius. */
void RS_Circle::setRadius(double r) {
    _data.radius = r;
}

/**
 * @return Length of the circle which is the circumference.
 */
double RS_Circle::getLength() const {
    return 2 * M_PI * _data.radius;
}

bool RS_Circle::isTangent(const RS_CircleData &circleData) const {
    const double d = circleData.center.distanceTo(_data.center);
//    DEBUG_HEADER
    const double r0 = fabs(circleData.radius);
    const double r1 = fabs(_data.radius);
//    std::cout<<fabs(d-fabs(r0-r1))<<" : "<<fabs(d-fabs(r0+r1))<<std::endl;
    if (fabs(d - fabs(r0 - r1)) < 20. * RS_TOLERANCE ||
        fabs(d - fabs(r0 + r1)) < 20. * RS_TOLERANCE)
        return true;
    return false;
}


/**
 * Creates this circle from a center point and a radius.
 *
 * @param center_point Center.
 * @param radius Radius
 */
std::unique_ptr<RS_Circle> RS_Circle::createFromCenterPointAndRadius(const RS_Vector &center_point, double radius) {
    if (fabs(radius) <= RS_TOLERANCE || !center_point.valid) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Circle::createFromCenterPointAndRadius(): "
                                            "Cannot create a circle with radius 0.0.");

        return {nullptr};
    }

    RS_CircleData data;
    data.radius = fabs(radius);
    data.center = center_point;

    return std::make_unique<RS_Circle>(nullptr, data);;
}


/**
 * Creates this circle from two opposite points.
 *
 * @param p1 1st point.
 * @param p2 2nd point.
 */
std::unique_ptr<RS_Circle> RS_Circle::createFrom2P(const RS_Vector &p1, const RS_Vector &p2) {
    double r = 0.5 * p1.distanceTo(p2);

    if (r > RS_TOLERANCE) {
        RS_CircleData data;
        data.radius = r;
        data.center = (p1 + p2) * 0.5;
        return std::make_unique<RS_Circle>(nullptr, data);
    }
    return {nullptr};
}


/**
 * Creates this circle from 3 given points which define the circle line.
 *
 * @param p1 1st point.
 * @param p2 2nd point.
 * @param p3 3rd point.
 */
std::unique_ptr<RS_Circle> RS_Circle::createFrom3P(const RS_Vector &p1, const RS_Vector &p2,
                             const RS_Vector &p3) {
    RS_Vector vra = p2 - p1;
    RS_Vector vrb = p3 - p1;
    double ra2 = vra.squared() * 0.5;
    double rb2 = vrb.squared() * 0.5;
    double crossp = vra.x * vrb.y - vra.y * vrb.x;
    if (fabs(crossp) < RS_TOLERANCE2) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Circle::createFrom3P(): "
                                             "Cannot create a circle with radius 0.0.");
        return {nullptr};
    }
    crossp = 1. / crossp;
    RS_CircleData data;
    data.center.set((ra2 * vrb.y - rb2 * vra.y) * crossp, (rb2 * vra.x - ra2 * vrb.x) * crossp);
    data.radius = data.center.magnitude();
    data.center += p1;
    return std::make_unique<RS_Circle>(nullptr, data);
}

//*create Circle from 3 points
//Author: Dongxu Li
std::unique_ptr<RS_Circle> RS_Circle::createFrom3P(const RS_VectorSolutions &sol) {
    if (sol.getNumber() < 2) {
        return {nullptr};
    }
    if (sol.getNumber() == 2) {
        return createFrom2P(sol.get(0), sol.get(1));
    }
    if ((sol.get(1) - sol.get(2)).squared() < RS_TOLERANCE2) {
        return createFrom2P(sol.get(0), sol.get(1));
    }
    RS_Vector vra(sol.get(1) - sol.get(0));
    RS_Vector vrb(sol.get(2) - sol.get(0));
    double ra2 = vra.squared() * 0.5;
    double rb2 = vrb.squared() * 0.5;
    double crossp = vra.x * vrb.y - vra.y * vrb.x;
    if (fabs(crossp) < RS_TOLERANCE2) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Circle::createFrom3P(): "
                                             "Cannot create a circle with radius 0.0.");
        return {nullptr};
    }
    crossp = 1. / crossp;
    RS_CircleData data;
    data.center.set((ra2 * vrb.y - rb2 * vra.y) * crossp, (rb2 * vra.x - ra2 * vrb.x) * crossp);
    data.radius = data.center.magnitude();
    data.center += sol.get(0);
    return std::make_unique<RS_Circle>(nullptr, data);
}

/**
  *create circle inscribled in a triangle
  *
  *Author: Dongxu Li
  */
std::unique_ptr<RS_Circle> RS_Circle::createInscribe(const RS_Vector &coord, const std::vector<RS_Line *> &lines) {
    if (lines.size() < 3) {
        return {nullptr};
    }
    std::vector<RS_Line *> tri(lines);
    RS_VectorSolutions sol = RS_Information::getIntersectionLineLine(tri[0], tri[1]);
    if (sol.getNumber() == 0) {//move parallel to opposite
        std::swap(tri[1], tri[2]);
        sol = RS_Information::getIntersectionLineLine(tri[0], tri[1]);
    }
    if (sol.getNumber() == 0) {
        return {nullptr};
    }
    RS_Vector vp0(sol.get(0));
    sol = RS_Information::getIntersectionLineLine(tri[2], tri[1]);
    if (sol.getNumber() == 0) {
        return {nullptr};
    }
    RS_Vector vp1(sol.get(0));
    RS_Vector dvp(vp1 - vp0);
    double a(dvp.squared());
    if (a < RS_TOLERANCE2) {
        //three lines share a common intersecting point
        return {nullptr};
    }
    RS_Vector vp(coord - vp0);
    vp -= dvp * (RS_Vector::dotP(dvp, vp) / a); //normal component
    RS_Vector vl0(tri[0]->getEndpoint() - tri[0]->getStartpoint());
    a = dvp.angle();
    double angle0(0.5 * (vl0.angle() + a));
    if (RS_Vector::dotP(vp, vl0) < 0.) {
        angle0 += 0.5 * M_PI;
    }

    RS_Line line0(vp0, vp0 + RS_Vector(angle0));//first bisecting line
    vl0 = (tri[2]->getEndpoint() - tri[2]->getStartpoint());
    angle0 = 0.5 * (vl0.angle() + a + M_PI);
    if (RS_Vector::dotP(vp, vl0) < 0.) {
        angle0 += 0.5 * M_PI;
    }
    RS_Line line1(vp1, vp1 + RS_Vector(angle0));//second bisection line
    sol = RS_Information::getIntersectionLineLine(&line0, &line1);
    if (sol.getNumber() == 0) {
        return {nullptr};
    }

    std::unique_ptr<RS_Circle> ret = createFromCenterPointAndRadius(sol.get(0),
                                              tri[1]->getDistanceToPoint(sol.get(0), nullptr, RS2::ResolveNone,
                                                                         RS_MAXDOUBLE));
    if (!ret) {
        return ret;
    }

    if(std::all_of(lines.cbegin(), lines.cend(), [&](const RS_Line *line) { return line->isTangent(ret->getData()); })){
        return ret;
    }
    return {nullptr};
}

std::vector<RS_Entity *> RS_Circle::offsetTwoSides(const double &distance) const {
    std::vector<RS_Entity *> ret(0, nullptr);
    ret.push_back(new RS_Circle(nullptr, {getCenter(), getRadius() + distance}));
    if (fabs(getRadius() - distance) > RS_TOLERANCE)
        ret.push_back(new RS_Circle(nullptr, {getCenter(), fabs(getRadius() - distance)}));
    return ret;
}

/**
  * create a circle of radius r and tangential to two given entities
  */
RS_VectorSolutions RS_Circle::createTan2(const std::vector<RS_AtomicEntity *> &circles, const double &r) {
    if (circles.size() < 2) return false;
    auto e0 = circles[0]->offsetTwoSides(r);
    auto e1 = circles[1]->offsetTwoSides(r);
    RS_VectorSolutions centers;
    if (!e0.empty() && !e1.empty()) {
        for (auto &it0: e0) {
            for (auto &it1: e1) {
                centers.push_back(RS_Information::getIntersection(it0, it1));
            }
        }
    }
    for (auto &it0: e0) {
        delete it0;
    }
    for (auto &it0: e1) {
        delete it0;
    }
    return centers;

}

std::vector<std::unique_ptr<RS_Circle>> RS_Circle::createTan3(const std::vector<RS_AtomicEntity *> &circles) {
    std::vector<std::unique_ptr<RS_Circle>> result;

    //it might be better to work here with std::array<.., 3>
    if (circles.size() != 3) {
        return result;
    }
    std::vector<std::unique_ptr<RS_Circle>> copied_circles;
    std::transform(circles.cbegin(), circles.cend(), std::back_inserter(copied_circles),
                   [](const RS_AtomicEntity *entity) {
                       return std::make_unique<RS_Circle>(nullptr,
                                                          RS_CircleData{entity->getCenter(), entity->getRadius()});
                   });
    for (unsigned short flags = 0; flags < 8u; flags++) {
        for (unsigned short j = 0u; j < 3u; ++j) {
            copied_circles[j]->setRadius(fabs(copied_circles[j]->getRadius()) * ((flags & (1u << j)) ? -1.0 : 1.0));
        }
        auto list = solveApolloniusSingle(copied_circles);
        if (list.empty()) {
            continue;
        }


        std::copy_if(std::make_move_iterator(list.begin()),
                     std::make_move_iterator(list.end()),
                     std::back_inserter(result),
                     [&result](const auto &source) -> bool {
                         return std::all_of(result.cbegin(), result.cend(),
                                            [&source](const auto &result_circle) -> bool {
                                                return isDistanceValid(source.get(), result_circle.get());
                                            });
                     });
    }
    result.erase(std::remove_if(result.begin(), result.end(),
                                [&circles](const std::unique_ptr<RS_Circle> &result_circle) {
                                    return !result_circle->testTan3(circles);
                                }),
                 result.cend());
    return result;
}

bool RS_Circle::testTan3(const std::vector<RS_AtomicEntity *> &circles) const {

    if (circles.size() != 3) return false;

    auto circle = circles.cbegin();
    const double r0 = fabs(_data.radius);
    const double r1 = fabs((*circle)->getRadius());

    const double dist = fabs((_data.center - (*circle)->getCenter()).magnitude());
    const double max_radius = std::max(r0, r1);
    return (dist < max_radius) ?
           fabs(dist - fabs(r0 - r1)) <= sqrt(DBL_EPSILON) * max_radius
                               :
           fabs(dist - fabs(r0 + r1)) <= sqrt(DBL_EPSILON) * max_radius;
}

/** solve one of the eight Appollonius Equations
| Cx - Ci|^2=(Rx+Ri)^2
with Cx the center of the common tangent circle, Rx the radius. Ci and Ri are the Center and radius of the i-th existing circle
**/
std::vector<std::unique_ptr<RS_Circle>>
RS_Circle::solveApolloniusSingle(const std::vector<std::unique_ptr<RS_Circle>> &circles) {
    std::vector<std::unique_ptr<RS_Circle>> ret;

    std::vector<RS_Vector> centers;
    std::vector<double> radii;

    for (const auto &c: circles) {
        if (!c->getCenter().valid) { return ret; }
        centers.push_back(c->getCenter());
        radii.push_back(c->getRadius());
    }
/** form the linear equation to solve center in radius **/
    std::vector<std::vector<double>> mat(2, std::vector<double>(3, 0.));
    mat[0][0] = centers[2].x - centers[0].x;
    mat[0][1] = centers[2].y - centers[0].y;
    mat[1][0] = centers[2].x - centers[1].x;
    mat[1][1] = centers[2].y - centers[1].y;
    if (fabs(mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0]) < RS_TOLERANCE2) {
        size_t i0 = 0;
        if (centers[0].distanceTo(centers[1]) <= RS_TOLERANCE || centers[0].distanceTo(centers[2]) <= RS_TOLERANCE) {
            i0 = 1;
        }
        LC_Quadratic lc0(circles[i0].get(), circles[(i0 + 1) % 3].get());
        LC_Quadratic lc1(circles[i0].get(), circles[(i0 + 2) % 3].get());
        auto c0 = LC_Quadratic::getIntersection(lc0, lc1);
        for (const auto &i: c0) {
            const double dc = i.distanceTo(centers[i0]);
            ret.push_back(std::make_unique<RS_Circle>(nullptr, RS_CircleData{i, fabs(dc - radii[i0])}));
            if (dc > radii[i0]) {
                ret.push_back(std::make_unique<RS_Circle>(nullptr, RS_CircleData{i, dc + radii[i0]}));
            }
        }
        return ret;
    }
    // r^0 term
    mat[0][2] = 0.5 * (centers[2].squared() - centers[0].squared() + radii[0] * radii[0] - radii[2] * radii[2]);
    mat[1][2] = 0.5 * (centers[2].squared() - centers[1].squared() + radii[1] * radii[1] - radii[2] * radii[2]);
    std::vector<double> sm(2, 0.);
    if (!RS_Math::linearSolver(mat, sm)) {
        return ret;
    }

    RS_Vector vp(sm[0], sm[1]);
    // r term
    mat[0][2] = radii[0] - radii[2];
    mat[1][2] = radii[1] - radii[2];
    if (!RS_Math::linearSolver(mat, sm)) {
        return ret;
    }
    RS_Vector vq(sm[0], sm[1]);
    //form quadratic equation for r
    RS_Vector dcp = vp - centers[0];
    double a = vq.squared() - 1.;
    if (fabs(a) < RS_TOLERANCE * 1e-4) {
        return ret;
    }
    std::vector<double> ce(0, 0.);
    ce.push_back(2. * (dcp.dotP(vq) - radii[0]) / a);
    ce.push_back((dcp.squared() - radii[0] * radii[0]) / a);
    std::vector<double> &&vr = RS_Math::quadraticSolver(ce);
    for (double i: vr) {
        if (i < RS_TOLERANCE) { continue; }

        ret.push_back(std::make_unique<RS_Circle>(nullptr, RS_CircleData{vp + vq * i, fabs(i)}));
    }
    return ret;
}

RS_VectorSolutions RS_Circle::getRefPoints() const {
    RS_Vector v1(_data.radius, 0.0);
    RS_Vector v2(0.0, _data.radius);

    return RS_VectorSolutions({_data.center,
                               _data.center + v1, _data.center + v2,
                               _data.center - v1, _data.center - v2});
}


/**
 * @brief compute nearest endpoint, intersection with X/Y axis at 0, 90, 180 and 270 degree
 *
 * Use getNearestMiddle() method to compute the nearest circle quadrant endpoints
 *
 * @param coord coordinates to compute, e.g. mouse cursor position
 * @param dist double pointer to return distance between mouse pointer and nearest entity point
 * @return the nearest intersection of the circle with X/Y axis.
 */
RS_Vector RS_Circle::getNearestEndpoint(const RS_Vector &coord, double *dist /*= nullptr*/) const {
    return getNearestMiddle(coord, dist, 0);
}


RS_Vector RS_Circle::getNearestPointOnEntity(const RS_Vector &coord,
                                             bool /*onEntity*/, double *dist, RS_Entity **entity) const {

    if (entity) {
        *entity = const_cast<RS_Circle *>(this);
    }
    RS_Vector vp(coord - _data.center);
    double d(vp.magnitude());
    if (d < RS_TOLERANCE) return RS_Vector(false);
    vp = _data.center + vp * (_data.radius / d);
//    RS_DEBUG->print(RS_Debug::D_ERROR, "circle(%g, %g), r=%g: distance to point (%g, %g)\n",data.center.x,data.center.y,coord.x,coord.y);

    if (dist) {
        *dist = coord.distanceTo(vp);
//        RS_DEBUG->print(RS_Debug::D_ERROR, "circle(%g, %g), r=%g: distance to point (%g, %g)=%g\n",data.center.x,data.center.y,coord.x,coord.y,*dist);
    }
    return vp;
}


/**
  *find the tangential points from a given point, i.e., the tangent lines should pass
  * the given point and tangential points
  *
  *Author: Dongxu Li
  */
RS_VectorSolutions RS_Circle::getTangentPoint(const RS_Vector &point) const {
    RS_VectorSolutions ret;
    double r2(getRadius() * getRadius());
    if (r2 < RS_TOLERANCE2) return ret; //circle too small
    RS_Vector vp(point - getCenter());
    double c2(vp.squared());
    if (c2 < r2 - getRadius() * 2. * RS_TOLERANCE) {
        //inside point, no tangential point
        return ret;
    }
    if (c2 > r2 + getRadius() * 2. * RS_TOLERANCE) {
        //external point
        RS_Vector vp1(-vp.y, vp.x);
        vp1 *= getRadius() * sqrt(c2 - r2) / c2;
        vp *= r2 / c2;
        vp += getCenter();
        if (vp1.squared() > RS_TOLERANCE2) {
            ret.push_back(vp + vp1);
            ret.push_back(vp - vp1);
            return ret;
        }
    }
    ret.push_back(point);
    return ret;
}

RS_Vector RS_Circle::getTangentDirection(const RS_Vector &point) const {
    RS_Vector vp(point - getCenter());
//    double c2(vp.squared());
//    if(c2<r2-getRadius()*2.*RS_TOLERANCE) {
//        //inside point, no tangential point
//        return RS_Vector(false);
//    }
    return {-vp.y, vp.x};

}

RS_Vector RS_Circle::getNearestCenter(const RS_Vector &coord,
                                      double *dist) const {
    if (dist) {
        *dist = coord.distanceTo(_data.center);
    }
    return _data.center;
}


RS_Vector RS_Circle::getMiddlePoint() const {
    return RS_Vector(false);
}

/**
 * @brief compute middlePoints for each quadrant of a circle
 *
 * 0 middlePoints snaps to axis intersection at 0, 90, 180 and 270 degree (getNearestEndpoint) \n
 * 1 middlePoints snaps to 45, 135, 225 and 315 degree \n
 * 2 middlePoints snaps to 30, 60, 120, 150, 210, 240, 300 and 330 degree \n
 * and so on
 *
 * @param coord coordinates to compute, e.g. mouse cursor position
 * @param dist double pointer to return distance between mouse pointer and nearest entity point
 * @param middlePoints number of middle points to compute per quadrant (0 for endpoints)
 * @return the nearest of equidistant middle points of the circles quadrants.
 */
RS_Vector RS_Circle::getNearestMiddle(const RS_Vector &coord,
                                      double *dist /*= nullptr*/,
                                      const int middlePoints /*= 1*/) const {
    if (_data.radius <= RS_TOLERANCE) {
        //circle too short
        if (nullptr != dist) {
            *dist = RS_MAXDOUBLE;
        }
        return RS_Vector(false);
    }

    RS_Vector vPoint(getNearestPointOnEntity(coord, true, dist, nullptr));
    int iCounts = middlePoints + 1;
    double dAngleSteps = M_PI_2 / iCounts;
    double dAngleToPoint = _data.center.angleTo(vPoint);
    int iStepCount = static_cast<int>((dAngleToPoint + 0.5 * dAngleSteps) / dAngleSteps);
    if (0 < middlePoints) {
        // for nearest middle eliminate start/endpoints
        int iQuadrant = static_cast<int>(dAngleToPoint / 0.5 / M_PI);
        int iQuadrantStep = iStepCount - iQuadrant * iCounts;
        if (0 == iQuadrantStep) {
            ++iStepCount;
        } else if (iCounts == iQuadrantStep) {
            --iStepCount;
        }
    }

    vPoint.setPolar(_data.radius, dAngleSteps * iStepCount);
    vPoint.move(_data.center);

    if (dist) {
        *dist = vPoint.distanceTo(coord);
    }

    return vPoint;
}


RS_Vector RS_Circle::getNearestDist(double /*distance*/,
                                    const RS_Vector & /*coord*/,
                                    double *dist) const {

    if (dist) {
        *dist = RS_MAXDOUBLE;
    }
    return RS_Vector(false);
}

RS_Vector RS_Circle::getNearestDist(double /*distance*/,
                                    bool /*startp*/) const {

    return RS_Vector(false);
}


RS_Vector RS_Circle::getNearestOrthTan(const RS_Vector &coord,
                                       const RS_Line &normal,
                                       bool /*onEntity = false*/) const {
    if (!coord.valid) {
        return RS_Vector(false);
    }
    RS_Vector vp0(coord - getCenter());
    RS_Vector vp1(normal.getAngle1());
    double d = RS_Vector::dotP(vp0, vp1);
    if (d >= 0.) {
        return getCenter() + vp1 * getRadius();
    } else {
        return getCenter() - vp1 * getRadius();
    }
}

void RS_Circle::move(const RS_Vector &offset) {
    _data.center.move(offset);
    moveBorders(offset);
//    calculateBorders();
}

/**
  * this function creates offset
  *@coord, position indicates the direction of offset
  *@distance, distance of offset
  * return true, if success, otherwise, false
  *
  *Author: Dongxu Li
  */
bool RS_Circle::offset(const RS_Vector &coord, const double &distance) {
    double r0(coord.distanceTo(getCenter()));
    if (r0 > getRadius()) {
        //external
        r0 = getRadius() + fabs(distance);
    } else {
        r0 = getRadius() - fabs(distance);
        if (r0 < RS_TOLERANCE) {
            return false;
        }
    }
    setRadius(r0);
    calculateBorders();
    return true;
}

void RS_Circle::rotate(const RS_Vector &center, const double &angle) {
    _data.center.rotate(center, angle);
    calculateBorders();
}

void RS_Circle::rotate(const RS_Vector &center, const RS_Vector &angleVector) {
    _data.center.rotate(center, angleVector);
    calculateBorders();
}

void RS_Circle::scale(const RS_Vector &center, const RS_Vector &factor) {
    _data.center.scale(center, factor);
    //radius always is positive
    _data.radius *= fabs(factor.x);
    scaleBorders(center, factor);
//    calculateBorders();
}

double RS_Circle::getDirection1() const {
    return M_PI_2;
}

double RS_Circle::getDirection2() const {
    return M_PI_2 * 3.0;
}


void RS_Circle::mirror(const RS_Vector &axisPoint1, const RS_Vector &axisPoint2) {
    _data.center.mirror(axisPoint1, axisPoint2);
    calculateBorders();
}


/** whether the entity's bounding box intersects with visible portion of graphic view
//fix me, need to handle overlay container separately
*/
bool RS_Circle::isVisibleInWindow(RS_GraphicView *view) const {

    RS_Vector vpMin(view->toGraph(0, view->getHeight()));
    RS_Vector vpMax(view->toGraph(view->getWidth(), 0));
    QPolygonF visualBox(QRectF(vpMin.x, vpMin.y, vpMax.x - vpMin.x, vpMax.y - vpMin.y));
    std::vector<RS_Vector> vps;
    for (unsigned short i = 0; i < 4; i++) {
        const QPointF &vp(visualBox.at(i));
        vps.emplace_back(vp.x(), vp.y());
    }
    for (unsigned short i = 0; i < 4; i++) {
        RS_Line line{nullptr, {vps.at(i), vps.at((i + 1) % 4)}};
        RS_Circle c0{nullptr, getData()};
        if (RS_Information::getIntersection(&c0, &line, true).size() > 0) return true;
    }
    if (!getCenter().isInWindowOrdered(vpMin, vpMax)) return false;
    return (vpMin - getCenter()).squared() > getRadius() * getRadius();
}


void RS_Circle::draw(RS_Painter *painter, RS_GraphicView *view, double & /*patternOffset*/) {
    painter->drawCircle(view->toGui(getCenter()), view->toGuiDX(getRadius()));
}


void RS_Circle::moveRef(const RS_Vector &ref, const RS_Vector &offset) {
    if (ref.distanceTo(_data.center) < 1.0e-4) {
        _data.center += offset;
        return;
    }
    RS_Vector v1(_data.radius, 0.0);
    RS_VectorSolutions sol;
    sol.push_back(_data.center + v1);
    sol.push_back(_data.center - v1);
    v1.set(0., _data.radius);
    sol.push_back(_data.center + v1);
    sol.push_back(_data.center - v1);
    double dist;
    v1 = sol.getClosest(ref, &dist);
    if (dist > 1.0e-4) return;
    _data.radius = _data.center.distanceTo(v1 + offset);
}


/** return the equation of the entity
for quadratic,

return a vector contains:
m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

for linear:
m0 x + m1 y + m2 =0
**/
LC_Quadratic RS_Circle::getQuadratic() const {
    std::vector<double> ce(6, 0.);
    ce[0] = 1.;
    ce[2] = 1.;
    ce[5] = -_data.radius * _data.radius;
    LC_Quadratic ret(ce);
    ret.move(_data.center);
    return ret;
}


/**
* @brief Returns area of full circle
* Note: Circular arcs are handled separately by RS_Arc (areaLIneIntegral) 
* However, full ellipses and ellipse arcs are handled by RS_Ellipse
* @return \pi r^2
*/
double RS_Circle::areaLineIntegral() const {
    const double r = getRadius();

    return M_PI * r * r;
}


/**
 * Dumps the circle's data to stdout.
 */
std::ostream &operator<<(std::ostream &os, const RS_Circle &a) {
    os << " Circle: " << a._data << "\n";
    return os;
}

bool RS_Circle::isDistanceValid(const RS_Circle *left, const RS_Circle *right) {
    return (left->getCenter() - right->getCenter()).squared() >= RS_TOLERANCE15 ||
           fabs(left->getRadius() - right->getRadius()) >= RS_TOLERANCE;
}

