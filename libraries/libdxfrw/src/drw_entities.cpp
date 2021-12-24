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
#include "drw_entities.h"
#include "intern/dxfreader.h"
#include "intern/dwgbuffer.h"
#include "intern/drw_dbg.h"

//! Calculate arbitrary axis
/*!
*   Calculate arbitrary axis for apply extrusions
*  @author Rallaz
*/
void DRW_Entity::calculateAxis(DRW_Coord extPoint){
    //Follow the arbitrary DXF definitions for extrusion axes.
    if (fabs(extPoint.x) < 0.015625 && fabs(extPoint.y) < 0.015625) {
        //If we get here, implement Ax = Wy x N where Wy is [0,1,0] per the DXF spec.
        //The cross product works out to Wy.y*N.z-Wy.z*N.y, Wy.z*N.x-Wy.x*N.z, Wy.x*N.y-Wy.y*N.x
        //Factoring in the fixed values for Wy gives N.z,0,-N.x
        extAxisX.x = extPoint.z;
        extAxisX.y = 0;
        extAxisX.z = -extPoint.x;
    } else {
        //Otherwise, implement Ax = Wz x N where Wz is [0,0,1] per the DXF spec.
        //The cross product works out to Wz.y*N.z-Wz.z*N.y, Wz.z*N.x-Wz.x*N.z, Wz.x*N.y-Wz.y*N.x
        //Factoring in the fixed values for Wz gives -N.y,N.x,0.
        extAxisX.x = -extPoint.y;
        extAxisX.y = extPoint.x;
        extAxisX.z = 0;
    }

    extAxisX.unitize();

    //Ay = N x Ax
    extAxisY.x = (extPoint.y * extAxisX.z) - (extAxisX.y * extPoint.z);
    extAxisY.y = (extPoint.z * extAxisX.x) - (extAxisX.z * extPoint.x);
    extAxisY.z = (extPoint.x * extAxisX.y) - (extAxisX.x * extPoint.y);

    extAxisY.unitize();
}

//! Extrude a point using arbitrary axis
/*!
*   apply extrusion in a point using arbitrary axis (previous calculated)
*  @author Rallaz
*/
void DRW_Entity::extrudePoint(DRW_Coord extPoint, DRW_Coord *point){
    double px, py, pz;
    px = (extAxisX.x*point->x)+(extAxisY.x*point->y)+(extPoint.x*point->z);
    py = (extAxisX.y*point->x)+(extAxisY.y*point->y)+(extPoint.y*point->z);
    pz = (extAxisX.z*point->x)+(extAxisY.z*point->y)+(extPoint.z*point->z);

    point->x = px;
    point->y = py;
    point->z = pz;
}

bool DRW_Entity::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 5:
        handle = reader->getHandleString();
        break;
    case 330:
        parentHandle = reader->getHandleString();
        break;
    case 8:
        layer = reader->getUtf8String();
        break;
    case 6:
        lineType = reader->getUtf8String();
        break;
    case 62:
        color = reader->getInt32();
        break;
    case 370:
        lWeight = DRW_LW_Conv::dxfInt2lineWidth(reader->getInt32());
        break;
    case 48:
        ltypeScale = reader->getDouble();
        break;
    case 60:
        visible = reader->getBool();
        break;
    case 420:
        color24 = reader->getInt32();
        break;
    case 430:
        colorName = reader->getString();
        break;
    case 67:
        space = static_cast<DRW::Space>(reader->getInt32());
        break;
    case 102:
        parseDxfGroups(code, reader);
        break;
    case 1000:
    case 1001:
    case 1002:
    case 1003:
    case 1004:
    case 1005:
		extData.push_back(std::make_shared<DRW_Variant>(code, reader->getString()));
        break;
    case 1010:
    case 1011:
    case 1012:
    case 1013:
		curr =std::make_shared<DRW_Variant>(code, DRW_Coord(reader->getDouble(), 0.0, 0.0));
        extData.push_back(curr);
        break;
    case 1020:
    case 1021:
    case 1022:
    case 1023:
        if (curr)
            curr->setCoordY(reader->getDouble());
        break;
    case 1030:
    case 1031:
    case 1032:
    case 1033:
        if (curr)
            curr->setCoordZ(reader->getDouble());
		//FIXME, why do we discard curr right after setting the its Z
//        curr=NULL;
        break;
    case 1040:
    case 1041:
    case 1042:
		extData.push_back(std::make_shared<DRW_Variant>(code, reader->getDouble() ));
        break;
    case 1070:
    case 1071:
		extData.push_back(std::make_shared<DRW_Variant>(code, reader->getInt32() ));
        break;
    default:
        break;
    }
    return true;
}

//parses dxf 102 groups to read entity
bool DRW_Entity::parseDxfGroups(int code, dxfReader *reader){
    std::list<DRW_Variant> ls;
    DRW_Variant curr;
    int nc;
    std::string appName= reader->getString();
    if (!appName.empty() && appName.at(0)== '{'){
        curr.addString(code, appName.substr(1, (int) appName.size()-1));
        ls.push_back(curr);
        while (code !=102 && appName.at(0)== '}'){
            reader->readRec(&nc);//RLZ curr.code = code or nc?
//            curr.code = code;
            //RLZ code == 330 || code == 360 OR nc == 330 || nc == 360 ?
            if (code == 330 || code == 360)
                curr.addInt(code, reader->getHandleString());//RLZ code or nc
            else {
                switch (reader->type) {
                case dxfReader::STRING:
                    curr.addString(code, reader->getString());//RLZ code or nc
                    break;
                case dxfReader::INT32:
                case dxfReader::INT64:
                    curr.addInt(code, reader->getInt32());//RLZ code or nc
                    break;
                case dxfReader::DOUBLE:
                    curr.addDouble(code, reader->getDouble());//RLZ code or nc
                    break;
                case dxfReader::BOOL:
                    curr.addInt(code, reader->getInt32());//RLZ code or nc
                    break;
                default:
                    break;
                }
            }
            ls.push_back(curr);
        }
    }

    appData.push_back(ls);
    return true;
}

