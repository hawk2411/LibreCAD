//
// Created by hawk on 27.09.2023.
//

#include <QPicture>
#include <QPainter>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QSettings>
#include <QComboBox>
#include <QLabel>
#include <cmath>

#include <QMessageBox>

#include "textBox.h"
#include "document_interface.h"
#include "dibPunto.h"

/*****************************/
dibPunto::dibPunto( QWidget *parent)
    :  QDialog(parent)
{
//    setParent(parent);
    setWindowTitle(tr("Read ascii points"));
    QStringList txtformats;

    auto mainLayout = new QGridLayout;
//readSettings();

    auto filebut = new QPushButton(tr("File..."));
    fileedit = new QLineEdit();
    auto lofile = new QHBoxLayout;
    lofile->addWidget(filebut);
    lofile->addWidget(fileedit);
    mainLayout->addLayout(lofile, 0, 0);

    auto formatlabel = new QLabel(tr("Format:"));
    formatedit = new QComboBox();
    txtformats << tr("Space Separator") << tr("Tab Separator") << tr("Comma Separator") << tr("Space in Columns") << tr("*.odb for Psion 2");
    formatedit->addItems(txtformats);
    _connectPoints = new QCheckBox(tr("Connect points"));

    auto loformat = new QHBoxLayout;
    loformat->addWidget(formatlabel);
    loformat->addWidget(formatedit);
    loformat->addWidget(_connectPoints);
    mainLayout->addLayout(loformat, 0, 1);

    _pt2d = new pointBox(tr("2D Point"), tr("Draw 2D Point"));
    _pt3d = new pointBox(tr("3D Point"), tr("Draw 3D Point"));
    _ptnumber = new textBox(tr("Point Number"), tr("Draw point number"));
    _ptelev = new textBox(tr("Point Elevation"), tr("Draw point elevation"));
    _ptcode = new textBox(tr("Point Code"), tr("Draw point code"));
    _ptnumber->setPos(DPT::NO);

    auto lo2d3d = new QVBoxLayout;

    lo2d3d->addWidget(_pt2d);
    lo2d3d->addWidget(_pt3d);
    mainLayout->addLayout(lo2d3d, 1, 0);

    mainLayout->addWidget(_ptnumber, 1, 1);
    mainLayout->addWidget(_ptelev, 2, 0);
    mainLayout->addWidget(_ptcode, 2, 1);

    auto loaccept = new QHBoxLayout;
    auto acceptbut = new QPushButton(tr("Accept"));
    loaccept->addStretch();
    loaccept->addWidget(acceptbut);
    mainLayout->addLayout(loaccept, 3, 0);

    auto cancelbut = new QPushButton(tr("Cancel"));
    auto locancel = new QHBoxLayout;
    locancel->addWidget(cancelbut);
    locancel->addStretch();
    mainLayout->addLayout(locancel, 3, 1);

    setLayout(mainLayout);
    readSettings();

    connect(cancelbut, &QPushButton::clicked, this, &dibPunto::reject);
    connect(acceptbut, &QPushButton::clicked, this, &dibPunto::checkAccept);

    connect(filebut, &QPushButton::clicked, this, &dibPunto::dptFile);
}

void dibPunto::checkAccept()
{

    _errmsg.clear();
    if (failGUI(&_errmsg)) {
        QMessageBox::critical (this, "Sample plugin", _errmsg );
        _errmsg.clear();
        return;
    }
    writeSettings();
    accept();
}

void dibPunto::dptFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select file"));
    fileedit->setText(fileName);
}

bool dibPunto::failGUI(QString *msg)
{
    if (_pt2d->checkOn()) {
        if (_pt2d->getLayer().isEmpty()) {
            msg->insert(0, tr("Point 2D layer is empty"));
            return true;
        }
    }
    if (_pt3d->checkOn()) {
        if (_pt3d->getLayer().isEmpty()) {
            msg->insert(0, tr("Point 3D layer is empty"));
            return true;
        }
    }
    if (_ptelev->checkOn()) {
        if (_ptelev->getLayer().isEmpty()) {
            msg->insert(0, tr("Point elevation layer is empty"));
            return true;
        }
        if (_ptelev->getHeightStr().isEmpty()) {
            msg->insert(0, tr("Point elevation height is empty"));
            return true;
        }
        if (_ptelev->getSeparationStr().isEmpty()) {
            msg->insert(0, tr("Point elevation separation is empty"));
            return true;
        }
    }
    if (_ptnumber->checkOn()) {
        if (_ptnumber->getLayer().isEmpty()) {
            msg->insert(0, tr("Point number layer is empty"));
            return true;
        }
        if (_ptnumber->getHeightStr().isEmpty()) {
            msg->insert(0, tr("Point number height is empty"));
            return true;
        }
        if (_ptnumber->getSeparationStr().isEmpty()) {
            msg->insert(0, tr("Point number separation is empty"));
            return true;
        }
    }
    if (_ptcode->checkOn()) {
        if (_ptcode->getLayer().isEmpty()) {
            msg->insert(0, tr("Point code layer is empty"));
            return true;
        }
        if (_ptcode->getHeightStr().isEmpty()) {
            msg->insert(0, tr("Point code height is empty"));
            return true;
        }
        if (_ptcode->getSeparationStr().isEmpty()) {
            msg->insert(0, tr("Point code separation is empty"));
            return true;
        }
    }
    return false;
}

