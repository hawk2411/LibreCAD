/****************************************************************************
*  divide.cpp - divide lines, circles and arcs                              *
*                                                                           *
*  Copyright (C) 2018 mad-hatter                                            *
*  some code borrowed from                                                  *
*  list.cpp - Copyrighted by Rallaz, rallazz@gmail.com                      *
*                                                                           *
*  This library is free software, licensed under the terms of the GNU       *
*  General Public License as published by the Free Software Foundation,     *
*  either version 2 of the License, or (at your option) any later version.  *
*  You should have received a copy of the GNU General Public License        *
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
****************************************************************************/

#include <QtGui>
#include <QApplication>
#include <QDesktopWidget>
#include <QDialog>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QGridLayout>
#include <QMessageBox>
#include <QLabel>
#include <complex>
#include <utility>

#include "divide.h"
#include "dividedlg.h"

//#include <QDebug>

QString divide::name() const {
    return (tr("Divide"));
}

PluginCapabilities divide::getCapabilities() const {
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
            << PluginMenuLocation("plugins_menu", tr("Divide"));
    return pluginCapabilities;
}

void divide::execComm(IDocumentPlugin *doc,
                      QWidget *parent,
                      QString cmd) {
    Q_UNUSED (parent);
    Q_UNUSED (cmd);

    document = doc;

    QList<Plug_Entity *> obj;
    bool yes = doc->getSelect(&obj, tr("Select a line, circle or"
                                       " arc and press return"));
    if (divide::jumpOutWithError(yes, &obj)) {
        clearObjectList(&obj);
        return;
    }

    QString passedData;
    for (int i = 0; i < obj.size(); ++i) {
        passedData.append(QString("%1 %2: ").arg(tr("n")).arg(i + 1));
        passedData.append(getStrData(obj.at(i)));
        passedData.append("\n");
    }

    dividedlg dlg(document, passedData, parent);
    QObject::connect(&dlg, &dividedlg::returnData, this, &divide::gotReturnedDataSlot);

    if (dlg.exec() != QDialog::Accepted) {
        clearObjectList(&obj);
        return;
    }

    auto data = returnedData.split(":");

    int qty = data.at(1).toInt(); //defaults to zero if
    if (qty <= 0) {
        //qty is empty
        clearObjectList(&obj);
        return;
    }

    bool ticks = (data.at(3) == "t");
    bool breaks = (data.at(4) == "t");

    if ((!ticks) && (!breaks)) {
        clearObjectList(&obj);
        return;
    }


    QString oldLayer = doc->getCurrentLayer();
    int dataSize = data.size();
    if (data.at(dataSize - 1) == "lay") {
        //layer for ticks
        doc->setLayer(data.at(dataSize - 2));
    }

    QList<QString> pData = (passedData.split
            (QRegularExpression(R"([\n\t\r])")));

    doc->updateView();
    QString entType = data.at(0).simplified().toLower();

    //***********
    while (entType.size() > 0) {
        if (entType.startsWith("ci")) {
            handleCircle(entType, pData, data, ticks, breaks);
            break;
        }

        if (entType.startsWith("li")) {
            handleLine(entType, pData, data, ticks, breaks);
            break;
        }

        if (entType.startsWith("ar")) {
            handleArc(entType, pData, data, ticks, breaks);
            break;
        }
        break;
    }

    doc->setLayer(oldLayer);
    doc->updateView(); //updates & removes highlights

    clearObjectList(&obj);
} // end execComm


void divide::segmentLine(QPointF startPoint, QPointF lineEnd, QPointF lineStart,
                         const QString &type, int qty, int count) {
    QString layerName = ("Break layer - " + type);
    QString currentLayer = document->getCurrentLayer();
    document->setLayer(layerName);

    static QPointF newStart;

    while(true) {
        if (count == 1) {
            //1st line
            document->addLine(&lineStart, &startPoint);
            newStart = startPoint;
            break;
        }
        if ((count > 1) && (count < qty)) {
            //mid line
            document->addLine(&newStart, &startPoint);
            newStart = startPoint;
            break;
        }
        if (count == qty) {
            //end line
            document->addLine(&newStart, &lineEnd);
            break;
        }
        break;
    }
    document->setLayer(currentLayer); //restore layer
}

void divide::segment(QPointF *centerCircle, double radius,
                     double angle, double arcAngle, const QString &type) {
    QString layerName = ("Break layer - " + type);
    QString cl = document->getCurrentLayer();
    document->setLayer(layerName);

    document->addArc(centerCircle, radius, angle, (angle + arcAngle));

    document->setLayer(cl);
}

