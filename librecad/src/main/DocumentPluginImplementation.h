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
#ifndef DOC_PLUGIN_INTERFACE_H
#define DOC_PLUGIN_INTERFACE_H

#include <QObject>

#include "document_interface.h"
#include "rs_graphic.h"


class DocumentPluginImplementation : public IDocumentPlugin {
public:
    DocumentPluginImplementation(RS_Document *d, RS_GraphicView *gv, QWidget *parent);

    void updateView() override;

    void addPoint(QPointF *start) override;

    void addLine(QPointF *start, QPointF *end) override;

    void addMText(const QString& txt, const QString& sty, QPointF *start,
                  double height, double angle, DPI::HAlign ha, DPI::VAlign va);

    void addText(const QString &txt, const QString &sty, QPointF *start,
                 double height, double angle, DPI::HAlign ha, DPI::VAlign va) override;

    void addCircle(QPointF *start, qreal radius) override;

    void addArc(QPointF *start, qreal radius, qreal a1, qreal a2) override;

    void addEllipse(QPointF *start, QPointF *end, qreal ratio, qreal a1, qreal a2) override;

    void addLines(std::vector<QPointF> const &points, bool closed) override;

    void addPolyline(std::vector<Plug_VertexData> const &points, bool closed) override;

    void addSplinePoints(std::vector<QPointF> const &points, bool closed) override;

    void addImage(int handle, QPointF *start, QPointF *uvr, QPointF *vvr,
                  int w, int h, const QString &name, int br, int con, int fade) override;

    void addInsert(const QString &name, QPointF ins, QPointF scale, qreal rot) override;

    QString addBlockFromDisk(const QString &fullName) override;

    void addEntity(Plug_Entity *handle) override;

    Plug_Entity *newEntity(enum DPI::ETYPE type) override;

    void removeEntity(Plug_Entity *ent) override;

    void updateEntity(RS_Entity *org, RS_Entity *newe);

    void setLayer(const QString &name) override;

    QString getCurrentLayer() override;

    QStringList getAllLayer() override;

    QStringList getAllBlocks() override;

    bool deleteLayer(const QString &name) override;

    void getCurrentLayerProperties(int *c, DPI::LineWidth *w, DPI::LineType *t) override;

    void getCurrentLayerProperties(int *c, QString *w, QString *t) override;

    void setCurrentLayerProperties(int c, DPI::LineWidth w, DPI::LineType t) override;

    void setCurrentLayerProperties(int c, QString const &w, QString const &t) override;

    bool getPoint(QPointF *point, const QString &message, QPointF *base) override;

    Plug_Entity *getEnt(const QString &message) override;

    bool getSelect(QList<Plug_Entity *> *sel, const QString &message) override;

    bool getAllEntities(QList<Plug_Entity *> *plug_entities, bool visible) override;

    bool getVariableInt(const QString &key, int *num) override;

    bool getVariableDouble(const QString &key, double *num) override;

    bool addVariable(const QString &key, int value, int code) override;

    bool addVariable(const QString &key, double value, int code) override;

    bool getInt(int *num, const QString &message, const QString &title) override;

    bool getReal(qreal *num, const QString &message, const QString &title) override;

    bool getString(QString *txt, const QString &message, const QString &title) override;

    QString realToStr(qreal num, int units, int prec) override;

    //method to handle undo in Plugin_Entity 
    bool addToUndo(RS_Entity *current, RS_Entity *modified);

private:
    RS_Document *doc;
    RS_Graphic *docGr;
    RS_GraphicView *gView;
    QWidget *main_window;
};

/*void addArc(QPointF *start);			->Without start
void addCircle(QPointF *start);			->Without start
more...
*/
#endif // DOC_PLUGIN_INTERFACE_H