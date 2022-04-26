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


#include<cmath>
#include<QMouseEvent>
#include "rs_snapper.h"

#include "rs_point.h"
#include "rs_circle.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_grid.h"
#include "rs_settings.h"
#include "rs_overlayline.h"
#include "rs_entitycontainer.h"
#include "rs_debug.h"

/**
 * Constructor.
 */
RS_Snapper::RS_Snapper(RS_EntityContainer &container, RS_GraphicView &graphicView)
        : _container(&container), _graphicView(&graphicView), _pImpData(new ImpData{}), _snap_indicator(new Indicator{}) {}

RS_Snapper::~RS_Snapper() = default;

/**
 * Initialize (called by all constructors)
 */
void RS_Snapper::init() {
    _snapMode = _graphicView->getDefaultSnapMode();
    _keyEntity = nullptr;
    _pImpData->snapSpot = RS_Vector{false};
    _pImpData->snapCoord = RS_Vector{false};
    _m_SnapDistance = 1.0;

    RS_SETTINGS->beginGroup("/Appearance");
    _snap_indicator->lines_state = RS_SETTINGS->readNumEntry("/indicator_lines_state", 1);
    _snap_indicator->lines_type = RS_SETTINGS->readEntry("/indicator_lines_type", "Crosshair");
    _snap_indicator->shape_state = RS_SETTINGS->readNumEntry("/indicator_shape_state", 1);
    _snap_indicator->shape_type = RS_SETTINGS->readEntry("/indicator_shape_type", "Circle");
    RS_SETTINGS->endGroup();

    RS_SETTINGS->beginGroup("Colors");
    QString snap_color = RS_SETTINGS->readEntry("/snap_indicator", Colors::snap_indicator);
    RS_SETTINGS->endGroup();

    _snap_indicator->lines_pen = RS_Pen(RS_Color(snap_color), RS2::Width00, RS2::DashLine2);
    _snap_indicator->shape_pen = RS_Pen(RS_Color(snap_color), RS2::Width00, RS2::SolidLine);
    _snap_indicator->shape_pen.setScreenWidth(1);

    _snapRange = static_cast<int>(getSnapRange());
}


void RS_Snapper::finish() {
    _finished = true;
    deleteSnapper();
}


void RS_Snapper::setSnapMode(const RS_SnapMode &snapMode) {
    this->_snapMode = snapMode;
    GetDialogFactory()->requestSnapDistOptions(_m_SnapDistance, snapMode.snapDistance);
    GetDialogFactory()->requestSnapMiddleOptions(_middlePoints, snapMode.snapMiddle);
}


RS_SnapMode const *RS_Snapper::getSnapMode() const {
    return &(this->_snapMode);
}

RS_SnapMode *RS_Snapper::getSnapMode() {
    return &(this->_snapMode);
}

//get current mouse coordinates
RS_Vector RS_Snapper::snapFree(QMouseEvent *e) {
    if (!e) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Snapper::snapFree: event is nullptr");
        return RS_Vector(false);
    }
    _pImpData->snapSpot = _graphicView->toGraph(e->x(), e->y());
    _pImpData->snapCoord = _pImpData->snapSpot;
    _snap_indicator->lines_state = true;
    return _pImpData->snapCoord;
}

