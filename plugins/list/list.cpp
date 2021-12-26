/*****************************************************************************/
/*  list.cpp - List selected entities                                        */
/*                                                                           */
/*  Copyright (C) 2011 Rallaz, rallazz@gmail.com                             */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 2 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/


#include <QTextEdit>
//#include <QColor>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <cmath>
#include "list.h"

QString LC_List::name() const {
    return (tr("List entities"));
}

PluginCapabilities LC_List::getCapabilities() const {
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
            << PluginMenuLocation("plugins_menu", tr("List entities"));
    return pluginCapabilities;
}

void LC_List::execComm(IDocumentPlugin *doc,
                       QWidget *parent, QString cmd) {
    Q_UNUSED(parent);
    Q_UNUSED(cmd);
    document = doc;
    QList<Plug_Entity *> obj;
    bool yes = doc->getSelect(&obj, "");
    if (!yes || obj.isEmpty()) return;

    QString text;
    for (int i = 0; i < obj.size(); ++i) {
        text.append(QString("%1 %2: ").arg(tr("n")).arg(i + 1));
        text.append(getStrData(obj.at(i)));
        text.append("\n");
    }
    lc_ListDlg dlg(parent);
    dlg.setText(text);
    dlg.exec();

    while (!obj.isEmpty())
        delete obj.takeFirst();
}

