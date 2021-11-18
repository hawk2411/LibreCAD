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

#include <string>
#include "dxfwriter.h"

//RLZ TODO change std::endl to x0D x0A (13 10)
/*bool dxfWriter::readRec(int *codeData, bool skip) {
//    std::string text;
    int code;

#ifdef DRW_DBG
    count = count+2; //DBG
#endif

    if (!readCode(&code))
        return false;
    *codeData = code;

    if (code < 10)
        readString();
    else if (code < 60)
        readDouble();
    else if (code < 80)
        readInt();
    else if (code > 89 && code < 100) //TODO this is an int 32b
        readInt32();
    else if (code == 100 || code == 102 || code == 105)
        readString();
    else if (code > 109 && code < 150) //skip not used at the v2012
        readDouble();
    else if (code > 159 && code < 170) //skip not used at the v2012
        readInt64();
    else if (code < 180)
        readInt();
    else if (code > 209 && code < 240) //skip not used at the v2012
        readDouble();
    else if (code > 269 && code < 290) //skip not used at the v2012
        readInt();
    else if (code < 300) //TODO this is a boolean indicator, int in Binary?
        readBool();
    else if (code < 370)
        readString();
    else if (code < 390)
        readInt();
    else if (code < 400)
        readString();
    else if (code < 410)
        readInt();
    else if (code < 420)
        readString();
    else if (code < 430) //TODO this is an int 32b
        readInt32();
    else if (code < 440)
        readString();
    else if (code < 450) //TODO this is an int 32b
        readInt32();
    else if (code < 460) //TODO this is long??
        readInt();
    else if (code < 470) //TODO this is a floating point double precision??
        readDouble();
    else if (code < 481)
        readString();
    else if (code > 998 && code < 1009) //skip not used at the v2012
        readString();
    else if (code < 1060) //TODO this is a floating point double precision??
        readDouble();
    else if (code < 1071)
        readInt();
    else if (code == 1071) //TODO this is an int 32b
        readInt32();
    else if (skip)
        //skip safely this dxf entry ( ok for ascii dxf)
        readString();
    else
        //break in binary files because the conduct is unpredictable
        return false;

    return (filestr->good());
}*/

bool dxfWriter::writeUtf8String(int code, const std::string &text) {
    std::string t = encoder.fromUtf8(text);
    return writeString(code, t);
}

bool dxfWriter::writeUtf8Caps(int code, const std::string &text) {
    std::string strname = DRW::toUpper(text);

    std::string t = encoder.fromUtf8(strname);
    return writeString(code, t);
}

bool dxfWriterBinary::writeString(int code, const std::string &text) {
    if(! this->writeBytes(2, code) ) {
        return false;
    }
    *filestr << text << '\0';
    return (filestr->good());
}

bool dxfWriterBinary::writeInt16(int code, int data) {
    if( writeBytes(2, code) && writeBytes(2, data) ) {
        return true;
    }
    return false;
}

bool dxfWriterBinary::writeInt32(int code, int data) {
    if( writeBytes(2, code) && writeBytes(4, data) ) {
        return true;
    }
    return false;
}

bool dxfWriterBinary::writeInt64(int code, unsigned long long int data) {
    if( writeBytes(2, code) && writeBytes(8, data) ) {
        return true;
    }
    return false;
}

bool dxfWriterBinary::writeDouble(int code, double data) {
    if(!writeBytes(2, code)) {
        return false;
    }

    char buffer[8];

    unsigned char *val;
    val = (unsigned char *) &data;
    for (int i=0; i<8; i++) {
        buffer[i] =val[i];
    }
    filestr->write(buffer, 8);
    return (filestr->good());
}

//saved as int or add a bool member??
bool dxfWriterBinary::writeBool(int code, bool data) {
    if( writeBytes(2, code) && writeBytes(1, data) ) {
        return true;
    }
    return false;
}

dxfWriterAscii::dxfWriterAscii(std::ofstream *stream):dxfWriter(stream){
    filestr->precision(16);
}

bool dxfWriterAscii::writeString(int code, const std::string &text) {
//    *filestr << code << std::endl << text << std::endl ;
    filestr->width(3);
    *filestr << std::right << code << std::endl;
    filestr->width(0);
    *filestr << std::left << text << std::endl;
    /*    std::getline(*filestr, strData, '\0');
    DBG(strData); DBG("\n");*/
    return (filestr->good());
}

bool dxfWriterAscii::writeInt16(int code, int data) {
//    *filestr << std::right << code << std::endl << data << std::endl;
    filestr->width(3);
    *filestr << std::right << code << std::endl;
    filestr->width(5);
    *filestr << data << std::endl;
    return (filestr->good());
}

bool dxfWriterAscii::writeInt32(int code, int data) {
    return writeInt16(code, data);
}

bool dxfWriterAscii::writeInt64(int code, unsigned long long int data) {
//    *filestr << code << std::endl << data << std::endl;
    filestr->width(3);
    *filestr << std::right << code << std::endl;
    filestr->width(5);
    *filestr << data << std::endl;
    return (filestr->good());
}

bool dxfWriterAscii::writeDouble(int code, double data) {
//    std::streamsize prec = filestr->precision();
//    filestr->precision(12);
//    *filestr << code << std::endl << data << std::endl;
    filestr->width(3);
    *filestr << std::right << code << std::endl;
    *filestr << data << std::endl;
//    filestr->precision(prec);
    return (filestr->good());
}

//saved as int or add a bool member??
bool dxfWriterAscii::writeBool(int code, bool data) {
    *filestr << code << std::endl << data << std::endl;
    return (filestr->good());
}



