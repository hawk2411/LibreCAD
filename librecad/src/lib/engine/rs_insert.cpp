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
#include <utility>
#include "rs_insert.h"

#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_block.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_math.h"
#include "rs_debug.h"

RS_InsertData::RS_InsertData(QString _name,
                             RS_Vector _insertionPoint,
                             RS_Vector _scaleFactor,
                             double _angle,
                             int _cols, int _rows, RS_Vector _spacing,
                             RS_BlockList *_blockSource,
                             RS2::UpdateMode _updateMode) :
        name(std::move(_name)), insertionPoint(_insertionPoint), scaleFactor(_scaleFactor), angle(_angle), cols(_cols),
        rows(_rows), spacing(_spacing), blockSource(_blockSource), updateMode(_updateMode) {
}

std::ostream &operator<<(std::ostream &os,
                         const RS_InsertData &d) {
    os << "(" << d.name.toLatin1().data() << ")";
    return os;
}

/**
 * @param parent The graphic this block belongs to.
 */
RS_Insert::RS_Insert(RS_EntityContainer *parent,
                     const RS_InsertData &d)
        : RS_EntityContainer(parent), _data(d) {

    _block = nullptr;

    if (_data.updateMode != RS2::NoUpdate) {
        update_local();
        //calculateBorders();
    }
}


RS_Entity *RS_Insert::clone() const {
    auto *copy = new RS_Insert(*this);
    copy->setOwner(isOwner());
    copy->initId();
    copy->detach();
    return copy;
}


/**
 * Updates the entity buffer of this insert entity. This method
 * needs to be called whenever the block this insert is based on changes.
 */
void RS_Insert::update() {
    update_local();

}

void RS_Insert::update_local() {
    RS_DEBUG->print("RS_Insert::update: name: %s", _data.name.toLatin1().data());

    if (!_updateEnabled) {
        return;
    }

    clear();

    RS_Block *block = getBlockForInsert();
    if (!block) {
        return;
    }

    if (isUndone()) {
        return;
    }

    if (fabs(_data.scaleFactor.x) < 1.0e-6 || fabs(_data.scaleFactor.y) < 1.0e-6) {
        return;
    }

    const bool isScaleFactorValid = ((_data.scaleFactor.x - _data.scaleFactor.y) > 1.0e-6);


    for (auto entity: *block) {
        for (int col = 0; col < _data.cols; ++col) {
            for (int row = 0; row < _data.rows; ++row) {
                if (entity->rtti() == RS2::EntityInsert &&
                    _data.updateMode != RS2::PreviewUpdate) {
                    dynamic_cast<RS_Insert *>(entity)->update();
                }
                RS_Entity *new_entity = createNewEntity(isScaleFactorValid, entity);
                new_entity->setParent(this);
                new_entity->setVisible(getFlag(RS2::FlagVisible));
                if (fabs(_data.scaleFactor.x) > 1.0e-6 &&
                    fabs(_data.scaleFactor.y) > 1.0e-6) {
                    new_entity->move(_data.insertionPoint +
                                     RS_Vector(_data.spacing.x / _data.scaleFactor.x * col,
                                               _data.spacing.y / _data.scaleFactor.y * row));
                } else {
                    new_entity->move(_data.insertionPoint);
                }
                // Move because of block base point:
                new_entity->move(block->getBasePoint() * -1);
                // Scale:
                new_entity->scale(_data.insertionPoint, _data.scaleFactor);
                // Rotate:
                new_entity->rotate(_data.insertionPoint, _data.angle);
                // Select:
                new_entity->setSelected(isSelected());

                // individual entities can be on indiv. layers
                RS_Pen new_entity_pen = new_entity->getPen(false);

                // color from block (free floating):
                if (new_entity_pen.getColor() == RS_Color(RS2::FlagByBlock)) {
                    new_entity_pen.setColor(getPen().getColor());
                }

                // line width from block (free floating):
                if (new_entity_pen.getWidth() == RS2::WidthByBlock) {
                    new_entity_pen.setWidth(getPen().getWidth());
                }

                // line type from block (free floating):
                if (new_entity_pen.getLineType() == RS2::LineByBlock) {
                    new_entity_pen.setLineType(getPen().getLineType());
                }

                // now that we've evaluated all flags, let's strip them:
                // TODO: strip all flags (width, line type)
                new_entity->setPen(new_entity_pen);

                new_entity->setUpdateEnabled(true);

                // insert must be updated even in preview mode
                if (_data.updateMode != RS2::PreviewUpdate
                    || new_entity->rtti() == RS2::EntityInsert) {
                    new_entity->update();
                }
                appendEntity(new_entity);
            }
        }
    }
    calculateBorders();

    RS_DEBUG->print("RS_Insert::update: OK");
}

