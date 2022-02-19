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
#include "rs_font.h"
#include "rs_text.h"

#include "rs_fontlist.h"
#include "rs_insert.h"
#include "rs_math.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_painter.h"

RS_TextData::RS_TextData(const RS_Vector &_insertionPoint,
                         const RS_Vector &_secondPoint,
                         double _height,
                         double _widthRel,
                         VAlign _valign,
                         HAlign _halign,
                         TextGeneration _textGeneration,
                         QString _text,
                         QString _style,
                         double _angle,
                         RS2::UpdateMode _updateMode) :
        insertionPoint(_insertionPoint), secondPoint(_secondPoint), height(_height), widthRel(_widthRel),
        valign(_valign), halign(_halign), textGeneration(_textGeneration), text(std::move(_text)), style(std::move(_style)), angle(_angle),
        updateMode(_updateMode) {
}


std::ostream &operator<<(std::ostream &os, const RS_TextData &td) {
    os << "("
       << td.insertionPoint << ','
       << td.secondPoint << ','
       << td.height << ','
       << td.widthRel << ','
       << td.valign << ','
       << td.halign << ','
       << td.textGeneration << ','
       << td.text.toLatin1().data() << ','
       << td.style.toLatin1().data() << ','
       << td.angle << ','
       << td.updateMode << ','
       << ")";
    return os;
}

/**
 * Constructor.
 */
RS_Text::RS_Text(RS_EntityContainer *parent,
                 RS_TextData d)
        : RS_EntityContainer(parent), _data(std::move(d)) {

    setText(_data.text);
}

RS_Entity *RS_Text::clone() const {
    auto *t = new RS_Text(*this);
    t->setOwner(isOwner());
    t->initId();
    t->detach();
    return t;
}

/**
 * Sets a new text. The entities representing the
 * text are updated.
 */
void RS_Text::setText(const QString &t) {
    _data.text = t;

    // handle some special flags embedded in the text:
    if (_data.text.left(4) == "\\A0;") {
        _data.text = _data.text.mid(4);
        _data.valign = RS_TextData::VABottom;
    } else if (_data.text.left(4) == "\\A1;") {
        _data.text = _data.text.mid(4);
        _data.valign = RS_TextData::VAMiddle;
    } else if (_data.text.left(4) == "\\A2;") {
        _data.text = _data.text.mid(4);
        _data.valign = RS_TextData::VATop;
    }

    if (_data.updateMode == RS2::Update) {
        update();
        //calculateBorders();
    }
}



/**
 * Gets the alignment as an int.
 *
 * @return  1: top left ... 9: bottom right
 */
//RLZ: bad function, this is MText style align
int RS_Text::getAlignment() const {
    if (_data.valign == RS_TextData::VATop) {
        if (_data.halign == RS_TextData::HALeft) {
            return 1;
        } else if (_data.halign == RS_TextData::HACenter) {
            return 2;
        } else if (_data.halign == RS_TextData::HARight) {
            return 3;
        }
    } else if (_data.valign == RS_TextData::VAMiddle) {
        if (_data.halign == RS_TextData::HALeft) {
            return 4;
        } else if (_data.halign == RS_TextData::HACenter) {
            return 5;
        } else if (_data.halign == RS_TextData::HARight) {
            return 6;
        }
    } else if (_data.valign == RS_TextData::VABaseline) {
        if (_data.halign == RS_TextData::HALeft) {
            return 7;
        } else if (_data.halign == RS_TextData::HACenter) {
            return 8;
        } else if (_data.halign == RS_TextData::HARight) {
            return 9;
        }
    } else if (_data.valign == RS_TextData::VABottom) {
        if (_data.halign == RS_TextData::HALeft) {
            return 10;
        } else if (_data.halign == RS_TextData::HACenter) {
            return 11;
        } else if (_data.halign == RS_TextData::HARight) {
            return 12;
        }
    }
    if (_data.halign == RS_TextData::HAFit) {
        return 13;
    } else if (_data.halign == RS_TextData::HAAligned) {
        return 14;
    } else if (_data.halign == RS_TextData::HAMiddle) {
        return 15;
    }

    return 1;
}



