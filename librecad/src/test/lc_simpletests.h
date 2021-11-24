#ifndef LC_SIMPLETESTS_H
#define LC_SIMPLETESTS_H

#include <QObject>

class QWidget;
class RS_EntityContainer;

class LC_SimpleTests:public QObject
{
	Q_OBJECT
public:
	explicit LC_SimpleTests(QWidget* parent);
	~LC_SimpleTests() override =default;

    void slotDumpEntities(RS_EntityContainer* d = nullptr);
public slots:
	/** dumps entities to file */
    void slotTestDumpEntities();

	/** dumps undo info to stdout */
	void slotTestDumpUndo();
	/** updates all inserts */
	void slotTestUpdateInserts();
	/** draws some random lines */
	void slotTestDrawFreehand();
	/** inserts a test block */
	void slotTestInsertBlock();
	/** inserts a test ellipse */
	void slotTestInsertEllipse();
	/** inserts a test mtext */
	void slotTestInsertMText();
	/** inserts a test text */
	void slotTestInsertText();
	/** inserts a test image */
	void slotTestInsertImage();
	/** unicode table */
	void slotTestUnicode();
	/** math experimental */
	void slotTestMath01();
	/** resizes window to 640x480 for screen shots */
	void slotTestResize640();
	/** resizes window to 640x480 for screen shots */
	void slotTestResize800();
	/** resizes window to 640x480 for screen shots */
	void slotTestResize1024();
};
#endif // LC_SIMPLETESTS_H