bool DRW_Entity::parseDwg(DRW::Version version, dwgBuffer *buf, dwgBuffer* strBuf, uint32_t bs){
    objSize=0;
    drw_dbg("\n***************************** parsing entity *********************************************\n");
    oType = buf->getObjType(version);
    drw_dbg("Object type: "); drw_dbg(oType); drw_dbg(", "); drw_dbgh(oType);

    if (version > DRW::AC1014 && version < DRW::AC1024) {//2000 & 2004
        objSize = buf->getRawLong32();  //RL 32bits object size in bits
        drw_dbg(" Object size: "); drw_dbg(objSize); drw_dbg("\n");
    }
    if (version > DRW::AC1021) {//2010+
        uint32_t ms = buf->size();
        objSize = ms*8 - bs;
        drw_dbg(" Object size: "); drw_dbg(objSize); drw_dbg("\n");
    }

    if (strBuf != NULL && version > DRW::AC1018) {//2007+
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
    drw_dbg("Entity Handle: "); drw_dbghl(ho.code, ho.size, ho.ref);
    int16_t extDataSize = buf->getBitShort(); //BS
    drw_dbg(" ext data size: "); drw_dbg(extDataSize);
    while (extDataSize>0 && buf->isGood()) {
        /* RLZ: TODO */
        dwgHandle ah = buf->getHandle();
        drw_dbg("App Handle: "); drw_dbghl(ah.code, ah.size, ah.ref);
        uint8_t *tmpExtData = new uint8_t[extDataSize];
        buf->getBytes(tmpExtData, extDataSize);
        dwgBuffer tmpExtDataBuf(tmpExtData, extDataSize, buf->decoder);

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
        delete[]tmpExtData;
        extDataSize = buf->getBitShort(); //BS
        drw_dbg(" ext data size: "); drw_dbg(extDataSize);
    } //end parsing extData (EED)
    uint8_t graphFlag = buf->getBit(); //B
    drw_dbg(" graphFlag: "); drw_dbg(graphFlag); drw_dbg("\n");
    if (graphFlag) {
        uint32_t graphDataSize = buf->getRawLong32();  //RL 32bits
        drw_dbg("graphData in bytes: "); drw_dbg(graphDataSize); drw_dbg("\n");
// RLZ: TODO
        //skip graphData bytes
        uint8_t *tmpGraphData = new uint8_t[graphDataSize];
        buf->getBytes(tmpGraphData, graphDataSize);
        dwgBuffer tmpGraphDataBuf(tmpGraphData, graphDataSize, buf->decoder);
        drw_dbg("graph data remaining bytes: "); drw_dbg(tmpGraphDataBuf.numRemainingBytes()); drw_dbg("\n");
        delete[]tmpGraphData;
    }
    if (version < DRW::AC1015) {//14-
        objSize = buf->getRawLong32();  //RL 32bits object size in bits
        drw_dbg(" Object size in bits: "); drw_dbg(objSize); drw_dbg("\n");
    }

    uint8_t entmode = buf->get2Bits(); //BB
    if (entmode == 0)
        ownerHandle= true;
//        entmode = 2;
    else if(entmode ==2)
        entmode = 0;
    space = (DRW::Space)entmode; //RLZ verify cast values
    drw_dbg("entmode: "); drw_dbg(entmode);
    numReactors = buf->getBitShort(); //BS
    drw_dbg(", numReactors: "); drw_dbg(numReactors);

    if (version < DRW::AC1015) {//14-
        if(buf->getBit()) {//is bylayer line type
            lineType = "BYLAYER";
            ltFlags = 0;
        } else {
            lineType = "";
            ltFlags = 3;
        }
        drw_dbg(" lineType: "); drw_dbg(lineType.c_str());
        drw_dbg(" ltFlags: "); drw_dbg(ltFlags);
    }
    if (version > DRW::AC1015) {//2004+
        xDictFlag = buf->getBit();
        drw_dbg(" xDictFlag: "); drw_dbg(xDictFlag); drw_dbg("\n");
    }

    if (version > DRW::AC1024 || version < DRW::AC1018) {
        haveNextLinks = buf->getBit(); //aka nolinks //B
        drw_dbg(", haveNextLinks (0 yes, 1 prev next): "); drw_dbg(haveNextLinks); drw_dbg("\n");
    } else {
        haveNextLinks = 1; //aka nolinks //B
        drw_dbg(", haveNextLinks (forced): "); drw_dbg(haveNextLinks); drw_dbg("\n");
    }
//ENC color
    color = buf->getEnColor(version); //BS or CMC //ok for R14 or negate
    ltypeScale = buf->getBitDouble(); //BD
    drw_dbg(" entity color: "); drw_dbg(color);
    drw_dbg(" ltScale: "); drw_dbg(ltypeScale); drw_dbg("\n");
    if (version > DRW::AC1014) {//2000+
        std::string plotStyleName;
        for (uint8_t i = 0; i<2;++i) { //two flags in one
            plotFlags = buf->get2Bits(); //BB
            if (plotFlags == 1)
                plotStyleName = "byblock";
            else if (plotFlags == 2)
                plotStyleName = "continuous";
            else if (plotFlags == 0)
                plotStyleName = "bylayer";
            else //handle at end
                plotStyleName = "";
            if (i == 0) {
                ltFlags = plotFlags;
                lineType = plotStyleName; //RLZ: howto solve? if needed plotStyleName;
                drw_dbg("ltFlags: "); drw_dbg(ltFlags);
                drw_dbg(" lineType: "); drw_dbg(lineType.c_str());
            } else {
                drw_dbg(", plotFlags: "); drw_dbg(plotFlags);
            }
        }
    }
    if (version > DRW::AC1018) {//2007+
        materialFlag = buf->get2Bits(); //BB
        drw_dbg("materialFlag: "); drw_dbg(materialFlag);
        shadowFlag = buf->getRawChar8(); //RC
        drw_dbg("shadowFlag: "); drw_dbg(shadowFlag); drw_dbg("\n");
    }
    if (version > DRW::AC1021) {//2010+
        uint8_t visualFlags = buf->get2Bits(); //full & face visual style
        drw_dbg("shadowFlag 2: "); drw_dbg(visualFlags); drw_dbg("\n");
        uint8_t unk = buf->getBit(); //edge visual style
        drw_dbg("unknown bit: "); drw_dbg(unk); drw_dbg("\n");
    }
    int16_t invisibleFlag = buf->getBitShort(); //BS
    drw_dbg(" invisibleFlag: "); drw_dbg(invisibleFlag);
    if (version > DRW::AC1014) {//2000+
        lWeight = DRW_LW_Conv::dwgInt2lineWidth( buf->getRawChar8() ); //RC
        drw_dbg(" lwFlag (lWeight): "); drw_dbg(lWeight); drw_dbg("\n");
    }
    //Only in blocks ????????
//    if (version > DRW::AC1018) {//2007+
//        uint8_t unk = buf->getBit();
//        drw_dbg("unknown bit: "); drw_dbg(unk); drw_dbg("\n");
//    }
    return buf->isGood();
}

bool DRW_Entity::parseDwgEntHandle(DRW::Version version, dwgBuffer *buf){
    if (version > DRW::AC1018) {//2007+ skip string area
        buf->setPosition(objSize >> 3);
        buf->setBitPos(objSize & 7);
    }

    if(ownerHandle){//entity are in block or in a polyline
        dwgHandle ownerH = buf->getOffsetHandle(handle);
        drw_dbg("owner (parent) Handle: "); drw_dbghl(ownerH.code, ownerH.size, ownerH.ref); drw_dbg("\n");
        drw_dbg("   Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
        parentHandle = ownerH.ref;
        drw_dbg("Block (parent) Handle: "); drw_dbghl(ownerH.code, ownerH.size, parentHandle); drw_dbg("\n");
    } else
        drw_dbg("NO Block (parent) Handle\n");

    drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    for (int i=0; i< numReactors;++i) {
        dwgHandle reactorsH = buf->getHandle();
        drw_dbg(" reactorsH control Handle: "); drw_dbghl(reactorsH.code, reactorsH.size, reactorsH.ref); drw_dbg("\n");
    }
    if (xDictFlag !=1){//linetype in 2004 seems not have XDicObjH or NULL handle
        dwgHandle XDicObjH = buf->getHandle();
        drw_dbg(" XDicObj control Handle: "); drw_dbghl(XDicObjH.code, XDicObjH.size, XDicObjH.ref); drw_dbg("\n");
    }
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");

    if (version < DRW::AC1015) {//R14-
        //layer handle
        layerH = buf->getOffsetHandle(handle);
        drw_dbg(" layer Handle: "); drw_dbghl(layerH.code, layerH.size, layerH.ref); drw_dbg("\n");
        drw_dbg("   Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
        //lineType handle
        if(ltFlags == 3){
            lTypeH = buf->getOffsetHandle(handle);
            drw_dbg("linetype Handle: "); drw_dbghl(lTypeH.code, lTypeH.size, lTypeH.ref); drw_dbg("\n");
            drw_dbg("   Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
        }
    }
    if (version < DRW::AC1018) {//2000+
        if (haveNextLinks == 0) {
            dwgHandle nextLinkH = buf->getOffsetHandle(handle);
            drw_dbg(" prev nextLinkers Handle: "); drw_dbghl(nextLinkH.code, nextLinkH.size, nextLinkH.ref); drw_dbg("\n");
            drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
            prevEntLink = nextLinkH.ref;
            nextLinkH = buf->getOffsetHandle(handle);
            drw_dbg(" next nextLinkers Handle: "); drw_dbghl(nextLinkH.code, nextLinkH.size, nextLinkH.ref); drw_dbg("\n");
            drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
            nextEntLink = nextLinkH.ref;
        } else {
            nextEntLink = handle+1;
            prevEntLink = handle-1;
        }
    }
    if (version > DRW::AC1015) {//2004+
        //Parses Bookcolor handle
    }
    if (version > DRW::AC1014) {//2000+
        //layer handle
        layerH = buf->getOffsetHandle(handle);
        drw_dbg(" layer Handle: "); drw_dbghl(layerH.code, layerH.size, layerH.ref); drw_dbg("\n");
        drw_dbg("   Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
        //lineType handle
        if(ltFlags == 3){
            lTypeH = buf->getOffsetHandle(handle);
            drw_dbg("linetype Handle: "); drw_dbghl(lTypeH.code, lTypeH.size, lTypeH.ref); drw_dbg("\n");
            drw_dbg("   Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
        }
    }
    if (version > DRW::AC1014) {//2000+
        if (version > DRW::AC1018) {//2007+
            if (materialFlag == 3) {
                dwgHandle materialH = buf->getOffsetHandle(handle);
                drw_dbg(" material Handle: "); drw_dbghl(materialH.code, materialH.size, materialH.ref); drw_dbg("\n");
                drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
            }
            if (shadowFlag == 3) {
                dwgHandle shadowH = buf->getOffsetHandle(handle);
                drw_dbg(" shadow Handle: "); drw_dbghl(shadowH.code, shadowH.size, shadowH.ref); drw_dbg("\n");
                drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
            }
        }
        if (plotFlags == 3) {
            dwgHandle plotStyleH = buf->getOffsetHandle(handle);
            drw_dbg(" plot style Handle: "); drw_dbghl(plotStyleH.code, plotStyleH.size, plotStyleH.ref); drw_dbg("\n");
            drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
        }
    }
    drw_dbg("\n DRW_Entity::parseDwgEntHandle Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    return buf->isGood();
}

void DRW_Point::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 10:
        basePoint.x = reader->getDouble();
        break;
    case 20:
        basePoint.y = reader->getDouble();
        break;
    case 30:
        basePoint.z = reader->getDouble();
        break;
    case 39:
        thickness = reader->getDouble();
        break;
    case 210:
        haveExtrusion = true;
        extPoint.x = reader->getDouble();
        break;
    case 220:
        extPoint.y = reader->getDouble();
        break;
    case 230:
        extPoint.z = reader->getDouble();
        break;
    default:
        DRW_Entity::parseCode(code, reader);
        break;
    }
}

bool DRW_Point::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing point *********************************************\n");

    basePoint.x = buf->getBitDouble();
    basePoint.y = buf->getBitDouble();
    basePoint.z = buf->getBitDouble();
    drw_dbg("point: "); drw_dbgpt(basePoint.x, basePoint.y, basePoint.z);
    thickness = buf->getThickness(version > DRW::AC1014);//BD
    drw_dbg("\nthickness: "); drw_dbg(thickness);
    extPoint = buf->getExtrusion(version > DRW::AC1014);
    drw_dbg(", Extrusion: "); drw_dbgpt(extPoint.x, extPoint.y, extPoint.z);

    double x_axis = buf->getBitDouble();//BD
    drw_dbg("\n  x_axis: ");drw_dbg(x_axis);drw_dbg("\n");
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
    //    RS crc;   //RS */

    return buf->isGood();
}

void DRW_Line::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 11:
        secPoint.x = reader->getDouble();
        break;
    case 21:
        secPoint.y = reader->getDouble();
        break;
    case 31:
        secPoint.z = reader->getDouble();
        break;
    default:
        DRW_Point::parseCode(code, reader);
        break;
    }
}

bool DRW_Line::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing line *********************************************\n");

    if (version < DRW::AC1015) {//14-
        basePoint.x = buf->getBitDouble();
        basePoint.y = buf->getBitDouble();
        basePoint.z = buf->getBitDouble();
        secPoint.x = buf->getBitDouble();
        secPoint.y = buf->getBitDouble();
        secPoint.z = buf->getBitDouble();
    }
    if (version > DRW::AC1014) {//2000+
        bool zIsZero = buf->getBit(); //B
        basePoint.x = buf->getRawDouble();//RD
        secPoint.x = buf->getDefaultDouble(basePoint.x);//DD
        basePoint.y = buf->getRawDouble();//RD
        secPoint.y = buf->getDefaultDouble(basePoint.y);//DD
        if (!zIsZero) {
            basePoint.z = buf->getRawDouble();//RD
            secPoint.z = buf->getDefaultDouble(basePoint.z);//DD
        }
    }
    drw_dbg("start point: "); drw_dbgpt(basePoint.x, basePoint.y, basePoint.z);
    drw_dbg("\nend point: "); drw_dbgpt(secPoint.x, secPoint.y, secPoint.z);
    thickness = buf->getThickness(version > DRW::AC1014);//BD
    drw_dbg("\nthickness: "); drw_dbg(thickness);
    extPoint = buf->getExtrusion(version > DRW::AC1014);
    drw_dbg(", Extrusion: "); drw_dbgpt(extPoint.x, extPoint.y, extPoint.z);drw_dbg("\n");
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
//    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_Ray::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing ray/xline *********************************************\n");
    basePoint.x = buf->getBitDouble();
    basePoint.y = buf->getBitDouble();
    basePoint.z = buf->getBitDouble();
    secPoint.x = buf->getBitDouble();
    secPoint.y = buf->getBitDouble();
    secPoint.z = buf->getBitDouble();
    drw_dbg("start point: "); drw_dbgpt(basePoint.x, basePoint.y, basePoint.z);
    drw_dbg("\nvector: "); drw_dbgpt(secPoint.x, secPoint.y, secPoint.z);
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
//    RS crc;   //RS */
    return buf->isGood();
}

void DRW_Circle::applyExtrusion(){
    if (haveExtrusion) {
        //NOTE: Commenting these out causes the the arcs being tested to be located
        //on the other side of the y axis (all x dimensions are negated).
        calculateAxis(extPoint);
        extrudePoint(extPoint, &basePoint);
    }
}

void DRW_Circle::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 40:
        radious = reader->getDouble();
        break;
    default:
        DRW_Point::parseCode(code, reader);
        break;
    }
}

bool DRW_Circle::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing circle *********************************************\n");

    basePoint.x = buf->getBitDouble();
    basePoint.y = buf->getBitDouble();
    basePoint.z = buf->getBitDouble();
    drw_dbg("center: "); drw_dbgpt(basePoint.x, basePoint.y, basePoint.z);
    radious = buf->getBitDouble();
    drw_dbg("\nradius: "); drw_dbg(radious);

    thickness = buf->getThickness(version > DRW::AC1014);
    drw_dbg(" thickness: "); drw_dbg(thickness);
    extPoint = buf->getExtrusion(version > DRW::AC1014);
    drw_dbg("\nextrusion: "); drw_dbgpt(extPoint.x, extPoint.y, extPoint.z); drw_dbg("\n");

    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
//    RS crc;   //RS */
    return buf->isGood();
}

void DRW_Arc::applyExtrusion(){
    DRW_Circle::applyExtrusion();

    if(haveExtrusion){
        // If the extrusion vector has a z value less than 0, the angles for the arc
        // have to be mirrored since DXF files use the right hand rule.
        // Note that the following code only handles the special case where there is a 2D
        // drawing with the z axis heading into the paper (or rather screen). An arbitrary
        // extrusion axis (with x and y values greater than 1/64) may still have issues.
        if (fabs(extPoint.x) < 0.015625 && fabs(extPoint.y) < 0.015625 && extPoint.z < 0.0) {
            staangle=M_PI-staangle;
            endangle=M_PI-endangle;

            double temp = staangle;
            staangle=endangle;
            endangle=temp;
        }
    }
}

void DRW_Arc::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 50:
        staangle = reader->getDouble()/ ARAD;
        break;
    case 51:
        endangle = reader->getDouble()/ ARAD;
        break;
    default:
        DRW_Circle::parseCode(code, reader);
        break;
    }
}

bool DRW_Arc::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing circle arc *********************************************\n");

    basePoint.x = buf->getBitDouble();
    basePoint.y = buf->getBitDouble();
    basePoint.z = buf->getBitDouble();
    drw_dbg("center point: "); drw_dbgpt(basePoint.x, basePoint.y, basePoint.z);

    radious = buf->getBitDouble();
    drw_dbg("\nradius: "); drw_dbg(radious);
    thickness = buf->getThickness(version > DRW::AC1014);
    drw_dbg(" thickness: "); drw_dbg(thickness);
    extPoint = buf->getExtrusion(version > DRW::AC1014);
    drw_dbg("\nextrusion: "); drw_dbgpt(extPoint.x, extPoint.y, extPoint.z);
    staangle = buf->getBitDouble();
    drw_dbg("\nstart angle: "); drw_dbg(staangle);
    endangle = buf->getBitDouble();
    drw_dbg(" end angle: "); drw_dbg(endangle); drw_dbg("\n");
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
    return buf->isGood();
}

void DRW_Ellipse::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 40:
        ratio = reader->getDouble();
        break;
    case 41:
        staparam = reader->getDouble();
        break;
    case 42:
        endparam = reader->getDouble();
        break;
    default:
        DRW_Line::parseCode(code, reader);
        break;
    }
}

void DRW_Ellipse::applyExtrusion(){
    if (haveExtrusion) {
        calculateAxis(extPoint);
        extrudePoint(extPoint, &secPoint);
        double intialparam = staparam;
        if (extPoint.z < 0.){
            staparam = M_PIx2 - endparam;
            endparam = M_PIx2 - intialparam;
        }
    }
}

//if ratio > 1 minor axis are greather than major axis, correct it
void DRW_Ellipse::correctAxis(){
    bool complete = false;
    if (staparam == endparam) {
        staparam = 0.0;
        endparam = M_PIx2; //2*M_PI;
        complete = true;
    }
    if (ratio > 1){
        if ( fabs(endparam - staparam - M_PIx2) < 1.0e-10)
            complete = true;
        double incX = secPoint.x;
        secPoint.x = -(secPoint.y * ratio);
        secPoint.y = incX*ratio;
        ratio = 1/ratio;
        if (!complete){
            if (staparam < M_PI_2)
                staparam += M_PI *2;
            if (endparam < M_PI_2)
                endparam += M_PI *2;
            endparam -= M_PI_2;
            staparam -= M_PI_2;
        }
    }
}

bool DRW_Ellipse::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing ellipse *********************************************\n");

    basePoint =buf->get3BitDouble();
    drw_dbg("center: "); drw_dbgpt(basePoint.x, basePoint.y, basePoint.z);
    secPoint =buf->get3BitDouble();
    drw_dbg(", axis: "); drw_dbgpt(secPoint.x, secPoint.y, secPoint.z); drw_dbg("\n");
    extPoint =buf->get3BitDouble();
    drw_dbg("Extrusion: "); drw_dbgpt(extPoint.x, extPoint.y, extPoint.z);
    ratio = buf->getBitDouble();//BD
    drw_dbg("\nratio: "); drw_dbg(ratio);
    staparam = buf->getBitDouble();//BD
    drw_dbg(" start param: "); drw_dbg(staparam);
    endparam = buf->getBitDouble();//BD
    drw_dbg(" end param: "); drw_dbg(endparam); drw_dbg("\n");

    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
//    RS crc;   //RS */
    return buf->isGood();
}

