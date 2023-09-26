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
#include <iostream>
#include <cmath>
#include <QPainterPath>
#include <QBrush>
#include <QString>
#include <utility>
#include "rs_hatch.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_graphicview.h"
#include "rs_dialogfactory.h"
#include "rs_infoarea.h"

#include "rs_information.h"
#include "rs_painter.h"
#include "rs_pattern.h"
#include "rs_patternlist.h"
#include "rs_math.h"
#include "rs_debug.h"


RS_HatchData::RS_HatchData(bool _solid,
                           double _scale,
                           double _angle,
                           QString _pattern) :
        _solid(_solid),
        _scale(_scale),
        _angle(_angle),
        _pattern(std::move(_pattern)) {}

std::ostream &operator<<(std::ostream &os, const RS_HatchData &td) {
    os << "(" << td._pattern.toLatin1().data() << ")";
    return os;
}

/**
 * Constructor.
 */
RS_Hatch::RS_Hatch(RS_EntityContainer *parent,
                   const RS_HatchData &d)
        : RS_EntityContainer(parent),
          _data(d),
          _hatch(nullptr),
          _updateRunning(false),
          _needOptimization(true),
          _updateError(RS_HatchError::HATCH_UNDEFINED) {}


/**
 * Validates the hatch.
 */
bool RS_Hatch::validate() {
    bool ret = true;

    // loops:
    for (auto l: _entities) {

        if (l->rtti() == RS2::EntityContainer) {
            auto *loop = dynamic_cast<RS_EntityContainer *>(l);

            ret = loop->optimizeContours() && ret;
        }
    }

    return ret;
}


RS_Entity *RS_Hatch::clone() const {
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::clone()");
    auto *t = new RS_Hatch(*this);
    t->setOwner(isOwner());
    t->initId();
    t->detach();
    t->update();
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::clone(): OK");
    return t;
}


/**
 * @return Number of loops.
 */
int RS_Hatch::countLoops() const {
    if (_data._solid) {
        return count();
    } else {
        return count() -
               1;     //TODO Is it possible that count() can be null??? WTF of course it can be null and countLoops returns -1
    }
}

bool RS_Hatch::isContainer() const {
    return !isSolid();
}

/**
 * Recalculates the borders of this hatch.
 */
void RS_Hatch::calculateBorders() {
    RS_DEBUG->print("RS_Hatch::calculateBorders");

    activateContour(true);

    RS_EntityContainer::calculateBorders();

    RS_DEBUG->print("RS_Hatch::calculateBorders: size: %f,%f",
                    getSize().x, getSize().y);

    activateContour(false);
}


/**
 * Updates the Hatch. Called when the
 * hatch or it's data, position, alignment, .. changes.
 *
 * Refill hatch with pattern. Move, scale, rotate, trim, etc.
 */
void RS_Hatch::update() {

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update");

    _updateError = RS_HatchError::HATCH_OK;
    if (_updateRunning) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Hatch::update: skip hatch in updating process");
        return;
    }

    if (!_updateEnabled) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Hatch::update: skip hatch forbidden to update");
        return;
    }

    if (_data._solid) {
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: processing solid hatch");
        calculateBorders();
        return;
    }

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: contour has %d loops", count());
    _updateRunning = true;      //is this really async or recursive???

    // save attributes for the current hatch
    RS_Layer *hatch_layer = getLayer();
    RS_Pen hatch_pen = getPen();

    // delete old hatch:
    if (_hatch) {
        //it is really necessary to delete a hatch when its position is changed
        removeEntity(_hatch);
        _hatch = nullptr;
    }

    if (isUndone()) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Hatch::update: skip undone hatch");
        _updateRunning = false;
        return;
    }

    if (!validate()) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::update: invalid contour in hatch found");
        _updateRunning = false;
        _updateError = RS_HatchError::HATCH_INVALID_CONTOUR;
        return;
    }

    RS_EntityContainer *patternCarpet = createPatternCarpet();
    if (!patternCarpet) {
        return;
    }

    // cut pattern to contour shape
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: cutting pattern carpet");
    RS_EntityContainer smallCutLines;   // container for small cut lines

    smallCutLines = collectSmallCutLines(patternCarpet, smallCutLines);

    // updating hatch / adding entities that are inside
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: cutting pattern carpet: OK");

    // add the hatch pattern entities
    _hatch = new RS_EntityContainer(this);
    _hatch->setPen(hatch_pen);
    _hatch->setLayer(hatch_layer);
    _hatch->setFlag(RS2::FlagTemp);

    //calculateBorders();
    addSmallCutLinesToHatch(smallCutLines, _hatch);

    addEntity(_hatch);
    forcedCalculateBorders();

    // deactivate contour:
    activateContour(false);

    _updateRunning = false;

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: OK");
}

