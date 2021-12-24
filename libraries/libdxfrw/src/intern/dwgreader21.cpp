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
#include "dwgreader21.h"
#include "drw_textcodec.h"
#include "../libdwgr.h"

bool dwgReader21::readMetaData() {
    version = parent->getVersion();
    decoder.setVersion(version, false);
    drw_dbg("dwgReader21::readFileHeader()\n");
    drw_dbg("dwgReader21::parsing metadata\n");
    if (! fileBuf->setPosition(11))
        return false;
    maintenanceVersion = fileBuf->getRawChar8();
    drw_dbg("maintenance version= "); drw_dbgh(maintenanceVersion);
    drw_dbg("\nbyte at 0x0C= "); drw_dbg(fileBuf->getRawChar8());
    previewImagePos = fileBuf->getRawLong32();
    drw_dbg("previewImagePos (seekerImageData) = "); drw_dbg(previewImagePos);
    drw_dbg("\n\napp writer version= "); drw_dbgh(fileBuf->getRawChar8());
    drw_dbg("\napp writer maintenance version= "); drw_dbgh(fileBuf->getRawChar8());
    duint16 cp = fileBuf->getRawShort16();
    drw_dbg("\ncodepage= "); drw_dbg(cp);
    if (cp == 30)
        decoder.setCodePage("ANSI_1252", false);
    /* UNKNOUWN SECTION 2 bytes*/
    drw_dbg("\nUNKNOWN SECTION= "); drw_dbg(fileBuf->getRawShort16());
    drw_dbg("\nUNKNOUWN SECTION 3b= "); drw_dbg(fileBuf->getRawChar8());
    duint32 secType = fileBuf->getRawLong32();
    drw_dbg("\nsecurity type flag= "); drw_dbgh(secType);
    /* UNKNOWN2 SECTION 4 bytes*/
    drw_dbg("\nUNKNOWN SECTION 4bytes= "); drw_dbg(fileBuf->getRawLong32());

    drw_dbg("\nSummary info address= "); drw_dbgh(fileBuf->getRawLong32());
    drw_dbg("\nVBA project address= "); drw_dbgh(fileBuf->getRawLong32());
    drw_dbg("\n0x00000080 32b= "); drw_dbgh(fileBuf->getRawLong32());
    drw_dbg("\nApp info address= "); drw_dbgh(fileBuf->getRawLong32());
    //current position are 0x30 from here to 0x80 are undocumented
    drw_dbg("\nAnother address? = "); drw_dbgh(fileBuf->getRawLong32());
    return true;
}

bool dwgReader21::parseSysPage(duint64 sizeCompressed, duint64 sizeUncompressed, duint64 correctionFactor, duint64 offset, duint8 *decompData){
    //round to 8
    duint64 alsize = (sizeCompressed + 7) &(-8);
    //minimum RS chunk:
    duint32 chunks = (((alsize * correctionFactor)+238)/239);
    duint64 fpsize = chunks * 255;

    if (! fileBuf->setPosition(offset))
        return false;
    std::vector<duint8> tmpDataRaw(fpsize);
    fileBuf->getBytes(&tmpDataRaw.front(), fpsize);
    std::vector<duint8> tmpDataRS(fpsize);
    dwgRSCodec::decode239I(&tmpDataRaw.front(), &tmpDataRS.front(), fpsize/255);

    return dwgCompressor::decompress21(&tmpDataRS.front(), decompData, sizeCompressed, sizeUncompressed);
}

