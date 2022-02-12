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

#include <iostream>
#include <QHash>
#include "rs_fontlist.h"
#include "rs_debug.h"
#include "rs_font.h"
#include "rs_system.h"

RS_FontList *RS_FontList::uniqueInstance = nullptr;

RS_FontList *RS_FontList::instance() {
    if (!uniqueInstance) {
        uniqueInstance = new RS_FontList();
    }
    return uniqueInstance;
}


/**
 * Initializes the font list by creating empty RS_Font
 * objects, one for each font that could be found.
 */
void RS_FontList::init() {
    QStringList font_list = RS_SYSTEM->getNewFontList();
    font_list.append(RS_SYSTEM->getFontList());

    for (const auto &font_name: font_list) {
        QFileInfo fileInfo(font_name);
        auto baseName = fileInfo.baseName();
        if (_fonts.find(baseName) != _fonts.end()) {
            continue;
        }
        _fonts.insert(std::pair<QString, std::unique_ptr<RS_Font>>(baseName.toLower(),
                                                                   std::make_unique<RS_Font>(baseName)));
    }
}

/**
 * @return Pointer to the font with the given name or
 * \p NULL if no such font was found. The font will be loaded into
 * memory if it's not already.
 */
RS_Font *RS_FontList::requestFont(const QString &name) {
    QString lowered_name = name.toLower();

    // QCAD 1 compatibility:
    if (lowered_name.contains('#') && lowered_name.contains('_')) {
        lowered_name = lowered_name.left(lowered_name.indexOf('_'));
    } else if (lowered_name.contains('#')) {
        lowered_name = lowered_name.left(lowered_name.indexOf('#'));
    }

    auto font = _fonts.find(lowered_name);
    if (font == _fonts.end()) {
        if (name != "standard") {
            return requestFont("standard");
        }
        return nullptr;
    }

    font->second->loadFont();
    return font->second.get();

}

QStringList RS_FontList::getFontList() const {
    QStringList result;

    for (const auto &font: _fonts) {
        result.append(font.second->getFileName());
    }
    return result;
}

/**
 * Dumps the fonts to stdout.
 */
std::ostream &operator<<(std::ostream &os, RS_FontList &l) {

    os << "Fontlist: \n";
    for (auto const &f: l._fonts) {
        os << f.second.get() << "\n";
    }

    return os;
}