/**
 * Sets the alignment from an int.
 *
 * @param a 1: top left ... 9: bottom right
 */
//RLZ: bad function, this is MText style align
void RS_Text::setAlignment(int a) {
    switch (a % 3) {
        default:
        case 1:
            _data.halign = RS_TextData::HALeft;
            break;
        case 2:
            _data.halign = RS_TextData::HACenter;
            break;
        case 0:
            _data.halign = RS_TextData::HARight;
            break;
    }

    switch ((int) ceil(a / 3.0)) {
        default:
        case 1:
            _data.valign = RS_TextData::VATop;
            break;
        case 2:
            _data.valign = RS_TextData::VAMiddle;
            break;
        case 3:
            _data.valign = RS_TextData::VABaseline;
            break;
        case 4:
            _data.valign = RS_TextData::VABottom;
            break;
    }
    if (a > 12) {
        _data.valign = RS_TextData::VABaseline;
        if (a == 13) {
            _data.halign = RS_TextData::HAFit;
        } else if (a == 14) {
            _data.halign = RS_TextData::HAAligned;
        } else if (a == 15) {
            _data.halign = RS_TextData::HAMiddle;
        }
    }

}


/**
 * Updates the Inserts (letters) of this text. Called when the
 * text or it's data, position, alignment, .. changes.
 * This method also updates the usedTextWidth / usedTextHeight property.
 */
