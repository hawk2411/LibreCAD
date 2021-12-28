//
// Created by hawk on 28.12.21.
//

#ifndef LIBRECAD_RS_SNAPMODE_H
#define LIBRECAD_RS_SNAPMODE_H

#include <rs.h>
#include <QtGlobal>

/**
  * This class holds information on how to snap the mouse.
  *
  * @author Kevin Cox
  */
struct RS_SnapMode {
    /* SnapModes for RS_SnapMode to Int conversion and vice versa
     *
     * The conversion is only used for save/restore of active snap modes in application settings.
     * Don't change existing mode order, because this will mess up settings on upgrades
     *
     * When adding new values, take care for correct implementation in \p toInt() and \p fromInt()
     */
    enum SnapModes {
        SnapIntersection    = 1 << 0,
        SnapOnEntity        = 1 << 1,
        SnapCenter          = 1 << 2,
        SnapDistance        = 1 << 3,
        SnapMiddle          = 1 << 4,
        SnapEndpoint        = 1 << 5,
        SnapGrid            = 1 << 6,
        SnapFree            = 1 << 7,
        RestrictHorizontal  = 1 << 8,
        RestrictVertical    = 1 << 9,
        RestrictOrthogonal  = RestrictHorizontal | RestrictVertical,
        SnapAngle           = 1 << 10
    };

    bool snapIntersection   {false}; //< Whether to snap to intersections or not.
    bool snapOnEntity       {false}; //< Whether to snap to entities or not.
    bool snapCenter         {false}; //< Whether to snap to centers or not.
    bool snapDistance       {false}; //< Whether to snap to distance from endpoints or not.
    bool snapMiddle         {false}; //< Whether to snap to midpoints or not.
    bool snapEndpoint       {false}; //< Whether to snap to endpoints or not.
    bool snapGrid           {false}; //< Whether to snap to grid or not.
    bool snapFree           {false}; //< Whether to snap freely
    bool snapAngle          {false}; //< Whether to snap along line under certain angle

    RS2::SnapRestriction restriction {RS2::RestrictNothing}; /// The restriction on the free snap.

    double distance {5.0}; //< The distance to snap before defaulting to free snapping.

    /**
      * Disable all snapping.
      *
      * This effectively puts the object into free snap mode.
      *
      * @returns A reference to itself.
      */
    RS_SnapMode const & clear();
    bool operator == (RS_SnapMode const& rhs) const;

    static uint toInt(const RS_SnapMode& s);    //< convert to int, to save settings
    static RS_SnapMode fromInt(unsigned int);   //< convert from int, to restore settings
};


#endif //LIBRECAD_RS_SNAPMODE_H
