//
// Created by hawk on 27.09.2023.
//

#ifndef LIBRECAD_IMGLABEL_H
#define LIBRECAD_IMGLABEL_H

#include <QLabel>
#include "dpt.h"

class imgLabel : public QLabel
{
    Q_OBJECT

public:
    explicit imgLabel(QWidget * parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags() );
    ~imgLabel() override = default;

    void setPos(DPT::txtposition pos = DPT::N);
    DPT::txtposition getPos() { return currPos;}

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void drawImage();
    void changePos(int x, int y);

private:
    QPicture *posimage;
    DPT::txtposition currPos;
};


#endif //LIBRECAD_IMGLABEL_H
