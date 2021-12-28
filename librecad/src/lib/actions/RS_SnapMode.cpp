//
// Created by hawk on 28.12.21.
//

#include "RS_SnapMode.h"

/**
  * Disable all snapping.
  *
  * This effectively puts the object into free snap mode.
  *
  * @returns A reference to itself.
  */
RS_SnapMode const &RS_SnapMode::clear() {
    snapIntersection = false;
    snapOnEntity = false;
    snapCenter = false;
    snapDistance = false;
    snapMiddle = false;
    snapEndpoint = false;
    snapGrid = false;
    snapFree = false;
    snapAngle = false;

    restriction = RS2::RestrictNothing;

    return *this;
}

bool RS_SnapMode::operator==(RS_SnapMode const &rhs) const {
    return snapIntersection == rhs.snapIntersection
           && snapOnEntity == rhs.snapOnEntity
           && snapCenter == rhs.snapCenter
           && snapDistance == rhs.snapDistance
           && snapMiddle == rhs.snapMiddle
           && snapEndpoint == rhs.snapEndpoint
           && snapGrid == rhs.snapGrid
           && snapFree == rhs.snapFree
           && restriction == rhs.restriction
           && snapAngle == rhs.snapAngle;
}


/**
  * snap mode to a flag integer
  */
uint RS_SnapMode::toInt(const RS_SnapMode &s) {
    uint ret{0};

    if (s.snapIntersection) ret |= RS_SnapMode::SnapIntersection;
    if (s.snapOnEntity) ret |= RS_SnapMode::SnapOnEntity;
    if (s.snapCenter) ret |= RS_SnapMode::SnapCenter;
    if (s.snapDistance) ret |= RS_SnapMode::SnapDistance;
    if (s.snapMiddle) ret |= RS_SnapMode::SnapMiddle;
    if (s.snapEndpoint) ret |= RS_SnapMode::SnapEndpoint;
    if (s.snapGrid) ret |= RS_SnapMode::SnapGrid;
    if (s.snapFree) ret |= RS_SnapMode::SnapFree;
    if (s.snapAngle) ret |= RS_SnapMode::SnapAngle;

    switch (s.restriction) {
        case RS2::RestrictHorizontal:
            ret |= RS_SnapMode::RestrictHorizontal;
            break;
        case RS2::RestrictVertical:
            ret |= RS_SnapMode::RestrictVertical;
            break;
        case RS2::RestrictOrthogonal:
            ret |= RS_SnapMode::RestrictOrthogonal;
            break;
        default:
            break;
    }

    return ret;
}

/**
  * integer flag to snapMode
  */
RS_SnapMode RS_SnapMode::fromInt(unsigned int ret) {
    RS_SnapMode s;

    if (RS_SnapMode::SnapIntersection & ret) s.snapIntersection = true;
    if (RS_SnapMode::SnapOnEntity & ret) s.snapOnEntity = true;
    if (RS_SnapMode::SnapCenter & ret) s.snapCenter = true;
    if (RS_SnapMode::SnapDistance & ret) s.snapDistance = true;
    if (RS_SnapMode::SnapMiddle & ret) s.snapMiddle = true;
    if (RS_SnapMode::SnapEndpoint & ret) s.snapEndpoint = true;
    if (RS_SnapMode::SnapGrid & ret) s.snapGrid = true;
    if (RS_SnapMode::SnapFree & ret) s.snapFree = true;
    if (RS_SnapMode::SnapAngle & ret) s.snapAngle = true;

    switch (RS_SnapMode::RestrictOrthogonal & ret) {
        case RS_SnapMode::RestrictHorizontal:
            s.restriction = RS2::RestrictHorizontal;
            break;
        case RS_SnapMode::RestrictVertical:
            s.restriction = RS2::RestrictVertical;
            break;
        case RS_SnapMode::RestrictOrthogonal:
            s.restriction = RS2::RestrictOrthogonal;
            break;
        default:
            s.restriction = RS2::RestrictNothing;
            break;
    }

    return s;
}
