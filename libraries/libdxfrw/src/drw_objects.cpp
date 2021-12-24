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

#include <cmath>
#include "drw_objects.h"
#include "intern/dwgutil.h"
#include "intern/dxfreader.h"
#include "intern/dwgbuffer.h"
#include "intern/drw_dbg.h"

//! Base class for tables entries
/*!
*  Base class for tables entries
*  @author Rallaz
*/
void DRW_TableEntry::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 5:
        handle = reader->getHandleString();
        break;
    case 330:
        parentHandle = reader->getHandleString();
        break;
    case 2:
        name = reader->getUtf8String();
        break;
    case 70:
        flags = reader->getInt32();
        break;
    case 1000:
    case 1001:
    case 1002:
    case 1003:
    case 1004:
    case 1005:
        extData.push_back(new DRW_Variant(code, reader->getString()));
        break;
    case 1010:
    case 1011:
    case 1012:
    case 1013:
        // don't trust in X, Y, Z order!
        if (nullptr != curr) {
            curr->setCoordX( reader->getDouble());
        }
        else {
            curr = new DRW_Variant( code, DRW_Coord( reader->getDouble(), 0.0, 0.0));
            extData.push_back(curr);
        }
        break;
    case 1020:
    case 1021:
    case 1022:
    case 1023:
        // don't trust in X, Y, Z order!
        if (nullptr != curr) {
            curr->setCoordY( reader->getDouble());
        }
        else {
            curr = new DRW_Variant( code, DRW_Coord( 0.0, reader->getDouble(), 0.0));
            extData.push_back(curr);
        }
        break;
    case 1030:
    case 1031:
    case 1032:
    case 1033:
        // don't trust in X, Y, Z order!
        if (nullptr != curr) {
            curr->setCoordZ( reader->getDouble());
        }
        else {
            curr = new DRW_Variant( code, DRW_Coord( 0.0, 0.0, reader->getDouble()));
            extData.push_back(curr);
        }
        break;
    case 1040:
    case 1041:
    case 1042:
        extData.push_back(new DRW_Variant(code, reader->getDouble()));
        break;
    case 1070:
    case 1071:
        extData.push_back(new DRW_Variant(code, reader->getInt32() ));
        break;
    default:
        break;
    }
}

bool DRW_TableEntry::parseDwg(DRW::Version version, dwgBuffer *buf, dwgBuffer *strBuf, uint32_t bs){
    drw_dbg("\n***************************** parsing table entry *********************************************\n");
    objSize=0;
    oType = buf->getObjType(version);
    drw_dbg("Object type: "); drw_dbg(oType); drw_dbg(", "); drw_dbgh(oType);
    if (version > DRW::AC1014 && version < DRW::AC1024) {//2000 to 2007
        objSize = buf->getRawLong32();  //RL 32bits object size in bits
        drw_dbg(" Object size: "); drw_dbg(objSize); drw_dbg("\n");
    }
    if (version > DRW::AC1021) {//2010+
        uint32_t ms = buf->size();
        objSize = ms*8 - bs;
        drw_dbg(" Object size: "); drw_dbg(objSize); drw_dbg("\n");
    }
    if (strBuf != nullptr && version > DRW::AC1018) {//2007+
        strBuf->moveBitPos(objSize-1);
        drw_dbg(" strBuf strbit pos 2007: "); drw_dbg(strBuf->getPosition()); drw_dbg(" strBuf bpos 2007: "); drw_dbg(strBuf->getBitPos()); drw_dbg("\n");
        if (strBuf->getBit() == 1){
            drw_dbg("DRW_TableEntry::parseDwg string bit is 1\n");
            strBuf->moveBitPos(-17);
            uint16_t strDataSize = strBuf->getRawShort16();
            drw_dbg("\nDRW_TableEntry::parseDwg string strDataSize: "); drw_dbgh(strDataSize); drw_dbg("\n");
            if ( (strDataSize& 0x8000) == 0x8000){
                drw_dbg("\nDRW_TableEntry::parseDwg string 0x8000 bit is set");
                strBuf->moveBitPos(-33);//RLZ pending to verify
                uint16_t hiSize = strBuf->getRawShort16();
                strDataSize = ((strDataSize&0x7fff) | (hiSize<<15));
            }
            strBuf->moveBitPos( -strDataSize -16); //-14
            drw_dbg("strBuf start strDataSize pos 2007: "); drw_dbg(strBuf->getPosition()); drw_dbg(" strBuf bpos 2007: "); drw_dbg(strBuf->getBitPos()); drw_dbg("\n");
        } else
            drw_dbg("\nDRW_TableEntry::parseDwg string bit is 0");
        drw_dbg("strBuf start pos 2007: "); drw_dbg(strBuf->getPosition()); drw_dbg(" strBuf bpos 2007: "); drw_dbg(strBuf->getBitPos()); drw_dbg("\n");
    }

    dwgHandle ho = buf->getHandle();
    handle = ho.ref;
    drw_dbg("TableEntry Handle: "); drw_dbghl(ho.code, ho.size, ho.ref);
    int16_t extDataSize = buf->getBitShort(); //BS
    drw_dbg(" ext data size: "); drw_dbg(extDataSize);
    while (extDataSize>0 && buf->isGood()) {
        /* RLZ: TODO */
        dwgHandle ah = buf->getHandle();
        drw_dbg("App Handle: "); drw_dbghl(ah.code, ah.size, ah.ref);
        uint8_t *tmpExtData = new uint8_t[extDataSize];
        buf->getBytes(tmpExtData, extDataSize);
        dwgBuffer tmpExtDataBuf(tmpExtData, extDataSize, buf->decoder);
        int pos = tmpExtDataBuf.getPosition();
        int bpos = tmpExtDataBuf.getBitPos();
        drw_dbg("ext data pos: "); drw_dbg(pos); drw_dbg("."); drw_dbg(bpos); drw_dbg("\n");
        uint8_t dxfCode = tmpExtDataBuf.getRawChar8();
        drw_dbg(" dxfCode: "); drw_dbg(dxfCode);
        switch (dxfCode){
        case 0:{
            uint8_t strLength = tmpExtDataBuf.getRawChar8();
            drw_dbg(" strLength: "); drw_dbg(strLength);
            uint16_t cp = tmpExtDataBuf.getBERawShort16();
            drw_dbg(" str codepage: "); drw_dbg(cp);
            for (int i=0;i< strLength+1;i++) {//string length + null terminating char
                uint8_t dxfChar = tmpExtDataBuf.getRawChar8();
                drw_dbg(" dxfChar: "); drw_dbg(dxfChar);
            }
            break;
        }
        default:
            /* RLZ: TODO */
            break;
        }
        drw_dbg("ext data pos: "); drw_dbg(tmpExtDataBuf.getPosition()); drw_dbg("."); drw_dbg(tmpExtDataBuf.getBitPos()); drw_dbg("\n");
        delete[]tmpExtData;
        extDataSize = buf->getBitShort(); //BS
        drw_dbg(" ext data size: "); drw_dbg(extDataSize);
    } //end parsing extData (EED)
    if (version < DRW::AC1015) {//14-
        objSize = buf->getRawLong32();  //RL 32bits size in bits
    }
    drw_dbg(" objSize in bits: "); drw_dbg(objSize);

    numReactors = buf->getBitLong(); //BL
    drw_dbg(", numReactors: "); drw_dbg(numReactors); drw_dbg("\n");
    if (version > DRW::AC1015) {//2004+
        xDictFlag = buf->getBit();
        drw_dbg("xDictFlag: "); drw_dbg(xDictFlag);
    }
    if (version > DRW::AC1024) {//2013+
        uint8_t bd = buf->getBit();
        drw_dbg(" Have binary data: "); drw_dbg(bd); drw_dbg("\n");
    }
    return buf->isGood();
}

