#ifndef DEL_TREEVIEW_H
#define DEL_TREEVIEW_H

#include <QtGui>
#include <QStyledItemDelegate>
#include <QLabel>
#include <QDebug>
#include "settings.h"

class del_treeview : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit del_treeview(QObject *parent = 0) : QStyledItemDelegate(parent) {}

    QString sport;

    void paint( QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
    {
        painter->save();
        const QAbstractItemModel *model = index.model();
        QString lapName = model->data(model->index(index.row(),0,QModelIndex())).toString().trimmed();
        QString breakName = settings::get_generalValue("breakname");
        //bool avgSelect = index.model()->data(index.model()->index(index.row(),index.model()->columnCount()-1,QModelIndex()),(Qt::UserRole+1)).toBool();

        QLinearGradient setGradient(option.rect.topLeft(),option.rect.bottomLeft());
        setGradient.setColorAt(0,QColor(255,255,255,100));
        setGradient.setSpread(QGradient::RepeatSpread);

        QColor setColor;
        QColor breakColor = settings::get_itemColor(breakName);

        if(option.state & QStyle::State_Selected)
        {
            setColor.setRgb(0,255,0,100);
            setGradient.setColorAt(1,setColor);
            painter->fillRect(option.rect,setGradient);
        }
        else
        {
            if(index.parent().isValid())
            {
                if(index.row() % 2 == 0)
                {
                    setColor.setRgb(200,200,200,75);
                }
                else
                {
                    setColor.setRgb(175,175,175,75);
                }
                painter->setPen(Qt::black);
            }
            else
            {
                if(lapName == breakName)
                {
                    setColor = breakColor;
                    setColor.setAlpha(125);
                    painter->setPen(Qt::white);
                }
                else
                {
                    if(index.row() % 2 == 0)
                    {
                        setColor.setRgb(175,175,175,75);
                    }
                    else
                    {
                        setColor.setRgb(150,150,150,75);
                    }
                    painter->setPen(Qt::black);
                }
            }
            painter->fillRect(option.rect,QBrush(setColor));
        }

        QRect rect_text(option.rect.x()+2,option.rect.y(), option.rect.width(),option.rect.height());
        painter->drawText(rect_text,index.data().toString(),QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
        //painter->setFont(cFont);
        painter->restore();
    }

};












#endif // DEL_TREEVIEW_H
