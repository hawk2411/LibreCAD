/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/


#ifndef RS_FILTERDXF1_H
#define RS_FILTERDXF1_H

#include <QFile>

#include "rs_filterinterface.h"

/**
 * This format filter class can import and export old DXF files
 * from QCad 1.x.
 *
 * This is legacy code from QCad 1.x.
 *
 * @author Andrew Mustun
 */
class RS_FilterDXF1 : public RS_FilterInterface {
public:
    RS_FilterDXF1();

    bool canImport(const QString & /*fileName*/, RS2::FormatType t) const override {
        return (t == RS2::FormatDXF1);
    }

    bool canExport(const QString & /*fileName*/, RS2::FormatType /*t*/) const override {
        return false;
    }

    bool fileImport(RS_Graphic &g, const QString &file, RS2::FormatType /*type*/) override;

    bool fileExport(RS_Graphic & /*g*/, const QString & /*file*/,
                            RS2::FormatType /*type*/) override;

    bool readFromBuffer();

    void reset();

    void resetBufP();

    void delBuffer();

    void dos2unix();

    QString getBufLine();

    void separateBuf(char _c1 = 13,
                     char _c2 = 10,
                     char _c3 = 0,
                     char _c4 = 0);

    bool readFileInBuffer(std::size_t _bNum = 0);

    static void strDecodeDxfString(QString &str);

    static RS_FilterInterface *createFilter() { return new RS_FilterDXF1(); }

    static RS2::LineWidth numberToWidth(int num);

private:
    /** Pointer to the graphic we currently operate on. */
    RS_Graphic *_graphic;
    FILE *_fPointer;         // File pointer
    char *_fBuf;             // Filebuffer
    int _fBufP;            // Filebuffer-Pointer (walks through 'fBuf')
    unsigned _fSize;            // Filesize
    QString _name;
    QFile _file;
};

#endif