//! Class to handle dimstyle entries
/*!
*  Class to handle ldim style symbol table entries
*  @author Rallaz
*/
void DRW_Dimstyle::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 105:
        handle = reader->getHandleString();
        break;
    case 3:
        dimpost = reader->getUtf8String();
        break;
    case 4:
        dimapost = reader->getUtf8String();
        break;
    case 5:
        dimblk = reader->getUtf8String();
        break;
    case 6:
        dimblk1 = reader->getUtf8String();
        break;
    case 7:
        dimblk2 = reader->getUtf8String();
        break;
    case 40:
        dimscale = reader->getDouble();
        break;
    case 41:
        dimasz = reader->getDouble();
        break;
    case 42:
        dimexo = reader->getDouble();
        break;
    case 43:
        dimdli = reader->getDouble();
        break;
    case 44:
        dimexe = reader->getDouble();
        break;
    case 45:
        dimrnd = reader->getDouble();
        break;
    case 46:
        dimdle = reader->getDouble();
        break;
    case 47:
        dimtp = reader->getDouble();
        break;
    case 48:
        dimtm = reader->getDouble();
        break;
    case 49:
        dimfxl = reader->getDouble();
        break;
    case 140:
        dimtxt = reader->getDouble();
        break;
    case 141:
        dimcen = reader->getDouble();
        break;
    case 142:
        dimtsz = reader->getDouble();
        break;
    case 143:
        dimaltf = reader->getDouble();
        break;
    case 144:
        dimlfac = reader->getDouble();
        break;
    case 145:
        dimtvp = reader->getDouble();
        break;
    case 146:
        dimtfac = reader->getDouble();
        break;
    case 147:
        dimgap = reader->getDouble();
        break;
    case 148:
        dimaltrnd = reader->getDouble();
        break;
    case 71:
        dimtol = reader->getInt32();
        break;
    case 72:
        dimlim = reader->getInt32();
        break;
    case 73:
        dimtih = reader->getInt32();
        break;
    case 74:
        dimtoh = reader->getInt32();
        break;
    case 75:
        dimse1 = reader->getInt32();
        break;
    case 76:
        dimse2 = reader->getInt32();
        break;
    case 77:
        dimtad = reader->getInt32();
        break;
    case 78:
        dimzin = reader->getInt32();
        break;
    case 79:
        dimazin = reader->getInt32();
        break;
    case 170:
        dimalt = reader->getInt32();
        break;
    case 171:
        dimaltd = reader->getInt32();
        break;
    case 172:
        dimtofl = reader->getInt32();
        break;
    case 173:
        dimsah = reader->getInt32();
        break;
    case 174:
        dimtix = reader->getInt32();
        break;
    case 175:
        dimsoxd = reader->getInt32();
        break;
    case 176:
        dimclrd = reader->getInt32();
        break;
    case 177:
        dimclre = reader->getInt32();
        break;
    case 178:
        dimclrt = reader->getInt32();
        break;
    case 179:
        dimadec = reader->getInt32();
        break;
    case 270:
        dimunit = reader->getInt32();
        break;
    case 271:
        dimdec = reader->getInt32();
        break;
    case 272:
        dimtdec = reader->getInt32();
        break;
    case 273:
        dimaltu = reader->getInt32();
        break;
    case 274:
        dimalttd = reader->getInt32();
        break;
    case 275:
        dimaunit = reader->getInt32();
        break;
    case 276:
        dimfrac = reader->getInt32();
        break;
    case 277:
        dimlunit = reader->getInt32();
        break;
    case 278:
        dimdsep = reader->getInt32();
        break;
    case 279:
        dimtmove = reader->getInt32();
        break;
    case 280:
        dimjust = reader->getInt32();
        break;
    case 281:
        dimsd1 = reader->getInt32();
        break;
    case 282:
        dimsd2 = reader->getInt32();
        break;
    case 283:
        dimtolj = reader->getInt32();
        break;
    case 284:
        dimtzin = reader->getInt32();
        break;
    case 285:
        dimaltz = reader->getInt32();
        break;
    case 286:
        dimaltttz = reader->getInt32();
        break;
    case 287:
        dimfit = reader->getInt32();
        break;
    case 288:
        dimupt = reader->getInt32();
        break;
    case 289:
        dimatfit = reader->getInt32();
        break;
    case 290:
        dimfxlon = reader->getInt32();
        break;
    case 340:
        dimtxsty = reader->getUtf8String();
        break;
    case 341:
        dimldrblk = reader->getUtf8String();
        break;
    case 342:
        dimblk = reader->getUtf8String();
        break;
    case 343:
        dimblk1 = reader->getUtf8String();
        break;
    case 344:
        dimblk2 = reader->getUtf8String();
        break;
    default:
        DRW_TableEntry::parseCode(code, reader);
        break;
    }
}