/**
 * Snap to a coordinate in the drawing using the current snap mode.
 *
 * @param e A mouse event.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapPoint(QMouseEvent *e) {
    _pImpData->snapSpot = RS_Vector(false);

    if (!e) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Snapper::snapPoint: event is nullptr");
        return _pImpData->snapSpot;
    }

    RS_Vector mouseCoord = _graphicView->toGraph(e->x(), e->y());
    double ds2Min = RS_MAXDOUBLE * RS_MAXDOUBLE;

    if (_snapMode.snapEndpoint) {
        auto t = snapEndpoint(mouseCoord);
        double ds2 = mouseCoord.squaredTo(t);

        if (ds2 < ds2Min) {
            ds2Min = ds2;
            _pImpData->snapSpot = t;
        }
    }
    if (_snapMode.snapCenter) {
        auto t = snapCenter(mouseCoord);
        double ds2 = mouseCoord.squaredTo(t);
        if (ds2 < ds2Min) {
            ds2Min = ds2;
            _pImpData->snapSpot = t;
        }
    }
    if (_snapMode.snapMiddle) {
        //this is still brutal force
        //todo: accept value from widget QG_SnapMiddleOptions
        GetDialogFactory()->requestSnapMiddleOptions(_middlePoints, _snapMode.snapMiddle);
        auto t = snapMiddle(mouseCoord);
        double ds2 = mouseCoord.squaredTo(t);
        if (ds2 < ds2Min) {
            ds2Min = ds2;
            _pImpData->snapSpot = t;
        }
    }
    if (_snapMode.snapDistance) {
        //this is still brutal force
        //todo: accept value from widget QG_SnapDistOptions
        GetDialogFactory()->requestSnapDistOptions(_m_SnapDistance, _snapMode.snapDistance);
        auto t = snapDist(mouseCoord);
        double ds2 = mouseCoord.squaredTo(t);
        if (ds2 < ds2Min) {
            ds2Min = ds2;
            _pImpData->snapSpot = t;
        }
    }
    if (_snapMode.snapIntersection) {
        auto t = snapIntersection(mouseCoord);
        double ds2 = mouseCoord.squaredTo(t);
        if (ds2 < ds2Min) {
            ds2Min = ds2;
            _pImpData->snapSpot = t;
        }
    }

    if (_snapMode.snapOnEntity &&
        _pImpData->snapSpot.distanceTo(mouseCoord) > _snapMode.distance) {
        auto t = snapOnEntity(mouseCoord);
        double ds2 = mouseCoord.squaredTo(t);
        if (ds2 < ds2Min) {
            ds2Min = ds2;
            _pImpData->snapSpot = t;
        }
    }

    if (_snapMode.snapGrid) {
        auto t = snapGrid(mouseCoord);
        double ds2 = mouseCoord.squaredTo(t);
        if (ds2 < ds2Min) {
//            ds2Min=ds2;
            _pImpData->snapSpot = t;
        }
    }

    if (!_pImpData->snapSpot.valid) {
        _pImpData->snapSpot = mouseCoord; //default to snapFree
    } else {
//        std::cout<<"mouseCoord.distanceTo(snapSpot)="<<mouseCoord.distanceTo(snapSpot)<<std::endl;
        //        std::cout<<"snapRange="<<snapRange<<std::endl;

        //retreat to snapFree when distance is more than half grid
        if (_snapMode.snapFree) {
            RS_Vector const &ds = mouseCoord - _pImpData->snapSpot;
            RS_Vector const &grid = _graphicView->getGrid()->getCellVector() * 0.5;
            if (fabs(ds.x) > fabs(grid.x) || fabs(ds.y) > fabs(grid.y)) _pImpData->snapSpot = mouseCoord;
        }

        //another choice is to keep snapRange in GUI coordinates instead of graph coordinates
//        if (mouseCoord.distanceTo(snapSpot) > snapRange ) snapSpot = mouseCoord;
    }
    //if (snapSpot.distanceTo(mouseCoord) > snapMode.distance) {
    // handle snap restrictions that can be activated in addition
    //   to the ones above:
    //apply restriction
    RS_Vector rz = _graphicView->getRelativeZero();
    RS_Vector vpv(rz.x, _pImpData->snapSpot.y);
    RS_Vector vph(_pImpData->snapSpot.x, rz.y);
    switch (_snapMode.restriction) {
        case RS2::RestrictOrthogonal:
            _pImpData->snapCoord = (mouseCoord.distanceTo(vpv) < mouseCoord.distanceTo(vph)) ?
                                   vpv : vph;
            break;
        case RS2::RestrictHorizontal:
            _pImpData->snapCoord = vph;
            break;
        case RS2::RestrictVertical:
            _pImpData->snapCoord = vpv;
            break;

            //case RS2::RestrictNothing:
        default:
            _pImpData->snapCoord = _pImpData->snapSpot;
            break;
    }
    //}
    //else snapCoord = snapSpot;

    snapPoint(_pImpData->snapSpot, false);

    return _pImpData->snapCoord;
}


/**manually set snapPoint*/
RS_Vector RS_Snapper::snapPoint(const RS_Vector &coord, bool setSpot) {
    if (!coord.valid) {
        return coord;
    }

    _pImpData->snapSpot = coord;
    if (setSpot) {
        _pImpData->snapCoord = coord;
    }
    drawSnapper();
    GetDialogFactory()->updateCoordinateWidget(
            _pImpData->snapCoord,
            _pImpData->snapCoord - _graphicView->getRelativeZero());
    return coord;
}


