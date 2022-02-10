/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
** Copyright (C) 2016 ravas (github.com/r-a-v-a-s)
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

#include <iostream>
#include <cmath>
#include <QDir>
//#include <QDebug>

#include "rs_graphic.h"
#include "rs_dialogfactory.h"

#include "rs_debug.h"
#include "rs_fileio.h"
#include "rs_math.h"
#include "rs_units.h"
#include "rs_settings.h"
#include "rs_layer.h"
#include "rs_block.h"


/**
 * Default constructor.
 */
RS_Graphic::RS_Graphic(RS_EntityContainer *parent)
        : RS_Document(parent),
          _paperScaleFixed(false),
          _marginLeft(0.0),
          _marginTop(0.0),
          _marginRight(0.0),
          _marginBottom(0.0),
          pagesNumH(1),
          pagesNumV(1) {

    RS_SETTINGS->beginGroup("/Defaults");
    setUnitLocal(RS_Units::stringToUnit(RS_SETTINGS->readEntry("/Unit", "None")));

    RS_SETTINGS->endGroup();
    RS_SETTINGS->beginGroup("/Appearance");
    //$ISOMETRICGRID == $SNAPSTYLE
    addVariable("$SNAPSTYLE", static_cast<int>(RS_SETTINGS->readNumEntry("/IsometricGrid", 0)), 70);
    _crosshairType = static_cast<RS2::CrosshairType>(RS_SETTINGS->readNumEntry("/CrosshairType", 0));
    RS_SETTINGS->endGroup();
    RS2::Unit unit = getUnitLocal();

    if (unit == RS2::Inch) {
        _variableDict->add("$DIMASZ", 0.1, 40);
        _variableDict->add("$DIMEXE", 0.05, 40);
        _variableDict->add("$DIMEXO", 0.025, 40);
        _variableDict->add("$DIMGAP", 0.025, 40);
        _variableDict->add("$DIMTXT", 0.1, 40);
    } else {
        _variableDict->add("$DIMASZ",
                           RS_Units::convert(2.5, RS2::Millimeter, unit), 40);
        _variableDict->add("$DIMEXE",
                           RS_Units::convert(1.25, RS2::Millimeter, unit), 40);
        _variableDict->add("$DIMEXO",
                           RS_Units::convert(0.625, RS2::Millimeter, unit), 40);
        _variableDict->add("$DIMGAP",
                           RS_Units::convert(0.625, RS2::Millimeter, unit), 40);
        _variableDict->add("$DIMTXT",
                           RS_Units::convert(2.5, RS2::Millimeter, unit), 40);
    }
    _variableDict->add("$DIMTIH", 0, 70);
    //initialize printer vars bug #3602444
    setPaperScale(getPaperScale());
    setPaperInsertionBase(getPaperInsertionBase());

    _modified = false;
    _layerList->setModified(false);
    _blockList->setModified(false);
}


/**
 * Destructor.
 */
RS_Graphic::~RS_Graphic() = default;


/**
 * Removes the given layer and undoes all entities on it.
 */
void RS_Graphic::removeLayer(RS_Layer *layer){

    if (layer && layer->getName() != "0") {

        std::vector<RS_Entity *> toRemove;
        //find entities on layer
        for (auto e: _entities) {
            if (e->getLayer() &&
                e->getLayer()->getName() == layer->getName()) {
                toRemove.push_back(e);
            }
        }
        // remove all entities on that layer:
        if (!toRemove.empty()) {
            auto undoCycle = startUndoCycle();
            for (auto e: toRemove) {
                e->setUndoState(true);
                e->setLayer("0");
                undoCycle->addUndoable(e);
            }
            endUndoCycle(std::move(undoCycle));
        }

        toRemove.clear();
        // remove all entities in blocks that are on that layer:
        for (RS_Block *blk: *_blockList) {
            if (!blk) continue;
            for (auto entity: *blk) {

                if (entity->getLayer() &&
                    entity->getLayer()->getName() == layer->getName()) {
                    toRemove.push_back(entity);
                }
            }
        }

        for (auto e: toRemove) {
            e->setUndoState(true);
            e->setLayer("0");
        }

        _layerList->remove(layer);
    }
}


