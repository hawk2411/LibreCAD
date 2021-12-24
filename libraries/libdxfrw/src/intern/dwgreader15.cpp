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
#include <vector>
#include "drw_dbg.h"
#include "dwgreader15.h"
#include "drw_textcodec.h"
#include "../libdwgr.h"

bool dwgReader15::readMetaData() {
    version = parent->getVersion();
    decoder.setVersion(version, false);
    drw_dbg("dwgReader15::readMetaData\n");
    if (! fileBuf->setPosition(13))
        return false;
    previewImagePos = fileBuf->getRawLong32();
    drw_dbg("previewImagePos (seekerImageData) = "); drw_dbg(previewImagePos);
    /* MEASUREMENT system variable 2 bytes*/
    uint16_t meas = fileBuf->getRawShort16();
    drw_dbg("\nMEASUREMENT (0 = English, 1 = Metric)= "); drw_dbg(meas);
    uint16_t cp = fileBuf->getRawShort16();
    drw_dbg("\ncodepage= "); drw_dbg(cp); drw_dbg("\n");
    if (cp == 29) //TODO RLZ: locate wath code page and correct this
        decoder.setCodePage("ANSI_1252", false);
    if (cp == 30)
        decoder.setCodePage("ANSI_1252", false);
    return true;
}

bool dwgReader15::readFileHeader() {
    bool ret = true;
    drw_dbg("dwgReader15::readFileHeader\n");
    if (! fileBuf->setPosition(21))
        return false;
    uint32_t count = fileBuf->getRawLong32();
    drw_dbg("count records= "); drw_dbg(count); drw_dbg("\n");

    for (unsigned int i = 0; i < count; i++) {
        uint8_t rec = fileBuf->getRawChar8();
        uint32_t address = fileBuf->getRawLong32();
        uint32_t size = fileBuf->getRawLong32();
        dwgSectionInfo si;
        si.Id = rec;
        si.size = size;
        si.address = address;
        if (rec == 0) {
            drw_dbg("\nSection HEADERS address= ");
            drw_dbg(address); drw_dbg(" size= "); drw_dbg(size);
            sections[secEnum::HEADER] = si;
        } else if (rec == 1) {
            drw_dbg("\nSection CLASSES address= ");
            drw_dbg(address); drw_dbg(" size= "); drw_dbg(size);
            sections[secEnum::CLASSES] = si;
        } else if (rec == 2) {
            drw_dbg("\nSection OBJECTS (handles) address= ");
            drw_dbg(address); drw_dbg(" size= "); drw_dbg(size);
            sections[secEnum::HANDLES] = si;
        } else if (rec == 3) {
            drw_dbg("\nSection UNKNOWN address= ");
            drw_dbg(address); drw_dbg(" size= "); drw_dbg(size);
            sections[secEnum::UNKNOWNS] = si;
        } else if (rec == 4) {
            drw_dbg("\nSection R14DATA (AcDb:Template) address= ");
            drw_dbg(address); drw_dbg(" size= "); drw_dbg(size);
            sections[secEnum::TEMPLATE] = si;
        } else if (rec == 5) {
            drw_dbg("\nSection R14REC5 (AcDb:AuxHeader) address= ");
            drw_dbg(address); drw_dbg(" size= "); drw_dbg(size);
            sections[secEnum::AUXHEADER] = si;
        } else {
            std::cerr << "\nUnsupported section number\n";
        }
    }
    if (! fileBuf->isGood())
        return false;
    drw_dbg("\nposition after read section locator records= "); drw_dbg(fileBuf->getPosition());
    drw_dbg(", bit are= "); drw_dbg(fileBuf->getBitPos());
    uint32_t ckcrc = fileBuf->crc8(0,0,fileBuf->getPosition());
    drw_dbg("\nfile header crc8 0 result= "); drw_dbg(ckcrc);
    switch (count){
    case 3:
        ckcrc = ckcrc ^ 0xA598;
        break;
    case 4:
        ckcrc = ckcrc ^ 0x8101;
        break;
    case 5:
        ckcrc = ckcrc ^ 0x3CC4;
        break;
    case 6:
        ckcrc = ckcrc ^ 0x8461;
    }
    drw_dbg("\nfile header crc8 xor result= "); drw_dbg(ckcrc);
    drw_dbg("\nfile header CRC= "); drw_dbg(fileBuf->getRawShort16());
    drw_dbg("\nfile header sentinel= ");
    checkSentinel(fileBuf.get(), secEnum::FILEHEADER, false);

    drw_dbg("\nposition after read file header sentinel= "); drw_dbg(fileBuf->getPosition());
    drw_dbg(", bit are= "); drw_dbg(fileBuf->getBitPos());

    drw_dbg("\ndwgReader15::readFileHeader END\n");
    return ret;
}

