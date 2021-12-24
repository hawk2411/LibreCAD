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

#ifndef DWGREADER_H
#define DWGREADER_H

#include <unordered_map>
#include <list>
#include <memory>
#include "drw_textcodec.h"
#include "dwgutil.h"
#include "dwgbuffer.h"
#include "../libdwgr.h"

struct objHandle {
    objHandle() = default;

    objHandle(uint32_t t, uint32_t h, uint32_t l)
            : type{t}, handle{h}, loc{l} {}

    uint32_t type{0};
    uint32_t handle{0};
    uint32_t loc{0};
};

//until 2000 = 2000-
//since 2004 except 2007 = 2004+
// 2007 = 2007
// pages of section
/* 2000-: No pages, only sections
 * 2004+: Id, page number (index)
 *        size, size of page in file stream
 *        address, address in file stream
 *        dataSize, data size for this page
 *        startOffset, start offset for this page
 *        cSize, compressed size of data
 *        uSize, uncompressed size of data
 * 2007: page Id, pageCount & pages
 *       size, size in file
 *       dataSize
 *       startOffset, start position in decompressed data stream
 *       cSize, compressed size of data
 *       uSize, uncompressed size of data
 *       address, address in file stream
 * */
struct dwgPageInfo {
    dwgPageInfo()=default;

    dwgPageInfo(uint64_t i, uint64_t ad, uint64_t sz)
            : Id(i), address(ad), size(sz) {
    }
    uint64_t Id {0};
    uint64_t address {0}; //in file stream, for rd18, rd21
    uint64_t size {0}; //in file stream, for rd18, rd21
    uint64_t dataSize {0}; //for rd18, rd21
    uint64_t startOffset {0}; //for rd18, rd21
    uint64_t cSize {0}; //compressed page size, for rd21
    uint64_t uSize {0}; //uncompressed page size, for rd21
};

// sections of file
/* 2000-: No pages, only section Id, size  & address in file
 * 2004+: Id, Section Id
 *        size, total size of uncompressed data
 *        pageCount & pages, number of pages in section
 *        maxSize, max decompressed Size per page
 *        compressed, (1 = no, 2 = yes, normally 2)
 *        encrypted, (0 = no, 1 = yes, 2 = unknown)
 *        name, read & stored but not used
 * 2007: same as 2004+ except encoding, saved in compressed field
 * */
struct dwgSectionInfo {
    dwgSectionInfo() = default;

    int32_t Id{-1}; //section Id, 2000-   rd15 rd18
    std::string name; //section name rd18
    uint32_t compressed{1};//is compressed? 1=no, 2=yes rd18, rd21(encoding)
    uint32_t encrypted{0};//encrypted (doc: 0=no, 1=yes, 2=unkn) on read: objects 0 and encrypted yes rd18
    std::unordered_map<uint32_t, dwgPageInfo> pages;//index, size, offset
    uint64_t size{0}; //size of section,  2000- rd15, rd18, rd21 (data size)
    uint64_t pageCount{0}; //number of pages (dwgPageInfo) in section rd18, rd21
    uint64_t maxSize{0}; //max decompressed size (needed??) rd18 rd21
    uint64_t address{0}; //address (seek) , 2000-
};


//! Class to handle dwg obj control entries
/*!
*  Class to handle dwg obj control entries
*  @author Rallaz
*/
class DRW_ObjControl : public DRW_TableEntry {
public:
    DRW_ObjControl() = default;

    void reset() override{}
    bool parseDwg(DRW::Version version, dwgBuffer *buf, uint32_t bs) override;
    std::list<uint32_t>handlesList;
};