double RS_Snapper::getSnapRange() const {
    if (_graphicView) {
        return (_graphicView->getGrid()->getCellVector() * 0.5).magnitude();
    }

    return 20.;
}

/**
 * Snaps to a free coordinate.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapFree(const RS_Vector &coord) {
    _keyEntity = nullptr;
    return coord;
}


/**
 * Snaps to the closest endpoint.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapEndpoint(const RS_Vector &coord) const {
    return _container->getNearestEndpoint(coord,
                                          nullptr/*, &keyEntity*/);
}


/**
 * Snaps to a grid point.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapGrid(const RS_Vector &coord) const {
    return _graphicView->getGrid()->snapGrid(coord);
}


/**
 * Snaps to a point on an entity.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapOnEntity(const RS_Vector &coord) {
    return _container->getNearestPointOnEntity(coord, true, nullptr, &_keyEntity);
}


/**
 * Snaps to the closest center.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapCenter(const RS_Vector &coord) const {
    return _container->getNearestCenter(coord, nullptr);
}


/**
 * Snaps to the closest middle.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapMiddle(const RS_Vector &coord) const {
    return _container->getNearestMiddle(coord, static_cast<double *>(nullptr), _middlePoints);
}


/**
 * Snaps to the closest point with a given distance to the endpoint.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapDist(const RS_Vector &coord) const {
    return _container->getNearestDist(_m_SnapDistance, coord, nullptr);
}


/**
 * Snaps to the closest intersection point.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapIntersection(const RS_Vector &coord) const {
    return _container->getNearestIntersection(coord, nullptr);
}


/**
 * 'Corrects' the given coordinates to 0, 90, 180, 270 degrees relative to
 * the current relative zero point.
 *
 * @param coord The uncorrected coordinates.
 * @return The corrected coordinates.
 */
RS_Vector RS_Snapper::restrictOrthogonal(const RS_Vector &coord) {
    RS_Vector rz = _graphicView->getRelativeZero();
    RS_Vector ret(coord);

    RS_Vector retx = RS_Vector(rz.x, ret.y);
    RS_Vector rety = RS_Vector(ret.x, rz.y);

    if (retx.distanceTo(ret) > rety.distanceTo(ret)) {
        ret = rety;
    } else {
        ret = retx;
    }

    return ret;
}

/**
 * 'Corrects' the given coordinates to 0, 180 degrees relative to
 * the current relative zero point.
 *
 * @param coord The uncorrected coordinates.
 * @return The corrected coordinates.
 */
RS_Vector RS_Snapper::restrictHorizontal(const RS_Vector &coord) {
    RS_Vector rz = _graphicView->getRelativeZero();
    RS_Vector ret = RS_Vector(coord.x, rz.y);
    return ret;
}


/**
 * 'Corrects' the given coordinates to 90, 270 degrees relative to
 * the current relative zero point.
 *
 * @param coord The uncorrected coordinates.
 * @return The corrected coordinates.
 */
RS_Vector RS_Snapper::restrictVertical(const RS_Vector &coord) {
    RS_Vector rz = _graphicView->getRelativeZero();
    RS_Vector ret = RS_Vector(rz.x, coord.y);
    return ret;
}


/**
 * Catches an entity which is close to the given position 'pos'.
 *
 * @param pos A graphic coordinate.
 * @param level The level of resolving for iterating through the entity
 *        container
 * @return Pointer to the entity or nullptr.
 */
RS_Entity *RS_Snapper::catchEntity(const RS_Vector &pos,
                                   RS2::ResolveLevel level) {

    RS_DEBUG->print("RS_Snapper::catchEntity");

    // set default distance for points inside solids
    double dist(0.);
//    std::cout<<"getSnapRange()="<<getSnapRange()<<"\tsnap distance = "<<dist<<std::endl;

    RS_Entity *entity = _container->getNearestEntity(pos, &dist, level);

    int idx = -1;
    if (entity && entity->getParent()) {
        idx = entity->getParent()->findEntity(entity);
    }

    if (entity && dist <= getSnapRange()) {
        // highlight:
        RS_DEBUG->print("RS_Snapper::catchEntity: found: %d", idx);
        return entity;
    } else {
        RS_DEBUG->print("RS_Snapper::catchEntity: not found");
        return nullptr;
    }
    RS_DEBUG->print("RS_Snapper::catchEntity: OK");
}


