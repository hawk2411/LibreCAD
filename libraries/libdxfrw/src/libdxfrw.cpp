/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2016-2021 A. Stebich (librecad@mail.lordofbikes.de)        **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/


#include "libdxfrw.h"
#include <fstream>
//#include <algorithm>
#include <cassert>
#include <intern/dwgutil.h>
#include "intern/drw_textcodec.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
#include "intern/drw_dbg.h"

#define FIRSTHANDLE 48

/*enum sections {
    secUnknown,
    secHeader,
    secTables,
    secBlocks,
    secEntities,
    secObjects
};*/

dxfRW::dxfRW(const char* name) :fileName(name){
    drw_dbgsl(DRW::DebugLevel::None);
}
dxfRW::~dxfRW(){
    for (auto &it: imageDef) { delete it; }

    imageDef.clear();
}

void dxfRW::setDebug(DRW::DebugLevel lvl){
    switch (lvl){
    case DRW::DebugLevel::Debug:
        drw_dbgsl(DRW::DebugLevel::Debug);
        break;
    case DRW::DebugLevel::None:
        drw_dbgsl(DRW::DebugLevel::None);
    }
}

bool dxfRW::read(DRW_Interface *interface_, bool ext){
    drw_assert(fileName.empty() == false);
    applyExt = ext;
    std::ifstream filestr;
    if (nullptr == interface_) {
        return setError(DRW::BAD_UNKNOWN);
    }
    drw_dbg("dxfRW::read 1def\n");
    filestr.open (fileName.c_str(), std::ios_base::in | std::ios::binary);
    if (!filestr.is_open()
        || !filestr.good()) {
        return setError(DRW::BAD_OPEN);
    }

    char line[22];
    char line2[22] = "AutoCAD Binary DXF\r\n";
    line2[20] = (char)26;
    line2[21] = '\0';
    filestr.read (line, 22);
    filestr.close();
    iface = interface_;
    std::unique_ptr<dxfReader> reader;
    drw_dbg("dxfRW::read 2\n");
    if (strcmp(line, line2) == 0) {
        filestr.open (fileName.c_str(), std::ios_base::in | std::ios::binary);
        //skip sentinel
        filestr.seekg (22, std::ios::beg);
        reader = std::unique_ptr<dxfReader>(new dxfReaderBinary(&filestr));
        drw_dbg("dxfRW::read binary file\n");
    } else {
        filestr.open (fileName.c_str(), std::ios_base::in);
        reader = std::unique_ptr<dxfReader>(new dxfReaderAscii(&filestr));
    }

    bool isOk {processDxf(reader.get())};
    filestr.close();
    version = (DRW::Version) reader->getVersion();
    reader = nullptr;
    return isOk;
}

bool dxfRW::write(DRW_Interface *interface_, DRW::Version ver, bool bin){
    bool isOk = false;
    std::ofstream filestr;
    version = ver;
    iface = interface_;
    std::unique_ptr<dxfWriter> writer;
    if (bin) {
        filestr.open (fileName.c_str(), std::ios_base::out | std::ios::binary | std::ios::trunc);
        //write sentinel
        filestr << "AutoCAD Binary DXF\r\n" << (char)26 << '\0';
        writer = std::unique_ptr<dxfWriter>(new dxfWriterBinary(&filestr));
        drw_dbg("dxfRW::read binary file\n");
    } else {
        filestr.open (fileName.c_str(), std::ios_base::out | std::ios::trunc);
        writer = std::unique_ptr<dxfWriterAscii>(new dxfWriterAscii(&filestr));
        std::string comm = std::string("dxfrw ") + std::string(DRW_VERSION);
        writer->writeString(999, comm);
    }
    DRW_Header header_local;
    iface->writeHeader(header_local, writer.get());
    writer->writeString(0, "SECTION");
    entCount =FIRSTHANDLE;
    header_local.write(writer.get(), version);
    writer->writeString(0, "ENDSEC");
    if (ver > DRW::AC1009) {
        writer->writeString(0, "SECTION");
        writer->writeString(2, "CLASSES");
        writer->writeString(0, "ENDSEC");
    }
    writer->writeString(0, "SECTION");
    writer->writeString(2, "TABLES");
    writeTables(writer.get());
    writer->writeString(0, "ENDSEC");
    writer->writeString(0, "SECTION");
    writer->writeString(2, "BLOCKS");
    writeBlocks(writer.get());
    writer->writeString(0, "ENDSEC");

    writer->writeString(0, "SECTION");
    writer->writeString(2, "ENTITIES");
    iface->writeEntities(writer.get());
    writer->writeString(0, "ENDSEC");

    if (version > DRW::AC1009) {
        writer->writeString(0, "SECTION");
        writer->writeString(2, "OBJECTS");
        writeObjects(writer.get());
        writer->writeString(0, "ENDSEC");
    }
    writer->writeString(0, "EOF");
    filestr.flush();
    filestr.close();
    isOk = true;
    return isOk;
}

bool dxfRW::writeEntity(DRW_Entity *ent, dxfWriter* writer) {
    ent->handle = ++entCount;
    writer->writeString(5, DRW::toHexStr(ent->handle));
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbEntity");
    }
    if (ent->space == 1)
        writer->writeInt16(67, 1);
    if (version > DRW::AC1009) {
        writer->writeUtf8String(8, ent->layer);
        writer->writeUtf8String(6, ent->lineType);
    } else {
        writer->writeUtf8Caps(8, ent->layer);
        writer->writeUtf8Caps(6, ent->lineType);
    }
    writer->writeInt16(62, ent->color);
    if (version > DRW::AC1015 && ent->color24 >= 0) {
        writer->writeInt32(420, ent->color24);
    }
    if (version > DRW::AC1014) {
        writer->writeInt16(370, DRW_LW_Conv::lineWidth2dxfInt(ent->lWeight));
    }
    if (version >= DRW::AC1014) {
        writeAppData(ent->appData, writer);
    }
    return true;
}

bool dxfRW::writeAppData(const std::list<std::list<DRW_Variant>>& appData, dxfWriter* writer) {
    for(auto group : appData) {
        //Search for application name
        bool found = false;

        for(auto data : group) {
            if(data.code() == 102 && data.type() == DRW_Variant::STRING) {
                writer->writeString(102, "{" + *(data.content.s));
                found = true;
                break;
            }
        }

        if(found) {
            for(auto data : group) {
                if(data.code() == 102) {
                    continue;
                }

                switch(data.type()) {
                    case DRW_Variant::STRING:
                        writer->writeString(data.code(), *(data.content.s));
                        break;

                    case DRW_Variant::INTEGER:
                        writer->writeInt32(data.code(), data.content.i);
                        break;

                    case DRW_Variant::DOUBLE:
                        writer->writeDouble(data.code(), data.content.i);
                        break;

                    default:
                        break;
                }
            }

            writer->writeString(102, "}");
        }
    }
    return true;
}

bool dxfRW::writeLineType(DRW_LType *ent, dxfWriter* writer){
    std::string strname = DRW::toUpper(ent->name);

//do not write linetypes handled by library
    if (strname == "BYLAYER" || strname == "BYBLOCK" || strname == "CONTINUOUS") {
        return true;
    }
    writer->writeString(0, "LTYPE");
    if (version > DRW::AC1009) {
        writer->writeString(5, DRW::toHexStr<int>(++entCount));
        if (version > DRW::AC1012) {
            writer->writeString(330, "5");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLinetypeTableRecord");
        writer->writeUtf8String(2, ent->name);
    } else
        writer->writeUtf8Caps(2, ent->name);
    writer->writeInt16(70, ent->flags);
    writer->writeUtf8String(3, ent->desc);
    ent->update();
    writer->writeInt16(72, 65);
    writer->writeInt16(73, ent->size);
    writer->writeDouble(40, ent->length);

    for (unsigned int i = 0;  i< ent->path.size(); i++){
        writer->writeDouble(49, ent->path.at(i));
        if (version > DRW::AC1009) {
            writer->writeInt16(74, 0);
        }
    }
    return true;
}

bool dxfRW::writeLayer(const DRW_Layer *ent, dxfWriter* writer){
    writer->writeString(0, "LAYER");
    if (!wlayer0 && ent->name == "0") {
        wlayer0 = true;
        if (version > DRW::AC1009) {
            writer->writeString(5, "10");
        }
    } else {
        if (version > DRW::AC1009) {
            writer->writeString(5, DRW::toHexStr(++entCount));
        }
    }
    if (version > DRW::AC1012) {
        writer->writeString(330, "2");
    }
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLayerTableRecord");
        writer->writeUtf8String(2, ent->name);
    } else {
        writer->writeUtf8Caps(2, ent->name);
    }
    writer->writeInt16(70, ent->flags);
    writer->writeInt16(62, ent->color);
    if (version > DRW::AC1015 && ent->color24 >= 0) {
        writer->writeInt32(420, ent->color24);
    }
    if (version > DRW::AC1009) {
        writer->writeUtf8String(6, ent->lineType);
        if (! ent->plotF)
            writer->writeBool(290, ent->plotF);
        writer->writeInt16(370, DRW_LW_Conv::lineWidth2dxfInt(ent->lWeight));
        writer->writeString(390, "F");
    } else
        writer->writeUtf8Caps(6, ent->lineType);
    if (!ent->extData.empty()){
        writeExtData(ent->extData, writer);
    }
//    writer->writeString(347, "10012");
    return true;
}

bool dxfRW::writeTextstyle(const DRW_Textstyle *ent, dxfWriter* writer){
    writer->writeString(0, "STYLE");
    if (!dimstyleStd) {
        //stringstream cause crash in OS/X, bug#3597944
        std::string name = DRW::toUpper(ent->name);
        if (name == "STANDARD")
            dimstyleStd = true;
    }
    if (version > DRW::AC1009) {
        writer->writeString(5, DRW::toHexStr(++entCount));
    }

    if (version > DRW::AC1012) {
        writer->writeString(330, "2");
    }
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbTextStyleTableRecord");
        writer->writeUtf8String(2, ent->name);
    } else {
        writer->writeUtf8Caps(2, ent->name);
    }
    writer->writeInt16(70, ent->flags);
    writer->writeDouble(40, ent->height);
    writer->writeDouble(41, ent->width);
    writer->writeDouble(50, ent->oblique);
    writer->writeInt16(71, ent->genFlag);
    writer->writeDouble(42, ent->lastHeight);
    if (version > DRW::AC1009) {
        writer->writeUtf8String(3, ent->font);
        writer->writeUtf8String(4, ent->bigFont);
        if (ent->fontFamily != 0)
            writer->writeInt32(1071, ent->fontFamily);
    } else {
        writer->writeUtf8Caps(3, ent->font);
        writer->writeUtf8Caps(4, ent->bigFont);
    }
    return true;
}

