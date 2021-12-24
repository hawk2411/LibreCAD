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

#include "drw_dbg.h"
#include "dwgutil.h"
#include "rscodec.h"
#include "../libdwgr.h"


/**
 * @brief dwgRSCodec::decode239I
 * @param in : input data (at least 255*blk bytes)
 * @param out : output data (at least 239*blk bytes)
 * @param blk number of codewords ( 1 cw == 255 bytes)
 */
void dwgRSCodec::decode239I(unsigned char *in, unsigned char *out, uint32_t blk){
    int k=0;
    unsigned char data[255];
    RScodec rsc(0x96, 8, 8); //(255, 239)
    for (uint32_t i=0; i<blk; i++){
        k = i;
        for (int j=0; j<255; j++) {
            data[j] = in[k];
            k +=blk;
        }
        int r = rsc.decode(data);
        if (r<0)
            drw_dbg("\nWARNING: dwgRSCodec::decode239I, can't correct all errors");
        k = i*239;
        for (int j=0; j<239; j++) {
            out[k++] = data[j];
        }
    }
}

/**
 * @brief dwgRSCodec::decode251I
 * @param in : input data (at least 255*blk bytes)
 * @param out : output data (at least 251*blk bytes)
 * @param blk number of codewords ( 1 cw == 255 bytes)
 */
void dwgRSCodec::decode251I(unsigned char *in, unsigned char *out, uint32_t blk){
    int k=0;
    unsigned char data[255];
    RScodec rsc(0xB8, 8, 2); //(255, 251)
    for (uint32_t i=0; i<blk; i++){
        k = i;
        for (int j=0; j<255; j++) {
            data[j] = in[k];
            k +=blk;
        }
        int r = rsc.decode(data);
        if (r<0)
            drw_dbg("\nWARNING: dwgRSCodec::decode251I, can't correct all errors");
        k = i*251;
        for (int j=0; j<251; j++) {
            out[k++] = data[j];
        }
    }
}

uint8_t *dwgCompressor::compressedBuffer {nullptr};
uint32_t dwgCompressor::compressedSize {0};
uint32_t dwgCompressor::compressedPos {0};
bool    dwgCompressor::compressedGood {true};
uint8_t *dwgCompressor::decompBuffer {nullptr};
uint32_t dwgCompressor::decompSize {0};
uint32_t dwgCompressor::decompPos {0};
bool    dwgCompressor::decompGood {true};

uint32_t dwgCompressor::twoByteOffset(uint32_t *ll){
    uint32_t cont = 0;
    uint8_t fb = compressedByte();
    cont = (fb >> 2) | (compressedByte() << 6);
    *ll = (fb & 0x03);
    return cont;
}

uint32_t dwgCompressor::longCompressionOffset(){
    uint32_t cont = 0;
    uint8_t ll = compressedByte();
    while (ll == 0x00 && compressedGood) {
        cont += 0xFF;
        ll = compressedByte();
    }
    cont += ll;
    return cont;
}

uint32_t dwgCompressor::long20CompressionOffset(){
//    uint32_t cont = 0;
    uint32_t cont = 0x0F;
    uint8_t ll = compressedByte();
    while (ll == 0x00 && compressedGood){
//        cont += 0xFF;
        ll = compressedByte();
    }
    cont += ll;
    return cont;
}

uint32_t dwgCompressor::litLength18(){
    uint32_t cont = 0;
    uint8_t ll = compressedByte();
    //no literal length, this byte is next opCode
    if (ll > 0x0F) {
        --compressedPos;
        return 0;
    }

    if (ll == 0x00) {
        cont = 0x0F;
        ll = compressedByte();
        while (ll == 0x00 && compressedGood) {//repeat until ll != 0x00
            cont += 0xFF;
            ll = compressedByte();
        }
    }

    return cont + ll + 3;
}