RS_Entity *RS_Insert::createNewEntity(const bool isScaleFactorValid, RS_Entity *entity)  {
    RS_Entity *new_entity;

    if (!isScaleFactorValid) {
        return entity->clone();
    }

    switch (entity->rtti()) {
        case RS2::EntityArc : {
            auto *arc = dynamic_cast<RS_Arc *>(entity);
            new_entity = new RS_Ellipse{this,
                                        {arc->getCenter(), {arc->getRadius(), 0.},
                                         1, arc->getAngle1(), arc->getAngle2(),
                                         arc->isReversed()}
            };
            new_entity->setLayer(entity->getLayer());
            new_entity->setPen(entity->getPen(false));
        }
            break;
        case RS2::EntityCircle: {
            auto *circle = dynamic_cast<RS_Circle *>(entity);
            new_entity = new RS_Ellipse{this,
                                        {circle->getCenter(), {circle->getRadius(), 0.}, 1, 0.,
                                         2. * M_PI, false}
            };
            new_entity->setLayer(entity->getLayer());
            new_entity->setPen(entity->getPen(false));
        }
            break;
        default:
            new_entity = entity->clone();
    }
    new_entity->initId();
    new_entity->setUpdateEnabled(false);

    RS_Layer *new_entity_layer = new_entity->getLayer();//special fontchar block don't have
    // if entity layer are 0 set to insert layer to allow "1 layer control" bug ID #3602152
    if (new_entity_layer && new_entity_layer->getName() == "0") {
        new_entity->setLayer(getLayer());
    }

    return new_entity;
}


/**
 * @return Pointer to the block associated with this Insert or
 *   nullptr if the block couldn't be found. Blocks are requested
 *   from the blockSource if one was supplied and otherwise from
 *   the closest parent graphic.
 */
RS_Block *RS_Insert::getBlockForInsert() const {
    RS_Block *blk = nullptr;
    if (_block) {
        blk = _block;
        return blk;
    }

    RS_BlockList *blkList;

    if (!_data.blockSource) {
        if (getGraphic()) {
            blkList = getGraphic()->getBlockList();
        } else {
            blkList = nullptr;
        }
    } else {
        blkList = _data.blockSource;
    }

    if (blkList) {
        blk = blkList->find(_data.name);
    }

    if (blk) {
    }

    _block = blk;

    return blk;
}


/**
 * Is this insert visible? (re-implementation from RS_Entity)
 *
 * @return true Only if the entity and the block and the layer it is on
 * are visible.
 * The Layer might also be nullptr. In that case the layer visibility
 * is ignored.
 * The Block might also be nullptr. In that case the block visibility
 * is ignored.
 */
bool RS_Insert::isVisible() const {
    RS_Block *blk = getBlockForInsert();
    if (blk) {
        if (blk->isFrozen()) {
            return false;
        }
    }

    return RS_Entity::isVisible();
}


RS_VectorSolutions RS_Insert::getRefPoints() const {
    return RS_VectorSolutions{_data.insertionPoint};
}


RS_Vector RS_Insert::getNearestRef(const RS_Vector &coord,
                                   double *dist) const {

    return getRefPoints().getClosest(coord, dist);
}


void RS_Insert::move(const RS_Vector &offset) {
    RS_DEBUG->print("RS_Insert::move: offset: %f/%f",
                    offset.x, offset.y);
    RS_DEBUG->print("RS_Insert::move1: insertionPoint: %f/%f",
                    _data.insertionPoint.x, _data.insertionPoint.y);
    _data.insertionPoint.move(offset);
    RS_DEBUG->print("RS_Insert::move2: insertionPoint: %f/%f",
                    _data.insertionPoint.x, _data.insertionPoint.y);
    update();
}


void RS_Insert::rotate(const RS_Vector &center, const double &angle) {
    RS_DEBUG->print("RS_Insert::rotate1: insertionPoint: %f/%f "
                    "/ center: %f/%f",
                    _data.insertionPoint.x, _data.insertionPoint.y,
                    center.x, center.y);
    _data.insertionPoint.rotate(center, angle);
    _data.angle = RS_Math::correctAngle(_data.angle + angle);
    RS_DEBUG->print("RS_Insert::rotate2: insertionPoint: %f/%f",
                    _data.insertionPoint.x, _data.insertionPoint.y);
    update();
}

void RS_Insert::rotate(const RS_Vector &center, const RS_Vector &angleVector) {
    RS_DEBUG->print("RS_Insert::rotate1: insertionPoint: %f/%f "
                    "/ center: %f/%f",
                    _data.insertionPoint.x, _data.insertionPoint.y,
                    center.x, center.y);
    _data.insertionPoint.rotate(center, angleVector);
    _data.angle = RS_Math::correctAngle(_data.angle + angleVector.angle());
    RS_DEBUG->print("RS_Insert::rotate2: insertionPoint: %f/%f",
                    _data.insertionPoint.x, _data.insertionPoint.y);
    update();
}


void RS_Insert::scale(const RS_Vector &center, const RS_Vector &factor) {
    RS_DEBUG->print("RS_Insert::scale1: insertionPoint: %f/%f",
                    _data.insertionPoint.x, _data.insertionPoint.y);
    _data.insertionPoint.scale(center, factor);
    _data.scaleFactor.scale(RS_Vector(0.0, 0.0), factor);
    _data.spacing.scale(RS_Vector(0.0, 0.0), factor);
    RS_DEBUG->print("RS_Insert::scale2: insertionPoint: %f/%f",
                    _data.insertionPoint.x, _data.insertionPoint.y);
    update();
}


void RS_Insert::mirror(const RS_Vector &axisPoint1, const RS_Vector &axisPoint2) {
    _data.insertionPoint.mirror(axisPoint1, axisPoint2);

    RS_Vector vec = RS_Vector::polar(1.0, _data.angle);
    vec.mirror(RS_Vector(0.0, 0.0), axisPoint2 - axisPoint1);
    _data.angle = RS_Math::correctAngle(vec.angle() - M_PI);

    _data.scaleFactor.x *= -1;

    update();
}


std::ostream &operator<<(std::ostream &os, const RS_Insert &i) {
    os << " Insert: " << i.getData() << std::endl;
    return os;
}