bool dxfRW::writeVport(DRW_Vport *ent, dxfWriter* writer){
    if (!dimstyleStd) {
        ent->name = "*ACTIVE";
        dimstyleStd = true;
    }
    writer->writeString(0, "VPORT");
    if (version > DRW::AC1009) {
        writer->writeString(5, DRW::toHexStr(++entCount));
        if (version > DRW::AC1012)
            writer->writeString(330, "2");
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbViewportTableRecord");
        writer->writeUtf8String(2, ent->name);
    } else
        writer->writeUtf8Caps(2, ent->name);
    writer->writeInt16(70, ent->flags);
    writer->writeDouble(10, ent->lowerLeft.x);
    writer->writeDouble(20, ent->lowerLeft.y);
    writer->writeDouble(11, ent->UpperRight.x);
    writer->writeDouble(21, ent->UpperRight.y);
    writer->writeDouble(12, ent->center.x);
    writer->writeDouble(22, ent->center.y);
    writer->writeDouble(13, ent->snapBase.x);
    writer->writeDouble(23, ent->snapBase.y);
    writer->writeDouble(14, ent->snapSpacing.x);
    writer->writeDouble(24, ent->snapSpacing.y);
    writer->writeDouble(15, ent->gridSpacing.x);
    writer->writeDouble(25, ent->gridSpacing.y);
    writer->writeDouble(16, ent->viewDir.x);
    writer->writeDouble(26, ent->viewDir.y);
    writer->writeDouble(36, ent->viewDir.z);
    writer->writeDouble(17, ent->viewTarget.x);
    writer->writeDouble(27, ent->viewTarget.y);
    writer->writeDouble(37, ent->viewTarget.z);
    writer->writeDouble(40, ent->height);
    writer->writeDouble(41, ent->ratio);
    writer->writeDouble(42, ent->lensHeight);
    writer->writeDouble(43, ent->frontClip);
    writer->writeDouble(44, ent->backClip);
    writer->writeDouble(50, ent->snapAngle);
    writer->writeDouble(51, ent->twistAngle);
    writer->writeInt16(71, ent->viewMode);
    writer->writeInt16(72, ent->circleZoom);
    writer->writeInt16(73, ent->fastZoom);
    writer->writeInt16(74, ent->ucsIcon);
    writer->writeInt16(75, ent->snap);
    writer->writeInt16(76, ent->grid);
    writer->writeInt16(77, ent->snapStyle);
    writer->writeInt16(78, ent->snapIsopair);
    if (version > DRW::AC1014) {
        writer->writeInt16(281, 0);
        writer->writeInt16(65, 1);
        writer->writeDouble(110, 0.0);
        writer->writeDouble(120, 0.0);
        writer->writeDouble(130, 0.0);
        writer->writeDouble(111, 1.0);
        writer->writeDouble(121, 0.0);
        writer->writeDouble(131, 0.0);
        writer->writeDouble(112, 0.0);
        writer->writeDouble(122, 1.0);
        writer->writeDouble(132, 0.0);
        writer->writeInt16(79, 0);
        writer->writeDouble(146, 0.0);
        if (version > DRW::AC1018) {
            writer->writeString(348, "10020");
            writer->writeInt16(60, ent->gridBehavior);//v2007 undocummented see DRW_Vport class
            writer->writeInt16(61, 5);
            writer->writeBool(292, 1);
            writer->writeInt16(282, 1);
            writer->writeDouble(141, 0.0);
            writer->writeDouble(142, 0.0);
            writer->writeInt16(63, 250);
            writer->writeInt32(421, 3358443);
        }
    }
    return true;
}

bool dxfRW::writeDimstyle(const DRW_Dimstyle *ent, dxfWriter* writer){
    writer->writeString(0, "DIMSTYLE");
    if (!dimstyleStd) {
        std::string name = DRW::toUpper(ent->name);
        if (name == "STANDARD")
            dimstyleStd = true;
    }
    if (version > DRW::AC1009) {
        writer->writeString(105, DRW::toHexStr(++entCount));
    }

    if (version > DRW::AC1012) {
        writer->writeString(330, "A");
    }
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbDimStyleTableRecord");
        writer->writeUtf8String(2, ent->name);
    } else
        writer->writeUtf8Caps(2, ent->name);
    writer->writeInt16(70, ent->flags);
    if ( version == DRW::AC1009 || !(ent->dimpost.empty()) )
        writer->writeUtf8String(3, ent->dimpost);
    if ( version == DRW::AC1009 || !(ent->dimapost.empty()) )
        writer->writeUtf8String(4, ent->dimapost);
    if ( version == DRW::AC1009 || !(ent->dimblk.empty()) )
        writer->writeUtf8String(5, ent->dimblk);
    if ( version == DRW::AC1009 || !(ent->dimblk1.empty()) )
        writer->writeUtf8String(6, ent->dimblk1);
    if ( version == DRW::AC1009 || !(ent->dimblk2.empty()) )
        writer->writeUtf8String(7, ent->dimblk2);
    writer->writeDouble(40, ent->dimscale);
    writer->writeDouble(41, ent->dimasz);
    writer->writeDouble(42, ent->dimexo);
    writer->writeDouble(43, ent->dimdli);
    writer->writeDouble(44, ent->dimexe);
    writer->writeDouble(45, ent->dimrnd);
    writer->writeDouble(46, ent->dimdle);
    writer->writeDouble(47, ent->dimtp);
    writer->writeDouble(48, ent->dimtm);
    if ( version > DRW::AC1018 || ent->dimfxl !=0 )
        writer->writeDouble(49, ent->dimfxl);
    writer->writeDouble(140, ent->dimtxt);
    writer->writeDouble(141, ent->dimcen);
    writer->writeDouble(142, ent->dimtsz);
    writer->writeDouble(143, ent->dimaltf);
    writer->writeDouble(144, ent->dimlfac);
    writer->writeDouble(145, ent->dimtvp);
    writer->writeDouble(146, ent->dimtfac);
    writer->writeDouble(147, ent->dimgap);
    if (version > DRW::AC1014) {
        writer->writeDouble(148, ent->dimaltrnd);
    }
    writer->writeInt16(71, ent->dimtol);
    writer->writeInt16(72, ent->dimlim);
    writer->writeInt16(73, ent->dimtih);
    writer->writeInt16(74, ent->dimtoh);
    writer->writeInt16(75, ent->dimse1);
    writer->writeInt16(76, ent->dimse2);
    writer->writeInt16(77, ent->dimtad);
    writer->writeInt16(78, ent->dimzin);
    if (version > DRW::AC1014) {
        writer->writeInt16(79, ent->dimazin);
    }
    writer->writeInt16(170, ent->dimalt);
    writer->writeInt16(171, ent->dimaltd);
    writer->writeInt16(172, ent->dimtofl);
    writer->writeInt16(173, ent->dimsah);
    writer->writeInt16(174, ent->dimtix);
    writer->writeInt16(175, ent->dimsoxd);
    writer->writeInt16(176, ent->dimclrd);
    writer->writeInt16(177, ent->dimclre);
    writer->writeInt16(178, ent->dimclrt);
    if (version > DRW::AC1014) {
        writer->writeInt16(179, ent->dimadec);
    }
    if (version > DRW::AC1009) {
        if (version < DRW::AC1015)
            writer->writeInt16(270, ent->dimunit);
        writer->writeInt16(271, ent->dimdec);
        writer->writeInt16(272, ent->dimtdec);
        writer->writeInt16(273, ent->dimaltu);
        writer->writeInt16(274, ent->dimalttd);
        writer->writeInt16(275, ent->dimaunit);
    }
    if (version > DRW::AC1014) {
        writer->writeInt16(276, ent->dimfrac);
        writer->writeInt16(277, ent->dimlunit);
        writer->writeInt16(278, ent->dimdsep);
        writer->writeInt16(279, ent->dimtmove);
    }
    if (version > DRW::AC1009) {
        writer->writeInt16(280, ent->dimjust);
        writer->writeInt16(281, ent->dimsd1);
        writer->writeInt16(282, ent->dimsd2);
        writer->writeInt16(283, ent->dimtolj);
        writer->writeInt16(284, ent->dimtzin);
        writer->writeInt16(285, ent->dimaltz);
        writer->writeInt16(286, ent->dimaltttz);
        if (version < DRW::AC1015)
            writer->writeInt16(287, ent->dimfit);
        writer->writeInt16(288, ent->dimupt);
    }
    if (version > DRW::AC1014) {
        writer->writeInt16(289, ent->dimatfit);
    }
    if ( version > DRW::AC1018 && ent->dimfxlon !=0 )
        writer->writeInt16(290, ent->dimfxlon);
    if (version > DRW::AC1009) {
        writer->writeUtf8String(340, ent->dimtxsty);
    }
    if (version > DRW::AC1014) {
        writer->writeUtf8String(341, ent->dimldrblk);
        writer->writeInt16(371, ent->dimlwd);
        writer->writeInt16(372, ent->dimlwe);
    }
    return true;
}

bool dxfRW::writeAppId(DRW_AppId *ent, dxfWriter* writer) {
    std::string strname = DRW::toUpper(ent->name);
//do not write mandatory ACAD appId, handled by library
    if (strname == "ACAD")
        return true;
    writer->writeString(0, "APPID");
    if (version > DRW::AC1009) {
        writer->writeString(5, DRW::toHexStr(++entCount));
        if (version > DRW::AC1014) {
            writer->writeString(330, "9");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbRegAppTableRecord");
        writer->writeUtf8String(2, ent->name);
    } else {
        writer->writeUtf8Caps(2, ent->name);
    }
    writer->writeInt16(70, ent->flags);
    return true;
}

bool dxfRW::writePoint(DRW_Point *ent, dxfWriter* writer) {
    writer->writeString(0, "POINT");
    writeEntity(ent, writer);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbPoint");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
    }
    return true;
}

