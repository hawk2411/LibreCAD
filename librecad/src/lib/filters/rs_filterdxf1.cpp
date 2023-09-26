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

#include<cstdlib>
#include "rs_filterdxfrw.h"
#include "rs_filterdxf1.h"

#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_font.h"
#include "rs_information.h"
#include "rs_system.h"
#include "rs_dimlinear.h"
#include "rs_dimaligned.h"
#include "rs_dimangular.h"
#include "rs_dimradial.h"
#include "rs_layer.h"
#include "rs_leader.h"
#include "rs_point.h"
#include "rs_math.h"
#include "rs_debug.h"


/**
 * Default constructor.
 */
RS_FilterDXF1::RS_FilterDXF1()
        : RS_FilterInterface(), _graphic(nullptr), _fPointer{nullptr}, _fBuf{nullptr}, _fBufP{0}, _fSize{0} {
    RS_DEBUG->print("Setting up DXF 1 filter...");
}

/**
 * Implementation of the method used for RS_Import to communicate
 * with this filter.
 *
 * @param graphic The graphic in which the entities from the file
 * will be created or the graphics from which the entities are
 * taken to be stored in a file.
 */
bool RS_FilterDXF1::fileImport(RS_Graphic &g, const QString &file, RS2::FormatType /*type*/) {
    RS_DEBUG->print("DXF1 Filter: importing file '%s'...", file.toLatin1().data());

    this->_graphic = &g;

    _fPointer = nullptr;
    _fBuf = nullptr;
    _fBufP = 0;
    _fSize = 0;
    _name = file;

    if (readFileInBuffer()) {
        separateBuf();
        return readFromBuffer();
    }

    return false;
}

bool RS_FilterDXF1::fileExport(RS_Graphic & /*g*/, const QString & /*file*/,
                               RS2::FormatType /*type*/) {
    RS_DEBUG->print(RS_Debug::D_WARNING,
                    "Exporting of QCad 1.x file not implemented");
    return false;
}

/**
 * Reads a dxf1 file from buffer.
 */