bool dwgCompressor::decompress18(uint8_t *cbuf, uint8_t *dbuf, uint64_t csize, uint64_t dsize){
    compressedBuffer = cbuf;
    decompBuffer = dbuf;
    compressedSize = csize;
    decompSize = dsize;
    compressedPos = 0;
    decompPos = 0;

    drw_dbg("dwgCompressor::decompress, last 2 bytes: ");
    drw_dbgh(compressedBuffer[compressedSize - 2]);drw_dbg(" ");drw_dbgh(compressedBuffer[compressedSize - 1]);drw_dbg("\n");

    uint32_t compBytes {0};
    uint32_t compOffset {0};
    uint32_t litCount {litLength18()};

    //copy first literal length
    for (uint32_t i = 0; i < litCount && buffersGood(); ++i) {
        decompSet( compressedByte());
    }

    while (buffersGood()) {
        uint8_t oc = compressedByte(); //next opcode
        if (oc == 0x10){
            compBytes = longCompressionOffset()+ 9;
            compOffset = twoByteOffset(&litCount) + 0x3FFF;
            if (litCount == 0)
                litCount= litLength18();
        } else if (oc > 0x11 && oc< 0x20){
            compBytes = (oc & 0x0F) + 2;
            compOffset = twoByteOffset(&litCount) + 0x3FFF;
            if (litCount == 0)
                litCount= litLength18();
        } else if (oc == 0x20){
            compBytes = longCompressionOffset() + 0x21;
            compOffset = twoByteOffset(&litCount);
            if (litCount == 0)
                litCount= litLength18();
        } else if (oc > 0x20 && oc< 0x40){
            compBytes = oc - 0x1E;
            compOffset = twoByteOffset(&litCount);
            if (litCount == 0)
                litCount= litLength18();
        } else if ( oc > 0x3F){
            compBytes = ((oc & 0xF0) >> 4) - 1;
            uint8_t ll2 = compressedByte();
            compOffset =  (ll2 << 2) | ((oc & 0x0C) >> 2);
            litCount = oc & 0x03;
            if (litCount < 1){
                litCount= litLength18();}
        } else if (oc == 0x11){
            drw_dbg("dwgCompressor::decompress, end of input stream, Cpos: ");
            drw_dbg(compressedPos);drw_dbg(", Dpos: ");drw_dbg(decompPos);drw_dbg("\n");
            return true; //end of input stream
        } else { //ll < 0x10
            drw_dbg("WARNING dwgCompressor::decompress, failed, illegal char: "); drw_dbgh(oc);
            drw_dbg(", Cpos: "); drw_dbg(compressedPos);
            drw_dbg(", Dpos: "); drw_dbg(decompPos); drw_dbg("\n");
            return false; //fails, not valid
        }

        //copy "compressed data", if size allows
        if (decompSize < decompPos + compBytes) {
            drw_dbg("WARNING dwgCompressor::decompress18, bad compBytes size, Cpos: ");
            drw_dbg(compressedPos);drw_dbg(", Dpos: ");drw_dbg(decompPos);drw_dbg(", need ");drw_dbg(compBytes);drw_dbg(", available ");drw_dbg(decompSize - decompPos);drw_dbg("\n");
            // only copy what we can fit
            compBytes = decompSize - decompPos;
        }
        uint32_t j {decompPos - compOffset - 1};
        for (uint32_t i = 0; i < compBytes && buffersGood(); i++) {
            decompSet( decompByte( j++));
        }

        //copy "uncompressed data", if size allows
        if (decompSize < decompPos + litCount) {
            drw_dbg("WARNING dwgCompressor::decompress18, bad litCount size, Cpos: ");
            drw_dbg(compressedPos);drw_dbg(", Dpos: ");drw_dbg(decompPos);drw_dbg(", need ");drw_dbg(litCount);drw_dbg(", available ");drw_dbg(decompSize - decompPos);drw_dbg("\n");
            // only copy what we can fit
            litCount = decompSize - decompPos;
        }
        for (uint32_t i=0; i < litCount && buffersGood(); i++) {
            decompSet( compressedByte());
        }
    }

    drw_dbg("WARNING dwgCompressor::decompress, bad out, Cpos: ");drw_dbg(compressedPos);drw_dbg(", Dpos: ");drw_dbg(decompPos);drw_dbg("\n");
    return false;
}

uint8_t dwgCompressor::compressedByte(void)
{
    uint8_t result {0};

    compressedGood = (compressedPos < compressedSize);
    if (compressedGood) {
        result = compressedBuffer[compressedPos];
        ++compressedPos;
    }

    return result;
}

uint8_t dwgCompressor::compressedByte(const uint32_t index)
{
    if (index < compressedSize) {
        return compressedBuffer[index];
    }

    return 0;
}

uint32_t dwgCompressor::compressedHiByte(void)
{
    return static_cast<uint32_t>(compressedByte()) << 8;
}

bool dwgCompressor::compressedInc(const int32_t inc /*= 1*/)
{
    compressedPos += inc;
    compressedGood = (compressedPos <= compressedSize);

    return compressedGood;
}

