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
#include "dwgreader.h"
#include "drw_textcodec.h"
#include "drw_dbg.h"

namespace {
    //helper function to cleanup pointers in Look Up Tables
    template<typename T>
    void mapCleanUp(std::unordered_map<duint32, T*>& table)
    {
        for (auto& item: table)
            delete item.second;
    }
}

dwgReader::~dwgReader() {
    mapCleanUp(ltypemap);
    mapCleanUp(layermap);
    mapCleanUp(blockmap);
    mapCleanUp(stylemap);
    mapCleanUp(dimstylemap);
    mapCleanUp(vportmap);
    mapCleanUp(classesmap);
    mapCleanUp(blockRecordmap);
    mapCleanUp(appIdmap);
}

void dwgReader::parseAttribs(DRW_Entity* e) {
    if (nullptr == e) {
        return;
    }

    duint32 ltref =e->lTypeH.ref;
    duint32 lyref =e->layerH.ref;
    auto lt_it = ltypemap.find(ltref);
    if (lt_it != ltypemap.end()) {
        e->lineType = (lt_it->second)->name;
    }
    auto ly_it = layermap.find(lyref);
    if (ly_it != layermap.end()) {
        e->layer = (ly_it->second)->name;
    }
}

std::string dwgReader::findTableName(DRW::TTYPE table, dint32 handle){
    std::string name;
    switch (table){
    case DRW::STYLE:{
        auto st_it = stylemap.find(handle);
        if (st_it != stylemap.end())
            name = (st_it->second)->name;
        break;}
    case DRW::DIMSTYLE:{
        auto ds_it = dimstylemap.find(handle);
        if (ds_it != dimstylemap.end())
            name = (ds_it->second)->name;
        break;}
    case DRW::BLOCK_RECORD:{ //use DRW_Block because name are more correct
//        auto bk_it = blockmap.find(handle);
//        if (bk_it != blockmap.end())
        auto bk_it = blockRecordmap.find(handle);
        if (bk_it != blockRecordmap.end())
            name = (bk_it->second)->name;
        break;}
/*    case DRW::VPORT:{
        auto vp_it = vportmap.find(handle);
        if (vp_it != vportmap.end())
            name = (vp_it->second)->name;
        break;}*/
    case DRW::LAYER:{
        auto ly_it = layermap.find(handle);
        if (ly_it != layermap.end())
            name = (ly_it->second)->name;
        break;}
    case DRW::LTYPE:{
        auto lt_it = ltypemap.find(handle);
        if (lt_it != ltypemap.end())
            name = (lt_it->second)->name;
        break;}
    default:
        break;
    }
    return name;
}

bool dwgReader::readDwgHeader(DRW_Header& hdr, dwgBuffer *buf, dwgBuffer *hBuf){
    bool ret = hdr.parseDwg(version, buf, hBuf, maintenanceVersion);
    //RLZ: copy objectControl handles
    return ret;
}

//RLZ: TODO add check instead print
bool dwgReader::checkSentinel(dwgBuffer *buf, enum secEnum::DWGSection, bool start){
    DRW_UNUSED(start);
    for (int i=0; i<16;i++) {
        drw_dbgh(buf->getRawChar8()); drw_dbg(" ");
    }
    return true;
}

/*********** objects map ************************/
/** Note: object map are split in sections with max size 2035?
 *  heach section are 2 bytes size + data bytes + 2 bytes crc
 *  size value are data bytes + 2 and to calculate crc are used
 *  2 bytes size + data bytes
 *  last section are 2 bytes size + 2 bytes crc (size value always 2)
**/
bool dwgReader::readDwgHandles(dwgBuffer *dbuf, duint64 offset, duint64 size) {
    drw_dbg("\ndwgReader::readDwgHandles\n");
    if (!dbuf->setPosition(offset))
        return false;

    duint32 maxPos = offset + size;
    drw_dbg("\nSection HANDLES offset= "); drw_dbg(offset);
    drw_dbg("\nSection HANDLES size= "); drw_dbg(size);
    drw_dbg("\nSection HANDLES maxPos= "); drw_dbg(maxPos);

    int startPos = offset;

    std::vector<duint8> tmpByteStr;
    while (maxPos > dbuf->getPosition()) {
        drw_dbg("\nstart handles section buf->curPosition()= "); drw_dbg(dbuf->getPosition()); drw_dbg("\n");
        duint16 size = dbuf->getBERawShort16();
        drw_dbg("object map section size= "); drw_dbg(size); drw_dbg("\n");
        dbuf->setPosition(startPos);
        tmpByteStr.resize(size);
        dbuf->getBytes(tmpByteStr.data(), size);
        dwgBuffer buff(tmpByteStr.data(), size, &decoder);
        if (size != 2){
            buff.setPosition(2);
            int lastHandle = 0;
            int lastLoc = 0;
            //read data
            while(buff.getPosition()< size){
                lastHandle += buff.getUModularChar();
                drw_dbg("object map lastHandle= "); drw_dbgh(lastHandle);
                lastLoc += buff.getModularChar();
                drw_dbg(" lastLoc= "); drw_dbg(lastLoc); drw_dbg("\n");
                ObjectMap[lastHandle]= objHandle(0, lastHandle, lastLoc);
            }
        }
        //verify crc
        duint16 crcCalc = buff.crc8(0xc0c1,0,size);
        duint16 crcRead = dbuf->getBERawShort16();
        drw_dbg("object map section crc8 read= "); drw_dbg(crcRead);
        drw_dbg("\nobject map section crc8 calculated= "); drw_dbg(crcCalc);
        drw_dbg("\nobject section buf->curPosition()= "); drw_dbg(dbuf->getPosition()); drw_dbg("\n");
        startPos = dbuf->getPosition();
    }

    bool ret = dbuf->isGood();
    return ret;
}

/*********** objects ************************/
/**
 * Reads all the object referenced in the object map section of the DWG file
 * (using their object file offsets)
 */