/**
 * Clears all layers, blocks and entities of this graphic.
 * A default layer (0) is created.
 */
void RS_Graphic::newDoc() {

    RS_DEBUG->print("RS_Graphic::newDoc");

    clear();
    _layerList->clear();
    _blockList->clear();

    _layerList->clear();
    _blockList->clear();

    _layerList->add(new RS_Layer("0"));
    //addLayer(new RS_Layer("ByBlock"));

    setModified(false);
}


/*
 * Description:	Create/update the drawing backup file, if necessary.
 * Author(s):		Claude Sylvain
 * Created:			13 July 2011
 * Last modified:
 *
 * Parameters:		const QString &filename:
 * 						Name of the drawing file to backup.
 *
 * Returns:			bool:
 * 						false	: Operation failed.
 * 						true	: Operation successful.
 */

bool RS_Graphic::BackupDrawingFile(const QString &filename) {
    /*	- Create backup only if drawing file name exist.
     *	- Remark: Not really necessary to check if the drawing file
     *	  name have been defined.
     *	----------------------------------------------------------- */
    if (filename.isEmpty()) { return false; }

    /*	Built Backup File Name.
     *	*/
    QString backupFileName(filename + '~');

    QFile drawingFile(filename);

    /*	Create backup file only if drawing file already exist.
     *	------------------------------------------------------ */
    if (!drawingFile.exists()) { return false; }

    /*	Create "Drawing File Backup" object.
     *	*/
    QFile drawingBackup(backupFileName);

    /*	If able to create the object, process...
     *	---------------------------------------- */
    /*	If a backup file already exist, remove it!
     *	------------------------------------------ */
    if (drawingBackup.exists()) { drawingBackup.remove(); }

    drawingFile.copy(backupFileName);    /*	Create backup file. */

    return true;
}


/*
 *	Description:	Saves this graphic with the current filename and settings.
 *	Author(s):		..., Claude Sylvain
 * Last modified:	13 July 2011
 *	Parameters:
 *
 *	Returns:			bool:
 *							false:	Operation failed.
 *							true:		Operation successful.
 *
 * Notes:			- If this is not an AutoSave, backup the drawing file
 * 					  (if necessary).
 * 					- Drawing is saved only when it has been modified.
 * 					  This prevent lost of backup file when file
 * 					  is saved more than one time without being modified.
 */