uint8_t dwgCompressor::decompByte(const uint32_t index)
{
    if (index < decompSize) {
        return decompBuffer[index];
    }

    return 0;
}

void dwgCompressor::decompSet(const uint8_t value)
{
    decompGood = (decompPos < decompSize);
    if (decompGood) {
        decompBuffer[decompPos] = value;
        ++decompPos;
    }
}

bool dwgCompressor::buffersGood(void)
{
    return compressedGood && decompGood;
}

void dwgCompressor::decrypt18Hdr(uint8_t *buf, uint64_t size, uint64_t offset){
    uint8_t max = size / 4;
    uint32_t secMask = 0x4164536b ^ offset;
    uint32_t* pHdr = reinterpret_cast<uint32_t*>(buf);
    for (uint8_t j = 0; j < max; j++)
        *pHdr++ ^= secMask;
}

/*void dwgCompressor::decrypt18Data(uint8_t *buf, uint32_t size, uint32_t offset){
    uint8_t max = size / 4;
    uint32_t secMask = 0x4164536b ^ offset;
    uint32_t* pHdr = (uint32_t*)buf;
    for (uint8_t j = 0; j < max; j++)
        *pHdr++ ^= secMask;
}*/

uint32_t dwgCompressor::litLength21(uint8_t opCode)
{
    uint32_t length = 8u + opCode;
    if (0x17 == length) {
        uint32_t n = compressedByte();
        length += n;
        if (0xffu == n) {
            do {
                n = compressedByte();
                n |= compressedHiByte();
                length += n;
            }
            while (0xffffu == n);
        }
    }

    return length;
}

bool dwgCompressor::decompress21(uint8_t *cbuf, uint8_t *dbuf, uint64_t csize, uint64_t dsize){
    compressedBuffer = cbuf;
    decompBuffer = dbuf;
    compressedSize = csize;
    decompSize = dsize;
    compressedPos = 0;
    decompPos = 0;
    compressedGood = true;
    decompGood = true;

    uint32_t length {0};
    uint32_t sourceOffset {0};
    uint8_t opCode {compressedByte()};
    if ((opCode >> 4) == 2){
        compressedInc( 2);
        length = compressedByte() & 0x07;
    }

    while (buffersGood()) {
        if (length == 0) {
            length = litLength21(opCode);
        }
        copyCompBytes21( length);

        if (decompPos >= decompSize) {
            break; //check if last chunk are compressed & terminate
        }

        length = 0;
        opCode = compressedByte();
        readInstructions21( opCode,  sourceOffset,  length);
        while (true) {
            //prevent crash with corrupted data
            if (sourceOffset > decompPos) {
                drw_dbg("\nWARNING dwgCompressor::decompress21 => sourceOffset> dstIndex.\n");
                drw_dbg("csize = "); drw_dbg(compressedSize); drw_dbg("  srcIndex = "); drw_dbg(compressedPos);
                drw_dbg("\ndsize = "); drw_dbg(decompSize); drw_dbg("  dstIndex = "); drw_dbg(decompPos);
                sourceOffset = decompPos;
            }
            //prevent crash with corrupted data
            if (length > decompSize - decompPos){
                drw_dbg("\nWARNING dwgCompressor::decompress21 => length > dsize - dstIndex.\n");
                drw_dbg("csize = "); drw_dbg(compressedSize); drw_dbg("  srcIndex = "); drw_dbg(compressedPos);
                drw_dbg("\ndsize = "); drw_dbg(decompSize); drw_dbg("  dstIndex = "); drw_dbg(decompPos);
                length = decompSize - decompPos;
                compressedPos = compressedSize; //force exit
                compressedGood = false;
            }
            sourceOffset = decompPos - sourceOffset;
            for (uint32_t i=0; i< length; i++)
                decompSet( decompByte( sourceOffset + i));

            length = opCode & 7;
            if ((length != 0) || (compressedPos >= compressedSize)) {
                break;
            }
            opCode = compressedByte();
            if ((opCode >> 4) == 0) {
                break;
            }
            if ((opCode >> 4) == 15) {
                opCode &= 15;
            }
            readInstructions21( opCode, sourceOffset, length);
        }

        if (compressedPos >= compressedSize) {
            break;
        }
    }
    drw_dbg("\ncsize = "); drw_dbg(compressedSize); drw_dbg("  srcIndex = "); drw_dbg(compressedPos);
    drw_dbg("\ndsize = "); drw_dbg(decompSize); drw_dbg("  dstIndex = "); drw_dbg(decompPos);drw_dbg("\n");

    return buffersGood();
}

