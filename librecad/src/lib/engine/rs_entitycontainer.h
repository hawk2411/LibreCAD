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


#ifndef RS_ENTITYCONTAINER_H
#define RS_ENTITYCONTAINER_H

#include <vector>
#include "rs_entity.h"

/**
 * Class representing a tree of entities.
 * Typical entity containers are graphics, polylines, groups, texts, ...)
 *
 * @author Andrew Mustun
 */
class RS_EntityContainer : public RS_Entity {
public:

    explicit RS_EntityContainer(RS_EntityContainer *parent = nullptr, bool owner = true);

    //RS_EntityContainer(const RS_EntityContainer& ec);
    ~RS_EntityContainer() override;

    RS_Entity *clone() const override;

    virtual void detach();

    /** @return RS2::EntityContainer */
    RS2::EntityType rtti() const override {
        return RS2::EntityContainer;
    }

    void reparent(RS_EntityContainer *parent) override;

    /**
     * @return true: because entities made from this class
	 *         and subclasses are containers for other entities.
     */
    bool isContainer() const override {
        return true;
    }

    /**
     * @return false: because entities made from this class
     *         and subclasses are containers for other entities.
     */
    bool isAtomic() const override {
        return false;
    }

    double getLength() const override;

    void setVisible(bool v) override;

    bool setSelected(bool select) override;

    bool toggleSelected() override;

    virtual void selectWindow(RS_Vector v1, RS_Vector v2,
                              bool select, bool cross);

    virtual void addEntity(RS_Entity *entity);

    virtual void appendEntity(RS_Entity *entity);

    virtual void moveEntity(int index, QList<RS_Entity *> &entList);

    virtual void insertEntity(int index, RS_Entity *entity);

    virtual bool removeEntity(RS_Entity *entity);

    //!
    //! \brief addRectangle add four lines to form a rectangle by
    //! the diagonal vertices v0,v1
    //! \param v0,v1 diagonal vertices of the rectangle
    //!
    void addRectangle(RS_Vector const &v0, RS_Vector const &v1);

    virtual RS_Entity *firstEntity(RS2::ResolveLevel level);

    virtual RS_Entity *lastEntity(RS2::ResolveLevel level);

    virtual RS_Entity *nextEntity(RS2::ResolveLevel level);

    virtual RS_Entity *prevEntity(RS2::ResolveLevel level);

    virtual RS_Entity *entityAt(int index);

    virtual int findEntity(const RS_Entity *entity);

    virtual void clear();

    //virtual unsigned long int count() {
    //	return count(false);
    //}
    virtual bool isEmpty() const {
        return count() == 0;
    }

    unsigned count() const override;

    unsigned countDeep() const override;
    //virtual unsigned long int countLayerEntities(RS_Layer* layer);
    /** \brief countSelected number of selected
    * @param deep count sub-containers, if true
    * @param types if is not empty, only counts by types listed
    */
    virtual unsigned countSelected(bool deep, const std::initializer_list<RS2::EntityType> &types);

    virtual double totalSelectedLength();

    /**
     * Enables / disables automatic update of borders on entity removals
     * and additions. By default this is turned on.
     */
    virtual void setAutoUpdateBorders(bool enable) {
        _autoUpdateBorders = enable;
    }

    virtual void adjustBorders(RS_Entity *entity);

    void calculateBorders() override;

    void forcedCalculateBorders();

    void updateDimensions(bool autoText = true);

    virtual void updateInserts();

    virtual void updateSplines();

    void update() override;

    virtual void renameInserts(const QString &oldName,
                               const QString &newName);

    RS_Vector getNearestEndpoint(const RS_Vector &coord,
                                 double *dist) const override;

    RS_Vector getNearestEndpoint(const RS_Vector &coord,
                                 double *dist, RS_Entity **pEntity) const;

    RS_Entity *getNearestEntity(const RS_Vector &point,
                                double *dist = nullptr,
                                RS2::ResolveLevel level = RS2::ResolveAll) const;