bool dwgReader::readDwgTables(DRW_Header& hdr, dwgBuffer *dbuf) {
    drw_dbg("\ndwgReader::readDwgTables start\n");
    bool ret = true;
    bool ret2 = true;
    objHandle oc;
    dint16 oType;
    duint32 bs = 0; //bit size of handle stream 2010+
    std::vector<duint8> tmpByteStr;

    //parse linetypes, start with linetype Control
    auto mit = ObjectMap.find(hdr.linetypeCtrl);
    if (mit==ObjectMap.end()) {
        drw_dbg("\nWARNING: LineType control not found\n");
        ret = false;
    } else {
        drw_dbg("\n**********Parsing LineType control*******\n");
        oc = mit->second;
        ObjectMap.erase(mit);
        DRW_ObjControl ltControl;
        dbuf->setPosition(oc.loc);
        int csize = dbuf->getModularShort();
        if (version > DRW::AC1021) //2010+
            bs = dbuf->getUModularChar();
        else
            bs = 0;
        tmpByteStr.resize(csize);
        dbuf->getBytes(tmpByteStr.data(), csize);
        dwgBuffer cbuff(tmpByteStr.data(), csize, &decoder);
        //verify if object are correct
        oType = cbuff.getObjType(version);
        if (oType != 0x38) {
                drw_dbg("\nWARNING: Not LineType control object, found oType ");
                drw_dbg(oType);  drw_dbg(" instead 0x38\n");
                ret = false;
            } else { //reset position
            cbuff.resetPosition();
            ret2 = ltControl.parseDwg(version, &cbuff, bs);
            if(ret)
                ret = ret2;
        }
        for (auto it = ltControl.handlesList.begin(); it != ltControl.handlesList.end(); ++it) {
            mit = ObjectMap.find(*it);
            if (mit==ObjectMap.end()) {
                drw_dbg("\nWARNING: LineType not found\n");
                ret = false;
            } else {
                oc = mit->second;
                ObjectMap.erase(mit);
                drw_dbg("\nLineType Handle= "); drw_dbgh(oc.handle); drw_dbg(" loc.: "); drw_dbg(oc.loc); drw_dbg("\n");
                DRW_LType *lt = new DRW_LType();
                dbuf->setPosition(oc.loc);
                int lsize = dbuf->getModularShort();
                drw_dbg("LineType size in bytes= "); drw_dbg(lsize);
                if (version > DRW::AC1021) //2010+
                    bs = dbuf->getUModularChar();
                else
                    bs = 0;
                tmpByteStr.resize(lsize);
                dbuf->getBytes(tmpByteStr.data(), lsize);
                dwgBuffer lbuff(tmpByteStr.data(), lsize, &decoder);
                ret2 = lt->parseDwg(version, &lbuff, bs);
                ltypemap[lt->handle] = lt;
                if(ret)
                    ret = ret2;
            }
        }
    }

    //parse layers, start with layer Control
    mit = ObjectMap.find(hdr.layerCtrl);
    if (mit==ObjectMap.end()) {
        drw_dbg("\nWARNING: Layer control not found\n");
        ret = false;
    } else {
        drw_dbg("\n**********Parsing Layer control*******\n");
        oc = mit->second;
        ObjectMap.erase(mit);
        DRW_ObjControl layControl;
        dbuf->setPosition(oc.loc);
        int size = dbuf->getModularShort();
        if (version > DRW::AC1021) //2010+
            bs = dbuf->getUModularChar();
        else
            bs = 0;
        tmpByteStr.resize(size);
        dbuf->getBytes(tmpByteStr.data(), size);
        dwgBuffer buff(tmpByteStr.data(), size, &decoder);
        //verify if object are correct
        oType = buff.getObjType(version);
        if (oType != 0x32) {
                drw_dbg("\nWARNING: Not Layer control object, found oType ");
                drw_dbg(oType);  drw_dbg(" instead 0x32\n");
                ret = false;
            } else { //reset position
            buff.resetPosition();
            ret2 = layControl.parseDwg(version, &buff, bs);
            if(ret)
                ret = ret2;
        }
        for (auto it = layControl.handlesList.begin(); it != layControl.handlesList.end(); ++it) {
            mit = ObjectMap.find(*it);
            if (mit==ObjectMap.end()) {
                drw_dbg("\nWARNING: Layer not found\n");
                ret = false;
            } else {
                oc = mit->second;
                ObjectMap.erase(mit);
                drw_dbg("Layer Handle= "); drw_dbgh(oc.handle); drw_dbg(" "); drw_dbg(oc.loc); drw_dbg("\n");
                DRW_Layer *la = new DRW_Layer();
                dbuf->setPosition(oc.loc);
                int size = dbuf->getModularShort();
                if (version > DRW::AC1021) //2010+
                    bs = dbuf->getUModularChar();
                else
                    bs = 0;
                tmpByteStr.resize(size);
                dbuf->getBytes(tmpByteStr.data(), size);
                dwgBuffer buff(tmpByteStr.data(), size, &decoder);
                ret2 = la->parseDwg(version, &buff, bs);
                layermap[la->handle] = la;
                if(ret)
                    ret = ret2;
            }
        }
    }

    //set linetype in layer
    for (auto it=layermap.begin(); it!=layermap.end(); ++it) {
        DRW_Layer *ly = it->second;
        duint32 ref =ly->lTypeH.ref;
        auto lt_it = ltypemap.find(ref);
        if (lt_it != ltypemap.end()){
            ly->lineType = (lt_it->second)->name;
        }
    }

    //parse text styles, start with style Control
    mit = ObjectMap.find(hdr.styleCtrl);
    if (mit==ObjectMap.end()) {
        drw_dbg("\nWARNING: Style control not found\n");
        ret = false;
    } else {
        drw_dbg("\n**********Parsing Style control*******\n");
        oc = mit->second;
        ObjectMap.erase(mit);
        DRW_ObjControl styControl;
        dbuf->setPosition(oc.loc);
        int size = dbuf->getModularShort();
        if (version > DRW::AC1021) //2010+
            bs = dbuf->getUModularChar();
        else
            bs = 0;
        tmpByteStr.resize(size);
        dbuf->getBytes(tmpByteStr.data(), size);
        dwgBuffer buff(tmpByteStr.data(), size, &decoder);
        //verify if object are correct
        oType = buff.getObjType(version);
        if (oType != 0x34) {
                drw_dbg("\nWARNING: Not Text Style control object, found oType ");
                drw_dbg(oType);  drw_dbg(" instead 0x34\n");
                ret = false;
            } else { //reset position
            buff.resetPosition();
            ret2 = styControl.parseDwg(version, &buff, bs);
            if(ret)
                ret = ret2;
        }
        for (auto it = styControl.handlesList.begin(); it != styControl.handlesList.end(); ++it) {
            mit = ObjectMap.find(*it);
            if (mit==ObjectMap.end()) {
                drw_dbg("\nWARNING: Style not found\n");
                ret = false;
            } else {
                oc = mit->second;
                ObjectMap.erase(mit);
                drw_dbg("Style Handle= "); drw_dbgh(oc.handle); drw_dbg(" "); drw_dbg(oc.loc); drw_dbg("\n");
                DRW_Textstyle *sty = new DRW_Textstyle();
                dbuf->setPosition(oc.loc);
                int size = dbuf->getModularShort();
                if (version > DRW::AC1021) //2010+
                    bs = dbuf->getUModularChar();
                else
                    bs = 0;
                tmpByteStr.resize(size);
                dbuf->getBytes(tmpByteStr.data(), size);
                dwgBuffer buff(tmpByteStr.data(), size, &decoder);
                ret2 = sty->parseDwg(version, &buff, bs);
                stylemap[sty->handle] = sty;
                if(ret)
                    ret = ret2;
            }
        }
    }

    //parse dim styles, start with dimstyle Control
    mit = ObjectMap.find(hdr.dimstyleCtrl);
    if (mit==ObjectMap.end()) {
        drw_dbg("\nWARNING: Dimension Style control not found\n");
        ret = false;
    } else {
        drw_dbg("\n**********Parsing Dimension Style control*******\n");
        oc = mit->second;
        ObjectMap.erase(mit);
        DRW_ObjControl dimstyControl;
        dbuf->setPosition(oc.loc);
        duint32 size = dbuf->getModularShort();
        if (version > DRW::AC1021) //2010+
            bs = dbuf->getUModularChar();
        else
            bs = 0;
        tmpByteStr.resize(size);
        dbuf->getBytes(tmpByteStr.data(), size);
        dwgBuffer buff(tmpByteStr.data(), size, &decoder);
        //verify if object are correct
        oType = buff.getObjType(version);
        if (oType != 0x44) {
                drw_dbg("\nWARNING: Not Dim Style control object, found oType ");
                drw_dbg(oType);  drw_dbg(" instead 0x44\n");
                ret = false;
            } else { //reset position
            buff.resetPosition();
            ret2 = dimstyControl.parseDwg(version, &buff, bs);
            if(ret)
                ret = ret2;
        }
        for (auto it = dimstyControl.handlesList.begin(); it != dimstyControl.handlesList.end(); ++it) {
            mit = ObjectMap.find(*it);
            if (mit==ObjectMap.end()) {
                drw_dbg("\nWARNING: Dimension Style not found\n");
                ret = false;
            } else {
                oc = mit->second;
                ObjectMap.erase(mit);
                drw_dbg("Dimstyle Handle= "); drw_dbgh(oc.handle); drw_dbg(" "); drw_dbg(oc.loc); drw_dbg("\n");
                DRW_Dimstyle *sty = new DRW_Dimstyle();
                dbuf->setPosition(oc.loc);
                int size = dbuf->getModularShort();
                if (version > DRW::AC1021) //2010+
                    bs = dbuf->getUModularChar();
                else
                    bs = 0;
                tmpByteStr.resize(size);
                dbuf->getBytes(tmpByteStr.data(), size);
                dwgBuffer buff(tmpByteStr.data(), size, &decoder);
                ret2 = sty->parseDwg(version, &buff, bs);
                dimstylemap[sty->handle] = sty;
                if(ret)
                    ret = ret2;
            }
        }
    }

    //parse vports, start with vports Control
    mit = ObjectMap.find(hdr.vportCtrl);
    if (mit==ObjectMap.end()) {
        drw_dbg("\nWARNING: vports control not found\n");
        ret = false;
    } else {
        drw_dbg("\n**********Parsing vports control*******\n");
        oc = mit->second;
        ObjectMap.erase(mit);
        DRW_ObjControl vportControl;
        dbuf->setPosition(oc.loc);
        int size = dbuf->getModularShort();
        if (version > DRW::AC1021) //2010+
            bs = dbuf->getUModularChar();
        else
            bs = 0;
        tmpByteStr.resize(size);
        dbuf->getBytes(tmpByteStr.data(), size);
        dwgBuffer buff(tmpByteStr.data(), size, &decoder);
        //verify if object are correct
        oType = buff.getObjType(version);
        if (oType != 0x40) {
                drw_dbg("\nWARNING: Not VPorts control object, found oType: ");
                drw_dbg(oType);  drw_dbg(" instead 0x40\n");
                ret = false;
            } else { //reset position
            buff.resetPosition();
            ret2 = vportControl.parseDwg(version, &buff, bs);
            if(ret)
                ret = ret2;
        }
        for (auto it = vportControl.handlesList.begin(); it != vportControl.handlesList.end(); ++it) {
            mit = ObjectMap.find(*it);
            if (mit==ObjectMap.end()) {
                drw_dbg("\nWARNING: vport not found\n");
                ret = false;
            } else {
                oc = mit->second;
                ObjectMap.erase(mit);
                drw_dbg("Vport Handle= "); drw_dbgh(oc.handle); drw_dbg(" "); drw_dbg(oc.loc); drw_dbg("\n");
                DRW_Vport *vp = new DRW_Vport();
                dbuf->setPosition(oc.loc);
                int size = dbuf->getModularShort();
                if (version > DRW::AC1021) //2010+
                    bs = dbuf->getUModularChar();
                else
                    bs = 0;
                tmpByteStr.resize(size);
                dbuf->getBytes(tmpByteStr.data(), size);
                dwgBuffer buff(tmpByteStr.data(), size, &decoder);
                ret2 = vp->parseDwg(version, &buff, bs);
                vportmap[vp->handle] = vp;
                if(ret)
                    ret = ret2;
            }
        }
    }

    //parse Block_records , start with Block_record Control
    mit = ObjectMap.find(hdr.blockCtrl);
    if (mit==ObjectMap.end()) {
        drw_dbg("\nWARNING: Block_record control not found\n");
        ret = false;
    } else {
        drw_dbg("\n**********Parsing Block_record control*******\n");
        oc = mit->second;
        ObjectMap.erase(mit);
        DRW_ObjControl blockControl;
        dbuf->setPosition(oc.loc);
        int csize = dbuf->getModularShort();
        if (version > DRW::AC1021) //2010+
            bs = dbuf->getUModularChar();
        else
            bs = 0;
        tmpByteStr.resize(csize);
        dbuf->getBytes(tmpByteStr.data(), csize);
        dwgBuffer buff(tmpByteStr.data(), csize, &decoder);
        //verify if object are correct
        oType = buff.getObjType(version);
        if (oType != 0x30) {
                drw_dbg("\nWARNING: Not Block Record control object, found oType ");
                drw_dbg(oType);  drw_dbg(" instead 0x30\n");
                ret = false;
            } else { //reset position
            buff.resetPosition();
            ret2 = blockControl.parseDwg(version, &buff, bs);
            if(ret)
                ret = ret2;
        }
        for (auto it = blockControl.handlesList.begin(); it != blockControl.handlesList.end(); ++it) {
            mit = ObjectMap.find(*it);
            if (mit==ObjectMap.end()) {
                drw_dbg("\nWARNING: block record not found\n");
                ret = false;
            } else {
                oc = mit->second;
                ObjectMap.erase(mit);
                drw_dbg("block record Handle= "); drw_dbgh(oc.handle); drw_dbg(" "); drw_dbg(oc.loc); drw_dbg("\n");
                DRW_Block_Record *br = new DRW_Block_Record();
                dbuf->setPosition(oc.loc);
                int size = dbuf->getModularShort();
                if (version > DRW::AC1021) //2010+
                    bs = dbuf->getUModularChar();
                else
                    bs = 0;
                tmpByteStr.resize(size);
                dbuf->getBytes(tmpByteStr.data(), size);
                dwgBuffer buff(tmpByteStr.data(), size, &decoder);
                ret2 = br->parseDwg(version, &buff, bs);
                blockRecordmap[br->handle] = br;
                if(ret)
                    ret = ret2;
            }
        }
    }

    //parse appId , start with appId Control
    mit = ObjectMap.find(hdr.appidCtrl);
    if (mit==ObjectMap.end()) {
        drw_dbg("\nWARNING: AppId control not found\n");
        ret = false;
    } else {
        drw_dbg("\n**********Parsing AppId control*******\n");
        oc = mit->second;
        ObjectMap.erase(mit);
        drw_dbg("AppId Control Obj Handle= "); drw_dbgh(oc.handle); drw_dbg(" "); drw_dbg(oc.loc); drw_dbg("\n");
        DRW_ObjControl appIdControl;
        dbuf->setPosition(oc.loc);
        int size = dbuf->getModularShort();
        if (version > DRW::AC1021) //2010+
            bs = dbuf->getUModularChar();
        else
            bs = 0;
        tmpByteStr.resize(size);
        dbuf->getBytes(tmpByteStr.data(), size);
        dwgBuffer buff(tmpByteStr.data(), size, &decoder);
        //verify if object are correct
        oType = buff.getObjType(version);
        if (oType != 0x42) {
                drw_dbg("\nWARNING: Not AppId control object, found oType ");
                drw_dbg(oType);  drw_dbg(" instead 0x42\n");
                ret = false;
            } else { //reset position
            buff.resetPosition();
            ret2 = appIdControl.parseDwg(version, &buff, bs);
            if(ret)
                ret = ret2;
        }
        for (unsigned int & it : appIdControl.handlesList) {
            mit = ObjectMap.find(it);
            if (mit==ObjectMap.end()) {
                drw_dbg("\nWARNING: AppId not found\n");
                ret = false;
            } else {
                oc = mit->second;
                ObjectMap.erase(mit);
                drw_dbg("AppId Handle= "); drw_dbgh(oc.handle); drw_dbg(" "); drw_dbg(oc.loc); drw_dbg("\n");
                DRW_AppId *ai = new DRW_AppId();
                dbuf->setPosition(oc.loc);
                int size = dbuf->getModularShort();
                if (version > DRW::AC1021) //2010+
                    bs = dbuf->getUModularChar();
                else
                    bs = 0;
                tmpByteStr.resize(size);
                dbuf->getBytes(tmpByteStr.data(), size);
                dwgBuffer buff(tmpByteStr.data(), size, &decoder);
                ret2 = ai->parseDwg(version, &buff, bs);
                appIdmap[ai->handle] = ai;
                if(ret)
                    ret = ret2;
            }
        }
    }

    //RLZ: parse remaining object controls, TODO: implement all
    if (drw_dbggl() == DRW::DebugLevel::Debug){
        mit = ObjectMap.find(hdr.viewCtrl);
        if (mit==ObjectMap.end()) {
            drw_dbg("\nWARNING: View control not found\n");
            ret = false;
        } else {
            drw_dbg("\n**********Parsing View control*******\n");
            oc = mit->second;
            ObjectMap.erase(mit);
            drw_dbg("View Control Obj Handle= "); drw_dbgh(oc.handle); drw_dbg(" "); drw_dbg(oc.loc); drw_dbg("\n");
            DRW_ObjControl viewControl;
            dbuf->setPosition(oc.loc);
            int size = dbuf->getModularShort();
            if (version > DRW::AC1021) //2010+
                bs = dbuf->getUModularChar();
            else
                bs = 0;
            tmpByteStr.resize(size);
            dbuf->getBytes(tmpByteStr.data(), size);
            dwgBuffer buff(tmpByteStr.data(), size, &decoder);
            //verify if object are correct
            oType = buff.getObjType(version);
            if (oType != 0x3C) {
                    drw_dbg("\nWARNING: Not View control object, found oType ");
                    drw_dbg(oType);  drw_dbg(" instead 0x3C\n");
                    ret = false;
                } else { //reset position
                buff.resetPosition();
                ret2 = viewControl.parseDwg(version, &buff, bs);
                if(ret)
                    ret = ret2;
            }
        }

        mit = ObjectMap.find(hdr.ucsCtrl);
        if (mit==ObjectMap.end()) {
            drw_dbg("\nWARNING: Ucs control not found\n");
            ret = false;
        } else {
            oc = mit->second;
            ObjectMap.erase(mit);
            drw_dbg("\n**********Parsing Ucs control*******\n");
            drw_dbg("Ucs Control Obj Handle= "); drw_dbgh(oc.handle); drw_dbg(" "); drw_dbg(oc.loc); drw_dbg("\n");
            DRW_ObjControl ucsControl;
            dbuf->setPosition(oc.loc);
            int size = dbuf->getModularShort();
            if (version > DRW::AC1021) //2010+
                bs = dbuf->getUModularChar();
            else
                bs = 0;
            tmpByteStr.resize(size);
            dbuf->getBytes(tmpByteStr.data(), size);
            dwgBuffer buff(tmpByteStr.data(), size, &decoder);
            //verify if object are correct
            oType = buff.getObjType(version);
            if (oType != 0x3E) {
                    drw_dbg("\nWARNING: Not Ucs control object, found oType ");
                    drw_dbg(oType);  drw_dbg(" instead 0x3E\n");
                    ret = false;
                } else { //reset position
                buff.resetPosition();
                ret2 = ucsControl.parseDwg(version, &buff, bs);
                if(ret)
                    ret = ret2;
            }
        }

        if (version < DRW::AC1018) {//r2000-
            mit = ObjectMap.find(hdr.vpEntHeaderCtrl);
            if (mit==ObjectMap.end()) {
                drw_dbg("\nWARNING: vpEntHeader control not found\n");
                ret = false;
            } else {
                drw_dbg("\n**********Parsing vpEntHeader control*******\n");
                oc = mit->second;
                ObjectMap.erase(mit);
                drw_dbg("vpEntHeader Control Obj Handle= "); drw_dbgh(oc.handle); drw_dbg(" "); drw_dbg(oc.loc); drw_dbg("\n");
                DRW_ObjControl vpEntHeaderCtrl;
                dbuf->setPosition(oc.loc);
                int size = dbuf->getModularShort();
                if (version > DRW::AC1021) //2010+
                    bs = dbuf->getUModularChar();
                else
                    bs = 0;
                tmpByteStr.resize(size);
                dbuf->getBytes(tmpByteStr.data(), size);
                dwgBuffer buff(tmpByteStr.data(), size, &decoder);
                //verify if object are correct
                oType = buff.getObjType(version);
                if (oType != 0x46) {
                        drw_dbg("\nWARNING: Not vpEntHeader control object, found oType ");
                        drw_dbg(oType);  drw_dbg(" instead 0x46\n");
                        ret = false;
                    } else { //reset position
                    buff.resetPosition();
/* RLZ: writeme                   ret2 = vpEntHeader.parseDwg(version, &buff, bs);
                    if(ret)
                        ret = ret2;*/
                }
            }
        }
    }

    return ret;
}

