/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include "drw_base.h"
#include "intern/drw_dbg.h"

const std::unordered_map< std::string, DRW::Version >& DRW::getDwgVersionStrings() {

    static std::unordered_map< std::string, DRW::Version > dwgVersionStrings {
            { "MC0.0", DRW::MC00 },
            { "AC1.2", DRW::AC12 },
            { "AC1.4", DRW::AC14 },
            { "AC1.50", DRW::AC150 },
            { "AC2.10", DRW::AC210 },
            { "AC1002", DRW::AC1002 },
            { "AC1003", DRW::AC1003 },
            { "AC1004", DRW::AC1004 },
            { "AC1006", DRW::AC1006 },
            { "AC1009", DRW::AC1009 },
            { "AC1012", DRW::AC1012 },
            { "AC1014", DRW::AC1014 },
            { "AC1015", DRW::AC1015 },
            { "AC1018", DRW::AC1018 },
            { "AC1021", DRW::AC1021 },
            { "AC1024", DRW::AC1024 },
            { "AC1027", DRW::AC1027 },
            { "AC1032", DRW::AC1032 },
    };
    return dwgVersionStrings;
}

std::string DRW::toUpper(const std::string &str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<unsigned char>(std::toupper(c)); });
    return result;
}