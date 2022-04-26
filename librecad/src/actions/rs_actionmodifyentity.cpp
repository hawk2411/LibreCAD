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

#include "rs_actionmodifyentity.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_debug.h"



RS_ActionModifyEntity::RS_ActionModifyEntity(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
		:RS_ActionInterface("Modify Entity", container, graphicView)
		,en(nullptr)
{
    _actionType=RS2::ActionModifyEntity;
}

void RS_ActionModifyEntity::trigger() {
    if (en) {
        RS_Entity* clone = en->clone();
        if (GetDialogFactory()->requestModifyEntityDialog(clone)) {
            _container->addEntity(clone);

            _graphicView->deleteEntity(en);
                        en->setSelected(false);

                        clone->setSelected(false);
            _graphicView->drawEntity(clone);

            if (_document) {
                _document->startUndoCycle();

                _document->addUndoable(clone);
                en->setUndoState(true);
                _document->addUndoable(en);

                _document->endUndoCycle();
            }
            GetDialogFactory()->updateSelectionWidget(_container->countSelected(true, {}), _container->totalSelectedLength());
        } else {
            delete clone;
        }

    } else {
        RS_DEBUG->print("RS_ActionModifyEntity::trigger: Entity is NULL\n");
    }
}



void RS_ActionModifyEntity::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton) {
        init(getStatus()-1);
    } else {
        en = catchEntity(e);
        trigger();
    }
}



void RS_ActionModifyEntity::updateMouseCursor() {
    _graphicView->setMouseCursor(RS2::SelectCursor);
}

void RS_ActionModifyEntity::updateMouseButtonHints() {
    GetDialogFactory()->updateMouseWidget(tr("Click on entity to modify"), tr("Cancel"));
}

// EOF