bool dwgReader::readDwgBlocks(DRW_Interface& intfa, dwgBuffer *dbuf){
    bool ret = true;
    bool ret2 = true;
    duint32 bs =0;
    drw_dbg("\nobject map total size= "); drw_dbg(ObjectMap.size());

    for (auto it=blockRecordmap.begin(); it != blockRecordmap.end(); ++it){
        DRW_Block_Record* bkr= it->second;
        drw_dbg("\nParsing Block, record handle= "); drw_dbgh(it->first); drw_dbg(" Name= "); drw_dbg(bkr->name); drw_dbg("\n");
        drw_dbg("\nFinding Block, handle= "); drw_dbgh(bkr->block); drw_dbg("\n");
        auto mit = ObjectMap.find(bkr->block);
        if (mit==ObjectMap.end()) {
            drw_dbg("\nWARNING: block entity not found\n");
            ret = false;
            continue;
        }
        objHandle oc = mit->second;
        ObjectMap.erase(mit);
        drw_dbg("Block Handle= "); drw_dbgh(oc.handle); drw_dbg(" Location: "); drw_dbg(oc.loc); drw_dbg("\n");
        if ( !(dbuf->setPosition(oc.loc)) ){
            drw_dbg("Bad Location reading blocks\n");
            ret = false;
            continue;
        }
        int size = dbuf->getModularShort();
        if (version > DRW::AC1021) //2010+
            bs = dbuf->getUModularChar();
        else
            bs = 0;

        std::vector<duint8> tmpByteStr(size);
        dbuf->getBytes(tmpByteStr.data(), size);
        dwgBuffer buff(tmpByteStr.data(), size, &decoder);
        DRW_Block bk;
        ret2 = bk.parseDwg(version, &buff, bs);
        ret = ret && ret2;
        parseAttribs(&bk);
        //complete block entity with block record data
        bk.basePoint = bkr->basePoint;
        bk.flags = bkr->flags;
        intfa.addBlock(bk);
        //and update block record name
        bkr->name = bk.name;

        /**read & send block entities**/
        // in dwg code 330 are not set like dxf in ModelSpace & PaperSpace, set it (RLZ: only tested in 2000)
        if (bk.parentHandle == DRW::NoHandle) {
            // in dwg code 330 are not set like dxf in ModelSpace & PaperSpace, set it
            bk.parentHandle= bkr->handle;
            //and do not send block entities like dxf
        } else {
            if (version < DRW::AC1018) { //pre 2004
                duint32 nextH = bkr->firstEH;
                while (nextH != 0){
                    mit = ObjectMap.find(nextH);
                    if (mit==ObjectMap.end()) {
                        nextH = bkr->lastEH;//end while if entity not foud
                        drw_dbg("\nWARNING: Entity of block not found\n");
                        ret = false;
                        continue;
                    } else {//foud entity reads it
                        oc = mit->second;
                        ObjectMap.erase(mit);
                        ret2 = readDwgEntity(dbuf, oc, intfa);
                        ret = ret && ret2;
                    }
                    if (nextH == bkr->lastEH)
                        nextH = 0; //redundant, but prevent read errors
                    else
                        nextH = nextEntLink;
                }
            } else {//2004+
                for (std::vector<duint32>::iterator it = bkr->entMap.begin() ; it != bkr->entMap.end(); ++it){
                    duint32 nextH = *it;
                    mit = ObjectMap.find(nextH);
                    if (mit==ObjectMap.end()) {
                        drw_dbg("\nWARNING: Entity of block not found\n");
                        ret = false;
                        continue;
                    } else {//foud entity reads it
                        oc = mit->second;
                        ObjectMap.erase(mit);
                        drw_dbg("\nBlocks, parsing entity: "); drw_dbgh(oc.handle); drw_dbg(", pos: "); drw_dbg(oc.loc); drw_dbg("\n");
                        ret2 = readDwgEntity(dbuf, oc, intfa);
                        ret = ret && ret2;
                    }
                }
            }//end 2004+
        }

        //end block entity, really needed to parse a dummy entity??
        mit = ObjectMap.find(bkr->endBlock);
        if (mit==ObjectMap.end()) {
            drw_dbg("\nWARNING: end block entity not found\n");
            ret = false;
            continue;
        }
        oc = mit->second;
        ObjectMap.erase(mit);
        drw_dbg("End block Handle= "); drw_dbgh(oc.handle); drw_dbg(" Location: "); drw_dbg(oc.loc); drw_dbg("\n");
        dbuf->setPosition(oc.loc);
        size = dbuf->getModularShort();
        if (version > DRW::AC1021) //2010+
            bs = dbuf->getUModularChar();
        else
            bs = 0;
        tmpByteStr.resize(size);
        dbuf->getBytes(tmpByteStr.data(), size);
        dwgBuffer buff1(tmpByteStr.data(), size, &decoder);
        DRW_Block end;
        end.isEnd = true;
        ret2 = end.parseDwg(version, &buff1, bs);
        ret = ret && ret2;
        if (bk.parentHandle == DRW::NoHandle) bk.parentHandle= bkr->handle;
        parseAttribs(&end);
        intfa.endBlock();
    }

    return ret;
}

