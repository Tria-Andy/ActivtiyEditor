#ifndef DEL_AVGSELECT_H
#define DEL_AVGSELECT_H

#include <QtGui>
#include <QItemDelegate>
#include <QLabel>
#include <QDebug>
#include "settings.h"


class del_avgselect : public QItemDelegate
{
    Q_OBJECT

private:
    QColor get_backcolor(bool isSel,QString name) const
    {
        if(isSel)
        {
            return QColor(Qt::green);
        }
        else
        {
            if(name == settings::get_generalValue("breakname"))
            {
                return settings::get_itemColor(settings::get_generalValue("breakname"));
            }
            else
            {
                return QColor(Qt::lightGray);
            }
        }
    }

    bool get_isSelected(const QModelIndex &index) const
    {
        return index.model()->data(index.model()->index(index.row(),index.model()->columnCount()-1,QModelIndex()),(Qt::UserRole+1)).toBool();
    }

    QString get_lapName(const QModelIndex &index) const
    {
        return index.model()->data(index.model()->index(index.row(),0,QModelIndex())).toString().trimmed();
    }

public:
    explicit del_avgselect(QObject *parent = 0) : QItemDelegate(parent) {}

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        drawBackground(painter, option, index);
        QItemDelegate::paint(painter, option, index);
    }

protected:
    virtual void drawBackground(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        painter->save();
        bool isSelected = del_avgselect::get_isSelected(index);
        QString lapName = del_avgselect::get_lapName(index);

        QRect rect_text(option.rect.x()+2,option.rect.y(), option.rect.width(),option.rect.height());
        painter->drawText(rect_text,index.data().toString(),QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
        painter->fillRect(option.rect,del_avgselect::get_backcolor(isSelected,lapName));
        painter->restore();
    }
};

#endif // DEL_AVGSELECT_H