double divide::findHypLength(double h1, double h2, double v1, double v2) {
    return (hypot(h1 - h2, v1 - v2)); //needs (C++11 or later) - hypotenuse
    //see - http://www.cplusplus.com/reference/cmath/hypot/  - C99
}

//line in any quadrant
//angle ± value in radians (range - double)
//startX, startY and length ± value (range - double)
//take care with values near double max and min limits ???
QPointF divide::findLineEndPoint(double startX, double startY,
                                 double length, double angle) //radians
{
    auto endPoint = (std::complex<double>(startX, startY)
                     + std::polar<double>(length, angle));

    return {endPoint.real(), endPoint.imag()};
}

void divide::drawTick(QPointF startX, double tickLength, double tickAngle) {
    QPointF result = findLineEndPoint(startX.rx(), startX.ry(),
                                      tickLength, tickAngle);
    document->addLine(&startX, &result);
}

QPointF divide::findStartX(double radius, double angle, QPointF centerCircle) {
    QPointF startPoint;

    startPoint.setX((radius * cos(angle)) + centerCircle.rx());
    startPoint.setY((radius * sin(angle)) + centerCircle.ry());

    return startPoint;
}

void divide::gotReturnedDataSlot(QString data) {
    returnedData = std::move(data);
}

QString divide::getStrData(Plug_Entity *ent) {
    if (nullptr == ent)
        return QString("%1\n").arg(tr("Empty Entity"));

    QHash<int, QVariant> data;
    QString strData(""),
            strEntity("%1\n"),
            strCommon("  %1: %2\n"),
            strSpecific("    %1: %2\n"),
            strSpecificXY(QString("    %1: %2=%3 %4=%5\n")
                                  .arg("%1", tr("X"), "%2", tr("Y"), "%3"));
    double numA{0.0};
    double numB{0.0};
    double numC{0.0};
    QPointF ptA, ptB, ptC;

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
            strData.append(strSpecific.arg(tr("Angle in XY plane"),
                                           document->realToStr(numC, 0, 0)));
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
            strData.append(strSpecific.arg(tr("initial angle"),
                                           document->realToStr(numB * 180 / M_PI, 0, 0)));
            strData.append(strSpecific.arg(tr("final angle"),
                                           document->realToStr(numC * 180 / M_PI, 0, 0)));
            if (numB > numC) {
                numB -= 2.0 * M_PI;
            }
            strData.append(strSpecific.arg(tr("length"),
                                           document->realToStr((numC - numB) * numA, 0, 0)));
            break;
        case DPI::CIRCLE:
            strData.prepend(strEntity.arg(tr("CIRCLE")));
            strData.append(strSpecificXY.arg(tr("center point"),
                                             document->realToStr(data.value(DPI::STARTX).toDouble(), 0, 0),
                                             document->realToStr(data.value(DPI::STARTY).toDouble(), 0, 0)));
            numA = data.value(DPI::RADIUS).toDouble();
            strData.append(strSpecific.arg(tr("radius"), document->realToStr(numA, 0, 0)));
            strData.append(strSpecific.arg(tr("circumference"),
                                           document->realToStr(numA * 2 * M_PI, 0, 0)));
            strData.append(strSpecific.arg(tr("area"),
                                           document->realToStr(numA * numA * M_PI, 0, 0)));
            break;
        case DPI::POLYLINE: {
            strData.prepend(strEntity.arg(tr("POLYLINE")));
            strData.append(strSpecific.arg(tr("Closed"),
                                           (0 == data.value(DPI::CLOSEPOLY).toInt())
                                           ? tr("No") : tr("Yes")));
            strData.append(strSpecific.arg(tr("Vertices"), ""));
            QList<Plug_VertexData> vl;
            ent->getPolylineData(&vl);
            int iVertices = vl.size();
            for (int i = 0; i < iVertices; ++i) {
                strData.append(strSpecificXY.arg(tr("in point"),
                                                 document->realToStr(vl.at(i).point.x(), 0, 0),
                                                 document->realToStr(vl.at(i).point.y(), 0, 0)));
                //***
                if (0.0 != vl.at(i).bulge) { //was 0
                    //***
                    strData.append(strSpecific.arg(tr("radius"),
                                                   document->realToStr(
                                                           polylineRadius(vl.at(i), vl.at((i + 1) % iVertices)), 0,
                                                           0)));
                }
            }
            break;
        }
        default:
            strData.prepend(strEntity.arg
                    (tr("MUST be a line, circle or arc")));
            break;
    }
    return strData;
}

