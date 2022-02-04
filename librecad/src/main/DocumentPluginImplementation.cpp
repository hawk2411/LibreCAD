/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
**
**
** This file is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include "DocumentPluginImplementation.h"
#include <QEventLoop>
#include <QList>
#include <QInputDialog>
#include <QFileInfo>
#include "rs_graphicview.h"
#include "rs_eventhandler.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_line.h"
#include "rs_point.h"
#include "rs_mtext.h"
#include "rs_text.h"
#include "rs_layer.h"
#include "rs_image.h"
#include "rs_block.h"
#include "rs_insert.h"
#include "rs_polyline.h"
#include "rs_ellipse.h"
#include "lc_splinepoints.h"
#include "lc_undosection.h"
#include "intern/qc_actiongetpoint.h"
#include "intern/qc_actiongetselect.h"
#include "intern/qc_actiongetent.h"
#include "rs_math.h"
#include "rs_debug.h"
#include "ConvertLine.h"
#include "PluginEntity.h"

DocumentPluginImplementation::DocumentPluginImplementation(RS_Document *d, RS_GraphicView *gv, QWidget *parent) :
        doc(d), docGr(doc->getGraphic()), gView(gv), main_window(parent) {
}

bool DocumentPluginImplementation::addToUndo(RS_Entity *current, RS_Entity *modified) {
    if (doc) {
        doc->addEntity(modified);
        LC_UndoSection undo(doc);
        if (current->isSelected())
            current->setSelected(false);
        current->changeUndoState();
        undo.addUndoable(current);
        undo.addUndoable(modified);
        return true;
    } else
        RS_DEBUG->print("Doc_plugin_interface::addToUndo: currentContainer is nullptr");
    return false;
}

void DocumentPluginImplementation::updateView() {
    doc->setSelected(false);
    gView->getContainer()->calculateBorders();
    gView->redraw();
}

