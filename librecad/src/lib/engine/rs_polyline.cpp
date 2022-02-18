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
#include<cassert>
#include "rs_polyline.h"

#include "rs_debug.h"
#include "rs_line.h"
#include "rs_arc.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs_information.h"

RS_PolylineData::RS_PolylineData() :
        startpoint(false), endpoint(false) {
}

RS_PolylineData::RS_PolylineData(const RS_Vector &_startpoint,
                                 const RS_Vector &_endpoint,
                                 bool _closed) :
        startpoint(_startpoint), endpoint(_endpoint) {

    if (_closed) {
        setFlag(RS2::FlagClosed);
    }
}

std::ostream &operator<<(std::ostream &os,
                         const RS_PolylineData &pd) {
    os << "(" << pd.startpoint <<
       "/" << pd.endpoint <<
       ")";
    return os;
}

/**
 * Constructor.
 */
RS_Polyline::RS_Polyline(RS_EntityContainer *parent)
        : RS_EntityContainer(parent, true), _closingEntity(nullptr), _nextBulge(0.) {
}


/**
 * Constructor.
 * @param d Polyline data
 */
RS_Polyline::RS_Polyline(RS_EntityContainer *parent,
                         const RS_PolylineData &d)
        : RS_EntityContainer(parent, true), _data(d), _closingEntity(nullptr), _nextBulge(0.) {
    RS_EntityContainer::calculateBordersLocal();
}

RS_Entity *RS_Polyline::clone() const {
    auto *p = new RS_Polyline(*this);
    p->setOwner(isOwner());
    p->initId();
    p->detach();
    return p;
}

/**
 * Removes the last vertex of this polyline.
 */
void RS_Polyline::removeLastVertex() {
    RS_Entity *lastEntity = last();
    if (!lastEntity) {
        return;
    }
    removeEntity(lastEntity);
    lastEntity = last();
    if (!lastEntity) {
        return;
    }

    if (lastEntity->isAtomic()) {
        _data.endpoint = lastEntity->getEndpoint();
    }
}


/**
 * Adds a vertex from the endpoint of the last segment or
 * from the startpoint of the first segment to 'v' or
 * sets the startpoint to the point 'v'.
 *
 * The very first vertex added with this method is the startpoint.
 *
 * @param v vertex coordinate to be added
 * @param bulge The bulge of the arc or 0 for a line segment (see DXF documentation)
 * @param prepend true: prepend at start instead of append at end
 *
 * @return Pointer to the entity that was added or nullptr if this
 *         was the first vertex added.
 */
RS_Entity *RS_Polyline::addVertex(const RS_Vector &v, double bulge, bool prepend) {

    RS_Entity *entity = nullptr;
    // very first vertex:
    if (!_data.startpoint.valid) {
        _data.startpoint = _data.endpoint = v;
        _nextBulge = bulge;
    } else {
        // consequent vertices:
        // add entity to the polyline:
        entity = createVertex(v, _nextBulge, prepend);
        if (!prepend) {
            RS_EntityContainer::addEntity(entity);
            _data.endpoint = v;
        } else {
            RS_EntityContainer::insertEntity(0, entity);
            _data.startpoint = v;
        }
        _nextBulge = bulge;
        endPolyline();
    }
    return entity;
}


/**
 * Appends a vertex list from the endpoint of the last segment
 * sets the startpoint to the first point if not exist.
 *
 * The very first vertex added with this method is the startpoint if not exists.
 *
 * @param vl list of vertexs coordinate to be added
 * @param Pair are RS_Vector of coord and the bulge of the arc or 0 for a line segment (see DXF documentation)
 *
 * @return None
 */
void RS_Polyline::appendVertexs(const std::vector<std::pair<RS_Vector, double> > &vl) {
    if (vl.empty()) {
        return;
    }
    size_t idx = 0;
    // very first vertex:
    if (!_data.startpoint.valid) {
        _data.startpoint = _data.endpoint = vl.at(idx).first;
        _nextBulge = vl.at(idx++).second;
    }

    // consequent vertices:
    for (; idx < vl.size(); idx++) {
        RS_Entity *entity = createVertex(vl.at(idx).first, _nextBulge, false);
        _data.endpoint = entity->getEndpoint();
        RS_EntityContainer::addEntity(entity);
        _nextBulge = vl.at(idx).second;
    }

    endPolyline();
}