bool dxfRW::writeLine(DRW_Line *ent, dxfWriter* writer) {
    writer->writeString(0, "LINE");
    writeEntity(ent, writer);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbLine");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0 || ent->secPoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
    } else {
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
    }
    return true;
}

bool dxfRW::writeRay(DRW_Ray *ent, dxfWriter* writer) {
    writer->writeString(0, "RAY");
    writeEntity(ent, writer);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbRay");
    }
    DRW_Coord crd = ent->secPoint;
    crd.unitize();
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0 || ent->secPoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(11, crd.x);
        writer->writeDouble(21, crd.y);
        writer->writeDouble(31, crd.z);
    } else {
        writer->writeDouble(11, crd.x);
        writer->writeDouble(21, crd.y);
    }
    return true;
}

bool dxfRW::writeXline(DRW_Xline *ent, dxfWriter* writer) {
    writer->writeString(0, "XLINE");
    writeEntity(ent, writer);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbXline");
    }
    DRW_Coord crd = ent->secPoint;
    crd.unitize();
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0 || ent->secPoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(11, crd.x);
        writer->writeDouble(21, crd.y);
        writer->writeDouble(31, crd.z);
    } else {
        writer->writeDouble(11, crd.x);
        writer->writeDouble(21, crd.y);
    }
    return true;
}

bool dxfRW::writeCircle(DRW_Circle *ent, dxfWriter* writer) {
    writer->writeString(0, "CIRCLE");
    writeEntity(ent, writer);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbCircle");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
    }
    writer->writeDouble(40, ent->radious);
    return true;
}

bool dxfRW::writeArc(DRW_Arc *ent, dxfWriter* writer) {
    writer->writeString(0, "ARC");
    writeEntity(ent, writer);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbCircle");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
    }
    writer->writeDouble(40, ent->radious);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbArc");
    }
    writer->writeDouble(50, ent->staangle*ARAD);
    writer->writeDouble(51, ent->endangle*ARAD);
    return true;
}

bool dxfRW::writeEllipse(DRW_Ellipse *ent, dxfWriter* writer){
    //verify axis/ratio and params for full ellipse
    ent->correctAxis();
    if (version > DRW::AC1009) {
        writer->writeString(0, "ELLIPSE");
        writeEntity(ent, writer);
        if (version > DRW::AC1009) {
            writer->writeString(100, "AcDbEllipse");
        }
        writer->writeDouble(10, ent->basePoint.x);
        writer->writeDouble(20, ent->basePoint.y);
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
        writer->writeDouble(40, ent->ratio);
        writer->writeDouble(41, ent->staparam);
        writer->writeDouble(42, ent->endparam);
    } else {
        DRW_Polyline pol;
        //RLZ: copy properties
        ent->toPolyline(&pol, elParts);
        writePolyline(&pol, writer);
    }
    return true;
}

bool dxfRW::writeTrace(DRW_Trace *ent, dxfWriter* writer){
    writer->writeString(0, "TRACE");
    writeEntity(ent, writer);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbTrace");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(11, ent->secPoint.x);
    writer->writeDouble(21, ent->secPoint.y);
    writer->writeDouble(31, ent->secPoint.z);
    writer->writeDouble(12, ent->thirdPoint.x);
    writer->writeDouble(22, ent->thirdPoint.y);
    writer->writeDouble(32, ent->thirdPoint.z);
    writer->writeDouble(13, ent->fourPoint.x);
    writer->writeDouble(23, ent->fourPoint.y);
    writer->writeDouble(33, ent->fourPoint.z);
    return true;
}

bool dxfRW::writeSolid(DRW_Solid *ent, dxfWriter* writer){
    writer->writeString(0, "SOLID");
    writeEntity(ent, writer);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbTrace");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(11, ent->secPoint.x);
    writer->writeDouble(21, ent->secPoint.y);
    writer->writeDouble(31, ent->secPoint.z);
    writer->writeDouble(12, ent->thirdPoint.x);
    writer->writeDouble(22, ent->thirdPoint.y);
    writer->writeDouble(32, ent->thirdPoint.z);
    writer->writeDouble(13, ent->fourPoint.x);
    writer->writeDouble(23, ent->fourPoint.y);
    writer->writeDouble(33, ent->fourPoint.z);
    return true;
}

bool dxfRW::write3dface(DRW_3Dface *ent, dxfWriter* writer){
    writer->writeString(0, "3DFACE");
    writeEntity(ent, writer);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbFace");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(11, ent->secPoint.x);
    writer->writeDouble(21, ent->secPoint.y);
    writer->writeDouble(31, ent->secPoint.z);
    writer->writeDouble(12, ent->thirdPoint.x);
    writer->writeDouble(22, ent->thirdPoint.y);
    writer->writeDouble(32, ent->thirdPoint.z);
    writer->writeDouble(13, ent->fourPoint.x);
    writer->writeDouble(23, ent->fourPoint.y);
    writer->writeDouble(33, ent->fourPoint.z);
    writer->writeInt16(70, ent->invisibleflag);
    return true;
}

bool dxfRW::writeLWPolyline(DRW_LWPolyline *ent, dxfWriter* writer){
    if (version > DRW::AC1009) {
        writer->writeString(0, "LWPOLYLINE");
        writeEntity(ent, writer);
        if (version > DRW::AC1009) {
            writer->writeString(100, "AcDbPolyline");
        }
        ent->vertexnum = ent->vertlist.size();
        writer->writeInt32(90, static_cast<int>(ent->vertexnum));
        writer->writeInt16(70, ent->flags);
        writer->writeDouble(43, ent->width);
        if (ent->elevation != 0)
            writer->writeDouble(38, ent->elevation);
        if (ent->thickness != 0)
            writer->writeDouble(39, ent->thickness);
        for (size_t i = 0;  i< ent->vertexnum; i++){
            auto& v = ent->vertlist.at(i);
            writer->writeDouble(10, v->x);
            writer->writeDouble(20, v->y);
            if (v->stawidth != 0)
                writer->writeDouble(40, v->stawidth);
            if (v->endwidth != 0)
                writer->writeDouble(41, v->endwidth);
            if (v->bulge != 0)
                writer->writeDouble(42, v->bulge);
        }
    } else {
        //RLZ: TODO convert lwpolyline in polyline (not exist in acad 12)
    }
    return true;
}

bool dxfRW::writePolyline(DRW_Polyline *ent, dxfWriter* writer) {
    writer->writeString(0, "POLYLINE");
    writeEntity(ent, writer);
    if (version > DRW::AC1009) {
        if (ent->flags & 8 || ent->flags & 16)
            writer->writeString(100, "AcDb3dPolyline");
        else
            writer->writeString(100, "AcDb2dPolyline");
    } else
        writer->writeInt16(66, 1);
    writer->writeDouble(10, 0.0);
    writer->writeDouble(20, 0.0);
    writer->writeDouble(30, ent->basePoint.z);
    if (ent->thickness != 0) {
        writer->writeDouble(39, ent->thickness);
    }
    writer->writeInt16(70, ent->flags);
    if (ent->defstawidth != 0) {
        writer->writeDouble(40, ent->defstawidth);
    }
    if (ent->defendwidth != 0) {
        writer->writeDouble(41, ent->defendwidth);
    }
    if (ent->flags & 16 || ent->flags & 32) {
        writer->writeInt16(71, ent->vertexcount);
        writer->writeInt16(72, ent->facecount);
    }
    if (ent->smoothM != 0) {
        writer->writeInt16(73, ent->smoothM);
    }
    if (ent->smoothN != 0) {
        writer->writeInt16(74, ent->smoothN);
    }
    if (ent->curvetype != 0) {
        writer->writeInt16(75, ent->curvetype);
    }
    DRW_Coord crd  = ent->extPoint;
    if (crd.x != 0 || crd.y != 0 || crd.z != 1) {
        writer->writeDouble(210, crd.x);
        writer->writeDouble(220, crd.y);
        writer->writeDouble(230, crd.z);
    }

    auto vertexnum = ent->vertlist.size();
    for (size_t i = 0;  i< vertexnum; i++){
        DRW_Vertex *v = ent->vertlist.at(i).get();
        writer->writeString(0, "VERTEX");
        writeEntity(ent, writer);
        if (version > DRW::AC1009)
            writer->writeString(100, "AcDbVertex");
        if ( (v->flags & 128) && !(v->flags & 64) ) {
            writer->writeDouble(10, 0);
            writer->writeDouble(20, 0);
            writer->writeDouble(30, 0);
        } else {
            writer->writeDouble(10, v->basePoint.x);
            writer->writeDouble(20, v->basePoint.y);
            writer->writeDouble(30, v->basePoint.z);
        }
        if (v->stawidth != 0)
            writer->writeDouble(40, v->stawidth);
        if (v->endwidth != 0)
            writer->writeDouble(41, v->endwidth);
        if (v->bulge != 0)
            writer->writeDouble(42, v->bulge);
        if (v->flags != 0) {
            writer->writeInt16(70, ent->flags);
        }
        if (v->flags & 2) {
            writer->writeDouble(50, v->tgdir);
        }
        if ( v->flags & 128 ) {
            if (v->vindex1 != 0) {
                writer->writeInt16(71, v->vindex1);
            }
            if (v->vindex2 != 0) {
                writer->writeInt16(72, v->vindex2);
            }
            if (v->vindex3 != 0) {
                writer->writeInt16(73, v->vindex3);
            }
            if (v->vindex4 != 0) {
                writer->writeInt16(74, v->vindex4);
            }
            if ( !(v->flags & 64) ) {
                writer->writeInt32(91, v->identifier);
            }
        }
    }
    writer->writeString(0, "SEQEND");
    writeEntity(ent, writer);
    return true;
}

