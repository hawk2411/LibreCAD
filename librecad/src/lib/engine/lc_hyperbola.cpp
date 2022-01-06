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

#include "lc_hyperbola.h"

#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_painter.h"
#include "rs_information.h"
#include "rs_linetypepattern.h"
#include "lc_quadratic.h"

std::ostream &operator<<(std::ostream &os, const LC_HyperbolaData &ed) {
    os << "(" << ed.center <<
       "/" << ed.majorP <<
       " " << ed.ratio <<
       " " << ed.angle1 <<
       "," << ed.angle2 <<
       ")";
    return os;
}

#ifdef EMU_C99
#include "emu_c99.h" /* C99 math */
#endif

/**
 * Constructor.
 */
LC_Hyperbola::LC_Hyperbola(RS_EntityContainer *parent,
                           const LC_HyperbolaData &data)
        : RS_AtomicEntity(parent), _data(data) {
    _bValid = _data.majorP.squared() >= RS_TOLERANCE2;
}


RS_Entity *LC_Hyperbola::clone() const {
    auto *hyperbola = new LC_Hyperbola(*this);
    hyperbola->initId();
    return hyperbola;
}


/**
  * return the foci of ellipse
  *
  *@Author: Dongxu Li
  */

RS_VectorSolutions LC_Hyperbola::getFoci() const {
    RS_Vector vp(getMajorP() * sqrt(1. - getRatio() * getRatio()));
    return RS_VectorSolutions({getCenter() + vp, getCenter() - vp});
}

RS_VectorSolutions LC_Hyperbola::getRefPoints() const {
    RS_VectorSolutions ret({_data.center});
    ret.push_back(getFoci());
    return ret;
}

bool LC_Hyperbola::isPointOnEntity(const RS_Vector &coord,
                                   double tolerance) const {
    double a = _data.majorP.magnitude();
    double b = a * _data.ratio;
    if (fabs(a) < tolerance || fabs(b) < tolerance) return false;
    RS_Vector vp(coord - _data.center);
    vp = vp.rotate(-_data.majorP.angle());
    return fabs(vp.x * vp.x / (a * a) - vp.y * vp.y / (b * b) - 1.) < tolerance;
}


LC_Quadratic LC_Hyperbola::getQuadratic() const {
    std::vector<double> ce(6, 0.);
    ce[0] = _data.majorP.squared();
    ce[2] = -_data.ratio * _data.ratio * ce[0];
    if (ce[0] > RS_TOLERANCE2) ce[0] = 1. / ce[0];
    if (fabs(ce[2]) > RS_TOLERANCE2) ce[2] = 1. / ce[2];
    ce[5] = -1.;
    LC_Quadratic ret(ce);
    if (ce[0] < RS_TOLERANCE2 || fabs(ce[2]) < RS_TOLERANCE2) {
        ret.setValid(false);
        return ret;
    }
    ret.rotate(_data.majorP.angle());
    ret.move(_data.center);
    return ret;
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream &operator<<(std::ostream &os, const LC_Hyperbola &a) {
    os << " Hyperbola: " << a._data << "\n";
    return os;
}
