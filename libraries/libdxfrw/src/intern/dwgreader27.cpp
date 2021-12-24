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

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "drw_dbg.h"
#include "dwgreader27.h"
#include "drw_textcodec.h"
#include "../libdwgr.h"


bool dwgReader27::readFileHeader() {
    drw_dbg("dwgReader27::readFileHeader\n");
    bool ret = dwgReader18::readFileHeader();
    drw_dbg("dwgReader27::readFileHeader END\n");
    return ret;
}

bool dwgReader27::readDwgHeader(DRW_Header& hdr){
    drw_dbg("dwgReader27::readDwgHeader\n");
    bool ret = dwgReader18::readDwgHeader(hdr);
    drw_dbg("dwgReader27::readDwgHeader END\n");
    return ret;
}

bool dwgReader27::readDwgClasses(){
    drw_dbg("dwgReader27::readDwgClasses");
    bool ret = dwgReader18::readDwgClasses();
    drw_dbg("\ndwgReader27::readDwgClasses END\n");
    return ret;
}

