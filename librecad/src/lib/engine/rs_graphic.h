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

    RS_LayerList *getLayerList() override { return &layerList; }

    RS_BlockList *getBlockList() const override { return blockList.get(); }

    void newDoc() override;

    bool save(bool isAutoSave) override;

    bool saveAs(const QString &filename, RS2::FormatType type, bool force) override;

    bool open(const QString &filename, RS2::FormatType type) override;

    bool loadTemplate(const QString &filename, RS2::FormatType type) override;

    // Wrappers for Layer functions:
    void clearLayers() {
        layerList.clear();
    }

    unsigned countLayers() const {
        return layerList.count();
    }

    RS_Layer *layerAt(unsigned i) {
        return layerList.at(i);
    }

    void activateLayer(const QString &name) {
        layerList.activate(name);
    }

    void activateLayer(RS_Layer *layer) {
        layerList.activate(layer);
    }

    RS_Layer *getActiveLayer() {
        return layerList.getActive();
    }

    virtual void addLayer(RS_Layer *layer) {
        layerList.add(layer);
    }

    void addEntity(RS_Entity *entity) override;

    virtual void removeLayer(RS_Layer *layer);

    virtual void editLayer(RS_Layer *layer, const RS_Layer &source) {
        layerList.edit(layer, source);
    }

    RS_Layer *findLayer(const QString &name) {
        return layerList.find(name);
    }

    void toggleLayer(RS_Layer *layer) {
        layerList.toggle(layer);
    }

    void toggleLayerLock(RS_Layer *layer) {
        layerList.toggleLock(layer);
    }

    void toggleLayerPrint(RS_Layer *layer) {
        layerList.togglePrint(layer);
    }

    void toggleLayerConstruction(RS_Layer *layer) {
        layerList.toggleConstruction(layer);
    }

    void freezeAllLayers(bool freeze) {
        layerList.freezeAll(freeze);
    }

    void lockAllLayers(bool lock) {
        layerList.lockAll(lock);
    }

    void addLayerListListener(RS_LayerListListener *listener) {
        layerList.addListener(listener);
    }

    // Wrappers for variable functions:
    void clearVariables() {
        variableDict.clear();
    }

    void addVariable(const QString &key, const RS_Vector &value, int code) {
        variableDict.add(key, value, code);
    }

    void addVariable(const QString &key, const QString &value, int code) {
        variableDict.add(key, value, code);
    }

    void addVariable(const QString &key, int value, int code) {
        variableDict.add(key, value, code);
    }

    void addVariable(const QString &key, double value, int code) {
        variableDict.add(key, value, code);
    }

    RS_Vector getVariableVector(const QString &key, const RS_Vector &def) {
        return variableDict.getVector(key, def);
    }

    QString getVariableString(const QString &key, const QString &def) {
        return variableDict.getString(key, def);
    }

    int getVariableInt(const QString &key, int def) {
        return variableDict.getInt(key, def);
    }

    double getVariableDouble(const QString &key, double def) {
        return variableDict.getDouble(key, def);
    }

    QHash<QString, RS_Variable> &getVariableDict() {
        return variableDict.getVariableDict();
    }

    RS2::LinearFormat getLinearFormat();

    RS2::LinearFormat getLinearFormat(int f);

    int getLinearPrecision();

    RS2::AngleFormat getAngleFormat();

    int getAnglePrecision();

    RS_Vector getPaperSize();

    void setPaperSize(const RS_Vector &s);

    RS_Vector getPrintAreaSize(bool total = true);

    RS_Vector getPaperInsertionBase();

    void setPaperInsertionBase(const RS_Vector &p);

    RS2::PaperFormat getPaperFormat(bool *landscape);

    void setPaperFormat(RS2::PaperFormat f, bool landscape);

    double getPaperScale();

    void setPaperScale(double s);

    virtual void setUnit(RS2::Unit u);

    virtual RS2::Unit getUnit();

    bool isGridOn();

    void setGridOn(bool on);

    bool isIsometricGrid();

    void setIsometricGrid(bool on);

    void setCrosshairType(RS2::CrosshairType chType);

    RS2::CrosshairType getCrosshairType();

    void centerToPage();

    bool fitToPage();

    bool isBiggerThanPaper();

    /**
     * @retval true The document has been modified since it was last saved.
     * @retval false The document has not been modified since it was last saved.
     */
    bool isModified() const override {
        return _modified || layerList.isModified() || blockList->isModified();
    }

    /**
     * Sets the documents modified status to 'm'.
     */
    void setModified(bool m) override {
        _modified = m;
        layerList.setModified(m);
        blockList->setModified(m);
    }

    //if set to true, will refuse to modify paper scale
    void setPaperScaleFixed(bool fixed) {
        paperScaleFixed = fixed;
    }

    bool getPaperScaleFixed() const {
        return paperScaleFixed;
    }

    /**
     * Paper margins in millimeters
     */
    void setMargins(double left, double top, double right, double bottom) {
        if (left >= 0.0) marginLeft = left;
        if (top >= 0.0) marginTop = top;
        if (right >= 0.0) marginRight = right;
        if (bottom >= 0.0) marginBottom = bottom;
    }

    double getMarginLeft() const {
        return marginLeft;
    }

    double getMarginTop() const {
        return marginTop;
    }

    double getMarginRight() const {
        return marginRight;
    }

    double getMarginBottom() const {
        return marginBottom;
    }

    /**
     * Paper margins in graphic units
     */
    void setMarginsInUnits(double left, double top, double right, double bottom);

    double getMarginLeftInUnits();

    double getMarginTopInUnits();

    double getMarginRightInUnits();

    double getMarginBottomInUnits();

    /**
     * Number of pages drawing occupies
     */
    void setPagesNum(int horiz, int vert);

    void setPagesNum(const QString &horizXvert);

    int getPagesNumHoriz() const {
        return pagesNumH;
    }

    int getPagesNumVert() const {
        return pagesNumV;
    }

    friend std::ostream &operator<<(std::ostream &os, RS_Graphic &g);

    int clean();

private:

    bool BackupDrawingFile(const QString &filename);

    QDateTime modifiedTime;
    QString currentFileName; //keep a copy of filename for the modifiedTime

    RS_BlockList *test;
    RS_LayerList layerList;
    std::unique_ptr<RS_BlockList> blockList = std::make_unique<RS_BlockList>(true);
    RS_VariableDict variableDict;
    RS2::CrosshairType crosshairType; //crosshair type used by isometric grid
    //if set to true, will refuse to modify paper scale
    bool paperScaleFixed;

    // Paper margins in millimeters
    double marginLeft;
    double marginTop;
    double marginRight;
    double marginBottom;

    // Number of pages drawing occupies
    int pagesNumH;
    int pagesNumV;
};


#endif
