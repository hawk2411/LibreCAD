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
#include "dwgreader18.h"
#include "dwgutil.h"
#include "drw_textcodec.h"
#include "../libdwgr.h"

void dwgReader18::genMagicNumber(){
    int size =0x114;
    std::vector<duint8> tmpMagicStr(size);
    duint8 *p = tmpMagicStr.data();
    int rSeed =1;
    while (size--) {
        rSeed *= 0x343fd;
        rSeed += 0x269ec3;
        *p++ = static_cast<duint8>(rSeed >> 0x10);
    }
    int j = 0;
    size =0x114;
    for (int i=0; i< size;i++) {
        drw_dbgh(tmpMagicStr[i]);
        if (j == 15) {
            drw_dbg("\n");
            j = 0;
        } else {
            drw_dbg(", ");
            j++;
        }
    }
}

duint32 dwgReader18::checksum(duint32 seed, duint8* data, duint64 sz){
    duint64 size = sz;
    duint32 sum1 = seed & 0xffff;
    duint32 sum2 = seed >> 0x10;
    while (size != 0) {
//        duint32 chunkSize = min(0x15b0, size);
        duint64 chunkSize = 0x15b0 < size? 0x15b0:size;
        size -= chunkSize;
        for (duint64 i = 0; i < chunkSize; i++) {
            sum1 += *data++;
            sum2 += sum1;
        }
        sum1 %= 0xFFF1;
        sum2 %= 0xFFF1;
    }
    return (sum2 << 0x10) | (sum1 & 0xffff);
}

//called: Section page map: 0x41630e3b
bool dwgReader18::parseSysPage(duint8 *decompSec, duint32 decompSize){
    drw_dbg("\nparseSysPage:\n ");
    duint32 compSize = fileBuf->getRawLong32();
    drw_dbg("Compressed size= "); drw_dbg(compSize); drw_dbg(", "); drw_dbgh(compSize);
    drw_dbg("\nCompression type= "); drw_dbgh(fileBuf->getRawLong32());
    drw_dbg("\nSection page checksum= "); drw_dbgh(fileBuf->getRawLong32()); drw_dbg("\n");

    duint8 hdrData[20];
    fileBuf->moveBitPos(-160);
    fileBuf->getBytes(hdrData, 20);
    for (duint8 i= 16; i<20; ++i)
        hdrData[i]=0;
    duint32 calcsH = checksum(0, hdrData, 20);
    drw_dbg("Calc hdr checksum= "); drw_dbgh(calcsH);
    std::vector<duint8> tmpCompSec(compSize);
    fileBuf->getBytes(tmpCompSec.data(), compSize);
    duint32 calcsD = checksum(calcsH, tmpCompSec.data(), compSize);
    drw_dbg("\nCalc data checksum= "); drw_dbgh(calcsD); drw_dbg("\n");

#ifdef DRW_DBG_DUMP
    for (unsigned int i=0, j=0; i< compSize;i++) {
        drw_dbgh( (unsigned char)compSec[i]);
        if (j == 7) { drw_dbg("\n"); j = 0;
        } else { drw_dbg(", "); j++; }
    } drw_dbg("\n");
#endif
    drw_dbg("decompressing "); drw_dbg(compSize); drw_dbg(" bytes in "); drw_dbg(decompSize); drw_dbg(" bytes\n");
    dwgCompressor comp;
    if (!comp.decompress18(tmpCompSec.data(), decompSec, compSize, decompSize)) {
        return false;
    }

#ifdef DRW_DBG_DUMP
    for (unsigned int i=0, j=0; i< decompSize;i++) {
        drw_dbgh( decompSec[i]);
        if (j == 7) { drw_dbg("\n"); j = 0;
        } else { drw_dbg(", "); j++; }
    } drw_dbg("\n");
#endif

    return true;
}

 //called ???: Section map: 0x4163003b