bool RS_Graphic::save(bool isAutoSave) {
    bool ret = false;

    RS_DEBUG->print("RS_Graphic::save: Entering...");

    /*	- Save drawing file only if it has been modified.
         *	- Notes: Potentially dangerous in case of an internal
         *	  coding error that make LibreCAD not aware of modification
         *	  when some kind of drawing modification is done.
         *	----------------------------------------------------------- */
    if (isModified()) {
        QString actualName;
        RS2::FormatType actualType;

        actualType = _formatType;

        if (isAutoSave) {
            actualName = _autosaveFilename;

            if (_formatType == RS2::FormatUnknown)
                actualType = RS2::FormatDXFRW;
        } else {
            //	- This is not an AutoSave operation.  This is a manual
            //	  save operation.  So, ...
            //		- Set working file name to the drawing file name.
            //		- Backup drawing file (if necessary).
            //	------------------------------------------------------
            QFileInfo finfo(_filename);
            QDateTime m = finfo.lastModified();
            //bug#3414993
            //modifiedTime should only be used for the same filename
//            DEBUG_HEADER
//            qDebug()<<"currentFileName= "<<currentFileName;
//            qDebug()<<"Checking file: filename= "<<filename;
//            qDebug()<<"Checking file: "<<filename;
//            qDebug()<<"modifiedTime.isValid()="<<modifiedTime.isValid();
//            qDebug()<<"Previous timestamp: "<<modifiedTime;
//            qDebug()<<"Current timestamp: "<<m;
            if (_currentFileName == QString(_filename)
                && _modifiedTime.isValid() && m != _modifiedTime) {
                //file modified by others
//            qDebug()<<"detected on disk change";
                GetDialogFactory()->commandMessage(QObject::tr(
                        "File on disk modified. Please save to another file to avoid data loss! File modified: %1").arg(
                        _filename));
                return false;
            }

            actualName = _filename;
            if (RS_SETTINGS->readNumEntry("/AutoBackupDocument", 1) != 0)
                BackupDrawingFile(_filename);
        }

        /*	Save drawing file if able to created associated object.
                 *	------------------------------------------------------- */
        if (!actualName.isEmpty()) {
            RS_DEBUG->print("RS_Graphic::save: File: %s", actualName.toLatin1().data());
            RS_DEBUG->print("RS_Graphic::save: Format: %d", (int) actualType);
            RS_DEBUG->print("RS_Graphic::save: Export...");

            ret = RS_FileIO::instance()->fileExport(*this, actualName, actualType);
            QFileInfo finfo(actualName);
            _modifiedTime = finfo.lastModified();
            _currentFileName = actualName;
        } else {
            RS_DEBUG->print("RS_Graphic::save: Can't create object!");
            RS_DEBUG->print("RS_Graphic::save: File not saved!");
        }

        /*	Remove AutoSave file after user has successfully saved file.
                 *	------------------------------------------------------------ */
        if (ret && !isAutoSave) {
            /*	Autosave file object.
                         *	*/
            QFile qf_file(_autosaveFilename);

            /*	Tell that drawing file is no more modified.
                         *	------------------------------------------- */
            setModified(false);
            _layerList->setModified(false);
            _blockList->setModified(false);

            /*	- Remove autosave file, if able to create associated object,
                         *	  and if autosave file exist.
                         *	------------------------------------------------------------ */
            if (qf_file.exists()) {
                RS_DEBUG->print("RS_Graphic::save: Removing old autosave file %s",
                                _autosaveFilename.toLatin1().data());
                qf_file.remove();
            }

        }

        RS_DEBUG->print("RS_Graphic::save: Done!");
    } else {
        RS_DEBUG->print("RS_Graphic::save: File not modified, not saved");
        ret = true;
    }

    RS_DEBUG->print("RS_Graphic::save: Exiting...");

    return ret;
}


/*
 *	Description:	- Saves this graphic with the given filename and current
 *						  settings.
 *
 *	Author(s):		..., Claude Sylvain
 *	Created:			?
 *	Last modified:	13 July 2011
 *	Parameters:         QString: name to save
 *                      RS2::FormatType: format to save
 *                      bool:
 *                          false: do not save if not needed
 *                          true: force to save (when called for save as...
 *
 *	Returns:			bool:
 *							false:	Operation failed.
 *							true:		Operation successful.
 *
 * Notes:			Backup the drawing file (if necessary).
 */

bool RS_Graphic::saveAs(const QString &filename, RS2::FormatType type, bool force) {
    RS_DEBUG->print("RS_Graphic::saveAs: Entering...");

    // Set to "failed" by default.

    // Check/memorize if file name we want to use as new file
    // name is the same as the actual file name.
    bool fn_is_same = filename == this->_filename;
    auto const filenameSaved = this->_filename;
    auto const autosaveFilenameSaved = this->_autosaveFilename;
    auto const formatTypeSaved = this->_formatType;

    this->_filename = filename;
    this->_formatType = type;

    // QString	const oldAutosaveName = this->autosaveFilename;
    QFileInfo finfo(filename);

    // Construct new autosave filename by prepending # to the filename
    // part, using the same directory as the destination file.
    this->_autosaveFilename = finfo.path() + "/#" + finfo.fileName();

    // When drawing is saved using a different name than the actual
    // drawing file name, make LibreCAD think that drawing file
    // has been modified, to make sure the drawing file saved.
    if (!fn_is_same || force)
        setModified(true);

    bool ret = save(false);        //	Save file.

    if (ret) {
        // Save was successful, remove old autosave file.
        QFile qf_file(autosaveFilenameSaved);

        if (qf_file.exists()) {
            RS_DEBUG->print("RS_Graphic::saveAs: Removing old autosave file %s",
                            autosaveFilenameSaved.toLatin1().data());
            qf_file.remove();
        }

    } else {
        //do not modify filenames:
        this->_filename = filenameSaved;
        this->_autosaveFilename = autosaveFilenameSaved;
        this->_formatType = formatTypeSaved;
    }

    return ret;
}


