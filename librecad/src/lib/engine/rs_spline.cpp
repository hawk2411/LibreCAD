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

#include<iostream>
#include<cmath>
#include<numeric>
#include <utility>

#include "rs_spline.h"


#include "rs_line.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_painter.h"
#include "rs_graphic.h"


RS_SplineData::RS_SplineData(int _degree, bool _closed) :
        degree(_degree), closed(_closed) {
}

std::ostream &operator<<(std::ostream &os, const RS_SplineData &ld) {
    os << "( degree: " << ld.degree <<
       " closed: " << ld.closed;
    if (!ld.controlPoints.empty()) {
        os << "\n(control points:\n";
        for (auto const &v: ld.controlPoints)
            os << v;
        os << ")\n";
    }
    if (!ld.knots.empty()) {
        os << "\n(knot vector:\n";
        for (auto const &v: ld.knots)
            os << v;
        os << ")\n";
    }
    os << ")";
    return os;
}

/**
 * Constructor.
 */
RS_Spline::RS_Spline(RS_EntityContainer *parent,
                     RS_SplineData data)
        : RS_EntityContainer(parent), _data(std::move(data)) {
}

RS_Entity *RS_Spline::clone() const {
    auto *copy = new RS_Spline(*this);
    copy->setOwner(isOwner());
    copy->initId();
    copy->detach();
    return copy;
}


void RS_Spline::calculateBorders() {}


void RS_Spline::setDegree(size_t deg) {
    if (deg >= 1 && deg <= 3) {
        _data.degree = deg;
    }
}

/** @return Degree of this spline curve (1-3).*/
size_t RS_Spline::getDegree() const {
    return _data.degree;
}

size_t RS_Spline::getNumberOfControlPoints() const {
    return _data.controlPoints.size();
}

/**
 * @retval true if the spline is closed.
 * @retval false otherwise.
 */
bool RS_Spline::isClosed() const {
    return _data.closed;
}

/**
 * Sets the closed flag of this spline.
 */
void RS_Spline::setClosed(bool c) {
    _data.closed = c;
    update();
}

RS_VectorSolutions RS_Spline::getRefPoints() const {
    return {_data.controlPoints};
}

RS_Vector RS_Spline::getNearestRef(const RS_Vector &coord,
                                   double *dist /*= nullptr*/) const {
    // override the RS_EntityContainer method
    // use RS_Entity instead for spline point dragging
    //NOLINTNEXTLINE
    return RS_Entity::getNearestRef(coord, dist);
}

RS_Vector RS_Spline::getNearestSelectedRef(const RS_Vector &coord,
                                           double *dist /*= nullptr*/) const {
    // override the RS_EntityContainer method
    // use RS_Entity instead for spline point dragging
    //NOLINTNEXTLINE
    return RS_Entity::getNearestSelectedRef(coord, dist);
}

/**
 * Updates the internal polygon of this spline. Called when the
 * spline or it's data, position, .. changes.
 */
void RS_Spline::update() {

    RS_DEBUG->print("RS_Spline::update");

    clear();

    if (isUndone()) {
        return;
    }

    if (_data.degree < 1 || _data.degree > 3) {
        RS_DEBUG->print("RS_Spline::update: invalid degree: %d", _data.degree);
        return;
    }

    if (_data.controlPoints.size() < _data.degree + 1) {
        RS_DEBUG->print("RS_Spline::update: not enough control points");
        return;
    }

    resetBorders();

    std::vector<RS_Vector> tControlPoints = _data.controlPoints;

    if (_data.closed) {
        for (size_t i = 0; i < _data.degree; ++i) {
            tControlPoints.push_back(_data.controlPoints.at(i));
        }
    }

    const size_t npts = tControlPoints.size();
    // order:
    const size_t k = _data.degree + 1;
    // resolution:
    const size_t p1 = getGraphicVariableInt("$SPLINESEGS", 8) * npts;

    std::vector<double> h(npts + 1, 1.);
    std::vector<RS_Vector> p(p1, {0., 0.});
    if (_data.closed) {
        rbsplinu(npts, k, p1, tControlPoints, h, p);
    } else {
        rbspline(npts, k, p1, tControlPoints, h, p);
    }

    RS_Vector prev{};
    for (auto const &vp: p) {
        if (prev.valid) {
            auto *line = new RS_Line{this, prev, vp};
            line->setLayer(nullptr);
            line->setPen(RS_Pen(RS2::FlagInvalid));
            addEntity(line);
        }
        prev = vp;
        _minV = RS_Vector::minimum(prev, _minV);
        _maxV = RS_Vector::maximum(prev, _maxV);
    }
}

