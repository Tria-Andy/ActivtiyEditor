/*
 * Copyright (c) 2016 Andreas Hunner (andy-atech@gmx.net)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "activity.h"
#include <math.h>
#include <QDebug>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>

Activity::Activity()
{
    zone_count = settings::get_listValues("Level").count();
    changeRowCount = false;
    isUpdated = false;
    intTreeModel = new QStandardItemModel;
    selItemModel = new QStandardItemModel;
    avgModel = new QStandardItemModel;
    avgModel->setColumnCount(2);
    itemHeader.insert(0,QStringList() << "Name:" << "Type:" << "Distance (M):" << "Duration (Sec):" << "Pace/100m:" << "Speed km/h:" << "Strokes:");
    itemHeader.insert(1,QStringList() << "Name:" << "Start:" << "Distance:" << "Duration:" << "Pace:" << "Speed km/h:");
}

void Activity::act_reset()
{
    changeRowCount = false;
}

void Activity::prepareData()
{
    int sampCount = samp_model->rowCount();
    int intModelCount = int_model->rowCount();
    int lapCount = 1;
    int lapKey = 1;
    int intCounter = 1;
    int breakCounter = 1;
    int dist = 0;
    int lapIdent = 0;
    QString breakName = settings::get_generalValue("breakname");
    QString lapName;
    sampSpeed.resize(sampCount);
    for(int i = 0; i < sampCount; ++i)
    {
        sampSpeed[i] = samp_model->data(samp_model->index(i,2,QModelIndex())).toDouble();
    }

    int_model->setData(int_model->index(intModelCount-1,2,QModelIndex()),sampCount-1);

    for(int row = 0; row < intModelCount; ++row)
    {
        if(curr_sport == settings::isSwim)
        {
            dist_factor = 1000;
            lapCount = this->get_swim_laps(row);
            dist = lapCount*swim_track;
            int_model->setData(int_model->index(row,3),dist);
            int_model->setData(int_model->index(row,4),lapCount);
            if(lapCount > 0)
            {
                lapIdent = lapCount == 1 ? 1 : 0;
                int_model->setData(int_model->index(row,5),"Int-"+QString::number(lapKey)+"_"+QString::number(lapIdent));
            }
            else
            {
                int_model->setData(int_model->index(row,5),breakName+"_"+QString::number(breakCounter));
                ++lapKey;
                ++breakCounter;
            }

            if(row == 0)
            {
                lapName = QString::number(intCounter)+"_Warmup_"+QString::number(dist);
                ++intCounter;
            }
            else if(row != 0 && row < intModelCount-1)
            {
                if(lapCount > 0)
                {
                    lapName = QString::number(intCounter)+"_Int_"+QString::number(dist);
                    ++intCounter;
                }
                else
                {
                    lapName = breakName;
                }
            }
            else if(row == intModelCount-1)
            {
                lapName = QString::number(intCounter)+"_Cooldown_"+QString::number(dist);
            }

            int_model->setData(int_model->index(row,0),lapName);
        }
        else
        {
            dist_factor = 1;
            int_model->setData(int_model->index(row,3),this->get_int_distance(row));
            int_model->setData(int_model->index(row,4),lapCount);
            int_model->setData(int_model->index(row,5),"Int-"+QString::number(row));
        }
    }

    if(curr_sport == settings::isSwim)
    {
        QString swimLapName,lapKey;          
        int rowCounter = xdata_model->rowCount();
        double swimoffset = 0.0;
        if(isUpdated) swimoffset = settings::get_thresValue("swimoffset");

        swim_xdata = new QStandardItemModel(rowCounter,8);
        swim_xdata->setHorizontalHeaderLabels(settings::get_swimtime_header());
        swimType = settings::get_listValues("SwimStyle");

        int lapNr = 0;
        int intCount = 1;
        int type = 0, typePrev = 0;
        double lapStart,lapStartPrev = 0,lapTimePrev = 0;
        double lapSpeed,lapTime;

        for(int row = 0; row < rowCounter; ++row)
        {
            lapTime = round(xdata_model->data(xdata_model->index(row,3,QModelIndex())).toDouble() - swimoffset);
            type = xdata_model->data(xdata_model->index(row,2,QModelIndex())).toInt();

            if(lapTime > 0 && type != 0)
            {
                ++lapNr;
                swimLapName = QString::number(intCount)+"_"+QString::number(lapNr*swim_track);
                lapKey = "Int-"+QString::number(intCount)+"_"+QString::number(lapNr);
                swim_xdata->setData(swim_xdata->index(row,0,QModelIndex()),swimLapName);

                if(typePrev == 0)
                {
                    lapStart = xdata_model->data(xdata_model->index(row,0,QModelIndex())).toDouble();
                    int breakTime = lapStart - lapStartPrev;
                    swim_xdata->setData(swim_xdata->index(row-1,2,QModelIndex()),breakTime);
                }
                else
                {
                    lapStart = lapStartPrev + lapTimePrev;
                }
                if(row == 1) lapStart = lapTimePrev-1;
                lapSpeed = this->calcSpeed(lapTime,swim_track,dist_factor);
                //lapSpeed = this->get_speed(QTime::fromString(this->set_time(lapTime),"mm:ss"),swim_track,settings::isSwim,false);
            }
            else
            {
                lapNr = 0;
                swim_xdata->setData(swim_xdata->index(row,0,QModelIndex()),breakName);
                lapKey = breakName;
                lapStart = lapStartPrev + lapTimePrev;
                lapSpeed = 0;
                ++intCount;
            }

            swim_xdata->setData(swim_xdata->index(row,1,QModelIndex()),type);
            swim_xdata->setData(swim_xdata->index(row,2,QModelIndex()),swim_track);
            swim_xdata->setData(swim_xdata->index(row,3,QModelIndex()),lapTime);
            swim_xdata->setData(swim_xdata->index(row,4,QModelIndex()),lapTime * (100.0/swim_track));
            swim_xdata->setData(swim_xdata->index(row,5,QModelIndex()),lapSpeed);
            swim_xdata->setData(swim_xdata->index(row,6,QModelIndex()),xdata_model->data(xdata_model->index(row,4,QModelIndex())));
            swim_xdata->setData(swim_xdata->index(row,7,QModelIndex()),lapKey);

            lapTimePrev = lapTime;
            lapStartPrev = lapStart;
            typePrev = type;
        }
        swimProxy = new QSortFilterProxyModel;
        swimProxy->setSourceModel(swim_xdata);

        xdata_model->clear();
        delete xdata_model;
    }

    ride_info.insert("Distance:",QString::number(samp_model->data(samp_model->index(sampCount-1,1,QModelIndex())).toDouble()));
    ride_info.insert("Duration:",QDateTime::fromTime_t(sampCount).toUTC().toString("hh:mm:ss"));

    if(curr_sport == settings::isBike)
    {
        avgValues.resize(5);
    }
    else
    {
        avgValues.resize(3);
    }
    avgModel->setRowCount(avgValues.count());
    this->reset_avg();

    if(curr_sport == settings::isSwim)
    {
        p_swim_timezone.resize(zone_count+1);
        p_swim_time.resize(zone_count+1);
        p_hf_timezone.resize(zone_count);
        hf_zone_avg.resize(zone_count);
        hf_avg = 0;
        move_time = 0.0;

        QStringList pace_header,hf_header;
        pace_header << "Zone" << "Low (min/100m)" << "High (min/100m)" << "Time in Zone";
        hf_header << "Zone" << "Low (1/Min)" << "High (1/min)" << "Time in Zone";

        //Set Tableinfos
        swim_pace_model = new QStandardItemModel(zone_count,4);
        swim_pace_model->setHorizontalHeaderLabels(pace_header);

        swim_hf_model = new QStandardItemModel(zone_count,4);
        swim_hf_model->setHorizontalHeaderLabels(hf_header);

        //Read current CV and HF Threshold
        double temp_cv = settings::get_thresValue("swimpace");
        swim_cv = (3600.0 / temp_cv) / 10.0;
        pace_cv = temp_cv;

        this->set_swim_data();
    }
    this->build_intTree();
}

void Activity::set_additional_ride_info()
{
    if(settings::get_act_isrecalc())
    {
        ride_info.insert("Distance:",QString::number(edit_samp_model->data(edit_samp_model->index(edit_samp_model->rowCount()-1,1,QModelIndex())).toDouble()));
        ride_info.insert("Duration:",QDateTime::fromTime_t(edit_samp_model->rowCount()).toUTC().toString("hh:mm:ss"));
    }
    else
    {
        ride_info.insert("Distance:",QString::number(samp_model->data(samp_model->index(samp_model->rowCount()-1,1,QModelIndex())).toDouble()));
        ride_info.insert("Duration:",QDateTime::fromTime_t(samp_model->rowCount()).toUTC().toString("hh:mm:ss"));
    }
}

void Activity::build_intTree()
{
    QStandardItem *rootItem = intTreeModel->invisibleRootItem();
    if(curr_sport == settings::isSwim)
    {
        QString intKey;
        for(int i = 0; i < int_model->rowCount(); ++i)
        {
            intKey = int_model->data(int_model->index(i,5)).toString().split("_").first();
            rootItem->appendRow(setSwimLap(i,intKey));
        }
    }
    else
    {
        for(int i = 0; i < int_model->rowCount(); ++i)
        {
            rootItem->appendRow(setIntRow(i));
        }
    }
    intTreeModel->setHorizontalHeaderLabels(settings::get_int_header(curr_sport));
}

QList<QStandardItem *> Activity::setIntRow(int pInt)
{
    QList<QStandardItem*> intItems;
    QModelIndex data_index = samp_model->index(int_model->data(int_model->index(pInt,2)).toInt()-1,1);

    intItems << new QStandardItem(int_model->data(int_model->index(pInt,0)).toString());
    intItems << new QStandardItem(this->set_time(this->get_int_duration(pInt)));
    intItems << new QStandardItem(this->set_time(int_model->data(int_model->index(pInt,1)).toInt()));
    intItems << new QStandardItem(QString::number(this->set_doubleValue(samp_model->data(data_index).toDouble(),true)));
    intItems << new QStandardItem(QString::number(this->set_doubleValue(this->get_int_distance(pInt),true)));
    intItems << new QStandardItem(this->set_time(this->get_int_pace(pInt)));
    intItems << new QStandardItem(QString::number(this->set_doubleValue(this->get_int_speed(pInt),false)));

    if(curr_sport == settings::isBike)
    {
        intItems << new QStandardItem(QString::number(round(this->get_int_value(pInt,4,false))));
        intItems << new QStandardItem(QString::number(round(this->get_int_value(pInt,3,false))));
    }
    intItems << new QStandardItem("-");

    return intItems;
}

QList<QStandardItem *> Activity::setSwimLap(int pInt,QString intKey)
{
    QList<QStandardItem*> intItems,subItems;
    int strokeCount = 0;
    int currType = 0,lastType;
    int lapType = 0;
    int lapCount = int_model->data(int_model->index(pInt,4)).toInt();

    swimProxy->setFilterRegExp("^"+intKey+"_");
    swimProxy->setFilterKeyColumn(7);

    intItems << new QStandardItem(int_model->data(int_model->index(pInt,0)).toString());
    intItems << new QStandardItem(swimType.at(currType));
    intItems << new QStandardItem(int_model->data(int_model->index(pInt,4)).toString());
    intItems << new QStandardItem(int_model->data(int_model->index(pInt,3)).toString());
    intItems << new QStandardItem(this->set_time(this->get_int_duration(pInt)));
    intItems << new QStandardItem(this->set_time(int_model->data(int_model->index(pInt,1)).toInt()));
    intItems << new QStandardItem(this->set_time(this->get_int_pace(pInt)));
    intItems << new QStandardItem(QString::number(this->get_int_speed(pInt)));
    intItems << new QStandardItem(QString::number(strokeCount));
    intItems << new QStandardItem("-");

    if(lapCount > 1)
    {
        for(int i = 0; i < swimProxy->rowCount();++i)
        {
            currType = swimProxy->data(swimProxy->index(i,1)).toInt();
            lapType = currType == lastType ? currType : 6;
            strokeCount = strokeCount+swimProxy->data(swimProxy->index(i,6)).toInt();
            subItems << new QStandardItem(swimProxy->data(swimProxy->index(i,0)).toString());
            subItems << new QStandardItem(swimType.at(currType));
            subItems << new QStandardItem("-");
            subItems << new QStandardItem(swimProxy->data(swimProxy->index(i,2)).toString());
            subItems << new QStandardItem(this->set_time(swimProxy->data(swimProxy->index(i,3)).toInt()));
            subItems << new QStandardItem("-");
            subItems << new QStandardItem(this->set_time(swimProxy->data(swimProxy->index(i,4)).toInt()));
            subItems << new QStandardItem(QString::number(swimProxy->data(swimProxy->index(i,5)).toDouble()));
            subItems << new QStandardItem(swimProxy->data(swimProxy->index(i,6)).toString());

            lastType = currType;
            intItems.first()->appendRow(subItems);
            subItems.clear();
        }
        intItems.at(1)->setData(swimType.at(lapType),Qt::EditRole);
        intItems.at(8)->setData(strokeCount,Qt::EditRole);
    }
    else
    {
        intItems.at(1)->setData(swimType.at(swimProxy->data(swimProxy->index(0,1)).toInt()),Qt::EditRole);
        intItems.at(8)->setData(swimProxy->data(swimProxy->index(0,6)).toString(),Qt::EditRole);
    }

    return intItems;
}

void Activity::showSwimLap(bool isInt)
{
    selectInt = isInt;
    selItemModel->setColumnCount(2);
    int rowCount = 0;

    if(isInt)
    {
        int intStart,dura;
        double dist;
        rowCount = 6;
        selItemModel->setRowCount(rowCount);
        intStart = this->get_timesec(intTreeModel->data(selItem.value(5)).toString());
        dura = this->get_timesec(intTreeModel->data(selItem.value(4)).toString());
        dist = intTreeModel->data(selItem.value(3)).toDouble();

        selItemModel->setData(selItemModel->index(0,1),intTreeModel->data(selItem.value(0)).toString());
        selItemModel->setData(selItemModel->index(1,1),intStart);
        selItemModel->setData(selItemModel->index(2,1),dist);
        selItemModel->setData(selItemModel->index(3,1),intTreeModel->data(selItem.value(4)).toString());
        selItemModel->setData(selItemModel->index(4,1),this->set_time(this->calc_lapPace(curr_sport,dura,dist)));
        selItemModel->setData(selItemModel->index(5,1),intTreeModel->data(selItem.value(7)).toDouble());
    }
    else
    {
        rowCount = 7;
        selItemModel->setRowCount(rowCount);
        selItemModel->setData(selItemModel->index(0,1), intTreeModel->data(selItem.value(0)).toString());
        selItemModel->setData(selItemModel->index(1,1), intTreeModel->data(selItem.value(1)).toString());
        selItemModel->setData(selItemModel->index(2,1), intTreeModel->data(selItem.value(3)).toString());
        selItemModel->setData(selItemModel->index(3,1), this->get_timesec(intTreeModel->data(selItem.value(4)).toString()));
        selItemModel->setData(selItemModel->index(4,1), intTreeModel->data(selItem.value(6)).toString());
        selItemModel->setData(selItemModel->index(5,1), intTreeModel->data(selItem.value(7)).toDouble());
        selItemModel->setData(selItemModel->index(6,1), intTreeModel->data(selItem.value(8)).toString());
    }

    for(int row = 0; row < rowCount;++row)
    {
       selItemModel->setData(selItemModel->index(row,0), itemHeader.value(isInt).at(row));
    }
}

void Activity::showInterval(bool isInt)
{
    selectInt = isInt;
    int rowCount = 6;
    selItemModel->setColumnCount(2);
    selItemModel->setRowCount(rowCount);
    selItemModel->setData(selItemModel->index(0,1),intTreeModel->data(selItem.value(0)).toString());
    selItemModel->setData(selItemModel->index(1,1),this->get_timesec(intTreeModel->data(selItem.value(2)).toString()));
    selItemModel->setData(selItemModel->index(2,1),intTreeModel->data(selItem.value(4)).toDouble());
    selItemModel->setData(selItemModel->index(3,1),intTreeModel->data(selItem.value(1)).toString());
    selItemModel->setData(selItemModel->index(4,1),intTreeModel->data(selItem.value(5)).toString());
    selItemModel->setData(selItemModel->index(5,1),intTreeModel->data(selItem.value(6)).toString());

    for(int row = 0; row < rowCount;++row)
    {
       selItemModel->setData(selItemModel->index(row,0), itemHeader.value(isInt).at(row));
    }
}

void Activity::updateIntTreeRow(QItemSelectionModel *treeSelect)
{
    if(intTreeModel->itemFromIndex(selItem.value(0))->parent() != nullptr)
    {
        this->updateSwimLap();
        this->updateSwimInt(intTreeModel->itemFromIndex(selItem.value(0))->parent()->index(),treeSelect);
    }
    else
    {
        if(selectInt)
        {
            this->updateInterval();
        }
        else
        {
            this->updateSwimLap();
            this->updateSwimBreak(selItem.value(0),treeSelect,selItemModel->data(selItemModel->index(3,1)).toInt());
        }
    }
}

void Activity::updateInterval()
{
    intTreeModel->setData(selItem.value(0),selItemModel->data(selItemModel->index(0,1)));
    intTreeModel->setData(selItem.value(1),selItemModel->data(selItemModel->index(3,1)));
    intTreeModel->setData(selItem.value(2),this->set_time(selItemModel->data(selItemModel->index(1,1)).toInt()));
    intTreeModel->setData(selItem.value(4),selItemModel->data(selItemModel->index(2,1)));
    intTreeModel->setData(selItem.value(5),selItemModel->data(selItemModel->index(4,1)));
    intTreeModel->setData(selItem.value(6),selItemModel->data(selItemModel->index(5,1)));
    this->recalcIntTree();
}

void Activity::updateSwimLap()
{
    intTreeModel->setData(selItem.value(0),selItemModel->data(selItemModel->index(0,1)));
    intTreeModel->setData(selItem.value(1),selItemModel->data(selItemModel->index(1,1)));
    intTreeModel->setData(selItem.value(4),this->set_time(selItemModel->data(selItemModel->index(3,1)).toInt()));
    intTreeModel->setData(selItem.value(6),selItemModel->data(selItemModel->index(4,1)));
    intTreeModel->setData(selItem.value(7),selItemModel->data(selItemModel->index(5,1)));
    intTreeModel->setData(selItem.value(8),selItemModel->data(selItemModel->index(6,1)));
}

void Activity::updateSwimInt(QModelIndex parentIndex,QItemSelectionModel *treeSelect)
{

    QVector<double> intValue(5);
    intValue.fill(0);

    if(intTreeModel->itemFromIndex(parentIndex)->hasChildren())
    {
        int child = 0;
        do
        {
            intValue[0] = intValue[0] + intTreeModel->itemFromIndex(parentIndex)->child(child,3)->text().toDouble();
            intValue[1] = intValue[1] + this->get_timesec(intTreeModel->itemFromIndex(parentIndex)->child(child,4)->text());
            intValue[2] = intValue[2] + this->get_timesec(intTreeModel->itemFromIndex(parentIndex)->child(child,6)->text());
            intValue[3] = intValue[3] + intTreeModel->itemFromIndex(parentIndex)->child(child,7)->text().toDouble();
            intValue[4] = intValue[4] + intTreeModel->itemFromIndex(parentIndex)->child(child,8)->text().toDouble();
            ++child;
        }
        while(intTreeModel->itemFromIndex(parentIndex)->child(child,0) != 0);
        intValue[2] = intValue[2] / child;
        intValue[3] = intValue[3] / child;
    }
    treeSelect->select(parentIndex,QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    intTreeModel->setData(treeSelect->selectedRows(3).at(0),intValue[0]);
    intTreeModel->setData(treeSelect->selectedRows(4).at(0),this->set_time(intValue[1]));
    intTreeModel->setData(treeSelect->selectedRows(6).at(0),this->set_time(intValue[2]));
    intTreeModel->setData(treeSelect->selectedRows(7).at(0),intValue[3]);
    intTreeModel->setData(treeSelect->selectedRows(8).at(0),intValue[4]);

    this->updateSwimBreak(parentIndex,treeSelect,intValue[1]);
}

void Activity::updateSwimBreak(QModelIndex intIndex,QItemSelectionModel *treeSelect,int value)
{
    QStandardItem *nextBreak = intTreeModel->item(intIndex.row()+1);
    int intStart,breakStart,breakStop;

    intStart = this->get_timesec(intTreeModel->data(treeSelect->selectedRows(5).at(0)).toString());
    breakStart = intStart + value;

    treeSelect->select(intTreeModel->indexFromItem(nextBreak),QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    breakStop = this->get_timesec(intTreeModel->data(treeSelect->selectedRows(5).at(0)).toString());

    intStart = this->get_timesec(intTreeModel->data(treeSelect->selectedRows(4).at(0)).toString()) +(breakStop-breakStart);

    intTreeModel->setData(treeSelect->selectedRows(4).at(0),this->set_time(intStart));
    intTreeModel->setData(treeSelect->selectedRows(5).at(0),this->set_time(breakStart));

    treeSelect->clearSelection();
}

void Activity::recalcIntTree()
{
    int intStart = 0;
    int intStop = 0;
    int intDura = 0;
    double workDist = 0;

    for(int row = 0; row < intTreeModel->rowCount(); ++row)
    {
       intStart = this->get_timesec(intTreeModel->data(intTreeModel->index(row,2)).toString());
       intDura = this->get_timesec(intTreeModel->data(intTreeModel->index(row,1)).toString());
       intStop = intStart + intDura;
       workDist = workDist + intTreeModel->data(intTreeModel->index(row,4)).toDouble();

       intTreeModel->setData(intTreeModel->index(row,1),this->set_time(intDura));
       intTreeModel->setData(intTreeModel->index(row,2),this->set_time(intStart));
       intTreeModel->setData(intTreeModel->index(row,3),workDist);
    }
}

void Activity::updateIntModel()
{

}

void Activity::set_swim_data()
{
    bool recalc = settings::get_act_isrecalc();
    QStringList levels = settings::get_listValues("Level");
    QString temp,zone_low,zone_high;

    hf_threshold = settings::get_thresValue("hfthres");
    hf_max = settings::get_thresValue("hfmax");

    //Set Swim zone low and high
    for(int i = 0; i < levels.count(); i++)
    {
        temp = settings::get_rangeValue(curr_sport,levels.at(i));
        zone_low = temp.split("-").first();
        zone_high = temp.split("-").last();

        swim_pace_model->setData(swim_pace_model->index(i,0,QModelIndex()),levels.at(i));
        swim_pace_model->setData(swim_pace_model->index(i,1,QModelIndex()),this->set_time(this->get_zone_values(zone_low.toDouble(),pace_cv,true)));

        p_swim_time[i] = swim_cv*(zone_low.toDouble()/100);

        if(i < zone_count-1)
        {
            swim_pace_model->setData(swim_pace_model->index(i,2,QModelIndex()),this->set_time(this->get_zone_values(zone_high.toDouble(),pace_cv,true)));
        }
        else
        {
            swim_pace_model->setData(swim_pace_model->index(i,2,QModelIndex()),"MAX");
        }
        p_swim_timezone[i] = 0;
    }

    this->set_time_in_zones(recalc);

    for (int x = 1; x <= zone_count;x++)
    {
        swim_pace_model->setData(swim_pace_model->index(x-1,3,QModelIndex()),this->set_time(p_swim_timezone[x]));
    }

    //Set HF zone low and high
    for(int i = 0; i < levels.count(); i++)
    {
        temp = settings::get_rangeValue("HF",levels.at(i));
        zone_low = temp.split("-").first();
        zone_high = temp.split("-").last();

        swim_hf_model->setData(swim_hf_model->index(i,0,QModelIndex()),levels.at(i));
        swim_hf_model->setData(swim_hf_model->index(i,1,QModelIndex()),this->get_zone_values(zone_low.toDouble(),hf_threshold,false));

        if(i < zone_count-1)
        {
            swim_hf_model->setData(swim_hf_model->index(i,2,QModelIndex()),this->get_zone_values(zone_high.toDouble(),hf_threshold,false));

            this->set_hf_zone_avg(this->get_zone_values(zone_low.toDouble(),hf_threshold,false),this->get_zone_values(zone_high.toDouble(),hf_threshold,false),i);
        }
        else
        {
            swim_hf_model->setData(swim_hf_model->index(i,2,QModelIndex()),hf_max);
            this->set_hf_zone_avg(this->get_zone_values(zone_low.toDouble(),hf_threshold,false),hf_max,i);
        }
    }
    this->set_hf_time_in_zone();
}

void Activity::set_time_in_zones(bool recalc)
{
    QStandardItemModel *model = this->set_samp_model_pointer(recalc);

    int z0=0,z1=0,z2=0,z3=0,z4=0,z5=0,z6=0,z7=0;
    double paceSec;

    for(int i = 0; i < model->rowCount(); i++)
    {
        paceSec = model->data(model->index(i,2,QModelIndex())).toDouble();

        if(paceSec <= p_swim_time[0])
        {
            ++z0;
            p_swim_timezone[0] = z0;
        }

        if(paceSec > p_swim_time[0] && paceSec <= p_swim_time[1])
        {
            ++z1;
            p_swim_timezone[1] = z1;
        }

        if(paceSec > p_swim_time[1] && paceSec <= p_swim_time[2])
        {
            ++z2;
            p_swim_timezone[2] = z2;
        }

        if(paceSec > p_swim_time[2] && paceSec <= p_swim_time[3])
        {
            ++z3;
            p_swim_timezone[3] = z3;
        }

        if(paceSec > p_swim_time[3] && paceSec <= p_swim_time[4])
        {
            ++z4;
            p_swim_timezone[4] = z4;
        }

        if(paceSec > p_swim_time[4] && paceSec <= p_swim_time[5])
        {
            ++z5;
            p_swim_timezone[5] = z5;
        }

        if(paceSec > p_swim_time[5] && paceSec <= p_swim_time[6])
        {
            ++z6;
            p_swim_timezone[6] = z6;
        }
        if(paceSec > p_swim_time[6])
        {
            ++z7;
            p_swim_timezone[7] = z7;
        }
    }

    //Calc Swim Data
    move_time = 0;
    for(int i = 1; i <= 7;i++)
    {
        move_time = move_time + p_swim_timezone[i];
    }

    swim_pace = ceil(static_cast<double>(move_time) / (samp_model->data(samp_model->index(samp_model->rowCount()-1,1,QModelIndex())).toDouble()*10));

    double goal = sqrt(pow(static_cast<double>(swim_pace),3.0))/10;
    swim_sri = static_cast<double>(pace_cv) / goal;
}

void Activity::set_hf_time_in_zone()
{
     double hf_factor[] = {0.125,0.25,0.5,0.75};

     //REKOM: Z0*0,125 + Z1 + Z2*0,5
     p_hf_timezone[0] = ceil((p_swim_timezone[0]*hf_factor[0]) + p_swim_timezone[1] + (p_swim_timezone[2]*hf_factor[2]));

     //END: Z0*0,5 + Z2*0,5 + Z3*0,75
     p_hf_timezone[1] = ceil((p_swim_timezone[0]*hf_factor[2]) + (p_swim_timezone[2]*hf_factor[2]) + (p_swim_timezone[3]*hf_factor[3]));

     //TEMP: Z0*0,25 + Z3*0,25 + Z4*0,75 + Z5*0,5
     p_hf_timezone[2] = ceil((p_swim_timezone[0]*hf_factor[1]) + (p_swim_timezone[3]*hf_factor[1]) + p_swim_timezone[4] + (p_swim_timezone[5]*hf_factor[2]));

     //LT: Z5*0,25 + Z6*0,75 + Z7*0,5
     p_hf_timezone[3] = ceil((p_swim_timezone[5]*hf_factor[1]) + (p_swim_timezone[6]*hf_factor[3]) + (p_swim_timezone[7]*hf_factor[2]));

     //VO2: Z5*0,25 + Z6*0,125 + Z7*0,25
     p_hf_timezone[4] = ceil((p_swim_timezone[5]*hf_factor[1]) + (p_swim_timezone[6]*hf_factor[0]) + (p_swim_timezone[7]*hf_factor[1]));

     //AC: Z6*0,125 + Z7*0,125
     p_hf_timezone[5] = ceil((p_swim_timezone[6]*hf_factor[0]) + (p_swim_timezone[7]*hf_factor[0]));

     //NEURO: Z7*0,125
     p_hf_timezone[6] = ceil(p_swim_timezone[7]*hf_factor[0]);

     for(int i = 0; i < zone_count; i++)
     {
         swim_hf_model->setData(swim_hf_model->index(i,3,QModelIndex()),this->set_time(p_hf_timezone[i]));
     }
    hf_avg = 0;

    //set HF Avg
    double hf_part;
    for(int i = 0; i < 7; i++)
    {
        hf_part = static_cast<double>(hf_zone_avg[i]) * (static_cast<double>(p_hf_timezone[i])/static_cast<double>(samp_model->rowCount()));
        hf_avg = hf_avg + ceil(hf_part);
    }
}

void Activity::set_hf_zone_avg(double low,double high, int pos)
{
    hf_zone_avg[pos] = ceil((low + high) / 2);
}

int Activity::get_zone_values(double factor, int max, bool ispace)
{
    int zone_value;
    if(ispace)
    {
        zone_value = ceil(max/(factor/100));
    }
    else
    {
        zone_value = ceil(max*(factor/100));
    }

    return zone_value;
}

int Activity::get_header_num()
{
    if(curr_sport == settings::isSwim)
    {
        return 6;
    }
    if(curr_sport == settings::isBike)
    {
        return 7;
    }
    if(curr_sport == settings::isRun)
    {
        return 5;
    }
    if(curr_sport == settings::isTria)
    {
        return 5;
    }

    return 0;
}

int Activity::get_int_duration(int row)
{
    int duration;

    duration = int_model->data(int_model->index(row,2,QModelIndex()),Qt::DisplayRole).toInt() - int_model->data(int_model->index(row,1,QModelIndex()),Qt::DisplayRole).toInt();

    return duration;
}

double Activity::get_int_distance(int row)
{
    double dist,dist_start,dist_stop;
    int int_start,int_stop;

    if(row == 0)
    {
        int_stop = int_model->data(int_model->index(row,2,QModelIndex()),Qt::DisplayRole).toInt();
        dist = samp_model->data(samp_model->index(int_stop,1,QModelIndex()),Qt::DisplayRole).toDouble();
    }
    else
    {
        int_start = int_model->data(int_model->index(row,1,QModelIndex()),Qt::DisplayRole).toInt();
        int_stop = int_model->data(int_model->index(row,2,QModelIndex()),Qt::DisplayRole).toInt();
        dist_start = samp_model->data(samp_model->index(int_start,1,QModelIndex()),Qt::DisplayRole).toDouble();
        dist_stop = samp_model->data(samp_model->index(int_stop,1,QModelIndex()),Qt::DisplayRole).toDouble();
        dist = dist_stop - dist_start;
    }

    return dist;
}

int Activity::get_int_pace(int row)
{
    int pace;
    if(this->get_int_distance(row) == 0)
    {
        pace = 0;
    }
    else
    {
        if (curr_sport == settings::isSwim)
        {
            pace = this->get_int_duration(row) / (this->get_int_distance(row)*10);
        }
        else
        {
            pace = this->get_int_duration(row) / this->get_int_distance(row);
        }
    }

    return pace;
}

int Activity::get_swim_laps(int row)
{
    return round((this->get_int_distance(row)*dist_factor)/swim_track);
}

double Activity::get_int_value(int row,int col, bool recalc)
{
    QStandardItemModel *model = this->set_samp_model_pointer(recalc);
    double value = 0.0;
    int int_start,int_stop;
    int_start = int_model->data(int_model->index(row,1,QModelIndex())).toInt();
    int_stop = int_model->data(int_model->index(row,2,QModelIndex())).toInt();

    for(int i = int_start; i < int_stop; ++i)
    {
        value = value + model->data(model->index(i,col,QModelIndex())).toDouble();
    }
    value = value / (int_stop - int_start);

    return value;
}

QStandardItemModel * Activity::set_int_model_pointer(bool recalc)
{
    if(recalc)
    {
        return edit_int_model;
    }
    else
    {
        return int_model;
    }
}

QStandardItemModel * Activity::set_samp_model_pointer(bool recalc)
{
    if(recalc)
    {
        return edit_samp_model;
    }
    else
    {
        return samp_model;
    }
}

double Activity::get_int_speed(int row)
{
    double speed,pace;

    if(curr_sport == settings::isSwim)
    {
        pace = this->get_int_duration(row) / (int_model->data(int_model->index(row,3,QModelIndex())).toDouble() / dist_factor);
    }
    else
    {
        pace = this->get_int_duration(row) / (int_model->data(int_model->index(row,3,QModelIndex())).toDouble()); // *10.0
    }
    speed = 3600.0 / pace;

    return speed;
}

double Activity::polish_SpeedValues(double currSpeed,double avgSpeed,double factor,bool setrand)
{
    double randfact = 0;
    if(curr_sport == settings::isRun)
    {
        randfact = ((static_cast<double>(rand()) / static_cast<double>(RAND_MAX)) / (currSpeed/((factor*100)+1.0)));
    }
    if(curr_sport == settings::isBike)
    {
        randfact = ((static_cast<double>(rand()) / static_cast<double>(RAND_MAX)) / (currSpeed/((factor*1000)+1.0)));
    }

    double avgLow = avgSpeed-(avgSpeed*factor);
    double avgHigh = avgSpeed+(avgSpeed*factor);

    if(setrand)
    {
        if(currSpeed < avgLow)
        {
            return avgLow+randfact;
        }
        if(currSpeed > avgHigh)
        {
            return avgHigh-randfact;
        }
        if(currSpeed > avgLow && currSpeed < avgHigh)
        {
            return currSpeed;
        }
        return currSpeed + randfact;
    }
    else
    {
        if(currSpeed < avgLow)
        {
            return avgLow;
        }
        if(currSpeed > avgHigh)
        {
            return avgHigh;
        }
        if(currSpeed > avgLow && currSpeed < avgHigh)
        {
            return currSpeed;
        }
    }
    return 0;
}

double Activity::interpolate_speed(int row,int sec,double limit)
{
    double curr_speed = sampSpeed[sec];
    double avg_speed = this->get_int_speed(row);

    if(curr_speed == 0)
    {
        curr_speed = limit;
    }

    if(row == 0 && sec < 5)
    {
        return (static_cast<double>(sec) + ((static_cast<double>(rand()) / static_cast<double>(RAND_MAX)))) * 1.2;
    }
    else
    {
        if(avg_speed >= limit)
        {
            return this->polish_SpeedValues(curr_speed,avg_speed,polishFactor,true);
        }
        else
        {
            return curr_speed;
        }
    }

    return 0;
}

int Activity::check_is_intervall(int row)
{
    double isInt;
    isInt = edit_int_model->data(edit_int_model->index(row,3,QModelIndex())).toDouble();
    if(isInt == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void Activity::set_edit_samp_model(int rowcount)
{
    int sampRowCount = rowcount;
    int sampColCount = samp_model->columnCount();
    edit_samp_model = new QStandardItemModel(sampRowCount,sampColCount);
    new_dist.resize(sampRowCount);
    calc_speed.resize(sampRowCount);
    calc_cadence.resize(sampRowCount);
    double msec = 0.0;
    int int_start,int_stop,sportpace = 0,swimLaps;
    double overall = 0.0,lowLimit = 0.0,limitFactor = 0.0;
    double swimPace,swimSpeed,swimCycle;
    bool isBreak = true;
    if(curr_sport != settings::isSwim && curr_sport != settings::isTria)
    {
        if(curr_sport == settings::isBike)
        {
            sportpace = settings::get_thresValue("bikepace");
            limitFactor = 0.35;
        }
        if(curr_sport == settings::isRun)
        {
            sportpace = settings::get_thresValue("runpace");
            limitFactor = 0.20;
        }
        lowLimit = this->get_speed(QTime::fromString(this->set_time(sportpace),"mm:ss"),0,curr_sport,true);
        lowLimit = lowLimit - (lowLimit*limitFactor);
    }
    if(curr_sport == settings::isTria)
    {
        lowLimit = 1.0;
    }

    if(curr_sport == settings::isSwim)
    {
        swimLaps = 0;
        for(int sLap = 0; sLap < swim_xdata->rowCount();++sLap)
        {
            int_start = swim_xdata->data(swim_xdata->index(sLap,1,QModelIndex()),Qt::DisplayRole).toInt();
            swimPace = swim_xdata->data(swim_xdata->index(sLap,2,QModelIndex()),Qt::DisplayRole).toDouble();
            swimSpeed = swim_xdata->data(swim_xdata->index(sLap,4,QModelIndex()),Qt::DisplayRole).toDouble();
            swimCycle = swim_xdata->data(swim_xdata->index(sLap,3,QModelIndex()),Qt::DisplayRole).toDouble();

            if(sLap == swim_xdata->rowCount()-1)
            {
                int_stop = sampRowCount-1;
            }
            else
            {
                int_stop = swim_xdata->data(swim_xdata->index(sLap+1,1,QModelIndex()),Qt::DisplayRole).toInt();
            }

            if(swimSpeed > 0)
            {
                ++swimLaps;
                msec = (swim_track / swimPace) / dist_factor;
                overall = (swim_track * swimLaps) / dist_factor;
                isBreak = false;
            }
            else
            {
                msec = 0;
                isBreak = true;
            }

            for(int lapsec = int_start; lapsec <= int_stop; ++lapsec)
            {
                if(lapsec == 0)
                {
                    new_dist[lapsec] = 0.001;
                }
                else
                {
                    if(lapsec == int_start && isBreak)
                    {
                        new_dist[lapsec] = overall;
                    }
                    else
                    {
                        new_dist[lapsec] = new_dist[lapsec-1] + msec;
                    }
                    if(lapsec == int_stop)
                    {
                        new_dist[int_stop-1] = overall;
                        new_dist[int_stop] = overall;
                    }
                }
                calc_speed[lapsec] = swimSpeed;
                calc_cadence[lapsec] = swimCycle;
            }
        }

        for(int row = 0; row < sampRowCount;++row)
        {
            edit_samp_model->setData(edit_samp_model->index(row,0,QModelIndex()),row);
            edit_samp_model->setData(edit_samp_model->index(row,1,QModelIndex()),new_dist[row]);
            edit_samp_model->setData(edit_samp_model->index(row,2,QModelIndex()),calc_speed[row]);
            edit_samp_model->setData(edit_samp_model->index(row,3,QModelIndex()),calc_cadence[row]);
        }
        this->set_swim_data();
    }
    else
    {
      for(int c_int = 0; c_int < edit_int_model->rowCount(); ++c_int)
      {
         int_start = edit_int_model->data(edit_int_model->index(c_int,1,QModelIndex())).toInt();
         int_stop = edit_int_model->data(edit_int_model->index(c_int,2,QModelIndex())).toInt();
         msec = edit_int_model->data(edit_int_model->index(c_int,3,QModelIndex())).toDouble() / this->get_int_duration(c_int);

         if(curr_sport == settings::isRun)
         {
             for(int c_dist = int_start;c_dist <= int_stop; ++c_dist)
             {
                if(c_dist == 0)
                {
                    new_dist[0] = 0.0000;
                }
                else
                {
                    calc_speed[c_dist] = this->interpolate_speed(c_int,c_dist,lowLimit);
                    new_dist[c_dist] = new_dist[c_dist-1] + msec;
                }
             }
         }

         if(curr_sport == settings::isBike)
         {
             for(int c_dist = int_start;c_dist <= int_stop; ++c_dist)
             {
                if(c_dist == 0)
                {

                    new_dist[0] = 0.0000;
                }
                else
                {
                    calc_speed[c_dist] = this->interpolate_speed(c_int,c_dist,lowLimit);
                    calc_cadence[c_dist] = samp_model->data(samp_model->index(c_dist,3,QModelIndex())).toDouble();
                    new_dist[c_dist] = new_dist[c_dist-1] + msec;
                }
             }
          }
        }
      }

      if(curr_sport == settings::isBike)
      {
          for(int row = 0; row < sampRowCount;++row)
          {
              edit_samp_model->setData(edit_samp_model->index(row,0,QModelIndex()),row);
              edit_samp_model->setData(edit_samp_model->index(row,1,QModelIndex()),new_dist[row]);
              edit_samp_model->setData(edit_samp_model->index(row,2,QModelIndex()),calc_speed[row]);
              edit_samp_model->setData(edit_samp_model->index(row,3,QModelIndex()),calc_cadence[row]);
              edit_samp_model->setData(edit_samp_model->index(row,4,QModelIndex()),samp_model->data(samp_model->index(row,4,QModelIndex())).toDouble());
              for(int col = 5; col < sampColCount; ++col)
              {
                  edit_samp_model->setData(edit_samp_model->index(row,col,QModelIndex()),samp_model->data(samp_model->index(row,col,QModelIndex())).toDouble());
              }
          }
      }

      if(curr_sport == settings::isRun)
      {
          for(int row = 0; row < sampRowCount;++row)
          {
              edit_samp_model->setData(edit_samp_model->index(row,0,QModelIndex()),row);
              edit_samp_model->setData(edit_samp_model->index(row,1,QModelIndex()),new_dist[row]);
              edit_samp_model->setData(edit_samp_model->index(row,2,QModelIndex()),calc_speed[row]);
              for(int col = 3; col < sampColCount; ++col)
              {
                  edit_samp_model->setData(edit_samp_model->index(row,col,QModelIndex()),samp_model->data(samp_model->index(row,col,QModelIndex())).toDouble());
              }
          }
      }

      if(curr_sport == settings::isTria)
      {
          double triValue = 0,sportValue = 0;

          for(int row = 0; row < sampRowCount;++row)
          {
              edit_samp_model->setData(edit_samp_model->index(row,0,QModelIndex()),row);
              edit_samp_model->setData(edit_samp_model->index(row,1,QModelIndex()),new_dist[row]);
              edit_samp_model->setData(edit_samp_model->index(row,2,QModelIndex()),calc_speed[row]);
              edit_samp_model->setData(edit_samp_model->index(row,3,QModelIndex()),calc_cadence[row]);
              for(int col = 4; col < sampColCount; ++col)
              {
                  edit_samp_model->setData(edit_samp_model->index(row,col,QModelIndex()),samp_model->data(samp_model->index(row,col,QModelIndex())).toDouble());
              }
          }
          jsonhandler->set_overrideFlag(true);
          sportValue = round(this->estimate_stress(settings::isSwim,this->set_time(this->get_int_pace(0)/10),this->get_int_duration(0)));
          jsonhandler->set_overrideData("swimscore",QString::number(sportValue));
          triValue = triValue + sportValue;
          sportValue = round(this->estimate_stress(settings::isBike,QString::number(this->get_int_value(2,4,true)),this->get_int_duration(2)));
          jsonhandler->set_overrideData("skiba_bike_score",QString::number(sportValue));
          triValue = triValue + sportValue;
          sportValue = round(this->estimate_stress(settings::isRun,this->set_time(this->get_int_pace(4)),this->get_int_duration(4)));
          jsonhandler->set_overrideData("govss",QString::number(sportValue));
          triValue = triValue + sportValue;
          jsonhandler->set_overrideData("triscore",QString::number(triValue));
      }
}

void Activity::set_avgValues(int counter,int factor)
{
    avgCounter = counter;

    if(counter != 0)
    {
        if(curr_sport == settings::isSwim)
        {
            avgValues[0] = avgValues[0] + (static_cast<double>(this->get_timesec(intTreeModel->data(selItem.value(4)).toString()))*factor);
            avgValues[1] = avgValues.at(1) + (static_cast<double>(this->get_timesec(intTreeModel->data(selItem.value(6)).toString()))*factor);
            avgValues[2] = avgValues.at(2) + (intTreeModel->data(selItem.value(3)).toDouble()*factor);
        }
        else
        {
            avgValues[0] = avgValues.at(0) + (static_cast<double>(this->get_timesec(intTreeModel->data(selItem.value(1)).toString()))*factor);
            avgValues[1] = avgValues.at(1) + (static_cast<double>(this->get_timesec(intTreeModel->data(selItem.value(5)).toString()))*factor);
            avgValues[2] = avgValues.at(2) + (intTreeModel->data(selItem.value(4)).toDouble()*factor);

            if(curr_sport == settings::isBike)
            {
                avgValues[3] = avgValues.at(3) + (intTreeModel->data(selItem.value(7)).toDouble()*factor);
                avgValues[4] = avgValues.at(4) + (intTreeModel->data(selItem.value(8)).toDouble()*factor);
            }
        }

        avgModel->setData(avgModel->index(0,1),this->set_time(round(avgValues[0]/avgCounter)));
        avgModel->setData(avgModel->index(1,1),this->set_time(round(avgValues[1]/avgCounter)));
        avgModel->setData(avgModel->index(2,1),avgValues[2]/avgCounter);
    }
    else
    {
        this->reset_avg();
    }
}

void Activity::reset_avg()
{
    avgCounter = 1;
    avgValues.fill(0);
}