bool dwgReader21::parseDataPage(const dwgSectionInfo &si, duint8 *dData){
    drw_dbg("parseDataPage, section size: "); drw_dbg(si.size);
    for (auto it=si.pages.begin(); it!=si.pages.end(); ++it){
        dwgPageInfo pi = it->second;
        if (!fileBuf->setPosition(pi.address))
            return false;

        std::vector<duint8> tmpPageRaw(pi.size);
        fileBuf->getBytes(&tmpPageRaw.front(), pi.size);
    #ifdef DRW_DBG_DUMP
        drw_dbg("\nSection OBJECTS raw data=\n");
        for (unsigned int i=0, j=0; i< pi.size;i++) {
            drw_dbgh( (unsigned char)tmpPageRaw[i]);
            if (j == 7) { drw_dbg("\n"); j = 0;
            } else { drw_dbg(", "); j++; }
        } drw_dbg("\n");
    #endif

        std::vector<duint8> tmpPageRS(pi.size);

        duint8 chunks =pi.size / 255;
        dwgRSCodec::decode251I(&tmpPageRaw.front(), &tmpPageRS.front(), chunks);
    #ifdef DRW_DBG_DUMP
        drw_dbg("\nSection OBJECTS RS data=\n");
        for (unsigned int i=0, j=0; i< pi.size;i++) {
            drw_dbgh( (unsigned char)tmpPageRS[i]);
            if (j == 7) { drw_dbg("\n"); j = 0;
            } else { drw_dbg(", "); j++; }
        } drw_dbg("\n");
    #endif

        drw_dbg("\npage uncomp size: "); drw_dbg(pi.uSize); drw_dbg(" comp size: "); drw_dbg(pi.cSize);
        drw_dbg("\noffset: "); drw_dbg(pi.startOffset);
        duint8 *pageData = dData + pi.startOffset;
        if (!dwgCompressor::decompress21(&tmpPageRS.front(), pageData, pi.cSize, pi.uSize)) {
            return false;
        }

    #ifdef DRW_DBG_DUMP
        drw_dbg("\n\nSection OBJECTS decompressed data=\n");
        for (unsigned int i=0, j=0; i< pi.uSize;i++) {
            drw_dbgh( (unsigned char)pageData[i]);
            if (j == 7) { drw_dbg("\n"); j = 0;
            } else { drw_dbg(", "); j++; }
        } drw_dbg("\n");
    #endif

    }
    drw_dbg("\n");
    return true;
}