/**
 * Creates a vertex from the endpoint of the last element or
 * sets the startpoint to the point 'v'.
 *
 * The very first vertex added is the starting point.
 *
 * @param v vertex coordinate
 * @param bulge The bulge of the arc (see DXF documentation)
 * @param prepend true: Prepend instead of append at end
 *
 * @return Pointer to the entity that was created or nullptr if this
 *         was the first vertex added.
 */
RS_Entity *RS_Polyline::createVertex(const RS_Vector &v, double bulge, bool prepend) {

    RS_DEBUG->print("RS_Polyline::createVertex: %f/%f to %f/%f bulge: %f",
                    _data.endpoint.x, _data.endpoint.y, v.x, v.y, bulge);

    // create line for the polyline:
    if (fabs(bulge) < RS_TOLERANCE) {
        RS_Entity *entity = (prepend)
                            ?
                            new RS_Line{this, v, _data.startpoint}
                            :
                            new RS_Line{this, _data.endpoint, v};

        entity->setSelected(isSelected());
        entity->setPen(RS_Pen(RS2::FlagInvalid));
        entity->setLayer(nullptr);
        return entity;
    }

    // create arc for the polyline:
    const bool reversed = (bulge < 0.0);
    const double alpha = atan(bulge) * 4.0;

    RS_Vector middle;
    double dist;
    double angle;

    if (!prepend) {
        middle = (_data.endpoint + v) / 2.0;
        dist = _data.endpoint.distanceTo(v) / 2.0;
        angle = _data.endpoint.angleTo(v);
    } else {
        middle = (_data.startpoint + v) / 2.0;
        dist = _data.startpoint.distanceTo(v) / 2.0;
        angle = v.angleTo(_data.startpoint);
    }

    // alpha can't be 0.0 at this point
    const double radius = fabs(dist / sin(alpha / 2.0));

    const double wu = fabs(radius * radius - dist * dist);
    double h = sqrt(wu);
    angle = (bulge > 0.0) ? angle + M_PI_2 : angle - M_PI_2;

    if (fabs(alpha) > M_PI) {
        h *= -1.0;
    }

    RS_Vector center = RS_Vector::polar(h, angle);
    center += middle;

    double a1;
    double a2;

    if (!prepend) {
        a1 = center.angleTo(_data.endpoint);
        a2 = center.angleTo(v);
    } else {
        a1 = center.angleTo(v);
        a2 = center.angleTo(_data.startpoint);
    }

    RS_Entity *entity = new RS_Arc(this, {center, radius, a1, a2, reversed});
    entity->setSelected(isSelected());
    entity->setPen(RS_Pen(RS2::FlagInvalid));
    entity->setLayer(nullptr);

    return entity;
}


/**
 * Ends polyline and adds the last entity if the polyline is closed
 */
void RS_Polyline::endPolyline() {
    RS_DEBUG->print("RS_Polyline::endPolyline");

    if (isClosed()) {
        RS_DEBUG->print("RS_Polyline::endPolyline: adding closing entity");

        // remove old closing entity:
        if (_closingEntity) {
            removeEntity(_closingEntity);
        }

        // add closing entity to the polyline:
        _closingEntity = createVertex(_data.startpoint, _nextBulge);
        if (_closingEntity && _closingEntity->getLength() > 1.0E-4) {
            RS_EntityContainer::addEntity(_closingEntity);
            //data.endpoint = data.startpoint;
        }
    }
    calculateBorders();
}

//RLZ: rewrite this:
void RS_Polyline::setClosedPolyLine(bool cl) {
    bool areClosed = isClosed();
    setClosedFlag(cl);
    if (isClosed()) {
        endPolyline();
    } else if (areClosed) {
        removeLastVertex();
    }
}