void RS_Hatch::addSmallCutLinesToHatch(const RS_EntityContainer &smallCutLines, RS_EntityContainer *hatch) {
    for (auto e: smallCutLines) {

        RS_Vector middlePoint;
        RS_Vector middlePoint2;
        switch (e->rtti()) {
            case RS2::EntityLine : {
                auto *line = dynamic_cast<RS_Line *>(e);
                middlePoint = line->getMiddlePoint();
                middlePoint2 = line->getNearestDist(line->getLength() / 2.1, line->getStartpoint(), nullptr);
            }
                break;
            case RS2::EntityArc: {
                auto *arc = dynamic_cast<RS_Arc *>(e);
                middlePoint = arc->getMiddlePoint();
                middlePoint2 = arc->getNearestDist(arc->getLength() / 2.1,
                                                   arc->getStartpoint(), nullptr);
            }
                break;
            default:
                middlePoint._valid = middlePoint2._valid = false;
        }
        if (!middlePoint._valid) {
            continue;
        }
        bool onContour = false;

        if (!RS_Information::isPointInsideContour(middlePoint, this, &onContour) &&
            !RS_Information::isPointInsideContour(middlePoint2, this)) {
            continue;
        }

        RS_Entity *te = e->clone();
        te->setPen(hatch->getPen(false));
        te->setLayer(hatch->getLayer(false));
        te->reparent(hatch);
        hatch->addEntity(te);
    }
}

RS_EntityContainer &
RS_Hatch::collectSmallCutLines(const RS_EntityContainer *patternCarpet, RS_EntityContainer &smallCutLines) {
    for (auto patternEntity: *patternCarpet) {

        if (!patternEntity) {
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Hatch::update: nullptr entity found");
            continue;
        }

        auto info = getEntityInfo(patternEntity);
        if (!info->entity) {
            continue;
        }
        // getting all intersections of this pattern line with the contour:
        QList<RS_Vector> intersections = calcIntersections(patternEntity);

        QList<RS_Vector> sortedIntersections = sortIntersections(info, intersections);

        // add small cut lines / arcs to tmp2:
        smallCutLines = addSmallCutLines(smallCutLines, info, sortedIntersections);
    } // end for very very long for(auto e: tmp) loop

    return smallCutLines;
}

RS_EntityContainer &
RS_Hatch::addSmallCutLines(RS_EntityContainer &smallCutLines, const std::unique_ptr<RS_Hatch::EntityInfo> &info,
                           const QList<RS_Vector> &sortedIntersections) {
    for (int i = 1; i < sortedIntersections.size(); ++i) {
        auto v1 = sortedIntersections.at(i - 1);
        auto v2 = sortedIntersections.at(i);


        if (info->entity->rtti() == RS2::EntityLine) {

            smallCutLines.addEntity(new RS_Line{&smallCutLines, v1, v2});
        } else if (info->entity->rtti() == RS2::EntityArc || info->entity->rtti() == RS2::EntityCircle) {
            if (fabs(info->center.angleTo(v2) - info->center.angleTo(v1)) > RS_TOLERANCE_ANGLE) {
                //don't create an arc with a too small angle
                smallCutLines.addEntity(new RS_Arc(&smallCutLines,
                                                   RS_ArcData(info->center,
                                                              info->center.distanceTo(v1),
                                                              info->center.angleTo(v1),
                                                              info->center.angleTo(v2),
                                                              info->reversed)));
            }
        }
    }
    return smallCutLines;
}