    RS_Vector getNearestPointOnEntity(const RS_Vector &coord,
                                      bool onEntity,
                                      double *dist,
                                      RS_Entity **entity) const override;

    RS_Vector getNearestCenter(const RS_Vector &coord,
                               double *dist) const override;

    RS_Vector getNearestMiddle(const RS_Vector &coord,
                               double *dist,
                               int middlePoints) const override;

    RS_Vector getNearestDist(double distance,
                             const RS_Vector &coord,
                             double *dist ) const override;

    RS_Vector getNearestIntersection(const RS_Vector &coord,
                                     double *dist = nullptr);

    RS_Vector getNearestVirtualIntersection(const RS_Vector &coord,
                                            const double &angle,
                                            double *dist);

    RS_Vector getNearestRef(const RS_Vector &coord,
                            double *dist) const override;

    RS_Vector getNearestSelectedRef(const RS_Vector &coord,
                                    double *dist) const override;

    double getDistanceToPoint(const RS_Vector &coord,
                              RS_Entity **entity,
                              RS2::ResolveLevel level,
                              double solidDist) const override;

    virtual bool optimizeContours();

    bool hasEndpointsWithinWindow(const RS_Vector &v1, const RS_Vector &v2) override;

    void move(const RS_Vector &offset) override;

    void rotate(const RS_Vector &center, const double &angle) override;

    void rotate(const RS_Vector &center, const RS_Vector &angleVector) override;

    void scale(const RS_Vector &center, const RS_Vector &factor) override;

    void mirror(const RS_Vector &axisPoint1, const RS_Vector &axisPoint2a) override;

    void stretch(const RS_Vector &firstCorner,
                 const RS_Vector &secondCorner,
                 const RS_Vector &offset) override;

    void moveRef(const RS_Vector &ref, const RS_Vector &offset) override;

    void moveSelectedRef(const RS_Vector &ref, const RS_Vector &offset) override;

    void revertDirection() override;


    void draw(RS_Painter *painter, RS_GraphicView *view, double &patternOffset) override;

    friend std::ostream &operator<<(std::ostream &os, RS_EntityContainer &ec);

    bool isOwner() const { return _autoDelete; }

    void setOwner(bool owner) { _autoDelete = owner; }

    /**
     * @brief areaLineIntegral, line integral for contour area calculation by Green's Theorem
     * Contour Area =\oint x dy
     * @return line integral \oint x dy along the entity
     * returns absolute value
     */
    double areaLineIntegral() const override;

    /**
	 * @brief ignoreForModification ignore this entity for entity catch for certain actions
     * like catching circles to create tangent circles
     * @return, true, indicate this entity container should be ignored
     */
    bool ignoredOnModification() const;

    /**
     * @brief begin/end to support range based loop
     * @return iterator
     */
    QList<RS_Entity *>::const_iterator begin() const;

    QList<RS_Entity *>::const_iterator end() const;

    QList<RS_Entity *>::iterator begin();

    QList<RS_Entity *>::iterator end();

    //! \{
    //! first and last without resolving into children, assume the container is
    //! not empty
    RS_Entity *last() const;

    RS_Entity *first() const;
    //! \}

    const QList<RS_Entity *> &getEntityList();

protected:

    /** entities in the container */
    QList<RS_Entity *> _entities;

    /** sub container used only temporarily for iteration. */
    RS_EntityContainer *_subContainer;

    /**
     * Automatically update the borders of the container when entities
     * are added or removed.
     */
    static bool _autoUpdateBorders;

private:
    static bool hasEntitiesInArea(RS_EntityContainer *entityContainer, const RS_Vector &vec1, const RS_Vector &vec2);
    /**
     * @brief ignoredSnap whether snapping is ignored
     * @return true when entity of this container won't be considered for snapping points
     */
    bool ignoredSnap() const;

    int _entIdx;
    bool _autoDelete;

};

#endif