bool dxfRW::writeSpline(DRW_Spline *ent, dxfWriter* writer){
    if (version > DRW::AC1009) {
        writer->writeString(0, "SPLINE");
        writeEntity(ent, writer);
        if (version > DRW::AC1009) {
            writer->writeString(100, "AcDbSpline");
        }
        writer->writeDouble(210, ent->normalVec.x);
        writer->writeDouble(220, ent->normalVec.y);
        writer->writeDouble(230, ent->normalVec.z);
        writer->writeInt16(70, ent->flags);
        writer->writeInt16(71, ent->degree);
        writer->writeInt16(72, ent->nknots);
        writer->writeInt16(73, ent->ncontrol);
        writer->writeInt16(74, ent->nfit);
        writer->writeDouble(42, ent->tolknot);
        writer->writeDouble(43, ent->tolcontrol);
        //RLZ: warning check if nknots are correct and ncontrol
        for (int i = 0;  i< ent->nknots; i++){
            writer->writeDouble(40, ent->knotslist.at(i));
        }
        for (std::size_t i = 0; i< ent->weightlist.size(); i++) {
            writer->writeDouble(41, ent->weightlist.at(i));
        }
        for (int i = 0;  i< ent->ncontrol; i++){
            auto crd = ent->controllist.at(i);
            writer->writeDouble(10, crd->x);
            writer->writeDouble(20, crd->y);
            writer->writeDouble(30, crd->z);
        }
    } else {
        //RLZ: TODO convert spline in polyline (not exist in acad 12)
    }
    return true;
}

bool dxfRW::writeHatch(DRW_Hatch *ent, dxfWriter* writer){
    if (version > DRW::AC1009) {
        writer->writeString(0, "HATCH");
        writeEntity(ent, writer);
        writer->writeString(100, "AcDbHatch");
        writer->writeDouble(10, 0.0);
        writer->writeDouble(20, 0.0);
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
        writer->writeString(2, ent->name);
        writer->writeInt16(70, ent->solid);
        writer->writeInt16(71, ent->associative);
        ent->loopsnum = ent->looplist.size();
        writer->writeInt16(91, static_cast<int>(ent->loopsnum));
        //write paths data
        for (size_t i = 0;  i< ent->loopsnum; i++){
            DRW_HatchLoop *loop = ent->looplist.at(i).get();
            writer->writeInt16(92, loop->type);
            if ( (loop->type & 2) == 2){
                //RLZ: polyline boundary writeme
            } else {
                //boundary path
                loop->update();
                writer->writeInt16(93, static_cast<int>(loop->numedges));
                for (size_t j = 0; j<loop->numedges; ++j) {
                    switch ( (loop->objlist.at(j))->eType) {
                    case DRW::LINE: {
                        writer->writeInt16(72, 1);
                        DRW_Line* l = (DRW_Line*)loop->objlist.at(j).get();
                        writer->writeDouble(10, l->basePoint.x);
                        writer->writeDouble(20, l->basePoint.y);
                        writer->writeDouble(11, l->secPoint.x);
                        writer->writeDouble(21, l->secPoint.y);
                        break; }
                    case DRW::ARC: {
                        writer->writeInt16(72, 2);
                        DRW_Arc* a = (DRW_Arc*)loop->objlist.at(j).get();
                        writer->writeDouble(10, a->basePoint.x);
                        writer->writeDouble(20, a->basePoint.y);
                        writer->writeDouble(40, a->radious);
                        writer->writeDouble(50, a->staangle*ARAD);
                        writer->writeDouble(51, a->endangle*ARAD);
                        writer->writeInt16(73, a->isccw);
                        break; }
                    case DRW::ELLIPSE: {
                        writer->writeInt16(72, 3);
                        DRW_Ellipse* a = (DRW_Ellipse*)loop->objlist.at(j).get();
                        a->correctAxis();
                        writer->writeDouble(10, a->basePoint.x);
                        writer->writeDouble(20, a->basePoint.y);
                        writer->writeDouble(11, a->secPoint.x);
                        writer->writeDouble(21, a->secPoint.y);
                        writer->writeDouble(40, a->ratio);
                        writer->writeDouble(50, a->staparam*ARAD);
                        writer->writeDouble(51, a->endparam*ARAD);
                        writer->writeInt16(73, a->isccw);
                        break; }
                    case DRW::SPLINE:
                        //RLZ: spline boundary writeme
//                        writer->writeInt16(72, 4);
                        break;
                    default:
                        break;
                    }
                }
                writer->writeInt16(97, 0);
            }
        }
        writer->writeInt16(75, ent->hstyle);
        writer->writeInt16(76, ent->hpattern);
        if (!ent->solid){
            writer->writeDouble(52, ent->angle);
            writer->writeDouble(41, ent->scale);
            writer->writeInt16(77, ent->doubleflag);
            writer->writeInt16(78, ent->deflines);
        }
/*        if (ent->deflines > 0){
            writer->writeInt16(78, ent->deflines);
        }*/
        writer->writeInt32(98, 0);
    } else {
        //RLZ: TODO verify in acad12
    }
    return true;
}

bool dxfRW::writeLeader(DRW_Leader *ent, dxfWriter* writer){
    if (version > DRW::AC1009) {
        writer->writeString(0, "LEADER");
        writeEntity(ent, writer);
        writer->writeString(100, "AcDbLeader");
        writer->writeUtf8String(3, ent->style);
        writer->writeInt16(71, ent->arrow);
        writer->writeInt16(72, ent->leadertype);
        writer->writeInt16(73, ent->flag);
        writer->writeInt16(74, ent->hookline);
        writer->writeInt16(75, ent->hookflag);
        writer->writeDouble(40, ent->textheight);
        writer->writeDouble(41, ent->textwidth);
        writer->writeInt16(76, ent->vertnum);
        writer->writeInt16(76, static_cast<int>(ent->vertexlist.size()));
        for (auto &vert : ent->vertexlist) {
            writer->writeDouble(10, vert->x);
            writer->writeDouble(20, vert->y);
            writer->writeDouble(30, vert->z);
        }
    } else  {
        //RLZ: todo not supported by acad 12 saved as unnamed block
    }
    return true;
}
bool dxfRW::writeDimension(DRW_Dimension *ent, dxfWriter* writer) {
    if (version > DRW::AC1009) {
        writer->writeString(0, "DIMENSION");
        writeEntity(ent, writer);
        writer->writeString(100, "AcDbDimension");
        if (!ent->getName().empty()){
            writer->writeString(2, ent->getName());
        }
        writer->writeDouble(10, ent->getDefPoint().x);
        writer->writeDouble(20, ent->getDefPoint().y);
        writer->writeDouble(30, ent->getDefPoint().z);
        writer->writeDouble(11, ent->getTextPoint().x);
        writer->writeDouble(21, ent->getTextPoint().y);
        writer->writeDouble(31, ent->getTextPoint().z);
        if ( !(ent->type & 32))
            ent->type = ent->type +32;
        writer->writeInt16(70, ent->type);
        if ( !(ent->getText().empty()) )
            writer->writeUtf8String(1, ent->getText());
        writer->writeInt16(71, ent->getAlign());
        if ( ent->getTextLineStyle() != 1)
            writer->writeInt16(72, ent->getTextLineStyle());
        if ( ent->getTextLineFactor() != 1)
            writer->writeDouble(41, ent->getTextLineFactor());
        writer->writeUtf8String(3, ent->getStyle());
        if ( ent->getTextLineFactor() != 0)
            writer->writeDouble(53, ent->getDir());
        writer->writeDouble(210, ent->getExtrusion().x);
        writer->writeDouble(220, ent->getExtrusion().y);
        writer->writeDouble(230, ent->getExtrusion().z);

        switch (ent->eType) {
        case DRW::DIMALIGNED:
        case DRW::DIMLINEAR: {
            DRW_DimAligned * dd = (DRW_DimAligned*)ent;
            writer->writeString(100, "AcDbAlignedDimension");
            DRW_Coord crd = dd->getClonepoint();
            if (crd.x != 0 || crd.y != 0 || crd.z != 0) {
                writer->writeDouble(12, crd.x);
                writer->writeDouble(22, crd.y);
                writer->writeDouble(32, crd.z);
            }
            writer->writeDouble(13, dd->getDef1Point().x);
            writer->writeDouble(23, dd->getDef1Point().y);
            writer->writeDouble(33, dd->getDef1Point().z);
            writer->writeDouble(14, dd->getDef2Point().x);
            writer->writeDouble(24, dd->getDef2Point().y);
            writer->writeDouble(34, dd->getDef2Point().z);
            if (ent->eType == DRW::DIMLINEAR) {
                DRW_DimLinear * dl = (DRW_DimLinear*)ent;
                if (dl->getAngle() != 0)
                    writer->writeDouble(50, dl->getAngle());
                if (dl->getOblique() != 0)
                    writer->writeDouble(52, dl->getOblique());
                writer->writeString(100, "AcDbRotatedDimension");
            }
            break; }
        case DRW::DIMRADIAL: {
            DRW_DimRadial * dd = (DRW_DimRadial*)ent;
            writer->writeString(100, "AcDbRadialDimension");
            writer->writeDouble(15, dd->getDiameterPoint().x);
            writer->writeDouble(25, dd->getDiameterPoint().y);
            writer->writeDouble(35, dd->getDiameterPoint().z);
            writer->writeDouble(40, dd->getLeaderLength());
            break; }
        case DRW::DIMDIAMETRIC: {
            DRW_DimDiametric * dd = (DRW_DimDiametric*)ent;
            writer->writeString(100, "AcDbDiametricDimension");
            writer->writeDouble(15, dd->getDiameter1Point().x);
            writer->writeDouble(25, dd->getDiameter1Point().y);
            writer->writeDouble(35, dd->getDiameter1Point().z);
            writer->writeDouble(40, dd->getLeaderLength());
            break; }
        case DRW::DIMANGULAR: {
            DRW_DimAngular * dd = (DRW_DimAngular*)ent;
            writer->writeString(100, "AcDb2LineAngularDimension");
            writer->writeDouble(13, dd->getFirstLine1().x);
            writer->writeDouble(23, dd->getFirstLine1().y);
            writer->writeDouble(33, dd->getFirstLine1().z);
            writer->writeDouble(14, dd->getFirstLine2().x);
            writer->writeDouble(24, dd->getFirstLine2().y);
            writer->writeDouble(34, dd->getFirstLine2().z);
            writer->writeDouble(15, dd->getSecondLine1().x);
            writer->writeDouble(25, dd->getSecondLine1().y);
            writer->writeDouble(35, dd->getSecondLine1().z);
            writer->writeDouble(16, dd->getDimPoint().x);
            writer->writeDouble(26, dd->getDimPoint().y);
            writer->writeDouble(36, dd->getDimPoint().z);
            break; }
        case DRW::DIMANGULAR3P: {
            DRW_DimAngular3p * dd = (DRW_DimAngular3p*)ent;
            writer->writeDouble(13, dd->getFirstLine().x);
            writer->writeDouble(23, dd->getFirstLine().y);
            writer->writeDouble(33, dd->getFirstLine().z);
            writer->writeDouble(14, dd->getSecondLine().x);
            writer->writeDouble(24, dd->getSecondLine().y);
            writer->writeDouble(34, dd->getSecondLine().z);
            writer->writeDouble(15, dd->getVertexPoint().x);
            writer->writeDouble(25, dd->getVertexPoint().y);
            writer->writeDouble(35, dd->getVertexPoint().z);
            break; }
        case DRW::DIMORDINATE: {
            DRW_DimOrdinate * dd = (DRW_DimOrdinate*)ent;
            writer->writeString(100, "AcDbOrdinateDimension");
            writer->writeDouble(13, dd->getFirstLine().x);
            writer->writeDouble(23, dd->getFirstLine().y);
            writer->writeDouble(33, dd->getFirstLine().z);
            writer->writeDouble(14, dd->getSecondLine().x);
            writer->writeDouble(24, dd->getSecondLine().y);
            writer->writeDouble(34, dd->getSecondLine().z);
            break; }
        default:
            break;
        }
    } else  {
        //RLZ: todo not supported by acad 12 saved as unnamed block
    }
    return true;
}

