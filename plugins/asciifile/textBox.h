//
// Created by hawk on 27.09.2023.
//

#ifndef LIBRECAD_TEXTBOX_H
#define LIBRECAD_TEXTBOX_H

#include <QComboBox>
#include "pointBox.h"
#include "imgLabel.h"
#include "dpt.h"

class textBox : public pointBox
{
    Q_OBJECT

public:
    textBox(const QString & title, const QString & label, QWidget * parent = 0 );
    ~textBox() override = default;
    void setPos(DPT::txtposition p) { img->setPos(p); }
    QString getStyleStr() { return combostyle->currentText();}
    void setStyleIdx(int idx) { combostyle->setCurrentIndex(idx);}
    int getStyleIdx() { return combostyle->currentIndex();}
    void setHeight(double height) { heightedit->setText(QString::number(height, 'f'));}
//    double getHeight();
    QString getHeightStr() { return heightedit->text();}
    double getHeight() { return heightedit->text().toDouble();}
    void setSeparation(double separation) { sepedit->setText(QString::number(separation, 'f'));}
    QString getSeparationStr() { return sepedit->text();}
    double getSeparation() { return sepedit->text().toDouble();}
    void setPosition(DPT::txtposition p) { img->setPos(p);}
    DPT::txtposition getPosition() { return img->getPos();}

private:
    QComboBox *combostyle;
    QLineEdit *heightedit;
    QLineEdit *sepedit;
    imgLabel *img;
};


#endif //LIBRECAD_TEXTBOX_H