/**
 * Loads the given file into this graphic.
 */
bool RS_Graphic::loadTemplate(const QString &filename, RS2::FormatType type) {
    RS_DEBUG->print("RS_Graphic::loadTemplate(%s)", filename.toLatin1().data());

    // Construct new autosave filename by prepending # to the filename part,
    // using system temporary dir.
    this->_autosaveFilename = QDir::tempPath() + "/#" + "Unnamed.dxf";

    // clean all:
    newDoc();

    // import template file:
    bool ret = RS_FileIO::instance()->fileImport(*this, filename, type);

    setModified(false);
    _layerList->setModified(false);
    _blockList->setModified(false);
    QFileInfo finfo;
    _modifiedTime = finfo.lastModified();

    RS_DEBUG->print("RS_Graphic::loadTemplate(%s): OK", filename.toLatin1().data());

    return ret;
}

/**
 * Loads the given file into this graphic.
 */
bool RS_Graphic::open(const QString &filename, RS2::FormatType type) {
    RS_DEBUG->print("RS_Graphic::open(%s)", filename.toLatin1().data());

    this->_filename = filename;
    QFileInfo finfo(filename);
    // Construct new autosave filename by prepending # to the filename
    // part, using the same directory as the destination file.
    this->_autosaveFilename = finfo.path() + "/#" + finfo.fileName();

    // clean all:
    newDoc();

    // import file:
    bool ret = RS_FileIO::instance()->fileImport(*this, filename, type);

    if (ret) {
        setModified(false);
        _layerList->setModified(false);
        _blockList->setModified(false);
        _modifiedTime = finfo.lastModified();
        _currentFileName = QString(filename);

        //cout << *((RS_Graphic*)graphic);
        //calculateBorders();

        RS_DEBUG->print("RS_Graphic::open(%s): OK", filename.toLatin1().data());
    }

    return ret;
}


/**
 * @return true if the grid is switched on (visible).
 */
bool RS_Graphic::isGridOn() const {
    int on = _variableDict->getInt("$GRIDMODE", 1);
    return on != 0;
}


/**
 * Enables / disables the grid.
 */
void RS_Graphic::setGridOn(bool on) const {
    _variableDict->add("$GRIDMODE", (int) on, 70);
}

/**
 * @return true if the isometric grid is switched on (visible).
 */
bool RS_Graphic::isIsometricGrid() const {
    //$ISOMETRICGRID == $SNAPSTYLE
    int on = _variableDict->getInt("$SNAPSTYLE", 0);
    return on != 0;
}


/**
 * Enables / disables isometric grid.
 */
void RS_Graphic::setIsometricGrid(bool on) const {
    //$ISOMETRICGRID == $SNAPSTYLE
    _variableDict->add("$SNAPSTYLE", (int) on, 70);
}

void RS_Graphic::setCrosshairType(RS2::CrosshairType chType) {
    _crosshairType = chType;
}

RS2::CrosshairType RS_Graphic::getCrosshairType() {
    return _crosshairType;
}

/**
 * Sets the unit of this graphic to 'u'
 */
void RS_Graphic::setUnit(RS2::Unit u) const {
    setUnitLocal(u);
}


/**
 * Gets the unit of this graphic
 */
RS2::Unit RS_Graphic::getUnit() const {
    return getUnitLocal();
    //return unit;
}