bool dwgReader21::readFileHeader() {

    drw_dbg("\n\ndwgReader21::parsing file header\n");
    if (! fileBuf->setPosition(0x80))
        return false;
    duint8 fileHdrRaw[0x2FD];//0x3D8
    fileBuf->getBytes(fileHdrRaw, 0x2FD);
    duint8 fileHdrdRS[0x2CD];
    dwgRSCodec::decode239I(fileHdrRaw, fileHdrdRS, 3);

#ifdef DRW_DBG_DUMP
    drw_dbg("\ndwgReader21::parsed Reed Solomon decode:\n");
    int j = 0;
    for (int i=0, j=0; i<0x2CD; i++){
        drw_dbgh( (unsigned char)fileHdrdRS[i]);
        if (j== 15){ j=0; drw_dbg("\n");
        } else{ j++; drw_dbg(", "); }
    } drw_dbg("\n");
#endif

    dwgBuffer fileHdrBuf(fileHdrdRS, 0x2CD, &decoder);
    drw_dbg("\nCRC 64b= "); drw_dbgh(fileHdrBuf.getRawLong64());
    drw_dbg("\nunknown key 64b= "); drw_dbgh(fileHdrBuf.getRawLong64());
    drw_dbg("\ncomp data CRC 64b= "); drw_dbgh(fileHdrBuf.getRawLong64());
    dint32 fileHdrCompLength = fileHdrBuf.getRawLong32();
    drw_dbg("\ncompr len 4bytes= "); drw_dbg(fileHdrCompLength);
    dint32 fileHdrCompLength2 = fileHdrBuf.getRawLong32();
    drw_dbg("\nlength2 4bytes= "); drw_dbg(fileHdrCompLength2);

    int fileHdrDataLength = 0x110;
    std::vector<duint8> fileHdrData;
    if (fileHdrCompLength < 0) {
        fileHdrDataLength = fileHdrCompLength * -1;
        fileHdrData.resize(fileHdrDataLength);
        fileHdrBuf.getBytes(&fileHdrData.front(), fileHdrDataLength);
    }
    else {
        drw_dbg("\ndwgReader21:: file header are compressed:\n");
        std::vector<duint8> compByteStr(fileHdrCompLength);
        fileHdrBuf.getBytes(compByteStr.data(), fileHdrCompLength);
        fileHdrData.resize(fileHdrDataLength);
        if (!dwgCompressor::decompress21(compByteStr.data(), &fileHdrData.front(),
                                         fileHdrCompLength, fileHdrDataLength)) {
            return false;
        }
    }

#ifdef DRW_DBG_DUMP
    drw_dbg("\ndwgReader21::parsed file header:\n");
    for (int i=0, j=0; i<fileHdrDataLength; i++){
        drw_dbgh( (unsigned char)fileHdrData[i]);
        if (j== 15){ j=0; drw_dbg("\n");
        } else{ j++; drw_dbg(", "); }
    } drw_dbg("\n");
#endif

    dwgBuffer fileHdrDataBuf(&fileHdrData.front(), fileHdrDataLength, &decoder);
    drw_dbg("\nHeader size = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    drw_dbg("\nFile size = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    drw_dbg("\nPagesMapCrcCompressed = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    duint64 PagesMapCorrectionFactor = fileHdrDataBuf.getRawLong64();
    drw_dbg("\nPagesMapCorrectionFactor = "); drw_dbg(PagesMapCorrectionFactor);
    drw_dbg("\nPagesMapCrcSeed = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    drw_dbg("\nPages map2offset = "); drw_dbgh(fileHdrDataBuf.getRawLong64()); //relative to data page map 1, add 0x480 to get stream position
    drw_dbg("\nPages map2Id = "); drw_dbg(fileHdrDataBuf.getRawLong64());
    duint64 PagesMapOffset = fileHdrDataBuf.getRawLong64();
    drw_dbg("\nPagesMapOffset = "); drw_dbgh(PagesMapOffset); //relative to data page map 1, add 0x480 to get stream position
    drw_dbg("\nPagesMapId = "); drw_dbg(fileHdrDataBuf.getRawLong64());
    drw_dbg("\nHeader2offset = "); drw_dbgh(fileHdrDataBuf.getRawLong64()); //relative to data page map 1, add 0x480 to get stream position
    duint64 PagesMapSizeCompressed = fileHdrDataBuf.getRawLong64();
    drw_dbg("\nPagesMapSizeCompressed = "); drw_dbg(PagesMapSizeCompressed);
    duint64 PagesMapSizeUncompressed = fileHdrDataBuf.getRawLong64();
    drw_dbg("\nPagesMapSizeUncompressed = "); drw_dbg(PagesMapSizeUncompressed);
    drw_dbg("\nPagesAmount = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    duint64 PagesMaxId = fileHdrDataBuf.getRawLong64();
    drw_dbg("\nPagesMaxId = "); drw_dbg(PagesMaxId);
    drw_dbg("\nUnknown (normally 0x20) = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    drw_dbg("\nUnknown (normally 0x40) = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    drw_dbg("\nPagesMapCrcUncompressed = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    drw_dbg("\nUnknown (normally 0xf800) = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    drw_dbg("\nUnknown (normally 4) = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    drw_dbg("\nUnknown (normally 1) = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    drw_dbg("\nSectionsAmount (number of sections + 1) = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    drw_dbg("\nSectionsMapCrcUncompressed = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    duint64 SectionsMapSizeCompressed = fileHdrDataBuf.getRawLong64();
    drw_dbg("\nSectionsMapSizeCompressed = "); drw_dbgh(SectionsMapSizeCompressed);
    drw_dbg("\nSectionsMap2Id = "); drw_dbg(fileHdrDataBuf.getRawLong64());
    duint64 SectionsMapId = fileHdrDataBuf.getRawLong64();
    drw_dbg("\nSectionsMapId = "); drw_dbg(SectionsMapId);
    duint64 SectionsMapSizeUncompressed = fileHdrDataBuf.getRawLong64();
    drw_dbg("\nSectionsMapSizeUncompressed = "); drw_dbgh(SectionsMapSizeUncompressed);
    drw_dbg("\nSectionsMapCrcCompressed = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    duint64 SectionsMapCorrectionFactor = fileHdrDataBuf.getRawLong64();
    drw_dbg("\nSectionsMapCorrectionFactor = "); drw_dbg(SectionsMapCorrectionFactor);
    drw_dbg("\nSectionsMapCrcSeed = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    drw_dbg("\nStreamVersion (normally 0x60100) = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    drw_dbg("\nCrcSeed = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    drw_dbg("\nCrcSeedEncoded = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    drw_dbg("\nRandomSeed = "); drw_dbgh(fileHdrDataBuf.getRawLong64());
    drw_dbg("\nHeader CRC64 = "); drw_dbgh(fileHdrDataBuf.getRawLong64()); drw_dbg("\n");

    drw_dbg("\ndwgReader21::parse page map:\n");
    std::vector<duint8> PagesMapData(PagesMapSizeUncompressed);

    if (!parseSysPage(PagesMapSizeCompressed, PagesMapSizeUncompressed,
                      PagesMapCorrectionFactor, 0x480+PagesMapOffset,
                      &PagesMapData.front())) {
        return false;
    }

    duint64 address = 0x480;
    duint64 i = 0;
    dwgBuffer PagesMapBuf(&PagesMapData.front(), PagesMapSizeUncompressed, &decoder);
    //stores temporarily info of all pages:
    std::unordered_map<duint64, dwgPageInfo >sectionPageMapTmp;

//    dwgPageInfo *m_pages= new dwgPageInfo[PagesMaxId+1];
    while (PagesMapSizeUncompressed > i ) {
        duint64 size = PagesMapBuf.getRawLong64();
        dint64 id = PagesMapBuf.getRawLong64();
        duint64 ind = id > 0 ? id : -id;
        i += 16;

        drw_dbg("Page gap= "); drw_dbg(id); drw_dbg(" Page num= "); drw_dbg(ind); drw_dbg(" size= "); drw_dbgh(size);
        drw_dbg(" address= "); drw_dbgh(address);  drw_dbg("\n");
        sectionPageMapTmp[ind] = dwgPageInfo(ind, address,size);
        address += size;
        //TODO num can be negative indicating gap
//        seek += offset;
    }

    drw_dbg("\n*** dwgReader21: Processing Section Map ***\n");
    std::vector<duint8> SectionsMapData( SectionsMapSizeUncompressed);
    dwgPageInfo sectionMap = sectionPageMapTmp[SectionsMapId];
    if (!parseSysPage( SectionsMapSizeCompressed, SectionsMapSizeUncompressed,
                       SectionsMapCorrectionFactor, sectionMap.address, &SectionsMapData.front()) ) {
        return false;
    }

//reads sections:
    //Note: compressed value are not stored in file then, commpresed field are use to store
    // encoding value
    dwgBuffer SectionsMapBuf( &SectionsMapData.front(), SectionsMapSizeUncompressed, &decoder);
    duint8 nextId = 1;
    while(SectionsMapBuf.getPosition() < SectionsMapBuf.size()){
        dwgSectionInfo secInfo;
        secInfo.size = SectionsMapBuf.getRawLong64();
        drw_dbg("\nSize of section (data size)= "); drw_dbgh(secInfo.size);
        secInfo.maxSize = SectionsMapBuf.getRawLong64();
        drw_dbg("\nMax Decompressed Size= "); drw_dbgh(secInfo.maxSize);
        secInfo.encrypted = SectionsMapBuf.getRawLong64();
        //encrypted (doc: 0 no, 1 yes, 2 unkn) on read: objects 0 and encrypted yes
        drw_dbg("\nencription= "); drw_dbgh(secInfo.encrypted);
        drw_dbg("\nHashCode = "); drw_dbgh(SectionsMapBuf.getRawLong64());
        duint64 SectionNameLength = SectionsMapBuf.getRawLong64();
        drw_dbg("\nSectionNameLength = "); drw_dbg(SectionNameLength);
        drw_dbg("\nUnknown = "); drw_dbgh(SectionsMapBuf.getRawLong64());
        secInfo.compressed = SectionsMapBuf.getRawLong64();
        drw_dbg("\nEncoding (compressed) = "); drw_dbgh(secInfo.compressed);
        secInfo.pageCount = SectionsMapBuf.getRawLong64();
        drw_dbg("\nPage count= "); drw_dbgh(secInfo.pageCount);
        secInfo.name = SectionsMapBuf.getUCSStr(SectionNameLength);
        drw_dbg("\nSection name = "); drw_dbg(secInfo.name); drw_dbg("\n");

        for (unsigned int i=0; i< secInfo.pageCount; i++){
            duint64 po = SectionsMapBuf.getRawLong64();
            duint32 ds = SectionsMapBuf.getRawLong64();
            duint32 pn = SectionsMapBuf.getRawLong64();
            drw_dbg("  pag Id = "); drw_dbgh(pn); drw_dbg(" data size = "); drw_dbgh(ds);
            dwgPageInfo pi = sectionPageMapTmp[pn]; //get a copy
            pi.dataSize = ds;
            pi.startOffset = po;
            pi.uSize = SectionsMapBuf.getRawLong64();
            pi.cSize = SectionsMapBuf.getRawLong64();
            secInfo.pages[pn]= pi;//complete copy in secInfo
            drw_dbg("\n    Page number= "); drw_dbgh(secInfo.pages[pn].Id);
            drw_dbg("\n    address in file= "); drw_dbgh(secInfo.pages[pn].address);
            drw_dbg("\n    size in file= "); drw_dbgh(secInfo.pages[pn].size);
            drw_dbg("\n    Data size= "); drw_dbgh(secInfo.pages[pn].dataSize);
            drw_dbg("\n    Start offset= "); drw_dbgh(secInfo.pages[pn].startOffset);
            drw_dbg("\n    Page uncompressed size = "); drw_dbgh(secInfo.pages[pn].uSize);
            drw_dbg("\n    Page compressed size = "); drw_dbgh(secInfo.pages[pn].cSize);

            drw_dbg("\n    Page checksum = "); drw_dbgh(SectionsMapBuf.getRawLong64());
            drw_dbg("\n    Page CRC = "); drw_dbgh(SectionsMapBuf.getRawLong64()); drw_dbg("\n");
        }

        if (!secInfo.name.empty()) {
            secInfo.Id = nextId++;
            drw_dbg("Saved section Name= "); drw_dbg( secInfo.name.c_str() ); drw_dbg("\n");
            sections[secEnum::getEnum(secInfo.name)] = secInfo;
        }
    }

    if (! fileBuf->isGood())
        return false;

    drw_dbg("\ndwgReader21::readFileHeader END\n");
    return true;
}

bool dwgReader21::readDwgHeader(DRW_Header& hdr){
    drw_dbg("\ndwgReader21::readDwgHeader\n");
    dwgSectionInfo si = sections[secEnum::HEADER];
    if (si.Id < 0) { //not found, ends
        return false;
    }

    drw_dbg("\nprepare section of size "); drw_dbg(si.size);drw_dbg("\n");
    std::vector<duint8> tmpHeaderData( si.size);

    if (!dwgReader21::parseDataPage( si, &tmpHeaderData.front())) {
        return false;
    }

    dwgBuffer dataBuf(&tmpHeaderData.front(), si.size, &decoder);
    dwgBuffer handleBuf(&tmpHeaderData.front(), si.size, &decoder);
    drw_dbg("Header section sentinel= ");
    checkSentinel(&dataBuf, secEnum::HEADER, true);

    return dwgReader::readDwgHeader(hdr, &dataBuf, &handleBuf);
}

bool dwgReader21::readDwgClasses(){
    drw_dbg("\ndwgReader21::readDwgClasses");
    dwgSectionInfo si = sections[secEnum::CLASSES];
    if (si.Id<0)//not found, ends
        return false;

    drw_dbg("\nprepare section of size "); drw_dbg(si.size);drw_dbg("\n");
    std::vector<duint8> tmpClassesData( si.size);
    bool ret = dwgReader21::parseDataPage(si, tmpClassesData.data());
    if (!ret) {
        return ret;
    }

    dwgBuffer buff(tmpClassesData.data(), si.size, &decoder);
    drw_dbg("classes section sentinel= ");
    checkSentinel(&buff, secEnum::CLASSES, true);

    duint32 size = buff.getRawLong32();
    drw_dbg("\ndata size in bytes "); drw_dbg(size);

    duint32 bitSize = buff.getRawLong32();
    drw_dbg("\ntotal size in bits "); drw_dbg(bitSize);

    duint32 maxClassNum = buff.getBitShort();
    drw_dbg("\nMaximum class number "); drw_dbg(maxClassNum);
    drw_dbg("\nRc 1 "); drw_dbg(buff.getRawChar8());
    drw_dbg("\nRc 2 "); drw_dbg(buff.getRawChar8());
    drw_dbg("\nBit "); drw_dbg(buff.getBit());

    /*******************************/
    //prepare string stream
    dwgBuffer strBuff(tmpClassesData.data(), si.size, &decoder);
    duint32 strStartPos = bitSize + 159;//size in bits + 20 bytes (sn+size) - 1 bit (endbit)
    drw_dbg("\nstrStartPos: "); drw_dbg(strStartPos);
    strBuff.setPosition(strStartPos >> 3);
    strBuff.setBitPos(strStartPos & 7);
    drw_dbg("\nclasses strings buff.getPosition: "); drw_dbg(strBuff.getPosition());
    drw_dbg("\nclasses strings buff.getBitPos: "); drw_dbg(strBuff.getBitPos());
    drw_dbg("\nendBit "); drw_dbg(strBuff.getBit());
    strStartPos -= 16;//decrement 16 bits
    drw_dbg("\nstrStartPos: "); drw_dbg(strStartPos);
    strBuff.setPosition(strStartPos >> 3);
    strBuff.setBitPos(strStartPos & 7);
    drw_dbg("\nclasses strings buff.getPosition: "); drw_dbg(strBuff.getPosition());
    drw_dbg("\nclasses strings buff.getBitPos: "); drw_dbg(strBuff.getBitPos());
    duint32 strDataSize = strBuff.getRawShort16();
    drw_dbg("\nstrDataSize: "); drw_dbg(strDataSize);
    if (strDataSize & 0x8000) {
        strStartPos -= 16;//decrement 16 bits
        strDataSize &= 0x7FFF; //strip 0x8000;
        strBuff.setPosition(strStartPos >> 3);
        strBuff.setBitPos(strStartPos & 7);
        duint32 hiSize = strBuff.getRawShort16();
        strDataSize |= (hiSize << 15);
    }
    strStartPos -= strDataSize;
    drw_dbg("\nstrStartPos: "); drw_dbg(strStartPos);
    strBuff.setPosition(strStartPos >> 3);
    strBuff.setBitPos(strStartPos & 7);
    drw_dbg("\nclasses strings buff.getPosition: "); drw_dbg(strBuff.getPosition());
    drw_dbg("\nclasses strings buff.getBitPos: "); drw_dbg(strBuff.getBitPos());


    /*******************************/

    duint32 endDataPos = maxClassNum-499;
    drw_dbg("\nbuff.getPosition: "); drw_dbg(buff.getPosition());
    for (duint32 i= 0; i<endDataPos;i++) {
        DRW_Class *cl = new DRW_Class();
        cl->parseDwg(version, &buff, &strBuff);
        classesmap[cl->classNum] = cl;
        drw_dbg("\nbuff.getPosition: "); drw_dbg(buff.getPosition());
    }
    drw_dbg("\nend classes data buff.getPosition: "); drw_dbg(buff.getPosition());
    drw_dbg("\nend classes data buff.getBitPos: "); drw_dbg(buff.getBitPos());

    buff.setPosition(size+20);//sizeVal+sn+32bSize
    drw_dbg("\nCRC: "); drw_dbgh(buff.getRawShort16());
    drw_dbg("\nclasses section end sentinel= ");
    checkSentinel(&buff, secEnum::CLASSES, true);
    return buff.isGood();
}


bool dwgReader21::readDwgHandles(){
    drw_dbg("\ndwgReader21::readDwgHandles");
    dwgSectionInfo si = sections[secEnum::HANDLES];
    if (si.Id<0)//not found, ends
        return false;

    drw_dbg("\nprepare section of size "); drw_dbg(si.size);drw_dbg("\n");
    std::vector<duint8> tmpHandlesData(si.size);
    bool ret = dwgReader21::parseDataPage(si, tmpHandlesData.data());
    if (!ret)
        return ret;

    dwgBuffer dataBuf(tmpHandlesData.data(), si.size, &decoder);

    ret = dwgReader::readDwgHandles(&dataBuf, 0, si.size);
    return ret;
}

/*********** objects ************************/
/**
 * Reads all the object referenced in the object map section of the DWG file
 * (using their object file offsets)
 */
bool dwgReader21::readDwgTables(DRW_Header& hdr) {
    drw_dbg("\ndwgReader21::readDwgTables\n");
    dwgSectionInfo si = sections[secEnum::OBJECTS];
    if (si.Id<0)//not found, ends
        return false;

    drw_dbg("\nprepare section of size "); drw_dbg(si.size);drw_dbg("\n");
    dataSize = si.size;
    objData.reset( new duint8 [dataSize] );
    bool ret = dwgReader21::parseDataPage(si, objData.get());
    if (!ret)
        return ret;

    drw_dbg("readDwgTables total data size= "); drw_dbg(dataSize); drw_dbg("\n");
    dwgBuffer dataBuf(objData.get(), dataSize, &decoder);
    ret = dwgReader::readDwgTables(hdr, &dataBuf);

    return ret;
}


bool dwgReader21::readDwgBlocks(DRW_Interface& intfa){
    bool ret = true;
    dwgBuffer dataBuf(objData.get(), dataSize, &decoder);
    ret = dwgReader::readDwgBlocks(intfa, &dataBuf);
    return ret;
}

