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


#include "rs_debug.h"

#include <iostream>
#include <cstdio>
#include <cstdarg>
#include <QString>

#include <QDateTime>
#include <QDebug>

RS_Debug *RS_Debug::_uniqueInstance = nullptr;

void debugHeader(char const *file, char const *func, int line) {
    std::cout << file << " : " << func << " : line " << line << std::endl;
}

/**
 *  Gets the one and only RS_Debug instance
 *  (creates a new one on first call only)
 *
 *  @return Pointer to the single instance of this
 * singleton class
 */
RS_Debug *RS_Debug::instance() {
    if (!_uniqueInstance) {
        _uniqueInstance = new RS_Debug;
        _uniqueInstance->_stream = stderr;
    }
    return _uniqueInstance;
}


/**
 * Deletes the one and only RS_Debug instance.
 */
void RS_Debug::deleteInstance() {

    if (_uniqueInstance) {
        fclose(_uniqueInstance->_stream);
        delete _uniqueInstance;
        _uniqueInstance = nullptr;
    }
}

/**
 * Constructor setting the default debug level.
 */
RS_Debug::RS_Debug() : _debugLevel{D_DEBUGGING}, _stream{nullptr} {}

/**
 * Sets the debugging level.
 */
void RS_Debug::setLevel(RS_DebugLevel level) {
    if (_debugLevel == level) return;
    _debugLevel = level;
    print(D_NOTHING, "RS_DEBUG::setLevel(%d)", level);
    print(D_CRITICAL, "RS_DEBUG: Critical");
    print(D_ERROR, "RS_DEBUG: Errors");
    print(D_WARNING, "RS_DEBUG: Warnings");
    print(D_NOTICE, "RS_DEBUG: Notice");
    print(D_INFORMATIONAL, "RS_DEBUG: Informational");
    print(D_DEBUGGING, "RS_DEBUG: Debugging");
}


/**
 * Gets the current debugging level.
 */
RS_Debug::RS_DebugLevel RS_Debug::getLevel() {
    return _debugLevel;
}


/**
 * Prints the given message to stdout.
 */
void RS_Debug::print(const char *format ...) {
    if (_debugLevel == D_DEBUGGING) {
        va_list ap;
        va_start(ap, format);
        vfprintf(_stream, format, ap);
        fprintf(_stream, "\n");
        va_end(ap);
        fflush(_stream);
    }

}

/**
 * Prints the given message to stdout if the current debug level
 * is lower then the given level
 *
 * @param level Debug level.
 */
void RS_Debug::print(RS_DebugLevel level, const char *format ...) {

    if (_debugLevel >= level) {
        va_list ap;
        va_start(ap, format);
        vfprintf(_stream, format, ap);
        fprintf(_stream, "\n");
        va_end(ap);
        fflush(_stream);
    }

}


/**
 * Prints a time stamp in the format yyyyMMdd_hhmmss.
 */
void RS_Debug::timestamp() {
    QDateTime now = QDateTime::currentDateTime();
    QString nowStr;

    nowStr = now.toString("yyyyMMdd_hh:mm:ss:zzz ");
    fprintf(_stream, "%s", nowStr.toLatin1().data());
    fprintf(_stream, "\n");
    fflush(_stream);
}


/**
 * Prints the unicode for every character in the given string.
 */
void RS_Debug::printUnicode(const QString &text) {
    for (auto const &v: text) {
        print("[%X] %c", v.unicode(), v.toLatin1());
    }
}


// EOF