void RS_Text::update() {

    RS_DEBUG->print("RS_Text::update");

    clear();

    if (isUndone()) {
        return;
    }

    RS_Font *font = RS_FONTLIST->requestFont(_data.style);

    if (font == nullptr) {
        return;
    }

    RS_Vector letterPos = RS_Vector(0.0, -9.0);
    RS_Vector letterSpace = RS_Vector(font->getLetterSpacing(), 0.0);
    RS_Vector space = RS_Vector(font->getWordSpacing(), 0.0);

    // First every text line is created with
    //   alignment: top left
    //   angle: 0
    //   height: 9.0
    // Rotation, scaling and centering is done later

    // For every letter:
    for (auto aChar : _data.text) {
        // Space:
        if (aChar.unicode() == 0x20) {
            letterPos += space;
        } else {
            // One Letter:
            QString letterText = QString(aChar);
            if (font->findLetter(letterText) == nullptr) {
                RS_DEBUG->print("RS_Text::update: missing font for letter( %s ), replaced it with QChar(0xfffd)",
                                qPrintable(letterText));
                letterText = QChar(0xfffd);
            }
            RS_DEBUG->print("RS_Text::update: insert a "
                            "letter at pos: %f/%f", letterPos.x, letterPos.y);

            RS_InsertData d(letterText,
                            letterPos,
                            RS_Vector(1.0, 1.0),
                            0.0,
                            1, 1, RS_Vector(0.0, 0.0),
                            font->getLetterList(), RS2::NoUpdate);

            auto *letter = new RS_Insert(this, d);
            RS_Vector letterWidth;
            letter->setPen(RS_Pen(RS2::FlagInvalid));
            letter->setLayer(nullptr);
            letter->update();
            letter->forcedCalculateBorders();

            letterWidth = RS_Vector(letter->getMax().x - letterPos.x, 0.0);
            if (letterWidth.x < 0)
                letterWidth.x = -letterSpace.x;

            addEntity(letter);

            // next letter position:
            letterPos += letterWidth;
            letterPos += letterSpace;
        }
    }

    if (!RS_EntityContainer::_autoUpdateBorders) {
        //only update borders when needed
        forcedCalculateBorders();
    }
    RS_Vector textSize = getSize();

    RS_DEBUG->print("RS_Text::updateAddLine: width 2: %f", textSize.x);

    // Vertical Align:
    double vSize = 9.0;
    //HAAligned, HAFit, HAMiddle require VABaseline
    if (_data.halign == RS_TextData::HAAligned
        || _data.halign == RS_TextData::HAFit
        || _data.halign == RS_TextData::HAMiddle) {
        _data.valign = RS_TextData::VABaseline;
    }
    RS_Vector offset(0.0, 0.0);
    switch (_data.valign) {
        case RS_TextData::VAMiddle:
            offset.move(RS_Vector(0.0, vSize / 2.0));
            break;

        case RS_TextData::VABottom:
            offset.move(RS_Vector(0.0, vSize + 3));
            break;

        case RS_TextData::VABaseline:
            offset.move(RS_Vector(0.0, vSize));
            break;

        default:
            break;
    }

    // Horizontal Align:
    switch (_data.halign) {
        case RS_TextData::HAMiddle: {
            offset.move(RS_Vector(-textSize.x / 2.0, -(vSize + textSize.y / 2.0 + getMin().y)));
            break;
        }
        case RS_TextData::HACenter:
            RS_DEBUG->print("RS_Text::updateAddLine: move by: %f", -textSize.x / 2.0);
            offset.move(RS_Vector(-textSize.x / 2.0, 0.0));
            break;
        case RS_TextData::HARight:
            offset.move(RS_Vector(-textSize.x, 0.0));
            break;

        default:
            break;
    }

    if (_data.halign != RS_TextData::HAAligned && _data.halign != RS_TextData::HAFit) {
        _data.secondPoint = RS_Vector(offset.x, offset.y - vSize);
    }
    RS_EntityContainer::move(offset);


    // Scale:
    if (_data.halign == RS_TextData::HAAligned) {
        double dist = _data.insertionPoint.distanceTo(_data.secondPoint) / textSize.x;
        _data.height = vSize * dist;
        RS_EntityContainer::scale(RS_Vector(0.0, 0.0),
                                  RS_Vector(dist, dist));
    } else if (_data.halign == RS_TextData::HAFit) {
        double dist = _data.insertionPoint.distanceTo(_data.secondPoint) / textSize.x;
        RS_EntityContainer::scale(RS_Vector(0.0, 0.0),
                                  RS_Vector(dist, _data.height / 9.0));
    } else {
        RS_EntityContainer::scale(RS_Vector(0.0, 0.0),
                                  RS_Vector(_data.height * _data.widthRel / 9.0, _data.height / 9.0));
        _data.secondPoint.scale(RS_Vector(0.0, 0.0),
                                RS_Vector(_data.height * _data.widthRel / 9.0, _data.height / 9.0));
    }

    forcedCalculateBorders();

    // Rotate:
    if (_data.halign == RS_TextData::HAAligned || _data.halign == RS_TextData::HAFit) {
        double angle = _data.insertionPoint.angleTo(_data.secondPoint);
        _data.angle = angle;
    } else {
        _data.secondPoint.rotate(RS_Vector(0.0, 0.0), _data.angle);
        _data.secondPoint.move(_data.insertionPoint);
    }
    RS_EntityContainer::rotate(RS_Vector(0.0, 0.0), _data.angle);

    // Move to insertion point:
    RS_EntityContainer::move(_data.insertionPoint);

    forcedCalculateBorders();

    RS_DEBUG->print("RS_Text::update: OK");
}


RS_Vector RS_Text::getNearestEndpoint(const RS_Vector &coord, double *dist) const {
    if (dist) {
        *dist = _data.insertionPoint.distanceTo(coord);
    }
    return _data.insertionPoint;
}

RS_VectorSolutions RS_Text::getRefPoints() const {
    RS_VectorSolutions ret({_data.insertionPoint, _data.secondPoint});
    return ret;
}