bool DRW_Dimstyle::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    drw_dbg("\n***************************** parsing dimension style **************************************\n");
    if (!ret)
        return ret;
    name = sBuf->getVariableText(version, false);
    drw_dbg("dimension style name: "); drw_dbg(name.c_str()); drw_dbg("\n");

//    handleObj = shpControlH.ref;
    drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    //    RS crc;   //RS */
    return buf->isGood();
}


//! Class to handle line type entries
/*!
*  Class to handle line type symbol table entries
*  @author Rallaz
*/
void DRW_LType::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 3:
        desc = reader->getUtf8String();
        break;
    case 73:
        size = reader->getInt32();
        path.clear();
        path.reserve(size);
        break;
    case 40:
        length = reader->getDouble();
        break;
    case 49:
        path.push_back(reader->getDouble());
        pathIdx++;
        break;
/*    case 74:
        haveShape = reader->getInt32();
        break;*/
    default:
        DRW_TableEntry::parseCode(code, reader);
        break;
    }
}

//! Update line type
/*!
*  Update the size and length of line type according to the path
*  @author Rallaz
*/
/*TODO: control max length permited */
void DRW_LType::update(){
    double d =0;
    size = path.size();
    for (int i = 0;  i< size; i++){
        d += fabs(path.at(i));
    }
    length = d;
}

bool DRW_LType::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    drw_dbg("\n***************************** parsing line type *********************************************\n");
    if (!ret)
        return ret;
    name = sBuf->getVariableText(version, false);
    drw_dbg("linetype name: "); drw_dbg(name.c_str()); drw_dbg("\n");
    flags = buf->getBit()<< 6;
    drw_dbg("flags: "); drw_dbg(flags);
    if (version > DRW::AC1018) {//2007+
    } else {//2004- //RLZ: verify in 2004, 2010 &2013
        int16_t xrefindex = buf->getBitShort();
        drw_dbg(" xrefindex: "); drw_dbg(xrefindex);
    }
    uint8_t xdep = buf->getBit();
    drw_dbg(" xdep: "); drw_dbg(xdep);
    flags |= xdep<< 4;
    drw_dbg(" flags: "); drw_dbg(flags);
    desc = sBuf->getVariableText(version, false);
    drw_dbg(" desc: "); drw_dbg(desc.c_str());
    length = buf->getBitDouble();
    drw_dbg(" pattern length: "); drw_dbg(length);
    char align = buf->getRawChar8();
    drw_dbg(" align: "); drw_dbg(std::string(&align, 1));
    size = buf->getRawChar8();
    drw_dbg(" num dashes, size: "); drw_dbg(size);
    drw_dbg("\n    dashes:\n");
    bool haveStrArea = false;
    for (int i=0; i< size; i++){
        path.push_back(buf->getBitDouble());
        /*int bs1 =*/ buf->getBitShort();
        /*double d1= */buf->getRawDouble();
        /*double d2=*/ buf->getRawDouble();
        /*double d3= */buf->getBitDouble();
        /*double d4= */buf->getBitDouble();
        int bs2 = buf->getBitShort();
        if((bs2 & 2) !=0) haveStrArea = true;
    }
    for (unsigned i=0; i<path.size() ; i++){
        drw_dbg(", "); drw_dbg(path[i]); drw_dbg("\n");
    }
    drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    if (version < DRW::AC1021) { //2004-
        uint8_t strarea[256];
        buf->getBytes(strarea, 256);
        drw_dbg("string area 256 bytes:\n"); drw_dbg(reinterpret_cast<char*>(strarea)); drw_dbg("\n");
    } else { //2007+
        //first verify flag
        if (haveStrArea) {
            uint8_t strarea[512];
            buf->getBytes(strarea, 512);
            drw_dbg("string area 512 bytes:\n"); drw_dbg(reinterpret_cast<char*>(strarea)); drw_dbg("\n");
        } else
            drw_dbg("string area 512 bytes not present\n");
    }

    if (version > DRW::AC1021) {//2007+ skip string area
        drw_dbg(" ltype end of object data pos 2010: "); drw_dbg(buf->getPosition()); drw_dbg(" strBuf bpos 2007: "); drw_dbg(buf->getBitPos()); drw_dbg("\n");
    }
    if (version > DRW::AC1018) {//2007+ skip string area
        buf->setPosition(objSize >> 3);
        buf->setBitPos(objSize & 7);
    }

    if (version > DRW::AC1021) {//2007+ skip string area
        drw_dbg(" ltype start of handles data pos 2010: "); drw_dbg(buf->getPosition()); drw_dbg(" strBuf bpos 2007: "); drw_dbg(buf->getBitPos()); drw_dbg("\n");
    }

    drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    dwgHandle ltControlH = buf->getHandle();
    drw_dbg("linetype control Handle: "); drw_dbghl(ltControlH.code, ltControlH.size, ltControlH.ref);
    parentHandle = ltControlH.ref;
    drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    for (int i=0; i< numReactors;++i) {
        dwgHandle reactorsH = buf->getHandle();
        drw_dbg(" reactorsH control Handle: "); drw_dbghl(reactorsH.code, reactorsH.size, reactorsH.ref); drw_dbg("\n");
    }
    if (xDictFlag !=1){//linetype in 2004 seems not have XDicObjH or NULL handle
        dwgHandle XDicObjH = buf->getHandle();
        drw_dbg(" XDicObj control Handle: "); drw_dbghl(XDicObjH.code, XDicObjH.size, XDicObjH.ref); drw_dbg("\n");
        drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    }
    if(size>0){
        dwgHandle XRefH = buf->getHandle();
        drw_dbg(" XRefH control Handle: "); drw_dbghl(XRefH.code, XRefH.size, XRefH.ref); drw_dbg("\n");
        dwgHandle shpHandle = buf->getHandle();
        drw_dbg(" shapeFile Handle: "); drw_dbghl(shpHandle.code, shpHandle.size, shpHandle.ref);
        drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    }
    dwgHandle shpHandle = buf->getHandle();
    drw_dbg(" shapeFile +1 Handle ??: "); drw_dbghl(shpHandle.code, shpHandle.size, shpHandle.ref); drw_dbg("\n");

    drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