void DocumentPluginImplementation::addPoint(QPointF *start) {

    RS_Vector v1(start->x(), start->y());
    if (doc) {
        auto *entity = new RS_Point(doc, RS_PointData(v1));
        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else
        RS_DEBUG->print("Doc_plugin_interface::addPoint: currentContainer is nullptr");
}

void DocumentPluginImplementation::addLine(QPointF *start, QPointF *end) {

    RS_Vector v1(start->x(), start->y());
    RS_Vector v2(end->x(), end->y());
    if (doc) {
        auto *entity = new RS_Line{doc, v1, v2};
        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else
        RS_DEBUG->print("Doc_plugin_interface::addLine: currentContainer is nullptr");
}

void DocumentPluginImplementation::addMText(const QString& txt, const QString& sty, QPointF *start,
                                            double height, double angle, DPI::HAlign ha, DPI::VAlign va) {

    RS_Vector v1(start->x(), start->y());
    if (doc) {
        double width = 100.0;

        auto valign = static_cast <RS_MTextData::VAlign>(va);
        auto halign = static_cast <RS_MTextData::HAlign>(ha);
        RS_MTextData d(v1, height, width, valign, halign,
                       RS_MTextData::ByStyle, RS_MTextData::Exact, 0.0,
                       txt, sty, angle, RS2::Update);
        auto *entity = new RS_MText(doc, d);

        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else
        RS_DEBUG->print("Doc_plugin_interface::addMtext: currentContainer is nullptr");
}

void DocumentPluginImplementation::addText(const QString &txt, const QString &sty, QPointF *start,
                                           double height, double angle, DPI::HAlign ha, DPI::VAlign va) {

    RS_Vector v1(start->x(), start->y());
    if (doc) {
        double width = 1.0;

        auto valign = static_cast <RS_TextData::VAlign>(va);
        auto halign = static_cast <RS_TextData::HAlign>(ha);
        RS_TextData d(v1, v1, height, width, valign, halign,
                      RS_TextData::None, txt, sty, angle, RS2::Update);
        auto *entity = new RS_Text(doc, d);

        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else
        RS_DEBUG->print("Doc_plugin_interface::addText: currentContainer is nullptr");
}

void DocumentPluginImplementation::addCircle(QPointF *start, qreal radius) {
    if (doc) {
        RS_Vector v(start->x(), start->y());
        RS_CircleData d(v, radius);
        auto *entity = new RS_Circle(doc, d);

        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else {
        RS_DEBUG->print("Doc_plugin_interface::addCircle: currentContainer is nullptr");
    }
}

void DocumentPluginImplementation::addArc(QPointF *start, qreal radius, qreal a1, qreal a2) {
    if (doc) {
        RS_Vector v(start->x(), start->y());
        RS_ArcData d(v, radius,
                     RS_Math::deg2rad(a1),
                     RS_Math::deg2rad(a2),
                     false);
        auto *entity = new RS_Arc(doc, d);
        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else
        RS_DEBUG->print("Doc_plugin_interface::addArc: currentContainer is nullptr");
}

void DocumentPluginImplementation::addEllipse(QPointF *start, QPointF *end, qreal ratio, qreal a1, qreal a2) {
    if (doc) {
        RS_Vector v1(start->x(), start->y());
        RS_Vector v2(end->x(), end->y());

        RS_EllipseData ed{v1, v2, ratio, a1, a2, false};
        auto *entity = new RS_Ellipse(doc, ed);

        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else {
        RS_DEBUG->print("Doc_plugin_interface::addEllipse: currentContainer is nullptr");
    }
}

void DocumentPluginImplementation::addLines(std::vector<QPointF> const &points, bool closed) {
    if (doc) {
        RS_LineData data;

        LC_UndoSection undo(doc);
        data._endpoint = RS_Vector(points.front().x(), points.front().y());

        for (size_t i = 1; i < points.size(); ++i) {
            data._startpoint = data._endpoint;
            data._endpoint = RS_Vector(points[i].x(), points[i].y());
            auto *line = new RS_Line(doc, data);
            doc->addEntity(line);
            undo.addUndoable(line);
        }
        if (closed) {
            data._startpoint = data._endpoint;
            data._endpoint = RS_Vector(points.front().x(), points.front().y());
            auto *line = new RS_Line(doc, data);
            doc->addEntity(line);
            undo.addUndoable(line);
        }
    } else
        RS_DEBUG->print("%s: currentContainer is nullptr", __func__);
}

void DocumentPluginImplementation::addPolyline(std::vector<Plug_VertexData> const &points, bool closed) {
    if (doc) {
        RS_PolylineData data;
        if (closed) {
            data.setFlag(RS2::FlagClosed);
        }
        auto *entity = new RS_Polyline(doc, data);

        for (auto const &pt: points) {
            entity->addVertex(RS_Vector(pt.point.x(), pt.point.y()), pt.bulge);
        }

        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else {
        RS_DEBUG->print("%s: currentContainer is nullptr", __func__);
    }
}

void DocumentPluginImplementation::addSplinePoints(std::vector<QPointF> const &points, bool closed) {
    if (doc) {
        LC_SplinePointsData data(closed, false); //cut = false
        for (auto const &pt: points) {
            data.splinePoints.emplace_back(RS_Vector(pt.x(), pt.y()));
        }

        auto *entity = new LC_SplinePoints(doc, data);

        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else
        RS_DEBUG->print("%s: currentContainer is nullptr", __func__);
}

void DocumentPluginImplementation::addImage(int handle, QPointF *start, QPointF *uvr, QPointF *vvr,
                                            int w, int h, const QString &name, int br, int con, int fade) {
    if (doc) {
        RS_Vector ip(start->x(), start->y());
        RS_Vector uv(uvr->x(), uvr->y());
        RS_Vector vv(vvr->x(), vvr->y());
        RS_Vector size(w, h);

        auto *image =
                new RS_Image(
                        doc,
                        RS_ImageData(handle /*QString(data.ref.c_str()).toInt(nullptr, 16)*/,
                                     ip, uv, vv,
                                     size,
                                     name,
                                     br,
                                     con,
                                     fade));

        doc->addEntity(image);
        LC_UndoSection undo(doc);
        undo.addUndoable(image);
    } else
        RS_DEBUG->print("Doc_plugin_interface::addImage: currentContainer is nullptr");
}

void DocumentPluginImplementation::addInsert(const QString &name, QPointF ins, QPointF scale, qreal rot) {
    if (doc) {
        RS_Vector ip(ins.x(), ins.y());
        RS_Vector sp(scale.x(), scale.y());

        RS_InsertData id(name, ip, sp, rot, 1, 1, RS_Vector(0.0, 0.0));
        auto *entity = new RS_Insert(doc, id);

        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else
        RS_DEBUG->print("Doc_plugin_interface::addInsert: currentContainer is nullptr");
}

/*TODO RLZ: add undo support in this method*/
QString DocumentPluginImplementation::addBlockFromDisk(const QString &fullName) {
    if (fullName.isEmpty() || !doc)
        return nullptr;
    RS_BlockList *blockList = doc->getBlockList();
    if (!blockList)
        return nullptr;

    QFileInfo fi(fullName);
    QString s = fi.completeBaseName();

    QString name = blockList->newName(s);

    if (fi.isReadable()) {
        RS_BlockData d(name, RS_Vector(0, 0), false);
        auto *b = new RS_Block(doc, d);
        RS_Graphic g;
        if (!g.open(fi.absoluteFilePath(), RS2::FormatUnknown)) {
            RS_DEBUG->print(RS_Debug::D_WARNING,
                            "Doc_plugin_interface::addBlockFromDisk: Cannot open file: %s");
            delete b;
            return nullptr;
        }
        for (auto & layer : *g.getLayerList()) {
            RS_Layer *nl = layer->clone();
            docGr->getLayerList()->add(nl);
        }
        RS_BlockList *bl = g.getBlockList();
        for (auto & block : *bl) {
            auto *nb = (RS_Block *) block->clone();
            docGr->getBlockList()->add(nb, true);
        }
        for (unsigned int i = 0; i < g.count(); i++) {
            RS_Entity *e = g.entityAt(i)->clone();
            e->reparent(b);
            b->addEntity(e);
        }
        docGr->getBlockList()->add(b, true);
        return name;

    } else {
        return nullptr;
    }
}

void DocumentPluginImplementation::addEntity(Plug_Entity *handle) {
    if (doc) {
        RS_Entity *ent = (reinterpret_cast<PluginEntity *>(handle))->getEnt();
        if (ent) {
            doc->addEntity(ent);
            LC_UndoSection undo(doc);
            undo.addUndoable(ent);
        }
    } else
        RS_DEBUG->print("Doc_plugin_interface::addEntity: currentContainer is nullptr");
}

/*newEntity not added into graphic, then not needed undo support*/
Plug_Entity *DocumentPluginImplementation::newEntity(enum DPI::ETYPE type) {
    auto *e = new PluginEntity(doc, type);
    if (!(e->isValid())) {
        delete e;
        return nullptr;
    }
    return reinterpret_cast<Plug_Entity *>(e);
}

/*TODO RLZ: add undo support in this method*/
void DocumentPluginImplementation::removeEntity(Plug_Entity *ent) {
    RS_Entity *e = (reinterpret_cast<PluginEntity *>(ent))->getEnt();
    if (doc && e) {
        LC_UndoSection undo(doc);
        e->setSelected(false);
        e->changeUndoState();
        undo.addUndoable(e);

        gView->redraw(RS2::RedrawDrawing);
    }
}

void DocumentPluginImplementation::updateEntity(RS_Entity *org, RS_Entity *newe) {
    if (doc) {
        LC_UndoSection undo(doc);
        doc->addEntity(newe);
        undo.addUndoable(newe);
        undo.addUndoable(org);
        org->setUndoState(true);
    } else
        RS_DEBUG->print("Doc_plugin_interface::addEntity: currentContainer is nullptr");
}

/*TODO RLZ: add undo support in the remaining methods*/
void DocumentPluginImplementation::setLayer(const QString &name) {
    RS_LayerList *listLay = doc->getLayerList();
    RS_Layer *lay = listLay->find(name);
    if (!lay) {
        lay = new RS_Layer(name);
        docGr->getLayerList()->add(lay);
    }
    listLay->activate(lay, true);
}

QString DocumentPluginImplementation::getCurrentLayer() {
    return docGr->getLayerList()->getActive()->getName();
}

QStringList DocumentPluginImplementation::getAllLayer() {
    QStringList listName;
    RS_LayerList *listLay = doc->getLayerList();
    for (auto & layer: *listLay) {
        listName << layer->getName();
    }
    return listName;
}

QStringList DocumentPluginImplementation::getAllBlocks() {
    QStringList listName;
    RS_BlockList *listBlk = doc->getBlockList();
    for (auto & block : *listBlk) {
        listName << block->getName();
    }
    return listName;
}

bool DocumentPluginImplementation::deleteLayer(const QString &name) {
    RS_Layer *layer = docGr->getLayerList()->find(name);
    if (layer) {
        docGr->removeLayer(layer);
        return true;
    }
    return false;
}

void DocumentPluginImplementation::getCurrentLayerProperties(int *c, DPI::LineWidth *w, DPI::LineType *t) {
    RS_Pen pen = docGr->getLayerList()->getActive()->getPen();
    *c = pen.getColor().toIntColor();
//    RS_Color col = pen.getColor();
//    c->setRgb(col.red(), col.green(), col.blue());
    *w = static_cast<DPI::LineWidth>(pen.getWidth());
    *t = static_cast<DPI::LineType>(pen.getLineType());
}

void DocumentPluginImplementation::getCurrentLayerProperties(int *c, QString *w, QString *t) {
    RS_Pen pen = docGr->getLayerList()->getActive()->getPen();
    *c = pen.getColor().toIntColor();
//    RS_Color col = pen.getColor();
//    c->setRgb(col.red(), col.green(), col.blue());
    w->clear();
    w->append(getConverter().lw2str(pen.getWidth()));
    t->clear();
    t->append(getConverter().lt2str(pen.getLineType()));
}

void DocumentPluginImplementation::setCurrentLayerProperties(int c, DPI::LineWidth w, DPI::LineType t) {
    RS_Layer *layer = docGr->getLayerList()->getActive();
    if (layer) {
        RS_Color co;
        co.fromIntColor(c);
        RS_Pen pen(co, static_cast<RS2::LineWidth>(w), static_cast<RS2::LineType>(t));
//        RS_Pen pen(RS_Color(c), static_cast<RS2::LineWidth>(w), static_cast<RS2::LineType>(t));
        layer->setPen(pen);
    }
}

void DocumentPluginImplementation::setCurrentLayerProperties(int c, QString const &w,
                                                             QString const &t) {
    RS_Layer *layer = docGr->getLayerList()->getActive();
    if (layer) {
        RS_Color co;
        co.fromIntColor(c);
        RS_Pen pen(co, getConverter().str2lw(w), getConverter().str2lt(t));
//        RS_Pen pen(RS_Color(c), getConverter().str2lw(w), getConverter().str2lt(t));
        layer->setPen(pen);
    }
}

bool DocumentPluginImplementation::getPoint(QPointF *point, const QString &message,
                                            QPointF *base) {
    bool status = false;
    auto *actionPoint = new QC_ActionGetPoint(*doc, *gView);
    if (!(message.isEmpty())) { actionPoint->setMessage(message); }
    gView->killAllActions();
    gView->setCurrentAction(actionPoint);
    if (base) actionPoint->setBasepoint(base);
    QEventLoop ev;
    while (!actionPoint->isCompleted()) {
        ev.processEvents();
        if (!gView->getEventHandler()->hasAction()) {
            break;
        }
    }
    if (actionPoint->isCompleted() && !actionPoint->wasCanceled()) {
        actionPoint->getPoint(point);
        status = true;
    }
    //RLZ: delete QC_ActionGetPoint. Investigate how to kill only this action
    gView->killAllActions();
    return status;
}

Plug_Entity *DocumentPluginImplementation::getEnt(const QString &message) {
    auto *entity = new QC_ActionGetEnt(*doc, *gView);

    if (!(message.isEmpty())) { entity->setMessage(message); }
    gView->killAllActions();
    gView->setCurrentAction(entity);
    QEventLoop ev;
    while (!entity->isCompleted()) {
        ev.processEvents();
        if (!gView->getEventHandler()->hasAction()) { break; }
    }

    auto *e = reinterpret_cast<Plug_Entity *>(entity->getSelected(this));
    entity->finish();
    gView->killAllActions();
    return e;
}

bool DocumentPluginImplementation::getSelect(QList<Plug_Entity *> *sel, const QString &message) {
    bool status = false;
    auto *action = new QC_ActionGetSelect(*doc, *gView);
    if (!(message.isEmpty())) {
        action->setMessage(message);
    }
    gView->killAllActions();
    gView->setCurrentAction(action);
    QEventLoop ev;
    while (!action->isCompleted()) {
        ev.processEvents();
        if (!gView->getEventHandler()->hasAction()) { break; }
    }
    // qDebug() << "getSelect: passed event loop";
//    check if a are cancelled by the user issue #349
    RS_EventHandler *event_handle = gView->getEventHandler();
    if (event_handle && event_handle->isValid(action)) {
        action->getSelected(sel, this);
        status = true;
    }
    gView->killAllActions();
    return status;

}

bool DocumentPluginImplementation::getAllEntities(QList<Plug_Entity *> *plug_entities, bool visible) {
    for (auto e: *doc) {
        if (e->isVisible() || !visible) {
            auto *pe = new PluginEntity(e, this);
            plug_entities->append(reinterpret_cast<Plug_Entity *>(pe));
        }
    }
    return true;
}

bool DocumentPluginImplementation::getVariableInt(const QString &key, int *num) {
    if ((*num = docGr->getVariableInt(key, 0)))
        return true;
    else
        return false;
}

bool DocumentPluginImplementation::getVariableDouble(const QString &key, double *num) {
    *num = docGr->getVariableDouble(key, 0.0);
    return (*num != 0.0);
}

bool DocumentPluginImplementation::addVariable(const QString &key, int value, int code) {
    docGr->addVariable(key, value, code);
    if (key.startsWith("$DIM"))
        doc->updateDimensions(true);
    return true;
}

bool DocumentPluginImplementation::addVariable(const QString &key, double value, int code) {
    docGr->addVariable(key, value, code);
    if (key.startsWith("$DIM"))
        doc->updateDimensions(true);
    return true;
}

bool DocumentPluginImplementation::getInt(int *num, const QString &message, const QString &title) {
    bool ok;
    QString msg, tit;
    if (message.isEmpty())
        msg = QObject::tr("enter an integer number");
    else
        msg = message;
    if (title.isEmpty())
        tit = QObject::tr("LibreCAD query");
    else
        tit = title;

    int data = QInputDialog::getInt(main_window, tit, msg, 0, -2147483647, 2147483647, 1, &ok);

    if (ok)
        *num = data;
    return ok;
}

bool DocumentPluginImplementation::getReal(qreal *num, const QString &message, const QString &title) {
    bool ok;
    QString msg, tit;
    if (message.isEmpty())
        msg = QObject::tr("enter a number");
    else
        msg = message;
    if (title.isEmpty())
        tit = QObject::tr("LibreCAD query");
    else
        tit = title;

    double data = QInputDialog::getDouble(main_window, tit, msg, 0, -2147483647, 2147483647, 4, &ok);
    if (ok)
        *num = data;
    return ok;
}

bool DocumentPluginImplementation::getString(QString *txt, const QString &message, const QString &title) {
    bool ok;
    QString msg, tit;
    if (message.isEmpty())
        msg = QObject::tr("enter text");
    else
        msg = message;
    if (title.isEmpty())
        tit = QObject::tr("LibreCAD query");
    else
        tit = title;

    QString text = QInputDialog::getText(main_window, tit, msg, QLineEdit::Normal,
                                         QString(), &ok);
    if (ok && !text.isEmpty()) {
        txt->clear();
        txt->append(text);
    }
    return ok;
}

QString DocumentPluginImplementation::realToStr(const qreal num, const int units, const int prec) {
    RS2::LinearFormat lf;
    int pr = prec;
    if (pr == 0)
        pr = docGr->getLinearPrecision();

    switch (units) {
        case 0:
            lf = docGr->getLinearFormat();
            break;
        case 1:
            lf = RS2::Scientific;
            break;
        case 3:
            lf = RS2::Engineering;
            break;
        case 4:
            lf = RS2::Architectural;
            break;
        case 5:
            lf = RS2::Fractional;
            break;
        case 6:
            lf = RS2::ArchitecturalMetric;
            break;
        default:
            lf = RS2::Decimal;
    }

    QString msg = RS_Units::formatLinear(num, RS2::None, lf, pr);
    return msg;
}
