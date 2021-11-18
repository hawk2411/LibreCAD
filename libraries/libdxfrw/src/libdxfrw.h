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

#ifndef LIBDXFRW_H
#define LIBDXFRW_H

#include <string>
#include <unordered_map>
#include <algorithm>
#include "drw_entities.h"
#include "drw_objects.h"
#include "drw_header.h"
#include "drw_interface.h"


class dxfReader;
class dxfWriter;

class dxfRW {
public:
    explicit dxfRW(const char* name);
    ~dxfRW();
    static void setDebug(DRW::DebugLevel lvl);
    /// reads the file specified in constructor
    /*!
     * An interface must be provided. It is used by the class to signal various
     * components being added.
     * @param interface_ the interface to use
     * @param ext should the extrusion be applied to convert in 2D?
     * @return true for success
     */
    bool read(DRW_Interface *interface_, bool ext);
    bool write(DRW_Interface *interface_, DRW::Version ver, bool bin);
    bool writeLineType(DRW_LType *ent, dxfWriter* writer);
    bool writeLayer(const DRW_Layer *ent, dxfWriter* writer);
    bool writeDimstyle(const DRW_Dimstyle *ent, dxfWriter* writer);
    bool writeTextstyle(const DRW_Textstyle *ent, dxfWriter* writer);
    bool writeVport(DRW_Vport *ent, dxfWriter* writer);
    bool writeAppId(DRW_AppId *ent, dxfWriter* writer);
    bool writePoint(DRW_Point *ent, dxfWriter* writer);
    bool writeLine(DRW_Line *ent, dxfWriter* writer);
    bool writeRay(DRW_Ray *ent, dxfWriter* writer);
    bool writeXline(DRW_Xline *ent, dxfWriter* writer);
    bool writeCircle(DRW_Circle *ent, dxfWriter* writer);
    bool writeArc(DRW_Arc *ent, dxfWriter* writer);
    bool writeEllipse(DRW_Ellipse *ent, dxfWriter* writer);
    bool writeTrace(DRW_Trace *ent, dxfWriter* writer);
    bool writeSolid(DRW_Solid *ent, dxfWriter* writer);
    bool write3dface(DRW_3Dface *ent, dxfWriter* writer);
    bool writeLWPolyline(DRW_LWPolyline *ent, dxfWriter* writer);
    bool writePolyline(DRW_Polyline *ent, dxfWriter* writer);
    bool writeSpline(DRW_Spline *ent, dxfWriter* writer);
    bool writeBlockRecord(std::string name, dxfWriter* writer);
    bool writeBlock(DRW_Block *ent, dxfWriter* writer);
    bool writeInsert(DRW_Insert *ent, dxfWriter* writer);
    bool writeMText(DRW_MText *ent, dxfWriter* writer);
    bool writeText(DRW_Text *ent, dxfWriter* writer);
    bool writeHatch(DRW_Hatch *ent, dxfWriter* writer);
    bool writeViewport(DRW_Viewport *ent, dxfWriter* writer);
    DRW_ImageDef *writeImage(DRW_Image *ent, const std::string& name, dxfWriter* writer);
    bool writeLeader(DRW_Leader *ent, dxfWriter* writer);
    bool writeDimension(DRW_Dimension *ent, dxfWriter* writer);
    void setEllipseParts(int parts){elParts = parts;} /*!< set parts number when convert ellipse to polyline */
    bool writePlotSettings(DRW_PlotSettings *ent, dxfWriter* writer);

    DRW::Version getVersion() const;
    DRW::error getError() const;

private:
    /// used by read() to parse the content of the file
    bool processDxf(dxfReader *reader);
    bool processHeader(dxfReader *reader);
    bool processTables(dxfReader *reader);
    bool processBlocks(dxfReader *reader);
    bool processBlock(dxfReader *reader);
    bool processEntities(bool isblock, dxfReader *reader);
    bool processObjects(dxfReader *reader);

    bool processLType(dxfReader *reader);
    bool processLayer(dxfReader *reader);
    bool processDimStyle(dxfReader *reader);
    bool processTextStyle(dxfReader *reader);
    bool processVports(dxfReader *reader);
    bool processAppId(dxfReader *reader);

    bool processPoint(dxfReader *reader);
    bool processLine(dxfReader *reader);
    bool processRay(dxfReader *reader);
    bool processXline(dxfReader *reader);
    bool processCircle(dxfReader *reader);
    bool processArc(dxfReader *reader);
    bool processEllipse(dxfReader *reader);
    bool processTrace(dxfReader *reader);
    bool processSolid(dxfReader *reader);
    bool processInsert(dxfReader *reader);
    bool processLWPolyline(dxfReader *reader);
    bool processPolyline(dxfReader *reader);
    bool processVertex(DRW_Polyline* pl, dxfReader *reader);
    bool processText(dxfReader *reader);
    bool processMText(dxfReader *reader);
    bool processHatch(dxfReader *reader);
    bool processSpline(dxfReader *reader);
    bool process3dface(dxfReader *reader);
    bool processViewport(dxfReader *reader);
    bool processImage(dxfReader *reader);
    bool processImageDef(dxfReader *reader);
    bool processDimension(dxfReader *reader);
    bool processLeader(dxfReader *reader);
    bool processPlotSettings(dxfReader *reader);

//    bool writeHeader();
    bool writeEntity(DRW_Entity *ent, dxfWriter* writer);
    bool writeTables(dxfWriter *writer);
    bool writeBlocks(dxfWriter *writer);
    bool writeObjects(dxfWriter *writer);
    bool writeExtData(const std::vector<DRW_Variant*> &ed, dxfWriter* writer);
    bool writeAppData(const std::list<std::list<DRW_Variant>> &appData, dxfWriter* writer);

    bool setError(DRW::error lastError);
private:
    DRW::Version version{DRW::UNKNOWNV};
    DRW::error error{DRW::BAD_NONE};
    std::string fileName;
    std::string codePage;
    //dxfReader *reader{nullptr};
    //dxfWriter *writer{nullptr};
    DRW_Interface *iface{nullptr};
    DRW_Header header;
//    int section;
    std::string nextentity;
    int entCount{0};
    bool wlayer0{false};
    bool dimstyleStd{false};
    bool applyExt{false};
    bool writingBlock{false};
    int elParts{128};  /*!< parts number when convert ellipse to polyline */
    std::unordered_map<std::string, int> blockMap;
    std::vector<DRW_ImageDef *> imageDef;  /*!< imageDef list */

    int currHandle{0};

};

#endif // LIBDXFRW_H