//return center of active window
QPoint divide::findWindowCentre() {
    QPoint centXY;

    centXY.setX((QApplication::activeWindow()->width() / 2)
                + QApplication::activeWindow()->x());
    centXY.setY((QApplication::activeWindow()->height() / 2)
                + QApplication::activeWindow()->y());

    return (centXY);
}

double divide::polylineRadius(const Plug_VertexData &ptA,
                              const Plug_VertexData &ptB) {
    double dChord = sqrt(pow(ptA.point.x() - ptB.point.x(), 2) +
                         pow(ptA.point.y() - ptB.point.y(), 2));

    return fabs(0.5 * dChord / sin(2.0 * atan(ptA.bulge)));
}

bool divide::jumpOutWithError(bool user_button, QList<Plug_Entity *> *obj_list) {

    if (!user_button || obj_list->isEmpty() || (obj_list->size() > 1)) {

        //none or multiple
        //entity selection
        QMessageBox msgBox;
        msgBox.setStyleSheet("QLabel, QPushButton { color: blue; }"
                             "QMessageBox { border: none;"
                             //"background: rgb( 255, 0, 0 );" //white is default
                             //"font-family: Arial;" //use default font
                             //text colour is blue from QLabel
                             "font-style: italic; font-size: 11pt; }");
        msgBox.setWindowTitle(tr("Error"));

        QString text = ("Select a line, circle or arc.<br>");
        if (obj_list->size() > 1)
            text.append("One entity only!<br>");
        text.append("Press \"<b>Enter/Return</b>\".");
        msgBox.setText(text);
        msgBox.setIcon(QMessageBox::Warning);

/*  Why are we messing with screen geometry just to show an error message?
 *    Assume Qt will just take care of it!
 *
 *        msgBox.show(); //need show to get msgBox size
 *        QPoint centerXY = findWindowCentre();
 *        int x = centerXY.rx() - ( msgBox.width() / 2 );
 *        int y = centerXY.ry() - ( msgBox.height() / 2 );
 *
 *        QRect screenGeometry = QApplication::desktop()->availableGeometry();
 *        //in case msgBox is wholly or partially offscreen
 *        if ( x >= ( screenGeometry.width() - msgBox.width() ) )
 *            x = QApplication::desktop()->width() - msgBox.width() - 10;
 *        if ( y >= ( screenGeometry.height() - msgBox.height() ) )
 *            y = QApplication::desktop()->height() - msgBox.height() - 60;
 *
 *        msgBox.move( x, y );
 *********************************/
        msgBox.exec();

        return true;
    }

    return false;
}

void divide::clearObjectList(QList<Plug_Entity *> *obj_list) {
    while (!obj_list->isEmpty()) { delete obj_list->takeFirst(); }
}

void
divide::handleCircle(const QString &entityType, const QList<QString> &pData, const QList<QString> &data, bool ticks,
                     bool breaks) {
    auto dataSize = data.size();
    auto qty = data.at(1).toInt(); //defaults to zero if
    double radius{0.0};
    QPointF centerCircle{0.0, 0.0};

    for (const auto & i : pData) {
        QString test(i.simplified().toLower());
        if (test.startsWith("ra")) {
            //radius
            radius = (test.split(":").at(1)).toDouble();
            continue;
        }
        if (test.startsWith("ce")) {
            //center point
            QList<QString> xy = ((test.split(":")).at(1).split(" "));
            centerCircle = {(((xy.at(1)
                    .split("=")).at(1)).toDouble()),
                            (((xy.at(2)
                                    .split("=")).at(1)).toDouble())};
        }
    }

    //if data.at(5) is empty, startAngle defaults to zero
    double startAngle = (data.at(5).toDouble() * M_PI) / 180.0; //radians
    double tickLength = (2.0 * radius * (data.at(2).toDouble() / 100.0)); //%

    if (data.at(dataSize - 3) == "i") {
        //ticks inside/outside circle
        tickLength *= -1;
    }

    for (int i = 1; i <= qty; i++) {
        double tickAngle = (((2 * M_PI) / qty) * i) + startAngle;

        if (ticks) {
            drawTick((findStartX(radius, tickAngle, centerCircle)),
                     tickLength, tickAngle);
        }

        if (breaks) {
            segment(&centerCircle, radius,
                    (startAngle * (180.0 / M_PI))
                    + ((360.0 / qty) * (i - 1)),
                    (360.0 / qty), entityType);
        }
    }

}

