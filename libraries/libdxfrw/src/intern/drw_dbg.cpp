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

#include <iostream>
#include <iomanip>
#include <memory>
#include "drw_dbg.h"

DRW_dbg *DRW_dbg::instance{nullptr};

void setCustomDebugPrinter(DRW::IDebugPrinter *printer)
{
    DRW_dbg::getInstance()->setCustomDebugPrinter(std::unique_ptr<DRW::IDebugPrinter>(printer));
}

/********* debug class *************/
DRW_dbg *DRW_dbg::getInstance() {
    if (!instance) {
        instance = new DRW_dbg;
    }
    return instance;
}


DRW_dbg::DRW_dbg() {
    defaultPrinter = std::make_unique<DRW::CErrPrinter>();
    currentPrinter = &silentDebug;
}

void DRW_dbg::setCustomDebugPrinter(std::unique_ptr<DRW::IDebugPrinter> printer) {
    if (printer == nullptr) {
        defaultPrinter = std::make_unique<DRW::CErrPrinter>();
    } else {
        defaultPrinter = std::move(printer);
    }

    if (level == DRW::DebugLevel::Debug) {
        currentPrinter = defaultPrinter.get();
    }
}

void DRW_dbg::setLevel(DRW::DebugLevel lvl) {
    level = lvl;
    switch (level) {
        case DRW::DebugLevel::Debug:
            currentPrinter = defaultPrinter.get();
            break;
        case DRW::DebugLevel::None:
            currentPrinter = &silentDebug;
            break;
    }
}

DRW::DebugLevel DRW_dbg::getLevel() const {
    return level;
}

void DRW_dbg::print(const std::string &s) const {
    currentPrinter->printString(s);
}

void DRW_dbg::print(signed char i) const {
    currentPrinter->printInt(i);
}

void DRW_dbg::print(unsigned char i) const {
    currentPrinter->printUnsignedInt(i);
}

void DRW_dbg::print(int i) const {
    currentPrinter->printInt(i);
}

void DRW_dbg::print(unsigned int i) const {
    currentPrinter->printUnsignedInt(i);
}

void DRW_dbg::print(long long int i) const {
    currentPrinter->printInt(i);
}

void DRW_dbg::print(long unsigned int i) const {
    currentPrinter->printUnsignedInt(i);
}

void DRW_dbg::print(long long unsigned int i) const {
    currentPrinter->printUnsignedInt(i);
}

void DRW_dbg::print(double d) const {
    currentPrinter->printDouble(d);
}

void DRW_dbg::printH(long long int i) const {
    currentPrinter->printHex(i);
}

void DRW_dbg::printB(int i) const {
    currentPrinter->printBinary(i);
}

void DRW_dbg::printHL(int c, int s, int h) const {
    currentPrinter->printHL(c, s, h);
}

void DRW_dbg::printPT(double x, double y, double z) const {
    currentPrinter->printPoint(x, y, z);
}

//template<typename T>
//void DRW_dbg::print(T value) const {
//    if( std::is_arithmetic<T>(value) ) {
//        if(std::is_signed<T>(value))  {
//            currentPrinter->printInt(value);
//        } else {
//            currentPrinter->printUnsignedInt(value);
//        }
//    }
//}
