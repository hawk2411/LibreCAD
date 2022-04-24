//
// Created by hawk on 07.01.22.
//
#include <gtest/gtest.h>
#include "rs_debug.h"
#include "rs_undo.h"
#include "rs_undoable.h"

class MyUndoImplementation : public RS_Undo {
public:
    void removeUndoable(RS_Undoable*) override {}

    ~MyUndoImplementation() override = default;

};

class MyUndoable : public RS_Undoable {
public:
    ~MyUndoable() override = default;

    void undoStateChanged(bool) override {}

};

// NOLINTNEXTLINE
TEST(UndoTest, Bla) {
    MyUndoImplementation rsUndo;
    MyUndoable undoable[2];

    rsUndo.startUndoCycle();
    ASSERT_EQ(rsUndo.countUndoCycles(), 0) << "Should be 0";
    ASSERT_EQ(rsUndo.countRedoCycles(), 0) << "Should be 0";
    rsUndo.addUndoable(&undoable[0]);
    rsUndo.endUndoCycle();
    ASSERT_EQ(rsUndo.countUndoCycles(), 1) << "Should be 1";
    ASSERT_EQ(rsUndo.countRedoCycles(), 0) << "Should be 0";
    rsUndo.startUndoCycle();
    rsUndo.addUndoable(&undoable[1]);
    rsUndo.endUndoCycle();
    ASSERT_EQ(rsUndo.countUndoCycles(), 2) << "Should be 2";
    ASSERT_EQ(rsUndo.countRedoCycles(), 0) << "Should be 0";

    rsUndo.undo();
    ASSERT_EQ(rsUndo.countUndoCycles(), 1) << "Should be 1";
    ASSERT_EQ(rsUndo.countRedoCycles(), 1) << "Should be 1";

    rsUndo.startUndoCycle();
    ASSERT_EQ(rsUndo.countUndoCycles(), 1) << "Should be 1";
    ASSERT_EQ(rsUndo.countRedoCycles(), 0) << "Should be 0";
    rsUndo.addUndoable(&undoable[1]);
    rsUndo.endUndoCycle();
    ASSERT_EQ(rsUndo.countUndoCycles(), 2) << "Should be 2";
    ASSERT_EQ(rsUndo.countRedoCycles(), 0) << "Should be 0";

    rsUndo.undo();
    rsUndo.redo();
    ASSERT_EQ(rsUndo.countUndoCycles(), 2) << "Should be 0";
    ASSERT_EQ(rsUndo.countRedoCycles(), 0) << "Should be 0";
}

// NOLINTNEXTLINE
TEST(DebugTest, deb) {
    RS_DEBUG_VERBOSE->print("Hallo");
    RS_DEBUG_VERBOSE->deleteInstance();
    RS_DEBUG_VERBOSE->timestamp();
    RS_DEBUG_VERBOSE->setStream(stderr);
    RS_DEBUG_VERBOSE->print("Hallo");
    RS_DEBUG_VERBOSE->print("Hallo2");
}

