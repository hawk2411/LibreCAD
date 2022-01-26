/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
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


#ifndef RS_DOCUMENT_H
#define RS_DOCUMENT_H

#include "rs_layerlist.h"
#include "rs_entitycontainer.h"
#include "rs_undo.h"

class RS_BlockList;

/**
 * Base class for documents. Documents can be either graphics or
 * blocks and are typically shown in graphic views. Documents hold
 * an active pen for drawing in the Document, a file name and they
 * know whether they have been modified or not.
 *
 * @author Andrew Mustun
 */
class RS_Document : public RS_EntityContainer,
                    public RS_Undo {
public:
    explicit RS_Document(RS_EntityContainer *parent = nullptr);

    ~RS_Document() override = default;

    virtual RS_LayerList *getLayerList() const = 0;

    virtual RS_BlockList * getBlockList() const = 0;

    virtual void newDoc() = 0;

    virtual bool save(bool isAutoSave) = 0;     //false is default

    virtual bool saveAs(const QString &filename, RS2::FormatType type, bool force) = 0;

    virtual bool open(const QString &filename, RS2::FormatType type) = 0;

    virtual bool loadTemplate(const QString &filename, RS2::FormatType type) = 0;


    /**
     * @return true for all document entities (e.g. Graphics or Blocks).
     */
    bool isDocument() const override {
        return true;
    }

    /**
     * Removes an entity from the entiy container. Implementation
     * from RS_Undo.
     */
    void removeUndoable(RS_Undoable *u) override {
        if (u && u->undoRtti() == RS2::UndoableEntity && u->isUndone()) {
            removeEntity(dynamic_cast<RS_Entity *>(u));
        }
    }

    /**
     * @return Currently active drawing pen.
     */
    RS_Pen getActivePen() const {
        return activePen;
    }

    /**
     * Sets the currently active drawing pen to p.
     */
    void setActivePen(const RS_Pen &p) {
        activePen = p;
    }

    /**
     * @return File name of the document currently loaded.
     * Note, that the default file name is empty.
     */
    QString getFilename() const {
        return _filename;
    }

    /**
     * @return Auto-save file name of the document currently loaded.
     */
    QString getAutoSaveFilename() const {
        return _autosaveFilename;
    }

    /**
     * Sets file name for the document currently loaded.
     */
    void setFilename(const QString &fn) {
        _filename = fn;
    }

    /**
     * Sets the documents modified status to 'm'.
     */
    virtual void setModified(bool m) {
        _modified = m;
    }

    /**
     * @retval true The document has been modified since it was last saved.
     * @retval false The document has not been modified since it was last saved.
     */
    virtual bool isModified() const {
        return _modified;
    }

    /**
     * Overwritten to set modified flag when undo cycle finished with undoable(s).
     */
    void endUndoCycle(std::unique_ptr<RS_UndoCycle> undoCycle) override;

    void setGraphicView(RS_GraphicView *g) { _gv = g; }

    RS_GraphicView *getGraphicView() { return _gv; }

protected:
    /** Flag set if the document was modified and not yet saved. */
    bool _modified;
    /** Active pen. */
    RS_Pen activePen;
    /** File name of the document or empty for a new document. */
    QString _filename;
    /** Auto-save file name of document. */
    QString _autosaveFilename;
    /** Format type */
    RS2::FormatType _formatType;
    RS_GraphicView* _gv;//used to read/save current view

};


#endif