//parts are the number of vertex to split polyline, default 128
void DRW_Ellipse::toPolyline(DRW_Polyline *pol, int parts){
    double radMajor, radMinor, cosRot, sinRot, incAngle, curAngle;
    double cosCurr, sinCurr;
	radMajor = hypot(secPoint.x, secPoint.y);
    radMinor = radMajor*ratio;
    //calculate sin & cos of included angle
    incAngle = atan2(secPoint.y, secPoint.x);
    cosRot = cos(incAngle);
    sinRot = sin(incAngle);
    incAngle = M_PIx2 / parts;
    curAngle = staparam;
    int i = static_cast<int>(curAngle / incAngle);
    do {
        if (curAngle > endparam) {
            curAngle = endparam;
            i = parts+2;
        }
        cosCurr = cos(curAngle);
        sinCurr = sin(curAngle);
        double x = basePoint.x + (cosCurr*cosRot*radMajor) - (sinCurr*sinRot*radMinor);
        double y = basePoint.y + (cosCurr*sinRot*radMajor) + (sinCurr*cosRot*radMinor);
        pol->addVertex( DRW_Vertex(x, y, 0.0, 0.0));
        curAngle = (++i)*incAngle;
    } while (i<parts);
    if ( fabs(endparam - staparam - M_PIx2) < 1.0e-10){
        pol->flags = 1;
    }
    pol->layer = this->layer;
    pol->lineType = this->lineType;
    pol->color = this->color;
    pol->lWeight = this->lWeight;
    pol->extPoint = this->extPoint;
}

void DRW_Trace::applyExtrusion(){
    if (haveExtrusion) {
        calculateAxis(extPoint);
        extrudePoint(extPoint, &basePoint);
        extrudePoint(extPoint, &secPoint);
        extrudePoint(extPoint, &thirdPoint);
        extrudePoint(extPoint, &fourPoint);
    }
}

void DRW_Trace::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 12:
        thirdPoint.x = reader->getDouble();
        break;
    case 22:
        thirdPoint.y = reader->getDouble();
        break;
    case 32:
        thirdPoint.z = reader->getDouble();
        break;
    case 13:
        fourPoint.x = reader->getDouble();
        break;
    case 23:
        fourPoint.y = reader->getDouble();
        break;
    case 33:
        fourPoint.z = reader->getDouble();
        break;
    default:
        DRW_Line::parseCode(code, reader);
        break;
    }
}

bool DRW_Trace::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing Trace *********************************************\n");

    thickness = buf->getThickness(version>DRW::AC1014);
    basePoint.z = buf->getBitDouble();
    basePoint.x = buf->getRawDouble();
    basePoint.y = buf->getRawDouble();
    secPoint.x = buf->getRawDouble();
    secPoint.y = buf->getRawDouble();
    secPoint.z = basePoint.z;
    thirdPoint.x = buf->getRawDouble();
    thirdPoint.y = buf->getRawDouble();
    thirdPoint.z = basePoint.z;
    fourPoint.x = buf->getRawDouble();
    fourPoint.y = buf->getRawDouble();
    fourPoint.z = basePoint.z;
    extPoint = buf->getExtrusion(version>DRW::AC1014);

    drw_dbg(" - base "); drw_dbgpt(basePoint.x, basePoint.y, basePoint.z);
    drw_dbg("\n - sec "); drw_dbgpt(secPoint.x, secPoint.y, secPoint.z);
    drw_dbg("\n - third "); drw_dbgpt(thirdPoint.x, thirdPoint.y, thirdPoint.z);
    drw_dbg("\n - fourth "); drw_dbgpt(fourPoint.x, fourPoint.y, fourPoint.z);
    drw_dbg("\n - extrusion: "); drw_dbgpt(extPoint.x, extPoint.y, extPoint.z);
    drw_dbg("\n - thickness: "); drw_dbg(thickness); drw_dbg("\n");

    /* Common Entity Handle Data */
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;

    /* CRC X --- */
    return buf->isGood();
}


void DRW_Solid::parseCode(int code, dxfReader *reader){
    DRW_Trace::parseCode(code, reader);
}

bool DRW_Solid::parseDwg(DRW::Version v, dwgBuffer *buf, uint32_t bs){
    drw_dbg("\n***************************** parsing Solid *********************************************\n");
    return DRW_Trace::parseDwg(v, buf, bs);
}

void DRW_3Dface::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 70:
        invisibleflag = reader->getInt32();
        break;
    default:
        DRW_Trace::parseCode(code, reader);
        break;
    }
}

bool DRW_3Dface::parseDwg(DRW::Version v, dwgBuffer *buf, uint32_t bs){
    bool ret = DRW_Entity::parseDwg(v, buf, NULL, bs);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing 3Dface *********************************************\n");

    if ( v < DRW::AC1015 ) {// R13 & R14
        basePoint.x = buf->getBitDouble();
        basePoint.y = buf->getBitDouble();
        basePoint.z = buf->getBitDouble();
        secPoint.x = buf->getBitDouble();
        secPoint.y = buf->getBitDouble();
        secPoint.z = buf->getBitDouble();
        thirdPoint.x = buf->getBitDouble();
        thirdPoint.y = buf->getBitDouble();
        thirdPoint.z = buf->getBitDouble();
        fourPoint.x = buf->getBitDouble();
        fourPoint.y = buf->getBitDouble();
        fourPoint.z = buf->getBitDouble();
        invisibleflag = buf->getBitShort();
    } else { // 2000+
        bool has_no_flag = buf->getBit();
        bool z_is_zero = buf->getBit();
        basePoint.x = buf->getRawDouble();
        basePoint.y = buf->getRawDouble();
        basePoint.z = z_is_zero ? 0.0 : buf->getRawDouble();
        secPoint.x = buf->getDefaultDouble(basePoint.x);
        secPoint.y = buf->getDefaultDouble(basePoint.y);
        secPoint.z = buf->getDefaultDouble(basePoint.z);
        thirdPoint.x = buf->getDefaultDouble(secPoint.x);
        thirdPoint.y = buf->getDefaultDouble(secPoint.y);
        thirdPoint.z = buf->getDefaultDouble(secPoint.z);
        fourPoint.x = buf->getDefaultDouble(thirdPoint.x);
        fourPoint.y = buf->getDefaultDouble(thirdPoint.y);
        fourPoint.z = buf->getDefaultDouble(thirdPoint.z);
        invisibleflag = has_no_flag ? (int)NoEdge : buf->getBitShort();
    }

    drw_dbg(" - base "); drw_dbgpt(basePoint.x, basePoint.y, basePoint.z); drw_dbg("\n");
    drw_dbg(" - sec "); drw_dbgpt(secPoint.x, secPoint.y, secPoint.z); drw_dbg("\n");
    drw_dbg(" - third "); drw_dbgpt(thirdPoint.x, thirdPoint.y, thirdPoint.z); drw_dbg("\n");
    drw_dbg(" - fourth "); drw_dbgpt(fourPoint.x, fourPoint.y, fourPoint.z); drw_dbg("\n");
    drw_dbg(" - Invisibility mask: "); drw_dbg(invisibleflag); drw_dbg("\n");

    /* Common Entity Handle Data */
    ret = DRW_Entity::parseDwgEntHandle(v, buf);
    if (!ret)
        return ret;
    return buf->isGood();
}

void DRW_Block::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 2:
        name = reader->getUtf8String();
        break;
    case 70:
        flags = reader->getInt32();
        break;
    default:
        DRW_Point::parseCode(code, reader);
        break;
    }
}

bool DRW_Block::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    if (!isEnd){
        drw_dbg("\n***************************** parsing block *********************************************\n");
        name = sBuf->getVariableText(version, false);
        drw_dbg("Block name: "); drw_dbg(name.c_str()); drw_dbg("\n");
    } else {
        drw_dbg("\n***************************** parsing end block *********************************************\n");
    }
    if (version > DRW::AC1018) {//2007+
        uint8_t unk = buf->getBit();
        drw_dbg("unknown bit: "); drw_dbg(unk); drw_dbg("\n");
    }
//    X handleAssoc;   //X
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
//    RS crc;   //RS */
    return buf->isGood();
}

void DRW_Insert::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 2:
        name = reader->getUtf8String();
        break;
    case 41:
        xscale = reader->getDouble();
        break;
    case 42:
        yscale = reader->getDouble();
        break;
    case 43:
        zscale = reader->getDouble();
        break;
    case 50:
        angle = reader->getDouble();
        angle = angle/ARAD; //convert to radian
        break;
    case 70:
        colcount = reader->getInt32();
        break;
    case 71:
        rowcount = reader->getInt32();
        break;
    case 44:
        colspace = reader->getDouble();
        break;
    case 45:
        rowspace = reader->getDouble();
        break;
    default:
        DRW_Point::parseCode(code, reader);
        break;
    }
}

bool DRW_Insert::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    int32_t objCount = 0;
    bool ret = DRW_Entity::parseDwg(version, buf, nullptr, bs);
    if (!ret)
        return ret;
    drw_dbg("\n************************** parsing insert/minsert *****************************************\n");
    basePoint.x = buf->getBitDouble();
    basePoint.y = buf->getBitDouble();
    basePoint.z = buf->getBitDouble();
    drw_dbg("insertion point: "); drw_dbgpt(basePoint.x, basePoint.y, basePoint.z); drw_dbg("\n");
    if (version < DRW::AC1015) {//14-
        xscale = buf->getBitDouble();
        yscale = buf->getBitDouble();
        zscale = buf->getBitDouble();
    } else {
        uint8_t dataFlags = buf->get2Bits();
        if (dataFlags == 3){
            //none default value 1,1,1
        } else if (dataFlags == 1){ //x default value 1, y & z can be x value
            yscale = buf->getDefaultDouble(xscale);
            zscale = buf->getDefaultDouble(xscale);
        } else if (dataFlags == 2){
            xscale = buf->getRawDouble();
            yscale = zscale = xscale;
        } else { //dataFlags == 0
            xscale = buf->getRawDouble();
            yscale = buf->getDefaultDouble(xscale);
            zscale = buf->getDefaultDouble(xscale);
        }
    }
    angle = buf->getBitDouble();
    drw_dbg("scale : "); drw_dbgpt(xscale, yscale, zscale); drw_dbg(", angle: "); drw_dbg(angle);
    extPoint = buf->getExtrusion(false); //3BD R14 style
    drw_dbg("\nextrusion: "); drw_dbgpt(extPoint.x, extPoint.y, extPoint.z);

    bool hasAttrib = buf->getBit();
    drw_dbg("   has Attrib: "); drw_dbg(hasAttrib);

    if (hasAttrib && version > DRW::AC1015) {//2004+
        drw_dbg("   objCount: "); drw_dbg(buf->getBitLong()); drw_dbg("\n");
    }
    if (oType == 8) {//entity are minsert
        colcount = buf->getBitShort();
        rowcount = buf->getBitShort();
        colspace = buf->getBitDouble();
        rowspace = buf->getBitDouble();
    }
    drw_dbg("   Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    blockRecH = buf->getHandle(); /* H 2 BLOCK HEADER (hard pointer) */
    drw_dbg("BLOCK HEADER Handle: "); drw_dbghl(blockRecH.code, blockRecH.size, blockRecH.ref); drw_dbg("\n");
    drw_dbg("   Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");

    /*attribs follows*/
    if (hasAttrib) {
        if (version < DRW::AC1018) {//2000-
            dwgHandle attH = buf->getHandle(); /* H 2 BLOCK HEADER (hard pointer) */
            drw_dbg("first attrib Handle: "); drw_dbghl(attH.code, attH.size, attH.ref); drw_dbg("\n");
            attH = buf->getHandle(); /* H 2 BLOCK HEADER (hard pointer) */
            drw_dbg("second attrib Handle: "); drw_dbghl(attH.code, attH.size, attH.ref); drw_dbg("\n");
        } else {
            for (uint8_t i=0; i< objCount; ++i){
                dwgHandle attH = buf->getHandle(); /* H 2 BLOCK HEADER (hard pointer) */
                drw_dbg("attrib Handle #"); drw_dbg(i); drw_dbg(": "); drw_dbghl(attH.code, attH.size, attH.ref); drw_dbg("\n");
            }
        }
        seqendH = buf->getHandle(); /* H 2 BLOCK HEADER (hard pointer) */
        drw_dbg("seqendH Handle: "); drw_dbghl(seqendH.code, seqendH.size, seqendH.ref); drw_dbg("\n");
    }
    drw_dbg("   Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");

    if (!ret)
        return ret;
//    RS crc;   //RS */
    return buf->isGood();
}

void DRW_LWPolyline::applyExtrusion(){
    if (haveExtrusion) {
        calculateAxis(extPoint);
        for (unsigned int i=0; i<vertlist.size(); i++) {
			auto& vert = vertlist.at(i);
            DRW_Coord v(vert->x, vert->y, elevation);
            extrudePoint(extPoint, &v);
            vert->x = v.x;
            vert->y = v.y;
        }
    }
}

void DRW_LWPolyline::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 10: {
		vertex = std::make_shared<DRW_Vertex2D>();
        vertlist.push_back(vertex);
        vertex->x = reader->getDouble();
        break; }
    case 20:
		if(vertex)
            vertex->y = reader->getDouble();
        break;
    case 40:
		if(vertex)
            vertex->start_width = reader->getDouble();
        break;
    case 41:
		if(vertex)
            vertex->end_width = reader->getDouble();
        break;
    case 42:
		if(vertex)
            vertex->bulge = reader->getDouble();
        break;
    case 38:
        elevation = reader->getDouble();
        break;
    case 39:
        thickness = reader->getDouble();
        break;
    case 43:
        width = reader->getDouble();
        break;
    case 70:
        flags = reader->getInt32();
        break;
    case 90:
        vertexnum = reader->getInt32();
        vertlist.reserve(vertexnum);
        break;
    case 210:
        haveExtrusion = true;
        extPoint.x = reader->getDouble();
        break;
    case 220:
        extPoint.y = reader->getDouble();
        break;
    case 230:
        extPoint.z = reader->getDouble();
        break;
    default:
        DRW_Entity::parseCode(code, reader);
        break;
    }
}