void RS_Polyline::setClosedFlag(bool cl) {
    if (cl) {
        _data.setFlag(RS2::FlagClosed);
    } else {
        _data.delFlag(RS2::FlagClosed);
    }
}


/** sets a new start point of the polyline */
void RS_Polyline::setStartpoint(RS_Vector const &v) {
    _data.startpoint = v;
    if (!_data.endpoint.valid) {
        _data.endpoint = v;
    }
}

/** @return Start point of the entity */
RS_Vector RS_Polyline::getStartpoint() const {
    return _data.startpoint;
}

/** sets a new end point of the polyline */
void RS_Polyline::setEndpoint(RS_Vector const &v) {
    _data.endpoint = v;
}

void RS_Polyline::setLayer(const QString &name) {
    RS_Entity::setLayer(name);
    // set layer for sub-entities
    for (auto *e: _entities) {
        e->setLayer(_layer);
    }
}

void RS_Polyline::setLayer(RS_Layer *l) {
    _layer = l;
    // set layer for sub-entities
    for (auto *e: _entities) {
        e->setLayer(_layer);
    }
}

/** @return End point of the entity */
RS_Vector RS_Polyline::getEndpoint() const {
    return _data.endpoint;
}

/**
 * @return The bulge of the closing entity.
 */
double RS_Polyline::getClosingBulge() const {
    if (!isClosed()) {
        return 0.0;
    }
    const RS_Entity *e = last();
    return (e && e->rtti() == RS2::EntityArc)
           ? dynamic_cast<RS_Arc const *>(e)->getBulge()
           : 0.0;
}

bool RS_Polyline::isClosed() const {
    return _data.getFlag(RS2::FlagClosed);
}

/**
 * Sets the polylines start and endpoint to match the first and last vertex.
 */
void RS_Polyline::updateEndpoints() {
    RS_Entity *e1 = firstEntity(RS2::ResolveNone);
    if (e1 && e1->isAtomic()) {
        RS_Vector const &v = e1->getStartpoint();
        setStartpoint(v);
    }

    RS_Entity const *e2 = last();
    if (isClosed()) {
        e2 = prevEntity(RS2::ResolveNone);
    }
    if (e2 && e2->isAtomic()) {
        RS_Vector const &v = e2->getEndpoint();
        setEndpoint(v);
    }
}


/**
 * Reimplementation of the addEntity method for a normal container.
 * This reimplementation deletes the given entity!
 *
 * To add entities use addVertex() or addSegment() instead.
 */
void RS_Polyline::addEntity(RS_Entity * /*entity*/) {
    RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Polyline::addEntity:"
                                         " should never be called\n"
                                         "use addVertex() or addSegment() instead"
    );
    assert(false);
}

RS_VectorSolutions RS_Polyline::getRefPoints() const {
    RS_VectorSolutions ret{{_data.startpoint}};
    for (auto e: *this) {
        if (e->isAtomic()) {
            ret.push_back(e->getEndpoint());
        }
    }

    ret.push_back(_data.endpoint);

    return ret;
}

RS_Vector RS_Polyline::getNearestRef(const RS_Vector &coord,
                                     double *dist /*= nullptr*/) const {
    // override the RS_EntityContainer method
    // use RS_Entity instead for vertex dragging
    //NOLINTNEXTLINE
    return RS_Entity::getNearestRef(coord, dist);
}

RS_Vector RS_Polyline::getNearestSelectedRef(const RS_Vector &coord,
                                             double *dist /*= nullptr*/) const {
    // override the RS_EntityContainer method
    // use RS_Entity instead for vertex dragging
    //NOLINTNEXTLINE
    return RS_Entity::getNearestSelectedRef(coord, dist);
}


/**
  * this should handle modifyOffset
  *@ coord, indicate direction of offset
  *@ distance of offset
  *
  *@Author, Dongxu Li
  */
