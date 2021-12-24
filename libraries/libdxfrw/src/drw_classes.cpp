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

#include "drw_classes.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
#include "intern/dwgbuffer.h"
#include "intern/drw_dbg.h"

void DRW_Class::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 1:
        recName = reader->getUtf8String();
        break;
    case 2:
        className = reader->getUtf8String();
        break;
    case 3:
        appName = reader->getUtf8String();
        break;
    case 90:
        proxyFlag = reader->getInt32();
        break;
    case 91:
        instanceCount = reader->getInt32();
        break;
    case 280:
        wasaProxyFlag = reader->getInt32();
        break;
    case 281:
        entityFlag = reader->getInt32();
        break;
    default:
        break;
    }
}

bool DRW_Class::parseDwg(DRW::Version version, dwgBuffer *buf, dwgBuffer *strBuf){
    drw_dbg("\n***************************** parsing Class *********************************************\n");

    classNum = buf->getBitShort();
    drw_dbg("Class number: "); drw_dbg(classNum);
    proxyFlag = buf->getBitShort(); //in dwg specs says "version"

    appName = strBuf->getVariableText(version, false);
    className = strBuf->getVariableText(version, false);
    recName = strBuf->getVariableText(version, false);

    drw_dbg("\napp name: "); drw_dbg(appName.c_str());
    drw_dbg("\nclass name: "); drw_dbg(className.c_str());
    drw_dbg("\ndxf rec name: "); drw_dbg(recName.c_str());
    wasaProxyFlag = buf->getBit(); //in dwg says wasazombie
    entityFlag = buf->getBitShort();
    entityFlag = entityFlag == 0x1F2 ? 1: 0;

    drw_dbg("\nProxy capabilities flag: "); drw_dbg(proxyFlag);
    drw_dbg(", proxy flag (280): "); drw_dbg(wasaProxyFlag);
    drw_dbg(", entity flag: "); drw_dbgh(entityFlag);

    if (version > DRW::AC1015) {//2004+
        instanceCount = buf->getBitLong();
        drw_dbg("\nInstance Count: "); drw_dbg(instanceCount);
        duint32 dwgVersion = buf->getBitLong();
        drw_dbg("\nDWG version: "); drw_dbg(dwgVersion);
        drw_dbg("\nmaintenance version: "); drw_dbg(buf->getBitLong());
        drw_dbg("\nunknown 1: "); drw_dbg(buf->getBitLong());
        drw_dbg("\nunknown 2: "); drw_dbg(buf->getBitLong());
    }
    drw_dbg("\n");
    toDwgType();
    return buf->isGood();
}

void DRW_Class::write(dxfWriter *writer, DRW::Version ver){
    if (ver > DRW::AC1009) {
        writer->writeString(0, "CLASS");
        writer->writeString(1, recName);
        writer->writeString(2, className);
        writer->writeString(3, appName);
        writer->writeInt32(90, proxyFlag);
        if (ver > DRW::AC1015) { //2004+
            writer->writeInt32(91, instanceCount);
        }
        writer->writeInt16(280, wasaProxyFlag);
        writer->writeInt16(281, entityFlag);
    }
}

void DRW_Class::toDwgType(){
    if (recName == "LWPOLYLINE")
        dwgType = 77;
    else if (recName == "HATCH")
        dwgType = 78;
    else if (recName == "GROUP")
        dwgType = 72;
/*    else if (recName == "GROUP")
        dwgType = 72;*/
    else if (recName == "LAYOUT")
        dwgType = 82;
    else if (recName == "IMAGE")
        dwgType = 101;
    else if (recName == "IMAGEDEF")
        dwgType = 102;
    else
        dwgType =0;
}