bool dxfRW::writeInsert(DRW_Insert *ent, dxfWriter* writer){
    writer->writeString(0, "INSERT");
    writeEntity(ent, writer);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbBlockReference");
        writer->writeUtf8String(2, ent->name);
    } else
        writer->writeUtf8Caps(2, ent->name);
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(41, ent->xscale);
    writer->writeDouble(42, ent->yscale);
    writer->writeDouble(43, ent->zscale);
    writer->writeDouble(50, (ent->angle)*ARAD); //in dxf angle is writed in degrees
    writer->writeInt16(70, ent->colcount);
    writer->writeInt16(71, ent->rowcount);
    writer->writeDouble(44, ent->colspace);
    writer->writeDouble(45, ent->rowspace);
    return true;
}

bool dxfRW::writeText(DRW_Text *ent, dxfWriter* writer){
    writer->writeString(0, "TEXT");
    writeEntity(ent, writer);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbText");
    }
//    writer->writeDouble(39, ent->thickness);
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(40, ent->height);
    writer->writeUtf8String(1, ent->text);
    writer->writeDouble(50, ent->angle);
    writer->writeDouble(41, ent->widthscale);
    writer->writeDouble(51, ent->oblique);
    if (version > DRW::AC1009)
        writer->writeUtf8String(7, ent->style);
    else
        writer->writeUtf8Caps(7, ent->style);
    writer->writeInt16(71, ent->textgen);
    if (ent->alignH != DRW_Text::HLeft) {
        writer->writeInt16(72, ent->alignH);
    }
    if (ent->alignH != DRW_Text::HLeft || ent->alignV != DRW_Text::VBaseLine) {
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
    }
    writer->writeDouble(210, ent->extPoint.x);
    writer->writeDouble(220, ent->extPoint.y);
    writer->writeDouble(230, ent->extPoint.z);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbText");
    }
    if (ent->alignV != DRW_Text::VBaseLine) {
        writer->writeInt16(73, ent->alignV);
    }
    return true;
}

bool dxfRW::writeMText(DRW_MText *ent, dxfWriter* writer){
    if (version > DRW::AC1009) {
        writer->writeString(0, "MTEXT");
        writeEntity(ent, writer);
        writer->writeString(100, "AcDbMText");
        writer->writeDouble(10, ent->basePoint.x);
        writer->writeDouble(20, ent->basePoint.y);
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(40, ent->height);
        writer->writeDouble(41, ent->widthscale);
        writer->writeInt16(71, ent->textgen);
        writer->writeInt16(72, ent->alignH);
        std::string text = writer->fromUtf8String(ent->text);

        int i;
        for(i =0; (text.size()-i) > 250; ) {
            writer->writeString(3, text.substr(i, 250));
            i +=250;
        }
        writer->writeString(1, text.substr(i));
        writer->writeString(7, ent->style);
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
        writer->writeDouble(50, ent->angle);
        writer->writeInt16(73, ent->alignV);
        writer->writeDouble(44, ent->interlin);
//RLZ ... 11, 21, 31 needed?
    } else {
        //RLZ: TODO convert mtext in text lines (not exist in acad 12)
    }
    return true;
}

bool dxfRW::writeViewport(DRW_Viewport *ent, dxfWriter* writer) {
    writer->writeString(0, "VIEWPORT");
    writeEntity(ent, writer);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbViewport");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0)
        writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(40, ent->pswidth);
    writer->writeDouble(41, ent->psheight);
    writer->writeInt16(68, ent->vpstatus);
    writer->writeInt16(69, ent->vpID);
    writer->writeDouble(12, ent->centerPX);//RLZ: verify if exist in V12
    writer->writeDouble(22, ent->centerPY);//RLZ: verify if exist in V12
    return true;
}

DRW_ImageDef* dxfRW::writeImage(DRW_Image *ent, const std::string& name, dxfWriter* writer){
    if (version > DRW::AC1009) {
        //search if exist imagedef with this mane (image inserted more than 1 time)
        //RLZ: imagedef_reactor seem needed to read in acad
        DRW_ImageDef *id = NULL;
        for (unsigned int i=0; i<imageDef.size(); i++) {
            if (imageDef.at(i)->name == name ) {
                id = imageDef.at(i);
                continue;
            }
        }
        if (id == NULL) {
            id = new DRW_ImageDef();
            imageDef.push_back(id);
            id->handle = ++entCount;
        }
        id->name = name;
        std::string idReactor = DRW::toHexStr(++entCount);

        writer->writeString(0, "IMAGE");
        writeEntity(ent, writer);
        writer->writeString(100, "AcDbRasterImage");
        writer->writeDouble(10, ent->basePoint.x);
        writer->writeDouble(20, ent->basePoint.y);
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
        writer->writeDouble(12, ent->vVector.x);
        writer->writeDouble(22, ent->vVector.y);
        writer->writeDouble(32, ent->vVector.z);
        writer->writeDouble(13, ent->sizeu);
        writer->writeDouble(23, ent->sizev);
        writer->writeString(340, DRW::toHexStr(id->handle));
        writer->writeInt16(70, 1);
        writer->writeInt16(280, ent->clip);
        writer->writeInt16(281, ent->brightness);
        writer->writeInt16(282, ent->contrast);
        writer->writeInt16(283, ent->fade);
        writer->writeString(360, idReactor);
        id->reactors[idReactor] = DRW::toHexStr(ent->handle);
        return id;
    }
    return NULL; //not exist in acad 12
}

bool dxfRW::writeBlockRecord(std::string name, dxfWriter* writer){
    if (version > DRW::AC1009) {
        writer->writeString(0, "BLOCK_RECORD");
        writer->writeString(5, DRW::toHexStr(++entCount));

        blockMap[name] = entCount;
        entCount = 2+entCount;//reserve 2 for BLOCK & ENDBLOCK
        if (version > DRW::AC1014) {
            writer->writeString(330, "1");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbBlockTableRecord");
        writer->writeUtf8String(2, name);
        if (version > DRW::AC1018) {
            //    writer->writeInt16(340, 22);
            writer->writeInt16(70, 0);
            writer->writeInt16(280, 1);
            writer->writeInt16(281, 0);
        }
    }
    return true;
}

bool dxfRW::writeBlock(DRW_Block *bk, dxfWriter* writer){
    if (writingBlock) {
        writer->writeString(0, "ENDBLK");
        if (version > DRW::AC1009) {
            writer->writeString(5, DRW::toHexStr(currHandle+2));
            if (version > DRW::AC1014) {
                writer->writeString(330, DRW::toHexStr(currHandle));
            }
            writer->writeString(100, "AcDbEntity");
        }
        writer->writeString(8, "0");
        if (version > DRW::AC1009) {
            writer->writeString(100, "AcDbBlockEnd");
        }
    }
    writingBlock = true;
    writer->writeString(0, "BLOCK");
    if (version > DRW::AC1009) {
        currHandle = (*(blockMap.find(bk->name))).second;
        writer->writeString(5, DRW::toHexStr(currHandle+1));
        if (version > DRW::AC1014) {
            writer->writeString(330, DRW::toHexStr(currHandle));
        }
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, "0");
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbBlockBegin");
        writer->writeUtf8String(2, bk->name);
    } else
        writer->writeUtf8Caps(2, bk->name);
    writer->writeInt16(70, bk->flags);
    writer->writeDouble(10, bk->basePoint.x);
    writer->writeDouble(20, bk->basePoint.y);
    if (bk->basePoint.z != 0.0) {
        writer->writeDouble(30, bk->basePoint.z);
    }
    if (version > DRW::AC1009)
        writer->writeUtf8String(3, bk->name);
    else
        writer->writeUtf8Caps(3, bk->name);
    if(version >= DRW::AC1014) {
        writeAppData(bk->appData, writer);
    }
    writer->writeString(1, "");

    return true;
}