QString LC_List::getStrData(Plug_Entity *ent) {
    if (nullptr == ent)
        return QString("%1\n").arg(tr("Empty Entity"));

    QHash<int, QVariant> data;
    QString strData(""),
            strEntity("%1\n"),
            strCommon("  %1: %2\n"),
            strSpecific("    %1: %2\n"),
            strSpecificXY(QString("    %1: %2=%3 %4=%5\n").arg("%1", tr("X"), "%2", tr("Y"), "%3"));
    double numA{0.0};
    double numB{0.0};
    double numC{0.0};
    QPointF ptA, ptB, ptC;
    int intA{0};
    int intB{0};

    //common entity data
    ent->getData(&data);
    strData = strCommon.arg(tr("Layer"), data.value(DPI::LAYER).toString());
    int col = data.value(DPI::COLOR).toInt();
    strData.append(strCommon.arg(tr("Color"), ent->intColor2str(col)));
    strData.append(strCommon.arg(tr("Line type"), data.value(DPI::LTYPE).toString()));
    strData.append(strCommon.arg(tr("Line thickness"), data.value(DPI::LWIDTH).toString()));
    strData.append(strCommon.arg(tr("ID")).arg(data.value(DPI::EID).toLongLong()));

    //specific entity data
    int et = data.value(DPI::ETYPE).toInt();
    switch (et) {
        case DPI::POINT:
            strData.prepend(strEntity.arg(tr("POINT")));
            strData.append(strSpecificXY.arg(tr("in point"),
                                             document->realToStr(data.value(DPI::STARTX).toDouble(), 0, 0),
                                             document->realToStr(data.value(DPI::STARTY).toDouble(), 0, 0)));
            break;
        case DPI::LINE:
            strData.prepend(strEntity.arg(tr("LINE")));
            ptA.setX(data.value(DPI::STARTX).toDouble());
            ptA.setY(data.value(DPI::STARTY).toDouble());
            ptB.setX(data.value(DPI::ENDX).toDouble());
            ptB.setY(data.value(DPI::ENDY).toDouble());
            strData.append(strSpecificXY.arg(tr("from point"),
                                             document->realToStr(ptA.x(), 0, 0),
                                             document->realToStr(ptA.y(), 0, 0)));
            strData.append(strSpecificXY.arg(tr("to point"),
                                             document->realToStr(ptB.x(), 0, 0),
                                             document->realToStr(ptB.y(), 0, 0)));
            ptC = ptB - ptA;
            numA = sqrt((ptC.x() * ptC.x()) + (ptC.y() * ptC.y()));
            strData.append(strSpecific.arg(tr("length"), document->realToStr(numA, 0, 0)));
            numB = asin(ptC.y() / numA);
            numC = numB * 180 / M_PI;
            if (ptC.x() < 0) numC = 180 - numC;
            if (numC < 0) numC = 360 + numC;
            strData.append(strSpecific.arg(tr("Angle in XY plane"), document->realToStr(numC, 0, 0)));
            strData.append(strSpecificXY.arg(tr("Inc."),
                                             document->realToStr(ptC.x(), 0, 0),
                                             document->realToStr(ptC.y(), 0, 0)));
            break;
        case DPI::ARC:
            strData.prepend(strEntity.arg(tr("ARC")));
            strData.append(strSpecificXY.arg(tr("center point"),
                                             document->realToStr(data.value(DPI::STARTX).toDouble(), 0, 0),
                                             document->realToStr(data.value(DPI::STARTY).toDouble(), 0, 0)));
            numA = data.value(DPI::RADIUS).toDouble();
            numB = data.value(DPI::STARTANGLE).toDouble();
            numC = data.value(DPI::ENDANGLE).toDouble();
            strData.append(strSpecific.arg(tr("radius"), document->realToStr(numA, 0, 0)));
            strData.append(strSpecific.arg(tr("initial angle"), document->realToStr(numB * 180 / M_PI, 0, 0)));
            strData.append(strSpecific.arg(tr("final angle"), document->realToStr(numC * 180 / M_PI, 0, 0)));
            if (numB > numC) {
                numB -= 2.0 * M_PI;
            }
            strData.append(strSpecific.arg(tr("length"), document->realToStr((numC - numB) * numA, 0, 0)));
            break;
        case DPI::CIRCLE:
            strData.prepend(strEntity.arg(tr("CIRCLE")));
            strData.append(strSpecificXY.arg(tr("center point"),
                                             document->realToStr(data.value(DPI::STARTX).toDouble(), 0, 0),
                                             document->realToStr(data.value(DPI::STARTY).toDouble(), 0, 0)));
            numA = data.value(DPI::RADIUS).toDouble();
            strData.append(strSpecific.arg(tr("radius"), document->realToStr(numA, 0, 0)));
            strData.append(strSpecific.arg(tr("circumference"), document->realToStr(numA * 2 * M_PI, 0, 0)));
            strData.append(strSpecific.arg(tr("area"), document->realToStr(numA * numA * M_PI, 0, 0)));
            break;
        case DPI::ELLIPSE://toy aqui
            strData.prepend(strEntity.arg(tr("ELLIPSE")));
            strData.append(strSpecificXY.arg(tr("center point"),
                                             document->realToStr(data.value(DPI::STARTX).toDouble(), 0, 0),
                                             document->realToStr(data.value(DPI::STARTY).toDouble(), 0, 0)));
            strData.append(strSpecificXY.arg(tr("major axis"),
                                             document->realToStr(data.value(DPI::ENDX).toDouble(), 0, 0),
                                             document->realToStr(data.value(DPI::ENDY).toDouble(), 0, 0)));
/*        strData.append( QString(tr("   minor axis: X=%1 Y=%2\n")).arg(
                data.value(DPI::ENDX).toDouble()).arg(
                data.value(DPI::ENDY).toDouble() ) );
        strData.append( QString(tr("   start point: X=%1 Y=%2\n")).arg(
                data.value(DPI::ENDX).toDouble()).arg(
                data.value(DPI::ENDY).toDouble() ) );
        strData.append( QString(tr("   end point: X=%1 Y=%2\n")).arg(
                data.value(DPI::ENDX).toDouble()).arg(
                data.value(DPI::ENDY).toDouble() ) );
        strData.append( QString(tr("   initial angle: %1\n")).arg(numB*180/M_PI) );
        strData.append( QString(tr("   final angle: %1\n")).arg(numC*180/M_PI) );
        strData.append( QString(tr("   radius ratio: %1\n")).arg(numC*180/M_PI) );*/
            break;

        case DPI::CONSTRUCTIONLINE:
            strData.prepend(strEntity.arg(tr("CONSTRUCTIONLINE")));
            break;
        case DPI::OVERLAYBOX:
            strData.prepend(strEntity.arg(tr("OVERLAYBOX")));
            break;
        case DPI::SOLID:
            strData.prepend(strEntity.arg(tr("SOLID")));
            break;
//container entities
        case DPI::MTEXT:
            strData.prepend(strEntity.arg(tr("MTEXT")));
            break;
        case DPI::TEXT:
            strData.prepend(strEntity.arg(tr("TEXT")));
            strData.append(strSpecificXY.arg(tr("in point"),
                                             document->realToStr(data.value(DPI::STARTX).toDouble(), 0, 0),
                                             document->realToStr(data.value(DPI::STARTY).toDouble(), 0, 0)));

            strData.append(strSpecific.arg(tr("TEXTCONTENT"), data.value(DPI::TEXTCONTENT).toString()));
            break;
        case DPI::INSERT:
            strData.prepend(strEntity.arg(tr("INSERT")));
            ptA.setX(data.value(DPI::STARTX).toDouble());
            ptA.setY(data.value(DPI::STARTY).toDouble());
            strData.append(strSpecific.arg(tr("Name"),
                                           data.value(DPI::BLKNAME).toString()));
            strData.append(strSpecificXY.arg(tr("Insertion point"),
                                             document->realToStr(ptA.x(), 0, 0),
                                             document->realToStr(ptA.y(), 0, 0)));
            strData.append(strSpecificXY.arg(tr("Scale"),
                                             document->realToStr(data.value(DPI::XSCALE).toDouble(), 0, 0),
                                             document->realToStr(data.value(DPI::YSCALE).toDouble(), 0, 0)));
            strData.append(strSpecific.arg(tr("Rotation"), QString("%1°"),
                                           document->realToStr(data.value(DPI::STARTANGLE).toDouble() * 180 / M_PI, 0,
                                                               0)));
            intA = data.value(DPI::COLCOUNT).toInt();
            intB = data.value(DPI::ROWCOUNT).toInt();
            if (1 < intA || 1 < intB) {
                strData.append(strSpecific.arg(tr("Columns/Rows"),
                                               QString("%1 / %2").arg(intA).arg(intB)));
                strData.append(strSpecific.arg(tr("Column/Row Spacing"),
                                               QString("%1 / %2"),
                                               document->realToStr(data.value(DPI::COLSPACE).toDouble(), 0, 0),
                                               document->realToStr(data.value(DPI::ROWSPACE).toDouble(), 0, 0)));
            }
            break;
        case DPI::POLYLINE: {
            strData.prepend(strEntity.arg(tr("POLYLINE")));
            strData.append(strSpecific.arg(tr("Closed"),
                                           (0 == data.value(DPI::CLOSEPOLY).toInt()) ? tr("No") : tr("Yes")));
            strData.append(strSpecific.arg(tr("Vertices"), ""));
            QList<Plug_VertexData> vl;
            ent->getPolylineData(&vl);
            int iVertices = vl.size();
            for (int i = 0; i < iVertices; ++i) {
                strData.append(strSpecificXY.arg(tr("in point"),
                                                 document->realToStr(vl.at(i).point.x(), 0, 0),
                                                 document->realToStr(vl.at(i).point.y(), 0, 0)));
                if (0 != vl.at(i).bulge) {
                    strData.append(strSpecific.arg(tr("radius"),
                                                   document->realToStr(
                                                           polylineRadius(vl.at(i), vl.at((i + 1) % iVertices)), 0,
                                                           0)));
                }
            }
            break;
        }
        case DPI::IMAGE:
            strData.prepend(strEntity.arg(tr("IMAGE")));
            break;
        case DPI::SPLINE:
            strData.prepend(strEntity.arg(tr("SPLINE")));
            break;
        case DPI::SPLINEPOINTS:
            strData.prepend(strEntity.arg(tr("SPLINEPOINTS")));
            break;
        case DPI::HATCH:
            strData.prepend(strEntity.arg(tr("HATCH")));
            break;
        case DPI::DIMLEADER:
            strData.prepend(strEntity.arg(tr("DIMLEADER")));
            break;
        case DPI::DIMALIGNED:
            strData.prepend(strEntity.arg(tr("DIMALIGNED")));
            break;
        case DPI::DIMLINEAR:
            strData.prepend(strEntity.arg(tr("DIMLINEAR")));
            break;
        case DPI::DIMRADIAL:
            strData.prepend(strEntity.arg(tr("DIMRADIAL")));
            break;
        case DPI::DIMDIAMETRIC:
            strData.prepend(strEntity.arg(tr("DIMDIAMETRIC")));
            break;
        case DPI::DIMANGULAR:
            strData.prepend(strEntity.arg(tr("DIMANGULAR")));
            break;
        default:
            strData.prepend(strEntity.arg(tr("UNKNOWN")));
            break;
    }

    return strData;
}

double LC_List::polylineRadius(const Plug_VertexData &ptA, const Plug_VertexData &ptB) {
    double dChord = sqrt(pow(ptA.point.x() - ptB.point.x(), 2) + pow(ptA.point.y() - ptB.point.y(), 2));

    return fabs(0.5 * dChord / sin(2.0 * atan(ptA.bulge)));
}

/*****************************/
lc_ListDlg::lc_ListDlg(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("List entities"));
//    QTextEdit *edit= new QTextEdit(this);
    edit.setReadOnly(true);
    edit.setAcceptRichText(false);
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
    auto *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(&edit);
    mainLayout->addWidget(bb);
    this->setLayout(mainLayout);
    this->resize(450, 350);

    connect(bb, &QDialogButtonBox::rejected, this, &lc_ListDlg::accept);
}

void lc_ListDlg::setText(const QString& text) {
    edit.setText(text);
}

lc_ListDlg::~lc_ListDlg() = default;