/**
 * @return The linear format type for this document.
 * This is determined by the variable "$LUNITS".
 */
RS2::LinearFormat RS_Graphic::getLinearFormat() const{
    int lunits = _variableDict->getInt("$LUNITS", 2);
    return getLinearFormat(lunits);
}

/**
 * @return The linear format type used by the variable "$LUNITS" & "$DIMLUNIT".
 */
RS2::LinearFormat RS_Graphic::getLinearFormat(int f){
    switch (f) {
        default:
        case 2:
            return RS2::Decimal;

        case 1:
            return RS2::Scientific;

        case 3:
            return RS2::Engineering;

        case 4:
            return RS2::Architectural;

        case 5:
            return RS2::Fractional;

        case 6:
            return RS2::ArchitecturalMetric;
    }
}


/**
 * @return The linear precision for this document.
 * This is determined by the variable "$LUPREC".
 */
int RS_Graphic::getLinearPrecision() const{
    return _variableDict->getInt("$LUPREC", 4);
}


/**
 * @return The angle format type for this document.
 * This is determined by the variable "$AUNITS".
 */
RS2::AngleFormat RS_Graphic::getAngleFormat() const {
    int aunits = _variableDict->getInt("$AUNITS", 0);

    switch (aunits) {
        default:
        case 0:
            return RS2::DegreesDecimal;
        case 1:
            return RS2::DegreesMinutesSeconds;
        case 2:
            return RS2::Gradians;
        case 3:
            return RS2::Radians;
        case 4:
            return RS2::Surveyors;
    }
}


/**
 * @return The linear precision for this document.
 * This is determined by the variable "$LUPREC".
 */
int RS_Graphic::getAnglePrecision() const {
    return _variableDict->getInt("$AUPREC", 4);
}


/**
 * @return The insertion point of the drawing into the paper space.
 * This is the distance from the lower left paper edge to the zero
 * point of the drawing. DXF: $PINSBASE.
 */
RS_Vector RS_Graphic::getPaperInsertionBase() const {
    return _variableDict->getVector("$PINSBASE", RS_Vector(0.0, 0.0));
}


/**
 * Sets the PINSBASE variable.
 */
void RS_Graphic::setPaperInsertionBase(const RS_Vector &p) const {
    _variableDict->add("$PINSBASE", p, 10);
}


/**
 * @return Paper size in graphic units.
 */
RS_Vector RS_Graphic::getPaperSize() const {
    RS_SETTINGS->beginGroup("/Print");
    bool okX, okY;
    double sX = RS_SETTINGS->readEntry("/PaperSizeX", "0.0").toDouble(&okX);
    double sY = RS_SETTINGS->readEntry("/PaperSizeY", "0.0").toDouble(&okY);
    RS_SETTINGS->endGroup();
    RS_Vector def;
    if (okX && okY && sX > RS_TOLERANCE && sY > RS_TOLERANCE) {
        def = RS_Units::convert(RS_Vector(sX, sY),
                                RS2::Millimeter, getUnit());
    } else {
        def = RS_Units::convert(RS_Vector(210.0, 297.0),
                                RS2::Millimeter, getUnit());
    }

    RS_Vector v1 = _variableDict->getVector("$PLIMMIN", RS_Vector(0.0, 0.0));
    RS_Vector v2 = _variableDict->getVector("$PLIMMAX", def);

    return v2 - v1;
}


/**
 * Sets a new paper size.
 */
void RS_Graphic::setPaperSize(const RS_Vector &s) const {
    _variableDict->add("$PLIMMIN", RS_Vector(0.0, 0.0), 10);
    _variableDict->add("$PLIMMAX", s, 10);
    //set default paper size
    RS_Vector def = RS_Units::convert(s,
                                      getUnit(), RS2::Millimeter);
    RS_SETTINGS->beginGroup("/Print");
    RS_SETTINGS->writeEntry("/PaperSizeX", def.x);
    RS_SETTINGS->writeEntry("/PaperSizeY", def.y);
    RS_SETTINGS->endGroup();

}


