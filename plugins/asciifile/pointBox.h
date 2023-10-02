//
// Created by hawk on 27.09.2023.
//

#ifndef LIBRECAD_POINTBOX_H
#define LIBRECAD_POINTBOX_H

#include <QGroupBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QLineEdit>
/***********/
class pointBox : public QGroupBox
{
    Q_OBJECT

public:
    pointBox(const QString & title, const QString & label, QWidget * parent = 0 );
    ~pointBox() override = default;
    void setInLayout(QLayout *lo);
    bool checkOn() { return rb->isChecked();}
    void setCheck(bool val) { rb->setChecked(val);}
    QString getLayer() { return layedit->text();}
    void setLayer(QString l) { layedit->setText(l);}
private:
    QCheckBox *rb;
    QLineEdit *layedit;
    QVBoxLayout *vbox;
};


#endif //LIBRECAD_POINTBOX_H
