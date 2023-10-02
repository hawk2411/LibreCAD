//
// Created by hawk on 27.09.2023.
//

#ifndef LIBRECAD_DIBPUNTO_H
#define LIBRECAD_DIBPUNTO_H

#include <QDialog>
#include <QFile>
#include <QLineEdit>
#include "dpt.h"
#include "qc_plugininterface.h"
#include "document_interface.h"
#include "pointData.h"
#include "textBox.h"

class dibPunto : public QDialog
{
    Q_OBJECT

public:
    explicit dibPunto( QWidget *parent = 0);
    ~dibPunto() override;
    //void SetupUI(QWidget *parent);

public slots:
            void dptFile();
    void processFile(IDocumentPlugin *doc);
    void checkAccept();

private:
    void readSettings();
    void writeSettings();
    void processFileODB(QFile* file, const QString& sep);
    void processFileNormal(QFile* file, const QString& sep, Qt::SplitBehavior skip = Qt::KeepEmptyParts);
    void drawLine(IDocumentPlugin *doc);
    void draw2D(IDocumentPlugin *doc);
    void draw3D(IDocumentPlugin *doc);
    void drawNumber(IDocumentPlugin *doc);
    void drawElev(IDocumentPlugin *doc);
    void drawCode(IDocumentPlugin *doc);
    bool failGUI(QString *msg);

    struct CalcPosResult
    {
        DPI::VAlign v = DPI::VAlignMiddle;
        DPI::HAlign h = DPI::HAlignCenter;
        double x = 0.0; double y =0.0;
    };

    static CalcPosResult calcPos(double sep, DPT::txtposition sit);
private:
    QString _errmsg;
    pointBox *_pt2d;
    pointBox *_pt3d;
    textBox *_ptnumber;
    textBox *_ptelev;
    textBox *_ptcode;
    QLineEdit *fileedit;
    QComboBox *formatedit;
    QCheckBox *_connectPoints;
    QList<pointData*> _dataList;
};

#endif //LIBRECAD_DIBPUNTO_H
