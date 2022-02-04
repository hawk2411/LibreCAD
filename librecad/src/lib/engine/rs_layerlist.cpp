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

#include<iostream>
#include "rs_debug.h"
#include "rs_layerlist.h"
#include "rs_layer.h"
#include "rs_layerlistlistener.h"

/**
 * Default constructor.
 */
RS_LayerList::RS_LayerList() : _layerWidget(nullptr), _activeLayer(nullptr), _modified(false) {
}


/**
 * Removes all layers in the layerlist.
 */
void RS_LayerList::clear() {
    _layers.clear();
    setModified(true);
}


QList<RS_Layer *>::iterator RS_LayerList::begin() {
    return _layers.begin();
}

QList<RS_Layer *>::iterator RS_LayerList::end() {
    return _layers.end();
}

QList<RS_Layer *>::const_iterator RS_LayerList::begin() const {
    return _layers.begin();
}

QList<RS_Layer *>::const_iterator RS_LayerList::end() const {
    return _layers.end();
}


/**
 * Activates the given layer.
 * 
 * @param notify Notify listeners.
 */
void RS_LayerList::activate(const QString &name, bool notify) {
    RS_DEBUG->print("RS_LayerList::activate: %s, notify: %d begin",
                    name.toLatin1().data(), notify);

    activate(find(name), notify);
    /*
    if (activeLayer==NULL) {
        RS_DEBUG->print("activeLayer is NULL");
} else {
        RS_DEBUG->print("activeLayer is %s", activeLayer->getName().latin1());
}
    */

    RS_DEBUG->print("RS_LayerList::activate: %s end", name.toLatin1().data());
}


/**
 * Activates the given layer.
 * 
 * @param notify Notify listeners.
 */
void RS_LayerList::activate(RS_Layer *layer, bool notify) {
    RS_DEBUG->print("RS_LayerList::activate notify: %d begin", notify);

    /*if (layer) {
        RS_DEBUG->print("RS_LayerList::activate: %s",
                        layer->getName().latin1());
} else {
        RS_DEBUG->print("RS_LayerList::activate: NULL");
}*/

    _activeLayer = layer;

    if (notify) {
        for (int i = 0; i < _layerListListeners.size(); ++i) {
            RS_LayerListListener *l = _layerListListeners.at(i);

            l->layerActivated(_activeLayer);
            RS_DEBUG->print("RS_LayerList::activate listener notified");
        }
    }

    RS_DEBUG->print("RS_LayerList::activate end");
}


/**
 * @brief sort by layer names
 */
void RS_LayerList::sort() {
    std::stable_sort(_layers.begin(), _layers.end(), [](const RS_Layer *l0, const RS_Layer *l1) -> bool {
        return l0->getName() < l1->getName();
    });
}

/**
 * Adds a layer to the layer list.
 * If there is already a layer with the same name, no layer is 
 * added. In that case the layer passed to the method will be deleted!
 * If no layer was active so far, the new layer becomes the active one.
 *
 * Listeners are notified.
 */
void RS_LayerList::add(RS_Layer *layer) {
    RS_DEBUG->print("RS_LayerList::addLayer()");

    if (layer == NULL) {
        return;
    }

    // check if layer already exists:
    RS_Layer *l = find(layer->getName());
    if (l == NULL) {
        _layers.append(layer);
        this->sort();
        // notify listeners
        for (int i = 0; i < _layerListListeners.size(); ++i) {
            RS_LayerListListener *l = _layerListListeners.at(i);
            l->layerAdded(layer);
        }
        setModified(true);

        // if there was no active layer so far, activate this one.
        if (_activeLayer == NULL) {
            activate(layer);
        }
    } else {
        // if there was no active layer so far, activate this one.
        if (_activeLayer == NULL) {
            activate(l);
        }

        l->freeze(layer->isFrozen());
        l->lock(layer->isLocked());
        l->setPrint(layer->isPrint());
        l->setConverted(layer->isConverted());
        l->setConstruction(layer->isConstruction());
        l->visibleInLayerList(layer->isVisibleInLayerList());
        l->setPen(layer->getPen());

        delete layer;
        layer = NULL;
    }
}


/**
 * Removes a layer from the list.
 * Listeners are notified after the layer was removed from 
 * the list but before it gets deleted.
 */
void RS_LayerList::remove(RS_Layer *layer) {
    RS_DEBUG->print("RS_LayerList::removeLayer()");
    if (layer == NULL) {
        return;
    }

    // here the layer is removed from the list but not deleted
    _layers.removeOne(layer);

    for (int i = 0; i < _layerListListeners.size(); ++i) {
        RS_LayerListListener *l = _layerListListeners.at(i);
        l->layerRemoved(layer);
    }

    setModified(true);

    // activate an other layer if necessary:
    if (_activeLayer == layer) {
        activate(_layers.first());
    }

    // now it's save to delete the layer
    delete layer;
}