/**
 * @return Print Area size in graphic units.
 */
RS_Vector RS_Graphic::getPrintAreaSize(bool total) const{
    RS_Vector printArea = getPaperSize();
    printArea.x -= RS_Units::convert(_marginLeft + _marginRight, RS2::Millimeter, getUnit());
    printArea.y -= RS_Units::convert(_marginTop + _marginBottom, RS2::Millimeter, getUnit());
    if (total) {
        printArea.x *= _pagesNumH;
        printArea.y *= _pagesNumV;
    }
    return printArea;
}


/**
 * @return Paper format.
 * This is determined by the variables "$PLIMMIN" and "$PLIMMAX".
 *
 * @param landscape will be set to true for landscape and false for portrait if not NULL.
 */
RS2::PaperFormat RS_Graphic::getPaperFormat(bool *landscape) const {
    RS_Vector size = RS_Units::convert(getPaperSize(),
                                       getUnit(), RS2::Millimeter);

    if (landscape) {
        *landscape = (size.x > size.y);
    }

    return RS_Units::paperSizeToFormat(size);
}


/**
 * Sets the paper format to the given format.
 */
void RS_Graphic::setPaperFormat(RS2::PaperFormat f, bool landscape) const {
    RS_Vector size = RS_Units::paperFormatToSize(f);

    if (landscape ^ (size.x > size.y)) {
        std::swap(size.x, size.y);
    }

    setPaperSize(RS_Units::convert(size, RS2::Millimeter, getUnit()));
}


/**
 * @return Paper space scaling (DXF: $PSVPSCALE).
 */
double RS_Graphic::getPaperScale() const {
    double ret;

    ret = _variableDict->getDouble("$PSVPSCALE", 1.0);
//    if (ret<1.0e-6) {
//        ret = 1.0;
//    }

    return ret;
}


/**
 * Sets a new scale factor for the paper space.
 */
void RS_Graphic::setPaperScale(double s) {
    if (_paperScaleFixed == false) addVariable("$PSVPSCALE", s, 40);
}


/**
 * Centers drawing on page. Affects DXF variable $PINSBASE.
 */
void RS_Graphic::centerToPage() const {
    RS_Vector size = getPrintAreaSize();
    double scale = getPaperScale();
    auto s = getSize();
    auto sMin = getMin();
    /** avoid zero size, bug#3573158 */
    if (fabs(s.x) < RS_TOLERANCE) {
        s.x = 10.;
        sMin.x = -5.;
    }
    if (fabs(s.y) < RS_TOLERANCE) {
        s.y = 10.;
        sMin.y = -5.;
    }

    RS_Vector pinsbase = (size - s * scale) / 2.0 - sMin * scale;
    pinsbase.x += RS_Units::convert(_marginLeft, RS2::Millimeter, getUnit());
    pinsbase.y += RS_Units::convert(_marginBottom, RS2::Millimeter, getUnit());

    setPaperInsertionBase(pinsbase);
}


/**
 * Fits drawing on page. Affects DXF variable $PINSBASE.
 */
bool RS_Graphic::fitToPage() const {
    bool ret(true);
    RS_Vector ps = getPrintAreaSize();
    RS_Vector s = getSize();
    /** avoid zero size, bug#3573158 */
    if (fabs(s.x) < RS_TOLERANCE) s.x = 10.;
    if (fabs(s.y) < RS_TOLERANCE) s.y = 10.;
    double fx = RS_MAXDOUBLE;
    double fy = RS_MAXDOUBLE;
    double fxy;
    //ps = RS_Units::convert(ps, getUnit(), RS2::Millimeter);

    // tin-pot 2011-12-30: TODO: can s.x < 0.0 (==> fx < 0.0) happen?
    if (fabs(s.x) > RS_TOLERANCE) {
        fx = ps.x / s.x;
        // ret=false;
    }
    if (fabs(s.y) > RS_TOLERANCE) {
        fy = ps.y / s.y;
        // ret=false;
    }

    fxy = std::min(fx, fy);
    if (fxy >= RS_MAXDOUBLE || fxy <= 1.0e-10) {
        setPaperSize(
                RS_Units::convert(RS_Vector(210., 297.), RS2::Millimeter, getUnit()
                )
        );
        ret = false;
    }
    setPaperScale(fxy);
    centerToPage();
    return ret;
}