bool dwgReader::readPlineVertex(DRW_Polyline& pline, dwgBuffer *dbuf){
    bool ret = true;
    bool ret2 = true;
    objHandle oc;
    duint32 bs = 0;

    if (version < DRW::AC1018) { //pre 2004
        duint32 nextH = pline.firstEH;
        while (nextH != 0){
            auto mit = ObjectMap.find(nextH);
            if (mit==ObjectMap.end()) {
                nextH = pline.lastEH;//end while if entity not foud
                drw_dbg("\nWARNING: pline vertex not found\n");
                ret = false;
                continue;
            } else {//foud entity reads it
                oc = mit->second;
                ObjectMap.erase(mit);
                DRW_Vertex vt;
                dbuf->setPosition(oc.loc);
                //RLZ: verify if pos is ok
                int size = dbuf->getModularShort();
                if (version > DRW::AC1021) {//2010+
                    bs = dbuf->getUModularChar();
                }
                std::vector<duint8> tmpByteStr(size);
                dbuf->getBytes(tmpByteStr.data(), size);
                dwgBuffer buff(tmpByteStr.data(), size, &decoder);
                dint16 oType = buff.getObjType(version);
                buff.resetPosition();
                drw_dbg(" object type= "); drw_dbg(oType); drw_dbg("\n");
                ret2 = vt.parseDwg(version, &buff, bs, pline.basePoint.z);
                pline.addVertex(vt);
                nextEntLink = vt.nextEntLink; \
                prevEntLink = vt.prevEntLink;
                ret = ret && ret2;
            }
            if (nextH == pline.lastEH)
                nextH = 0; //redundant, but prevent read errors
            else
                nextH = nextEntLink;
        }
    } else {//2004+
        for (std::list<duint32>::iterator it = pline.hadlesList.begin() ; it != pline.hadlesList.end(); ++it){
            duint32 nextH = *it;
            auto mit = ObjectMap.find(nextH);
            if (mit==ObjectMap.end()) {
                drw_dbg("\nWARNING: Entity of block not found\n");
                ret = false;
                continue;
            } else {//foud entity reads it
                oc = mit->second;
                ObjectMap.erase(mit);
                drw_dbg("\nPline vertex, parsing entity: "); drw_dbgh(oc.handle); drw_dbg(", pos: "); drw_dbg(oc.loc); drw_dbg("\n");
                DRW_Vertex vt;
                dbuf->setPosition(oc.loc);
                //RLZ: verify if pos is ok
                int size = dbuf->getModularShort();
                if (version > DRW::AC1021) {//2010+
                    bs = dbuf->getUModularChar();
                }
                std::vector<duint8> tmpByteStr(size);
                dbuf->getBytes(tmpByteStr.data(), size);
                dwgBuffer buff(tmpByteStr.data(), size, &decoder);
                dint16 oType = buff.getObjType(version);
                buff.resetPosition();
                drw_dbg(" object type= "); drw_dbg(oType); drw_dbg("\n");
                ret2 = vt.parseDwg(version, &buff, bs, pline.basePoint.z);
                pline.addVertex(vt);
                nextEntLink = vt.nextEntLink; \
                prevEntLink = vt.prevEntLink;
                ret = ret && ret2;
            }
        }
    }//end 2004+
    drw_dbg("\nRemoved SEQEND entity: "); drw_dbgh(pline.seqEndH.ref);drw_dbg("\n");
    ObjectMap.erase(pline.seqEndH.ref);

    return ret;
}

