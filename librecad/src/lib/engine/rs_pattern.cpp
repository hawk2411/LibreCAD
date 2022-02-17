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


#include "rs_pattern.h"

#include <utility>

#include "rs_system.h"
#include "rs_fileio.h"
#include "rs_layer.h"
#include "rs_debug.h"


/**
 * Constructor.
 *
 * @param fileName File name of a DXF file defining the pattern
 */
RS_Pattern::RS_Pattern(QString fileName)
        : RS_EntityContainer(nullptr), _fileName(std::move(fileName)), _loaded(false) {
    RS_DEBUG->print("RS_Pattern::RS_Pattern() ");
}


/**
 * Loads the given pattern file into this pattern.
 * Entities other than lines are ignored.
 *
 * @param filename File name of the pattern file (without path and
 * extension or full path.
 */
bool RS_Pattern::loadPattern() {
    if (_loaded) {
        return true;
    }

    RS_DEBUG->print("RS_Pattern::loadPattern");

    QString path = getPath();

    // No pattern paths found:
    if (path.isEmpty()) {
        RS_DEBUG->print("No pattern \"%s\"available.", _fileName.toLatin1().data());
        return false;
    }

    RS_Graphic graphic;
    RS_FileIO::instance()->fileImport(graphic, path);
    for (auto entity: graphic) {
        switch(entity->rtti()) {
            case RS2::EntityLine:
            case RS2::EntityArc:
            case RS2::EntityEllipse:
            {
                RS_Layer *layer = entity->getLayer();
                RS_Entity *clone = entity->clone();
                clone->reparent(this);
                if (layer) {
                    clone->setLayer(layer->getName());
                }
                addEntity(clone);
            }
                break;
            default:
                break;
        }
    }
    _loaded = true;
    RS_DEBUG->print("RS_Pattern::loadPattern: OK");

    return _loaded;
}

QString RS_Pattern::getPath() const {
    QString path;

    // Search for the appropriate pattern if we have only the name of the pattern:

    if (!_fileName.contains(".dxf", Qt::CaseInsensitive)) {
        QStringList patterns = RS_SYSTEM->getPatternList();
        for (auto & pattern : patterns) {

            if (QFileInfo(pattern).baseName().toLower() == _fileName.toLower()) {
                path = pattern;
                RS_DEBUG->print("Pattern found: %s", path.toLatin1().data());
                break;
            }
        }
    }
    else {
        // We have the full path of the pattern:
        path = _fileName;
    }
    return path;
}