//    RS crc;   //RS */
    return buf->isGood();
}

//! Class to handle layer entries
/*!
*  Class to handle layer symbol table entries
*  @author Rallaz
*/
void DRW_Layer::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 6:
        lineType = reader->getUtf8String();
        break;
    case 62:
        color = reader->getInt32();
        break;
    case 290:
        plotF = reader->getBool();
        break;
    case 370:
        lWeight = DRW_LW_Conv::dxfInt2lineWidth(reader->getInt32());
        break;
    case 390:
        handlePlotS = reader->getString();
        break;
    case 347:
        handleMaterialS = reader->getString();
        break;
    case 420:
        color24 = reader->getInt32();
        break;
    default:
        DRW_TableEntry::parseCode(code, reader);
        break;
    }
}

bool DRW_Layer::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    drw_dbg("\n***************************** parsing layer *********************************************\n");
    if (!ret)
        return ret;
    name = sBuf->getVariableText(version, false);
    drw_dbg("layer name: "); drw_dbg(name);

    flags |= buf->getBit()<< 6;//layer have entity
    if (version < DRW::AC1021) {//2004-
        drw_dbg(", xrefindex = "); drw_dbg(buf->getBitShort()); drw_dbg("\n");
        //int16_t xrefindex = buf->getBitShort();
    }
    flags |= buf->getBit() << 4;//is refx dependent
    if (version < DRW::AC1015) {//14-
        flags |= buf->getBit(); //layer frozen
        /*flags |=*/ buf->getBit(); //unused, negate the color
        flags |= buf->getBit() << 1;//frozen in new
        flags |= buf->getBit()<< 3;//locked
    }
    if (version > DRW::AC1014) {//2000+
        int16_t f = buf->getSBitShort();//bit2 are layer on
        drw_dbg(", flags 2000+: "); drw_dbg(f); drw_dbg("\n");
        flags |= f & 0x0001; //layer frozen
        flags |= ( f>> 1) & 0x0002;//frozen in new
        flags |= ( f>> 1) & 0x0004;//locked
        plotF = ( f>> 4) & 0x0001;
        lWeight = DRW_LW_Conv::dwgInt2lineWidth( (f & 0x03E0) >> 5 );
    }
    color = buf->getCmColor(version); //BS or CMC //ok for R14 or negate
    drw_dbg(", entity color: "); drw_dbg(color); drw_dbg("\n");

    if (version > DRW::AC1018) {//2007+ skip string area
        buf->setPosition(objSize >> 3);
        buf->setBitPos(objSize & 7);
    }
    dwgHandle layerControlH = buf->getHandle();
    drw_dbg("layer control Handle: "); drw_dbghl(layerControlH.code, layerControlH.size, layerControlH.ref);
    parentHandle = layerControlH.ref;

    if (xDictFlag !=1){//linetype in 2004 seems not have XDicObjH or NULL handle
        dwgHandle XDicObjH = buf->getHandle();
        drw_dbg(" XDicObj control Handle: "); drw_dbghl(XDicObjH.code, XDicObjH.size, XDicObjH.ref); drw_dbg("\n");
    }
    dwgHandle XRefH = buf->getHandle();
    drw_dbg(" XRefH control Handle: "); drw_dbghl(XRefH.code, XRefH.size, XRefH.ref); drw_dbg("\n");
    if (version > DRW::AC1014) {//2000+
        dwgHandle plotStyH = buf->getHandle();
        drw_dbg(" PLot style control Handle: "); drw_dbghl(plotStyH.code, plotStyH.size, plotStyH.ref); drw_dbg("\n");
        handlePlotS = DRW::toHexStr(plotStyH.ref);// std::string(plotStyH.ref);//RLZ: verify conversion
    }
    if (version > DRW::AC1018) {//2007+
        dwgHandle materialH = buf->getHandle();
        drw_dbg(" Material control Handle: "); drw_dbghl(materialH.code, materialH.size, materialH.ref); drw_dbg("\n");
        handleMaterialS = DRW::toHexStr(materialH.ref);//RLZ: verify conversion
    }
    //lineType handle
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    lTypeH = buf->getHandle();
    drw_dbg("line type Handle: "); drw_dbghl(lTypeH.code, lTypeH.size, lTypeH.ref);
    drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