bool dwgReader::readDwgEntities(DRW_Interface& intfa, dwgBuffer *dbuf){
    bool ret = true;
    bool ret2 = true;

    drw_dbg("\nobject map total size= "); drw_dbg(ObjectMap.size());
    auto itB=ObjectMap.begin();
    auto itE=ObjectMap.end();
    while (itB != itE){
        ret2 = readDwgEntity(dbuf, itB->second, intfa);
        ObjectMap.erase(itB);
        itB=ObjectMap.begin();
        if (ret)
            ret = ret2;
    }
    return ret;
}
#define ENTRY_PARSE(e) \
    ret = e.parseDwg(version, &buff, bs); \
    parseAttribs(&e); \
    nextEntLink = e.nextEntLink; \
    prevEntLink = e.prevEntLink;

/**
 * Reads a dwg drawing entity (dwg object entity) given its offset in the file
 */
bool dwgReader::readDwgEntity(dwgBuffer *dbuf, objHandle &obj, DRW_Interface &intfa) {
    bool ret = true;
    duint32 bs = 0;


    nextEntLink = prevEntLink = 0;// set to 0 to skip unimplemented entities
    dbuf->setPosition(obj.loc);
    //verify if position is ok:
    if (!dbuf->isGood()) {
        drw_dbg(" Warning: readDwgEntity, bad location\n");
        return false;
    }
    int size = dbuf->getModularShort();
    if (version > DRW::AC1021) {//2010+
        bs = dbuf->getUModularChar();
    }
    std::vector<duint8> tmpByteStr(size);
    dbuf->getBytes(tmpByteStr.data(), size);
    //verify if getBytes is ok:
    if (!dbuf->isGood()) {
        drw_dbg(" Warning: readDwgEntity, bad size\n");
        return false;
    }
    dwgBuffer buff(tmpByteStr.data(), size, &decoder);
    dint16 oType = buff.getObjType(version);
    buff.resetPosition();

    if (oType > 499) {
        auto it = classesmap.find(oType);
        if (it == classesmap.end()) {//fail, not found in classes set error
            drw_dbg("Class ");
            drw_dbg(oType);
            drw_dbg("not found, handle: ");
            drw_dbg(obj.handle);
            drw_dbg("\n");
            return false;
        } else {
            DRW_Class *cl = it->second;
            if (cl->dwgType != 0)
                oType = cl->dwgType;
        }
    }

    obj.type = oType;
    switch (oType) {
        case 17: {
            DRW_Arc e;
            ENTRY_PARSE(e)
            intfa.addArc(e);
            break;
        }
        case 18: {
            DRW_Circle e;
            ENTRY_PARSE(e)
            intfa.addCircle(e);
            break;
        }
        case 19: {
            DRW_Line e;
            ENTRY_PARSE(e)
            intfa.addLine(e);
            break;
        }
        case 27: {
            DRW_Point e;
            ENTRY_PARSE(e)
            intfa.addPoint(e);
            break;
        }
        case 35: {
            DRW_Ellipse e;
            ENTRY_PARSE(e)
            intfa.addEllipse(e);
            break;
        }
        case 7:
        case 8: {//minsert = 8
            DRW_Insert e;
            ENTRY_PARSE(e)
            e.name = findTableName(DRW::BLOCK_RECORD, e.blockRecH.ref);//RLZ: find as block or blockrecord (ps & ps0)
            intfa.addInsert(e);
            break;
        }
        case 77: {
            DRW_LWPolyline e;
            ENTRY_PARSE(e)
            intfa.addLWPolyline(e);
            break;
        }
        case 1: {
            DRW_Text e;
            ENTRY_PARSE(e)
            e.style = findTableName(DRW::STYLE, e.styleH.ref);
            intfa.addText(e);
            break;
        }
        case 44: {
            DRW_MText e;
            ENTRY_PARSE(e)
            e.style = findTableName(DRW::STYLE, e.styleH.ref);
            intfa.addMText(e);
            break;
        }
        case 28: {
            DRW_3Dface e;
            ENTRY_PARSE(e)
            intfa.add3dFace(e);
            break;
        }
        case 20: {
            DRW_DimOrdinate e;
            ENTRY_PARSE(e)
            e.style = findTableName(DRW::DIMSTYLE, e.dimStyleH.ref);
            intfa.addDimOrdinate(&e);
            break;
        }
        case 21: {
            DRW_DimLinear e;
            ENTRY_PARSE(e)
            e.style = findTableName(DRW::DIMSTYLE, e.dimStyleH.ref);
            intfa.addDimLinear(&e);
            break;
        }
        case 22: {
            DRW_DimAligned e;
            ENTRY_PARSE(e)
            e.style = findTableName(DRW::DIMSTYLE, e.dimStyleH.ref);
            intfa.addDimAlign(&e);
            break;
        }
        case 23: {
            DRW_DimAngular3p e;
            ENTRY_PARSE(e)
            e.style = findTableName(DRW::DIMSTYLE, e.dimStyleH.ref);
            intfa.addDimAngular3P(&e);
            break;
        }
        case 24: {
            DRW_DimAngular e;
            ENTRY_PARSE(e)
            e.style = findTableName(DRW::DIMSTYLE, e.dimStyleH.ref);
            intfa.addDimAngular(&e);
            break;
        }
        case 25: {
            DRW_DimRadial e;
            ENTRY_PARSE(e)
            e.style = findTableName(DRW::DIMSTYLE, e.dimStyleH.ref);
            intfa.addDimRadial(&e);
            break;
        }
        case 26: {
            DRW_DimDiametric e;
            ENTRY_PARSE(e)
            e.style = findTableName(DRW::DIMSTYLE, e.dimStyleH.ref);
            intfa.addDimDiametric(&e);
            break;
        }
        case 45: {
            DRW_Leader e;
            ENTRY_PARSE(e)
            e.style = findTableName(DRW::DIMSTYLE, e.dimStyleH.ref);
            intfa.addLeader(&e);
            break;
        }
        case 31: {
            DRW_Solid e;
            ENTRY_PARSE(e)
            intfa.addSolid(e);
            break;
        }
        case 78: {
            DRW_Hatch e;
            ENTRY_PARSE(e)
            intfa.addHatch(&e);
            break;
        }
        case 32: {
            DRW_Trace e;
            ENTRY_PARSE(e)
            intfa.addTrace(e);
            break;
        }
        case 34: {
            DRW_Viewport e;
            ENTRY_PARSE(e)
            intfa.addViewport(e);
            break;
        }
        case 36: {
            DRW_Spline e;
            ENTRY_PARSE(e)
            intfa.addSpline(&e);
            break;
        }
        case 40: {
            DRW_Ray e;
            ENTRY_PARSE(e)
            intfa.addRay(e);
            break;
        }
        case 15:    // pline 2D
        case 16:    // pline 3D
        case 29: {  // pline PFACE
            DRW_Polyline e;
            ENTRY_PARSE(e)
            readPlineVertex(e, dbuf);
            intfa.addPolyline(e);
            break;
        }
//        case 30: {
//            DRW_Polyline e;// MESH (not pline)
//            ENTRY_PARSE(e)
//            intfa.addRay(e);
//            break; }
        case 41: {
            DRW_Xline e;
            ENTRY_PARSE(e)
            intfa.addXline(e);
            break;
        }
        case 101: {
            DRW_Image e;
            ENTRY_PARSE(e)
            intfa.addImage(&e);
            break;
        }

        default:
            //not supported or are object add to remaining map
            objObjectMap[obj.handle] = obj;
            break;
    }
    if (!ret) {
        drw_dbg("Warning: Entity type ");
        drw_dbg(oType);
        drw_dbg("has failed, handle: ");
        drw_dbg(obj.handle);
        drw_dbg("\n");
    }
    return ret;
}