bool dwgReader18::parseDataPage(const dwgSectionInfo &si/*, duint8 *dData*/){
    drw_dbg("\nparseDataPage\n ");
    objData.reset( new duint8 [si.pageCount * si.maxSize] );

    for (auto it=si.pages.begin(); it!=si.pages.end(); ++it){
        dwgPageInfo pi = it->second;
        if (!fileBuf->setPosition(pi.address))
            return false;
        //decript section header
        duint8 hdrData[32];
        fileBuf->getBytes(hdrData, 32);
        dwgCompressor::decrypt18Hdr(hdrData, 32, pi.address);
        drw_dbg("Section  "); drw_dbg(si.name); drw_dbg(" page header=\n");
        for (unsigned int i=0, j=0; i< 32;i++) {
            drw_dbgh( static_cast<unsigned char>(hdrData[i]));
            if (j == 7) {
                drw_dbg("\n");
                j = 0;
            } else {
                drw_dbg(", ");
                j++;
            }
        } drw_dbg("\n");

        drw_dbg("\n    Page number= "); drw_dbgh(pi.Id);
        drw_dbg("\n    size in file= "); drw_dbgh(pi.size);
        drw_dbg("\n    address in file= "); drw_dbgh(pi.address);
        drw_dbg("\n    Data size= "); drw_dbgh(pi.dataSize);
        drw_dbg("\n    Start offset= "); drw_dbgh(pi.startOffset); drw_dbg("\n");
        dwgBuffer bufHdr(hdrData, 32, &decoder);
        drw_dbg("      section page type= "); drw_dbgh(bufHdr.getRawLong32());
        drw_dbg("\n      section number= "); drw_dbgh(bufHdr.getRawLong32());
        pi.cSize = bufHdr.getRawLong32();
        drw_dbg("\n      data size (compressed)= "); drw_dbgh(pi.cSize); drw_dbg(" dec "); drw_dbg(pi.cSize);
        pi.uSize = bufHdr.getRawLong32();
        drw_dbg("\n      page size (decompressed)= "); drw_dbgh(pi.uSize); drw_dbg(" dec "); drw_dbg(pi.uSize);
        drw_dbg("\n      start offset (in decompressed buffer)= "); drw_dbgh(bufHdr.getRawLong32());
        drw_dbg("\n      unknown= "); drw_dbgh(bufHdr.getRawLong32());
        drw_dbg("\n      header checksum= "); drw_dbgh(bufHdr.getRawLong32());
        drw_dbg("\n      data checksum= "); drw_dbgh(bufHdr.getRawLong32()); drw_dbg("\n");

        //get compressed data
        std::vector<duint8> cData(pi.cSize);
        if (!fileBuf->setPosition(pi.address + 32)) {
            return false;
        }
        fileBuf->getBytes(cData.data(), pi.cSize);

        //calculate checksum
        duint32 calcsD = checksum(0, cData.data(), pi.cSize);
        for (duint8 i= 24; i<28; ++i)
            hdrData[i]=0;
        duint32 calcsH = checksum(calcsD, hdrData, 32);
        drw_dbg("Calc header checksum= "); drw_dbgh(calcsH);
        drw_dbg("\nCalc data checksum= "); drw_dbgh(calcsD); drw_dbg("\n");

        duint8* oData = objData.get() + pi.startOffset;
        pi.uSize = si.maxSize;
        drw_dbg("decompressing "); drw_dbg(pi.cSize); drw_dbg(" bytes in "); drw_dbg(pi.uSize); drw_dbg(" bytes\n");
        dwgCompressor comp;
        if (!comp.decompress18(cData.data(), oData, pi.cSize, pi.uSize)) {
            return false;
        }
    }
    return true;
}