bool dxfRW::writeTables(dxfWriter *writer) {
    writer->writeString(0, "TABLE");
    writer->writeString(2, "VPORT");
    if (version > DRW::AC1009) {
        writer->writeString(5, "8");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 1); //end table def
/*** VPORT ***/
    dimstyleStd =false;
    iface->writeVports(writer);
    if (!dimstyleStd) {
        DRW_Vport portact;
        portact.name = "*ACTIVE";
        writeVport(&portact, writer);
    }
    writer->writeString(0, "ENDTAB");
/*** LTYPE ***/
    writer->writeString(0, "TABLE");
    writer->writeString(2, "LTYPE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "5");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 4); //end table def
//Mandatory linetypes
    writer->writeString(0, "LTYPE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "14");
        if (version > DRW::AC1014) {
            writer->writeString(330, "5");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLinetypeTableRecord");
        writer->writeString(2, "ByBlock");
    } else
        writer->writeString(2, "BYBLOCK");
    writer->writeInt16(70, 0);
    writer->writeString(3, "");
    writer->writeInt16(72, 65);
    writer->writeInt16(73, 0);
    writer->writeDouble(40, 0.0);

    writer->writeString(0, "LTYPE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "15");
        if (version > DRW::AC1014) {
            writer->writeString(330, "5");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLinetypeTableRecord");
        writer->writeString(2, "ByLayer");
    } else {
        writer->writeString(2, "BYLAYER");
    }
    writer->writeInt16(70, 0);
    writer->writeString(3, "");
    writer->writeInt16(72, 65);
    writer->writeInt16(73, 0);
    writer->writeDouble(40, 0.0);

    writer->writeString(0, "LTYPE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "16");
        if (version > DRW::AC1014) {
            writer->writeString(330, "5");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLinetypeTableRecord");
        writer->writeString(2, "Continuous");
    } else {
        writer->writeString(2, "CONTINUOUS");
    }
    writer->writeInt16(70, 0);
    writer->writeString(3, "Solid line");
    writer->writeInt16(72, 65);
    writer->writeInt16(73, 0);
    writer->writeDouble(40, 0.0);
//Application linetypes
    iface->writeLTypes(writer);
    writer->writeString(0, "ENDTAB");
/*** LAYER ***/
    writer->writeString(0, "TABLE");
    writer->writeString(2, "LAYER");
    if (version > DRW::AC1009) {
        writer->writeString(5, "2");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 1); //end table def
    wlayer0 =false;
    iface->writeLayers(writer);
    if (!wlayer0) {
        DRW_Layer lay0;
        lay0.name = "0";
        writeLayer(&lay0, writer);
    }
    writer->writeString(0, "ENDTAB");
/*** STYLE ***/
    writer->writeString(0, "TABLE");
    writer->writeString(2, "STYLE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "3");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 3); //end table def
    dimstyleStd =false;
    iface->writeTextstyles(writer);
    if (!dimstyleStd) {
        DRW_Textstyle tsty;
        tsty.name = "Standard";
        writeTextstyle(&tsty, writer);
    }
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "VIEW");
    if (version > DRW::AC1009) {
        writer->writeString(5, "6");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 0); //end table def
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "UCS");
    if (version > DRW::AC1009) {
        writer->writeString(5, "7");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 0); //end table def
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "APPID");
    if (version > DRW::AC1009) {
        writer->writeString(5, "9");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 1); //end table def
    writer->writeString(0, "APPID");
    if (version > DRW::AC1009) {
        writer->writeString(5, "12");
        if (version > DRW::AC1014) {
            writer->writeString(330, "9");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbRegAppTableRecord");
    }
    writer->writeString(2, "ACAD");
    writer->writeInt16(70, 0);
    iface->writeAppId(writer);
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "DIMSTYLE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "A");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 1); //end table def
    if (version > DRW::AC1014) {
        writer->writeString(100, "AcDbDimStyleTable");
        writer->writeInt16(71, 1); //end table def
    }
    dimstyleStd =false;
    iface->writeDimstyles(writer);
    if (!dimstyleStd) {
        DRW_Dimstyle dsty;
        dsty.name = "Standard";
        writeDimstyle(&dsty, writer);
    }
    writer->writeString(0, "ENDTAB");

    if (version > DRW::AC1009) {
        writer->writeString(0, "TABLE");
        writer->writeString(2, "BLOCK_RECORD");
        writer->writeString(5, "1");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
        writer->writeInt16(70, 2); //end table def
        writer->writeString(0, "BLOCK_RECORD");
        writer->writeString(5, "1F");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbBlockTableRecord");
        writer->writeString(2, "*Model_Space");
        if (version > DRW::AC1018) {
            //    writer->writeInt16(340, 22);
            writer->writeInt16(70, 0);
            writer->writeInt16(280, 1);
            writer->writeInt16(281, 0);
        }
        writer->writeString(0, "BLOCK_RECORD");
        writer->writeString(5, "1E");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbBlockTableRecord");
        writer->writeString(2, "*Paper_Space");
        if (version > DRW::AC1018) {
            //    writer->writeInt16(340, 22);
            writer->writeInt16(70, 0);
            writer->writeInt16(280, 1);
            writer->writeInt16(281, 0);
        }
    }
    /* always call writeBlockRecords to iface for prepare unnamed blocks */
    iface->writeBlockRecords(writer);
    if (version > DRW::AC1009) {
        writer->writeString(0, "ENDTAB");
    }
return true;
}

bool dxfRW::writeBlocks(dxfWriter *writer) {
    writer->writeString(0, "BLOCK");
    if (version > DRW::AC1009) {
        writer->writeString(5, "20");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1F");
        }
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, "0");
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbBlockBegin");
        writer->writeString(2, "*Model_Space");
    } else { writer->writeString(2, "$MODEL_SPACE"); }
    writer->writeInt16(70, 0);
    writer->writeDouble(10, 0.0);
    writer->writeDouble(20, 0.0);
    writer->writeDouble(30, 0.0);
    if (version > DRW::AC1009) {
        writer->writeString(3, "*Model_Space");
    }
    else {
        writer->writeString(3, "$MODEL_SPACE");
    }
    writer->writeString(1, "");
    writer->writeString(0, "ENDBLK");
    if (version > DRW::AC1009) {
        writer->writeString(5, "21");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1F");
        }
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, "0");
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbBlockEnd");
    }

    writer->writeString(0, "BLOCK");
    if (version > DRW::AC1009) {
        writer->writeString(5, "1C");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1B");
        }
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, "0");
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbBlockBegin");
        writer->writeString(2, "*Paper_Space");
    } else
        writer->writeString(2, "$PAPER_SPACE");
    writer->writeInt16(70, 0);
    writer->writeDouble(10, 0.0);
    writer->writeDouble(20, 0.0);
    writer->writeDouble(30, 0.0);
    if (version > DRW::AC1009) {
        writer->writeString(3, "*Paper_Space");
    }
    else {
        writer->writeString(3, "$PAPER_SPACE");
    }
    writer->writeString(1, "");
    writer->writeString(0, "ENDBLK");
    if (version > DRW::AC1009) {
        writer->writeString(5, "1D");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1F");
        }
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, "0");
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbBlockEnd");
    }
    writingBlock = false;
    iface->writeBlocks(writer);
    if (writingBlock) {
        writingBlock = false;
        writer->writeString(0, "ENDBLK");
        if (version > DRW::AC1009) {
            writer->writeString(5, DRW::toHexStr(currHandle+2));
//            writer->writeString(5, "1D");
            if (version > DRW::AC1014) {
                writer->writeString(330, DRW::toHexStr(currHandle));
            }
            writer->writeString(100, "AcDbEntity");
        }
        writer->writeString(8, "0");
        if (version > DRW::AC1009) {
            writer->writeString(100, "AcDbBlockEnd");
        }
    }
    return true;
}

bool dxfRW::writeObjects(dxfWriter *writer) {
    writer->writeString(0, "DICTIONARY");
    std::string imgDictH;
    writer->writeString(5, "C");
    if (version > DRW::AC1014) {
        writer->writeString(330, "0");
    }
    writer->writeString(100, "AcDbDictionary");
    writer->writeInt16(281, 1);
    writer->writeString(3, "ACAD_GROUP");
    writer->writeString(350, "D");
    if (imageDef.size() != 0) {
        writer->writeString(3, "ACAD_IMAGE_DICT");
        imgDictH = DRW::toHexStr(++entCount);
        writer->writeString(350, imgDictH);
    }
    writer->writeString(0, "DICTIONARY");
    writer->writeString(5, "D");
    writer->writeString(330, "C");
    writer->writeString(100, "AcDbDictionary");
    writer->writeInt16(281, 1);
//write IMAGEDEF_REACTOR
    for (unsigned int i=0; i<imageDef.size(); i++) {
        DRW_ImageDef *id = imageDef.at(i);
        for (auto it=id->reactors.begin() ; it != id->reactors.end(); ++it ) {
            writer->writeString(0, "IMAGEDEF_REACTOR");
            writer->writeString(5, (*it).first);
            writer->writeString(330, (*it).second);
            writer->writeString(100, "AcDbRasterImageDefReactor");
            writer->writeInt16(90, 2); //version 2=R14 to v2010
            writer->writeString(330, (*it).second);
        }
    }
    if (imageDef.size() != 0) {
        writer->writeString(0, "DICTIONARY");
        writer->writeString(5, imgDictH);
        writer->writeString(330, "C");
        writer->writeString(100, "AcDbDictionary");
        writer->writeInt16(281, 1);
        for (unsigned int i=0; i<imageDef.size(); i++) {
            size_t f1, f2;
            f1 = imageDef.at(i)->name.find_last_of("/\\");
            f2 =imageDef.at(i)->name.find_last_of('.');
            ++f1;
            writer->writeString(3, imageDef.at(i)->name.substr(f1,f2-f1));
            writer->writeString(350, DRW::toHexStr(imageDef.at(i)->handle) );
        }
    }
    for (unsigned int i=0; i<imageDef.size(); i++) {
        DRW_ImageDef *id = imageDef.at(i);
        writer->writeString(0, "IMAGEDEF");
        writer->writeString(5, DRW::toHexStr(id->handle) );
        if (version > DRW::AC1014) {
//            writer->writeString(330, "0"); handle to DICTIONARY
        }
        writer->writeString(102, "{ACAD_REACTORS");
        for (auto it=id->reactors.begin() ; it != id->reactors.end(); ++it ) {
            writer->writeString(330, (*it).first);
        }
        writer->writeString(102, "}");
        writer->writeString(100, "AcDbRasterImageDef");
        writer->writeInt16(90, 0); //version 0=R14 to v2010
        writer->writeUtf8String(1, id->name);
        writer->writeDouble(10, id->u);
        writer->writeDouble(20, id->v);
        writer->writeDouble(11, id->up);
        writer->writeDouble(21, id->vp);
        writer->writeInt16(280, id->loaded);
        writer->writeInt16(281, id->resolution);
    }
    //no more needed imageDef, delete it
    while (!imageDef.empty()) {
       imageDef.pop_back();
    }

    iface->writeObjects(writer);

    return true;
}