//    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_Block_Record::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    drw_dbg("\n***************************** parsing block record ******************************************\n");
    if (!ret)
        return ret;
    uint32_t insertCount = 0;//only 2000+
    uint32_t objectCount = 0; //only 2004+

    name = sBuf->getVariableText(version, false);
    drw_dbg("block record name: "); drw_dbg(name.c_str()); drw_dbg("\n");

    flags |= buf->getBit()<< 6;//referenced external reference, block code 70, bit 7 (64)
    if (version > DRW::AC1018) {//2007+
    } else {//2004- //RLZ: verify in 2004, 2010 &2013
        int16_t xrefindex = buf->getBitShort();
        drw_dbg(" xrefindex: "); drw_dbg(xrefindex); drw_dbg("\n");
    }
    flags |= buf->getBit() << 4;//is refx dependent, block code 70, bit 5 (16)
    flags |= buf->getBit(); //if is anonymous block (*U) block code 70, bit 1 (1)
    flags |= buf->getBit() << 1; //if block contains attdefs, block code 70, bit 2 (2)
    bool blockIsXref = buf->getBit(); //if is a Xref, block code 70, bit 3 (4)
    bool xrefOverlaid = buf->getBit(); //if is a overlaid Xref, block code 70, bit 4 (8)
    flags |= blockIsXref << 2; //if is a Xref, block code 70, bit 3 (4)
    flags |= xrefOverlaid << 3; //if is a overlaid Xref, block code 70, bit 4 (8)
    if (version > DRW::AC1014) {//2000+
        flags |= buf->getBit() << 5; //if is a loaded Xref, block code 70, bit 6 (32)
    }
    drw_dbg("flags: "); drw_dbg(flags); drw_dbg(", ");
    if (version > DRW::AC1015) {//2004+ fails in 2007
        objectCount = buf->getBitLong(); //Number of objects owned by this block
        entMap.reserve(objectCount);
    }
    basePoint.x = buf->getBitDouble();
    basePoint.y = buf->getBitDouble();
    basePoint.z = buf->getBitDouble();
    drw_dbg("insertion point: "); drw_dbgpt(basePoint.x, basePoint.y, basePoint.z); drw_dbg("\n");
    std::string path = sBuf->getVariableText(version, false);
    drw_dbg("XRef path name: "); drw_dbg(path.c_str()); drw_dbg("\n");

    if (version > DRW::AC1014) {//2000+
        insertCount = 0;
        while (uint8_t i = buf->getRawChar8() != 0)
            insertCount +=i;
        std::string bkdesc = sBuf->getVariableText(version, false);
        drw_dbg("Block description: "); drw_dbg(bkdesc.c_str()); drw_dbg("\n");

        uint32_t prevData = buf->getBitLong();
        for (unsigned int j= 0; j < prevData; ++j)
            buf->getRawChar8();
    }
    if (version > DRW::AC1018) {//2007+
        uint16_t insUnits = buf->getBitShort();
        bool canExplode = buf->getBit(); //if block can be exploded
        uint8_t bkScaling = buf->getRawChar8();

       (void)insUnits;
       (void)canExplode;
       (void)bkScaling;
    }

    if (version > DRW::AC1018) {//2007+ skip string area
        buf->setPosition(objSize >> 3);
        buf->setBitPos(objSize & 7);
    }

    dwgHandle blockControlH = buf->getHandle();
    drw_dbg("block control Handle: "); drw_dbghl(blockControlH.code, blockControlH.size, blockControlH.ref); drw_dbg("\n");
    parentHandle = blockControlH.ref;

    for (int i=0; i<numReactors; i++){
        dwgHandle reactorH = buf->getHandle();
        drw_dbg(" reactor Handle #"); drw_dbg(i); drw_dbg(": "); drw_dbghl(reactorH.code, reactorH.size, reactorH.ref); drw_dbg("\n");
    }
    if (xDictFlag !=1) {//R14+ //seems present in 2000
        dwgHandle XDicObjH = buf->getHandle();
        drw_dbg(" XDicObj control Handle: "); drw_dbghl(XDicObjH.code, XDicObjH.size, XDicObjH.ref); drw_dbg("\n");
    }
    if (version != DRW::AC1021) {//2007+ XDicObjH or NullH not present
    }
    dwgHandle NullH = buf->getHandle();
    drw_dbg(" NullH control Handle: "); drw_dbghl(NullH.code, NullH.size, NullH.ref); drw_dbg("\n");
    dwgHandle blockH = buf->getOffsetHandle(handle);
    drw_dbg(" blockH Handle: "); drw_dbghl(blockH.code, blockH.size, blockH.ref); drw_dbg("\n");
    block = blockH.ref;

    if (version > DRW::AC1015) {//2004+
        for (unsigned int i=0; i< objectCount; i++){
            dwgHandle entityH = buf->getHandle();
            drw_dbg(" entityH Handle #"); drw_dbg(i); drw_dbg(": "); drw_dbghl(entityH.code, entityH.size, entityH.ref); drw_dbg("\n");
            entMap.push_back(entityH.ref);
        }
    } else {//2000-
        if(!blockIsXref && !xrefOverlaid){
            dwgHandle firstH = buf->getHandle();
            drw_dbg(" firstH entity Handle: "); drw_dbghl(firstH.code, firstH.size, firstH.ref); drw_dbg("\n");
            firstEH = firstH.ref;
            dwgHandle lastH = buf->getHandle();
            drw_dbg(" lastH entity Handle: "); drw_dbghl(lastH.code, lastH.size, lastH.ref); drw_dbg("\n");
            lastEH = lastH.ref;
        }
    }
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    dwgHandle endBlockH = buf->getOffsetHandle(handle);
    drw_dbg(" endBlockH Handle: "); drw_dbghl(endBlockH.code, endBlockH.size, endBlockH.ref); drw_dbg("\n");
    endBlock = endBlockH.ref;

    if (version > DRW::AC1014) {//2000+
        for (unsigned int i=0; i< insertCount; i++){
            dwgHandle insertsH = buf->getHandle();
            drw_dbg(" insertsH Handle #"); drw_dbg(i); drw_dbg(": "); drw_dbghl(insertsH.code, insertsH.size, insertsH.ref); drw_dbg("\n");
        }
        drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
        dwgHandle layoutH = buf->getHandle();
        drw_dbg(" layoutH Handle: "); drw_dbghl(layoutH.code, layoutH.size, layoutH.ref); drw_dbg("\n");
    }
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n\n");
//    RS crc;   //RS */
    return buf->isGood();
}

//! Class to handle text style entries
/*!
*  Class to handle text style symbol table entries
*  @author Rallaz
*/
void DRW_Textstyle::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 3:
        font = reader->getUtf8String();
        break;
    case 4:
        bigFont = reader->getUtf8String();
        break;
    case 40:
        height = reader->getDouble();
        break;
    case 41:
        width = reader->getDouble();
        break;
    case 50:
        oblique = reader->getDouble();
        break;
    case 42:
        lastHeight = reader->getDouble();
        break;
    case 71:
        genFlag = reader->getInt32();
        break;
    case 1071:
        fontFamily = reader->getInt32();
        break;
    default:
        DRW_TableEntry::parseCode(code, reader);
        break;
    }
}

