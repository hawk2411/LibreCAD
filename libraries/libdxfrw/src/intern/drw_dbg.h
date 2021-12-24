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

#ifndef DRW_DBG_H
#define DRW_DBG_H

#include <string>
#include <iostream>
#include <memory>
#include "../drw_dbg_base.h"
#include "../drw_base.h"

class DRW_dbg {
public:
    void setLevel( DRW::DebugLevel lvl);
    /**
     * Sets a custom debug printer to use when non-silent output
     * is required.
     */
    void setCustomDebugPrinter(std::unique_ptr<DRW::IDebugPrinter> printer);
    DRW::DebugLevel getLevel()const;
    static DRW_dbg *getInstance();
    void print(const std::string &s) const;
    void print(signed char i) const;
    void print(unsigned char i) const;
    void print(int i) const;
    void print(unsigned int i) const;
    void print(long long int i) const;
    void print(long unsigned int i) const;
    void print(long long unsigned int i) const;
    void print(double d) const;
    void printH(long long int i) const;
    void printB(int i) const;
    void printHL(int c, int s, int h) const;
    void printPT(double x, double y, double z) const;

private:
    DRW_dbg();
    static DRW_dbg *instance;
    DRW::DebugLevel level{DRW::DebugLevel::None};
    DRW::SilentPrinter silentDebug;     //default implementation of DRW::IDebugPrinter that do nothing (silent)
    std::unique_ptr<DRW::IDebugPrinter> defaultPrinter;
    DRW::IDebugPrinter* currentPrinter{nullptr};
};
/**
 * Sets a custom debug printer to use when outputting debug messages.
 *
 * Ownership of `printer` is transferred.
 */
void setCustomDebugPrinter(DRW::IDebugPrinter* printer );

auto drw_dbgsl = [](DRW::DebugLevel level) {
    DRW_dbg::getInstance()->setLevel(level);
};

auto drw_dbggl = [] () -> DRW::DebugLevel { return DRW_dbg::getInstance()->getLevel(); };
auto drw_dbg = [](const auto & value) {DRW_dbg::getInstance()->print(value);};
auto drw_dbgh = [](const auto & value) { DRW_dbg::getInstance()->printH(value); };
auto drw_dbgb = [](const auto& value) { DRW_dbg::getInstance()->printB(value); };
auto drw_dbghl = [](const auto& a, const auto& b, const auto& c) { DRW_dbg::getInstance()->printHL(a, b, c); };
auto drw_dbgpt = [](const auto& a, const auto &b, const auto& c) { DRW_dbg::getInstance()->printPT(a, b, c); };
#endif // DRW_DBG_H
