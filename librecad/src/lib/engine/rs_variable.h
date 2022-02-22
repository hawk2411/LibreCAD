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


#ifndef RS_VARIABLE_H
#define RS_VARIABLE_H

#include <QString>
#include "rs.h"
#include "rs_vector.h"

/**
 * A variable of type int, double, string or vector.
 * The variable also contains its type and an int code
 * which can identify its contents in any way.
 *
 * @author Andrew Mustun
 */
class RS_Variable {
    struct RS_VariableContents {
        QString s;
        int i = 0;
        double d = 0.0;
        RS_Vector v;
    };
public:

    RS_Variable() = default;

    RS_Variable(const RS_Vector &v, int c) :
            _code{c} {
        setVector(v);
    }

    RS_Variable(const QString &v, int c) :
            _code{c} {
        setString(v);
    }

    RS_Variable(int v, int c) :
            _code{c} {
        setInt(v);
    }

    RS_Variable(double v, int c) :
            _code{c} {
        setDouble(v);
    }

    void setString(const QString &str) {
        _contents.s = str;
        _type = RS2::VariableString;
    }

    void setInt(int i) {
        _contents.i = i;
        _type = RS2::VariableInt;
    }

    void setDouble(double d) {
        _contents.d = d;
        _type = RS2::VariableDouble;
    }

    void setVector(const RS_Vector &v) {
        _contents.v = v;
        _type = RS2::VariableVector;
    }

    QString getString() const {
        return _contents.s;
    }

    int getInt() const {
        return _contents.i;
    }

    double getDouble() const {
        return _contents.d;
    }

    RS_Vector getVector() const {
        return _contents.v;
    }

    RS2::VariableType getType() const {
        return _type;
    }

    int getCode() const {
        return _code;
    }

    //friend std::ostream& operator << (std::ostream& os, RS_Variable& v);

private:
    RS_VariableContents _contents;
    RS2::VariableType _type = RS2::VariableVoid;
    int _code = 0;
};

#endif