bool RS_FilterDXF1::readFromBuffer() {
    RS_DEBUG->print("\nDXF: Read from buffer");

    bool ret;                    // returned value
    QString dxfLine;                // A line in the dxf file
    QString dxfCode;                // A Code in the dxf file as string
    int code = -1;                // Dxf-code as number
    double vx1 = 0.0, vy1 = 0.0;       // Start point
    double vx2 = 0.0, vy2 = 0.0;       // End point
    double vcx = 0.0, vcy = 0.0;       // Centre
    double vcr = 0.0;                // Radius
    double va1 = 0.0, va2 = 0.0;       // Start / End Angle
    QString lastLayer;              // Last used layer name (test adding only
    RS_Layer *currentLayer = nullptr;         // Pointer to current layer
    RS_Pen pen;

    RS_DEBUG->print("\nUnit set");

    resetBufP();

    if (_fBuf) {

        RS_DEBUG->print("\nBuffer OK");
        RS_DEBUG->print("\nBuffer: ");
        RS_DEBUG->print(_fBuf);

        do {
            dxfLine = getBufLine();
            pen = RS_Pen(RS_Color(RS2::FlagByLayer), RS2::WidthByLayer, RS2::LineByLayer);

            RS_DEBUG->print("\ndxfLine: ");
            RS_DEBUG->print(dxfLine.toLatin1().data());

            // $-Setting in the header of DXF found
            // RVT_PORT changed all occurenses of if (dxfline && ....) to if (dxfline.size() ......)
            if (dxfLine.size() &&
                dxfLine[0] == '$') {


                // Units:
                //
                if (dxfLine == "$INSUNITS") {
                    dxfCode = getBufLine();
                    if (dxfCode.size()) {
                        if (dxfCode.toInt() == 70) {
                            dxfLine = getBufLine();
                            if (dxfLine.size()) {
                                _graphic->addVariable("$INSUNITS", dxfLine, 70);
                                /*
                                            switch( dxfLine.toInt() ) {
                                              case  0: graphic->setUnit( RS2::None );       break;
                                              case  1: graphic->setUnit( RS2::Inch );       break;
                                              case  2: graphic->setUnit( RS2::Foot );       break;
                                              case  3: graphic->setUnit( RS2::Mile );       break;
                                              case  4: graphic->setUnit( RS2::Millimeter ); break;
                                              case  5: graphic->setUnit( RS2::Centimeter ); break;
                                              case  6: graphic->setUnit( RS2::Meter );      break;
                                              case  7: graphic->setUnit( RS2::Kilometer );  break;
                                              case  8: graphic->setUnit( RS2::Microinch );  break;
                                              case  9: graphic->setUnit( RS2::Mil );        break;
                                              case 10: graphic->setUnit( RS2::Yard );       break;
                                              case 11: graphic->setUnit( RS2::Angstrom );   break;
                                              case 12: graphic->setUnit( RS2::Nanometer );  break;
                                              case 13: graphic->setUnit( RS2::Micron );     break;
                                              case 14: graphic->setUnit( RS2::Decimeter );  break;
                                              case 15: graphic->setUnit( RS2::Decameter );  break;
                                              case 16: graphic->setUnit( RS2::Hectometer ); break;
                                              case 17: graphic->setUnit( RS2::Gigameter );  break;
                                              case 18: graphic->setUnit( RS2::Astro );      break;
                                              case 19: graphic->setUnit( RS2::Lightyear );  break;
                                              case 20: graphic->setUnit( RS2::Parsec );     break;
                                            }

                                            graphic->setDimensionUnit( graphic->getUnit() );
                                            //graphic->setGridUnit( graphic->getUnit() );
                                */
                            }
                        }
                    }
                }

                    // Dimension Units:
                    //
                else if (dxfLine == "$DIMALT") {
                    dxfCode = getBufLine();
                    if (dxfCode.size()) {
                        if (dxfCode.toInt() == 70) {
                            dxfLine = getBufLine();
                            if (dxfLine.size()) {
                                _graphic->addVariable("$DIMALT", dxfLine, 70);
                                /*
                                            switch( dxfLine.toInt() ) {
                                              case  0: graphic->setDimensionUnit( RS2::None );       break;
                                              case  1: graphic->setDimensionUnit( RS2::Inch );       break;
                                              case  2: graphic->setDimensionUnit( RS2::Foot );       break;
                                              case  3: graphic->setDimensionUnit( RS2::Mile );       break;
                                              case  4: graphic->setDimensionUnit( RS2::Millimeter ); break;
                                              case  5: graphic->setDimensionUnit( RS2::Centimeter ); break;
                                              case  6: graphic->setDimensionUnit( RS2::Meter );      break;
                                              case  7: graphic->setDimensionUnit( RS2::Kilometer );  break;
                                              case  8: graphic->setDimensionUnit( RS2::Microinch );  break;
                                              case  9: graphic->setDimensionUnit( RS2::Mil );        break;
                                              case 10: graphic->setDimensionUnit( RS2::Yard );       break;
                                              case 11: graphic->setDimensionUnit( RS2::Angstrom );   break;
                                              case 12: graphic->setDimensionUnit( RS2::Nanometer );  break;
                                              case 13: graphic->setDimensionUnit( RS2::Micron );     break;
                                              case 14: graphic->setDimensionUnit( RS2::Decimeter );  break;
                                              case 15: graphic->setDimensionUnit( RS2::Decameter );  break;
                                              case 16: graphic->setDimensionUnit( RS2::Hectometer ); break;
                                              case 17: graphic->setDimensionUnit( RS2::Gigameter );  break;
                                              case 18: graphic->setDimensionUnit( RS2::Astro );      break;
                                              case 19: graphic->setDimensionUnit( RS2::Lightyear );  break;
                                              case 20: graphic->setDimensionUnit( RS2::Parsec );     break;
                                            }
                                */
                            }
                        }
                    }
                }

                    // Dimension Format:
                    //
                    /*else if( dxfLine=="$DIMLUNIT" ) {
                      if(dxfCode=getBufLine()) {
                        if( dxfCode.toInt()==70 ) {
                          if( dxfLine=getBufLine() ) {
                            switch( dxfLine.toInt() ) {
                              case 1: graphic->setDimensionFormat( Scientific ); break;
                              case 2:
                              case 3: graphic->setDimensionFormat( Decimal ); break;
                              case 4:
                              case 5: graphic->setDimensionFormat( Fractional ); break;
                              default: break;
                            }
                          }
                        }
                      }
                }*/

                    // Dimension Arrow Size:
                    //
                else if (dxfLine == "$DIMASZ") {
                    dxfCode = getBufLine();
                    if (dxfCode.size()) {
                        if (dxfCode.toInt() == 40) {
                            dxfLine = getBufLine();
                            if (dxfLine.size()) {
                                _graphic->addVariable("$DIMASZ", dxfLine, 40);
                                //graphic->setDimensionArrowSize( dxfLine.toDouble() );
                            }
                        }
                    }
                }

                    // Dimension Scale:
                    //
                    /*
                    else if( dxfLine=="$DIMSCALE" ) {
                      if(dxfCode=getBufLine()) {
                        if( dxfCode.toInt()==40 ) {
                          if( dxfLine=getBufLine() ) {
                            graphic->setDimensionScale( dxfLine.toDouble() );
                          }
                        }
                      }
                }
                    */

                    // Dimension Text Height:
                    //







                else if (dxfLine == "$DIMTXT") {
                    dxfLine = getBufLine();
                    if (dxfCode.size()) {
                        if (dxfCode.toInt() == 40) {
                            dxfLine = getBufLine();
                            if (dxfLine.size()) {
                                _graphic->addVariable("$DIMTXT", dxfLine, 40);
                                //graphic->setDimensionTextHeight( dxfLine.toDouble() );
                            }
                        }
                    }
                }

                    // Dimension exactness:
                    //







                else if (dxfLine == "$DIMRND") {
                    dxfLine = getBufLine();
                    if (dxfCode.size()) {
                        if (dxfCode.toInt() == 40) {
                            dxfLine = getBufLine();
                            if (dxfLine.size()) {
                                _graphic->addVariable("$DIMRND", dxfLine, 40);
                                //if( dxfLine.toDouble()>0.000001 ) {
                                //graphic->setDimensionExactness( dxfLine.toDouble() );
                            }
                            //}
                        }
                    }
                }

                    // Dimension over length:
                    //







                else if (dxfLine == "$DIMEXE") {
                    dxfLine = getBufLine();
                    if (dxfCode.size()) {
                        if (dxfCode.toInt() == 40) {
                            dxfLine = getBufLine();
                            if (dxfLine.size()) {
                                _graphic->addVariable("$DIMEXE", dxfLine, 40);
                                //graphic->setDimensionOverLength( dxfLine.toDouble() );
                            }
                        }
                    }
                }

                    // Dimension under length:
                    //







                else if (dxfLine == "$DIMEXO") {
                    dxfLine = getBufLine();
                    if (dxfCode.size()) {
                        if (dxfCode.toInt() == 40) {
                            dxfLine = getBufLine();
                            if (dxfLine.size()) {
                                _graphic->addVariable("$DIMEXO", dxfLine, 40);
                                //graphic->setDimensionUnderLength( dxfLine.toDouble() );
                            }
                        }
                    }
                }


                    // Angle dimension format:
                    //







                else if (dxfLine == "$DIMAUNIT") {
                    dxfLine = getBufLine();
                    if (dxfCode.size()) {
                        if (dxfCode.toInt() == 70) {
                            dxfLine = getBufLine();
                            if (dxfLine.size()) {
                                _graphic->addVariable("$DIMAUNIT", dxfLine, 70);
                                /*
                                            switch( dxfLine.toInt() ) {
                                              case 0: graphic->setAngleDimensionFormat( DecimalDegrees ); break;
                                              case 1: graphic->setAngleDimensionFormat( DegreesMinutesSeconds ); break;
                                              case 2: graphic->setAngleDimensionFormat( Gradians ); break;
                                              case 3: graphic->setAngleDimensionFormat( Radians ); break;
                                              case 4: graphic->setAngleDimensionFormat( Surveyor ); break;
                                              default: break;
                                            }
                                */
                            }
                        }
                    }
                }

                    // Angle dimension exactness:
                    //
                else if (dxfLine == "$DIMADEC") {
                    dxfLine = getBufLine();
                    if (dxfCode.size()) {
                        if (dxfCode.toInt() == 70) {
                            dxfLine = getBufLine();
                            if (dxfLine.size()) {
                                _graphic->addVariable("$DIMADEC", dxfLine, 70);
                                //graphic->setAngleDimensionExactness( RS_Math::pow(0.1, dxfLine.toInt()) );
                            }
                        }
                    }
                }

                    // Grid x/y:
                    //
                else if (dxfLine == "$GRIDUNIT") {
                    dxfLine = getBufLine();
                    if (dxfCode.size()) {
                        if (dxfCode.toInt() == 10) {
                            dxfLine = getBufLine();
                            if (dxfLine.size()) {
                                double x = strtod(dxfLine.toLatin1().data(), nullptr);
                                dxfLine = getBufLine();
                                if (dxfLine.size()) {
                                    double y = strtod(dxfLine.toLatin1().data(), nullptr);

                                    _graphic->addVariable("$GRIDUNIT", RS_Vector(x, y), 10);
                                }
                            }
                        }
                    }
                }
            }

                // Entity
                //
            else if (dxfLine.size() &&
                     dxfLine[0] >= 'A' && dxfLine[0] <= 'Z') {

                if (dxfLine == "EOF") {
                    // End of file reached
                    //
                }

                    // ------
                    // Layer:
                    // ------
                else if (dxfLine == "LAYER") {
                    currentLayer = nullptr;
                    do {
                        dxfCode = getBufLine();
                        if (dxfCode.size())
                            code = dxfCode.toInt();
                        if (dxfCode.size() && code != 0) {
                            dxfLine = getBufLine();
                            if (dxfLine.size()) {
                                switch (code) {
                                    case 2:  // Layer name
                                        if (dxfLine == "(null)" || dxfLine == "default") {
                                            dxfLine = "0";
                                        }
                                        _graphic->getLayerList()->add(new RS_Layer(dxfLine));
                                        _graphic->getLayerList()->activate(dxfLine);
                                        currentLayer = _graphic->getLayerList()->getActive();
                                        lastLayer = dxfLine;
                                        break;
                                    case 70:  // Visibility
                                        /*
                                        if(dxfLine.toInt()&5) {
                                          if(currentLayerNum>=0 && currentLayerNum<DEF_MAXLAYERS) {
                                            graphic->layer[currentLayerNum].DelFlag(Y_VISIBLE);
                                          }
                                    }
                                        */
                                        break;
                                    case 6:  // style
                                        //if(currentLayer)
                                        //currentLayer->setStyle( graphic->nameToStyle(dxfLine) );
                                        pen.setLineType(RS_FilterDXFRW::nameToLineType(dxfLine));
                                        break;
                                    case 39:  // Thickness
                                        //if(currentLayer) currentLayer->setWidth(dxfLine.toInt());
                                        pen.setWidth(numberToWidth(dxfLine.toInt()));
                                        break;
                                    case 62:  // Color
                                        pen.setColor(RS_FilterDXFRW::numberToColor(dxfLine.toInt()));
                                        //if(currentLayer) {
                                        //	currentLayer->setColor( graphic->numberToColor(dxfLine.toInt(), !oldColorNumbers));
                                        //}
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                    } while (dxfCode.size() && code != 0);
                    if (currentLayer) {
                        currentLayer->setPen(pen);
                    }
                    //graphic->setStyle("CONTINUOUS");
                    //graphic->setWidth(0);
                    //graphic->setColor(0, false);
                }

                    // ------
                    // Point:
                    // ------
                else if (dxfLine == "POINT") {
                    do {
                        dxfCode = getBufLine();
                        if (dxfCode.size())
                            code = dxfCode.toInt();
                        if (dxfCode.size() && code != 0) {
                            dxfLine = getBufLine();
                            if (dxfLine.size()) {
                                switch (code) {
                                    case 6:  // style
                                        pen.setLineType(RS_FilterDXFRW::nameToLineType(dxfLine));
                                        break;
                                    case 8:  // Layer
                                        //if(dxfLine!=lastLayer) {
                                        if (dxfLine == "(null)" || dxfLine == "default") {
                                            dxfLine = "0";
                                        }
                                        _graphic->getLayerList()->activate(dxfLine);
                                        //lastLayer=dxfLine;
                                        //}
                                        break;
                                    case 10:  // X1
                                        dxfLine.replace(QRegExp(","), ".");
                                        vx1 = dxfLine.toDouble();
                                        break;
                                    case 20:  // Y1
                                        dxfLine.replace(QRegExp(","), ".");
                                        vy1 = dxfLine.toDouble();
                                        break;
                                    case 39:  // Thickness
                                        pen.setWidth(numberToWidth(dxfLine.toInt()));
                                        break;
                                    case 62:  // Color
                                        pen.setColor(RS_FilterDXFRW::numberToColor(dxfLine.toInt()));
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                    } while (dxfCode.size() && code != 0);
                    _graphic->setActivePen(pen);
                    _graphic->addEntity(new RS_Point(_graphic,
                                                     RS_PointData(RS_Vector(vx1, vy1))));
                }

                    // -----
                    // Line:
                    // -----
                else if (dxfLine == "LINE") {
                    do {
                        dxfCode = getBufLine();

                        if (dxfCode.size())
                            code = dxfCode.toInt();
                        if (dxfCode.size() && code != 0) {

                            dxfLine = getBufLine();

                            if (dxfLine.size()) {
                                switch (code) {
                                    case 6:  // style
                                        pen.setLineType(RS_FilterDXFRW::nameToLineType(dxfLine));
                                        break;
                                    case 8:  // Layer
                                        //if(dxfLine!=lastLayer) {
                                        if (dxfLine == "(null)" || dxfLine == "default") {
                                            dxfLine = "0";
                                        }
                                        _graphic->getLayerList()->activate(dxfLine);
                                        //lastLayer=dxfLine;
                                        //}
                                        break;
                                    case 10:  // X1
                                        dxfLine.replace(QRegExp(","), ".");
                                        vx1 = dxfLine.toDouble();
                                        break;
                                    case 20:  // Y1
                                        dxfLine.replace(QRegExp(","), ".");
                                        vy1 = dxfLine.toDouble();
                                        break;
                                    case 11:  // X2
                                        dxfLine.replace(QRegExp(","), ".");
                                        vx2 = dxfLine.toDouble();
                                        break;
                                    case 21:  // Y2
                                        dxfLine.replace(QRegExp(","), ".");
                                        vy2 = dxfLine.toDouble();
                                        break;
                                    case 39:  // Thickness
                                        pen.setWidth(numberToWidth(dxfLine.toInt()));
                                        break;
                                    case 62:  // Color
                                        pen.setColor(RS_FilterDXFRW::numberToColor(dxfLine.toInt()));
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                    } while (dxfCode.size() && code != 0);

                    _graphic->setActivePen(pen);
                    _graphic->addEntity(new RS_Line{_graphic,
                                                    {vx1, vy1}, {vx2, vy2}});
                    //}
                }


                    // ----
                    // Arc:
                    // ----
                else if (dxfLine == "ARC") {
                    do {
                        dxfCode = getBufLine();
                        if (dxfCode.size())
                            code = dxfCode.toInt();
                        if (dxfCode.size() && code != 0) {
                            dxfLine = getBufLine();
                            if (dxfLine.size()) {
                                switch (code) {
                                    case 6:  // style
                                        pen.setLineType(RS_FilterDXFRW::nameToLineType(dxfLine));
                                        break;
                                    case 8:  // Layer
                                        //if(dxfLine!=lastLayer) {
                                        if (dxfLine == "(null)" || dxfLine == "default") {
                                            dxfLine = "0";
                                        }
                                        _graphic->getLayerList()->activate(dxfLine);
                                        //lastLayer=dxfLine;
                                        //}
                                        break;
                                    case 10:  // Centre X
                                        dxfLine.replace(QRegExp(","), ".");
                                        vcx = dxfLine.toDouble();
                                        break;
                                    case 20:  // Centre Y
                                        dxfLine.replace(QRegExp(","), ".");
                                        vcy = dxfLine.toDouble();
                                        break;
                                    case 40:  // Radius
                                        dxfLine.replace(QRegExp(","), ".");
                                        vcr = dxfLine.toDouble();
                                        break;
                                    case 50:  // Start Angle
                                        dxfLine.replace(QRegExp(","), ".");
                                        va1 = RS_Math::correctAngle(dxfLine.toDouble() / ARAD);
                                        break;
                                    case 51:  // End Angle
                                        dxfLine.replace(QRegExp(","), ".");
                                        va2 = RS_Math::correctAngle(dxfLine.toDouble() / ARAD);
                                        break;
                                    case 39:  // Thickness
                                        pen.setWidth(numberToWidth(dxfLine.toInt()));
                                        break;
                                    case 62:  // Color
                                        pen.setColor(RS_FilterDXFRW::numberToColor(dxfLine.toInt()));
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                    } while (dxfCode.size() && code != 0);
                    _graphic->setActivePen(pen);
                    _graphic->addEntity(new RS_Arc(_graphic,
                                                   RS_ArcData(RS_Vector(vcx, vcy),
                                                              vcr, va1, va2, false)));
                }

                    // -------
                    // Circle:
                    // -------
                else if (dxfLine == "CIRCLE") {
                    do {
                        dxfCode = getBufLine();
                        if (dxfCode.size())
                            code = dxfCode.toInt();
                        if (dxfCode.size() && code != 0) {
                            dxfLine = getBufLine();
                            if (dxfLine.size()) {
                                switch (code) {
                                    case 6:  // style
                                        pen.setLineType(RS_FilterDXFRW::nameToLineType(dxfLine));
                                        break;
                                    case 8:  // Layer
                                        //if(dxfLine!=lastLayer) {
                                        if (dxfLine == "(null)" || dxfLine == "default") {
                                            dxfLine = "0";
                                        }
                                        _graphic->getLayerList()->activate(dxfLine);
                                        //lastLayer=dxfLine;
                                        //}
                                        break;
                                    case 10:  // Centre X
                                        dxfLine.replace(QRegExp(","), ".");
                                        vcx = dxfLine.toDouble();
                                        break;
                                    case 20:  // Centre Y
                                        dxfLine.replace(QRegExp(","), ".");
                                        vcy = dxfLine.toDouble();
                                        break;
                                    case 40:  // Radius
                                        dxfLine.replace(QRegExp(","), ".");
                                        vcr = dxfLine.toDouble();
                                        break;
                                    case 39:  // Thickness
                                        pen.setWidth(numberToWidth(dxfLine.toInt()));
                                        break;
                                    case 62:  // Color
                                        pen.setColor(RS_FilterDXFRW::numberToColor(dxfLine.toInt()));
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                    } while (dxfCode.size() && code != 0);
                    _graphic->setActivePen(pen);
                    _graphic->addEntity(new RS_Circle(_graphic,
                                                      {{vcx, vcy}, vcr}));
                }



                    // -----
                    // Text:
                    // -----
                else if (dxfLine == "TEXT") {

                    QString vtext;          // the text
                    char vtextStyle[256];  // text style (normal_ro, cursive_ri, normal_st, ...)
                    double vheight = 10.0;     // text height
                    double vtextAng = 0.0;     // text angle
                    QString vfont;         // font "normal", "cursive", ...
                    RS_MTextData::HAlign vhalign = RS_MTextData::HALeft;

                    vtextStyle[0] = '\0';
                    vfont = "normal";

                    do {
                        dxfCode = getBufLine();
                        if (dxfCode.size())
                            code = dxfCode.toInt();
                        if (dxfCode.size() && code != 0) {
                            if (code != 1 && code != 3 && code != 7)
                                dxfLine = getBufLine();
                            if (dxfLine.size() || code == 1 || code == 3 || code == 7) {

                                switch (code) {

                                    case 1:  // Text itself
                                        vtext = getBufLine();
                                        strDecodeDxfString(vtext);
                                        break;

                                    case 3:  // Text parts (always 250 chars)
                                        vtext = getBufLine();
                                        break;

                                    case 6:  // style
                                        pen.setLineType(RS_FilterDXFRW::nameToLineType(dxfLine));
                                        break;

                                    case 7:
                                        // Text style (normal_ro#50.0,
                                        //    cursive_ri#20.0, normal_st)
                                        qstrncpy(vtextStyle, getBufLine().toLatin1().data(), 249);

                                        // get font typ:
                                        //
                                        {
                                            char dummy[256];
                                            sscanf(vtextStyle, "%[^_#\n]", dummy);
                                            vfont = dummy;
                                        }

                                        // get radius, letterspace, wordspace:
                                        //
                                        {
                                            char *ptr;  // pointer to value
                                            ptr = strchr(vtextStyle, '#');
                                            if (ptr) {
                                            }
                                        }
                                        break;

                                    case 8:  // Layer
                                        if (dxfLine == "(null)" || dxfLine == "default") {
                                            dxfLine = "0";
                                        }
                                        _graphic->getLayerList()->activate(dxfLine);
                                        //}
                                        break;

                                    case 10:  // X1
                                        dxfLine.replace(QRegExp(","), ".");
                                        vx1 = dxfLine.toDouble();
                                        break;
                                    case 20:  // Y1
                                        dxfLine.replace(QRegExp(","), ".");
                                        vy1 = dxfLine.toDouble();
                                        break;
                                    case 40:  // height
                                        dxfLine.replace(QRegExp(","), ".");
                                        vheight = dxfLine.toDouble();
                                        break;
                                    case 50:  // angle
                                        dxfLine.replace(QRegExp(","), ".");
                                        vtextAng = dxfLine.toDouble() / ARAD;
                                        break;
                                    case 72: {// alignment
                                        //if(!mtext) {
                                        int v = dxfLine.toInt();
                                        if (v == 1)
                                            vhalign = RS_MTextData::HACenter;
                                        else if (v == 2)
                                            vhalign = RS_MTextData::HARight;
                                        else
                                            vhalign = RS_MTextData::HALeft;
                                        //}
                                    }
                                        break;
                                    case 39:  // Thickness
                                        pen.setWidth(numberToWidth(dxfLine.toInt()));
                                        break;
                                    case 62:  // Color
                                        pen.setColor(RS_FilterDXFRW::numberToColor(dxfLine.toInt()));
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                    } while (dxfCode.size() && code != 0);
                    char *i = strchr(vtextStyle, '#');
                    if (i) {
                        i[0] = '\0';
                    }
                    _graphic->addEntity(
                            new RS_MText(_graphic,
                                         RS_MTextData(
                                                 RS_Vector(vx1, vy1),
                                                 vheight,
                                                 100.0,
                                                 RS_MTextData::VABottom,
                                                 vhalign,
                                                 RS_MTextData::LeftToRight,
                                                 RS_MTextData::Exact,
                                                 1.0,
                                                 vtext,
                                                 vtextStyle,
                                                 vtextAng
                                         )
                            )
                    );
                }

                    // ----------
                    // Dimension:
                    // ----------
                else if (dxfLine == "DIMENSION") {
                    int typ = 1;
                    double v10 = 0.0, v20 = 0.0;
                    double v13 = 0.0, v23 = 0.0;
                    double v14 = 0.0, v24 = 0.0;
                    double v15 = 0.0, v25 = 0.0;
                    double v16 = 0.0, v26 = 0.0;
                    double v40 = 0.0, v50 = 0.0;
                    QString dimText;
                    do {
                        dxfCode = getBufLine();
                        if (dxfCode.size()) {
                            code = dxfCode.toInt();
                        }
                        if (dxfCode.size() && code != 0) {
                            dxfLine = getBufLine();
                            if (dxfLine.size()) {
                                switch (code) {
                                    case 1:  // Text (if any)
                                        dimText = dxfLine;

                                        // Mend unproper savings of older versions:
                                        if (dimText == " " || dimText == ";;")
                                            dimText = "";

                                            //else dimText.replace(QRegExp("%%c"), "¯");
                                        else
                                            strDecodeDxfString(dimText);
                                        break;
                                    case 6:  // style
                                        pen.setLineType(RS_FilterDXFRW::nameToLineType(dxfLine));
                                        break;
                                    case 8:  // Layer
                                        //if(dxfLine!=lastLayer) {
                                        if (dxfLine == "(null)" || dxfLine == "default") {
                                            dxfLine = "0";
                                        }
                                        _graphic->getLayerList()->activate(dxfLine);
                                        //lastLayer=dxfLine;
                                        //}
                                        break;
                                    case 10:  // line position x
                                        dxfLine.replace(QRegExp(","), ".");
                                        v10 = dxfLine.toDouble();
                                        break;
                                    case 20:  // line position y
                                        dxfLine.replace(QRegExp(","), ".");
                                        v20 = dxfLine.toDouble();
                                        break;
                                    case 13:  // X1
                                        dxfLine.replace(QRegExp(","), ".");
                                        v13 = dxfLine.toDouble();
                                        break;
                                    case 23:  // Y1
                                        dxfLine.replace(QRegExp(","), ".");
                                        v23 = dxfLine.toDouble();
                                        break;
                                    case 14:  // X2
                                        dxfLine.replace(QRegExp(","), ".");
                                        v14 = dxfLine.toDouble();
                                        break;
                                    case 24:  // Y2
                                        dxfLine.replace(QRegExp(","), ".");
                                        v24 = dxfLine.toDouble();
                                        break;
                                    case 15:  // X3
                                        dxfLine.replace(QRegExp(","), ".");
                                        v15 = dxfLine.toDouble();
                                        break;
                                    case 25:  // Y3
                                        dxfLine.replace(QRegExp(","), ".");
                                        v25 = dxfLine.toDouble();
                                        break;
                                    case 16:  // X4
                                        dxfLine.replace(QRegExp(","), ".");
                                        v16 = dxfLine.toDouble();
                                        break;
                                    case 26:  // Y4
                                        dxfLine.replace(QRegExp(","), ".");
                                        v26 = dxfLine.toDouble();
                                        break;
                                    case 40:
                                        dxfLine.replace(QRegExp(","), ".");
                                        v40 = dxfLine.toDouble();
                                        break;
                                    case 50:
                                        dxfLine.replace(QRegExp(","), ".");
                                        v50 = dxfLine.toDouble();
                                        break;
                                    case 70:  // Typ
                                        typ = dxfLine.toInt();
                                        break;
                                    case 39:  // Thickness
                                        pen.setWidth(numberToWidth(dxfLine.toInt()));
                                        break;
                                    case 62:  // Color
                                        pen.setColor(RS_FilterDXFRW::numberToColor(dxfLine.toInt()));
                                        break;

                                    default:
                                        break;
                                }
                            }
                        }
                    } while (dxfCode.size() && code != 0);

                    //double dist;

                    // Remove Bit values:
                    if (typ >= 128) {
                        typ -= 128;   // Location of Text
                    }
                    if (typ >= 64) {
                        typ -= 64;   // Ordinate
                    }

                    switch (typ) {
                        // Horiz. / vert.:
                        case 0: {
                            auto *d = new RS_DimLinear(
                                            _graphic,
                                            RS_DimensionData(
                                                    RS_Vector(v10, v20),
                                                    RS_Vector(0.0, 0.0),
                                                    RS_MTextData::VABottom,
                                                    RS_MTextData::HACenter,
                                                    RS_MTextData::Exact,
                                                    1.0,
                                                    dimText,
                                                    "ISO-25",
                                                    0.0
                                            ),
                                            RS_DimLinearData(
                                                    RS_Vector(v13, v23),
                                                    RS_Vector(v14, v24),
                                                    v50 / ARAD,
                                                    0.0
                                            )
                                    );
                            d->update();
                            _graphic->addEntity(d);
                        }
                            break;

                            // Aligned:
                        case 1: {
                            double angle =
                                    RS_Vector(v13, v23).angleTo(RS_Vector(v10, v20));
                            double dist =
                                    RS_Vector(v13, v23).distanceTo(RS_Vector(v10, v20));

                            RS_Vector defP = RS_Vector::polar(dist, angle);
                            defP += RS_Vector(v14, v24);

                            auto *d = new RS_DimAligned(
                                            _graphic,
                                            RS_DimensionData(
                                                    defP,
                                                    RS_Vector(0.0, 0.0),
                                                    RS_MTextData::VABottom,
                                                    RS_MTextData::HACenter,
                                                    RS_MTextData::Exact,
                                                    1.0,
                                                    dimText,
                                                    "ISO-25",
                                                    0.0
                                            ),
                                            RS_DimAlignedData(
                                                    RS_Vector(v13, v23),
                                                    RS_Vector(v14, v24)
                                            )
                                    );
                            d->update();
                            _graphic->addEntity(d);
                        }
                            break;

                            // Angle:
                        case 2: {
                            RS_Line tl1{{v13, v23},
                                        {v14, v24}};
                            RS_Line tl2{{v10, v20},
                                        {v15, v25}};

                            RS_VectorSolutions const &s = RS_Information::getIntersection(
                                    &tl1, &tl2, false);

                            if (s.get(0)._valid) {
                                vcx = s.get(0).x;
                                vcy = s.get(0).y;
                                auto *d = new RS_DimAngular(
                                                _graphic,
                                                RS_DimensionData(
                                                        RS_Vector(v10, v20),
                                                        RS_Vector(0.0, 0.0),
                                                        RS_MTextData::VABottom,
                                                        RS_MTextData::HACenter,
                                                        RS_MTextData::Exact,
                                                        1.0,
                                                        dimText,
                                                        "ISO-25",
                                                        0.0
                                                ),
                                                RS_DimAngularData(
                                                        RS_Vector(v13, v23),
                                                        RS_Vector(vcx, vcy),
                                                        RS_Vector(vcx, vcy),
                                                        RS_Vector(v16, v26)
                                                )
                                        );
                                d->update();
                                _graphic->addEntity(d);
                            }
                        }
                            break;

                            // Radius:
                        case 4: {
                            double ang =
                                    RS_Vector(v10, v20)
                                            .angleTo(RS_Vector(v15, v25));
                            RS_Vector v2 = RS_Vector::polar(v40, ang);
                            auto *d = new RS_DimRadial(
                                            _graphic,
                                            RS_DimensionData(
                                                    RS_Vector(v10, v20),
                                                    RS_Vector(0.0, 0.0),
                                                    RS_MTextData::VABottom,
                                                    RS_MTextData::HACenter,
                                                    RS_MTextData::Exact,
                                                    1.0,
                                                    dimText,
                                                    "ISO-25",
                                                    0.0
                                            ),
                                            RS_DimRadialData(
                                                    RS_Vector(v10, v20) + v2,
                                                    0.0
                                            )
                                    );
                            d->update();
                            _graphic->addEntity(d);
                        }
                            break;

                            // Arrow:
                        case 7: {
                            RS_LeaderData data(true);
                            auto *d = new RS_Leader(_graphic, data);
                            d->addVertex(RS_Vector(v14, v24));
                            d->addVertex(RS_Vector(v10, v20));
                            d->update();
                            _graphic->addEntity(d);
                        }
                            break;
                        default:
                            break;
                    }
                }
            }
        } while (dxfLine.size() && dxfLine != "EOF");

        ret = true;
    } else {
        ret = false;
    }

    return ret;
}


/**
 * Resets  the whole object
 *   (base class too)
 */
void RS_FilterDXF1::reset() {
    _file.reset();

    delBuffer();
    _fBufP = 0;
    _fSize = 0;
    if (_fPointer) {
        fclose(_fPointer);
        _fPointer = nullptr;
    }
}


/**
 * Reset buffer pointer to the beginning of the buffer:
 */
void RS_FilterDXF1::resetBufP() {
    _fBufP = 0;
}


/**
 * delete buffer:
 */
void RS_FilterDXF1::delBuffer() {
    if (_fBuf) {
        delete[] _fBuf;
        _fBuf = nullptr;
    }
}


/**
 * Remove any 13-characters in the buffer:
 */
void RS_FilterDXF1::dos2unix() {
    char *src = _fBuf, *dst = _fBuf;

    if (!_fBuf)
        return;

    while (*src != '\0') {
        if (*src != '\r') {
            *dst++ = *src;
        }
        src++;
    }

    *dst = '\0';
}


// Get next line in the buffer:
//   and overread ALL separators
//
// return:  -Null-string: end of buffer
//          -String which is the next line in buffer
//
QString RS_FilterDXF1::getBufLine() {
    char *ret;


    if (_fBufP >= (int) _fSize)
        return {};

    ret = &_fBuf[_fBufP];

    // Move fBufP pointer to the next line
    while (_fBufP < (int) _fSize && _fBuf[_fBufP++] != '\0');

    auto str = QString::fromLocal8Bit(ret).simplified();

    return (str.isNull()) ? "" : str;
}


// Separate buffer (change chars sc1 and sc2 in '\0'
//
void RS_FilterDXF1::separateBuf(char _c1,
                                char _c2,
                                char _c3,
                                char _c4) {
    int bc;

    for (bc = 0; bc < (int) _fSize; ++bc) {
        if (_fBuf[bc] == _c1 || _fBuf[bc] == _c2 ||
            _fBuf[bc] == _c3 || _fBuf[bc] == _c4) {
            _fBuf[bc] = '\0';
        }
    }
}


// Read file in buffer (buf)
//
// 'bNum' : Max number of Bytes
//        : 0: All
// return: true: successful
//         false: file not found
//
bool RS_FilterDXF1::readFileInBuffer(std::size_t _bNum) {
    _fPointer = fopen(_name.toLatin1().data(), "rb");//RLZ verify with locales
    if (_fPointer) {
        if (_file.open(_fPointer, QIODevice::ReadOnly)) {
            _fSize = _file.size();
            if (_bNum == 0)
                _bNum = _fSize;

            _fBuf = new char[_bNum + 16];

            _file.read(_fBuf, static_cast<qint64>(_bNum));
            _fBuf[_bNum] = '\0';
            _file.close();
        }
        fclose(_fPointer);

        // Convert 13/10 to 10
        dos2unix();
        _fPointer = nullptr;

        return true;
    }
    return false;
}


// Decode a DXF string to the C-convention (special character \P is a \n)
//
void RS_FilterDXF1::strDecodeDxfString(QString &str) {
    if (str.isEmpty())
        return;
    str.replace(QRegExp("%%c"), QChar(0xF8)); // Diameter
    str.replace(QRegExp("%%d"), QChar(0xB0)); // Degree
    str.replace(QRegExp("%%p"), QChar(0xB1)); // Plus/minus
    str.replace(QRegExp("\\\\[pP]"), QChar('\n'));
}


/**
 * Converts a line width number (e.g. 1) into a RS2::LineWidth.
 */
RS2::LineWidth RS_FilterDXF1::numberToWidth(int num) {
    switch (num) {
        case -1:
            return RS2::WidthByLayer;
            break;
        case -2:
            return RS2::WidthByBlock;
            break;
        case -3:
            return RS2::WidthDefault;
            break;
        default:
            if (num < 3) {
                return RS2::Width00;
            } else if (num < 7) {
                return RS2::Width01;
            } else if (num < 11) {
                return RS2::Width02;
            } else if (num < 14) {
                return RS2::Width03;
            } else if (num < 16) {
                return RS2::Width04;
            } else if (num < 19) {
                return RS2::Width05;
            } else if (num < 22) {
                return RS2::Width06;
            } else if (num < 27) {
                return RS2::Width07;
            } else if (num < 32) {
                return RS2::Width08;
            } else if (num < 37) {
                return RS2::Width09;
            } else if (num < 45) {
                return RS2::Width10;
            } else if (num < 52) {
                return RS2::Width11;
            } else if (num < 57) {
                return RS2::Width12;
            } else if (num < 65) {
                return RS2::Width13;
            } else if (num < 75) {
                return RS2::Width14;
            } else if (num < 85) {
                return RS2::Width15;
            } else if (num < 95) {
                return RS2::Width16;
            } else if (num < 103) {
                return RS2::Width17;
            } else if (num < 112) {
                return RS2::Width18;
            } else if (num < 130) {
                return RS2::Width19;
            } else if (num < 149) {
                return RS2::Width20;
            } else if (num < 180) {
                return RS2::Width21;
            } else if (num < 205) {
                return RS2::Width22;
            } else {
                return RS2::Width23;
            }
            break;
    }
    return (RS2::LineWidth) num;
}


// EOF











