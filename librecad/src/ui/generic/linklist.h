#ifndef LINKLIST_H
#define LINKLIST_H

#include <QListWidget>

class LinkList : public QListWidget
{
    Q_OBJECT

public:
    explicit LinkList(QWidget* parent);

    void addLink(const QString& text, const QString& url);

protected slots:
    static void showWebPage(QListWidgetItem*);
};

#endif // LINKLIST_H