QList<RS_Vector>
RS_Hatch::sortIntersections(const std::unique_ptr<RS_Hatch::EntityInfo> &info, QList<RS_Vector> &intersections) const {
    QList<RS_Vector> sortedIntersections;       //to be filled with sorted intersections
    sortedIntersections.append(info->startPoint);

    // sort the intersection points into sortedIntersections (only if there are intersections):
    if (intersections.size() == 1) {        //only one intersection
        sortedIntersections.append(intersections.first());
    } else if (intersections.size() > 1) {
        RS_Vector sp = info->startPoint;
        double sa = (info->entity->rtti() == RS2::EntityEllipse)
                    ? ((RS_Ellipse *) info->entity)->getEllipseAngle(sp)
                    : info->center.angleTo(sp);
        bool done;
        RS_Vector last{};
        do {    // very long while(!done) loop
            RS_Vector av;
            done = true;
            double minDist = RS_MAXDOUBLE;
            for (const auto &intersection: intersections) {
                double dist = getDistance(info, sp, sa, intersection);

                if (dist < minDist) {
                    minDist = dist;
                    done = false;
                    av = intersection;
                }
            }

            // copy to sorted list, removing double points
            if (!done && av) {
                if (!last._valid || last.distanceTo(av) > RS_TOLERANCE) {
                    sortedIntersections.append(av);
                    last = av;
                }
                intersections.removeOne(av);

                av._valid = false;
            }
        } while (!done);
    }

    sortedIntersections.append(info->endPoint);
    return sortedIntersections;
}

QList<RS_Vector> RS_Hatch::calcIntersections(const RS_Entity *patternEntity) {
    QList<RS_Vector> intersections;

    for (auto loop: _entities) {
        if (!loop->isContainer()) {
            continue;
        }
        for (auto entityContainer: *dynamic_cast<RS_EntityContainer *>(loop)) {

            RS_VectorSolutions sol =
                    RS_Information::getIntersection(patternEntity, entityContainer, true);

            for (const RS_Vector &vp: sol) {
                if (vp._valid) {
                    intersections.append(vp);
                    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "  pattern line intersection: %f/%f", vp.x, vp.y);
                }
            }
        }
    }
    return intersections;
}

double RS_Hatch::getDistance(const std::unique_ptr<RS_Hatch::EntityInfo> &info, const RS_Vector &sp, double sa,
                             const RS_Vector &v) {
    double dist;
    switch (info->entity->rtti()) {
        case RS2::EntityLine:
            dist = sp.distanceTo(v);
            break;
        case RS2::EntityArc:
        case RS2::EntityCircle: {
            double a = info->center.angleTo(v);
            dist = info->reversed ?
                   fmod(sa - a + 2. * M_PI, 2. * M_PI) :
                   fmod(a - sa + 2. * M_PI, 2. * M_PI);
        }
            break;
        case RS2::EntityEllipse: {
            double a = ((RS_Ellipse *) info->entity)->getEllipseAngle(v);
            dist = info->reversed ?
                   fmod(sa - a + 2. * M_PI, 2. * M_PI) :
                   fmod(a - sa + 2. * M_PI, 2. * M_PI);
        }
            break;
        default:
            dist = 0.0;
            break;
    }
    return dist;
}


/**
 * Activates of deactivates the hatch boundary.
 */
void RS_Hatch::activateContour(bool on) {
    for (auto e: _entities) {
        if (e->isUndone())
            continue;
        if (!e->getFlag(RS2::FlagTemp)) {
            e->setVisible(on);
        }
    }
}

/**
 * Overrides drawing of subentities. This is only ever called for solid fills.
 */
