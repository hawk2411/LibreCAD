/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 Dongxu Li (dongxuli2011@gmail.com)
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

#ifndef RS_SNAPPER_H
#define RS_SNAPPER_H

#include<memory>
#include <rs_vector.h>
#include <rs_pen.h>
#include "rs.h"
#include "RS_SnapMode.h"

class RS_Entity;
class RS_GraphicView;
class RS_Vector;
class RS_Preview;
class QMouseEvent;
class RS_EntityContainer;


typedef std::initializer_list<RS2::EntityType> EntityTypeList;


/**
 * This class is used for snapping functions in a graphic view.
 * Actions are usually derived from this base class if they need
 * to catch entities or snap to coordinates. Use the methods to
 * retrieve a graphic coordinate from a mouse coordinate.
 *
 * Possible snapping functions are described in RS_SnapMode.
 *
 * @author Andrew Mustun
 */
class RS_Snapper {
public:
	RS_Snapper() = delete;
    RS_Snapper(RS_EntityContainer& container, RS_GraphicView& graphicView);
	virtual ~RS_Snapper();

    void init();
	//!
	//! \brief finish stop using snapper
	//!
        void finish();

    /**
     * @return Pointer to the entity which was the key entity for the
     * last successful snapping action. If the snap mode is "end point"
     * the key entity is the entity whose end point was caught.
     * If the snap mode didn't require an entity (e.g. free, grid) this
     * method will return NULL.
     */
	RS_Entity* getKeyEntity() const {
        return keyEntity;
    }

    /** Sets a new snap mode. */
    void setSnapMode(const RS_SnapMode& snapMode);

	RS_SnapMode const* getSnapMode() const;
	RS_SnapMode* getSnapMode();

    /** Sets a new snap restriction. */
    void setSnapRestriction(RS2::SnapRestriction /*snapRes*/) {
        //this->snapRes = snapRes;
    }

        /**
         * Sets the snap range in pixels for catchEntity().
         *
         * @see catchEntity()
         */
        void setSnapRange(int r) {
                snapRange = r;
        }

        /**manually set snapPoint*/
    RS_Vector snapPoint(const RS_Vector& coord, bool setSpot = false);
    RS_Vector snapPoint(QMouseEvent* e);
    RS_Vector snapFree(QMouseEvent* e);

    RS_Vector snapFree(const RS_Vector& coord);
    RS_Vector snapGrid(const RS_Vector& coord) const;
    RS_Vector snapEndpoint(const RS_Vector& coord) const;
    RS_Vector snapOnEntity(const RS_Vector& coord);
    RS_Vector snapCenter(const RS_Vector& coord) const;
    RS_Vector snapMiddle(const RS_Vector& coord) const;
    RS_Vector snapDist(const RS_Vector& coord) const;
    RS_Vector snapIntersection(const RS_Vector& coord)const;
    //RS_Vector snapDirect(RS_Vector coord, bool abs);
    RS_Vector snapToAngle(const RS_Vector &coord, const RS_Vector &ref_coord, double ang_res);

    RS_Vector restrictOrthogonal(const RS_Vector& coord);
    RS_Vector restrictHorizontal(const RS_Vector& coord);
    RS_Vector restrictVertical(const RS_Vector& coord);


    //RS_Entity* catchLeafEntity(const RS_Vector& pos);
    //RS_Entity* catchLeafEntity(QMouseEvent* e);
    RS_Entity* catchEntity(const RS_Vector& pos,
                           RS2::ResolveLevel level=RS2::ResolveNone);
    RS_Entity* catchEntity(QMouseEvent* e,
                           RS2::ResolveLevel level=RS2::ResolveNone);
    // catch Entity closest to pos and of the given entity type of enType, only search for a particular entity type
    RS_Entity* catchEntity(const RS_Vector& pos, RS2::EntityType enType,
                           RS2::ResolveLevel level=RS2::ResolveNone);
    RS_Entity* catchEntity(QMouseEvent* e, RS2::EntityType enType,
                           RS2::ResolveLevel level=RS2::ResolveNone);
	RS_Entity* catchEntity(QMouseEvent* e, const EntityTypeList& enTypeList,
                           RS2::ResolveLevel level=RS2::ResolveNone);

    /**
     * Suspends this snapper while another action takes place.
     */
	virtual void suspend();

    /**
     * Resumes this snapper after it has been suspended.
     */
    virtual void resume() {
        drawSnapper();
    }

    virtual void hideOptions();
    virtual void showOptions();

    void drawSnapper();

    struct ImpData {
        RS_Vector snapCoord;
        RS_Vector snapSpot;
    };

/**
  * Methods and structs for class RS_Snapper
  */
    struct Indicator {
        bool lines_state;
        QString lines_type;
        RS_Pen lines_pen;

        bool shape_state;
        QString shape_type;
        RS_Pen shape_pen;
    };

protected:
    void deleteSnapper();
    double getSnapRange() const;
    RS_EntityContainer* container;
    RS_GraphicView* graphicView;
	RS_Entity* keyEntity{};
    RS_SnapMode snapMode;
    //RS2::SnapRestriction snapRes;
    /**
     * Snap distance for snapping to points with a
     * given distance from endpoints.
     */
	double m_SnapDistance{};
    /**
     * Snap to equidistant middle points
     * default to 1, i.e., equidistant to start/end points
     */
    int middlePoints{};
    /**
     * Snap range for catching entities.
     */
    int snapRange{};
    bool finished{false};

private:
	std::unique_ptr<ImpData> pImpData;
    std::unique_ptr<Indicator>snap_indicator;
};

#endif
//EOF
