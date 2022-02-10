/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
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


#ifndef RS_LAYERLIST_H
#define RS_LAYERLIST_H

#include <QMap>
#include "rs_layer.h"

class RS_LayerListListener;

class QG_LayerWidget;

/**
 * A list of layers.
 *
 * @author Andrew Mustun
 */
class RS_LayerList {
public:
    RS_LayerList();

    virtual ~RS_LayerList() = default;

    void clear();

    /**
     * @return Number of layers in the list.
     */
    unsigned int count() const {
        return _layers.count();
    }

    QMap<QString, RS_Layer *>::iterator begin();

    QMap<QString, RS_Layer *>::iterator end();

    QMap<QString, RS_Layer *>::const_iterator begin() const;

    QMap<QString, RS_Layer *>::const_iterator end() const;

    void activate(const QString &name, bool notify = false);

    void activate(RS_Layer *layer, bool notify = false);

    //! @return The active layer of NULL if no layer is activated.
    RS_Layer *getActive() {
        return _activeLayer;
    }

    virtual void add(RS_Layer *layer);

    virtual void remove(RS_Layer *layer);

    virtual void edit(RS_Layer *layer, const RS_Layer &source);

    RS_Layer *find(const QString &name);

    void toggle(const QString &name);

    void toggle(RS_Layer *layer);

    void toggleLock(RS_Layer *layer);

    void togglePrint(RS_Layer *layer);

    void toggleConstruction(RS_Layer *layer);

    void freezeAll(bool freeze);

    void lockAll(bool lock);

    //! sets the layerWidget pointer in RS_LayerListClass
    void setLayerWitget(QG_LayerWidget *lw) {
        _layerWidget = lw;
    }

    //! @return the layerWidget pointer inside the RS_LayerListClass
    QG_LayerWidget *getLayerWitget() {
        return _layerWidget;
    }
    //! @return First layer of the list.
    //RS_Layer* firstLayer() {
    //    return layers.first();
    //}
    /** @return Next layer from the list after
     * calling firstLayer() or nextLayer().
     */
    //RS_Layer* nextLayer() {
    //    return layers.next();
    //}

    void addListener(RS_LayerListListener *listener);

    void removeListener(RS_LayerListListener *listener);

    /**
     * Sets the layer lists modified status to 'm'.
     */
    void setModified(bool m);

    /**
     * @retval true The layer list has been modified.
     * @retval false The layer list has not been modified.
     */
    virtual bool isModified() const {
        return _modified;
    }

    friend std::ostream &operator<<(std::ostream &os, RS_LayerList &l);

private:
    //! layers in the graphic
    QMap<QString, RS_Layer *> _layers;
    //! List of registered LayerListListeners
    QList<RS_LayerListListener *> _layerListListeners;
    QG_LayerWidget *_layerWidget;
    //! Currently active layer
    RS_Layer *_activeLayer;
    /** Flag set if the layer list was modified and not yet saved. */
    bool _modified;
};

#endif
