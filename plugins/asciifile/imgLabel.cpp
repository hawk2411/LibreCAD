//
// Created by hawk on 27.09.2023.
//
#include <QPicture>
#include <QPainter>
#include <QMouseEvent>

#include "imgLabel.h"
#define POINT 12

imgLabel::imgLabel(QWidget * parent, Qt::WindowFlags f) :
        QLabel(parent, f)
{
    posimage = new QPicture;
    posimage->setBoundingRect(QRect(0,0,POINT*8,POINT*8));
    currPos = DPT::N;
    drawImage();
    setPicture(*posimage);
}

void imgLabel::drawImage()
{
    int a1, a2, a3, a4;
    int b1, b2, b3, b4;
    QPainter painter;
    painter.begin(posimage);
    painter.fillRect ( 0, 0, POINT*8,POINT*8, Qt::black );
    a1 = POINT*1.75;
    a2 = POINT*3.5;
    a3 = POINT*5.25;
    a4 = POINT*6;
    painter.fillRect ( a1, a1, POINT, POINT, Qt::white );//NO
    painter.fillRect ( a2, POINT, POINT, POINT, Qt::white );//N
    painter.fillRect ( POINT, a2, POINT, POINT, Qt::white );//O
    painter.fillRect ( a3, a1, POINT, POINT, Qt::white );//NE
    painter.fillRect ( a1, a3, POINT, POINT, Qt::white );//SO
    painter.fillRect ( a3, a3, POINT, POINT, Qt::white );//SE
    painter.fillRect ( a4, a2, POINT, POINT, Qt::white );//E
    painter.fillRect ( a2, a4, POINT, POINT, Qt::white );//S
    painter.setPen ( Qt::white );
    b1 = POINT*3.2;
    b2 = POINT*3.6;
    b3 = POINT*4;
    b4 = POINT*4.4;
    painter.drawLine ( b2, b2, b4, b2 );
    painter.drawLine ( b2, b2, b2, b4 );
    painter.drawLine ( b4, b2, b4, b4 );
    painter.drawLine ( b2, b4, b4, b4 );
    b4 = POINT*4.8;
    painter.drawLine ( b1, b3, b4, b3 );
    painter.drawLine ( b3, b1, b3, b4 );

    switch (currPos) {
        case DPT::NO:
            a2 = a1 = POINT*1.75;
            break;
        case DPT::O:
            a1 = POINT;
            a2 = POINT*3.5;
            break;
        case DPT::NE:
            a1 = POINT*5.25;
            a2 = POINT*1.75;
            break;
        case DPT::SO:
            a1 = POINT*1.75;
            a2 = POINT*5.25;
            break;
        case DPT::SE:
            a2 = a1 = POINT*5.25;
            break;
        case DPT::E:
            a1 = POINT*6;
            a2 = POINT*3.5;
            break;
        case DPT::S:
            a1 = POINT*3.5;
            a2 = POINT*6;
            break;
        default: //N
            a1 = POINT*3.5;
            a2 = POINT;
    }
    painter.fillRect ( a1, a2, POINT, POINT, Qt::red );
    painter.end();
    update ();
}

void imgLabel::changePos(int x, int y)
{
    if (x < POINT*3.1) {
        if (y < POINT*3.1) { setPos(DPT::NO); }
        else if (y < POINT*4.9) { setPos(DPT::O); }
        else { setPos(DPT::SO); }

    } else if (x < POINT*4.9) {
        if (y < POINT*4) { setPos(DPT::N); }
        else { setPos(DPT::S); }

    } else {
        if (y < POINT*3.1) { setPos(DPT::NE); }
        else if (y < POINT*4.9) { setPos(DPT::E); }
        else { setPos(DPT::SE); }
    }
}

void imgLabel::setPos(DPT::txtposition pos)
{
    currPos = pos;
    drawImage();
}

void imgLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        changePos(event->x(), event->y());
    } else {
        QLabel::mousePressEvent(event);
    }
}