bool dwgReader18::readMetaData() {
    version = parent->getVersion();
    decoder.setVersion(version, false);
    drw_dbg("dwgReader18::readMetaData\n");
    if (! fileBuf->setPosition(11))
        return false;
    maintenanceVersion = fileBuf->getRawChar8();
    drw_dbg("maintenance version= "); drw_dbgh(maintenanceVersion);
    drw_dbg("\nbyte at 0x0C= "); drw_dbgh(fileBuf->getRawChar8());
    previewImagePos = fileBuf->getRawLong32(); //+ page header size (0x20).
    drw_dbg("\npreviewImagePos (seekerImageData) = "); drw_dbg(previewImagePos);
    drw_dbg("\napp Dwg version= "); drw_dbgh(fileBuf->getRawChar8()); drw_dbg(", ");
    drw_dbg("\napp maintenance version= "); drw_dbgh(fileBuf->getRawChar8());
    duint16 cp = fileBuf->getRawShort16();
    drw_dbg("\ncodepage= "); drw_dbg(cp);
    if (cp == 30)
        decoder.setCodePage("ANSI_1252", false);
    drw_dbg("\n3 0x00 bytes(seems 0x00, appDwgV & appMaintV) = "); drw_dbgh(fileBuf->getRawChar8()); drw_dbg(", ");
    drw_dbgh(fileBuf->getRawChar8()); drw_dbg(", "); drw_dbgh(fileBuf->getRawChar8());
    securityFlags = fileBuf->getRawLong32();
    drw_dbg("\nsecurity flags= "); drw_dbg(securityFlags);
    // UNKNOWN SECTION 4 bytes
    duint32 uk =    fileBuf->getRawLong32();
    drw_dbg("\nUNKNOWN SECTION ( 4 bytes) = "); drw_dbg(uk);
    duint32 sumInfoAddr =    fileBuf->getRawLong32();
    drw_dbg("\nsummary Info Address= "); drw_dbg(sumInfoAddr);
    duint32 vbaAdd =    fileBuf->getRawLong32();
    drw_dbg("\nVBA address= "); drw_dbgh(vbaAdd);
    drw_dbg("\npos 0x28 are 0x00000080= "); drw_dbgh(fileBuf->getRawLong32());
     drw_dbg("\n");
    return true;
}