void RS_Text::move(const RS_Vector &offset) {
    RS_EntityContainer::move(offset);
    _data.insertionPoint.move(offset);
    _data.secondPoint.move(offset);
//    update();
}


void RS_Text::rotate(const RS_Vector &center, const double &angle) {
    RS_Vector angleVector(angle);
    RS_EntityContainer::rotate(center, angleVector);
    _data.insertionPoint.rotate(center, angleVector);
    _data.secondPoint.rotate(center, angleVector);
    _data.angle = RS_Math::correctAngle(_data.angle + angle);
//    update();
}

void RS_Text::rotate(const RS_Vector &center, const RS_Vector &angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    _data.insertionPoint.rotate(center, angleVector);
    _data.secondPoint.rotate(center, angleVector);
    _data.angle = RS_Math::correctAngle(_data.angle + angleVector.angle());
//    update();
}


void RS_Text::scale(const RS_Vector &center, const RS_Vector &factor) {
    _data.insertionPoint.scale(center, factor);
    _data.secondPoint.scale(center, factor);
    _data.height *= factor.x;
    update();
}


void RS_Text::mirror(const RS_Vector &axisPoint1, const RS_Vector &axisPoint2) {
    bool readable = RS_Math::isAngleReadable(_data.angle);

    RS_Vector vec = RS_Vector::polar(1.0, _data.angle);
    vec.mirror(RS_Vector(0.0, 0.0), axisPoint2 - axisPoint1);
    _data.angle = vec.angle();

    bool corr;
    _data.angle = RS_Math::makeAngleReadable(_data.angle, readable, corr);

    if (corr) {
        _data.insertionPoint.mirror(axisPoint1, axisPoint2);
        _data.secondPoint.mirror(axisPoint1, axisPoint2);
        if (_data.halign == RS_TextData::HALeft) {
            _data.halign = RS_TextData::HARight;
        } else if (_data.halign == RS_TextData::HARight) {
            _data.halign = RS_TextData::HALeft;
        } else if (_data.halign == RS_TextData::HAFit || _data.halign == RS_TextData::HAAligned) {
            RS_Vector tmp = _data.insertionPoint;
            _data.insertionPoint = _data.secondPoint;
            _data.secondPoint = tmp;
        }
    } else {
        RS_Vector minP = RS_Vector(getMin().x, getMax().y);
        minP = minP.mirror(axisPoint1, axisPoint2);
        double mirrAngle = axisPoint1.angleTo(axisPoint2) * 2.0;
        _data.insertionPoint.move(minP - getMin());
        _data.secondPoint.move(minP - getMin());
        _data.insertionPoint.rotate(minP, mirrAngle);
        _data.secondPoint.rotate(minP, mirrAngle);
    }
    update();
}


bool RS_Text::hasEndpointsWithinWindow(const RS_Vector & /*v1*/, const RS_Vector & /*v2*/) {
    return false;
}


/**
 * Implementations must stretch the given range of the entity
 * by the given offset.
 */
void RS_Text::stretch(const RS_Vector &firstCorner, const RS_Vector &secondCorner, const RS_Vector &offset) {

    if (getMin().isInWindow(firstCorner, secondCorner) &&
        getMax().isInWindow(firstCorner, secondCorner)) {

        move(offset);
    }
}


/**
 * Dumps the point's data to stdout.
 */
std::ostream &operator<<(std::ostream &os, const RS_Text &p) {
    os << " Text: " << p.getData() << "\n";
    return os;
}


void RS_Text::draw(RS_Painter *painter, RS_GraphicView *view, double & /*patternOffset*/) {
    if (!(painter && view)) {
        return;
    }

    if (!view->isPrintPreview() && !view->isPrinting()) {
        if (view->isPanning() || view->toGuiDY(getHeight()) < 4) {
            painter->drawRect(view->toGui(getMin()), view->toGui(getMax()));
            return;
        }
    }

            foreach (auto e, _entities) {
            view->drawEntity(painter, e);
        }
}

