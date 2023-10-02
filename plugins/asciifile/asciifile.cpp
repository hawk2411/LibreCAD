/*****************************************************************************/
/*  Asciifile.cpp - ascii file importer                                          */
/*                                                                           */
/*  Copyright (C) 2011 Rallaz, rallazz@gmail.com                             */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 2 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

#include <QPicture>
#include <QPainter>
#include <QMouseEvent>
#include <QPushButton>
#include <QSettings>

#include <QMessageBox>

#include "document_interface.h"
#include "dibPunto.h"
#include "asciifile.h"

PluginCapabilities AsciiFile::getCapabilities() const
{
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
            << PluginMenuLocation("plugins_menu", tr("Read ascii points"));
    return pluginCapabilities;
}

QString AsciiFile::name() const
 {
     return (tr("Read ascii points"));
 }

void AsciiFile::execComm(IDocumentPlugin *doc,
                         QWidget *parent, QString cmd)
{
    Q_UNUSED(cmd);
    dibPunto pdt( parent);
    int result = pdt.exec();
    if (result == QDialog::Accepted) {
        pdt.processFile(doc);
    }
}




/*****************************/

