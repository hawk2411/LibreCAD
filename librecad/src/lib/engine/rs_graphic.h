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


#ifndef RS_GRAPHIC_H
#define RS_GRAPHIC_H

#include <QDateTime>
#include "rs_blocklist.h"
#include "rs_layerlist.h"
#include "rs_variabledict.h"
#include "rs_document.h"
#include "rs_units.h"

class RS_VariableDict;

class QG_LayerWidget;

/**
 * A graphic document which can contain entities layers and blocks.
 *
 * @author Andrew Mustun
 */
class RS_Graphic : public RS_Document {
public:
    explicit RS_Graphic(RS_EntityContainer *parent = nullptr);

    ~RS_Graphic() override;

    /** @return RS2::EntityGraphic */
    RS2::EntityType rtti() const override {
        return RS2::EntityGraphic;
    }

    RS_LayerList *getLayerList() const override { return _layerList.get(); }

    RS_BlockList *getBlockList() const override { return _blockList.get(); }

    void newDoc() override;

    bool save(bool isAutoSave) override;

    bool saveAs(const QString &filename, RS2::FormatType type, bool force) override;

    bool open(const QString &filename, RS2::FormatType type) override;

    bool loadTemplate(const QString &filename, RS2::FormatType type) override;

    void addEntity(RS_Entity *entity) override;

    virtual void removeLayer(RS_Layer *layer);

    // Wrappers for variable functions:
    void clearVariables() {
        _variableDict.clear();
    }

    void addVariable(const QString &key, const RS_Vector &value, int code) {
        _variableDict.add(key, value, code);
    }

    void addVariable(const QString &key, const QString &value, int code) {
        _variableDict.add(key, value, code);
    }

    void addVariable(const QString &key, int value, int code) {
        _variableDict.add(key, value, code);
    }

    void addVariable(const QString &key, double value, int code) {
        _variableDict.add(key, value, code);
    }

    RS_Vector getVariableVector(const QString &key, const RS_Vector &def) {
        return _variableDict.getVector(key, def);
    }

    QString getVariableString(const QString &key, const QString &def) {
        return _variableDict.getString(key, def);
    }

    int getVariableInt(const QString &key, int def) {
        return _variableDict.getInt(key, def);
    }

    double getVariableDouble(const QString &key, double def) {
        return _variableDict.getDouble(key, def);
    }

    QHash<QString, RS_Variable> &getVariableDict() {
        return _variableDict.getVariableDict();
    }

    RS_Vector getPaperInsertionBase() const;

    void setPaperInsertionBase(const RS_Vector &p) const;

    RS2::PaperFormat getPaperFormat(bool *landscape) const;

    void setPaperFormat(RS2::PaperFormat f, bool landscape) const;

    double getPaperScale() const;

    void setPaperScale(double s) const;

    virtual void setUnit(RS2::Unit u) const;

    virtual RS2::Unit getUnit() const;

    bool isGridOn() const;

    void setGridOn(bool on) const;

    bool isIsometricGrid() const;

    void setIsometricGrid(bool on) const;

    void setCrosshairType(RS2::CrosshairType chType);

    RS2::CrosshairType getCrosshairType() const;

    void centerToPage() const;

    bool fitToPage() const;

    bool isBiggerThanPaper() const;

    /**
     * @retval true The document has been modified since it was last saved.
     * @retval false The document has not been modified since it was last saved.
     */
    bool isModified() const override {
        return _modified || _layerList->isModified() || _blockList->isModified();
    }

    /**
     * Sets the documents modified status to 'm'.
     */
    void setModified(bool m) override {
        _modified = m;
        _layerList->setModified(m);
        _blockList->setModified(m);
    }

    //if set to true, will refuse to modify paper scale
    void setPaperScaleFixed(bool fixed) {
        _paperScaleFixed = fixed;
    }

    bool getPaperScaleFixed() const {
        return _paperScaleFixed;
    }

    /**
     * Paper margins in millimeters
     */
    void setMargins(double left, double top, double right, double bottom) {
        if (left >= 0.0) _marginLeft = left;
        if (top >= 0.0) _marginTop = top;
        if (right >= 0.0) _marginRight = right;
        if (bottom >= 0.0) _marginBottom = bottom;
    }

    double getMarginLeft() const {
        return _marginLeft;
    }

    double getMarginTop() const {
        return _marginTop;
    }

    double getMarginRight() const {
        return _marginRight;
    }

    double getMarginBottom() const {
        return _marginBottom;
    }

    /**
     * Paper margins in graphic units
     */
    void setMarginsInUnits(double left, double top, double right, double bottom);

    double getMarginLeftInUnits() const;

    double getMarginTopInUnits() const;

    double getMarginRightInUnits() const;

    double getMarginBottomInUnits() const;

    /**
     * Number of pages drawing occupies
     */
    void setPagesNum(int horiz, int vert);

    void setPagesNum(const QString &horizXvert);

    int getPagesNumHoriz() const {
        return _pagesNumH;
    }

    int getPagesNumVert() const {
        return _pagesNumV;
    }

    friend std::ostream &operator<<(std::ostream &os, RS_Graphic &g);

    int clean();

private:
    RS2::Unit getUnitLocal() const { return (RS2::Unit) _variableDict->getInt("$INSUNITS", 0); }
    void setUnitLocal(RS2::Unit u) const {
        setPaperSize(RS_Units::convert(getPaperSize(), getUnit(), u));

        _variableDict->add("$INSUNITS", (int) u, 70);
    }
    static bool BackupDrawingFile(const QString &filename);

    QDateTime _modifiedTime;
    QString _currentFileName; //keep a copy of filename for the modifiedTime

    std::unique_ptr<RS_LayerList> _layerList = std::make_unique<RS_LayerList>();
    std::unique_ptr<RS_BlockList> _blockList = std::make_unique<RS_BlockList>();
    RS_VariableDict _variableDict;
    RS2::CrosshairType _crosshairType; //crosshair type used by isometric grid
    //if set to true, will refuse to modify paper scale
    bool _paperScaleFixed;

    // Paper margins in millimeters
    double _marginLeft;
    double _marginTop;
    double _marginRight;
    double _marginBottom;

    // Number of pages drawing occupies
    int _pagesNumH;
    int _pagesNumV;
};


#endif