class dwgReader {
    friend class dwgR;
public:
    dwgReader(std::ifstream *stream, dwgR *p)
       :fileBuf{ new dwgBuffer(stream) }
       ,parent{p}
    {
        decoder.setVersion(DRW::AC1021, false);//default 2007 in utf8(no convert)
        decoder.setCodePage("UTF-16", false);
//        blockCtrl=0; //RLZ: temporary
//        blockCtrl=layerCtrl=styleCtrl=linetypeCtrl=viewCtrl=0;
//        ucsCtrl=vportCtrl=appidCtrl=dimstyleCtrl=vpEntHeaderCtrl=0;
    }
    virtual ~dwgReader();

protected:
    virtual bool readMetaData() = 0;
    virtual bool readPreview(){return false;}
    virtual bool readFileHeader() = 0;
    virtual bool readDwgHeader(DRW_Header& hdr)=0;
    virtual bool readDwgClasses() = 0;
    virtual bool readDwgHandles() = 0;
    virtual bool readDwgTables(DRW_Header& hdr)=0;
    virtual bool readDwgBlocks(DRW_Interface& intfa) = 0;
    virtual bool readDwgEntities(DRW_Interface& intfa) = 0;
    virtual bool readDwgObjects(DRW_Interface& intfa) = 0;

    virtual bool readDwgEntity(dwgBuffer *dbuf, objHandle& obj, DRW_Interface& intfa);
    bool readDwgObject(dwgBuffer *dbuf, objHandle& obj, DRW_Interface& intfa);
    void parseAttribs(DRW_Entity* e);
    std::string findTableName(DRW::TTYPE table, int32_t handle);

    void setCodePage(const std::string &c){decoder.setCodePage(c, false);}
    std::string getCodePage(){ return decoder.getCodePage();}
    bool readDwgHeader(DRW_Header& hdr, dwgBuffer *buf, dwgBuffer *hBuf);
    bool readDwgHandles(dwgBuffer *dbuf, uint64_t offset, uint64_t size);
    bool readDwgTables(DRW_Header& hdr, dwgBuffer *dbuf);
    bool checkSentinel(dwgBuffer *buf, enum secEnum::DWGSection, bool start);

    bool readDwgBlocks(DRW_Interface& intfa, dwgBuffer *dbuf);
    bool readDwgEntities(DRW_Interface& intfa, dwgBuffer *dbuf);
    bool readDwgObjects(DRW_Interface& intfa, dwgBuffer *dbuf);
    bool readPlineVertex(DRW_Polyline& pline, dwgBuffer *dbuf);

public:
    std::unordered_map<uint32_t, objHandle>ObjectMap;
    std::unordered_map<uint32_t, objHandle>objObjectMap; //stores the objects & entities not read in readDwgEntities
    std::unordered_map<uint32_t, objHandle>remainingMap; //stores the objects & entities not read in all processes, for debug only
    std::unordered_map<uint32_t, DRW_LType*> ltypemap;
    std::unordered_map<uint32_t, DRW_Layer*> layermap;
    std::unordered_map<uint32_t, DRW_Block*> blockmap;
    std::unordered_map<uint32_t, DRW_Textstyle*> stylemap;
    std::unordered_map<uint32_t, DRW_Dimstyle*> dimstylemap;
    std::unordered_map<uint32_t, DRW_Vport*> vportmap;
    std::unordered_map<uint32_t, DRW_Block_Record*> blockRecordmap;
    std::unordered_map<uint32_t, DRW_AppId*> appIdmap;
//    uint32_t currBlock;
    uint8_t maintenanceVersion{0};

protected:
    std::unique_ptr<dwgBuffer> fileBuf;
    dwgR *parent{nullptr};
    DRW::Version version{DRW::UNKNOWNV};

//seeker (position) for the beginning sentinel of the image data (R13 to R15)
    uint32_t previewImagePos {0};

//sections map
    std::unordered_map<int, dwgSectionInfo >sections;
    std::unordered_map<uint32_t, DRW_Class*> classesmap;

protected:
    DRW_TextCodec decoder;

protected:
//    uint32_t blockCtrl;
    uint32_t nextEntLink{0};
    uint32_t prevEntLink{0};
};



#endif // DWGREADER_H