bool dwgReader18::readFileHeader() {

    if (! fileBuf->setPosition(0x80))
        return false;

//    genMagicNumber(); DBG("\n"); DBG("\n");
    drw_dbg("Encrypted Header Data=\n");
    duint8 byteStr[0x6C];
    int size =0x6C;
    for (int i=0, j=0; i< 0x6C;i++) {
        duint8 ch = fileBuf->getRawChar8();
        drw_dbgh(ch);
        if (j == 15) {
            drw_dbg("\n");
            j = 0;
        } else {
            drw_dbg(", ");
            j++;
        }
        byteStr[i] = DRW_magicNum18[i] ^ ch;
    }
    drw_dbg("\n");

//    size =0x6C;
    drw_dbg("Decrypted Header Data=\n");
    for (int i=0, j = 0; i< size;i++) {
        drw_dbgh( static_cast<unsigned char>(byteStr[i]));
        if (j == 15) {
            drw_dbg("\n");
            j = 0;
        } else {
            drw_dbg(", ");
            j++;
        }
    }
    dwgBuffer buff(byteStr, 0x6C, &decoder);
    std::string name = reinterpret_cast<char*>(byteStr);
    drw_dbg("\nFile ID string (AcFssFcAJMB)= "); drw_dbg(name.c_str());
    //ID string + NULL = 12
    buff.setPosition(12);
    drw_dbg("\n0x00 long= "); drw_dbgh(buff.getRawLong32());
    drw_dbg("\n0x6c long= "); drw_dbgh(buff.getRawLong32());
    drw_dbg("\n0x04 long= "); drw_dbgh(buff.getRawLong32());
    drw_dbg("\nRoot tree node gap= "); drw_dbgh(buff.getRawLong32());
    drw_dbg("\nLowermost left tree node gap= "); drw_dbgh(buff.getRawLong32());
    drw_dbg("\nLowermost right tree node gap= "); drw_dbgh(buff.getRawLong32());
    drw_dbg("\nUnknown long (1)= "); drw_dbgh(buff.getRawLong32());
    drw_dbg("\nLast section page Id= "); drw_dbgh(buff.getRawLong32());
    drw_dbg("\nLast section page end address 64b= "); drw_dbgh(buff.getRawLong64());
    drw_dbg("\nStart of second header data address 64b= "); drw_dbgh(buff.getRawLong64());
    drw_dbg("\nGap amount= "); drw_dbgh(buff.getRawLong32());
    drw_dbg("\nSection page amount= "); drw_dbgh(buff.getRawLong32());
    drw_dbg("\n0x20 long= "); drw_dbgh(buff.getRawLong32());
    drw_dbg("\n0x80 long= "); drw_dbgh(buff.getRawLong32());
    drw_dbg("\n0x40 long= "); drw_dbgh(buff.getRawLong32());
    dint32 secPageMapId = buff.getRawLong32();
    drw_dbg("\nSection Page Map Id= "); drw_dbgh(secPageMapId);
    duint64 secPageMapAddr = buff.getRawLong64()+0x100;
    drw_dbg("\nSection Page Map address 64b= "); drw_dbgh(secPageMapAddr);
    drw_dbg("\nSection Page Map address 64b dec= "); drw_dbg(secPageMapAddr);
    duint32 secMapId = buff.getRawLong32();
    drw_dbg("\nSection Map Id= "); drw_dbgh(secMapId);
    drw_dbg("\nSection page array size= "); drw_dbgh(buff.getRawLong32());
    drw_dbg("\nGap array size= "); drw_dbgh(buff.getRawLong32());
    //TODO: verify CRC
    drw_dbg("\nCRC32= "); drw_dbgh(buff.getRawLong32());
    for (duint8 i = 0x68; i < 0x6c; ++i)
        byteStr[i] = '\0';
//    byteStr[i] = '\0';
    duint32 crcCalc = buff.crc32(0x00,0,0x6C);
    drw_dbg("\nCRC32 calculated= "); drw_dbgh(crcCalc);

    drw_dbg("\nEnd Encrypted Data. Reads 0x14 bytes, equal to magic number:\n");
    for (int i=0, j=0; i< 0x14;i++) {
        drw_dbg("magic num: "); drw_dbgh( static_cast<unsigned char>(DRW_magicNumEnd18[i]));
        drw_dbg(",read "); drw_dbgh( static_cast<unsigned char>(fileBuf->getRawChar8()));
        if (j == 3) {
            drw_dbg("\n");
            j = 0;
        } else {
            drw_dbg(", ");
            j++;
        }
    }
// At this point are parsed the first 256 bytes
    drw_dbg("\nJump to Section Page Map address: "); drw_dbgh(secPageMapAddr);

    if (! fileBuf->setPosition(secPageMapAddr))
        return false;
    duint32 pageType = fileBuf->getRawLong32();
    drw_dbg("\nSection page type= "); drw_dbgh(pageType);
    duint32 decompSize = fileBuf->getRawLong32();
    drw_dbg("\nDecompressed size= "); drw_dbg(decompSize); drw_dbg(", "); drw_dbgh(decompSize);
    if (pageType != 0x41630e3b){
        //bad page type, ends
        drw_dbg("Warning, bad page type, was expected 0x41630e3b instead of");  drw_dbgh(pageType); drw_dbg("\n");
        return false;
    }
    std::vector<duint8> tmpDecompSec(decompSize);
    if (!parseSysPage(tmpDecompSec.data(), decompSize)) {
        return false;
    }

//parses "Section page map" decompressed data
    dwgBuffer buff2(tmpDecompSec.data(), decompSize, &decoder);
    duint32 address = 0x100;
    //stores temporarily info of all pages:
    std::unordered_map<duint64, dwgPageInfo >sectionPageMapTmp;

    for (unsigned int i = 0; i < decompSize;) {
        dint32 id = buff2.getRawLong32();//RLZ bad can be +/-
        duint32 size = buff2.getRawLong32();
        i += 8;
        drw_dbg("Page num= "); drw_dbg(id); drw_dbg(" size= "); drw_dbgh(size);
        drw_dbg(" address= "); drw_dbgh(address);  drw_dbg("\n");
        //TODO num can be negative indicating gap
//        duint64 ind = id > 0 ? id : -id;
        if (id < 0){
            drw_dbg("Parent= "); drw_dbg(buff2.getRawLong32());
            drw_dbg("\nLeft= "); drw_dbg(buff2.getRawLong32());
            drw_dbg(", Right= "); drw_dbg(buff2.getRawLong32());
            drw_dbg(", 0x00= ");drw_dbgh(buff2.getRawLong32()); drw_dbg("\n");
            i += 16;
        }

        sectionPageMapTmp[id] = dwgPageInfo(id, address, size);
        address += size;
    }

    drw_dbg("\n*** dwgReader18: Processing Data Section Map ***\n");
    dwgPageInfo sectionMap = sectionPageMapTmp[secMapId];
    if (!fileBuf->setPosition(sectionMap.address))
        return false;
    pageType = fileBuf->getRawLong32();
    drw_dbg("\nSection page type= "); drw_dbgh(pageType);
    decompSize = fileBuf->getRawLong32();
    drw_dbg("\nDecompressed size= "); drw_dbg(decompSize); drw_dbg(", "); drw_dbgh(decompSize);
    if (pageType != 0x4163003b){
        //bad page type, ends
        drw_dbg("Warning, bad page type, was expected 0x4163003b instead of");  drw_dbgh(pageType); drw_dbg("\n");
        return false;
    }
    tmpDecompSec.resize(decompSize);
    if (!parseSysPage(tmpDecompSec.data(), decompSize)) {
        return false;
    }

//reads sections:
    drw_dbg("\n*** dwgReader18: reads sections:");
    dwgBuffer buff3(tmpDecompSec.data(), decompSize, &decoder);
    duint32 numDescriptions = buff3.getRawLong32();
    drw_dbg("\nnumDescriptions (sections)= "); drw_dbg(numDescriptions);
    drw_dbg("\n0x02 long= "); drw_dbgh(buff3.getRawLong32());
    drw_dbg("\n0x00007400 long= "); drw_dbgh(buff3.getRawLong32());
    drw_dbg("\n0x00 long= "); drw_dbgh(buff3.getRawLong32());
    drw_dbg("\nunknown long (numDescriptions?)= "); drw_dbg(buff3.getRawLong32()); drw_dbg("\n");

    for (unsigned int i = 0; i < numDescriptions; i++) {
        dwgSectionInfo secInfo;
        secInfo.size = buff3.getRawLong64();
        drw_dbg("\nSize of section= "); drw_dbgh(secInfo.size);
        secInfo.pageCount = buff3.getRawLong32();
        drw_dbg("\nPage count= "); drw_dbgh(secInfo.pageCount);
        secInfo.maxSize = buff3.getRawLong32();
        drw_dbg("\nMax Decompressed Size= "); drw_dbgh(secInfo.maxSize);
        drw_dbg("\nunknown long= "); drw_dbgh(buff3.getRawLong32());
        secInfo.compressed = buff3.getRawLong32();
        drw_dbg("\nis Compressed? 1:no, 2:yes= "); drw_dbgh(secInfo.compressed);
        secInfo.Id = buff3.getRawLong32();
        drw_dbg("\nSection Id= "); drw_dbgh(secInfo.Id);
        secInfo.encrypted = buff3.getRawLong32();
        //encrypted (doc: 0 no, 1 yes, 2 unkn) on read: objects 0 and encrypted yes
        drw_dbg("\nEncrypted= "); drw_dbgh(secInfo.encrypted);
        duint8 nameCStr[64];
        buff3.getBytes(nameCStr, 64);
        secInfo.name = reinterpret_cast<char*>(nameCStr);
        drw_dbg("\nSection std::Name= "); drw_dbg( secInfo.name.c_str() ); drw_dbg("\n");
        for (unsigned int i = 0; i < secInfo.pageCount; i++){
            duint32 pn = buff3.getRawLong32();
            dwgPageInfo pi = sectionPageMapTmp[pn]; //get a copy
            drw_dbg(" reading pag num = "); drw_dbgh(pn);
            pi.dataSize = buff3.getRawLong32();
            pi.startOffset = buff3.getRawLong64();
            secInfo.pages[pn]= pi;//complete copy in secInfo
            drw_dbg("\n    Page number= "); drw_dbgh(secInfo.pages[pn].Id);
            drw_dbg("\n    size in file= "); drw_dbgh(secInfo.pages[pn].size);
            drw_dbg("\n    address in file= "); drw_dbgh(secInfo.pages[pn].address);
            drw_dbg("\n    Data size= "); drw_dbgh(secInfo.pages[pn].dataSize);
            drw_dbg("\n    Start offset= "); drw_dbgh(secInfo.pages[pn].startOffset); drw_dbg("\n");
        }
        //do not save empty section
        if (!secInfo.name.empty()) {
            drw_dbg("Saved section Name= "); drw_dbg( secInfo.name.c_str() ); drw_dbg("\n");
            sections[secEnum::getEnum(secInfo.name)] = secInfo;
        }
    }

    if (! fileBuf->isGood())
        return false;
    drw_dbg("\ndwgReader18::readFileHeader END\n\n");
    return true;
}