/**
 * Catches an entity which is close to the given position 'pos'.
 *
 * @param pos A graphic coordinate.
 * @param level The level of resolving for iterating through the entity
 *        container
 * @enType, only search for a particular entity type
 * @return Pointer to the entity or nullptr.
 */
RS_Entity *RS_Snapper::catchEntity(const RS_Vector &pos, RS2::EntityType enType,
                                   RS2::ResolveLevel level) {

    RS_DEBUG->print("RS_Snapper::catchEntity");
//                    std::cout<<"RS_Snapper::catchEntity(): enType= "<<enType<<std::endl;

    // set default distance for points inside solids
    RS_EntityContainer ec(nullptr, false);
    //isContainer
    bool isContainer{false};
    switch (enType) {
        case RS2::EntityPolyline:
        case RS2::EntityContainer:
        case RS2::EntitySpline:
            isContainer = true;
            break;
        default:
            break;
    }

    for (RS_Entity *en = _container->firstEntity(level); en; en = _container->nextEntity(level)) {
        if (en->isVisible() == false) continue;
        if (en->rtti() != enType && isContainer) {
            //whether this entity is a member of member of the type enType
            RS_Entity *parent(en->getParent());
            bool matchFound{false};
            while (parent) {
//                    std::cout<<"RS_Snapper::catchEntity(): parent->rtti()="<<parent->rtti()<<" enType= "<<enType<<std::endl;
                if (parent->rtti() == enType) {
                    matchFound = true;
                    ec.addEntity(en);
                    break;
                }
                parent = parent->getParent();
            }
            if (!matchFound) continue;
        }
        if (en->rtti() == enType) {
            ec.addEntity(en);
        }
    }
    if (ec.count() == 0) return nullptr;
    double dist(0.);

    RS_Entity *entity = ec.getNearestEntity(pos, &dist, RS2::ResolveNone);

    int idx = -1;
    if (entity && entity->getParent()) {
        idx = entity->getParent()->findEntity(entity);
    }

    if (entity && dist <= getSnapRange()) {
        // highlight:
        RS_DEBUG->print("RS_Snapper::catchEntity: found: %d", idx);
        return entity;
    } else {
        RS_DEBUG->print("RS_Snapper::catchEntity: not found");
        return nullptr;
    }
}


/**
 * Catches an entity which is close to the mouse cursor.
 *
 * @param e A mouse event.
 * @param level The level of resolving for iterating through the entity
 *        container
 * @return Pointer to the entity or nullptr.
 */
RS_Entity *RS_Snapper::catchEntity(QMouseEvent *e,
                                   RS2::ResolveLevel level) {

    return catchEntity(
            RS_Vector(_graphicView->toGraphX(e->x()),
                      _graphicView->toGraphY(e->y())),
            level);
}


/**
 * Catches an entity which is close to the mouse cursor.
 *
 * @param e A mouse event.
 * @param level The level of resolving for iterating through the entity
 *        container
 * @enType, only search for a particular entity type
 * @return Pointer to the entity or nullptr.
 */
RS_Entity *RS_Snapper::catchEntity(QMouseEvent *e, RS2::EntityType enType,
                                   RS2::ResolveLevel level) {
    return catchEntity(
            {_graphicView->toGraphX(e->x()), _graphicView->toGraphY(e->y())},
            enType,
            level);
}

RS_Entity *RS_Snapper::catchEntity(QMouseEvent *e, const EntityTypeList &enTypeList,
                                   RS2::ResolveLevel level) {
    RS_Entity *pten = nullptr;
    RS_Vector coord{_graphicView->toGraphX(e->x()), _graphicView->toGraphY(e->y())};
    switch (enTypeList.size()) {
        case 0:
            return catchEntity(coord, level);
        default: {

            RS_EntityContainer ec(nullptr, false);
            for (auto t0: enTypeList) {
                RS_Entity *en = catchEntity(coord, t0, level);
                if (en) ec.addEntity(en);
//			if(en) {
//            std::cout<<__FILE__<<" : "<<__func__<<" : lines "<<__LINE__<<std::endl;
//            std::cout<<"caught id= "<<en->getId()<<std::endl;
//            }
            }
            if (ec.count() > 0) {
                ec.getDistanceToPoint(coord, &pten, RS2::ResolveNone, RS_MAXDOUBLE);
                return pten;
            }
        }

    }
    return nullptr;
}

void RS_Snapper::suspend() {
    // RVT Don't delete the snapper here!
    // RVT_PORT (can be deleted)();
    _pImpData->snapSpot = _pImpData->snapCoord = RS_Vector{false};
}