bool dwgReader15::readDwgHeader(DRW_Header& hdr){
    drw_dbg("dwgReader15::readDwgHeader\n");
    dwgSectionInfo si = sections[secEnum::HEADER];
    if (si.Id<0)//not found, ends
        return false;
    if (!fileBuf->setPosition(si.address))
        return false;
    std::vector<uint8_t> tmpByteStr(si.size);
    fileBuf->getBytes(tmpByteStr.data(), si.size);
    dwgBuffer buff(tmpByteStr.data(), si.size, &decoder);
    drw_dbg("Header section sentinel= ");
    checkSentinel(&buff, secEnum::HEADER, true);
    bool ret = dwgReader::readDwgHeader(hdr, &buff, &buff);
    return ret;
}


bool dwgReader15::readDwgClasses(){
    drw_dbg("\ndwgReader15::readDwgClasses\n");
    dwgSectionInfo si = sections[secEnum::CLASSES];
    if (si.Id<0)//not found, ends
        return false;
    if (!fileBuf->setPosition(si.address))
        return false;

    drw_dbg("classes section sentinel= ");
    checkSentinel(fileBuf.get(), secEnum::CLASSES, true);

    uint32_t size = fileBuf->getRawLong32();
    if (size != (si.size - 38)) {
        drw_dbg("\nWARNING dwgReader15::readDwgClasses size are "); drw_dbg(size);
        drw_dbg(" and secSize - 38 are "); drw_dbg(si.size - 38); drw_dbg("\n");
    }
    std::vector<uint8_t> tmpByteStr(size);
    fileBuf->getBytes(tmpByteStr.data(), size);
    dwgBuffer buff(tmpByteStr.data(), size, &decoder);
    size--; //reduce 1 byte instead of check pos + bitPos
    while (size > buff.getPosition()) {
        DRW_Class *cl = new DRW_Class();
        cl->parseDwg(version, &buff, &buff);
        classesmap[cl->classNum] = cl;
    }
     drw_dbg("\nCRC: "); drw_dbgh(fileBuf->getRawShort16());
     drw_dbg("\nclasses section end sentinel= ");
     checkSentinel(fileBuf.get(), secEnum::CLASSES, false);
     bool ret = buff.isGood();
     return ret;
}

bool dwgReader15::readDwgHandles() {
    drw_dbg("\ndwgReader15::readDwgHandles\n");
    dwgSectionInfo si = sections[secEnum::HANDLES];
    if (si.Id<0)//not found, ends
        return false;

    bool ret = dwgReader::readDwgHandles(fileBuf.get(), si.address, si.size);
    return ret;
}

/*********** objects ************************/
/**
 * Reads all the object referenced in the object map section of the DWG file
 * (using their object file offsets)
 */
bool dwgReader15::readDwgTables(DRW_Header& hdr) {
    bool ret = dwgReader::readDwgTables(hdr, fileBuf.get());

    return ret;
}

/**
 * Reads all the object referenced in the object map section of the DWG file
 * (using their object file offsets)
 */
bool dwgReader15::readDwgBlocks(DRW_Interface& intfa) {
    bool ret = true;
    ret = dwgReader::readDwgBlocks(intfa, fileBuf.get());
    return ret;
}

