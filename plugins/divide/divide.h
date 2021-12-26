/****************************************************************************
*  divide.h - divide lines, circles and arcs                                *
*                                                                           *
*  Copyright (C) 2018 mad-hatter                                            *
*                                                                           *
*  This library is free software, licensed under the terms of the GNU       *
*  General Public License as published by the Free Software Foundation,     *
*  either version 2 of the License, or (at your option) any later version.  *
*  You should have received a copy of the GNU General Public License        *
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
****************************************************************************/

#ifndef DIVIDE_H
#define DIVIDE_H

#include "qc_plugininterface.h"
#include "document_interface.h"

class Plug_Entity;

class divide : public QObject, QC_PluginInterface {
Q_OBJECT
    Q_INTERFACES(QC_PluginInterface)
    Q_PLUGIN_METADATA(IID LC_DocumentInterface_iid FILE "divide.json")

public:
    PluginCapabilities getCapabilities() const Q_DECL_OVERRIDE;

    QString name() const Q_DECL_OVERRIDE;

    void execComm(IDocumentPlugin *doc,
                  QWidget *parent, QString cmd) Q_DECL_OVERRIDE;

public slots:

    void gotReturnedDataSlot(QString);

private:
    QString getStrData(Plug_Entity *ent);

    static double polylineRadius(const Plug_VertexData &ptA,
                                 const Plug_VertexData &ptB);

    IDocumentPlugin *document;
    QString returnedData;

    static double findHypLength(double, double, double, double);

    static QPointF findLineEndPoint(double, double, double, double);

    static QPoint findWindowCentre();

    static QPointF findStartX(double, double, QPointF);

    void drawTick(QPointF, double, double);

    void segmentLine(QPointF, QPointF, QPointF, const QString &, int, int);

    void segment(QPointF *, double, double, double, const QString &);

    static bool jumpOutWithError(bool user_button, QList<Plug_Entity *> *obj_list);
    static void clearObjectList(QList<Plug_Entity *> *obj_list);

    void handleCircle(const QString &entityType, const QList<QString> &pData, const QList<QString> &data, bool ticks,
                      bool breaks);

    void
    handleLine(const QString &entityType, const QList<QString> &pData, const QList<QString> &data, bool ticks,
               bool breaks);
    void
    handleArc(const QString &entityType, const QList<QString> &pData, const QList<QString> &data, bool ticks,
               bool breaks);
};

#endif //end DIVIDE_H
