//
// Created by hawk on 27.04.22.
//
//

#include <gtest/gtest.h>
#include "./lib/filters/rs_filterdxfrw.h"

TEST(RS_F, Bla) {
    RS2::AngleFormat angle = RS_FilterDXFRW::numberToAngleFormat(10);
    auto number = RS_FilterDXFRW::angleFormatToNumber(angle);
    auto unit_number = RS_FilterDXFRW::unitToNumber(RS2::Unit::Foot);
    auto is_2_dimension = RS_FilterDXFRW::isVariableTwoDimensional("$LIMMAX");
    RS_FilterDXFRW filterDxfrw;

    bool is_exact_color =filterDxfrw.isExactColor();

    ASSERT_EQ(angle, RS2::DegreesDecimal) << "Should be 0";
    ASSERT_EQ(number, 0) << "Should be 0";
    ASSERT_EQ(unit_number, 2) << "Should be 2";
    ASSERT_EQ(is_2_dimension, true) << "Should be true";
    ASSERT_EQ(is_exact_color, false) << "Should be false";
}
