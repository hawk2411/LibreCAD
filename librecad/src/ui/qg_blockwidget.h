/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
**
**
** This file is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/

#ifndef QG_BLOCKWIDGET_H
#define QG_BLOCKWIDGET_H

#include <QWidget>
#include <QIcon>
#include <QAbstractTableModel>
#include <QItemSelection>

#include "rs_blocklistlistener.h"

class QG_ActionHandler;
class QTableView;
class QLineEdit;


/**
 * Implementation of a model to use in QG_BlockWidget
 */
class QG_BlockModel: public QAbstractTableModel {
public:
    enum {
        VISIBLE,
        NAME,
        LAST
    };
    explicit QG_BlockModel(QObject * parent = nullptr);
	~QG_BlockModel() override = default;
    [[nodiscard]] Qt::ItemFlags flags ( const QModelIndex & /*index*/ ) const override {
            return Qt::ItemIsSelectable|Qt::ItemIsEnabled;}
    [[nodiscard]] int columnCount(const QModelIndex &/*parent*/) const override {return LAST;}
    [[nodiscard]] int rowCount ( const QModelIndex & parent ) const override;
    [[nodiscard]] QVariant data ( const QModelIndex & index, int role ) const override;
    [[nodiscard]] QModelIndex parent ( const QModelIndex & index ) const override;
    [[nodiscard]] QModelIndex index ( int row, int column, const QModelIndex & parent ) const override;
    void setBlockList(RS_BlockList* bl);
    RS_Block *getBlock( int row );
    QModelIndex getIndex (RS_Block * blk);
    void setActiveBlock(RS_Block* b) { activeBlock = b; };

private:
    QList<RS_Block*> listBlock;
    QIcon blockVisible;
    QIcon blockHidden;
    RS_Block* activeBlock {nullptr};
};


/**
 * This is the Qt implementation of a widget which can view a 
 * block list.
 */
class QG_BlockWidget: public QWidget, public RS_BlockListListener {
    Q_OBJECT

public:
    QG_BlockWidget(QG_ActionHandler* ah, QWidget* parent,
                   const char* name=nullptr);
    ~QG_BlockWidget() override;

    void setBlockList(RS_BlockList* blockList) {
        this->_blockList = blockList;
        updateBlock();
    }

    RS_BlockList* getBlockList() {
        return _blockList;
    }

    void updateBlock();
    void activateBlock(RS_Block* block);

    void blockAdded(RS_Block*) override;

    void blockEdited(RS_Block*) override {
        updateBlock();
    }
    void blockRemoved(RS_Block*) override {
        updateBlock();
    }
    void blockToggled(RS_Block*) override {
        updateBlock();
	}

signals:
	void escape();

public slots:
    void slotActivated(QModelIndex blockIdx);
    void slotSelectionChanged(
        const QItemSelection &selected,
        const QItemSelection &deselected);
    void slotUpdateBlockList();

protected:
    void contextMenuEvent(QContextMenuEvent *e) override;
	void keyPressEvent(QKeyEvent* e) override;

private:
    RS_BlockList* _blockList;
    QLineEdit* _matchBlockName;
    QTableView* _blockView;
    std::unique_ptr<QG_BlockModel> _blockModel;
    RS_Block* _lastBlock;

    QG_ActionHandler* _actionHandler;

    void restoreSelections();
};

#endif
