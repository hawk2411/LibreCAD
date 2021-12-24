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

#ifndef DWGBUFFER_H
#define DWGBUFFER_H

#include <fstream>
#include <sstream>
#include <memory>
#include "../drw_base.h"

class DRW_Coord;
class DRW_TextCodec;

class dwgBasicStream{
protected:
    dwgBasicStream() = default;
public:
    virtual ~dwgBasicStream() = default;
    virtual bool read(uint8_t* s, uint64_t n) = 0;
    virtual uint64_t size() const = 0;
    virtual uint64_t getPos() const = 0;
    virtual bool setPos(uint64_t p) = 0;
    virtual bool good() const = 0;
    virtual dwgBasicStream* clone() const = 0;
};

class dwgFileStream: public dwgBasicStream{
public:
    explicit dwgFileStream(std::ifstream *s)
        :stream{s}
    {
        stream->seekg (0, std::ios::end);
        sz = stream->tellg();
        stream->seekg(0, std::ios_base::beg);
    }
    bool read(uint8_t* s, uint64_t n) override;
    uint64_t size() const override{return sz;}
    uint64_t getPos() const override{return stream->tellg();}
    bool setPos(uint64_t p) override;
    bool good() const override{return stream->good();}
    dwgBasicStream* clone() const override{return new dwgFileStream(stream);}
private:
    std::ifstream *stream{nullptr};
    uint64_t sz{0};
};

class dwgCharStream: public dwgBasicStream{
public:
    dwgCharStream(uint8_t *buf, uint64_t s)
        :stream{buf}
        ,sz{s}
    {}
    bool read(uint8_t* s, uint64_t n) override;
    uint64_t size() const override {return sz;}
    uint64_t getPos() const override {return pos;}
    bool setPos(uint64_t p) override;
    bool good() const override {return isOk;}
    dwgBasicStream* clone() const override {return new dwgCharStream(stream, sz);}
private:
    uint8_t *stream{nullptr};
    uint64_t sz{0};
    uint64_t pos{0};
    bool isOk{true};
};

class dwgBuffer {
public:
    dwgBuffer(std::ifstream *stream, DRW_TextCodec *decoder = nullptr);
    dwgBuffer(uint8_t *buf, uint64_t size, DRW_TextCodec *decoder= nullptr);
    dwgBuffer( const dwgBuffer& org );
    dwgBuffer& operator=( const dwgBuffer& org );
    virtual ~dwgBuffer() = default;
    uint64_t size() const {return filestr->size();}
    bool setPosition(uint64_t pos);
    uint64_t getPosition() const;
    void resetPosition(){setPosition(0); setBitPos(0);}
    void setBitPos(uint8_t pos);
    uint8_t getBitPos() const {return bitPos;}
    bool moveBitPos(int32_t size);

    uint8_t getBit();  //B
    bool getBoolBit();  //B as bool
    uint8_t get2Bits(); //BB
    uint8_t get3Bits(); //3B
    uint16_t getBitShort(); //BS
    int16_t getSBitShort(); //BS
    int32_t getBitLong(); //BL
    uint64_t getBitLongLong();  //BLL (R24)
    double getBitDouble(); //BD
    //2BD => call BD 2 times
    DRW_Coord get3BitDouble(); //3BD
    uint8_t getRawChar8();  //RC
    uint16_t getRawShort16();  //RS
    double getRawDouble(); //RD
    uint32_t getRawLong32();   //RL
    uint64_t getRawLong64();   //RLL
    DRW_Coord get2RawDouble(); //2RD
    //3RD => call RD 3 times
    uint32_t getUModularChar(); //UMC, unsigned for offsets in 1015
    int32_t getModularChar(); //MC
    int32_t getModularShort(); //MS
    dwgHandle getHandle(); //H
    dwgHandle getOffsetHandle(uint32_t href); //H converted to hard
    std::string getVariableText(DRW::Version v, bool nullTerm = true); //TV => call TU for 2007+ or T for previous versions
    std::string getCP8Text(); //T 8 bit text converted from codepage to utf8
    std::string getUCSText(bool nullTerm = true); //TU unicode 16 bit (UCS) text converted to utf8
    std::string getUCSStr(uint16_t ts);

    uint16_t getObjType(DRW::Version v);  //OT

    //X, U, SN,

    DRW_Coord getExtrusion(bool b_R2000_style); //BE
    double getDefaultDouble(double d); //DD
    double getThickness(bool b_R2000_style);//BT
    //3DD
    uint32_t getCmColor(DRW::Version v); //CMC
    uint32_t getEnColor(DRW::Version v); //ENC
    //TC

    uint16_t getBERawShort16();  //RS big-endian order

    bool isGood() const {return filestr->good();}
    bool getBytes(uint8_t *buf, uint64_t size);
    int numRemainingBytes() const {return (maxSize- filestr->getPos());}

    uint16_t crc8(uint16_t dx,int32_t start,int32_t end);
    uint32_t crc32(uint32_t seed,int32_t start,int32_t end);

//    uint8_t getCurrByte(){return currByte;}
    DRW_TextCodec *decoder{nullptr};

private:
    std::unique_ptr<dwgBasicStream> filestr;
    uint64_t maxSize{0};
    uint8_t currByte{0};
    uint8_t bitPos{0};

    std::string get8bitStr();
    std::string get16bitStr(uint16_t textSize, bool nullTerm = true);
};

#endif // DWGBUFFER_H