void dwgCompressor::readInstructions21(uint8_t &opCode, uint32_t &sourceOffset, uint32_t &length){

    switch (opCode >> 4) {
    case 0:
        length = (opCode & 0x0f) + 0x13;
        sourceOffset = compressedByte();
        opCode = compressedByte();
        length = ((opCode >> 3) & 0x10) + length;
        sourceOffset = ((opCode & 0x78) << 5) + 1 + sourceOffset;
        break;
    case 1:
        length = (opCode & 0xf) + 3;
        sourceOffset = compressedByte();
        opCode = compressedByte();
        sourceOffset = ((opCode & 0xf8) << 5) + 1 + sourceOffset;
        break;
    case 2:
        sourceOffset = compressedByte();
        sourceOffset = (compressedHiByte() & 0xff00) | sourceOffset;
        length = opCode & 7;
        if ((opCode & 8) == 0) {
            opCode = compressedByte();
            length = (opCode & 0xf8) + length;
        } else {
            ++sourceOffset;
            length = (static_cast<uint32_t>(compressedByte()) << 3) + length;
            opCode = compressedByte();
            length = (((opCode & 0xf8) << 8) + length) + 0x100;
        }
        break;
    default:
        length = opCode >> 4;
        sourceOffset = opCode & 15;
        opCode = compressedByte();
        sourceOffset = (((opCode & 0xf8) << 1) + sourceOffset) + 1;
        break;
    }
}

