/*****************************************************************************/
/*  Asciifile.h - ascii file importer                                        */
/*                                                                           */
/*  Copyright (C) 2011 Rallaz, rallazz@gmail.com                             */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 2 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

#ifndef DRAWPOINTS_H
#define DRAWPOINTS_H

#include <QWidget>
#include "qc_plugininterface.h"
#include "document_interface.h"

class AsciiFile : public QObject, QC_PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QC_PluginInterface)
    Q_PLUGIN_METADATA(IID LC_DocumentInterface_iid FILE  "asciifile.json")

 public:
    [[nodiscard]] PluginCapabilities getCapabilities() const override;
    [[nodiscard]] QString name() const override;
    void execComm(IDocumentPlugin *doc, QWidget *parent, QString cmd) override;
};

#endif // ECHOPLUG_H