bool dxfRW::writeExtData(const std::vector<DRW_Variant*> &ed, dxfWriter* writer){
    for (std::vector<DRW_Variant*>::const_iterator it=ed.begin(); it!=ed.end(); ++it){
        switch ((*it)->code()) {
        case 1000:
        case 1001:
        case 1002:
        case 1003:
        case 1004:
        case 1005:
        {int cc = (*it)->code();
            if ((*it)->type() == DRW_Variant::STRING)
                writer->writeUtf8String(cc, *(*it)->content.s);
//            writer->writeUtf8String((*it)->code, (*it)->content.s);
            break;}
        case 1010:
        case 1011:
        case 1012:
        case 1013:
            if ((*it)->type() == DRW_Variant::COORD) {
                writer->writeDouble((*it)->code(), (*it)->content.v->x);
                writer->writeDouble((*it)->code()+10 , (*it)->content.v->y);
                writer->writeDouble((*it)->code()+20 , (*it)->content.v->z);
            }
            break;
        case 1040:
        case 1041:
        case 1042:
            if ((*it)->type() == DRW_Variant::DOUBLE)
                writer->writeDouble((*it)->code(), (*it)->content.d);
            break;
        case 1070:
            if ((*it)->type() == DRW_Variant::INTEGER)
                writer->writeInt16((*it)->code(), (*it)->content.i);
            break;
        case 1071:
            if ((*it)->type() == DRW_Variant::INTEGER)
                writer->writeInt32((*it)->code(), (*it)->content.i);
            break;
        default:
            break;
        }
    }
    return true;
}

/********* Reader Process *********/

bool dxfRW::processDxf(dxfReader *reader) {
    drw_dbg("dxfRW::processDxf() start processing dxf\n");
    int code {-1};
    bool inSection {false};

    reader->setIgnoreComments( false);
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg(" code\n");
        /* at this level we should only get:
         999 - Comment
         0 - SECTION or EOF
         2 - section name
         everything else between "2 - section name" and "0 - ENDSEC" is handled in process() methods
        */
        switch (code) {
        case 999: // when DXF was created by libdxfrw, first record is a comment with dxfrw version info
            header.addComment( reader->getString());
            continue;

        case 0:
            // ignore further comments, as libdxfrw doesn't support comments in sections
            reader->setIgnoreComments( true);
            if (!inSection) {
                std::string sectionstr {reader->getString()};

                if ("SECTION" == sectionstr) {
                    drw_dbg(sectionstr); drw_dbg(" new section\n");
                    inSection = true;
                    continue;
                }
                if ("EOF" == sectionstr) {
                    return true;  //found EOF terminate
                }
            }
            else {
                // in case SECTION was unknown or not supported
                if ("ENDSEC" == reader->getString()) {
                    inSection = false;
                }
            }
            break;

        case 2:
            if (inSection) {
                bool processed {false};
                std::string sectionname {reader->getString()};

                drw_dbg(sectionname); drw_dbg(" process section\n");
                if ("HEADER" == sectionname) {
                    processed = processHeader(reader);
                }
                else if ("TABLES" == sectionname) {
                    processed = processTables(reader);
                }
                else if ("BLOCKS" == sectionname) {
                    processed = processBlocks(reader);
                }
                else if ("ENTITIES" == sectionname) {
                    processed = processEntities(false, reader);
                }
                else if ("OBJECTS" == sectionname) {
                    processed = processObjects(reader);
                }
                else {
                    //TODO handle CLASSES

                    drw_dbg("section unknown or not supported\n");
                    continue;
                }

                if (!processed) {
                    drw_dbg("  failed\n");
                    return setError(DRW::BAD_READ_SECTION);
                }

                inSection = false;
            }
            continue;

        default:
            // landing here means an unknown or not supported SECTION
            inSection = false;
            break;
        }
    }

    if (0 == code && "EOF" == reader->getString()) {
        // in case the final EOF has no newline we end up here!
        // this is caused by filestr->good() which is false for missing newline on EOF
        return true;
    }

    return setError(DRW::BAD_UNKNOWN);
}

/********* Header Section *********/

bool dxfRW::processHeader(dxfReader *reader) {
    drw_dbg("dxfRW::processHeader\n");
    int code;
    std::string sectionstr;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg(" processHeader\n");
        if (code == 0) {
            sectionstr = reader->getString();
            drw_dbg(sectionstr); drw_dbg(" processHeader\n\n");
            if (sectionstr == "ENDSEC") {
                iface->addHeader(&header);
                return true;  //found ENDSEC terminate
            }

            drw_dbg("unexpected 0 code in header!\n");
            return setError(DRW::BAD_READ_HEADER);
        }

        header.parseCode(code, reader);
    }

    return setError(DRW::BAD_READ_HEADER);
}

/********* Tables Section *********/