bool dwgReader::readDwgObjects(DRW_Interface& intfa, dwgBuffer *dbuf){
    bool ret = true;
    bool ret2 = true;

    duint32 i=0;
    drw_dbg("\nentities map total size= "); drw_dbg(ObjectMap.size());
    drw_dbg("\nobjects map total size= "); drw_dbg(objObjectMap.size());
    auto itB=objObjectMap.begin();
    auto itE=objObjectMap.end();
    while (itB != itE){
        ret2 = readDwgObject(dbuf, itB->second, intfa);
        objObjectMap.erase(itB);
        itB=objObjectMap.begin();
        if (ret)
            ret = ret2;
    }
    if (drw_dbggl() == DRW::DebugLevel::Debug) {
        for (auto it=remainingMap.begin(); it != remainingMap.end(); ++it){
            drw_dbg("\nnum.# "); drw_dbg(i++); drw_dbg(" Remaining object Handle, loc, type= "); drw_dbg(it->first);
            drw_dbg(" "); drw_dbg(it->second.loc); drw_dbg(" "); drw_dbg(it->second.type);
        }
        drw_dbg("\n");
    }
    return ret;
}

/**
 * Reads a dwg drawing object (dwg object object) given its offset in the file
 */
bool dwgReader::readDwgObject(dwgBuffer *dbuf, objHandle& obj, DRW_Interface& intfa){
    bool ret = true;
    duint32 bs = 0;

        dbuf->setPosition(obj.loc);
        //verify if position is ok:
        if (!dbuf->isGood()){
            drw_dbg(" Warning: readDwgObject, bad location\n");
            return false;
        }
        int size = dbuf->getModularShort();
        if (version > DRW::AC1021) {//2010+
            bs = dbuf->getUModularChar();
        }
        duint8 *tmpByteStr = new duint8[size];
        dbuf->getBytes(tmpByteStr, size);
        //verify if getBytes is ok:
        if (!dbuf->isGood()){
            drw_dbg(" Warning: readDwgObject, bad size\n");
            delete[]tmpByteStr;
            return false;
        }
        dwgBuffer buff(tmpByteStr, size, &decoder);
        //oType are set parsing entities
        dint16 oType = obj.type;

        switch (oType){
        case 102: {
            DRW_ImageDef e;
            ret = e.parseDwg(version, &buff, bs);
            intfa.linkImage(&e);
            break; }
        default:
            //not supported object or entity add to remaining map for debug
            remainingMap[obj.handle]= obj;
            break;
        }
        if (!ret){
            drw_dbg("Warning: Object type "); drw_dbg(oType);drw_dbg("has failed, handle: "); drw_dbg(obj.handle); drw_dbg("\n");
        }
        delete[]tmpByteStr;
    return ret;
}



