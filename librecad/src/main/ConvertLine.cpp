//
// Created by hawk on 25.12.21.
//
#include <qstring.h>
#include <rs.h>
#include "ConvertLine.h"

ConvertLine::ConvertLine() {
//    QHash<int, QString> lType;
    lType.insert(RS2::LineByLayer, "BYLAYER");
    lType.insert(RS2::LineByBlock, "BYBLOCK");
    lType.insert(RS2::SolidLine, "SolidLine");
    lType.insert(RS2::DotLine, "DotLine");
    lType.insert(RS2::DotLine2, "DotLine2");
    lType.insert(RS2::DotLineX2, "DotLineX2");
    lType.insert(RS2::DashLine, "DashLine");
    lType.insert(RS2::DashLine2, "DashLine2");
    lType.insert(RS2::DashLineX2, "DashLineX2");
    lType.insert(RS2::DashDotLine, "DashDotLine");
    lType.insert(RS2::DashDotLine2, "DashDotLine2");
    lType.insert(RS2::DashDotLineX2, "DashDotLineX2");
    lType.insert(RS2::DivideLine, "DivideLine");
    lType.insert(RS2::DivideLine2, "DivideLine2");
    lType.insert(RS2::DivideLineX2, "DivideLineX2");
    lType.insert(RS2::CenterLine, "CenterLine");
    lType.insert(RS2::CenterLine2, "CenterLine2");
    lType.insert(RS2::CenterLineX2, "CenterLineX2");
    lType.insert(RS2::BorderLine, "BorderLine");
    lType.insert(RS2::BorderLine2, "BorderLine");
    lType.insert(RS2::BorderLineX2, "BorderLine");

    lWidth.insert(RS2::Width00, "0.00mm");
    lWidth.insert(RS2::Width01, "0.05mm");
    lWidth.insert(RS2::Width02, "0.09mm");
    lWidth.insert(RS2::Width03, "0.13mm");
    lWidth.insert(RS2::Width04, "0.15mm");
    lWidth.insert(RS2::Width05, "0.18mm");
    lWidth.insert(RS2::Width06, "0.20mm");
    lWidth.insert(RS2::Width07, "0.25mm");
    lWidth.insert(RS2::Width08, "0.30mm");
    lWidth.insert(RS2::Width09, "0.35mm");
    lWidth.insert(RS2::Width10, "0.40mm");
    lWidth.insert(RS2::Width11, "0.50mm");
    lWidth.insert(RS2::Width12, "0.53mm");
    lWidth.insert(RS2::Width13, "0.60mm");
    lWidth.insert(RS2::Width14, "0.70mm");
    lWidth.insert(RS2::Width15, "0.80mm");
    lWidth.insert(RS2::Width16, "0.90mm");
    lWidth.insert(RS2::Width17, "1.00mm");
    lWidth.insert(RS2::Width18, "1.06mm");
    lWidth.insert(RS2::Width19, "1.20mm");
    lWidth.insert(RS2::Width20, "1.40mm");
    lWidth.insert(RS2::Width21, "1.58mm");
    lWidth.insert(RS2::Width22, "2.00mm");
    lWidth.insert(RS2::Width23, "2.11mm");
    lWidth.insert(RS2::WidthByLayer, "BYLAYER");
    lWidth.insert(RS2::WidthByBlock, "BYBLOCK");
    lWidth.insert(RS2::WidthDefault, "BYDEFAULT");
}

QString ConvertLine::lt2str(enum RS2::LineType lt) const {
    return lType.value(lt, "BYLAYER");
}

QString ConvertLine::lw2str(enum RS2::LineWidth lw) const {
    return lWidth.value(lw, "BYDEFAULT");
}

enum RS2::LineType ConvertLine::str2lt(const QString& s) const {
    return lType.key(s, RS2::LineByLayer);
}

enum RS2::LineWidth ConvertLine::str2lw(const QString& w) const {
    return lWidth.key(w, RS2::WidthDefault);
}

QString ConvertLine::intColor2str(int col) {
    switch (col) {
        case -1:
            return "BYLAYER";
        case -2:
            return "BYBLOCK";
        default:
            return QString::number(col >> 16) + ", " + QString::number((col >> 8) & 0xFF) + ", " +
                   QString::number(col & 0xFF);
    }
}

ConvertLine getConverter() {
    static ConvertLine converter;
    return converter;
}