bool DRW_Textstyle::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    drw_dbg("\n***************************** parsing text style *********************************************\n");
    if (!ret)
        return ret;
    name = sBuf->getVariableText(version, false);
    drw_dbg("text style name: "); drw_dbg(name.c_str()); drw_dbg("\n");
    flags |= buf->getBit()<< 6;//style are referenced for a entity, style code 70, bit 7 (64)
    /*int16_t xrefindex =*/ buf->getBitShort();
    flags |= buf->getBit() << 4; //is refx dependent, style code 70, bit 5 (16)
    flags |= buf->getBit() << 2; //vertical text, stile code 70, bit 3 (4)
    flags |= buf->getBit(); //if is a shape file instead of text, style code 70, bit 1 (1)
    height = buf->getBitDouble();
    width = buf->getBitDouble();
    oblique = buf->getBitDouble();
    genFlag = buf->getRawChar8();
    lastHeight = buf->getBitDouble();
    font = sBuf->getVariableText(version, false);
    bigFont = sBuf->getVariableText(version, false);
    if (version > DRW::AC1018) {//2007+ skip string area
        buf->setPosition(objSize >> 3);
        buf->setBitPos(objSize & 7);
    }
    dwgHandle shpControlH = buf->getHandle();
    drw_dbg(" parentControlH Handle: "); drw_dbghl(shpControlH.code, shpControlH.size, shpControlH.ref); drw_dbg("\n");
    parentHandle = shpControlH.ref;
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    if (xDictFlag !=1){//linetype in 2004 seems not have XDicObjH or NULL handle
        dwgHandle XDicObjH = buf->getHandle();
        drw_dbg(" XDicObj control Handle: "); drw_dbghl(XDicObjH.code, XDicObjH.size, XDicObjH.ref); drw_dbg("\n");
        drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    }
/*RLZ: fails verify this part*/    dwgHandle XRefH = buf->getHandle();
    drw_dbg(" XRefH control Handle: "); drw_dbghl(XRefH.code, XRefH.size, XRefH.ref); drw_dbg("\n");

    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n\n");
    //    RS crc;   //RS */
    return buf->isGood();
}

//! Class to handle vport entries
/*!
*  Class to handle vport symbol table entries
*  @author Rallaz
*/
void DRW_Vport::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 10:
        lowerLeft.x = reader->getDouble();
        break;
    case 20:
        lowerLeft.y = reader->getDouble();
        break;
    case 11:
        UpperRight.x = reader->getDouble();
        break;
    case 21:
        UpperRight.y = reader->getDouble();
        break;
    case 12:
        center.x = reader->getDouble();
        break;
    case 22:
        center.y = reader->getDouble();
        break;
    case 13:
        snapBase.x = reader->getDouble();
        break;
    case 23:
        snapBase.y = reader->getDouble();
        break;
    case 14:
        snapSpacing.x = reader->getDouble();
        break;
    case 24:
        snapSpacing.y = reader->getDouble();
        break;
    case 15:
        gridSpacing.x = reader->getDouble();
        break;
    case 25:
        gridSpacing.y = reader->getDouble();
        break;
    case 16:
        viewDir.x = reader->getDouble();
        break;
    case 26:
        viewDir.y = reader->getDouble();
        break;
    case 36:
        viewDir.z = reader->getDouble();
        break;
    case 17:
        viewTarget.x = reader->getDouble();
        break;
    case 27:
        viewTarget.y = reader->getDouble();
        break;
    case 37:
        viewTarget.z = reader->getDouble();
        break;
    case 40:
        height = reader->getDouble();
        break;
    case 41:
        ratio = reader->getDouble();
        break;
    case 42:
        lensHeight = reader->getDouble();
        break;
    case 43:
        frontClip = reader->getDouble();
        break;
    case 44:
        backClip = reader->getDouble();
        break;
    case 50:
        snapAngle = reader->getDouble();
        break;
    case 51:
        twistAngle = reader->getDouble();
        break;
    case 71:
        viewMode = reader->getInt32();
        break;
    case 72:
        circleZoom = reader->getInt32();
        break;
    case 73:
        fastZoom = reader->getInt32();
        break;
    case 74:
        ucsIcon = reader->getInt32();
        break;
    case 75:
        snap = reader->getInt32();
        break;
    case 76:
        grid = reader->getInt32();
        break;
    case 77:
        snapStyle = reader->getInt32();
        break;
    case 78:
        snapIsopair = reader->getInt32();
        break;
    default:
        DRW_TableEntry::parseCode(code, reader);
        break;
    }
}