const uint8_t dwgCompressor::CopyOrder21_01[] = {0};
const uint8_t dwgCompressor::CopyOrder21_02[] = {1,0};
const uint8_t dwgCompressor::CopyOrder21_03[] = {2,1,0};
const uint8_t dwgCompressor::CopyOrder21_04[] = {0,1,2,3};
const uint8_t dwgCompressor::CopyOrder21_05[] = {4,0,1,2,3};
const uint8_t dwgCompressor::CopyOrder21_06[] = {5,1,2,3,4,0};
const uint8_t dwgCompressor::CopyOrder21_07[] = {6,5,1,2,3,4,0};
const uint8_t dwgCompressor::CopyOrder21_08[] = {0,1,2,3,4,5,6,7};
const uint8_t dwgCompressor::CopyOrder21_09[] = {8,0,1,2,3,4,5,6,7};
const uint8_t dwgCompressor::CopyOrder21_10[] = {9,1,2,3,4,5,6,7,8,0};
const uint8_t dwgCompressor::CopyOrder21_11[] = {10,9,1,2,3,4,5,6,7,8,0};
const uint8_t dwgCompressor::CopyOrder21_12[] = {8,9,10,11,0,1,2,3,4,5,6,7};
const uint8_t dwgCompressor::CopyOrder21_13[] = {12,8,9,10,11,0,1,2,3,4,5,6,7};
const uint8_t dwgCompressor::CopyOrder21_14[] = {13,9,10,11,12,1,2,3,4,5,6,7,8,0};
const uint8_t dwgCompressor::CopyOrder21_15[] = {14,13,9,10,11,12,1,2,3,4,5,6,7,8,0};
const uint8_t dwgCompressor::CopyOrder21_16[] = {8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const uint8_t dwgCompressor::CopyOrder21_17[] = {9,10,11,12,13,14,15,16,8,0,1,2,3,4,5,6,7};
const uint8_t dwgCompressor::CopyOrder21_18[] = {17,9,10,11,12,13,14,15,16,1,2,3,4,5,6,7,8,0};
const uint8_t dwgCompressor::CopyOrder21_19[] = {18,17,16,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const uint8_t dwgCompressor::CopyOrder21_20[] = {16,17,18,19,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const uint8_t dwgCompressor::CopyOrder21_21[] = {20,16,17,18,19,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const uint8_t dwgCompressor::CopyOrder21_22[] = {21,20,16,17,18,19,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const uint8_t dwgCompressor::CopyOrder21_23[] = {22,21,20,16,17,18,19,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const uint8_t dwgCompressor::CopyOrder21_24[] = {16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const uint8_t dwgCompressor::CopyOrder21_25[] = {17,18,19,20,21,22,23,24,16,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const uint8_t dwgCompressor::CopyOrder21_26[] = {25,17,18,19,20,21,22,23,24,16,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const uint8_t dwgCompressor::CopyOrder21_27[] = {26,25,17,18,19,20,21,22,23,24,16,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const uint8_t dwgCompressor::CopyOrder21_28[] = {24,25,26,27,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const uint8_t dwgCompressor::CopyOrder21_29[] = {28,24,25,26,27,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const uint8_t dwgCompressor::CopyOrder21_30[] = {29,28,24,25,26,27,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const uint8_t dwgCompressor::CopyOrder21_31[] = {30,26,27,28,29,18,19,20,21,22,23,24,25,10,11,12,13,14,15,16,17,2,3,4,5,6,7,8,9,1,0};
const uint8_t dwgCompressor::CopyOrder21_32[] = {24,25,26,27,28,29,30,31,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const uint8_t *dwgCompressor::CopyOrder21[dwgCompressor::Block21OrderArray] = {
        nullptr,
        CopyOrder21_01, CopyOrder21_02, CopyOrder21_03, CopyOrder21_04,
        CopyOrder21_05, CopyOrder21_06, CopyOrder21_07, CopyOrder21_08,
        CopyOrder21_09, CopyOrder21_10, CopyOrder21_11, CopyOrder21_12,
        CopyOrder21_13, CopyOrder21_14, CopyOrder21_15, CopyOrder21_16,
        CopyOrder21_17, CopyOrder21_18, CopyOrder21_19, CopyOrder21_20,
        CopyOrder21_21, CopyOrder21_22, CopyOrder21_23, CopyOrder21_24,
        CopyOrder21_25, CopyOrder21_26, CopyOrder21_27, CopyOrder21_28,
        CopyOrder21_29, CopyOrder21_30, CopyOrder21_31, CopyOrder21_32
};

void dwgCompressor::copyBlock21(const uint32_t length)
{
    if (MaxBlock21Length < length) {
        return;
    }

    const uint8_t *order {CopyOrder21[length]};
    if (nullptr == order) {
        return;
    }

    for (uint32_t index = 0; (length > index) && buffersGood(); ++index) {
        decompSet( compressedByte( compressedPos + order[index]));
    }
    compressedInc( length);
}

bool dwgCompressor::copyCompBytes21(uint32_t length)
{
    drw_dbg("\ncopyCompBytes21() "); drw_dbg(length); drw_dbg("\n");

    while (length >= MaxBlock21Length) {
        copyBlock21( MaxBlock21Length);
        length -= MaxBlock21Length;
    }

    copyBlock21( length);

    return buffersGood();
}


secEnum::DWGSection secEnum::getEnum(const std::string &nameSec){
    //TODO: complete it
    if (nameSec=="AcDb:Header"){
        return HEADER;
    } else if (nameSec=="AcDb:Classes"){
        return CLASSES;
    } else if (nameSec=="AcDb:SummaryInfo"){
        return SUMARYINFO;
    } else if (nameSec=="AcDb:Preview"){
        return PREVIEW;
    } else if (nameSec=="AcDb:VBAProject"){
        return VBAPROY;
    } else if (nameSec=="AcDb:AppInfo"){
        return APPINFO;
    } else if (nameSec=="AcDb:FileDepList"){
        return FILEDEP;
    } else if (nameSec=="AcDb:RevHistory"){
        return REVHISTORY;
    } else if (nameSec=="AcDb:Security"){
        return SECURITY;
    } else if (nameSec=="AcDb:AcDbObjects"){
        return OBJECTS;
    } else if (nameSec=="AcDb:ObjFreeSpace"){
        return OBJFREESPACE;
    } else if (nameSec=="AcDb:Template"){
        return TEMPLATE;
    } else if (nameSec=="AcDb:Handles"){
        return HANDLES;
    } else if (nameSec=="AcDb:AcDsPrototype_1b"){
        return PROTOTYPE;
    } else if (nameSec=="AcDb:AuxHeader"){
        return AUXHEADER;
    } else if (nameSec=="AcDb:Signature"){
        return SIGNATURE;
    } else if (nameSec=="AcDb:AppInfoHistory"){ //in ac1021
        return APPINFOHISTORY;
//    } else if (nameSec=="AcDb:Extended Entity Data"){
//        return EXTEDATA;
//    } else if (nameSec=="AcDb:PROXY ENTITY GRAPHICS"){
//        return PROXYGRAPHICS;
    }
    return UNKNOWNS;
}