bool DRW_LWPolyline::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing LWPolyline *******************************************\n");

    flags = buf->getBitShort();
    drw_dbg("flags value: "); drw_dbg(flags);
    if (flags & 4)
        width = buf->getBitDouble();
    if (flags & 8)
        elevation = buf->getBitDouble();
    if (flags & 2)
        thickness = buf->getBitDouble();
    if (flags & 1)
        extPoint = buf->getExtrusion(false);
    vertexnum = buf->getBitLong();
    vertlist.reserve(vertexnum);
    unsigned int bulgesnum = 0;
    if (flags & 16)
        bulgesnum = buf->getBitLong();
    int vertexIdCount = 0;
    if (version > DRW::AC1021) {//2010+
        if (flags & 1024)
            vertexIdCount = buf->getBitLong();
    }

    unsigned int widthsnum = 0;
    if (flags & 32)
        widthsnum = buf->getBitLong();
    drw_dbg("\nvertex num: "); drw_dbg(vertexnum); drw_dbg(" bulges num: "); drw_dbg(bulgesnum);
    drw_dbg(" vertexIdCount: "); drw_dbg(vertexIdCount); drw_dbg(" widths num: "); drw_dbg(widthsnum);
    //clear all bit except 128 = plinegen and set 1 to open/close //RLZ:verify plinegen & open
    //dxf: plinegen 128 & open 1
    flags = (flags & 512)? (flags | 1):(flags | 0);
    flags &= 129;
    drw_dbg("end flags value: "); drw_dbg(flags);

    if (vertexnum > 0) { //verify if is lwpol without vertex (empty)
        // add vertexes
		vertex = std::make_shared<DRW_Vertex2D>();
        vertex->x = buf->getRawDouble();
        vertex->y = buf->getRawDouble();
        vertlist.push_back(vertex);
		auto pv = vertex;
        for (size_t i = 1; i< vertexnum; i++){
			vertex = std::make_shared<DRW_Vertex2D>();
			if (version < DRW::AC1015) {//14-
                vertex->x = buf->getRawDouble();
                vertex->y = buf->getRawDouble();
            } else {
//                DRW_Vertex2D *pv = vertlist.back();
                vertex->x = buf->getDefaultDouble(pv->x);
                vertex->y = buf->getDefaultDouble(pv->y);
            }
            pv = vertex;
            vertlist.push_back(vertex);
        }
        //add bulges
        for (unsigned int i = 0; i < bulgesnum; i++){
            double bulge = buf->getBitDouble();
            if (vertlist.size()> i)
                vertlist.at(i)->bulge = bulge;
        }
        //add vertexId
        if (version > DRW::AC1021) {//2010+
            for (int i = 0; i < vertexIdCount; i++){
                int32_t vertexId = buf->getBitLong();
                //TODO implement vertexId, do not exist in dxf
               (void)vertexId;
//                if (vertlist.size()< i)
//                    vertlist.at(i)->vertexId = vertexId;
            }
        }
        //add widths
        for (unsigned int i = 0; i < widthsnum; i++){
            double staW = buf->getBitDouble();
            double endW = buf->getBitDouble();
            if (vertlist.size()< i) {
                vertlist.at(i)->start_width = staW;
                vertlist.at(i)->end_width = endW;
            }
        }
    }
    if (drw_dbggl() == DRW::DebugLevel::Debug){
        drw_dbg("\nVertex list: ");
		for (auto& pv: vertlist) {
            drw_dbg("\n   x: "); drw_dbg(pv->x); drw_dbg(" y: "); drw_dbg(pv->y); drw_dbg(" bulge: "); drw_dbg(pv->bulge);
            drw_dbg(" stawidth: "); drw_dbg(pv->start_width); drw_dbg(" endwidth: "); drw_dbg(pv->end_width);
        }
    }

    drw_dbg("\n");
    /* Common Entity Handle Data */
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
    /* CRC X --- */
    return buf->isGood();
}


void DRW_Text::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 40:
        height = reader->getDouble();
        break;
    case 41:
        widthscale = reader->getDouble();
        break;
    case 50:
        angle = reader->getDouble();
        break;
    case 51:
        oblique = reader->getDouble();
        break;
    case 71:
        textgen = reader->getInt32();
        break;
    case 72:
        alignH = (HAlign)reader->getInt32();
        break;
    case 73:
        alignV = (VAlign)reader->getInt32();
        break;
    case 1:
        text = reader->getUtf8String();
        break;
    case 7:
        style = reader->getUtf8String();
        break;
    default:
        DRW_Line::parseCode(code, reader);
        break;
    }
}

bool DRW_Text::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing text *********************************************\n");

 // DataFlags RC Used to determine presence of subsequent data, set to 0xFF for R14-
    uint8_t data_flags = 0x00;
    if (version > DRW::AC1014) {//2000+
        data_flags = buf->getRawChar8(); /* DataFlags RC Used to determine presence of subsequent data */
        drw_dbg("data_flags: "); drw_dbg(data_flags); drw_dbg("\n");
        if ( !(data_flags & 0x01) ) { /* Elevation RD --- present if !(DataFlags & 0x01) */
            basePoint.z = buf->getRawDouble();
        }
    } else {//14-
        basePoint.z = buf->getBitDouble(); /* Elevation BD --- */
    }
    basePoint.x = buf->getRawDouble(); /* Insertion pt 2RD 10 */
    basePoint.y = buf->getRawDouble();
    drw_dbg("Insert point: "); drw_dbgpt(basePoint.x, basePoint.y, basePoint.z); drw_dbg("\n");
    if (version > DRW::AC1014) {//2000+
        if ( !(data_flags & 0x02) ) { /* Alignment pt 2DD 11 present if !(DataFlags & 0x02), use 10 & 20 values for 2 default values.*/
            secPoint.x = buf->getDefaultDouble(basePoint.x);
            secPoint.y = buf->getDefaultDouble(basePoint.y);
        } else {
            secPoint = basePoint;
        }
    } else {//14-
        secPoint.x = buf->getRawDouble();  /* Alignment pt 2RD 11 */
        secPoint.y = buf->getRawDouble();
    }
    secPoint.z = basePoint.z;
    drw_dbg("Alignment: "); drw_dbgpt(secPoint.x, secPoint.y, basePoint.z); drw_dbg("\n");
    extPoint = buf->getExtrusion(version > DRW::AC1014);
    drw_dbg("Extrusion: "); drw_dbgpt(extPoint.x, extPoint.y, extPoint.z); drw_dbg("\n");
    thickness = buf->getThickness(version > DRW::AC1014); /* Thickness BD 39 */

    if (version > DRW::AC1014) {//2000+
        if ( !(data_flags & 0x04) ) { /* Oblique ang RD 51 present if !(DataFlags & 0x04) */
            oblique = buf->getRawDouble();
        }
        if ( !(data_flags & 0x08) ) { /* Rotation ang RD 50 present if !(DataFlags & 0x08) */
            angle = buf->getRawDouble();
        }
        height = buf->getRawDouble(); /* Height RD 40 */
        if ( !(data_flags & 0x10) ) { /* Width factor RD 41 present if !(DataFlags & 0x10) */
            widthscale = buf->getRawDouble();
        }
    } else {//14-
        oblique = buf->getBitDouble(); /* Oblique ang BD 51 */
        angle = buf->getBitDouble(); /* Rotation ang BD 50 */
        height = buf->getBitDouble(); /* Height BD 40 */
        widthscale = buf->getBitDouble(); /* Width factor BD 41 */
    }
    angle *= ARAD;
    drw_dbg("thickness: "); drw_dbg(thickness); drw_dbg(", Oblique ang: "); drw_dbg(oblique); drw_dbg(", Width: ");
    drw_dbg(widthscale); drw_dbg(", Rotation: "); drw_dbg(angle); drw_dbg(", height: "); drw_dbg(height); drw_dbg("\n");
    text = sBuf->getVariableText(version, false); /* Text value TV 1 */
    drw_dbg("text string: "); drw_dbg(text.c_str());drw_dbg("\n");
    //textgen, alignH, alignV always present in R14-, data_flags set in initialisation
    if ( !(data_flags & 0x20) ) { /* Generation BS 71 present if !(DataFlags & 0x20) */
        textgen = buf->getBitShort();
        drw_dbg("textgen: "); drw_dbg(textgen);
    }
    if ( !(data_flags & 0x40) ) { /* Horiz align. BS 72 present if !(DataFlags & 0x40) */
        alignH = (HAlign)buf->getBitShort();
        drw_dbg(", alignH: "); drw_dbg(alignH);
    }
    if ( !(data_flags & 0x80) ) { /* Vert align. BS 73 present if !(DataFlags & 0x80) */
        alignV = (VAlign)buf->getBitShort();
        drw_dbg(", alignV: "); drw_dbg(alignV);
    }
    drw_dbg("\n");

    /* Common Entity Handle Data */
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;

    styleH = buf->getHandle(); /* H 7 STYLE (hard pointer) */
    drw_dbg("text style Handle: "); drw_dbghl(styleH.code, styleH.size, styleH.ref); drw_dbg("\n");

    /* CRC X --- */
    return buf->isGood();
}

void DRW_MText::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 1:
        text += reader->getString();
        text = reader->toUtf8String(text);
        break;
    case 11:
        hasXAxisVec = true;
        DRW_Text::parseCode(code, reader);
        break;
    case 3:
        text += reader->getString();
        break;
    case 44:
        interlin = reader->getDouble();
        break;
    case 50: // djm: per dxf docs, last of code 11 or code 50 prevails
        hasXAxisVec = false;
        angle = reader->getDouble();
        break;
    default:
        DRW_Text::parseCode(code, reader);
        break;
    }
}

bool DRW_MText::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing mtext *********************************************\n");

    basePoint = buf->get3BitDouble(); /* Insertion pt 3BD 10 - First picked point. */
    drw_dbg("Insertion: "); drw_dbgpt(basePoint.x, basePoint.y, basePoint.z); drw_dbg("\n");
    extPoint = buf->get3BitDouble(); /* Extrusion 3BD 210 Undocumented; */
    secPoint = buf->get3BitDouble(); /* X-axis dir 3BD 11 */
    hasXAxisVec = true;
    updateAngle();
    widthscale = buf->getBitDouble(); /* Rect width BD 41 */
    if (version > DRW::AC1018) {//2007+
        /* Rect height BD 46 Reference rectangle height. */
        /** @todo */buf->getBitDouble();
    }
    height = buf->getBitDouble();/* Text height BD 40 Undocumented */
    textgen = buf->getBitShort(); /* Attachment BS 71 Similar to justification; */
    /* Drawing dir BS 72 Left to right, etc.; see DXF doc */
    int16_t draw_dir = buf->getBitShort();
   (void)draw_dir;
    /* Extents ht BD Undocumented and not present in DXF or entget */
    double ext_ht = buf->getBitDouble();
   (void)ext_ht;
    /* Extents wid BD Undocumented and not present in DXF or entget The extents
    rectangle, when rotated the same as the text, fits the actual text image on
    the screen (although we've seen it include an extra row of text in height). */
    double ext_wid = buf->getBitDouble();
   (void)ext_wid;
    /* Text TV 1 All text in one long string (without '\n's 3 for line wrapping).
    ACAD seems to add braces ({ }) and backslash-P's to indicate paragraphs
    based on the "\r\n"'s found in the imported file. But, all the text is in
    this one long string -- not broken into 1- and 3-groups as in DXF and
    entget. ACAD's entget breaks this string into 250-char pieces (not 255 as
    doc'd) – even if it's mid-word. The 1-group always gets the tag end;
    therefore, the 3's are always 250 chars long. */
    text = sBuf->getVariableText(version, false); /* Text value TV 1 */
    if (version > DRW::AC1014) {//2000+
        buf->getBitShort();/* Linespacing Style BS 73 */
        buf->getBitDouble();/* Linespacing Factor BD 44 */
        buf->getBit();/* Unknown bit B */
    }
    if (version > DRW::AC1015) {//2004+
        /* Background flags BL 0 = no background, 1 = background fill, 2 =background
        fill with drawing fill color. */
        int32_t bk_flags = buf->getBitLong(); /** @todo add to DRW_MText */
        if ( bk_flags == 1 ) {
            /* Background scale factor BL Present if background flags = 1, default = 1.5*/
            buf->getBitLong();
            /* Background color CMC Present if background flags = 1 */
            buf->getCmColor(version); //RLZ: warning CMC or ENC
            /** @todo buf->getCMC */
            /* Background transparency BL Present if background flags = 1 */
            buf->getBitLong();
        }
    }

    /* Common Entity Handle Data */
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;

    styleH = buf->getHandle(); /* H 7 STYLE (hard pointer) */
    drw_dbg("text style Handle: "); drw_dbg(styleH.code); drw_dbg(".");
    drw_dbg(styleH.size); drw_dbg("."); drw_dbg(styleH.ref); drw_dbg("\n");

    /* CRC X --- */
    return buf->isGood();
}

void DRW_MText::updateAngle() {
    if (hasXAxisVec) {
        angle = atan2(secPoint.y, secPoint.x) * ARAD;
    }
}

void DRW_Polyline::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 70:
        flags = reader->getInt32();
        break;
    case 40:
        defstawidth = reader->getDouble();
        break;
    case 41:
        defendwidth = reader->getDouble();
        break;
    case 71:
        vertexcount = reader->getInt32();
        break;
    case 72:
        facecount = reader->getInt32();
        break;
    case 73:
        smoothM = reader->getInt32();
        break;
    case 74:
        smoothN = reader->getInt32();
        break;
    case 75:
        curvetype = reader->getInt32();
        break;
    default:
        DRW_Point::parseCode(code, reader);
        break;
    }
}