bool DRW_ObjControl::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
int unkData=0;
    bool ret = DRW_TableEntry::parseDwg(version, buf, nullptr, bs);
    drw_dbg("\n***************************** parsing object control entry *********************************************\n");
    if (!ret)
        return ret;
    //last parsed is: XDic Missing Flag 2004+
    int numEntries = buf->getBitLong();
    drw_dbg(" num entries: "); drw_dbg(numEntries); drw_dbg("\n");
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");

//    if (oType == 68 && version== DRW::AC1015){//V2000 dimstyle seems have one unknown byte hard handle counter??
    if (oType == 68 && version > DRW::AC1014){//dimstyle seems have one unknown byte hard handle counter??
        unkData = buf->getRawChar8();
        drw_dbg(" unknown v2000 byte: "); drw_dbg( unkData); drw_dbg("\n");
    }
    if (version > DRW::AC1018){//from v2007+ have a bit for strings follows (ObjControl do not have)
        int stringBit = buf->getBit();
        drw_dbg(" string bit for  v2007+: "); drw_dbg( stringBit); drw_dbg("\n");
    }

    dwgHandle objectH = buf->getHandle();
    drw_dbg(" NULL Handle: "); drw_dbghl(objectH.code, objectH.size, objectH.ref); drw_dbg("\n");
    drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");