/**
 * Hides the snapper options. Default implementation does nothing.
 */
void RS_Snapper::hideOptions() {
    //not used any more, will be removed
}

/**
 * Shows the snapper options. Default implementation does nothing.
 */
void RS_Snapper::showOptions() {
    //not used any more, will be removed
}


/**
 * Deletes the snapper from the screen.
 */
void RS_Snapper::deleteSnapper() {
    _graphicView->getOverlayContainer(RS2::Snapper)->clear();
    _graphicView->redraw(RS2::RedrawOverlay); // redraw will happen in the mouse movement event
}


/**
 * creates the snap indicator
 */
void RS_Snapper::drawSnapper() {
    // We could properly speed this up by calling the draw function of this snapper within the paint event
    // this will avoid creating/deletion of the lines

    _graphicView->getOverlayContainer(RS2::Snapper)->clear();
    if (!_finished && _pImpData->snapSpot.valid) {
        RS_EntityContainer *container = _graphicView->getOverlayContainer(RS2::Snapper);

        if (_snap_indicator->lines_state) {
            QString type = _snap_indicator->lines_type;

            if (type == "Crosshair") {
                RS_OverlayLine *line = new RS_OverlayLine(nullptr,
                                                          {{0., _graphicView->toGuiY(_pImpData->snapCoord.y)},
                                                           {double(_graphicView->getWidth()),
                                                                _graphicView->toGuiY(_pImpData->snapCoord.y)}});

                line->setPen(_snap_indicator->lines_pen);
                container->addEntity(line);

                line = new RS_OverlayLine(nullptr,
                                          {{_graphicView->toGuiX(_pImpData->snapCoord.x), 0.},
                                           {_graphicView->toGuiX(_pImpData->snapCoord.x),
                                                                                          double(_graphicView->getHeight())}});

                line->setPen(_snap_indicator->lines_pen);
                container->addEntity(line);
            } else if (type == "Crosshair2") {
                double xenoRadius = 16;

                double snapX = _graphicView->toGuiX(_pImpData->snapCoord.x);
                double snapY = _graphicView->toGuiY(_pImpData->snapCoord.y);

                auto viewWidth = double(_graphicView->getWidth());
                auto viewHeight = double(_graphicView->getHeight());

                RS_OverlayLine *line;

                // ----O     (Left)
                line = new RS_OverlayLine(nullptr, {
                        {0.,                 snapY},
                        {snapX - xenoRadius, snapY}
                });
                {
                    line->setPen(_snap_indicator->lines_pen);
                    container->addEntity(line);
                }

                //     O---- (Right)
                line = new RS_OverlayLine(nullptr, {
                        {snapX + xenoRadius, snapY},
                        {viewWidth,          snapY}
                });
                {
                    line->setPen(_snap_indicator->lines_pen);
                    container->addEntity(line);
                }

                // (Top)
                line = new RS_OverlayLine(nullptr, {
                        {snapX, 0.},
                        {snapX, snapY - xenoRadius}
                });
                {
                    line->setPen(_snap_indicator->lines_pen);
                    container->addEntity(line);
                }

                // (Bottom)
                line = new RS_OverlayLine(nullptr, {
                        {snapX, snapY + xenoRadius},
                        {snapX, viewHeight}
                });
                {
                    line->setPen(_snap_indicator->lines_pen);
                    container->addEntity(line);
                }
            } else if (type == "Isometric") {
                //isometric crosshair
                RS2::CrosshairType chType = _graphicView->getCrosshairType();
                RS_Vector direction1;
                RS_Vector direction2(0., 1.);
                double l = _graphicView->getWidth() + _graphicView->getHeight();
                switch (chType) {
                    case RS2::RightCrosshair:
                        direction1 = RS_Vector(M_PI * 5. / 6.) * l;
                        direction2 *= l;
                        break;
                    case RS2::LeftCrosshair:
                        direction1 = RS_Vector(M_PI * 1. / 6.) * l;
                        direction2 *= l;
                        break;
                    default:
                        direction1 = RS_Vector(M_PI * 1. / 6.) * l;
                        direction2 = RS_Vector(M_PI * 5. / 6.) * l;
                }
                RS_Vector center(_graphicView->toGui(_pImpData->snapCoord));
                RS_OverlayLine *line = new RS_OverlayLine(container,
                                                          {center - direction1, center + direction1});
                line->setPen(_snap_indicator->lines_pen);
                container->addEntity(line);
                line = new RS_OverlayLine(nullptr,
                                          {center - direction2, center + direction2});
                line->setPen(_snap_indicator->lines_pen);
                container->addEntity(line);
            } else if (type == "Spiderweb") {
                RS_OverlayLine *line;
                RS_Vector point1;
                RS_Vector point2;

                point1 = RS_Vector{0, 0};
                point2 = RS_Vector{_graphicView->toGuiX(_pImpData->snapCoord.x),
                                   _graphicView->toGuiY(_pImpData->snapCoord.y)};
                line = new RS_OverlayLine{nullptr, {point1, point2}};
                line->setPen(_snap_indicator->lines_pen);
                container->addEntity(line);

                point1 = RS_Vector(0, _graphicView->getHeight());
                line = new RS_OverlayLine{nullptr, {point1, point2}};
                line->setPen(_snap_indicator->lines_pen);
                container->addEntity(line);

                point1 = RS_Vector(_graphicView->getWidth(), 0);
                line = new RS_OverlayLine(nullptr, {point1, point2});
                line->setPen(_snap_indicator->lines_pen);
                container->addEntity(line);

                point1 = RS_Vector(_graphicView->getWidth(), _graphicView->getHeight());
                line = new RS_OverlayLine(nullptr, {point1, point2});
                line->setPen(_snap_indicator->lines_pen);
                container->addEntity(line);
            }
        }
        if (_snap_indicator->shape_state) {
            QString type = _snap_indicator->shape_type;

            if (type == "Circle") {
                RS_Circle *circle = new RS_Circle(container,
                                                  {_pImpData->snapCoord, 4. / _graphicView->getFactor().x});
                circle->setPen(_snap_indicator->shape_pen);
                container->addEntity(circle);
            } else if (type == "Point") {
                RS_Point *point = new RS_Point(container, RS_PointData(_pImpData->snapCoord));
                point->setPen(_snap_indicator->shape_pen);
                container->addEntity(point);
            } else if (type == "Square") {
                RS_Vector snap_point{_graphicView->toGuiX(_pImpData->snapCoord.x),
                                     _graphicView->toGuiY(_pImpData->snapCoord.y)};

                double a = 6.0;
                RS_Vector p1 = snap_point + RS_Vector(-a, a);
                RS_Vector p2 = snap_point + RS_Vector(a, a);
                RS_Vector p3 = snap_point + RS_Vector(a, -a);
                RS_Vector p4 = snap_point + RS_Vector(-a, -a);

                RS_OverlayLine *line;
                line = new RS_OverlayLine{nullptr, {p1, p2}};
                line->setPen(_snap_indicator->shape_pen);
                container->addEntity(line);

                line = new RS_OverlayLine{nullptr, {p2, p3}};
                line->setPen(_snap_indicator->shape_pen);
                container->addEntity(line);

                line = new RS_OverlayLine(nullptr, {p3, p4});
                line->setPen(_snap_indicator->shape_pen);
                container->addEntity(line);

                line = new RS_OverlayLine(nullptr, {p4, p1});
                line->setPen(_snap_indicator->shape_pen);
                container->addEntity(line);
            }
        }
        _graphicView->redraw(RS2::RedrawOverlay); // redraw will happen in the mouse movement event
    }
}

RS_Vector RS_Snapper::snapToAngle(const RS_Vector &currentCoord, const RS_Vector &referenceCoord,
                                  const double angularResolution) {

    if (_snapMode.restriction != RS2::RestrictNothing || _snapMode.snapGrid) {
        return currentCoord;
    }

    double angle = referenceCoord.angleTo(currentCoord) * 180.0 / M_PI;
    angle -= remainder(angle, angularResolution);
    angle *= M_PI / 180.;
    RS_Vector res = RS_Vector::polar(referenceCoord.distanceTo(currentCoord), angle);
    res += referenceCoord;

    if (_snapMode.snapOnEntity) {
        RS_Vector t(false);
        //RS_Vector mouseCoord = graphicView->toGraph(currentCoord.x(), currentCoord.y());
        t = _container->getNearestVirtualIntersection(res, angle, nullptr);

        _pImpData->snapSpot = t;
        snapPoint(_pImpData->snapSpot, true);
        return t;
    } else {
        snapPoint(res, true);
        return res;
    }
}