void divide::handleLine(const QString &entityType, const QList<QString> &pData, const QList<QString> &data, bool ticks,
                        bool breaks) {
    auto dataSize = data.size();
    auto qty = data.at(1).toInt(); //defaults to zero if
    double tickAngle{0.0};
    double tickLength{0.0};
    QPointF startX{0.0, 0.0};
    QPointF startXY{0.0, 0.0};


    QPointF endXY(0.0, 0.0);

    int size = data.at(2).toInt();

    for (const auto &i: pData) {
        QString test{i.simplified().toLower()};
        QList<QString> part{test.split(":")};

        if (test.startsWith("fr")) {
            //from point
            part = part.at(1).split(" ");
            startXY += QPointF(((part.at(1).split("=")).at(1)).toDouble(),
                               ((part.at(2).split("=")).at(1)).toDouble());
            continue;
        }
        if (test.startsWith("to")) {
            //to point
            part = part.at(1).split(" ");
            endXY += QPointF(((part.at(1).split("=")).at(1)).toDouble(),
                             ((part.at(2).split("=")).at(1)).toDouble());
            continue;
        }
        if (test.startsWith("an")) {
            //angle
            tickAngle = ((part.at(1).simplified().toDouble() + 90.0)
                         * M_PI) / 180; //rads
            continue;
        }
        if (test.startsWith("le")) {
            //length
            tickLength = (test.split(":").at(1)
                    .toDouble()) * size / 100.0; //%
            continue;
        }
        if (test.startsWith("in")) {
            //inc
            //test if line goes left to right or right to left
            part = part.at(1).split("=");
            if (part.at(1).startsWith("-")) {
                //inc, minus 'x' pos
                if (tickAngle >= M_PI) {
                    //180°
                    tickAngle -= M_PI;
                }
            }
            continue;
        }
    }

    if (data.at(dataSize - 3) == "i") {
        //ticks above/below linw
        tickLength *= -1;
    }

    qty += 1;
    for (int i = 1; i <= qty; i++) {
        startX.setX((((endXY.rx() - startXY.rx()) / qty) * i)
                    + startXY.rx());
        startX.setY((((endXY.ry() - startXY.ry()) / qty) * i)
                    + startXY.ry());

        if (ticks && (i < qty)) { drawTick(startX, tickLength, tickAngle); }

        if (breaks) { segmentLine(startX, endXY, startXY, entityType, qty, i); }
    }
}

void divide::handleArc(const QString &entityType, const QList<QString> &pData, const QList<QString> &data, bool ticks,
                    bool breaks) {

    auto dataSize = data.size();
    auto qty = data.at(1).toInt(); //defaults to zero if
    double initial{0.0};
    double final{0.0};
    double arcLength{0.0};
    double radius{0.0};
    QString copyTest{""};
    QPointF centerCircle{0.0, 0.0};

    for (const auto &i: pData) {
        QString test{i.simplified().toLower()};

        if (test.isEmpty()) continue; //not valid data for 'res = (...'

        auto res = (copyTest.split(":").at(1)).toDouble();

        if (test.startsWith("ce")) //center
        {
            auto xy = ((test.split(":")).at(1).split(" "));
            centerCircle = {(((xy.at(1)
                    .split("=")).at(1)).toDouble()),
                            (((xy.at(2)
                                    .split("=")).at(1)).toDouble())};
            continue;
        }
        if (test.startsWith("ra")) {
            //radius
            radius = res;
            continue;
        }

        if (test.startsWith("in")) {
            //initial angle
            //degrees to rads
            initial = res / 180.0 * M_PI;
            continue;
        }

        if (test.startsWith("fi")) {
            //final angle
            //degrees to rads
            final = res / 180.0 * M_PI;
            continue;
        }

        if (test.startsWith("le")) {
            //length
            arcLength = res;
            continue;
        }
    }

    double tickLength = (2 * radius * (data.at(2).toDouble() / 100.0)); //%
    if (data.at(dataSize - 3) == "i") {
        //ticks inside/outside arc
        tickLength *= -1;
    }

    qty += 1;

    double angle = (final > initial) ? final : initial;
    double diff = (final > initial) ? (initial - final) / qty
                                    : (arcLength / radius) / qty;

    for (int i = 1; i <= qty; i++) {
        double tickAngle = (diff * i) + angle;

        if (ticks && (i < qty)) {
            drawTick((findStartX(radius, tickAngle, centerCircle)),
                     tickLength, tickAngle);
        }

        if (breaks) {
            segment(&centerCircle, radius,
                    ((tickAngle * 180.0) / M_PI),
                    ((diff * 180.0) / M_PI) * -1, entityType);
        }
    }
}