bool dxfRW::processTables(dxfReader *reader) {
    drw_dbg("dxfRW::processTables\n");
    int code;
    std::string sectionstr;
    bool more = true;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        if (code == 0) {
            sectionstr = reader->getString();
            drw_dbg(sectionstr); drw_dbg(" processHeader\n\n");
            if (sectionstr == "TABLE") {
                more = reader->readRec(&code);
                drw_dbg(code); drw_dbg("\n");
                if (!more) {
                    return setError(DRW::BAD_READ_TABLES); //wrong dxf file
                }
                if (code == 2) {
                    sectionstr = reader->getString();
                    drw_dbg(sectionstr); drw_dbg(" processHeader\n\n");
                //found section, process it
                    if (sectionstr == "LTYPE") {
                        processLType(reader);
                    } else if (sectionstr == "LAYER") {
                        processLayer(reader);
                    } else if (sectionstr == "STYLE") {
                        processTextStyle(reader);
                    } else if (sectionstr == "VPORT") {
                        processVports(reader);
                    } else if (sectionstr == "VIEW") {
//                        processView();
                    } else if (sectionstr == "UCS") {
//                        processUCS();
                    } else if (sectionstr == "APPID") {
                        processAppId(reader);
                    } else if (sectionstr == "DIMSTYLE") {
                        processDimStyle(reader);
                    } else if (sectionstr == "BLOCK_RECORD") {
//                        processBlockRecord();
                    }
                }
            } else if (sectionstr == "ENDSEC") {
                return true;  //found ENDSEC terminate
            }
        }
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processLType(dxfReader *reader) {
    drw_dbg("dxfRW::processLType\n");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_LType ltype;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        if (code == 0) {
            if (reading) {
                ltype.update();
                iface->addLType(ltype);
            }
            sectionstr = reader->getString();
            drw_dbg(sectionstr); drw_dbg("\n");
            if (sectionstr == "LTYPE") {
                reading = true;
                ltype.reset();
            } else if (sectionstr == "ENDTAB") {
                return true;  //found ENDTAB terminate
            }
        } else if (reading)
            ltype.parseCode(code, reader);
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processLayer(dxfReader *reader) {
    drw_dbg("dxfRW::processLayer\n");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_Layer layer;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        if (code == 0) {
            if (reading)
                iface->addLayer(layer);
            sectionstr = reader->getString();
            drw_dbg(sectionstr); drw_dbg("\n");
            if (sectionstr == "LAYER") {
                reading = true;
                layer.reset();
            } else if (sectionstr == "ENDTAB") {
                return true;  //found ENDTAB terminate
            }
        } else if (reading)
            layer.parseCode(code, reader);
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processDimStyle(dxfReader *reader) {
    drw_dbg("dxfRW::processDimStyle");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_Dimstyle dimSty;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        if (code == 0) {
            if (reading)
                iface->addDimStyle(dimSty);
            sectionstr = reader->getString();
            drw_dbg(sectionstr); drw_dbg("\n");
            if (sectionstr == "DIMSTYLE") {
                reading = true;
                dimSty.reset();
            } else if (sectionstr == "ENDTAB") {
                return true;  //found ENDTAB terminate
            }
        } else if (reading)
            dimSty.parseCode(code, reader);
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processTextStyle(dxfReader *reader){
    drw_dbg("dxfRW::processTextStyle");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_Textstyle TxtSty;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        if (code == 0) {
            if (reading)
                iface->addTextStyle(TxtSty);
            sectionstr = reader->getString();
            drw_dbg(sectionstr); drw_dbg("\n");
            if (sectionstr == "STYLE") {
                reading = true;
                TxtSty.reset();
            } else if (sectionstr == "ENDTAB") {
                return true;  //found ENDTAB terminate
            }
        } else if (reading)
            TxtSty.parseCode(code, reader);
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processVports(dxfReader *reader){
    drw_dbg("dxfRW::processVports");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_Vport vp;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        if (code == 0) {
            if (reading)
                iface->addVport(vp);
            sectionstr = reader->getString();
            drw_dbg(sectionstr); drw_dbg("\n");
            if (sectionstr == "VPORT") {
                reading = true;
                vp.reset();
            } else if (sectionstr == "ENDTAB") {
                return true;  //found ENDTAB terminate
            }
        } else if (reading)
            vp.parseCode(code, reader);
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processAppId(dxfReader *reader){
    drw_dbg("dxfRW::processAppId");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_AppId vp;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        if (code == 0) {
            if (reading)
                iface->addAppId(vp);
            sectionstr = reader->getString();
            drw_dbg(sectionstr); drw_dbg("\n");
            if (sectionstr == "APPID") {
                reading = true;
                vp.reset();
            } else if (sectionstr == "ENDTAB") {
                return true;  //found ENDTAB terminate
            }
        } else if (reading)
            vp.parseCode(code, reader);
    }

    return setError(DRW::BAD_READ_TABLES);
}

/********* Block Section *********/

bool dxfRW::processBlocks(dxfReader *reader) {
    drw_dbg("dxfRW::processBlocks\n");
    int code;
    std::string sectionstr;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        if (code == 0) {
            sectionstr = reader->getString();
            drw_dbg(sectionstr); drw_dbg("\n");
            if (sectionstr == "BLOCK") {
                processBlock(reader);
            } else if (sectionstr == "ENDSEC") {
                return true;  //found ENDSEC terminate
            }
        }
    }

    return setError(DRW::BAD_READ_BLOCKS);
}

bool dxfRW::processBlock(dxfReader *reader) {
    drw_dbg("dxfRW::processBlock");
    int code;
    DRW_Block block;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            iface->addBlock(block);
            if (nextentity == "ENDBLK") {
                iface->endBlock();
                return true;  //found ENDBLK, terminate
            } else {
                processEntities(true, reader);
                iface->endBlock();
                return true;  //found ENDBLK, terminate
            }
        }
        default:
            block.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_BLOCKS);
}


/********* Entities Section *********/

bool dxfRW::processEntities(bool isblock, dxfReader *reader) {
    drw_dbg("dxfRW::processEntities\n");
    int code;
    if (!reader->readRec(&code)){
        return setError(DRW::BAD_READ_ENTITIES);
    }

    if (code == 0) {
        nextentity = reader->getString();
    } else if (!isblock) {
        return setError(DRW::BAD_READ_ENTITIES);  //first record in entities is 0
    }

    bool processed {false};
    do {
        if (nextentity == "ENDSEC" || nextentity == "ENDBLK") {
            return true;  //found ENDSEC or ENDBLK terminate
        }
        else if (nextentity == "POINT") {
            processed = processPoint(reader);
        } else if (nextentity == "LINE") {
            processed = processLine(reader);
        } else if (nextentity == "CIRCLE") {
            processed = processCircle(reader);
        } else if (nextentity == "ARC") {
            processed = processArc(reader);
        } else if (nextentity == "ELLIPSE") {
            processed = processEllipse(reader);
        } else if (nextentity == "TRACE") {
            processed = processTrace(reader);
        } else if (nextentity == "SOLID") {
            processed = processSolid(reader);
        } else if (nextentity == "INSERT") {
            processed = processInsert(reader);
        } else if (nextentity == "LWPOLYLINE") {
            processed = processLWPolyline(reader);
        } else if (nextentity == "POLYLINE") {
            processed = processPolyline(reader);
        } else if (nextentity == "TEXT") {
            processed = processText(reader);
        } else if (nextentity == "MTEXT") {
            processed = processMText(reader);
        } else if (nextentity == "HATCH") {
            processed = processHatch(reader);
        } else if (nextentity == "SPLINE") {
            processed = processSpline(reader);
        } else if (nextentity == "3DFACE") {
            processed = process3dface(reader);
        } else if (nextentity == "VIEWPORT") {
            processed = processViewport(reader);
        } else if (nextentity == "IMAGE") {
            processed = processImage(reader);
        } else if (nextentity == "DIMENSION") {
            processed = processDimension(reader);
        } else if (nextentity == "LEADER") {
            processed = processLeader(reader);
        } else if (nextentity == "RAY") {
            processed = processRay(reader);
        } else if (nextentity == "XLINE") {
            processed = processXline(reader);
        } else {
            if (!reader->readRec(&code)) {
                return setError(DRW::BAD_READ_ENTITIES); //end of file without ENDSEC
            }

            if (code == 0) {
                nextentity = reader->getString();
            }
            processed = true;
        }
    } while (processed);

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processEllipse(dxfReader *reader) {
    drw_dbg("dxfRW::processEllipse");
    int code;
    DRW_Ellipse ellipse;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            if (applyExt)
                ellipse.applyExtrusion();
            iface->addEllipse(ellipse);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            ellipse.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processTrace(dxfReader *reader) {
    drw_dbg("dxfRW::processTrace");
    int code;
    DRW_Trace trace;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            if (applyExt)
                trace.applyExtrusion();
            iface->addTrace(trace);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            trace.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processSolid(dxfReader *reader) {
    drw_dbg("dxfRW::processSolid");
    int code;
    DRW_Solid solid;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            if (applyExt)
                solid.applyExtrusion();
            iface->addSolid(solid);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            solid.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::process3dface(dxfReader *reader) {
    drw_dbg("dxfRW::process3dface");
    int code;
    DRW_3Dface face;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            iface->add3dFace(face);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            face.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processViewport(dxfReader *reader) {
    drw_dbg("dxfRW::processViewport");
    int code;
    DRW_Viewport vp;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            iface->addViewport(vp);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            vp.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processPoint(dxfReader *reader) {
    drw_dbg("dxfRW::processPoint\n");
    int code;
    DRW_Point point;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            iface->addPoint(point);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            point.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processLine(dxfReader *reader) {
    drw_dbg("dxfRW::processLine\n");
    int code;
    DRW_Line line;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            iface->addLine(line);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            line.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processRay(dxfReader *reader) {
    drw_dbg("dxfRW::processRay\n");
    int code;
    DRW_Ray line;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            iface->addRay(line);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            line.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processXline(dxfReader *reader) {
    drw_dbg("dxfRW::processXline\n");
    int code;
    DRW_Xline line;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            iface->addXline(line);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            line.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processCircle(dxfReader *reader) {
    drw_dbg("dxfRW::processPoint\n");
    int code;
    DRW_Circle circle;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            if (applyExt)
                circle.applyExtrusion();
            iface->addCircle(circle);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            circle.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processArc(dxfReader *reader) {
    drw_dbg("dxfRW::processPoint\n");
    int code;
    DRW_Arc arc;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            if (applyExt)
                arc.applyExtrusion();
            iface->addArc(arc);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            arc.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processInsert(dxfReader *reader) {
    drw_dbg("dxfRW::processInsert");
    int code;
    DRW_Insert insert;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            iface->addInsert(insert);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            insert.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processLWPolyline(dxfReader *reader) {
    drw_dbg("dxfRW::processLWPolyline");
    int code;
    DRW_LWPolyline pl;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            if (applyExt)
                pl.applyExtrusion();
            iface->addLWPolyline(pl);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            pl.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processPolyline(dxfReader *reader) {
    drw_dbg("dxfRW::processPolyline");
    int code;
    DRW_Polyline pl;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        if(0 == code) {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            if (nextentity != "VERTEX") {
                iface->addPolyline(pl);
                return true;  //found new entity or ENDSEC, terminate
            }
            processVertex(&pl, reader);
        }
        pl.parseCode(code, reader); //parseCode just initialize the members of pl
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processVertex(DRW_Polyline *pl, dxfReader *reader) {
    drw_dbg("dxfRW::processVertex");
    int code;
    auto v = std::make_shared<DRW_Vertex>();
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        if(0 == code)  {
            pl->appendVertex(v);
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            if (nextentity == "SEQEND") {
                return true;  //found SEQEND no more vertex, terminate
            }
            if (nextentity == "VERTEX"){
                v = std::make_shared<DRW_Vertex>(); //another vertex
            }
        }
        v->parseCode(code, reader); //the members of v are reinitialized here
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processText(dxfReader *reader) {
    drw_dbg("dxfRW::processText");
    int code;
    DRW_Text txt;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            iface->addText(txt);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            txt.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processMText(dxfReader *reader) {
    drw_dbg("dxfRW::processMText");
    int code;
    DRW_MText txt;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            txt.updateAngle();
            iface->addMText(txt);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            txt.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processHatch(dxfReader *reader) {
    drw_dbg("dxfRW::processHatch");
    int code;
    DRW_Hatch hatch;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            iface->addHatch(&hatch);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            hatch.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}


bool dxfRW::processSpline(dxfReader *reader) {
    drw_dbg("dxfRW::processSpline");
    int code;
    DRW_Spline sp;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            iface->addSpline(&sp);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            sp.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}


bool dxfRW::processImage(dxfReader *reader) {
    drw_dbg("dxfRW::processImage");
    int code;
    DRW_Image img;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            iface->addImage(&img);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            img.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}


bool dxfRW::processDimension(dxfReader *reader) {
    drw_dbg("dxfRW::processDimension");
    int code;
    DRW_Dimension dim;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            int type = dim.type & 0x0F;
            switch (type) {
            case 0: {
                DRW_DimLinear d(dim);
                iface->addDimLinear(&d);
                break; }
            case 1: {
                DRW_DimAligned d(dim);
                iface->addDimAlign(&d);
                break; }
            case 2:  {
                DRW_DimAngular d(dim);
                iface->addDimAngular(&d);
                break;}
            case 3: {
                DRW_DimDiametric d(dim);
                iface->addDimDiametric(&d);
                break; }
            case 4: {
                DRW_DimRadial d(dim);
                iface->addDimRadial(&d);
                break; }
            case 5: {
                DRW_DimAngular3p d(dim);
                iface->addDimAngular3P(&d);
                break; }
            case 6: {
                DRW_DimOrdinate d(dim);
                iface->addDimOrdinate(&d);
                break; }
            }
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            dim.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processLeader(dxfReader *reader) {
    drw_dbg("dxfRW::processLeader");
    int code;
    DRW_Leader leader;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            iface->addLeader(&leader);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            leader.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}


/********* Objects Section *********/

bool dxfRW::processObjects(dxfReader *reader) {
    drw_dbg("dxfRW::processObjects\n");
    int code;
    if (!reader->readRec(&code)
            || 0 != code){
        return setError(DRW::BAD_READ_OBJECTS); //first record in objects must be 0
    }

    bool processed {false};
    nextentity = reader->getString();
    do {
        if ("ENDSEC" == nextentity) {
            return true;  //found ENDSEC terminate
        }

        if ("IMAGEDEF" == nextentity) {
            processed = processImageDef(reader);
        }
        else if ("PLOTSETTINGS" == nextentity) {
            processed = processPlotSettings(reader);
        }
        else {
            if (!reader->readRec(&code)) {
                return setError(DRW::BAD_READ_OBJECTS); //end of file without ENDSEC
            }

            if (code == 0) {
                nextentity = reader->getString();
            }
            processed = true;
        }
    }
    while (processed);

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processImageDef(dxfReader *reader) {
    drw_dbg("dxfRW::processImageDef");
    int code;
    DRW_ImageDef img;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            iface->linkImage(&img);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            img.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processPlotSettings(dxfReader* reader) {
    drw_dbg("dxfRW::processPlotSettings");
    int code;
    DRW_PlotSettings ps;
    while (reader->readRec(&code)) {
        drw_dbg(code); drw_dbg("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            drw_dbg(nextentity); drw_dbg("\n");
            iface->addPlotSettings(&ps);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            ps.parseCode(code, reader);
            break;
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::writePlotSettings(DRW_PlotSettings *ent, dxfWriter* writer) {
    writer->writeString(0, "PLOTSETTINGS");
    writer->writeString(5, DRW::toHexStr(++entCount));
    writer->writeString(100, "AcDbPlotSettings");
    writer->writeUtf8String(6, ent->plotViewName);
    writer->writeDouble(40, ent->marginLeft);
    writer->writeDouble(41, ent->marginBottom);
    writer->writeDouble(42, ent->marginRight);
    writer->writeDouble(43, ent->marginTop);
    return true;
}

DRW::Version dxfRW::getVersion() const {
    return version;
}

DRW::error dxfRW::getError() const
{
    return error;
}

bool dxfRW::setError(DRW::error lastError)
{
    error = lastError;
    return (DRW::BAD_NONE == error);
}
