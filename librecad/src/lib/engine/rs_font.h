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


#ifndef RS_FONT_H
#define RS_FONT_H

#include <iosfwd>
#include <QStringList>
#include <QMap>
#include "rs_blocklist.h"
#include "rs_fontchar.h"

/**
 * Class for representing a font. This is implemented as a RS_Graphic
 * with a name (the font name) and several blocks, one for each letter
 * in the font.
 *
 * @author Andrew Mustun
 */
class RS_Font {
public:
    explicit RS_Font(QString name, bool owner = true);

    /** @return the fileName of this font. */
    QString getFileName() const {
        return _fileName;
    }

    /** @return the fileLicense of this font. */
    QString getFileLicense() const {
        return _fileLicense;
    }

    /** @return the creation date of this font. */
    QString getFileCreate() const {
        return _fileCreate;
    }

    /** @return the encoding of this font. */
    QString getEncoding() const {
        return _encoding;
    }

    /** @return the alternative names of this font. */
    const QStringList &getNames() const {
        return _names;
    }

    /** @return the author(s) of this font. */
    const QStringList &getAuthors() const {
        return _authors;
    }

    /** @return Default letter spacing for this font */
    double getLetterSpacing() const {
        return _letterSpacing;
    }

    /** @return Default word spacing for this font */
    double getWordSpacing() const {
        return _wordSpacing;
    }

    /** @return Default line spacing factor for this font */
    double getLineSpacingFactor() const {
        return _lineSpacingFactor;
    }

    bool loadFont();

    void generateAllFonts();

    // Wrappers for block list (letters) functions
    RS_BlockList *getLetterList() {
        return &_letterList;
    }

    RS_Block *findLetter(const QString &name);

    friend std::ostream &operator<<(std::ostream &os, const RS_Font &l);

    friend class RS_FontList;

private:
    void readCXF(const QString& path);

    void readLFF(const QString& path);

    RS_Block *generateLffFont(const QString &ch);

private:
    //raw lff font file list, not processed into blocks yet
    QMap<QString, QStringList> _rawLffFontList;

    //! block list (letters)
    RS_BlockList _letterList;

    //! Font file name
    QString _fileName;

    //! Font file license
    QString _fileLicense;

    //! Font file license
    QString _fileCreate;

    //! Font encoding (see docu for QTextCodec)
    QString _encoding;

    //! Font names
    QStringList _names;

    //! Authors
    QStringList _authors;

    //! Is this font currently loaded into memory?
    bool _loaded;

    //! Default letter spacing for this font
    double _letterSpacing;

    //! Default word spacing for this font
    double _wordSpacing;

    //! Default line spacing factor for this font
    double _lineSpacingFactor;

    void handleChar(RS_FontChar *letter, QString &line);
};

#endif