//0x0F polyline 2D bit 4(8) & 5(16) NOT set
//0x10 polyline 3D bit 4(8) set
//0x1D PFACE bit 5(16) set
bool DRW_Polyline::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing polyline *********************************************\n");

    int32_t ooCount = 0;
    if (oType == 0x0F) { //pline 2D
        flags = buf->getBitShort();
        drw_dbg("flags value: "); drw_dbg(flags);
        curvetype = buf->getBitShort();
        defstawidth = buf->getBitDouble();
        defendwidth = buf->getBitDouble();
        thickness = buf->getThickness(version > DRW::AC1014);
        basePoint = DRW_Coord(0,0,buf->getBitDouble());
        extPoint = buf->getExtrusion(version > DRW::AC1014);
    } else if (oType == 0x10) { //pline 3D
        uint8_t tmpFlag = buf->getRawChar8();
        drw_dbg("flags 1 value: "); drw_dbg(tmpFlag);
        if (tmpFlag & 1)
            curvetype = 5;
        else if (tmpFlag & 2)
            curvetype = 6;
        if (tmpFlag & 3) {
            curvetype = 8;
            flags |= 4;
        }
        tmpFlag = buf->getRawChar8();
        if (tmpFlag & 1)
            flags |= 1;
        flags |= 8; //indicate 3DPOL
        drw_dbg("flags 2 value: "); drw_dbg(tmpFlag);
    } else if (oType == 0x1D) { //PFACE
        flags = 64;
        vertexcount = buf->getBitShort();
        drw_dbg("vertex count: "); drw_dbg(vertexcount);
        facecount = buf->getBitShort();
        drw_dbg("face count: "); drw_dbg(facecount);
        drw_dbg("flags value: "); drw_dbg(flags);
    }
    if (version > DRW::AC1015){ //2004+
        ooCount = buf->getBitLong();
    }

    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;

    if (version < DRW::AC1018){ //2000-
        dwgHandle objectH = buf->getOffsetHandle(handle);
        firstEH = objectH.ref;
        drw_dbg(" first Vertex Handle: "); drw_dbghl(objectH.code, objectH.size, objectH.ref); drw_dbg("\n");
        objectH = buf->getOffsetHandle(handle);
        lastEH = objectH.ref;
        drw_dbg(" last Vertex Handle: "); drw_dbghl(objectH.code, objectH.size, objectH.ref); drw_dbg("\n");
        drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    } else {
        for (int32_t i = 0; i < ooCount; ++i){
                dwgHandle objectH = buf->getOffsetHandle(handle);
                hadlesList.push_back (objectH.ref);
                drw_dbg(" Vertex Handle: "); drw_dbghl(objectH.code, objectH.size, objectH.ref); drw_dbg("\n");
                drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
        }
    }
    seqEndH = buf->getOffsetHandle(handle);
    drw_dbg(" SEQEND Handle: "); drw_dbghl(seqEndH.code, seqEndH.size, seqEndH.ref); drw_dbg("\n");
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");

//    RS crc;   //RS */
    return buf->isGood();
}

void DRW_Vertex::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 70:
        flags = reader->getInt32();
        break;
    case 40:
        stawidth = reader->getDouble();
        break;
    case 41:
        endwidth = reader->getDouble();
        break;
    case 42:
        bulge = reader->getDouble();
        break;
    case 50:
        tgdir = reader->getDouble();
        break;
    case 71:
        vindex1 = reader->getInt32();
        break;
    case 72:
        vindex2 = reader->getInt32();
        break;
    case 73:
        vindex3 = reader->getInt32();
        break;
    case 74:
        vindex4 = reader->getInt32();
        break;
    case 91:
        identifier = reader->getInt32();
        break;
    default:
        DRW_Point::parseCode(code, reader);
        break;
    }
}

//0x0A vertex 2D
//0x0B vertex 3D
//0x0C MESH
//0x0D PFACE
//0x0E PFACE FACE
bool DRW_Vertex::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs, double el){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing pline Vertex *********************************************\n");

    if (oType == 0x0A) { //pline 2D, needed example
        flags = buf->getRawChar8(); //RLZ: EC  unknown type
        drw_dbg("flags value: "); drw_dbg(flags);
        basePoint = buf->get3BitDouble();
        basePoint.z = el;
        drw_dbg("basePoint: "); drw_dbgpt(basePoint.x, basePoint.y, basePoint.z);
        stawidth = buf->getBitDouble();
        if (stawidth < 0) { endwidth = stawidth = fabs(stawidth); }
        else
            endwidth = buf->getBitDouble();
        bulge = buf->getBitDouble();
        if (version > DRW::AC1021) {
            //2010+
            drw_dbg("Vertex ID: ");
            drw_dbg(buf->getBitLong());
        }
        tgdir = buf->getBitDouble();
    } else if (oType == 0x0B || oType == 0x0C || oType == 0x0D) { //PFACE
        flags = buf->getRawChar8(); //RLZ: EC  unknown type
        drw_dbg("flags value: "); drw_dbg(flags);
        basePoint = buf->get3BitDouble();
        drw_dbg("basePoint: "); drw_dbgpt(basePoint.x, basePoint.y, basePoint.z);
    } else if (oType == 0x0E) { //PFACE FACE
        vindex1 = buf->getBitShort();
        vindex2 = buf->getBitShort();
        vindex3 = buf->getBitShort();
        vindex4 = buf->getBitShort();
    }

    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
//    RS crc;   //RS */
    return buf->isGood();
}

void DRW_Hatch::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 2:
        name = reader->getUtf8String();
        break;
    case 70:
        solid = reader->getInt32();
        break;
    case 71:
        associative = reader->getInt32();
        break;
    case 72:        /*edge type*/
        if (ispol){ //if is polyline is a as_bulge flag
            break;
        } else if (reader->getInt32() == 1){ //line
            addLine();
        } else if (reader->getInt32() == 2){ //arc
            addArc();
        } else if (reader->getInt32() == 3){ //elliptic arc
            addEllipse();
        } else if (reader->getInt32() == 4){ //spline
            addSpline();
        }
        break;
    case 10:
        if (pt) pt->basePoint.x = reader->getDouble();
        else if (pline) {
            plvert = pline->addVertex();
            plvert->x = reader->getDouble();
        }
        break;
    case 20:
        if (pt) pt->basePoint.y = reader->getDouble();
        else if (plvert) plvert ->y = reader->getDouble();
        break;
    case 11:
        if (line) line->secPoint.x = reader->getDouble();
        else if (ellipse) ellipse->secPoint.x = reader->getDouble();
        break;
    case 21:
        if (line) line->secPoint.y = reader->getDouble();
        else if (ellipse) ellipse->secPoint.y = reader->getDouble();
        break;
    case 40:
        if (arc) arc->radious = reader->getDouble();
        else if (ellipse) ellipse->ratio = reader->getDouble();
        break;
    case 41:
        scale = reader->getDouble();
        break;
    case 42:
        if (plvert) plvert ->bulge = reader->getDouble();
        break;
    case 50:
        if (arc) arc->staangle = reader->getDouble()/ARAD;
        else if (ellipse) ellipse->staparam = reader->getDouble()/ARAD;
        break;
    case 51:
        if (arc) arc->endangle = reader->getDouble()/ARAD;
        else if (ellipse) ellipse->endparam = reader->getDouble()/ARAD;
        break;
    case 52:
        angle = reader->getDouble();
        break;
    case 73:
        if (arc) arc->isccw = reader->getInt32();
        else if (pline) pline->flags = reader->getInt32();
        break;
    case 75:
        hstyle = reader->getInt32();
        break;
    case 76:
        hpattern = reader->getInt32();
        break;
    case 77:
        doubleflag = reader->getInt32();
        break;
    case 78:
        deflines = reader->getInt32();
        break;
    case 91:
        loopsnum = reader->getInt32();
        looplist.reserve(loopsnum);
        break;
    case 92:
        loop = std::make_shared<DRW_HatchLoop>(reader->getInt32());
        looplist.push_back(loop);
        if (reader->getInt32() & 2) {
            ispol = true;
            clearEntities();
            pline = std::make_shared<DRW_LWPolyline>();
            loop->objlist.push_back(pline);
        } else ispol = false;
        break;
    case 93:
        if (pline) pline->vertexnum = reader->getInt32();
        else loop->numedges = reader->getInt32();//aqui reserve
        break;
    case 98: //seed points ??
        clearEntities();
        break;
    default:
        DRW_Point::parseCode(code, reader);
        break;
    }
}

bool DRW_Hatch::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    uint32_t totalBoundItems = 0;
    bool havePixelSize = false;

    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing hatch *********************************************\n");

    //Gradient data, RLZ: is ok or if grad > 0 continue read ?
    if (version > DRW::AC1015) { //2004+
        int32_t isGradient = buf->getBitLong();
        drw_dbg("is Gradient: "); drw_dbg(isGradient);
        int32_t res = buf->getBitLong();
        drw_dbg(" reserved: "); drw_dbg(res);
        double gradAngle = buf->getBitDouble();
        drw_dbg(" Gradient angle: "); drw_dbg(gradAngle);
        double gradShift = buf->getBitDouble();
        drw_dbg(" Gradient shift: "); drw_dbg(gradShift);
        int32_t singleCol = buf->getBitLong();
        drw_dbg("\nsingle color Grad: "); drw_dbg(singleCol);
        double gradTint = buf->getBitDouble();
        drw_dbg(" Gradient tint: "); drw_dbg(gradTint);
        int32_t numCol = buf->getBitLong();
        drw_dbg(" num colors: "); drw_dbg(numCol);
        for (int32_t i = 0 ; i < numCol; ++i){
            double unkDouble = buf->getBitDouble();
            drw_dbg("\nunkDouble: "); drw_dbg(unkDouble);
            uint16_t unkShort = buf->getBitShort();
            drw_dbg(" unkShort: "); drw_dbg(unkShort);
            int32_t rgbCol = buf->getBitLong();
            drw_dbg(" rgb color: "); drw_dbg(rgbCol);
            uint8_t ignCol = buf->getRawChar8();
            drw_dbg(" ignored color: "); drw_dbg(ignCol);
        }
        std::string gradName = sBuf->getVariableText(version, false);
        drw_dbg("\ngradient name: "); drw_dbg(gradName.c_str()); drw_dbg("\n");
    }
    basePoint.z = buf->getBitDouble();
    extPoint = buf->get3BitDouble();
    drw_dbg("base point: "); drw_dbgpt(basePoint.x, basePoint.y, basePoint.z);
    drw_dbg("\nextrusion: "); drw_dbgpt(extPoint.x, extPoint.y, extPoint.z);
    name = sBuf->getVariableText(version, false);
    drw_dbg("\nhatch pattern name: "); drw_dbg(name.c_str()); drw_dbg("\n");
    solid = buf->getBit();
    associative = buf->getBit();
    loopsnum = buf->getBitLong();

    //read loops
    for (size_t i = 0 ; i < loopsnum; ++i){
        loop = std::make_shared<DRW_HatchLoop>(buf->getBitLong());
        havePixelSize |= loop->type & 4;
        if (!(loop->type & 2)){ //Not polyline
            int32_t numPathSeg = buf->getBitLong();
            for (int32_t j = 0; j<numPathSeg;++j){
                uint8_t typePath = buf->getRawChar8();
                if (typePath == 1){ //line
                    addLine();
                    line->basePoint = buf->get2RawDouble();
                    line->secPoint = buf->get2RawDouble();
                } else if (typePath == 2){ //circle arc
                    addArc();
                    arc->basePoint = buf->get2RawDouble();
                    arc->radious = buf->getBitDouble();
                    arc->staangle = buf->getBitDouble();
                    arc->endangle = buf->getBitDouble();
                    arc->isccw = buf->getBit();
                } else if (typePath == 3){ //ellipse arc
                    addEllipse();
                    ellipse->basePoint = buf->get2RawDouble();
                    ellipse->secPoint = buf->get2RawDouble();
                    ellipse->ratio = buf->getBitDouble();
                    ellipse->staparam = buf->getBitDouble();
                    ellipse->endparam = buf->getBitDouble();
                    ellipse->isccw = buf->getBit();
                } else if (typePath == 4){ //spline
                    addSpline();
                    spline->degree = buf->getBitLong();
                    bool isRational = buf->getBit();
                    spline->flags |= (isRational << 2); //rational
                    spline->flags |= (buf->getBit() << 1); //periodic
                    spline->nknots = buf->getBitLong();
                    spline->knotslist.reserve(spline->nknots);
                    spline->ncontrol = buf->getBitLong();
                    spline->controllist.reserve(spline->ncontrol);
                    for (int32_t j = 0; j < spline->nknots;++j){
                        spline->knotslist.push_back (buf->getBitDouble());
                    }
                    for (int32_t j = 0; j < spline->ncontrol;++j){
                        std::shared_ptr<DRW_Coord> crd = std::make_shared<DRW_Coord>(buf->get2RawDouble());
                        if(isRational)
                            crd->z =  buf->getBitDouble(); //RLZ: investigate how store weight
                        spline->controllist.push_back(crd);
                    }
                    if (version > DRW::AC1021) { //2010+
                        spline->nfit = buf->getBitLong();
                        spline->fitlist.reserve(spline->nfit);
                        for (int32_t j = 0; j < spline->nfit;++j){
                            std::shared_ptr<DRW_Coord> crd = std::make_shared<DRW_Coord>(buf->get2RawDouble());
                            spline->fitlist.push_back(crd);
                        }
                        spline->tgStart = buf->get2RawDouble();
                        spline->tgEnd = buf->get2RawDouble();
                    }
                }
            }
        } else { //end not pline, start polyline
            pline = std::make_shared<DRW_LWPolyline>();
            bool asBulge = buf->getBit();
            pline->flags = buf->getBit();//closed bit
            int32_t numVert = buf->getBitLong();
            for (int32_t j = 0; j<numVert;++j){
                DRW_Vertex2D v;
                v.x = buf->getRawDouble();
                v.y = buf->getRawDouble();
                if (asBulge)
                    v.bulge = buf->getBitDouble();
                pline->addVertex(v);
            }
            loop->objlist.push_back(pline);
        }//end polyline
        loop->update();
        looplist.push_back(loop);
        totalBoundItems += buf->getBitLong();
        drw_dbg(" totalBoundItems: "); drw_dbg(totalBoundItems);
    } //end read loops

    hstyle = buf->getBitShort();
    hpattern = buf->getBitShort();
    drw_dbg("\nhatch style: "); drw_dbg(hstyle); drw_dbg(" pattern type"); drw_dbg(hpattern);
    if (!solid){
        angle = buf->getBitDouble();
        scale = buf->getBitDouble();
        doubleflag = buf->getBit();
        deflines = buf->getBitShort();
        for (int32_t i = 0 ; i < deflines; ++i){
            DRW_Coord ptL, offL;
            double angleL = buf->getBitDouble();
            ptL.x = buf->getBitDouble();
            ptL.y = buf->getBitDouble();
            offL.x = buf->getBitDouble();
            offL.y = buf->getBitDouble();
            uint16_t numDashL = buf->getBitShort();
            drw_dbg("\ndef line: "); drw_dbg(angleL); drw_dbg(","); drw_dbg(ptL.x); drw_dbg(","); drw_dbg(ptL.y);
            drw_dbg(","); drw_dbg(offL.x); drw_dbg(","); drw_dbg(offL.y); drw_dbg(","); drw_dbg(angleL);
            for (uint16_t i = 0 ; i < numDashL; ++i){
                double lengthL = buf->getBitDouble();
                drw_dbg(","); drw_dbg(lengthL);
            }
        }//end deflines
    } //end not solid

    if (havePixelSize){
        double pixsize = buf->getBitDouble();
        drw_dbg("\npixel size: "); drw_dbg(pixsize);
    }
    int32_t numSeedPoints = buf->getBitLong();
    drw_dbg("\nnum Seed Points  "); drw_dbg(numSeedPoints);
    //read Seed Points
    DRW_Coord seedPt;
    for (int32_t i = 0 ; i < numSeedPoints; ++i){
        seedPt.x = buf->getRawDouble();
        seedPt.y = buf->getRawDouble();
        drw_dbg("\n  "); drw_dbg(seedPt.x); drw_dbg(","); drw_dbg(seedPt.y);
    }

    drw_dbg("\n");
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");

    for (uint32_t i = 0 ; i < totalBoundItems; ++i){
        dwgHandle biH = buf->getHandle();
        drw_dbg("Boundary Items Handle: "); drw_dbghl(biH.code, biH.size, biH.ref);
    }
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
//    RS crc;   //RS */
    return buf->isGood();
}

