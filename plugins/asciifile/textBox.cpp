//
// Created by hawk on 27.09.2023.
//

#include <QDoubleValidator>
#include <QLineEdit>
#include <QFormLayout>

#include "textBox.h"

textBox::textBox(const QString & title, const QString & label, QWidget * parent) :
        pointBox(title, label, parent)
{
    combostyle = new QComboBox();
    QStringList txtstyles;
    txtstyles << "txt" << "simplex" << "romans";
    combostyle->addItems(txtstyles);
    auto val = new QDoubleValidator(0);
    val->setBottom ( 0.0 );
    heightedit = new QLineEdit();
    heightedit->setValidator(val);
    sepedit = new QLineEdit();
    sepedit->setValidator(val);

    auto *flo = new QFormLayout;
    flo->addRow( tr("Style:"), combostyle);
    flo->addRow( tr("Height:"), heightedit);
    flo->addRow( tr("Separation"), sepedit);
//    posimage.fill(Qt::black);
    img = new imgLabel();
    auto *loimage = new QHBoxLayout;
    loimage->addLayout(flo);
    loimage->addWidget(img);

    setInLayout(loimage);
}