RS_Vector RS_Spline::getStartpoint() const {
    if (_data.closed) return RS_Vector(false);
    return dynamic_cast<RS_Line *>(const_cast<RS_Spline *>(this)->firstEntity(RS2::ResolveNone))->getStartpoint();
}

RS_Vector RS_Spline::getEndpoint() const {
    if (_data.closed) return RS_Vector(false);
    return dynamic_cast<RS_Line *>(const_cast<RS_Spline *>(this)->lastEntity(RS2::ResolveNone))->getEndpoint();
}


RS_Vector RS_Spline::getNearestEndpoint(const RS_Vector &coord,
                                        double *dist) const {
    double minDist = RS_MAXDOUBLE;
    RS_Vector ret(false);
    if (!_data.closed) { // no endpoint for closed spline
        RS_Vector vp1(getStartpoint());
        RS_Vector vp2(getEndpoint());
        double d1((coord - vp1).squared());
        double d2((coord - vp2).squared());
        if (d1 < d2) {
            ret = vp1;
            minDist = sqrt(d1);
        } else {
            ret = vp2;
            minDist = sqrt(d2);
        }
    }
    if (dist) {
        *dist = minDist;
    }
    return ret;
}

RS_Vector RS_Spline::getNearestCenter(const RS_Vector & /*coord*/,
                                      double *dist) const {

    if (dist) {
        *dist = RS_MAXDOUBLE;
    }

    return RS_Vector(false);
}


RS_Vector RS_Spline::getNearestMiddle(const RS_Vector & /*coord*/,
                                      double *dist,
                                      int /*middlePoints*/) const {
    if (dist) {
        *dist = RS_MAXDOUBLE;
    }

    return RS_Vector(false);
}


RS_Vector RS_Spline::getNearestDist(double /*distance*/,
                                    const RS_Vector & /*coord*/,
                                    double *dist) const {
    if (dist) {
        *dist = RS_MAXDOUBLE;
    }

    return RS_Vector(false);
}


void RS_Spline::move(const RS_Vector &offset) {
    RS_EntityContainer::move(offset);
    for (RS_Vector &vp: _data.controlPoints) {
        vp.move(offset);
    }
//    update();
}


void RS_Spline::rotate(const RS_Vector &center, const double &angle) {
    rotate(center, RS_Vector(angle));
}


void RS_Spline::rotate(const RS_Vector &center, const RS_Vector &angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    for (RS_Vector &vp: _data.controlPoints) {
        vp.rotate(center, angleVector);
    }
//    update();
}

void RS_Spline::scale(const RS_Vector &center, const RS_Vector &factor) {
    for (RS_Vector &vp: _data.controlPoints) {
        vp.scale(center, factor);
    }

    update();
}


void RS_Spline::mirror(const RS_Vector &axisPoint1, const RS_Vector &axisPoint2) {
    for (RS_Vector &vp: _data.controlPoints) {
        vp.mirror(axisPoint1, axisPoint2);
    }

    update();
}


void RS_Spline::moveRef(const RS_Vector &ref, const RS_Vector &offset) {
    for (RS_Vector &vp: _data.controlPoints) {
        if (ref.distanceTo(vp) < 1.0e-4) {
            vp.move(offset);
        }
    }

    update();
}

void RS_Spline::revertDirection() {
    std::reverse(_data.controlPoints.begin(), _data.controlPoints.end());
}