bool RS_Polyline::offset(const RS_Vector &coord, const double &distance) {
    //find the nearest one
    std::vector<RS_Vector> intersections = getSortedIntersections();
    double dist;    //fixme this is an input and output value why is not initialized???
    RS_Entity *en = getNearestEntity(coord, &dist, RS2::ResolveNone);
    if (!en) {
        return false;
    }

    auto *polyline = dynamic_cast<RS_Polyline *>(clone());
    setAllOffsets(polyline, en, coord, distance);

    //trim
    //connect and trim        RS_Modification m(*container, graphicView);
    for (unsigned int i = 0; i < count(); i++) {

        if ((i >= count()- 1) && !isClosed()) {
            break;
        }

        RS_Entity *en0 = polyline->entityAt(static_cast<int>(i));
        RS_Entity *en1 = polyline->entityAt((i < count() - 1) ? static_cast<int>(i + 1) : 0);

        RS_VectorSolutions sol0 = RS_Information::getIntersection(en0, en1, true);
        if (sol0.getNumber() == 0) {
            sol0 = RS_Information::getIntersection(en0, en1);
            if (sol0.getNumber() == 0) {
                continue;
            }
        }
        RS_Vector trimP(sol0.getClosest(intersections.at(i)));
        dynamic_cast<RS_AtomicEntity *>(en0)->trimEndpoint(trimP);
        dynamic_cast<RS_AtomicEntity *>(en1)->trimStartpoint(trimP);
    }

    *this = *polyline;  //fixme polyline is new created pointer. what happens with the memory of *this
    return true;
}

std::vector<RS_Vector> RS_Polyline::getSortedIntersections() {
    std::vector<RS_Vector> intersections(count());
    if (count() > 1) {//sort the polyline entity start/end point order
        double d0, d1;
        RS_Entity *en0(entityAt(0));
        RS_Entity *en1(entityAt(1));

        RS_Vector vStart(en0->getStartpoint());
        RS_Vector vEnd(en0->getEndpoint());
        en1->getNearestEndpoint(vStart, &d0);
        en1->getNearestEndpoint(vEnd, &d1);
        if (d0 < d1) {
            en0->revertDirection();
        }
        for (unsigned int i = 1; i < count(); i++) {
            //linked to head-tail chain
            en1 = entityAt(static_cast<int>(i));
            vStart = en1->getStartpoint();
            vEnd = en1->getEndpoint();
            en0->getNearestEndpoint(vStart, &d0);
            en0->getNearestEndpoint(vEnd, &d1);
            if (d0 > d1) {
                en1->revertDirection();
            }
            intersections[i - 1] = (en0->getEndpoint() + en1->getStartpoint()) * 0.5;
            en0 = en1;
        }
        if (isClosed()) {
            en1 = entityAt(0);
            intersections[count() - 1] = (en0->getEndpoint() + en1->getStartpoint()) * 0.5;
        }
    }
    return intersections;
}

void RS_Polyline::setAllOffsets(RS_Polyline *polyline, const RS_Entity *en, const RS_Vector &coord, double distance) {
    int indexNearest = findEntity(en);
    polyline->entityAt(indexNearest)->offset(coord, distance);
    //offset all
    for (int i = indexNearest - 1; i >= 0; i--) {
        auto previousIndex = i + 1;
        setOffsets(polyline, distance, i, previousIndex);
    }

    for (unsigned int i = indexNearest + 1; i < count(); i++) {
        auto previousIndex = i - 1;
        setOffsets(polyline, distance, static_cast<int>(i), static_cast<int>(previousIndex));
    }
}

void RS_Polyline::setOffsets(RS_Polyline *polyline, double distance, int currentEntityIndex, int previousEntityIndex) {
    RS_VectorSolutions sol0 = RS_Information::getIntersection(polyline->entityAt(previousEntityIndex),
                                                              entityAt(currentEntityIndex), true);
    double max = RS_TOLERANCE15;    //near null number
    RS_Vector trimP;
    for (const RS_Vector &solVector: sol0) {
        double d0 = (solVector - polyline->entityAt(
                previousEntityIndex)->getStartpoint()).squared();//potential bug, need to trim better
        if (d0 > max) {
            max = d0;
            trimP = solVector;
        }
    }
    RS_Vector position;
    if (trimP.valid) {
        dynamic_cast<RS_AtomicEntity *>(polyline->entityAt(previousEntityIndex))->trimStartpoint(trimP);
        dynamic_cast<RS_AtomicEntity *>(polyline->entityAt(currentEntityIndex))->trimEndpoint(trimP);
        position = polyline->entityAt(previousEntityIndex)->getMiddlePoint();
    } else {
        position = polyline->entityAt(previousEntityIndex)->getStartpoint();
        position.rotate(entityAt(previousEntityIndex)->getStartpoint(),
                        entityAt(currentEntityIndex)->getDirection2() - entityAt(previousEntityIndex)->getDirection1() +
                        M_PI);
    }
    polyline->entityAt(currentEntityIndex)->offset(position, distance);
}