/**
 * Changes a layer's attributes. The attributes of layer 'layer'
 * are copied from layer 'source'.
 * Listeners are notified.
 */
void RS_LayerList::edit(RS_Layer *layer, const RS_Layer &source) {
    if (layer == NULL) {
        return;
    }

    *layer = source;

    for (int i = 0; i < _layerListListeners.size(); ++i) {
        RS_LayerListListener *l = _layerListListeners.at(i);

        l->layerEdited(layer);
    }

    setModified(true);
}


/**
 * @return Pointer to the layer with the given name or
 * \p NULL if no such layer was found.
 */
RS_Layer *RS_LayerList::find(const QString &name) {
    //RS_DEBUG->print("RS_LayerList::find begin");

    RS_Layer *ret = NULL;

    for (int i = 0; i < _layers.size(); ++i) {
        RS_Layer *l = _layers.at(i);
        if (l->getName() == name) {
            ret = l;
            break;
        }
    }

    //RS_DEBUG->print("RS_LayerList::find end");

    return ret;
}

/**
 * Switches on / off the given layer. 
 * Listeners are notified.
 */
void RS_LayerList::toggle(const QString &name) {
    toggle(find(name));
}


/**
 * Switches on / off the given layer. 
 * Listeners are notified.
 */
void RS_LayerList::toggle(RS_Layer *layer) {

    if (!layer) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_LayerList::toggle: nullptr layer");
        return;
    }

    // set flags
    layer->toggle();
    setModified(true);

    // Notify listeners:
    for (auto *i: _layerListListeners) {

        if (!i) {
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_LayerList::toggle: nullptr layer listener");
            continue;
        }

        RS_LayerListListener *l = (RS_LayerListListener *) i;
        l->layerToggled(layer);
    }
}


/**
 * Locks or unlocks the given layer.
 * Listeners are notified.
 */
void RS_LayerList::toggleLock(RS_Layer *layer) {
    if (layer == NULL) {
        return;
    }

    layer->toggleLock();
    setModified(true);

    // Notify listeners:
    for (int i = 0; i < _layerListListeners.size(); ++i) {
        RS_LayerListListener *l = _layerListListeners.at(i);
        l->layerToggledLock(layer);
    }
}


/**
 * Switch printing for the given layer on / off.
 * Listeners are notified.
 */
void RS_LayerList::togglePrint(RS_Layer *layer) {
    if (layer == NULL) {
        return;
    }

    layer->togglePrint();
    setModified(true);

    // Notify listeners:
    for (int i = 0; i < _layerListListeners.size(); ++i) {
        RS_LayerListListener *l = _layerListListeners.at(i);
        l->layerToggledPrint(layer);
    }
}


/**
 * Switch construction attribute for the given layer on / off.
 * Listeners are notified.
 */
void RS_LayerList::toggleConstruction(RS_Layer *layer) {
    if (layer == NULL) {
        return;
    }

    layer->toggleConstruction();
    setModified(true);

    // Notify listeners:
    for (int i = 0; i < _layerListListeners.size(); ++i) {
        RS_LayerListListener *l = _layerListListeners.at(i);
        l->layerToggledConstruction(layer);
    }
}


/**
 * Freezes or defreezes all layers.
 *
 * @param freeze true: freeze, false: defreeze
 */
void RS_LayerList::freezeAll(bool freeze) {

    for ( auto & layer : _layers) {
        if (layer->isVisibleInLayerList()) {
            layer->freeze(freeze);
        }
    }
    setModified(true);

    for (auto listener : _layerListListeners) {
        listener->layerToggled(nullptr);
    }
}


/**
 * Locks or unlocks all layers.
 *
 * @param lock true: lock, false: unlock
 */
void RS_LayerList::lockAll(bool lock) {

    for (auto & layer : _layers) {
        if (layer->isVisibleInLayerList()) {
            layer->lock(lock);
        }
    }
    setModified(true);

    for (auto l : _layerListListeners) {
        l->layerToggled(nullptr);
    }
}


/**
 * adds a LayerListListener to the list of listeners. Listeners
 * are notified when the layer list changes.
 *
 * Typical listeners are: layer list widgets, pen toolbar, graphic view
 */
void RS_LayerList::addListener(RS_LayerListListener *listener) {
    _layerListListeners.append(listener);
}


/**
 * removes a LayerListListener from the list of listeners. 
 */
void RS_LayerList::removeListener(RS_LayerListListener *listener) {
    _layerListListeners.removeOne(listener);
}


/**
 * Dumps the layers to stdout.
 */
std::ostream &operator<<(std::ostream &os, RS_LayerList &layerList) {

    os << "Layerlist: \n";
    for (auto & layer: layerList) {
        os << *layer << "\n";
    }

    return os;
}


/**
 * Sets the layer lists modified status to 'm'.
 * Listeners are notified.
 */
void RS_LayerList::setModified(bool m) {
    _modified = m;

    // Notify listeners
    for (auto l: _layerListListeners) {
        l->layerListModified(m);
    }
}

