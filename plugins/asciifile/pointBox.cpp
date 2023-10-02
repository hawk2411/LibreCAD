//
// Created by hawk on 27.09.2023.
//
#include <QLabel>
#include <QLineEdit>

#include "pointBox.h"
/*****************************/
pointBox::pointBox(const QString & title, const QString & label, QWidget * parent ) :
        QGroupBox(title, parent)
{
    rb = new QCheckBox(label);
    rb->setTristate (false );
    vbox = new QVBoxLayout;
    vbox->addWidget(rb);
    QLabel *but = new QLabel(tr("Layer:"));
    layedit = new QLineEdit();
    QHBoxLayout *lolayer = new QHBoxLayout;
    lolayer->addWidget(but);
    lolayer->addWidget(layedit);
    vbox->addLayout(lolayer);
    setLayout(vbox);
}

void pointBox::setInLayout(QLayout *lo)
{
    vbox->addLayout(lo);
}
