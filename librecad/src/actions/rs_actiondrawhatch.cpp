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
#include <QAction>
#include <QMouseEvent>
#include "rs_actiondrawhatch.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_hatch.h"
#include "rs_debug.h"

RS_ActionDrawHatch::RS_ActionDrawHatch(RS_EntityContainer &container, RS_GraphicView &graphicView)
        : RS_PreviewActionInterface("Draw Hatch", container, graphicView), data{new RS_HatchData{}} {
    _actionType = RS2::ActionDrawHatch;
}

RS_ActionDrawHatch::~RS_ActionDrawHatch() = default;

void RS_ActionDrawHatch::setShowArea(bool s) {
    m_bShowArea = s;
}

void RS_ActionDrawHatch::init(int status) {
    RS_ActionInterface::init(status);

    RS_Hatch tmp(container, *data);
    tmp.setLayerToActive();
    tmp.setPenToActive();
    if (GetDialogFactory()->requestHatchDialog(&tmp)) {
        *data = tmp.getData();
        trigger();
        finish(false);
        graphicView->redraw(RS2::RedrawDrawing);

    } else {
        finish(false);
    }
}

void RS_ActionDrawHatch::trigger() {

    RS_DEBUG->print("RS_ActionDrawHatch::trigger()");

    deselectUnhatchAbleEntities();

    // look for selected contours:
    bool isAnySelected = container->isAnySelected(RS2::ResolveAll);

    if (!isAnySelected) {
        std::cerr << "no contour selected\n";
        return;
    }

    std::unique_ptr<RS_Hatch> hatch_managed = std::make_unique<RS_Hatch>(container, *data);
    hatch_managed->setLayerToActive();
    hatch_managed->setPenToActive();

    // add selected contour:
    RS_EntityContainer * loop = collectAllSelectedEntities(hatch_managed.get());

    hatch_managed->addEntity(loop);
    if(!hatch_managed->validate() ) {
        GetDialogFactory()->commandMessage(tr("Invalid hatch area. Please check that "
                                              "the entities chosen form one or more closed contours."));
        return;
    }

    RS_Hatch* hatch = hatch_managed.release();  //is no longer responsible for the RS_Hatch pointer
    
    container->addEntity(hatch);

    if (_document) {
        _document->startUndoCycle();
        _document->addUndoable(hatch);
        _document->endUndoCycle();
    }
    hatch->update();

    graphicView->redraw(RS2::RedrawDrawing);

    bool printArea = true;
    switch (hatch->getUpdateError()) {
        case RS_Hatch::RS_HatchError::HATCH_OK :
            GetDialogFactory()->commandMessage(tr("Hatch created successfully."));
            break;
        case RS_Hatch::RS_HatchError::HATCH_INVALID_CONTOUR :
            GetDialogFactory()->commandMessage(tr("Hatch Error: Invalid contour found!"));
            printArea = false;
            break;
        case RS_Hatch::RS_HatchError::HATCH_PATTERN_NOT_FOUND :
            GetDialogFactory()->commandMessage(tr("Hatch Error: Pattern not found!"));
            break;
        case RS_Hatch::RS_HatchError::HATCH_TOO_SMALL :
            GetDialogFactory()->commandMessage(tr("Hatch Error: Contour or pattern too small!"));
            break;
        case RS_Hatch::RS_HatchError::HATCH_AREA_TOO_BIG :
            GetDialogFactory()->commandMessage(tr("Hatch Error: Contour too big!"));
            break;
        default :
            GetDialogFactory()->commandMessage(tr("Hatch Error: Undefined Error!"));
            printArea = false;
            break;
    }
    if (m_bShowArea && printArea) {
        GetDialogFactory()->commandMessage(tr("Total hatch area = %1").
                arg(hatch->getTotalArea(), 10, 'g', 8));
    }
}

RS_EntityContainer* RS_ActionDrawHatch::collectAllSelectedEntities(RS_Hatch *hatch) {
    auto *loop = new RS_EntityContainer(hatch);
    loop->setPen(RS_Pen(RS2::FlagInvalid));

    for (RS_Entity *e = container->firstEntity(RS2::ResolveAll);
         e != nullptr; e = container->nextEntity(RS2::ResolveAll)) {

        if (!e->isSelected()) {
            continue;
        }

        e->setSelected(false);
        // entity is part of a complex entity (spline, polyline, ..):
        if (e->getParent() &&
            // RVT - Don't de-delect the parent EntityPolyline, this is messing up the getFirst and getNext iterators
            //			    (e->getParent()->rtti()==RS2::EntitySpline ||
            //				 e->getParent()->rtti()==RS2::EntityPolyline)) {
            (e->getParent()->rtti() == RS2::EntitySpline)) {
            e->getParent()->setSelected(false);
        }
        RS_Entity *cp = e->clone();
        cp->setPen(RS_Pen(RS2::FlagInvalid));
        cp->reparent(loop);
        loop->addEntity(cp);
    }
    return loop;
}

bool RS_ActionDrawHatch::isEntityUnhatchAble(const RS_Entity& entity) {
    RS2::EntityType unhatch_able[] = {RS2::EntityHatch, RS2::EntityPoint, RS2::EntityMText, RS2::EntityText};

    if(  std::any_of(std::begin(unhatch_able), std::end(unhatch_able),
                      [&entity](RS2::EntityType type){ return (entity.rtti() == type);})) {
        return true;
    }
    return RS_Information::isDimension(entity.rtti());
}

void RS_ActionDrawHatch::deselectUnhatchAbleEntities() {
    for (auto e: *container) {
        if(! e->isSelected()) {
            continue;
        }
        if(isEntityUnhatchAble(*e)) {
            e->setSelected(false);
        }
    }
    RS_Entity *e;
    for (e = container->firstEntity(RS2::ResolveAll); e; e = container->nextEntity(RS2::ResolveAll)) {
        if(! e->isSelected()) {
            continue;
        }
        if(isEntityUnhatchAble(*e)) {
            e->setSelected(false);
        }
    }
}


void RS_ActionDrawHatch::mouseMoveEvent(QMouseEvent *) {
    RS_DEBUG->print("RS_ActionDrawHatch::mouseMoveEvent begin");

    /*if (getStatus()==SetPos) {
        RS_Vector mouse = snapPoint(e);
        pos = mouse;


        deletePreview();
		if (hatch && !hatch->isVisible()) {
            hatch->setVisible(true);
        }
        offset = RS_Vector(graphicView->toGuiDX(pos.x),
                           -graphicView->toGuiDY(pos.y));
        drawPreview();
}*/

    RS_DEBUG->print("RS_ActionDrawHatch::mouseMoveEvent end");
}


void RS_ActionDrawHatch::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        snapPoint(e);

        switch (getStatus()) {
            case ShowDialog:
                break;

            default:
                break;
        }
    } else if (e->button() == Qt::RightButton) {
        //deletePreview();
        init(getStatus() - 1);
    }
}


void RS_ActionDrawHatch::updateMouseButtonHints() {
    GetDialogFactory()->updateMouseWidget();
}

void RS_ActionDrawHatch::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

// EOF