void RS_Polyline::move(const RS_Vector &offset) {
    RS_EntityContainer::move(offset);
    _data.startpoint.move(offset);
    _data.endpoint.move(offset);
    calculateBorders();
}


void RS_Polyline::rotate(const RS_Vector &center, const double &angle) {
    rotate(center, RS_Vector(angle));
}


void RS_Polyline::rotate(const RS_Vector &center, const RS_Vector &angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    _data.startpoint.rotate(center, angleVector);
    _data.endpoint.rotate(center, angleVector);
    calculateBorders();
}


void RS_Polyline::scale(const RS_Vector &center, const RS_Vector &factor) {
    RS_EntityContainer::scale(center, factor);
    _data.startpoint.scale(center, factor);
    _data.endpoint.scale(center, factor);
    calculateBorders();
}


void RS_Polyline::mirror(const RS_Vector &axisPoint1, const RS_Vector &axisPoint2) {
    RS_EntityContainer::mirror(axisPoint1, axisPoint2);
    _data.startpoint.mirror(axisPoint1, axisPoint2);
    _data.endpoint.mirror(axisPoint1, axisPoint2);
    calculateBorders();
}


void RS_Polyline::moveRef(const RS_Vector &ref, const RS_Vector &offset) {
    RS_EntityContainer::moveRef(ref, offset);
    if (ref.distanceTo(_data.startpoint) < 1.0e-4) {
        _data.startpoint.move(offset);
    }
    if (ref.distanceTo(_data.endpoint) < 1.0e-4) {
        _data.endpoint.move(offset);
    }
    calculateBorders();
    //update();
}

void RS_Polyline::revertDirection() {
    RS_EntityContainer::revertDirection();
    RS_Vector tmp = _data.startpoint;
    _data.startpoint = _data.endpoint;
    _data.endpoint = tmp;
}

void RS_Polyline::stretch(const RS_Vector &firstCorner,
                          const RS_Vector &secondCorner,
                          const RS_Vector &offset) {

    if (_data.startpoint.isInWindow(firstCorner, secondCorner)) {
        _data.startpoint.move(offset);
    }
    if (_data.endpoint.isInWindow(firstCorner, secondCorner)) {
        _data.endpoint.move(offset);
    }

    RS_EntityContainer::stretch(firstCorner, secondCorner, offset);
    calculateBorders();
}


/**
 * Slightly optimized drawing for polylines.
 */
void RS_Polyline::draw(RS_Painter *painter, RS_GraphicView *view, double & /*patternOffset*/) {

    if (!view) return;

    // draw first entity and set correct pen:
    RS_Entity *e = firstEntity(RS2::ResolveNone);
    // We get the pen from the entitycontainer and apply it to the
    // first line so that subsequent line are draw in the right color
    //prevent segfault if polyline is empty
    if (e) {
        RS_Pen p = this->getPen(true);
        e->setPen(p);
        double patternOffset = 0.;
        view->drawEntity(painter, e, patternOffset);

        e = nextEntity(RS2::ResolveNone);
        while (e) {
            view->drawEntityPlain(painter, e, patternOffset);
            e = nextEntity(RS2::ResolveNone);
        }
    }
}


/**
 * Dumps the point's data to stdout.
 */
std::ostream &operator<<(std::ostream &os, const RS_Polyline &l) {
    os << " Polyline: " << l.getData() << " {\n";

    os << (RS_EntityContainer &) l;

    os << "\n}\n";

    return os;
}