void dibPunto::processFile(IDocumentPlugin *doc)
{
    QString sep;
    QMessageBox::information(this, "Info", "dibpunto processFile");

//Warning, can change adding or reordering "formatedit"
    auto skip = Qt::KeepEmptyParts;
    switch (formatedit->currentIndex()) {
        case 0:
            sep = " ";
            break;
        case 3:
            sep = " ";
            skip = Qt::SkipEmptyParts;
            break;
        case 2:
            sep = ",";
            break;
        default:
            sep = "\t";
    }
    if (!QFile::exists(fileedit->text()) ) {
        QMessageBox::critical ( this, "DibPunto", QString(tr("The file %1 not exist")).arg(fileedit->text()) );
        return;
    }
    QFile infile(fileedit->text());
    if (!infile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical ( this, "DibPunto", QString(tr("Can't open the file %1")).arg(fileedit->text()) );
        return;
    }

//Warning, can change adding or reordering "formatedit"
    if (formatedit->currentIndex() == 4)
        processFileODB(&infile, sep);
    else
        processFileNormal(&infile, sep, skip);
    infile.close ();
    QString currlay = doc->getCurrentLayer();

    if (_pt2d->checkOn())
        draw2D(doc);
    if (_pt3d->checkOn())
        draw3D(doc);
    if (_ptelev->checkOn())
        drawElev(doc);
    if (_ptnumber->checkOn())
        drawNumber(doc);
    if (_ptcode->checkOn())
        drawCode(doc);

    doc->setLayer(currlay);
    /* draw lines in current layer */
    if ( _connectPoints->isChecked() ) {
        drawLine(doc);
    }
}

void dibPunto::drawLine(IDocumentPlugin *doc)
{
    QPointF prevP, nextP;
    int i;

    for (i = 0; i < _dataList.size(); ++i) {
        pointData *pd = _dataList.at(i);
        if (!pd->x.isEmpty() && !pd->y.isEmpty()){
            prevP.setX(pd->x.toDouble());
            prevP.setY(pd->y.toDouble());
            break;
        }
    }
    for (; i < _dataList.size(); ++i) {
        pointData *pd = _dataList.at(i);
        if (!pd->x.isEmpty() && !pd->y.isEmpty()){
            nextP.setX(pd->x.toDouble());
            nextP.setY(pd->y.toDouble());
            doc->addLine(&prevP, &nextP);
            prevP = nextP;
        }
    }
}

void dibPunto::draw2D(IDocumentPlugin *doc)
{
    QPointF pt;
    doc->setLayer(_pt2d->getLayer());
    for (auto pd : _dataList) {
        if (!pd->x.isEmpty() && !pd->y.isEmpty()){
            pt.setX(pd->x.toDouble());
            pt.setY(pd->y.toDouble());
            doc->addPoint(&pt);
        }
    }
}
void dibPunto::draw3D(IDocumentPlugin *doc)
{
    QPointF pt;
    doc->setLayer(_pt3d->getLayer());
    for (auto pd : _dataList) {
        if (!pd->x.isEmpty() && !pd->y.isEmpty()){
            pt.setX(pd->x.toDouble());
            pt.setY(pd->y.toDouble());
/*RLZ:3d support            if (pd->z.isEmpty()) pt.setZ(0.0);
            else  pt.setZ(pd->z.toDouble());*/
            doc->addPoint(&pt);
        }
    }
}