void RS_Hatch::draw(RS_Painter *painter, RS_GraphicView *view, double & /*patternOffset*/) {

    if (!_data._solid) {
        for (auto &se: _entities) {
            view->drawEntity(painter, se);
        }
        return;
    }

    //area of solid fill. Use polygon approximation, except trivial cases
    QPainterPath path;
    QList<QPolygon> paClosed;
    QPolygon pa;

    // loops:
    optimize();

    // loops:
    for (auto &entity: _entities) {
        entity->setLayer(getLayer());

        if (entity->rtti() != RS2::EntityContainer) {
            continue;
        }
        auto *loop = (RS_EntityContainer *) entity;

        // edges:
        for (auto container_item: *loop) {

            container_item->setLayer(getLayer());
            switch (container_item->rtti()) {
                case RS2::EntityLine: {
                    QPoint pt1(RS_Math::round(view->toGuiX(container_item->getStartpoint().x)),
                               RS_Math::round(view->toGuiY(container_item->getStartpoint().y)));
                    QPoint pt2(RS_Math::round(view->toGuiX(container_item->getEndpoint().x)),
                               RS_Math::round(view->toGuiY(container_item->getEndpoint().y)));

                    if (!pa.empty() && (pa.last() - pt1).manhattanLength() >= 1)
                        pa << pt1;
                    pa << pt2;
                }
                    break;

                case RS2::EntityArc: {
                    QPolygon pa2;
                    auto *arc = dynamic_cast<RS_Arc *>(container_item);

                    painter->createArc(pa2, view->toGui(arc->getCenter()),
                                       view->toGuiDX(arc->getRadius()), arc->getAngle1(), arc->getAngle2(),
                                       arc->isReversed());
                    if (!pa.empty() && !pa2.empty() && (pa.last() - pa2.first()).manhattanLength() < 1) {
                        pa2.remove(0, 1);
                    }
                    pa << pa2;

                }
                    break;

                case RS2::EntityCircle: {
                    auto *circle = dynamic_cast<RS_Circle *>(container_item);
                    RS_Vector c = view->toGui(circle->getCenter());
                    const double r = view->toGuiDX(circle->getRadius());
                    path.addEllipse(QPointF(c.x, c.y), r, r);
                }
                    break;
                case RS2::EntityEllipse:
                    if (dynamic_cast<RS_Ellipse *>(container_item)->isArc()) {
                        QPolygon pa2;
                        auto ellipse = dynamic_cast<RS_Ellipse *>(container_item);

                        painter->createEllipse(pa2,
                                               view->toGui(ellipse->getCenter()),
                                               view->toGuiDX(ellipse->getMajorRadius()),
                                               view->toGuiDX(ellipse->getMinorRadius()),
                                               ellipse->getAngle(), ellipse->getAngle1(), ellipse->getAngle2(),
                                               ellipse->isReversed()
                        );
                        if (!pa.empty() && !pa2.empty() && (pa.last() - pa2.first()).manhattanLength() < 1) {
                            pa2.remove(0, 1);
                        }
                        pa << pa2;
                    } else {
                        QPolygon pa2;
                        auto ellipse = dynamic_cast<RS_Ellipse *>(container_item);
                        painter->createEllipse(pa2,
                                               view->toGui(ellipse->getCenter()),
                                               view->toGuiDX(ellipse->getMajorRadius()),
                                               view->toGuiDX(ellipse->getMinorRadius()),
                                               ellipse->getAngle(),
                                               ellipse->getAngle1(), ellipse->getAngle2(),
                                               ellipse->isReversed()
                        );
                        path.addPolygon(pa2);
                    }
                    break;
                default:
                    break;
            }
            if (pa.size() > 2 && pa.first() == pa.last()) {
                paClosed << pa;
                pa.clear();
            }

        }

    }
    if (pa.size() > 2) {
        pa << pa.first();
        paClosed << pa;
    }

    for (auto &p: paClosed) {
        path.addPolygon(p);
    }

    //bug#474, restore brush after solid fill
    const QBrush brush(painter->brush());
    const RS_Pen pen = painter->getPen();
    painter->setBrush(pen.getColor());
    painter->disablePen();
    painter->drawPath(path);
    painter->setBrush(brush);
    painter->setPen(pen);
}

