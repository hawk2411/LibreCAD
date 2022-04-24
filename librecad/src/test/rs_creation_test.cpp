//
// Created by hawk on 24.04.22.
//
#include <gtest/gtest.h>
#include <memory>

#include "rs_entitycontainer.h"
#include "rs_circle.h"
#include "rs_creation.h"
// NOLINTNEXTLINE
TEST(rs_creation_test, Bla) {

    std::vector<RS_Entity *> circle_tangent = RS_Creation::createCircleTangent2(
            new RS_Circle(nullptr, RS_CircleData(RS_Vector(1.0, 1.0), 2.0)),
            new RS_Circle(nullptr, RS_CircleData(RS_Vector(1.0, 1.0), 2.0)));
    bool all_are_valid = std::all_of(circle_tangent.begin(), circle_tangent.end(),
                                     [](RS_Entity* entity){return entity->isValid();} );
    ASSERT_TRUE(all_are_valid);
}