bool dwgReader18::readDwgHeader(DRW_Header& hdr){
    drw_dbg("dwgReader18::readDwgHeader\n");
    dwgSectionInfo si = sections[secEnum::HEADER];
    if (si.Id<0)//not found, ends
        return false;
    bool ret = parseDataPage(si/*, objData*/);
    //global store for uncompressed data of all pages
    uncompSize=si.size;
    if (ret) {
        dwgBuffer dataBuf(objData.get(), si.size, &decoder);
        drw_dbg("Header section sentinel= ");
        checkSentinel(&dataBuf, secEnum::HEADER, true);
        if (version == DRW::AC1018){
            ret = dwgReader::readDwgHeader(hdr, &dataBuf, &dataBuf);
        } else {
            dwgBuffer handleBuf(objData.get(), si.size, &decoder);
            ret = dwgReader::readDwgHeader(hdr, &dataBuf, &handleBuf);
        }
    }
    //Cleanup: global store for uncompressed data of all pages
    objData.reset();
    return ret;
}


bool dwgReader18::readDwgClasses(){
    drw_dbg("\ndwgReader18::readDwgClasses\n");
    dwgSectionInfo si = sections[secEnum::CLASSES];
    if (si.Id < 0) //not found, ends
        return false;
    if (!parseDataPage(si/*, objData*/)) {
        return false;
    }

    //global store for uncompressed data of all pages
    uncompSize=si.size;
    dwgBuffer dataBuf(objData.get(), uncompSize, &decoder);

    drw_dbg("classes section sentinel= ");
    checkSentinel(&dataBuf, secEnum::CLASSES, true);

    duint32 size = dataBuf.getRawLong32();
    drw_dbg("\ndata size in bytes "); drw_dbg(size);
    if (((version == DRW::AC1021 || version == DRW::AC1027 ) && maintenanceVersion > 3) || version >= DRW::AC1032 ) { //2010+
        duint32 hSize = dataBuf.getRawLong32();
        drw_dbg("\n2010+ & MV> 3, height 32b: "); drw_dbg(hSize);
    }
    duint32 bitSize = 0;
    if (version > DRW::AC1021) {//2007+
        bitSize = dataBuf.getRawLong32();
        drw_dbg("\ntotal size in bits "); drw_dbg(bitSize);
    }
    duint32 maxClassNum = dataBuf.getBitShort();
    drw_dbg("\nMaximum class number "); drw_dbg(maxClassNum);
    drw_dbg("\nRc 1 "); drw_dbg(dataBuf.getRawChar8());
    drw_dbg("\nRc 2 "); drw_dbg(dataBuf.getRawChar8());
    drw_dbg("\nBit "); drw_dbg(dataBuf.getBit());

    /*******************************/
    dwgBuffer *strBuf = &dataBuf;
    dwgBuffer strBuff(objData.get(), uncompSize, &decoder);
    //prepare string stream for 2007+
    if (version > DRW::AC1021) {//2007+
        strBuf = &strBuff;
        duint32 strStartPos = bitSize+191;//size in bits + 24 bytes (sn+size+hSize) - 1 bit (endbit)
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
    }

    /*******************************/

    duint32 endDataPos = maxClassNum-499;
    drw_dbg("\nbuff.getPosition: "); drw_dbg(dataBuf.getPosition());
    for (duint32 i= 0; i<endDataPos;i++) {
        DRW_Class *cl = new DRW_Class();
        cl->parseDwg(version, &dataBuf, strBuf);
        classesmap[cl->classNum] = cl;
        drw_dbg("\nbuff.getPosition: "); drw_dbg(dataBuf.getPosition());
    }
    drw_dbg("\nend classes data buff.getPosition: "); drw_dbg(dataBuf.getPosition());
    drw_dbg("\nend classes data buff.getBitPos: "); drw_dbg(dataBuf.getBitPos());
    drw_dbg("\nend classes strings buff.getPosition: "); drw_dbg(strBuf->getPosition());
    drw_dbg("\nend classes strings buff.getBitPos: "); drw_dbg(strBuf->getBitPos());

/***************/

    strBuf->setPosition(strBuf->getPosition()+1);//skip remaining bits
    drw_dbg("\nCRC: "); drw_dbgh(strBuf->getRawShort16());
    if (version > DRW::AC1018){
        drw_dbg("\nunknown CRC: "); drw_dbgh(strBuf->getRawShort16());
    }
    drw_dbg("\nclasses section end sentinel= ");
    checkSentinel(strBuf, secEnum::CLASSES, false);

    //Cleanup: global store for uncompressed data of all pages
    objData.reset();

    return strBuf->isGood();
}


