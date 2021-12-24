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

#ifndef DWGUTIL_H
#define DWGUTIL_H
#include <sstream>
#include "../drw_base.h"


namespace dwgRSCodec {
    void decode239I(uint8_t *in, uint8_t *out, uint32_t blk);
    void decode251I(uint8_t *in, uint8_t *out, uint32_t blk);
}

class dwgCompressor {
    enum R21Consts {
        MaxBlock21Length = 32,
        Block21OrderArray,
    };

public:
    dwgCompressor()=default;

    bool decompress18(uint8_t *cbuf, uint8_t *dbuf, uint64_t csize, uint64_t dsize);
    static void decrypt18Hdr(uint8_t *buf, uint64_t size, uint64_t offset);
//    static void decrypt18Data(uint8_t *buf, uint32_t size, uint32_t offset);
    static bool decompress21(uint8_t *cbuf, uint8_t *dbuf, uint64_t csize, uint64_t dsize);

private:
    uint32_t litLength18();
    static uint32_t litLength21(uint8_t opCode);
    static bool copyCompBytes21(uint32_t length);
    static void readInstructions21(uint8_t &opCode, uint32_t &sourceOffset, uint32_t &length);

    uint32_t longCompressionOffset();
    uint32_t long20CompressionOffset();
    uint32_t twoByteOffset(uint32_t *ll);

    static uint8_t compressedByte();
    static uint8_t compressedByte(uint32_t index);
    static uint32_t compressedHiByte();
    static bool compressedInc(int32_t inc = 1);
    static uint8_t decompByte(uint32_t index);
    static void decompSet(uint8_t value);
    static bool buffersGood();
    static void copyBlock21(uint32_t length);

    static uint8_t *compressedBuffer;
    static uint32_t compressedSize;
    static uint32_t compressedPos;
    static bool    compressedGood;
    static uint8_t *decompBuffer;
    static uint32_t decompSize;
    static uint32_t decompPos;
    static bool    decompGood;

    static const uint8_t CopyOrder21_01[];
    static const uint8_t CopyOrder21_02[];
    static const uint8_t CopyOrder21_03[];
    static const uint8_t CopyOrder21_04[];
    static const uint8_t CopyOrder21_05[];
    static const uint8_t CopyOrder21_06[];
    static const uint8_t CopyOrder21_07[];
    static const uint8_t CopyOrder21_08[];
    static const uint8_t CopyOrder21_09[];
    static const uint8_t CopyOrder21_10[];
    static const uint8_t CopyOrder21_11[];
    static const uint8_t CopyOrder21_12[];
    static const uint8_t CopyOrder21_13[];
    static const uint8_t CopyOrder21_14[];
    static const uint8_t CopyOrder21_15[];
    static const uint8_t CopyOrder21_16[];
    static const uint8_t CopyOrder21_17[];
    static const uint8_t CopyOrder21_18[];
    static const uint8_t CopyOrder21_19[];
    static const uint8_t CopyOrder21_20[];
    static const uint8_t CopyOrder21_21[];
    static const uint8_t CopyOrder21_22[];
    static const uint8_t CopyOrder21_23[];
    static const uint8_t CopyOrder21_24[];
    static const uint8_t CopyOrder21_25[];
    static const uint8_t CopyOrder21_26[];
    static const uint8_t CopyOrder21_27[];
    static const uint8_t CopyOrder21_28[];
    static const uint8_t CopyOrder21_29[];
    static const uint8_t CopyOrder21_30[];
    static const uint8_t CopyOrder21_31[];
    static const uint8_t CopyOrder21_32[];
    static const uint8_t *CopyOrder21[Block21OrderArray];
};

namespace secEnum {
    enum DWGSection {
        UNKNOWNS,      /*!< UNKNOWN section. */
        FILEHEADER,    /*!< File Header (in R3-R15*/
        HEADER,        /*!< AcDb:Header */
        CLASSES,       /*!< AcDb:Classes */
        SUMARYINFO,    /*!< AcDb:SummaryInfo */
        PREVIEW,       /*!< AcDb:Preview */
        VBAPROY,       /*!< AcDb:VBAProject */
        APPINFO,       /*!< AcDb:AppInfo */
        FILEDEP,       /*!< AcDb:FileDepList */
        REVHISTORY,    /*!< AcDb:RevHistory */
        SECURITY,      /*!< AcDb:Security */
        OBJECTS,       /*!< AcDb:AcDbObjects */
        OBJFREESPACE,  /*!< AcDb:ObjFreeSpace */
        TEMPLATE,      /*!< AcDb:Template */
        HANDLES,       /*!< AcDb:Handles */
        PROTOTYPE,     /*!< AcDb:AcDsPrototype_1b */
        AUXHEADER,     /*!< AcDb:AuxHeader, in (R13-R15) second file header */
        SIGNATURE,     /*!< AcDb:Signature */
        APPINFOHISTORY,     /*!< AcDb:AppInfoHistory (in ac1021 may be a renamed section?*/
        EXTEDATA,      /*!< Extended Entity Data */
        PROXYGRAPHICS /*!< PROXY ENTITY GRAPHICS */
    };

    DWGSection getEnum(const std::string &nameSec);
}

#endif // DWGUTIL_H
