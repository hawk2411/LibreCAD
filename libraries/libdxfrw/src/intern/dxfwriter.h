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

#ifndef DXFWRITER_H
#define DXFWRITER_H
#include <fstream>
#include "drw_textcodec.h"

class dxfWriter {
public:
    explicit dxfWriter(std::ofstream *stream){filestr = stream; /*count =0;*/}
    virtual ~dxfWriter()= default;
    virtual bool writeString(int code, const std::string &text) = 0;
    bool writeUtf8String(int code, const std::string &text);
    bool writeUtf8Caps(int code, const std::string &text);
    std::string fromUtf8String(const std::string& t)const {return encoder.fromUtf8(t);}
    virtual bool writeInt16(int code, int data) = 0;
    virtual bool writeInt32(int code, int data) = 0;
    virtual bool writeInt64(int code, unsigned long long int data) = 0;
    virtual bool writeDouble(int code, double data) = 0;
    virtual bool writeBool(int code, bool data) = 0;
    void setVersion(const std::string &v, bool dxfFormat){encoder.setVersion(v, dxfFormat);}
    void setCodePage(const std::string &c){encoder.setCodePage(c, true);}
    std::string getCodePage(){return encoder.getCodePage();}
protected:
    std::ofstream *filestr;
private:
    DRW_TextCodec encoder;
};

class dxfWriterBinary : public dxfWriter {
public:
    explicit dxfWriterBinary(std::ofstream *stream):dxfWriter(stream){}
    ~dxfWriterBinary() override = default;
    bool writeString(int code, const std::string &text) override;
    bool writeInt16(int code, int data) override;
    bool writeInt32(int code, int data) override;
    bool writeInt64(int code, unsigned long long int data) override;
    bool writeDouble(int code, double data) override;
    bool writeBool(int code, bool data) override;
protected:
    template<typename T>
    bool writeBytes(std::streamsize count, T bytes) {
        static_assert(std::numeric_limits<T>::is_integer, "Should be an integer type.");

        std::unique_ptr<char> buffer = std::unique_ptr<char>(new char[count]);
        for(std::streamsize i = 0; i < count; i++) {
            buffer.get()[i] = (bytes >> (i*8)) & 0XFF;
        }
        filestr->write(buffer.get(), count);
        return (filestr->good());
    }
};

class dxfWriterAscii : public dxfWriter {
public:
    explicit dxfWriterAscii(std::ofstream *stream);
    ~dxfWriterAscii() override= default;
    bool writeString(int code, const std::string &text) override;
    bool writeInt16(int code, int data) override;
    bool writeInt32(int code, int data) override;
    bool writeInt64(int code, unsigned long long int data) override;
    bool writeDouble(int code, double data) override;
    bool writeBool(int code, bool data) override;
};

#endif // DXFWRITER_H