dibPunto::CalcPosResult dibPunto::calcPos(double sep, DPT::txtposition sit)
{
    dibPunto::CalcPosResult result;

    result.x = result.y = sep;
    double inc = sqrt(result.x*result.x/2);
    switch (sit) {
        case DPT::NO:
            result.v = DPI::VAlignBottom;
            result.h = DPI::HAlignRight;
            result.x = -1.0*inc;
            result.y = inc;
            break;
        case DPT::O:
            result.v = DPI::VAlignMiddle;
            result.h = DPI::HAlignRight;
            result.x = -1.0*result.x;
            result.y = 0.0;
            break;
        case DPT::NE:
            result.v = DPI::VAlignBottom;
            result.h = DPI::HAlignLeft;
            result.x = inc;
            result.y = inc;
            break;
        case DPT::SO:
            result.v = DPI::VAlignTop;
            result.h = DPI::HAlignRight;
            result.x = -1.0*inc;
            result.y = -1.0*inc;
            break;
        case DPT::SE:
            result.v = DPI::VAlignTop;
            result.h = DPI::HAlignLeft;
            result.x = inc;
            result.y = -1.0*inc;
            break;
        case DPT::E:
            result.v = DPI::VAlignMiddle;
            result.h = DPI::HAlignLeft;
            result.y = 0.0;
            break;
        case DPT::S:
            result.v = DPI::VAlignMiddle;
            result.h = DPI::HAlignCenter;
            result.x = 0.0;
            result.y = -1.0*result.y;
            break;
        default: //N
            result.v = DPI::VAlignBottom;
            result.h = DPI::HAlignCenter;
            result.x = 0.0;
            break;
    }
    return result;
}


void dibPunto::drawNumber(IDocumentPlugin *doc)
{
    const auto pos = calcPos(_ptnumber->getSeparation(), _ptnumber->getPosition());
    doc->setLayer(_ptnumber->getLayer());
    QString sty = _ptnumber->getStyleStr();
    for (const auto pd : _dataList) {
        if (!pd->x.isEmpty() && !pd->y.isEmpty() && !pd->number.isEmpty()){
            const double new_x = pd->x.toDouble() + pos.x;
            const double new_y = pd->y.toDouble() + pos.y;
            QPointF pt(new_x, new_y);
            doc->addText(pd->number, sty, &pt, _ptnumber->getHeightStr().toDouble(), 0.0, pos.h, pos.v);
        }
    }
}

void dibPunto::drawElev(IDocumentPlugin *doc)
{
    const auto pos = calcPos(_ptelev->getSeparation(), _ptelev->getPosition());
    doc->setLayer(_ptelev->getLayer());
    QString sty = _ptelev->getStyleStr();
    for (const auto pd : _dataList) {
        if (!pd->x.isEmpty() && !pd->y.isEmpty() && !pd->z.isEmpty()){
            const double new_x = pd->x.toDouble() + pos.x;
            const double new_y = pd->y.toDouble() + pos.y;
            QPointF pt(new_x, new_y);
            doc->addText(pd->z, sty, &pt, _ptelev->getHeightStr().toDouble(), 0.0, pos.h, pos.v);
        }
    }
}
void dibPunto::drawCode(IDocumentPlugin *doc)
{
    auto pos = calcPos(_ptcode->getSeparation(), _ptcode->getPosition());
    doc->setLayer(_ptcode->getLayer());
    QString sty = _ptcode->getStyleStr();
    for (const auto& pd : _dataList) {
        if (!pd->x.isEmpty() && !pd->y.isEmpty() && !pd->code.isEmpty()){
            const double new_x = pd->x.toDouble() + pos.x;
            const double new_y = pd->y.toDouble() + pos.y;
            QPointF pt(new_x, new_y);
            doc->addText(pd->code, sty, &pt, _ptcode->getHeightStr().toDouble(), 0.0, pos.h, pos.v);
        }
    }
}

void dibPunto::processFileODB(QFile* file, const QString& sep)
{
    QStringList list;
    pointData *pd;

    while (!file->atEnd()) {
        QString line = file->readLine();
        line.remove ( line.size()-2, 1);
        list = line.split(sep);
        pd = new pointData;
        int i = 0;
        int j = list.size();
        if (i<j && list.at(i).compare("4") == 0 ){
            i = i+2;
            if (i<j) pd->x = list.at(i); else pd->x = QString();
            i++;
            if (i<j) pd->y = list.at(i); else pd->y = QString();
            i++;
            if (i<j) pd->z = list.at(i); else pd->z = QString();
            i++;
            if (i<j) pd->number = list.at(i); else pd->number = QString();
            i++;
            if (i<j) pd->code = list.at(i); else pd->code = QString();
        }
        _dataList.append(pd);
    }

}
void dibPunto::processFileNormal(QFile* file, const QString& sep, Qt::SplitBehavior skip)
{
    //    QString outname, sep;
    QStringList list;
    pointData *pd;
    while (!file->atEnd()) {
        QString line = file->readLine();
        if(line.isEmpty()) continue;
        line.remove ( line.size()-1, 1);
        list = line.split(sep, skip);
        pd = new pointData;
        int i = 0;
        switch(list.size()){
            case 0:
            case 1:
                delete pd;
                continue;

                //allow reading in raw 2D ascii list in format:
                // x y
            case 2:
                pd->x = list.at(0);
                pd->y = list.at(1);
                break;
            default:
            case 5:
                pd->code=list.at(4);
                // fall-through
            case 4:
                pd->z = list.at(3);
                // fall-through
            case 3:
                pd->number = list.at(i);
                pd->x = list.at(1);
                pd->y = list.at(2);
                break;
        }
        _dataList.append(pd);
    }
}