bool DRW_Vport::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    drw_dbg("\n***************************** parsing VPort ************************************************\n");
    if (!ret)
        return ret;
    name = sBuf->getVariableText(version, false);
    drw_dbg("vport name: "); drw_dbg(name.c_str()); drw_dbg("\n");
    flags |= buf->getBit()<< 6;// code 70, bit 7 (64)
    if (version < DRW::AC1021) { //2004-
        /*int16_t xrefindex =*/ buf->getBitShort();
    }
    flags |= buf->getBit() << 4; //is refx dependent, style code 70, bit 5 (16)
    height = buf->getBitDouble();
    ratio = buf->getBitDouble();
    drw_dbg("flags: "); drw_dbg(flags); drw_dbg(" height: "); drw_dbg(height);
    drw_dbg(" ratio: "); drw_dbg(ratio);
    center = buf->get2RawDouble();
    drw_dbg("\nview center: "); drw_dbgpt(center.x, center.y, center.z);
    viewTarget.x = buf->getBitDouble();
    viewTarget.y = buf->getBitDouble();
    viewTarget.z = buf->getBitDouble();
    drw_dbg("\nview target: "); drw_dbgpt(viewTarget.x, viewTarget.y, viewTarget.z);
    viewDir.x = buf->getBitDouble();
    viewDir.y = buf->getBitDouble();
    viewDir.z = buf->getBitDouble();
    drw_dbg("\nview dir: "); drw_dbgpt(viewDir.x, viewDir.y, viewDir.z);
    twistAngle = buf->getBitDouble();
    lensHeight = buf->getBitDouble();
    frontClip = buf->getBitDouble();
    backClip = buf->getBitDouble();
    drw_dbg("\ntwistAngle: "); drw_dbg(twistAngle); drw_dbg(" lensHeight: "); drw_dbg(lensHeight);
    drw_dbg(" frontClip: "); drw_dbg(frontClip); drw_dbg(" backClip: "); drw_dbg(backClip);
    viewMode = buf->getBit(); //view mode, code 71, bit 0 (1)
    viewMode |= buf->getBit() << 1; //view mode, code 71, bit 1 (2)
    viewMode |= buf->getBit() << 2; //view mode, code 71, bit 2 (4)
    viewMode |= buf->getBit() << 4; //view mode, code 71, bit 4 (16)
    if (version > DRW::AC1014) { //2000+
        //uint8_t renderMode = buf->getRawChar8();
        drw_dbg("\n renderMode: "); drw_dbg(buf->getRawChar8());
        if (version > DRW::AC1018) { //2007+
            drw_dbg("\n use default lights: "); drw_dbg(buf->getBit());
            drw_dbg(" default lighting type: "); drw_dbg(buf->getRawChar8());
            drw_dbg(" brightness: "); drw_dbg(buf->getBitDouble());
            drw_dbg("\n contrast: "); drw_dbg(buf->getBitDouble()); drw_dbg("\n");
            drw_dbg(" ambient color CMC: "); drw_dbg(buf->getCmColor(version));
        }
    }
    lowerLeft = buf->get2RawDouble();
    drw_dbg("\nlowerLeft: "); drw_dbgpt(lowerLeft.x, lowerLeft.y, lowerLeft.z);
    UpperRight = buf->get2RawDouble();
    drw_dbg("\nUpperRight: "); drw_dbgpt(UpperRight.x, UpperRight.y, UpperRight.z);
    viewMode |= buf->getBit() << 3; //UCSFOLLOW, view mode, code 71, bit 3 (8)
    circleZoom = buf->getBitShort();
    fastZoom = buf->getBit();
    drw_dbg("\nviewMode: "); drw_dbg(viewMode); drw_dbg(" circleZoom: ");
    drw_dbg(circleZoom); drw_dbg(" fastZoom: "); drw_dbg(fastZoom);
    ucsIcon = buf->getBit(); //ucs Icon, code 74, bit 0 (1)
    ucsIcon |= buf->getBit() << 1; //ucs Icon, code 74, bit 1 (2)
    grid = buf->getBit();
    drw_dbg("\nucsIcon: "); drw_dbg(ucsIcon); drw_dbg(" grid: "); drw_dbg(grid);
    gridSpacing = buf->get2RawDouble();
    drw_dbg("\ngrid Spacing: "); drw_dbgpt(gridSpacing.x, gridSpacing.y, gridSpacing.z);
    snap = buf->getBit();
    snapStyle = buf->getBit();
    drw_dbg("\nsnap on/off: "); drw_dbg(snap); drw_dbg(" snap Style: "); drw_dbg(snapStyle);
    snapIsopair = buf->getBitShort();
    snapAngle = buf->getBitDouble();
    drw_dbg("\nsnap Isopair: "); drw_dbg(snapIsopair); drw_dbg(" snap Angle: "); drw_dbg(snapAngle);
    snapBase = buf->get2RawDouble();
    drw_dbg("\nsnap Base: "); drw_dbgpt(snapBase.x, snapBase.y, snapBase.z);
    snapSpacing = buf->get2RawDouble();
    drw_dbg("\nsnap Spacing: "); drw_dbgpt(snapSpacing.x, snapSpacing.y, snapSpacing.z);
    if (version > DRW::AC1014) { //2000+
        drw_dbg("\n Unknown: "); drw_dbg(buf->getBit());
        drw_dbg(" UCS per Viewport: "); drw_dbg(buf->getBit());
        drw_dbg("\nUCS origin: "); drw_dbgpt(buf->getBitDouble(), buf->getBitDouble(), buf->getBitDouble());
        drw_dbg("\nUCS X Axis: "); drw_dbgpt(buf->getBitDouble(), buf->getBitDouble(), buf->getBitDouble());
        drw_dbg("\nUCS Y Axis: "); drw_dbgpt(buf->getBitDouble(), buf->getBitDouble(), buf->getBitDouble());
        drw_dbg("\nUCS elevation: "); drw_dbg(buf->getBitDouble());
        drw_dbg(" UCS Orthographic type: "); drw_dbg(buf->getBitShort());
        if (version > DRW::AC1018) { //2007+
            gridBehavior = buf->getBitShort();
            drw_dbg(" gridBehavior (flags): "); drw_dbg(gridBehavior);
            drw_dbg(" Grid major: "); drw_dbg(buf->getBitShort());
        }
    }

    //common handles
    if (version > DRW::AC1018) {//2007+ skip string area
        buf->setPosition(objSize >> 3);
        buf->setBitPos(objSize & 7);
    }
    dwgHandle vpControlH = buf->getHandle();
    drw_dbg("\n parentControlH Handle: "); drw_dbghl(vpControlH.code, vpControlH.size, vpControlH.ref); drw_dbg("\n");
    parentHandle = vpControlH.ref;
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    if (xDictFlag !=1){
        dwgHandle XDicObjH = buf->getHandle();
        drw_dbg(" XDicObj control Handle: "); drw_dbghl(XDicObjH.code, XDicObjH.size, XDicObjH.ref); drw_dbg("\n");
        drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    }