void DRW_Spline::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 210:
        normalVec.x = reader->getDouble();
        break;
    case 220:
        normalVec.y = reader->getDouble();
        break;
    case 230:
        normalVec.z = reader->getDouble();
        break;
    case 12:
        tgStart.x = reader->getDouble();
        break;
    case 22:
        tgStart.y = reader->getDouble();
        break;
    case 32:
        tgStart.z = reader->getDouble();
        break;
    case 13:
        tgEnd.x = reader->getDouble();
        break;
    case 23:
        tgEnd.y = reader->getDouble();
        break;
    case 33:
        tgEnd.z = reader->getDouble();
        break;
    case 70:
        flags = reader->getInt32();
        break;
    case 71:
        degree = reader->getInt32();
        break;
    case 72:
        nknots = reader->getInt32();
        break;
    case 73:
        ncontrol = reader->getInt32();
        break;
    case 74:
        nfit = reader->getInt32();
        break;
    case 42:
        tolknot = reader->getDouble();
        break;
    case 43:
        tolcontrol = reader->getDouble();
        break;
    case 44:
        tolfit = reader->getDouble();
        break;
    case 10: {
        controlpoint = std::make_shared<DRW_Coord>();
        controllist.push_back(controlpoint);
        controlpoint->x = reader->getDouble();
        break; }
    case 20:
        if(controlpoint)
            controlpoint->y = reader->getDouble();
        break;
    case 30:
        if(controlpoint)
            controlpoint->z = reader->getDouble();
        break;
    case 11: {
        fitpoint = std::make_shared<DRW_Coord>();
        fitlist.push_back(fitpoint);
        fitpoint->x = reader->getDouble();
        break; }
    case 21:
        if(fitpoint)
            fitpoint->y = reader->getDouble();
        break;
    case 31:
        if(fitpoint)
            fitpoint->z = reader->getDouble();
        break;
    case 40:
        knotslist.push_back(reader->getDouble());
        break;
    case 41:
        weightlist.push_back(reader->getDouble());
        break;
    default:
        DRW_Entity::parseCode(code, reader);
        break;
    }
}

bool DRW_Spline::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing spline *********************************************\n");
    uint8_t weight = 0; // RLZ ??? flags, weight, code 70, bit 4 (16)

    int32_t scenario = buf->getBitLong();
    drw_dbg("scenario: "); drw_dbg(scenario);
    if (version > DRW::AC1024) {
        int32_t splFlag1 = buf->getBitLong();
        if (splFlag1 & 1)
            scenario = 2;
        int32_t knotParam = buf->getBitLong();
        drw_dbg(" 2013 splFlag1: "); drw_dbg(splFlag1);
        drw_dbg(" 2013 knotParam: "); drw_dbg(knotParam);
//        drw_dbg("unk bit: "); drw_dbg(buf->getBit());
    }
    degree = buf->getBitLong(); //RLZ: code 71, verify with dxf
    drw_dbg(" degree: "); drw_dbg(degree); drw_dbg("\n");
    if (scenario == 2) {
        flags = 8;//scenario 2 = not rational & planar
        tolfit = buf->getBitDouble();//BD
        drw_dbg("flags: "); drw_dbg(flags); drw_dbg(" tolfit: "); drw_dbg(tolfit);
        tgStart =buf->get3BitDouble();
        drw_dbg(" Start Tangent: "); drw_dbgpt(tgStart.x, tgStart.y, tgStart.z);
        tgEnd =buf->get3BitDouble();
        drw_dbg("\nEnd Tangent: "); drw_dbgpt(tgEnd.x, tgEnd.y, tgEnd.z);
        nfit = buf->getBitLong();
        drw_dbg("\nnumber of fit points: "); drw_dbg(nfit);
    } else if (scenario == 1) {
        flags = 8;//scenario 1 = rational & planar
        flags |= buf->getBit() << 2; //flags, rational, code 70, bit 2 (4)
        flags |= buf->getBit(); //flags, closed, code 70, bit 0 (1)
        flags |= buf->getBit() << 1; //flags, periodic, code 70, bit 1 (2)
        tolknot = buf->getBitDouble();
        tolcontrol = buf->getBitDouble();
        drw_dbg("flags: "); drw_dbg(flags); drw_dbg(" knot tolerance: "); drw_dbg(tolknot);
        drw_dbg(" control point tolerance: "); drw_dbg(tolcontrol);
        nknots = buf->getBitLong();
        ncontrol = buf->getBitLong();
        weight = buf->getBit(); // RLZ ??? flags, weight, code 70, bit 4 (16)
        drw_dbg("\nnum of knots: "); drw_dbg(nknots); drw_dbg(" num of control pt: ");
        drw_dbg(ncontrol); drw_dbg(" weight bit: "); drw_dbg(weight);
    } else {
        drw_dbg("\ndwg Ellipse, unknouwn scenario\n");
        return false; //RLZ: from doc only 1 or 2 are ok ?
    }

    knotslist.reserve(nknots);
    for (int32_t i= 0; i<nknots; ++i){
        knotslist.push_back (buf->getBitDouble());
    }
    controllist.reserve(ncontrol);
    for (int32_t i= 0; i<ncontrol; ++i){
        controllist.push_back(std::make_shared<DRW_Coord>(buf->get3BitDouble()));
        if (weight) {
            drw_dbg("\n w: ");
            drw_dbg(buf->getBitDouble()); //RLZ Warning: D (BD or RD)
        }
    }
    fitlist.reserve(nfit);
    for (int32_t i= 0; i<nfit; ++i)
        fitlist.push_back(std::make_shared<DRW_Coord>(buf->get3BitDouble()));

    if (drw_dbggl() == DRW::DebugLevel::Debug) {
        drw_dbg("\nknots list: ");
        for (auto const& v: knotslist) {
            drw_dbg("\n"); drw_dbg(v);
        }
        drw_dbg("\ncontrol point list: ");
        for (auto const& v: controllist) {
            drw_dbg("\n"); drw_dbgpt(v->x, v->y, v->z);
        }
        drw_dbg("\nfit point list: ");
        for (auto const& v: fitlist) {
            drw_dbg("\n"); drw_dbgpt(v->x, v->y, v->z);
        }
    }

    /* Common Entity Handle Data */
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
//    RS crc;   //RS */
    return buf->isGood();
}

void DRW_Image::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 12:
        vVector.x = reader->getDouble();
        break;
    case 22:
        vVector.y = reader->getDouble();
        break;
    case 32:
        vVector.z = reader->getDouble();
        break;
    case 13:
        sizeu = reader->getDouble();
        break;
    case 23:
        sizev = reader->getDouble();
        break;
    case 340:
        ref = reader->getHandleString();
        break;
    case 280:
        clip = reader->getInt32();
        break;
    case 281:
        brightness = reader->getInt32();
        break;
    case 282:
        contrast = reader->getInt32();
        break;
    case 283:
        fade = reader->getInt32();
        break;
    default:
        DRW_Line::parseCode(code, reader);
        break;
    }
}

bool DRW_Image::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing image *********************************************\n");

    int32_t classVersion = buf->getBitLong();
    drw_dbg("class Version: "); drw_dbg(classVersion);
    basePoint = buf->get3BitDouble();
    drw_dbg("\nbase point: "); drw_dbgpt(basePoint.x, basePoint.y, basePoint.z);
    secPoint = buf->get3BitDouble();
    drw_dbg("\nU vector: "); drw_dbgpt(secPoint.x, secPoint.y, secPoint.z);
    vVector = buf->get3BitDouble();
    drw_dbg("\nV vector: "); drw_dbgpt(vVector.x, vVector.y, vVector.z);
    sizeu = buf->getRawDouble();
    sizev = buf->getRawDouble();
    drw_dbg("\nsize U: "); drw_dbg(sizeu); drw_dbg("\nsize V: "); drw_dbg(sizev);
    uint16_t displayProps = buf->getBitShort();
   (void)displayProps;//RLZ: temporary, complete API
    clip = buf->getBit();
    brightness = buf->getRawChar8();
    contrast = buf->getRawChar8();
    fade = buf->getRawChar8();
    if (version > DRW::AC1021){ //2010+
        bool clipMode = buf->getBit();
       (void)clipMode;//RLZ: temporary, complete API
    }
    uint16_t clipType = buf->getBitShort();
    if (clipType == 1){
        buf->get2RawDouble();
        buf->get2RawDouble();
    } else { //clipType == 2
        int32_t numVerts = buf->getBitLong();
        for (int i= 0; i< numVerts;++i)
            buf->get2RawDouble();
    }

    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");

    dwgHandle biH = buf->getHandle();
    drw_dbg("ImageDef Handle: "); drw_dbghl(biH.code, biH.size, biH.ref);
    ref = biH.ref;
    biH = buf->getHandle();
    drw_dbg("ImageDefReactor Handle: "); drw_dbghl(biH.code, biH.size, biH.ref);
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
//    RS crc;   //RS */
    return buf->isGood();
}

void DRW_Dimension::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 1:
        text = reader->getUtf8String();
        break;
    case 2:
        name = reader->getString();
        break;
    case 3:
        style = reader->getUtf8String();
        break;
    case 70:
        type = reader->getInt32();
        break;
    case 71:
        align = reader->getInt32();
        break;
    case 72:
        linesty = reader->getInt32();
        break;
    case 10:
        defPoint.x = reader->getDouble();
        break;
    case 20:
        defPoint.y = reader->getDouble();
        break;
    case 30:
        defPoint.z = reader->getDouble();
        break;
    case 11:
        textPoint.x = reader->getDouble();
        break;
    case 21:
        textPoint.y = reader->getDouble();
        break;
    case 31:
        textPoint.z = reader->getDouble();
        break;
    case 12:
        clonePoint.x = reader->getDouble();
        break;
    case 22:
        clonePoint.y = reader->getDouble();
        break;
    case 32:
        clonePoint.z = reader->getDouble();
        break;
    case 13:
        def1.x = reader->getDouble();
        break;
    case 23:
        def1.y = reader->getDouble();
        break;
    case 33:
        def1.z = reader->getDouble();
        break;
    case 14:
        def2.x = reader->getDouble();
        break;
    case 24:
        def2.y = reader->getDouble();
        break;
    case 34:
        def2.z = reader->getDouble();
        break;
    case 15:
        circlePoint.x = reader->getDouble();
        break;
    case 25:
        circlePoint.y = reader->getDouble();
        break;
    case 35:
        circlePoint.z = reader->getDouble();
        break;
    case 16:
        arcPoint.x = reader->getDouble();
        break;
    case 26:
        arcPoint.y = reader->getDouble();
        break;
    case 36:
        arcPoint.z = reader->getDouble();
        break;
    case 41:
        linefactor = reader->getDouble();
        break;
    case 53:
        rot = reader->getDouble();
        break;
    case 50:
        angle = reader->getDouble();
        break;
    case 52:
        oblique = reader->getDouble();
        break;
    case 40:
        length = reader->getDouble();
        break;
    case 51:
        hdir = reader->getDouble();
        break;
    default:
        DRW_Entity::parseCode(code, reader);
        break;
    }
}