//    if (oType == 56 && version== DRW::AC1015){//linetype in 2004 seems not have XDicObjH or NULL handle
    if (xDictFlag !=1){//linetype in 2004 seems not have XDicObjH or NULL handle
        dwgHandle XDicObjH = buf->getHandle();
        drw_dbg(" XDicObj control Handle: "); drw_dbghl(XDicObjH.code, XDicObjH.size, XDicObjH.ref); drw_dbg("\n");
        drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    }
//add 2 for modelspace, paperspace blocks & bylayer, byblock linetypes
    numEntries = ((oType == 48) || (oType == 56)) ? (numEntries +2) : numEntries;

    for (int i =0; i< numEntries; i++){
        objectH = buf->getOffsetHandle(handle);
        if (objectH.ref != 0) //in vports R14  I found some NULL handles
            handlesList.push_back (objectH.ref);
        drw_dbg(" objectH Handle: "); drw_dbghl(objectH.code, objectH.size, objectH.ref); drw_dbg("\n");
        drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    }

    for (int i =0; i< unkData; i++){
        objectH = buf->getOffsetHandle(handle);
        drw_dbg(" unknown Handle: "); drw_dbghl(objectH.code, objectH.size, objectH.ref); drw_dbg("\n");
        drw_dbg("Remaining bytes: "); drw_dbg(buf->numRemainingBytes()); drw_dbg("\n");
    }
    return buf->isGood();
}