/*********** objects map ************************/
/** Note: object map are split in sections with max size 2035?
 *  heach section are 2 bytes size + data bytes + 2 bytes crc
 *  size value are data bytes + 2 and to calculate crc are used
 *  2 bytes size + data bytes
 *  last section are 2 bytes size + 2 bytes crc (size value always 2)
**/
bool dwgReader18::readDwgHandles() {
    drw_dbg("\ndwgReader18::readDwgHandles\n");
    dwgSectionInfo si = sections[secEnum::HANDLES];
    if (si.Id<0)//not found, ends
        return false;

    if (!parseDataPage(si)) {
        return false;
    }

    //global store for uncompressed data of all pages
    uncompSize=si.size;
    dwgBuffer dataBuf(objData.get(), uncompSize, &decoder);

    bool ret {dwgReader::readDwgHandles(&dataBuf, 0, si.size)};

    //Cleanup: global store for uncompressed data of all pages
    if (objData){
        objData.reset();
        uncompSize = 0;
    }

    return ret;
}


/*********** objects ************************/
/**
 * Reads all the object referenced in the object map section of the DWG file
 * (using their object file offsets)
 */
bool dwgReader18::readDwgTables(DRW_Header& hdr) {
    drw_dbg("\ndwgReader18::readDwgTables\n");
    dwgSectionInfo si = sections[secEnum::OBJECTS];

    if (si.Id < 0   //not found, ends
        || !parseDataPage( si/*, objData*/)) {
        return false;
    }

    //global store for uncompressed data of all pages
    uncompSize=si.size;
    dwgBuffer dataBuf(objData.get(), uncompSize, &decoder);

    //Do not delete objData in this point, needed in the remaining code

    return dwgReader::readDwgTables(hdr, &dataBuf);
}