dibPunto::~dibPunto()
{
    while (!_dataList.isEmpty())
        delete _dataList.takeFirst();
}

void dibPunto::readSettings()
{
    QString str;
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LibreCAD", "asciifile");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(500,300)).toSize();
    str = settings.value("lastfile").toString();
    fileedit->setText(str);
    formatedit->setCurrentIndex( settings.value("format", 0).toInt() );
    _connectPoints->setChecked(settings.value("connectpoints", false).toBool() );
    _pt2d->setCheck(settings.value("draw2d", false).toBool() );
    str = settings.value("layer2d").toString();
    _pt2d->setLayer(str);
    _pt3d->setCheck(settings.value("draw3d", false).toBool() );
    str = settings.value("layer3d").toString();
    _pt3d->setLayer(str);
    _ptelev->setCheck(settings.value("drawelev", false).toBool() );
    str = settings.value("layerelev").toString();
    _ptelev->setLayer(str);
    _ptnumber->setCheck(settings.value("drawnumber", false).toBool() );
    str = settings.value("layernumber").toString();
    _ptnumber->setLayer(str);
    _ptcode->setCheck(settings.value("drawcode", false).toBool() );
    str = settings.value("layercode").toString();
    _ptcode->setLayer(str);
    _ptelev->setStyleIdx(settings.value("styleelev", 0).toInt() );
    _ptnumber->setStyleIdx(settings.value("stylenumber", 0).toInt() );
    _ptcode->setStyleIdx(settings.value("stylecode", 0).toInt() );
    _ptelev->setHeight(settings.value("heightelev", 0.5).toDouble() );
    _ptnumber->setHeight(settings.value("heightnumber", 0.5).toDouble() );
    _ptcode->setHeight(settings.value("heightcode", 0.5).toDouble() );
    _ptelev->setSeparation(settings.value("separationelev", 0.3).toDouble() );
    _ptnumber->setSeparation(settings.value("separationnumber", 0.3).toDouble() );
    _ptcode->setSeparation(settings.value("separationcode", 0.3).toDouble() );
    _ptelev->setPosition(static_cast<DPT::txtposition>( settings.value("positionelev", DPT::S).toInt() ) );
    _ptnumber->setPosition(static_cast<DPT::txtposition>( settings.value("positionnumber", DPT::N).toInt() ) );
    _ptcode->setPosition(static_cast<DPT::txtposition>( settings.value("positioncode", DPT::E).toInt() ) );
    resize(size);
    move(pos);
}

void dibPunto::writeSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LibreCAD", "asciifile");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("lastfile", fileedit->text());
    settings.setValue("format", formatedit->currentIndex());
    settings.setValue("draw2d", _pt2d->checkOn());
    settings.setValue("draw3d", _pt3d->checkOn());
    settings.setValue("drawelev", _ptelev->checkOn());
    settings.setValue("drawnumber", _ptnumber->checkOn());
    settings.setValue("drawcode", _ptcode->checkOn());
    settings.setValue("connectpoints", _connectPoints->isChecked());
    settings.setValue("layer2d", _pt2d->getLayer());
    settings.setValue("layer3d", _pt3d->getLayer());
    settings.setValue("layerelev", _ptelev->getLayer());
    settings.setValue("layernumber", _ptnumber->getLayer());
    settings.setValue("layercode", _ptcode->getLayer());
    settings.setValue("styleelev", _ptelev->getStyleIdx());
    settings.setValue("stylenumber", _ptnumber->getStyleIdx());
    settings.setValue("stylecode", _ptcode->getStyleIdx());
    settings.setValue("heightelev", _ptelev->getHeightStr());
    settings.setValue("heightnumber", _ptnumber->getHeightStr());
    settings.setValue("heightcode", _ptcode->getHeightStr());
    settings.setValue("separationelev", _ptelev->getSeparationStr());
    settings.setValue("separationnumber", _ptnumber->getSeparationStr());
    settings.setValue("separationcode", _ptcode->getSeparationStr());
    settings.setValue("positionelev", _ptelev->getPosition());
    settings.setValue("positionnumber", _ptnumber->getPosition());
    settings.setValue("positioncode", _ptcode->getPosition());
}
