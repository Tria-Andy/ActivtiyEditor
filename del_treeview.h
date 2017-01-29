#ifndef DEL_TREEVIEW_H
#define DEL_TREEVIEW_H

#include <QtGui>
#include <QItemDelegate>
#include <QLabel>
#include <QDebug>
#include "settings.h"

class del_treeview : public QItemDelegate
{
    Q_OBJECT

public:
    explicit del_treeview(QObject *parent = 0) : QItemDelegate(parent) {}

    QString sport;

    void paint( QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
    {
        painter->save();
        //QFont cFont;
        QString lapName;
        QString breakName = settings::get_generalValue("breakname");
        const QAbstractItemModel *model = index.model();
        //cFont.setPixelSize(12);
        QColor intColor,lapColor;
        QColor breakColor = settings::get_itemColor(breakName);

        if(sport == settings::isSwim)
        {
                intColor.setRgb(125,125,125,75);
                if(index.row() % 2 == 0)
                {
                    lapColor.setRgb(215,215,215,75);
                }
                else
                {
                    lapColor.setRgb(255,0,0,75);
                }
        }
        else
        {
            if(index.row() % 2 == 0)
            {
                intColor.setRgb(125,125,125,75);
            }
            else
            {
                intColor.setRgb(175,175,175,75);
            }
        }


        QRect rect_text(option.rect.x()+2,option.rect.y(), option.rect.width(),option.rect.height());
        lapName = model->data(model->index(index.row(),0,QModelIndex())).toString().trimmed();
        painter->setPen(Qt::black);
        painter->drawText(rect_text,index.data().toString(),QTextOption(Qt::AlignLeft | Qt::AlignVCenter));

        if(sport == settings::isSwim)
        {
            if(lapName == breakName)
            {
                painter->fillRect(option.rect,QBrush(breakColor));
            }
            else
            {
                if(model->index(index.row(),0).parent().isValid())
                {
                    qDebug() << index.row() << index.parent().row() << index.data().toString();
                    painter->fillRect(option.rect,QBrush(lapColor));
                }
                else
                {
                    painter->fillRect(option.rect,QBrush(intColor));
                }
            }
        }
        else
        {
            painter->fillRect(option.rect,QBrush(intColor));
        }

        painter->drawText(rect_text,index.data().toString(),QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
        //painter->setFont(cFont);
        painter->restore();
    }

};












#endif // DEL_TREEVIEW_H