bool DRW_Dimension::parseDwg(DRW::Version version, dwgBuffer *buf, dwgBuffer *sBuf){
    drw_dbg("\n***************************** parsing dimension *********************************************");
    if (version > DRW::AC1021) { //2010+
        uint8_t dimVersion = buf->getRawChar8();
        drw_dbg("\ndimVersion: "); drw_dbg(dimVersion);
    }
    extPoint = buf->getExtrusion(version > DRW::AC1014);
    drw_dbg("\nextPoint: "); drw_dbgpt(extPoint.x, extPoint.y, extPoint.z);
    if (version > DRW::AC1014) { //2000+
        drw_dbg("\nFive unknown bits: "); drw_dbg(buf->getBit()); drw_dbg(buf->getBit());
        drw_dbg(buf->getBit()); drw_dbg(buf->getBit()); drw_dbg(buf->getBit());
    }
    textPoint.x = buf->getRawDouble();
    textPoint.y = buf->getRawDouble();
    textPoint.z = buf->getBitDouble();
    drw_dbg("\ntextPoint: "); drw_dbgpt(textPoint.x, textPoint.y, textPoint.z);
    type = buf->getRawChar8();
    drw_dbg("\ntype (70) read: "); drw_dbg(type);
    type =  (type & 1) ? type & 0x7F : type | 0x80; //set bit 7
    type =  (type & 2) ? type | 0x20 : type & 0xDF; //set bit 5
    drw_dbg(" type (70) set: "); drw_dbg(type);
    //clear last 3 bits to set integer dim type
    type &= 0xF8;
    text = sBuf->getVariableText(version, false);
    drw_dbg("\nforced dim text: "); drw_dbg(text.c_str());
    rot = buf->getBitDouble();
    hdir = buf->getBitDouble();
    DRW_Coord inspoint = buf->get3BitDouble();
    drw_dbg("\ninspoint: "); drw_dbgpt(inspoint.x, inspoint.y, inspoint.z);
    double insRot_code54 = buf->getBitDouble(); //RLZ: unknown, investigate
    drw_dbg(" insRot_code54: "); drw_dbg(insRot_code54);
    if (version > DRW::AC1014) { //2000+
        align = buf->getBitShort();
        linesty = buf->getBitShort();
        linefactor = buf->getBitDouble();
        double actMeas = buf->getBitDouble();
        drw_dbg("\n  actMeas_code42: "); drw_dbg(actMeas);
        if (version > DRW::AC1018) { //2007+
            bool unk = buf->getBit();
            bool flip1 = buf->getBit();
            bool flip2 = buf->getBit();
            drw_dbg("\n2007, unk, flip1, flip2: "); drw_dbg(unk); drw_dbg(flip1); drw_dbg(flip2);
        }
    }
    clonePoint.x = buf->getRawDouble();
    clonePoint.y = buf->getRawDouble();
    drw_dbg("\nclonePoint: "); drw_dbgpt(clonePoint.x, clonePoint.y, clonePoint.z);

    return buf->isGood();
}

bool DRW_DimAligned::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    ret = DRW_Dimension::parseDwg(version, buf, sBuf);
    if (!ret)
        return ret;
    if (oType == 0x15)
        drw_dbg("\n***************************** parsing dim linear *********************************************\n");
    else
        drw_dbg("\n***************************** parsing dim aligned *********************************************\n");
    DRW_Coord pt = buf->get3BitDouble();
    setPt3(pt); //def1
    drw_dbg("def1: "); drw_dbgpt(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt4(pt);
    drw_dbg("\ndef2: "); drw_dbgpt(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setDefPoint(pt);
    drw_dbg("\ndefPoint: "); drw_dbgpt(pt.x, pt.y, pt.z);
    setOb52(buf->getBitDouble());
    if (oType == 0x15)
        setAn50(buf->getBitDouble() * ARAD);
    else
        type |= 1;
    drw_dbg("\n  type (70) final: "); drw_dbg(type); drw_dbg("\n");

    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    if (!ret)
        return ret;
    dimStyleH = buf->getHandle();
    drw_dbg("dim style Handle: "); drw_dbghl(dimStyleH.code, dimStyleH.size, dimStyleH.ref); drw_dbg("\n");
    blockH = buf->getHandle(); /* H 7 STYLE (hard pointer) */
    drw_dbg("anon block Handle: "); drw_dbghl(blockH.code, blockH.size, blockH.ref); drw_dbg("\n");
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");

    //    RS crc;   //RS */
    return buf->isGood();
 }

 bool DRW_DimRadial::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
     dwgBuffer sBuff = *buf;
     dwgBuffer *sBuf = buf;
     if (version > DRW::AC1018) {//2007+
         sBuf = &sBuff; //separate buffer for strings
     }
     bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
     if (!ret)
         return ret;
     ret = DRW_Dimension::parseDwg(version, buf, sBuf);
     if (!ret)
         return ret;
     drw_dbg("\n***************************** parsing dim radial *********************************************\n");
     DRW_Coord pt = buf->get3BitDouble();
     setDefPoint(pt); //code 10
     drw_dbg("defPoint: "); drw_dbgpt(pt.x, pt.y, pt.z);
     pt = buf->get3BitDouble();
     setPt5(pt); //center pt  code 15
     drw_dbg("\ncenter point: "); drw_dbgpt(pt.x, pt.y, pt.z);
     setRa40(buf->getBitDouble()); //leader length code 40
     drw_dbg("\nleader length: "); drw_dbg(getRa40());
     type |= 4;
     drw_dbg("\n  type (70) final: "); drw_dbg(type); drw_dbg("\n");

     ret = DRW_Entity::parseDwgEntHandle(version, buf);
     drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
     if (!ret)
         return ret;
     dimStyleH = buf->getHandle();
     drw_dbg("dim style Handle: "); drw_dbghl(dimStyleH.code, dimStyleH.size, dimStyleH.ref); drw_dbg("\n");
     blockH = buf->getHandle(); /* H 7 STYLE (hard pointer) */
     drw_dbg("anon block Handle: "); drw_dbghl(blockH.code, blockH.size, blockH.ref); drw_dbg("\n");
     drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");

     //    RS crc;   //RS */
     return buf->isGood();
 }

 bool DRW_DimDiametric::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
     dwgBuffer sBuff = *buf;
     dwgBuffer *sBuf = buf;
     if (version > DRW::AC1018) {//2007+
         sBuf = &sBuff; //separate buffer for strings
     }
     bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
     if (!ret)
         return ret;
     ret = DRW_Dimension::parseDwg(version, buf, sBuf);
     if (!ret)
         return ret;
     drw_dbg("\n***************************** parsing dim diametric *********************************************\n");
     DRW_Coord pt = buf->get3BitDouble();
     setPt5(pt); //center pt  code 15
     drw_dbg("center point: "); drw_dbgpt(pt.x, pt.y, pt.z);
     pt = buf->get3BitDouble();
     setDefPoint(pt); //code 10
     drw_dbg("\ndefPoint: "); drw_dbgpt(pt.x, pt.y, pt.z);
     setRa40(buf->getBitDouble()); //leader length code 40
     drw_dbg("\nleader length: "); drw_dbg(getRa40());
     type |= 3;
     drw_dbg("\n  type (70) final: "); drw_dbg(type); drw_dbg("\n");

     ret = DRW_Entity::parseDwgEntHandle(version, buf);
     drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
     if (!ret)
         return ret;
     dimStyleH = buf->getHandle();
     drw_dbg("dim style Handle: "); drw_dbghl(dimStyleH.code, dimStyleH.size, dimStyleH.ref); drw_dbg("\n");
     blockH = buf->getHandle(); /* H 7 STYLE (hard pointer) */
     drw_dbg("anon block Handle: "); drw_dbghl(blockH.code, blockH.size, blockH.ref); drw_dbg("\n");
     drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");

     //    RS crc;   //RS */
     return buf->isGood();
 }