/*RLZ: fails verify this part*/    dwgHandle XRefH = buf->getHandle();
    drw_dbg(" XRefH control Handle: "); drw_dbghl(XRefH.code, XRefH.size, XRefH.ref);

    if (version > DRW::AC1014) { //2000+
        drw_dbg("\nRemaining bytes: "); drw_dbg(buf->numRemainingBytes());
        if (version > DRW::AC1018) { //2007+
            dwgHandle bkgrdH = buf->getHandle();
            drw_dbg(" background Handle: "); drw_dbghl(bkgrdH.code, bkgrdH.size, bkgrdH.ref);
            drw_dbg("\nRemaining bytes: "); drw_dbg(buf->numRemainingBytes());
            dwgHandle visualStH = buf->getHandle();
            drw_dbg(" visual style Handle: "); drw_dbghl(visualStH.code, visualStH.size, visualStH.ref);
            drw_dbg("\nRemaining bytes: "); drw_dbg(buf->numRemainingBytes());
            dwgHandle sunH = buf->getHandle();
            drw_dbg(" sun Handle: "); drw_dbghl(sunH.code, sunH.size, sunH.ref);
            drw_dbg("\nRemaining bytes: "); drw_dbg(buf->numRemainingBytes());
        }
        dwgHandle namedUCSH = buf->getHandle();
        drw_dbg(" named UCS Handle: "); drw_dbghl(namedUCSH.code, namedUCSH.size, namedUCSH.ref);
        drw_dbg("\nRemaining bytes: "); drw_dbg(buf->numRemainingBytes());
        dwgHandle baseUCSH = buf->getHandle();
        drw_dbg(" base UCS Handle: "); drw_dbghl(baseUCSH.code, baseUCSH.size, baseUCSH.ref);
    }

    drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    //    RS crc;   //RS */
    return buf->isGood();
}

void DRW_ImageDef::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 1:
        name = reader->getUtf8String();
        break;
    case 5:
        handle = reader->getHandleString();
        break;
    case 10:
        u = reader->getDouble();
        break;
    case 20:
        v = reader->getDouble();
        break;
    case 11:
        up = reader->getDouble();
        break;
    case 12:
        vp = reader->getDouble();
        break;
    case 21:
        vp = reader->getDouble();
        break;
    case 280:
        loaded = reader->getInt32();
        break;
    case 281:
        resolution = reader->getInt32();
        break;
    default:
        break;
    }
}

bool DRW_ImageDef::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    drw_dbg("\n***************************** parsing Image Def *********************************************\n");
    if (!ret)
        return ret;
    int32_t imgVersion = buf->getBitLong();
    drw_dbg("class Version: "); drw_dbg(imgVersion);
    DRW_Coord size = buf->get2RawDouble();
   (void)size;//RLZ: temporary, complete API
    name = sBuf->getVariableText(version, false);
    drw_dbg("appId name: "); drw_dbg(name.c_str()); drw_dbg("\n");
    loaded = buf->getBit();
    resolution = buf->getRawChar8();
    up = buf->getRawDouble();
    vp = buf->getRawDouble();

    dwgHandle parentH = buf->getHandle();
    drw_dbg(" parentH Handle: "); drw_dbghl(parentH.code, parentH.size, parentH.ref); drw_dbg("\n");
    parentHandle = parentH.ref;
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    //RLZ: Reactors handles
    if (xDictFlag !=1){
        dwgHandle XDicObjH = buf->getHandle();
        drw_dbg(" XDicObj control Handle: "); drw_dbghl(XDicObjH.code, XDicObjH.size, XDicObjH.ref); drw_dbg("\n");
        drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    }
/*RLZ: fails verify this part*/    dwgHandle XRefH = buf->getHandle();
    drw_dbg(" XRefH control Handle: "); drw_dbghl(XRefH.code, XRefH.size, XRefH.ref); drw_dbg("\n");

    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n\n");
    //    RS crc;   //RS */
    return buf->isGood();
}

void DRW_PlotSettings::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 5:
        handle = reader->getHandleString();
        break;
    case 6:
        plotViewName = reader->getUtf8String();
        break;
    case 40:
        marginLeft = reader->getDouble();
        break;
    case 41:
        marginBottom = reader->getDouble();
        break;
    case 42:
        marginRight = reader->getDouble();
        break;
    case 43:
        marginTop = reader->getDouble();
        break;
    default:
        break;
    }
}

bool DRW_PlotSettings::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    (void) version;
    (void) bs;
    drw_dbg("\n********************** parsing Plot Settings not yet implemented **************************\n");
    return buf->isGood();
}

bool DRW_AppId::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    drw_dbg("\n***************************** parsing app Id *********************************************\n");
    if (!ret)
        return ret;
    name = sBuf->getVariableText(version, false);
    drw_dbg("appId name: "); drw_dbg(name); drw_dbg("\n");
    flags |= buf->getBit()<< 6;// code 70, bit 7 (64)
    /*int16_t xrefindex =*/ buf->getBitShort();
    flags |= buf->getBit() << 4; //is refx dependent, style code 70, bit 5 (16)
    uint8_t unknown = buf->getRawChar8(); // unknown code 71
    drw_dbg("unknown code 71: "); drw_dbg(unknown); drw_dbg("\n");
    if (version > DRW::AC1018) {//2007+ skip string area
        buf->setPosition(objSize >> 3);
        buf->setBitPos(objSize & 7);
    }
    dwgHandle appIdControlH = buf->getHandle();
    drw_dbg(" parentControlH Handle: "); drw_dbghl(appIdControlH.code, appIdControlH.size, appIdControlH.ref); drw_dbg("\n");
    parentHandle = appIdControlH.ref;
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    if (xDictFlag !=1){//linetype in 2004 seems not have XDicObjH or NULL handle
        dwgHandle XDicObjH = buf->getHandle();
        drw_dbg(" XDicObj control Handle: "); drw_dbghl(XDicObjH.code, XDicObjH.size, XDicObjH.ref); drw_dbg("\n");
        drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    }
/*RLZ: fails verify this part*/    dwgHandle XRefH = buf->getHandle();
    drw_dbg(" XRefH control Handle: "); drw_dbghl(XRefH.code, XRefH.size, XRefH.ref); drw_dbg("\n");

    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n\n");
    //    RS crc;   //RS */
    return buf->isGood();
}