void RS_Hatch::optimize() {
    if (!_needOptimization) {
        return;
    }
    for (RS_Entity *entity: _entities) {
        if (entity->rtti() == RS2::EntityContainer) {
            auto *loop = (RS_EntityContainer *) entity;

            loop->optimizeContours();
        }
    }
    _needOptimization = false;
}

//must be called after update()
double RS_Hatch::getTotalArea() {

    double totalArea = 0.;

    // loops:
    for (auto l: _entities) {

        if (l != _hatch && l->rtti() == RS2::EntityContainer) {
            totalArea += l->areaLineIntegral();
        }
    }
    return totalArea;
}

double RS_Hatch::getDistanceToPoint(
        const RS_Vector &coord,
        RS_Entity **entity,
        RS2::ResolveLevel level,
        double solidDist) const {

    if (!_data._solid) {
        return RS_EntityContainer::getDistanceToPoint(coord, entity, level, solidDist);
    }

    if (entity) {
        *entity = const_cast<RS_Hatch *>(this);
    }

    bool onContour;
    if (RS_Information::isPointInsideContour(coord, const_cast<RS_Hatch *>(this), &onContour)) {
        // distance is the snap range:
        return solidDist;
    }

    return RS_MAXDOUBLE;
}


void RS_Hatch::move(const RS_Vector &offset) {
    RS_EntityContainer::move(offset);
    update();
}


void RS_Hatch::rotate(const RS_Vector &center, const double &angle) {
    RS_EntityContainer::rotate(center, angle);
    _data._angle = RS_Math::correctAngle(_data._angle + angle);
    update();
}


void RS_Hatch::rotate(const RS_Vector &center, const RS_Vector &angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    _data._angle = RS_Math::correctAngle(_data._angle + angleVector.angle());
    update();
}

void RS_Hatch::scale(const RS_Vector &center, const RS_Vector &factor) {
    RS_EntityContainer::scale(center, factor);
    _data._scale *= factor.x;
    update();
}


void RS_Hatch::mirror(const RS_Vector &axisPoint1, const RS_Vector &axisPoint2) {
    RS_EntityContainer::mirror(axisPoint1, axisPoint2);
    double ang = axisPoint1.angleTo(axisPoint2);
    _data._angle = RS_Math::correctAngle(_data._angle + ang * 2.0);
    update();
}


void RS_Hatch::stretch(const RS_Vector &firstCorner,
                       const RS_Vector &secondCorner,
                       const RS_Vector &offset) {

    RS_EntityContainer::stretch(firstCorner, secondCorner, offset);
    update();
}


/**
 * Dumps the point's data to stdout.
 */
std::ostream &operator<<(std::ostream &os, const RS_Hatch &p) {
    os << " Hatch: " << p.getData() << "\n";
    return os;
}

bool RS_Hatch::isValid(RS_Pattern *pat, RS_Hatch *copy) {
    // create a pattern over the whole contour.
    RS_Vector pSize = pat->getSize();
    RS_Vector cSize = getSize();

    // check pattern sizes for sanity
    if (cSize.x < 1.0e-6 || cSize.y < 1.0e-6 ||
        pSize.x < 1.0e-6 || pSize.y < 1.0e-6 ||
        cSize.x > RS_MAXDOUBLE - 1 || cSize.y > RS_MAXDOUBLE - 1 ||
        pSize.x > RS_MAXDOUBLE - 1 || pSize.y > RS_MAXDOUBLE - 1) {
        delete pat;
        delete copy;
        _updateRunning = false;
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::update: contour size or pattern size too small");
        _updateError = RS_HatchError::HATCH_TOO_SMALL;
        return false;
    }
    // avoid huge memory consumption:
    if (cSize.x * cSize.y / (pSize.x * pSize.y) > 1e4) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::update: contour size too large or pattern size too small");
        delete pat;
        delete copy;
        _updateError = RS_HatchError::HATCH_AREA_TOO_BIG;
        return false;
    }
    return true;
}