bool DRW_DimAngular::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    ret = DRW_Dimension::parseDwg(version, buf, sBuf);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing dim angular *********************************************\n");
    DRW_Coord pt;
    pt.x = buf->getRawDouble();
    pt.y = buf->getRawDouble();
    setPt6(pt); //code 16
    drw_dbg("arc Point: "); drw_dbgpt(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt3(pt); //def1  code 13
    drw_dbg("\ndef1: "); drw_dbgpt(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt4(pt); //def2  code 14
    drw_dbg("\ndef2: "); drw_dbgpt(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt5(pt); //center pt  code 15
    drw_dbg("\ncenter point: "); drw_dbgpt(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setDefPoint(pt); //code 10
    drw_dbg("\ndefPoint: "); drw_dbgpt(pt.x, pt.y, pt.z);
    type |= 0x02;
    drw_dbg("\n  type (70) final: "); drw_dbg(type); drw_dbg("\n");

    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    if (!ret)
        return ret;
    dimStyleH = buf->getHandle();
    drw_dbg("dim style Handle: "); drw_dbghl(dimStyleH.code, dimStyleH.size, dimStyleH.ref); drw_dbg("\n");
    blockH = buf->getHandle(); /* H 7 STYLE (hard pointer) */
    drw_dbg("anon block Handle: "); drw_dbghl(blockH.code, blockH.size, blockH.ref); drw_dbg("\n");
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");

    //    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_DimAngular3p::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    ret = DRW_Dimension::parseDwg(version, buf, sBuf);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing dim angular3p *********************************************\n");
    DRW_Coord pt = buf->get3BitDouble();
    setDefPoint(pt); //code 10
    drw_dbg("defPoint: "); drw_dbgpt(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt3(pt); //def1  code 13
    drw_dbg("\ndef1: "); drw_dbgpt(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt4(pt); //def2  code 14
    drw_dbg("\ndef2: "); drw_dbgpt(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt5(pt); //center pt  code 15
    drw_dbg("\ncenter point: "); drw_dbgpt(pt.x, pt.y, pt.z);
    type |= 0x05;
    drw_dbg("\n  type (70) final: "); drw_dbg(type); drw_dbg("\n");

    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    if (!ret)
        return ret;
    dimStyleH = buf->getHandle();
    drw_dbg("dim style Handle: "); drw_dbghl(dimStyleH.code, dimStyleH.size, dimStyleH.ref); drw_dbg("\n");
    blockH = buf->getHandle(); /* H 7 STYLE (hard pointer) */
    drw_dbg("anon block Handle: "); drw_dbghl(blockH.code, blockH.size, blockH.ref); drw_dbg("\n");
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");

    //    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_DimOrdinate::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    ret = DRW_Dimension::parseDwg(version, buf, sBuf);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing dim ordinate *********************************************\n");
    DRW_Coord pt = buf->get3BitDouble();
    setDefPoint(pt);
    drw_dbg("defPoint: "); drw_dbgpt(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt3(pt); //def1
    drw_dbg("\ndef1: "); drw_dbgpt(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt4(pt);
    drw_dbg("\ndef2: "); drw_dbgpt(pt.x, pt.y, pt.z);
    uint8_t type2 = buf->getRawChar8();//RLZ: correct this
    drw_dbg("type2 (70) read: "); drw_dbg(type2);
    type =  (type2 & 1) ? type | 0x80 : type & 0xBF; //set bit 6
    drw_dbg(" type (70) set: "); drw_dbg(type);
    type |= 6;
    drw_dbg("\n  type (70) final: "); drw_dbg(type);

    ret = DRW_Entity::parseDwgEntHandle(version, buf); drw_dbg("\n");
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    if (!ret)
        return ret;
    dimStyleH = buf->getHandle();
    drw_dbg("dim style Handle: "); drw_dbghl(dimStyleH.code, dimStyleH.size, dimStyleH.ref); drw_dbg("\n");
    blockH = buf->getHandle(); /* H 7 STYLE (hard pointer) */
    drw_dbg("anon block Handle: "); drw_dbghl(blockH.code, blockH.size, blockH.ref); drw_dbg("\n");
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");

    //    RS crc;   //RS */
    return buf->isGood();
}

void DRW_Leader::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 3:
        style = reader->getUtf8String();
        break;
    case 71:
        arrow = reader->getInt32();
        break;
    case 72:
        leadertype = reader->getInt32();
        break;
    case 73:
        flag = reader->getInt32();
        break;
    case 74:
        hookline = reader->getInt32();
        break;
    case 75:
        hookflag = reader->getInt32();
        break;
    case 76:
        vertnum = reader->getInt32();
        break;
    case 77:
        coloruse = reader->getInt32();
        break;
    case 40:
        textheight = reader->getDouble();
        break;
    case 41:
        textwidth = reader->getDouble();
        break;
    case 10:
        vertexpoint= std::make_shared<DRW_Coord>();
        vertexlist.push_back(vertexpoint);
        vertexpoint->x = reader->getDouble();
        break;
    case 20:
        if(vertexpoint)
            vertexpoint->y = reader->getDouble();
        break;
    case 30:
        if(vertexpoint)
            vertexpoint->z = reader->getDouble();
        break;
    case 340:
        annotHandle = reader->getHandleString();
        break;
    case 210:
        extrusionPoint.x = reader->getDouble();
        break;
    case 220:
        extrusionPoint.y = reader->getDouble();
        break;
    case 230:
        extrusionPoint.z = reader->getDouble();
        break;
    case 211:
        horizdir.x = reader->getDouble();
        break;
    case 221:
        horizdir.y = reader->getDouble();
        break;
    case 231:
        horizdir.z = reader->getDouble();
        break;
    case 212:
        offsetblock.x = reader->getDouble();
        break;
    case 222:
        offsetblock.y = reader->getDouble();
        break;
    case 232:
        offsetblock.z = reader->getDouble();
        break;
    case 213:
        offsettext.x = reader->getDouble();
        break;
    case 223:
        offsettext.y = reader->getDouble();
        break;
    case 233:
        offsettext.z = reader->getDouble();
        break;
    default:
        DRW_Entity::parseCode(code, reader);
        break;
    }
}

bool DRW_Leader::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing leader *********************************************\n");
    drw_dbg("unknown bit "); drw_dbg(buf->getBit());
    drw_dbg(" annot type "); drw_dbg(buf->getBitShort());
    drw_dbg(" Path type "); drw_dbg(buf->getBitShort());
    int32_t nPt = buf->getBitLong();
    drw_dbg(" Num pts "); drw_dbg(nPt);

    // add vertexes
    for (int i = 0; i< nPt; i++){
        DRW_Coord vertex = buf->get3BitDouble();
        vertexlist.push_back(std::make_shared<DRW_Coord>(vertex));
        drw_dbg("\nvertex "); drw_dbgpt(vertex.x, vertex.y, vertex.z);
    }
    DRW_Coord Endptproj = buf->get3BitDouble();
    drw_dbg("\nEndptproj "); drw_dbgpt(Endptproj.x, Endptproj.y, Endptproj.z);
    extrusionPoint = buf->getExtrusion(version > DRW::AC1014);
    drw_dbg("\nextrusionPoint "); drw_dbgpt(extrusionPoint.x, extrusionPoint.y, extrusionPoint.z);
    if (version > DRW::AC1014) { //2000+
        drw_dbg("\nFive unknown bits: "); drw_dbg(buf->getBit()); drw_dbg(buf->getBit());
        drw_dbg(buf->getBit()); drw_dbg(buf->getBit()); drw_dbg(buf->getBit());
    }
    horizdir = buf->get3BitDouble();
    drw_dbg("\nhorizdir "); drw_dbgpt(horizdir.x, horizdir.y, horizdir.z);
    offsetblock = buf->get3BitDouble();
    drw_dbg("\noffsetblock "); drw_dbgpt(offsetblock.x, offsetblock.y, offsetblock.z);
    if (version > DRW::AC1012) { //R14+
        DRW_Coord unk = buf->get3BitDouble();
        drw_dbg("\nunknown "); drw_dbgpt(unk.x, unk.y, unk.z);
    }
    if (version < DRW::AC1015) { //R14 -
        drw_dbg("\ndimgap "); drw_dbg(buf->getBitDouble());
    }
    if (version < DRW::AC1024) { //2010-
        textheight = buf->getBitDouble();
        textwidth = buf->getBitDouble();
        drw_dbg("\ntextheight "); drw_dbg(textheight); drw_dbg(" textwidth "); drw_dbg(textwidth);
    }
    hookline = buf->getBit();
    arrow = buf->getBit();
    drw_dbg(" hookline "); drw_dbg(hookline); drw_dbg(" arrow flag "); drw_dbg(arrow);

    if (version < DRW::AC1015) { //R14 -
        drw_dbg("\nArrow head type "); drw_dbg(buf->getBitShort());
        drw_dbg("dimasz "); drw_dbg(buf->getBitDouble());
        drw_dbg("\nunk bit "); drw_dbg(buf->getBit());
        drw_dbg(" unk bit "); drw_dbg(buf->getBit());
        drw_dbg(" unk short "); drw_dbg(buf->getBitShort());
        drw_dbg(" byBlock color "); drw_dbg(buf->getBitShort());
        drw_dbg(" unk bit "); drw_dbg(buf->getBit());
        drw_dbg(" unk bit "); drw_dbg(buf->getBit());
    } else { //R2000+
        drw_dbg("\nunk short "); drw_dbg(buf->getBitShort());
        drw_dbg(" unk bit "); drw_dbg(buf->getBit());
        drw_dbg(" unk bit "); drw_dbg(buf->getBit());
    }
    drw_dbg("\n");
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    AnnotH = buf->getHandle();
    annotHandle = AnnotH.ref;
    drw_dbg("annot block Handle: "); drw_dbghl(AnnotH.code, AnnotH.size, dimStyleH.ref); drw_dbg("\n");
    dimStyleH = buf->getHandle(); /* H 7 STYLE (hard pointer) */
    drw_dbg("dim style Handle: "); drw_dbghl(dimStyleH.code, dimStyleH.size, dimStyleH.ref); drw_dbg("\n");
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
//    RS crc;   //RS */
    return buf->isGood();
}

void DRW_Viewport::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 40:
        pswidth = reader->getDouble();
        break;
    case 41:
        psheight = reader->getDouble();
        break;
    case 68:
        vpstatus = reader->getInt32();
        break;
    case 69:
        vpID = reader->getInt32();
        break;
    case 12: {
        centerPX = reader->getDouble();
        break; }
    case 22:
        centerPY = reader->getDouble();
        break;
    default:
        DRW_Point::parseCode(code, reader);
        break;
    }
}
//ex 22 dec 34
bool DRW_Viewport::parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    drw_dbg("\n***************************** parsing viewport *****************************************\n");
    basePoint.x = buf->getBitDouble();
    basePoint.y = buf->getBitDouble();
    basePoint.z = buf->getBitDouble();
    drw_dbg("center "); drw_dbgpt(basePoint.x, basePoint.y, basePoint.z);
    pswidth = buf->getBitDouble();
    psheight = buf->getBitDouble();
    drw_dbg("\nWidth: "); drw_dbg(pswidth); drw_dbg(", Height: "); drw_dbg(psheight); drw_dbg("\n");
    //RLZ TODO: complete in dxf
    if (version > DRW::AC1014) {//2000+
        viewTarget.x = buf->getBitDouble();
        viewTarget.y = buf->getBitDouble();
        viewTarget.z = buf->getBitDouble();
        drw_dbg("view Target "); drw_dbgpt(viewTarget.x, viewTarget.y, viewTarget.z);
        viewDir.x = buf->getBitDouble();
        viewDir.y = buf->getBitDouble();
        viewDir.z = buf->getBitDouble();
        drw_dbg("\nview direction "); drw_dbgpt(viewDir.x, viewDir.y, viewDir.z);
        twistAngle = buf->getBitDouble();
        drw_dbg("\nView twist Angle: "); drw_dbg(twistAngle);
        viewHeight = buf->getBitDouble();
        drw_dbg("\nview Height: "); drw_dbg(viewHeight);
        viewLength = buf->getBitDouble();
        drw_dbg(" Lens Length: "); drw_dbg(viewLength);
        frontClip = buf->getBitDouble();
        drw_dbg("\nfront Clip Z: "); drw_dbg(frontClip);
        backClip = buf->getBitDouble();
        drw_dbg(" back Clip Z: "); drw_dbg(backClip);
        snapAngle = buf->getBitDouble();
        drw_dbg("\n snap Angle: "); drw_dbg(snapAngle);
        centerPX = buf->getRawDouble();
        centerPY = buf->getRawDouble();
        drw_dbg("\nview center X: "); drw_dbg(centerPX); drw_dbg(", Y: "); drw_dbg(centerPX);
        snapPX = buf->getRawDouble();
        snapPY = buf->getRawDouble();
        drw_dbg("\nSnap base point X: "); drw_dbg(snapPX); drw_dbg(", Y: "); drw_dbg(snapPY);
        snapSpPX = buf->getRawDouble();
        snapSpPY = buf->getRawDouble();
        drw_dbg("\nSnap spacing X: "); drw_dbg(snapSpPX); drw_dbg(", Y: "); drw_dbg(snapSpPY);
        //RLZ: need to complete
        drw_dbg("\nGrid spacing X: "); drw_dbg(buf->getRawDouble()); drw_dbg(", Y: "); drw_dbg(buf->getRawDouble());drw_dbg("\n");
        drw_dbg("Circle zoom?: "); drw_dbg(buf->getBitShort()); drw_dbg("\n");
    }
    if (version > DRW::AC1018) {//2007+
        drw_dbg("Grid major?: "); drw_dbg(buf->getBitShort()); drw_dbg("\n");
    }
    if (version > DRW::AC1014) {//2000+
        frozenLyCount = buf->getBitLong();
        drw_dbg("Frozen Layer count?: "); drw_dbg(frozenLyCount); drw_dbg("\n");
        drw_dbg("Status Flags?: "); drw_dbg(buf->getBitLong()); drw_dbg("\n");
        //RLZ: Warning needed separate string buffer
        drw_dbg("Style sheet?: "); drw_dbg(sBuf->getVariableText(version, false)); drw_dbg("\n");
        drw_dbg("Render mode?: "); drw_dbg(buf->getRawChar8()); drw_dbg("\n");
        drw_dbg("UCS OMore...: "); drw_dbg(buf->getBit()); drw_dbg("\n");
        drw_dbg("UCS VMore...: "); drw_dbg(buf->getBit()); drw_dbg("\n");
        drw_dbg("UCS OMore...: "); drw_dbgpt(buf->getBitDouble(), buf->getBitDouble(), buf->getBitDouble()); drw_dbg("\n");
        drw_dbg("ucs XAMore...: "); drw_dbgpt(buf->getBitDouble(), buf->getBitDouble(), buf->getBitDouble()); drw_dbg("\n");
        drw_dbg("UCS YMore....: "); drw_dbgpt(buf->getBitDouble(), buf->getBitDouble(), buf->getBitDouble()); drw_dbg("\n");
        drw_dbg("UCS EMore...: "); drw_dbg(buf->getBitDouble()); drw_dbg("\n");
        drw_dbg("UCS OVMore...: "); drw_dbg(buf->getBitShort()); drw_dbg("\n");
    }
    if (version > DRW::AC1015) {//2004+
        drw_dbg("ShadePlot Mode...: "); drw_dbg(buf->getBitShort()); drw_dbg("\n");
    }
    if (version > DRW::AC1018) {//2007+
        drw_dbg("Use def Light...: "); drw_dbg(buf->getBit()); drw_dbg("\n");
        drw_dbg("Def light type?: "); drw_dbg(buf->getRawChar8()); drw_dbg("\n");
        drw_dbg("Brightness: "); drw_dbg(buf->getBitDouble()); drw_dbg("\n");
        drw_dbg("Contrast: "); drw_dbg(buf->getBitDouble()); drw_dbg("\n");
//        drw_dbg("Ambient Cmc or Enc: "); drw_dbg(buf->getCmColor(version)); drw_dbg("\n");
        drw_dbg("Ambient (Cmc or Enc?), Enc: "); drw_dbg(buf->getEnColor(version)); drw_dbg("\n");
    }
    ret = DRW_Entity::parseDwgEntHandle(version, buf);

    dwgHandle someHdl;
    if (version < DRW::AC1015) {//R13 & R14 only
        drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
        someHdl = buf->getHandle();
        drw_dbg("ViewPort ent header: "); drw_dbghl(someHdl.code, someHdl.size, someHdl.ref); drw_dbg("\n");
    }
    if (version > DRW::AC1014) {//2000+
        for (uint8_t i=0; i < frozenLyCount; ++i){
            someHdl = buf->getHandle();
            drw_dbg("Frozen layer handle "); drw_dbg(i); drw_dbg(": "); drw_dbghl(someHdl.code, someHdl.size, someHdl.ref); drw_dbg("\n");
        }
        someHdl = buf->getHandle();
        drw_dbg("Clip bpundary handle: "); drw_dbghl(someHdl.code, someHdl.size, someHdl.ref); drw_dbg("\n");
        if (version == DRW::AC1015) {//2000 only
            someHdl = buf->getHandle();
            drw_dbg("ViewPort ent header: "); drw_dbghl(someHdl.code, someHdl.size, someHdl.ref); drw_dbg("\n");
        }
        someHdl = buf->getHandle();
        drw_dbg("Named ucs handle: "); drw_dbghl(someHdl.code, someHdl.size, someHdl.ref); drw_dbg("\n");
        drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
        someHdl = buf->getHandle();
        drw_dbg("base ucs handle: "); drw_dbghl(someHdl.code, someHdl.size, someHdl.ref); drw_dbg("\n");
    }
    if (version > DRW::AC1018) {//2007+
        someHdl = buf->getHandle();
        drw_dbg("background handle: "); drw_dbghl(someHdl.code, someHdl.size, someHdl.ref); drw_dbg("\n");
        someHdl = buf->getHandle();
        drw_dbg("visual style handle: "); drw_dbghl(someHdl.code, someHdl.size, someHdl.ref); drw_dbg("\n");
        someHdl = buf->getHandle();
        drw_dbg("shadeplot ID handle: "); drw_dbghl(someHdl.code, someHdl.size, someHdl.ref); drw_dbg("\n");
        drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
        someHdl = buf->getHandle();
        drw_dbg("SUN handle: "); drw_dbghl(someHdl.code, someHdl.size, someHdl.ref); drw_dbg("\n");
    }
    drw_dbg("\n Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");

    if (!ret)
        return ret;
    return buf->isGood();
}