void RS_Spline::draw(RS_Painter *painter, RS_GraphicView *view, double & /*patternOffset*/) {

    if (!(painter && view)) {
        return;
    }


    RS_Entity *e = firstEntity(RS2::ResolveNone);
    if (e) {
        RS_Pen p = this->getPen(true);
        e->setPen(p);
        double patternOffset(0.0);
        view->drawEntity(painter, e, patternOffset);
        //RS_DEBUG->print("offset: %f\nlength was: %f", offset, e->getLength());

        e = nextEntity(RS2::ResolveNone);
        while (e) {
            view->drawEntityPlain(painter, e, patternOffset);
            e = nextEntity(RS2::ResolveNone);
            //RS_DEBUG->print("offset: %f\nlength was: %f", offset, e->getLength());
        }
    }
}



/**
 * Todo: draw the spline, user patterns.
 */
/*
void RS_Spline::draw(RS_Painter* painter, RS_GraphicView* view) {
   if (!(painter && view)) {
       return;
   }

   / *
      if (data.controlPoints.count()>0) {
          RS_Vector prev(false);
          QList<RS_Vector>::iterator it;
          for (it = data.controlPoints.begin(); it!=data.controlPoints.end(); ++it) {
              if (prev.valid) {
                  painter->drawLine(view->toGui(prev),
                                    view->toGui(*it));
              }
              prev = (*it);
          }
      }
   * /

   int i;
   int npts = data.controlPoints.count();
   // order:
   int k = 4;
   // resolution:
   int p1 = 100;

   double* b = new double[npts*3+1];
   double* h = new double[npts+1];
   double* p = new double[p1*3+1];

   QList<RS_Vector>::iterator it;
   i = 1;
   for (it = data.controlPoints.begin(); it!=data.controlPoints.end(); ++it) {
       b[i] = (*it).x;
       b[i+1] = (*it).y;
       b[i+2] = 0.0;

        RS_DEBUG->print("RS_Spline::draw: b[%d]: %f/%f", i, b[i], b[i+1]);
        i+=3;
   }

   // set all homogeneous weighting factors to 1.0
   for (i=1; i <= npts; i++) {
       h[i] = 1.0;
   }

   //
   for (i = 1; i <= 3*p1; i++) {
       p[i] = 0.0;
   }

   rbspline(npts,k,p1,b,h,p);

   RS_Vector prev(false);
   for (i = 1; i <= 3*p1; i=i+3) {
       if (prev.valid) {
           painter->drawLine(view->toGui(prev),
                             view->toGui(RS_Vector(p[i], p[i+1])));
       }
       prev = RS_Vector(p[i], p[i+1]);
   }
}
*/



/**
 * @return The reference points of the spline.
 */
const std::vector<RS_Vector> &RS_Spline::getControlPoints() const {
    return _data.controlPoints;
}


/**
 * Appends the given point to the control points.
 */
void RS_Spline::addControlPoint(const RS_Vector &v) {
    _data.controlPoints.push_back(v);
}


/**
 * Removes the control point that was last added.
 */
void RS_Spline::removeLastControlPoint() {
    _data.controlPoints.pop_back();
}

//TODO: private interface cleanup; de Boor's Algorithm
/**
 * Generates B-Spline open knot vector with multiplicity
 * equal to the order at the ends.
 */
std::vector<double> RS_Spline::knot(int num, int order) const {
    if (static_cast<int>(_data.knots.size()) == num + order) {
        //use custom knot vector
        return _data.knots;
    }

    std::vector<double> knotVector(num + order, 0.);
    //use uniform knots
    std::iota(knotVector.begin() +  order, knotVector.begin() +  num + 1, 1);
    std::fill(knotVector.begin() + num + 1, knotVector.end(), knotVector[num]);
    return knotVector;
}



/**
 * Generates rational B-spline basis functions for an open knot vector.
 */