RS_EntityContainer *RS_Hatch::createPatternCarpet() {
    // search for pattern
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: requesting pattern");
    RS_Pattern *pat = RS_PATTERNLIST->requestPattern(_data._pattern);
    if (!pat) {
        _updateRunning = false;
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::update: requesting pattern: not found");
        _updateError = RS_HatchError::HATCH_PATTERN_NOT_FOUND;
        return nullptr;
    }

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: requesting pattern: OK");
    // make a working copy of hatch pattern
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: cloning pattern");
    pat = (RS_Pattern *) pat->clone();
    if (pat) {
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: cloning pattern: OK");
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::update: error while cloning hatch pattern");
        return nullptr;
    }
    // scale pattern
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: scaling pattern");
    pat->scale(RS_Vector(0.0, 0.0), RS_Vector(_data._scale, _data._scale));
    pat->calculateBorders();
    forcedCalculateBorders();
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: scaling pattern: OK");

    // find out how many pattern-instances we need in x/y:
    auto *copy = (RS_Hatch *) clone();
    copy->rotate(RS_Vector(0.0, 0.0), -_data._angle);
    copy->forcedCalculateBorders();


    if (!isValid(pat, copy)) {
        return nullptr;
    }

    // create a pattern over the whole contour.
    RS_Vector pSize = pat->getSize();
    RS_Vector rot_center = pat->getMin();

    // calculate pattern pieces quantity
    double f = copy->getMin().x / pSize.x;
    int px1 = (int) floor(f);
    f = copy->getMin().y / pSize.y;
    int py1 = (int) floor(f);
    f = copy->getMax().x / pSize.x;
    int px2 = (int) ceil(f);
    f = copy->getMax().y / pSize.y;
    int py2 = (int) ceil(f);
    RS_Vector dvx = RS_Vector(_data._angle) * pSize.x;
    RS_Vector dvy = RS_Vector(_data._angle + M_PI * 0.5) * pSize.y;
    pat->rotate(rot_center, _data._angle);
    pat->move(-rot_center);

    auto *tmp = new RS_EntityContainer();   // container for untrimmed lines

    // adding array of patterns to tmp:
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: creating pattern carpet");
    for (int px = px1; px < px2; px++) {
        for (int py = py1; py < py2; py++) {
            for (auto e: *pat) {
                RS_Entity *te = e->clone();
                te->move(dvx * px + dvy * py);
                tmp->addEntity(te);
            }
        }
    }

    // clean memory
    delete pat;
    delete copy;
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: creating pattern carpet: OK");

    return tmp;
}

std::unique_ptr<RS_Hatch::EntityInfo> RS_Hatch::getEntityInfo(RS_Entity *patternEntity) const {

    auto blaName = std::make_unique<RS_Hatch::EntityInfo>();
    switch (patternEntity->rtti()) {
        case RS2::EntityLine:
            blaName->entity = dynamic_cast<RS_Line *>(patternEntity);
            blaName->startPoint = blaName->entity->getStartpoint();
            blaName->endPoint = blaName->entity->getEndpoint();
            break;
        case RS2::EntityArc:
            blaName->entity = dynamic_cast<RS_Arc *>(patternEntity);
            blaName->startPoint = blaName->entity->getStartpoint();
            blaName->endPoint = blaName->entity->getEndpoint();
            blaName->center = blaName->entity->getCenter();
            blaName->reversed = reinterpret_cast<RS_Arc *>(blaName->entity)->isReversed();
            break;
        case RS2::EntityCircle:
            blaName->entity = dynamic_cast<RS_Circle *>(patternEntity);
            blaName->startPoint = blaName->entity->getCenter() + RS_Vector(blaName->entity->getRadius(), 0.0);
            blaName->endPoint = blaName->startPoint;
            blaName->center = blaName->entity->getCenter();
            break;
        case RS2::EntityEllipse:
            blaName->entity = dynamic_cast<RS_Ellipse *>(patternEntity);
            blaName->startPoint = blaName->entity->getStartpoint();
            blaName->endPoint = blaName->entity->getEndpoint();
            blaName->center = blaName->entity->getCenter();
            blaName->reversed = reinterpret_cast<RS_Ellipse *>(blaName->entity)->isReversed();
            break;
        default:
            blaName->entity = nullptr;
    }
    return blaName;
}