bool RS_Graphic::isBiggerThanPaper() const{
    RS_Vector ps = getPrintAreaSize();
    RS_Vector s = getSize() * getPaperScale();
    return !s.isInWindow(RS_Vector(0.0, 0.0), ps);
}


void RS_Graphic::addEntity(RS_Entity *entity) {
    RS_EntityContainer::addEntity(entity);
    if (entity->rtti() == RS2::EntityBlock ||
        entity->rtti() == RS2::EntityContainer) {
        auto *e = dynamic_cast<RS_EntityContainer *>(entity);
        for (auto e1: *e) {
            addEntity(e1);
        }
    }
}


/**
 * Dumps the entities to stdout.
 */
std::ostream &operator<<(std::ostream &os, RS_Graphic &g) {
    os << "--- Graphic: \n";
    os << "---" << *g.getLayerList() << "\n";
    os << "---" << *g.getBlockList() << "\n";
    os << "---" << (RS_Undo &) g << "\n";
    os << "---" << (RS_EntityContainer &) g << "\n";

    return os;
}

/**
 * Removes invalid objects.
 * @return how many objects were removed
 */
int RS_Graphic::clean() {
    // author: ravas

    int how_many = 0;

    for (RS_Entity *e: _entities) {
        if (e->getMin().x > e->getMax().x
            || e->getMin().y > e->getMax().y
            || e->getMin().x > RS_MAXDOUBLE
            || e->getMax().x > RS_MAXDOUBLE
            || e->getMin().x < RS_MINDOUBLE
            || e->getMax().x < RS_MINDOUBLE
            || e->getMin().y > RS_MAXDOUBLE
            || e->getMax().y > RS_MAXDOUBLE
            || e->getMin().y < RS_MINDOUBLE
            || e->getMax().y < RS_MINDOUBLE) {
            removeEntity(e);
            how_many += 1;
        }
    }
    return how_many;
}

/**
 * Paper margins in graphic units
 */
void RS_Graphic::setMarginsInUnits(double left, double top, double right, double bottom) {
    setMargins(
            RS_Units::convert(left, getUnit(), RS2::Millimeter),
            RS_Units::convert(top, getUnit(), RS2::Millimeter),
            RS_Units::convert(right, getUnit(), RS2::Millimeter),
            RS_Units::convert(bottom, getUnit(), RS2::Millimeter));
}

double RS_Graphic::getMarginLeftInUnits() {
    return RS_Units::convert(_marginLeft, RS2::Millimeter, getUnit());
}

double RS_Graphic::getMarginTopInUnits() {
    return RS_Units::convert(_marginTop, RS2::Millimeter, getUnit());
}

double RS_Graphic::getMarginRightInUnits() {
    return RS_Units::convert(_marginRight, RS2::Millimeter, getUnit());
}

double RS_Graphic::getMarginBottomInUnits() {
    return RS_Units::convert(_marginBottom, RS2::Millimeter, getUnit());
}

void RS_Graphic::setPagesNum(int horiz, int vert) {
    if (horiz > 0)
        _pagesNumH = horiz;
    if (vert > 0)
        _pagesNumV = vert;
}

void RS_Graphic::setPagesNum(const QString &horizXvert) {
    if (horizXvert.contains('x')) {
        bool ok1 = false;
        bool ok2 = false;
        int i = horizXvert.indexOf('x');
        int h = (int) RS_Math::eval(horizXvert.left(i), &ok1);
        int v = (int) RS_Math::eval(horizXvert.mid(i + 1), &ok2);
        if (ok1 && ok2)
            setPagesNum(h, v);
    }
}