namespace {
    std::vector<double> rbasis(size_t c, double t, size_t npts,
                               const std::vector<double> &x,
                               const std::vector<double> &h) {

        const std::size_t nplusc = npts + c;

        std::vector<double> temp(nplusc, 0.);

        // calculate the first order nonrational basis functions n[i]
        for (size_t i = 0; i < nplusc - 1; i++)
            if ((t >= x[i]) && (t < x[i + 1])) temp[i] = 1;

        /* calculate the higher order nonrational basis functions */

        for (size_t k = 2; k <= c; k++) {
            for (size_t i = 0; i < nplusc - k; i++) {
                // if the lower order basis function is zero skip the calculation
                if (temp[i] != 0)
                    temp[i] = ((t - x[i]) * temp[i]) / (x[i + k - 1] - x[i]);
                // if the lower order basis function is zero skip the calculation
                if (temp[i + 1] != 0)
                    temp[i] += ((x[i + k] - t) * temp[i + 1]) / (x[i + k] - x[i + 1]);
            }
        }

        // pick up last point
        if (t >= x[nplusc - 1]) temp[npts - 1] = 1;

        // calculate sum for denominator of rational basis functions
        double sum = 0.;
        for (size_t i = 0; i < npts; i++) {
            sum += temp[i] * h[i];
        }

        std::vector<double> r(npts, 0);
        // form rational basis functions and put in r vector
        if (sum != 0) {
            for (size_t i = 0; i < npts; i++)
                r[i] = (temp[i] * h[i]) / sum;
        }
        return r;
    }
}


/**
 * Generates a rational B-spline curve using a uniform open knot vector.
 */
void RS_Spline::rbspline(size_t npts, size_t k, size_t p1,
                         const std::vector<RS_Vector> &b,
                         const std::vector<double> &h,
                         std::vector<RS_Vector> &p) const {
    size_t const nplusc = npts + k;

    // generate the open knot vector
    auto const x = knot(static_cast<int>(npts), static_cast<int>(k));

    // calculate the points on the rational B-spline curve
    double t{x[0]};
    const double step{(x[nplusc - 1] - t) / static_cast<double>((p1 - 1))};

    wtf(npts, k, b, h, p, nplusc, x, t, step);
}


std::vector<double> RS_Spline::knotu(size_t num, size_t order) const {
    if (_data.knots.size() == num + order) {
        //use custom knot vector
        return _data.knots;
    }
    std::vector<double> knotVector(num + order, 0.);
    std::iota(knotVector.begin(), knotVector.end(), 0);
    return knotVector;
}


void RS_Spline::rbsplinu(size_t npts, size_t k, size_t p1,
                         const std::vector<RS_Vector> &b,
                         const std::vector<double> &h,
                         std::vector<RS_Vector> &p) const {
    size_t const nplusc = npts + k;

    /* generate the periodic knot vector */
    std::vector<double> const x = knotu(npts, k);

    /*    calculate the points on the rational B-spline curve */
    auto t = static_cast<double>(k - 1);
    const double step = double(npts - k + 1) / static_cast<double>((p1 - 1));

    wtf(npts, k, b, h, p, nplusc, x, t, step);

}

void RS_Spline::wtf(size_t npts, size_t k, const std::vector<RS_Vector> &b, const std::vector<double> &h,
                    std::vector<RS_Vector> &p, size_t nplusc, const std::vector<double> &x, double t,
                    double step) {
    for (auto &vp: p) {
        if (x[nplusc - 1] - t < 5e-6) { //TODO magic number
            t = x[nplusc - 1];
        }

        /* generate the basis function for this value of t */
        auto const nbasis = rbasis(k, t, npts, x, h);

        /* generate a point on the curve, for x, y, z */
        for (size_t i = 0; i < npts; i++)
            vp += b[i] * nbasis[i];

        t += step;
    }
}


/**
 * Dumps the spline's data to stdout.
 */
std::ostream &operator<<(std::ostream &os, const RS_Spline &l) {
    os << " Spline: " << l.getData() << "\n";
    return os;
}


