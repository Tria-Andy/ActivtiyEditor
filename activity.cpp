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

Activity::Activity(QString jsonfile,bool intAct)
{
    this->readJsonFile(jsonfile,intAct);

}

void Activity::readJsonFile(QString jsonfile,bool intAct)
{
    QStringList valueList;
    QString stgValue;
    valueList = settings::get_listValues("JsonFile");

    curr_sport = this->readJsonContent(jsonfile);

    for(int i = 0; i < valueList.count();++i)
    {
        stgValue = valueList.at(i);
        ride_info.insert(stgValue,tagData.value(stgValue));
    }
    ride_info.insert("Date",rideData.value("STARTTIME"));

    if(intAct)
    {
        QMap<int,QString> mapValues;

        intModel = new QStandardItemModel;
        mapValues = settings::get_intList();
        this->init_actModel("INTERVALS",&mapValues,intModel,&intList,3);

        sampleModel = new QStandardItemModel;
        mapValues = settings::get_sampList();
        this->init_actModel("SAMPLES",&mapValues,sampleModel,&sampList,0);

        if(hasXdata)
        {
            xdata_model = new QStandardItemModel;
            this->init_xdataModel(curr_sport,xdata_model);
        }
        this->prepareData();
    }
}

void Activity::prepareData()
{
    isSwim = false;
    itemHeader.insert(0,QStringList() << "Name:" << "Type:" << "Distance (M):" << "Duration (Sec):" << "Pace/100m:" << "Speed km/h:" << "Strokes:");
    itemHeader.insert(1,QStringList() << "Name:" << "Start:" << "Distance:" << "Duration:" << "Pace:" << "Speed km/h:");
    avgHeader.insert(0,QStringList() << "Intervalls:" << "Duration:" << "Pace:" << "Distance:");
    avgHeader.insert(1,QStringList() << "Intervalls:" << "Duration:" << "Pace:" << "Distance:" << "Watts:" << "CAD:");
    intTreeModel = new QStandardItemModel;
    selItemModel = new QStandardItemModel;
    selItemModel->setColumnCount(1);
    avgModel = new QStandardItemModel;
    avgModel->setColumnCount(1);
    if(curr_sport == settings::isSwim) isSwim = true;

    int sampCount = sampleModel->rowCount();
    int intModelCount = intModel->rowCount();
    int avgHF = 0;
    int posHF = sampList.indexOf("HR");
    int lapCount = 1;
    int lapKey = 1;
    int dist = 0;
    int lapIdent = 0;
    QString breakName = settings::get_generalValue("breakname");
    QString lapName;
    sampSpeed.resize(sampCount);

    for(int i = 0; i < sampCount; ++i)
    {
        sampSpeed[i] = sampleModel->data(sampleModel->index(i,2,QModelIndex())).toDouble();
        avgHF = avgHF + sampleModel->data(sampleModel->index(i,posHF,QModelIndex())).toInt();
    }

    if(curr_sport == settings::isRun)
    {
        avgHF = (avgHF / sampCount);
        double totalWork = ceil(this->calc_totalWork(tagData.value("Weight").toDouble(),avgHF,sampCount));
        overrideData.insert("total_work",QString::number(totalWork));
        ride_info.insert("Total Cal",QString::number(totalWork));
        ride_info.insert("Total Work",QString::number(totalWork));
        ride_info.insert("AvgHF",QString::number(avgHF));
        hasOverride = true;
    }

    intModel->setData(intModel->index(intModelCount-1,2,QModelIndex()),sampCount-1);

    if(isSwim)
    {
        dist_factor = 1000;
        swim_track = tagData.value("Pool Length").toDouble();
        double swimoffset = 0.0;
        if(hasOverride) swimoffset = settings::get_thresValue("swimoffset");

        swimModel = new QStandardItemModel(xdata_model->rowCount(),10);
        swimType = settings::get_listValues("SwimStyle");

        int intCounter = 1;
        int breakCounter = 1;
        int intStart,intStop;
        int swimLap = 0;

        for(int row = 0; row < intModelCount; ++row)
        {
            lapCount = this->get_swim_laps(row);
            dist = lapCount*swim_track;
            intModel->setData(intModel->index(row,3),dist);
            intModel->setData(intModel->index(row,4),lapCount);

            intStart = intModel->data(intModel->index(row,1)).toInt();
            intStop = intModel->data(intModel->index(row,2)).toInt();

            if(lapCount > 0)
            {
                lapIdent = lapCount == 1 ? 1 : 0;
                intModel->setData(intModel->index(row,5),"Int-"+QString::number(lapKey)+"_"+QString::number(lapIdent));
            }
            else
            {
                intModel->setData(intModel->index(row,5),breakName+"_"+QString::number(breakCounter));
                ++lapKey;
                ++breakCounter;
            }

            if(row == 0)
            {
                lapName = QString::number(intCounter)+"_Warmup_"+QString::number(dist);
                swimLap = this->build_swimModel(true,"Int-",intCounter,intStart,intStop,swimLap,swimoffset);
                ++intCounter;
            }
            else if(row != 0 && row < intModelCount-1)
            {
                if(lapCount > 0)
                {
                    lapName = QString::number(intCounter)+"_Int_"+QString::number(dist);
                    swimLap = this->build_swimModel(true,"Int-",intCounter,intStart,intStop,swimLap,swimoffset);
                    ++intCounter;
                }
                else
                {
                    lapName = breakName;
                    swimLap = this->build_swimModel(false,breakName,intCounter,intStart,intStop,swimLap,swimoffset);
                }
            }
            else if(row == intModelCount-1)
            {
                lapName = QString::number(intCounter)+"_Cooldown_"+QString::number(dist);
                swimLap = this->build_swimModel(true,"Int-",intCounter,intStart,intStop,swimLap,swimoffset);
            }

            intModel->setData(intModel->index(row,0),lapName);
        }

        swimProxy = new QSortFilterProxyModel;
        swimProxy->setSourceModel(swimModel);

        xdata_model->clear();
        delete xdata_model;

        levels = settings::get_listValues("Level");
        zone_count = levels.count();

        swimTimezone.resize(zone_count+1);
        swimTimezone.fill(0);
        swimTime.resize(zone_count+1);
        swimTime.fill(0);
        hfTimezone.resize(zone_count);
        hfTimezone.fill(0);
        hf_avg = 0;
        move_time = 0.0;

        //Read current CV and HF Threshold
        double temp_cv = settings::get_thresValue("swimpace");
        swim_cv = (3600.0 / temp_cv) / 10.0;
        pace_cv = temp_cv;

        QString temp,zone_low,zone_high;
        double zoneLow, zoneHigh;
        hf_threshold = settings::get_thresValue("hfthres");
        hf_max = settings::get_thresValue("hfmax");

        //Set Swim zone low and high
        for(int i = 0; i < levels.count(); i++)
        {
            temp = settings::get_rangeValue(curr_sport,levels.at(i));
            zone_low = temp.split("-").first();
            zone_high = temp.split("-").last();

            swimSpeed.insert(levels.at(i),QPair::first = swim_cv*(zone_low.toDouble()/100));
            swimSpeed.insert(levels.at(i),QPair::second = swim_cv*(zone_high.toDouble()/100));
            //swimSpeed.insert(levels.at(i),swim_cv*(zone_low.toDouble()/100));
            paceTimeInZone.insert(levels.at(i),0);
            hfTimeInZone.insert(levels.at(i),0);

            zoneLow = this->get_zone_values(zone_low.toDouble(),hf_threshold,false);
            zoneHigh = this->get_zone_values(zone_high.toDouble(),hf_threshold,false);

            if(i < zone_count-1)
            {
                hfZoneAvg.insert(levels.at(i),ceil((zoneLow + zoneHigh) / 2));
            }
            else
            {
                hfZoneAvg.insert(levels.at(i),ceil((zoneLow + hf_max) / 2));
            }
        }
        this->calc_SwimTimeInZones();
    }
    else
    {
        dist_factor = 1;

        for(int row = 0; row < intModelCount; ++row)
        {

            intModel->setData(intModel->index(row,3),this->get_int_distance(row));
            intModel->setData(intModel->index(row,4),lapCount);
            intModel->setData(intModel->index(row,5),"Int-"+QString::number(row));
        }
    }

    ride_info.insert("Distance",QString::number(sampleModel->data(sampleModel->index(sampCount-1,1,QModelIndex())).toDouble()));
    ride_info.insert("Duration",QDateTime::fromTime_t(sampCount).toUTC().toString("hh:mm:ss"));

    if(curr_sport == settings::isBike)
    {
        avgValues.resize(6);
        avgModel->setVerticalHeaderLabels(avgHeader.value(1));
    }
    else
    {
        avgValues.resize(4);
        avgModel->setVerticalHeaderLabels(avgHeader.value(0));
    }
    avgModel->setRowCount(avgValues.count());
    selItemModel->setVerticalHeaderLabels(itemHeader.value(0));

    this->reset_avgSelection();
    this->build_intTree();
}

int Activity::build_swimModel(bool isInt,QString intName,int intCount,int intStart,int intStop,int swimLap,double swimoffset)
{
    QString swimLapName,lapKey;
    int lapStart = 0,lapStop = 0;
    int swimlapCount = 1;
    int type = 0;
    int lapTime;
    double lapDist,lapSpeed;

    lapStart = intStart;

    while (lapStart >= intStart && lapStop < intStop)
    {
        lapStart = xdata_model->data(xdata_model->index(swimLap,0)).toInt();
        lapStop = xdata_model->data(xdata_model->index(swimLap+1,0)).toInt();

        lapTime = round(xdata_model->data(xdata_model->index(swimLap,3)).toDouble() - swimoffset);
        type = xdata_model->data(xdata_model->index(swimLap,2)).toInt();
        lapDist = xdata_model->data(xdata_model->index(swimLap,1)).toDouble();

        if(isInt)
        {
            lapSpeed = this->calcSpeed(lapTime,swim_track,dist_factor);
            swimLapName = QString::number(intCount)+"_"+QString::number(swimlapCount*swim_track);
            lapKey = intName+QString::number(intCount)+"_"+QString::number(swimlapCount);
        }
        else
        {
            lapSpeed = 0;
            swimLapName = lapKey = intName;
        }

        swimModel->setData(swimModel->index(swimLap,0,QModelIndex()),swimLapName);
        swimModel->setData(swimModel->index(swimLap,1,QModelIndex()),lapStart);
        swimModel->setData(swimModel->index(swimLap,2,QModelIndex()),lapDist);
        swimModel->setData(swimModel->index(swimLap,3,QModelIndex()),type);
        swimModel->setData(swimModel->index(swimLap,4,QModelIndex()),swim_track);
        swimModel->setData(swimModel->index(swimLap,5,QModelIndex()),lapTime);
        swimModel->setData(swimModel->index(swimLap,6,QModelIndex()),lapTime * (100.0/swim_track));
        swimModel->setData(swimModel->index(swimLap,7,QModelIndex()),lapSpeed);
        swimModel->setData(swimModel->index(swimLap,8,QModelIndex()),xdata_model->data(xdata_model->index(swimLap,4,QModelIndex())));
        swimModel->setData(swimModel->index(swimLap,9,QModelIndex()),lapKey);

        ++swimLap;
        ++swimlapCount;
    }

    return swimLap;
}

void Activity::build_intTree()
{
    QStandardItem *rootItem = intTreeModel->invisibleRootItem();
    if(curr_sport == settings::isSwim)
    {
        QString intKey;
        for(int i = 0; i < intModel->rowCount(); ++i)
        {
            intKey = intModel->data(intModel->index(i,5)).toString().split("_").first();
            rootItem->appendRow(setSwimLap(i,intKey));
        }
    }
    else
    {
        for(int i = 0; i < intModel->rowCount(); ++i)
        {
            rootItem->appendRow(setIntRow(i));
        }
    }
    intTreeModel->setHorizontalHeaderLabels(settings::get_int_header(curr_sport));
}

QList<QStandardItem *> Activity::setIntRow(int pInt)
{
    QList<QStandardItem*> intItems;
    QModelIndex data_index = sampleModel->index(intModel->data(intModel->index(pInt,2)).toInt()-1,1);
    QString lapName = intModel->data(intModel->index(pInt,0)).toString();

    intItems << new QStandardItem(lapName);
    intItems << new QStandardItem(this->set_time(this->get_int_duration(pInt)));
    intItems << new QStandardItem(this->set_time(intModel->data(intModel->index(pInt,1)).toInt()));
    intItems << new QStandardItem(QString::number(this->set_doubleValue(sampleModel->data(data_index).toDouble(),true)));
    intItems << new QStandardItem(QString::number(this->set_doubleValue(this->get_int_distance(pInt),true)));
    intItems << new QStandardItem(this->set_time(this->get_int_pace(pInt,lapName)));
    intItems << new QStandardItem(QString::number(this->set_doubleValue(this->get_int_speed(pInt),false)));

    if(curr_sport == settings::isBike)
    {
        intItems << new QStandardItem(QString::number(round(this->get_int_value(pInt,4))));
        intItems << new QStandardItem(QString::number(round(this->get_int_value(pInt,3))));
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
    int lapCount = intModel->data(intModel->index(pInt,4)).toInt();
    QString lapName = intModel->data(intModel->index(pInt,0)).toString();

    swimProxy->setFilterRegExp("^"+intKey+"_");
    swimProxy->setFilterKeyColumn(9);

    intItems << new QStandardItem(lapName);
    intItems << new QStandardItem(swimType.at(currType));
    intItems << new QStandardItem(intModel->data(intModel->index(pInt,4)).toString());
    intItems << new QStandardItem(intModel->data(intModel->index(pInt,3)).toString());
    intItems << new QStandardItem(this->set_time(this->get_int_duration(pInt)));
    intItems << new QStandardItem(this->set_time(intModel->data(intModel->index(pInt,1)).toInt()));
    intItems << new QStandardItem(this->set_time(this->get_int_pace(pInt,lapName)));
    intItems << new QStandardItem(QString::number(this->get_int_speed(pInt)));
    intItems << new QStandardItem(QString::number(strokeCount));
    intItems << new QStandardItem("-");

    if(lapCount > 1)
    {
        int intTime = 0;
        int lapTime = 0;
        for(int i = 0; i < swimProxy->rowCount();++i)
        {
            lapTime = swimProxy->data(swimProxy->index(i,5)).toInt();
            currType = swimProxy->data(swimProxy->index(i,3)).toInt();
            lapType = currType == lastType ? currType : 6;
            strokeCount = strokeCount+swimProxy->data(swimProxy->index(i,8)).toInt();

            subItems << new QStandardItem(swimProxy->data(swimProxy->index(i,0)).toString());
            subItems << new QStandardItem(swimType.at(currType));
            subItems << new QStandardItem("-");
            subItems << new QStandardItem(swimProxy->data(swimProxy->index(i,4)).toString());
            subItems << new QStandardItem(this->set_time(lapTime));
            subItems << new QStandardItem("-");
            subItems << new QStandardItem(this->set_time(swimProxy->data(swimProxy->index(i,6)).toInt()));
            subItems << new QStandardItem(QString::number(swimProxy->data(swimProxy->index(i,7)).toDouble()));
            subItems << new QStandardItem(swimProxy->data(swimProxy->index(i,8)).toString());

            lastType = currType;
            intTime = intTime + lapTime;
            intItems.first()->appendRow(subItems);
            subItems.clear();
        }
        intItems.at(1)->setData(swimType.at(lapType),Qt::EditRole);
        intItems.at(6)->setData(this->set_time(intTime),Qt::EditRole);
        intItems.at(8)->setData(strokeCount,Qt::EditRole);
    }
    else
    {
        intItems.at(1)->setData(swimType.at(swimProxy->data(swimProxy->index(0,3)).toInt()),Qt::EditRole);
        intItems.at(8)->setData(swimProxy->data(swimProxy->index(0,8)).toString(),Qt::EditRole);
    }

    return intItems;
}

void Activity::set_selectedItem(QItemSelectionModel *treeSelect)
{
    selItem.clear();
    for(int i = 0; i < treeSelect->selectedIndexes().count();++i)
    {
        selItem.insert(i,treeSelect->selectedRows(i).at(0));
    }
}

void Activity::showSwimLap(bool isInt)
{
    selItemModel->setVerticalHeaderLabels(itemHeader.value(isInt));
    selectInt = isInt;
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

        selItemModel->setData(selItemModel->index(0,0),intTreeModel->data(selItem.value(0)).toString());
        selItemModel->setData(selItemModel->index(1,0),intStart);
        selItemModel->setData(selItemModel->index(2,0),dist);
        selItemModel->setData(selItemModel->index(3,0),intTreeModel->data(selItem.value(4)).toString());
        selItemModel->setData(selItemModel->index(4,0),this->set_time(this->calc_lapPace(curr_sport,dura,dist)));
        selItemModel->setData(selItemModel->index(5,0),intTreeModel->data(selItem.value(7)).toDouble());
    }
    else
    {
        rowCount = 7;
        selItemModel->setRowCount(rowCount);
        selItemModel->setData(selItemModel->index(0,0), intTreeModel->data(selItem.value(0)).toString());
        selItemModel->setData(selItemModel->index(1,0), intTreeModel->data(selItem.value(1)).toString());
        selItemModel->setData(selItemModel->index(2,0), intTreeModel->data(selItem.value(3)).toString());
        selItemModel->setData(selItemModel->index(3,0), this->get_timesec(intTreeModel->data(selItem.value(4)).toString()));
        selItemModel->setData(selItemModel->index(4,0), intTreeModel->data(selItem.value(6)).toString());
        selItemModel->setData(selItemModel->index(5,0), intTreeModel->data(selItem.value(7)).toDouble());
        selItemModel->setData(selItemModel->index(6,0), intTreeModel->data(selItem.value(8)).toString());
    }
}

void Activity::showInterval(bool isInt)
{
    selItemModel->setVerticalHeaderLabels(itemHeader.value(isInt));
    selectInt = isInt;
    int rowCount = 6;
    selItemModel->setRowCount(rowCount);
    selItemModel->setData(selItemModel->index(0,0),intTreeModel->data(selItem.value(0)).toString());
    selItemModel->setData(selItemModel->index(1,0),this->get_timesec(intTreeModel->data(selItem.value(2)).toString()));
    selItemModel->setData(selItemModel->index(2,0),intTreeModel->data(selItem.value(4)).toDouble());
    selItemModel->setData(selItemModel->index(3,0),intTreeModel->data(selItem.value(1)).toString());
    selItemModel->setData(selItemModel->index(4,0),intTreeModel->data(selItem.value(5)).toString());
    selItemModel->setData(selItemModel->index(5,0),intTreeModel->data(selItem.value(6)).toString());
}

void Activity::updateRow_intTree(QItemSelectionModel *treeSelect)
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
            this->updateSwimBreak(selItem.value(0),treeSelect,selItemModel->data(selItemModel->index(3,0)).toInt());
        }
    }
}

void Activity::addRow_intTree(QItemSelectionModel *treeSelect)
{
    if(selItem.value(0).isValid())
    {
        QString lapName = selItem.value(0).data().toString();
        QString swimType = selItem.value(1).data().toString();
        QModelIndex index = treeSelect->currentIndex();
        intTreeModel->insertRow(index.row(),index.parent());

        intTreeModel->setData(selItem.value(0),lapName);
        intTreeModel->setData(selItem.value(1),swimType);
        intTreeModel->setData(selItem.value(2),"-");
        intTreeModel->setData(selItem.value(3),swim_track);
        intTreeModel->setData(selItem.value(4),"00:00");
        intTreeModel->setData(selItem.value(5),"-");
        intTreeModel->setData(selItem.value(6),"00:00");
        intTreeModel->setData(selItem.value(7),0.0);
        intTreeModel->setData(selItem.value(8),0);
    }
}

void Activity::removeRow_intTree(QItemSelectionModel *treeSelect)
{
    if(selectInt)
    {
        if(selItem.value(0).isValid())
        {
            QModelIndex index = treeSelect->currentIndex();
            intTreeModel->removeRow(index.row(),index.parent());
        }
        this->recalcIntTree();
    }
    else
    {
        if(intTreeModel->itemFromIndex(selItem.value(0))->parent() != nullptr)
        {
            int delDura = this->get_timesec(treeSelect->selectedRows(4).at(0).data().toString());
            int strokes = treeSelect->selectedRows(8).at(0).data().toInt();

            if(selItem.value(0).isValid())
            {
                QModelIndex index = treeSelect->currentIndex();
                intTreeModel->removeRow(index.row(),index.parent());
            }

            delDura = delDura + this->get_timesec(treeSelect->selectedRows(4).at(0).data().toString());
            strokes = strokes + treeSelect->selectedRows(8).at(0).data().toInt();

            intTreeModel->setData(treeSelect->selectedRows(4).at(0),this->set_time(delDura));
            intTreeModel->setData(treeSelect->selectedRows(6).at(0),this->set_time(this->calc_lapPace(curr_sport,delDura,swim_track)));
            intTreeModel->setData(treeSelect->selectedRows(7).at(0),this->set_doubleValue(this->calcSpeed(delDura,swim_track,dist_factor),true));
            intTreeModel->setData(treeSelect->selectedRows(8).at(0),strokes);

            this->updateSwimInt(intTreeModel->itemFromIndex(treeSelect->selectedRows(0).at(0))->parent()->index(),treeSelect);
        }
        else
        {
            QModelIndex index = treeSelect->currentIndex();
            intTreeModel->removeRow(index.row(),index.parent());
            this->recalcIntTree();
        }
    }
}

void Activity::updateInterval()
{
    if(isSwim)
    {
        //Only SwimBreak
        QString lapName = swimType.at(0);
        intTreeModel->setData(selItem.value(0),selItemModel->data(selItemModel->index(0,0)));
        intTreeModel->setData(selItem.value(1),lapName);
        intTreeModel->setData(selItem.value(2),0);
        intTreeModel->setData(selItem.value(3),selItemModel->data(selItemModel->index(2,0)));
        intTreeModel->setData(selItem.value(4),selItemModel->data(selItemModel->index(3,0)));
        intTreeModel->setData(selItem.value(5),selItemModel->data(selItemModel->index(1,0)));
        intTreeModel->setData(selItem.value(6),selItemModel->data(selItemModel->index(4,0)));
        intTreeModel->setData(selItem.value(7),0);
    }
    else
    {
        intTreeModel->setData(selItem.value(0),selItemModel->data(selItemModel->index(0,0)));
        intTreeModel->setData(selItem.value(1),selItemModel->data(selItemModel->index(3,0)));
        intTreeModel->setData(selItem.value(2),this->set_time(selItemModel->data(selItemModel->index(1,0)).toInt()));
        intTreeModel->setData(selItem.value(4),selItemModel->data(selItemModel->index(2,0)));
        intTreeModel->setData(selItem.value(5),selItemModel->data(selItemModel->index(4,0)));
        intTreeModel->setData(selItem.value(6),this->set_doubleValue(selItemModel->data(selItemModel->index(5,0)).toDouble(),false));
    }
    this->recalcIntTree();
}

void Activity::updateSwimLap()
{
    intTreeModel->setData(selItem.value(0),selItemModel->data(selItemModel->index(0,0)));
    intTreeModel->setData(selItem.value(1),selItemModel->data(selItemModel->index(1,0)));
    intTreeModel->setData(selItem.value(4),this->set_time(selItemModel->data(selItemModel->index(3,0)).toInt()));
    intTreeModel->setData(selItem.value(6),selItemModel->data(selItemModel->index(4,0)));
    intTreeModel->setData(selItem.value(7),this->set_doubleValue(selItemModel->data(selItemModel->index(5,0)).toDouble(),true));
    intTreeModel->setData(selItem.value(8),selItemModel->data(selItemModel->index(6,0)));
}

void Activity::updateSwimInt(QModelIndex parentIndex,QItemSelectionModel *treeSelect)
{
    QVector<double> intValue(5);
    intValue.fill(0);
    QString lapName;
    int swimLap;
    if(intTreeModel->itemFromIndex(parentIndex)->hasChildren())
    {
        swimLap = 0;
        lapName = intTreeModel->data(parentIndex).toString().split("Int").first();
        do
        {
            intTreeModel->itemFromIndex(parentIndex)->child(swimLap,0)->setData(lapName+QString::number((swimLap+1)*swim_track),Qt::EditRole);
            intValue[0] = intValue[0] + intTreeModel->itemFromIndex(parentIndex)->child(swimLap,3)->text().toDouble();
            intValue[1] = intValue[1] + this->get_timesec(intTreeModel->itemFromIndex(parentIndex)->child(swimLap,4)->text());
            intValue[2] = intValue[2] + this->get_timesec(intTreeModel->itemFromIndex(parentIndex)->child(swimLap,6)->text());
            intValue[3] = intValue[3] + intTreeModel->itemFromIndex(parentIndex)->child(swimLap,7)->text().toDouble();
            intValue[4] = intValue[4] + intTreeModel->itemFromIndex(parentIndex)->child(swimLap,8)->text().toDouble();
            ++swimLap;
        }
        while(intTreeModel->itemFromIndex(parentIndex)->child(swimLap,0) != 0);
        intValue[2] = intValue[2] / swimLap;
        intValue[3] = intValue[3] / swimLap;
    }
    treeSelect->select(parentIndex,QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    intTreeModel->setData(treeSelect->selectedRows(0).at(0),lapName+"Int_"+QString::number(intValue[0]));
    intTreeModel->setData(treeSelect->selectedRows(2).at(0),swimLap);
    intTreeModel->setData(treeSelect->selectedRows(3).at(0),intValue[0]);
    intTreeModel->setData(treeSelect->selectedRows(4).at(0),this->set_time(intValue[1]));
    intTreeModel->setData(treeSelect->selectedRows(6).at(0),this->set_time(intValue[2]));
    intTreeModel->setData(treeSelect->selectedRows(7).at(0),this->set_doubleValue(intValue[3],true));
    intTreeModel->setData(treeSelect->selectedRows(8).at(0),intValue[4]);

    this->updateSwimBreak(parentIndex,treeSelect,intValue[1]);
    this->recalcIntTree();
    this->calc_SwimTimeInZones();
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
    int startTime = 0;
    int intTime = 0;
    double workDist = 0;
    int rowCount = intTreeModel->rowCount();
    if(isSwim)
    {
        int intCounter = 1;
        int lapDist;
        QString lapName;
        QString breakName = settings::get_generalValue("breakname");

        for(int row = 0; row < rowCount; ++row)
        {
           intTime = this->get_timesec(intTreeModel->data(intTreeModel->index(row,4)).toString());
           lapDist = intTreeModel->data(intTreeModel->index(row,3)).toDouble();
           workDist = workDist + lapDist;
           lapName = intTreeModel->data(intTreeModel->index(row,0)).toString();
           if(!lapName.contains(breakName))
           {
                lapName = QString::number(intCounter)+"_Int_"+QString::number(lapDist);
                ++intCounter;
           }
           intTreeModel->setData(intTreeModel->index(row,0),lapName);
           intTreeModel->setData(intTreeModel->index(row,4),this->set_time(intTime));
           intTreeModel->setData(intTreeModel->index(row,5),this->set_time(startTime));
           startTime = startTime+intTime;
        }
    }
    else
    {
        for(int row = 0; row < rowCount; ++row)
        {
           startTime = this->get_timesec(intTreeModel->data(intTreeModel->index(row,2)).toString());
           intTime = this->get_timesec(intTreeModel->data(intTreeModel->index(row,1)).toString());
           workDist = workDist + intTreeModel->data(intTreeModel->index(row,4)).toDouble();

           intTreeModel->setData(intTreeModel->index(row,1),this->set_time(intTime));
           intTreeModel->setData(intTreeModel->index(row,2),this->set_time(startTime));
           intTreeModel->setData(intTreeModel->index(row,3),this->set_doubleValue(workDist,true));
        }
    }
    ride_info.insert("Distance",QString::number(workDist/dist_factor));
}


void Activity::updateSwimModel()
{
    int child;
    double dist = 0;
    int time = 0;
    int swimStyle = 0;
    int lapTime = 0;
    int strokes = 0;

    for(int row = 0,swimRow = 0; row < intTreeModel->rowCount(); ++row)
    {
        if(intTreeModel->item(row,0)->hasChildren())
        {
            child = 0;
            do
            {
                swimStyle = swimType.indexOf(intTreeModel->item(row,0)->child(child,1)->text());
                lapTime = this->get_timesec(intTreeModel->item(row,0)->child(child,4)->text());
                strokes = intTreeModel->item(row,0)->child(child,8)->text().toInt();

                swimModel->setData(swimModel->index(swimRow,1),time);
                swimModel->setData(swimModel->index(swimRow,2),dist);
                swimModel->setData(swimModel->index(swimRow,3),swimStyle);
                swimModel->setData(swimModel->index(swimRow,5),lapTime);
                swimModel->setData(swimModel->index(swimRow,8),strokes);

                time = time + lapTime;
                dist = dist + (intTreeModel->item(row,0)->child(child,3)->text().toDouble() / dist_factor);
                ++child;
                ++swimRow;
            }
            while(intTreeModel->item(row,0)->child(child,0) != 0);
        }
        else
        {
            swimStyle = swimType.indexOf(intTreeModel->item(row,1)->text());
            lapTime = this->get_timesec(intTreeModel->item(row,4)->text());
            strokes = intTreeModel->item(row,8)->text().toInt();

            swimModel->setData(swimModel->index(swimRow,0),time);
            swimModel->setData(swimModel->index(swimRow,1),dist);
            swimModel->setData(swimModel->index(swimRow,2),swimStyle);
            swimModel->setData(swimModel->index(swimRow,3),lapTime);
            swimModel->setData(swimModel->index(swimRow,4),strokes);

            time = time + lapTime;
            dist = dist + (intTreeModel->item(row,3)->text().toDouble() / dist_factor);
            ++swimRow;
        }

    }
    this->updateIntModel(5,4);
}

void Activity::updateIntModel(int startCol,int duraCol)
{
    int intStart = 0;
    int intStop = 0;

    for(int row = 0; row < intTreeModel->rowCount(); ++row)
    {
        intStart = this->get_timesec(intTreeModel->data(intTreeModel->index(row,startCol)).toString());
        intStop = intStart + this->get_timesec(intTreeModel->data(intTreeModel->index(row,duraCol)).toString());

        intModel->setData(intModel->index(row,0),intTreeModel->data(intTreeModel->index(row,0)).toString().trimmed());
        intModel->setData(intModel->index(row,1),intStart);
        intModel->setData(intModel->index(row,2),intStop);
        intModel->setData(intModel->index(row,3),intTreeModel->data(intTreeModel->index(row,4)).toDouble());
    }

    this->updateSampleModel(++intStop);
}

void Activity::updateSampleModel(int rowcount)
{
    int sampRowCount = rowcount;
    new_dist.resize(sampRowCount);
    calc_speed.resize(sampRowCount);
    calc_cadence.resize(sampRowCount);
    double msec = 0.0;
    int int_start,int_stop,sportpace = 0,swimLaps;
    double overall = 0.0,lowLimit = 0.0,limitFactor = 0.0;
    double swimPace,swimSpeed,swimCycle;
    bool isBreak = true;
    if(!isSwim && curr_sport != settings::isTria)
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

    if(isSwim)
    {
        swimLaps = 0;
        for(int sLap = 0; sLap < swimModel->rowCount();++sLap)
        {
            int_start = swimModel->data(swimModel->index(sLap,1,QModelIndex()),Qt::DisplayRole).toInt();
            swimPace = swimModel->data(swimModel->index(sLap,2,QModelIndex()),Qt::DisplayRole).toDouble();
            swimSpeed = swimModel->data(swimModel->index(sLap,4,QModelIndex()),Qt::DisplayRole).toDouble();
            swimCycle = swimModel->data(swimModel->index(sLap,3,QModelIndex()),Qt::DisplayRole).toDouble();

            if(sLap == swimModel->rowCount()-1)
            {
                int_stop = sampRowCount-1;
            }
            else
            {
                int_stop = swimModel->data(swimModel->index(sLap+1,1,QModelIndex()),Qt::DisplayRole).toInt();
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
            sampleModel->setData(sampleModel->index(row,0,QModelIndex()),row);
            sampleModel->setData(sampleModel->index(row,1,QModelIndex()),new_dist[row]);
            sampleModel->setData(sampleModel->index(row,2,QModelIndex()),calc_speed[row]);
            sampleModel->setData(sampleModel->index(row,3,QModelIndex()),calc_cadence[row]);
        }
    }
    else
    {
      for(int c_int = 0; c_int < intModel->rowCount(); ++c_int)
      {
         int_start = intModel->data(intModel->index(c_int,1,QModelIndex())).toInt();
         int_stop = intModel->data(intModel->index(c_int,2,QModelIndex())).toInt();
         msec = intModel->data(intModel->index(c_int,3,QModelIndex())).toDouble() / (int_stop-int_start);

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
                    calc_cadence[c_dist] = sampleModel->data(sampleModel->index(c_dist,3,QModelIndex())).toDouble();
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
              sampleModel->setData(sampleModel->index(row,0,QModelIndex()),row);
              sampleModel->setData(sampleModel->index(row,1,QModelIndex()),new_dist[row]);
              sampleModel->setData(sampleModel->index(row,2,QModelIndex()),calc_speed[row]);
              sampleModel->setData(sampleModel->index(row,3,QModelIndex()),calc_cadence[row]);
          }
      }

      if(curr_sport == settings::isRun)
      {
          for(int row = 0; row < sampRowCount;++row)
          {
              sampleModel->setData(sampleModel->index(row,0,QModelIndex()),row);
              sampleModel->setData(sampleModel->index(row,1,QModelIndex()),new_dist[row]);
              sampleModel->setData(sampleModel->index(row,2,QModelIndex()),calc_speed[row]);
          }
      }

      if(curr_sport == settings::isTria)
      {
          double triValue = 0,sportValue = 0;

          for(int row = 0; row < sampRowCount;++row)
          {
              sampleModel->setData(sampleModel->index(row,0,QModelIndex()),row);
              sampleModel->setData(sampleModel->index(row,1,QModelIndex()),new_dist[row]);
              sampleModel->setData(sampleModel->index(row,2,QModelIndex()),calc_speed[row]);
              sampleModel->setData(sampleModel->index(row,3,QModelIndex()),calc_cadence[row]);
          }
          this->hasOverride = true;
          sportValue = round(this->estimate_stress(settings::isSwim,this->set_time(this->get_int_pace(0,settings::isSwim)/10),this->get_int_duration(0)));
          this->overrideData.insert("swimscore",QString::number(sportValue));
          triValue = triValue + sportValue;
          sportValue = round(this->estimate_stress(settings::isBike,QString::number(this->get_int_value(2,4)),this->get_int_duration(2)));
          this->overrideData.insert("skiba_bike_score",QString::number(sportValue));
          triValue = triValue + sportValue;
          sportValue = round(this->estimate_stress(settings::isRun,this->set_time(this->get_int_pace(4,settings::isRun)),this->get_int_duration(4)));
          this->overrideData.insert("govss",QString::number(sportValue));
          triValue = triValue + sportValue;
          this->overrideData.insert("triscore",QString::number(triValue));
      }
}

void Activity::writeChangedData()
{
    this->init_jsonFile();
    this->write_actModel("INTERVALS",intModel,&intList);
    this->write_actModel("SAMPLES",sampleModel,&sampList);
    this->write_xdataModel(curr_sport,swimModel);
    this->write_jsonFile();
}


void Activity::calc_SwimTimeInZones()
{
    //int z0=0,z1=0,z2=0,z3=0,z4=0,z5=0,z6=0,z7=0;
    double paceSec;
    int zoneValue;
    QString currZone;

    for(int i = 0; i < sampleModel->rowCount(); i++)
    {
        currZone = "REKOM";
        paceSec = sampleModel->data(sampleModel->index(i,2,QModelIndex())).toDouble();

        for(QHash<QString,QPair<double,double>>::const_iterator it = swimSpeed.cbegin(), end = swimSpeed.cend(); it != end; ++it)
        {
            if(paceSec >= it.value().first && paceSec <= it.value().second)
            {
                currZone = it.key();
            }
        }
        zoneValue = paceTimeInZone.value(currZone);
        paceTimeInZone.insert(currZone,++zoneValue);
        /*
        if(paceSec <= swimTime[0])
        {
            ++z0;
            swimTimezone[0] = z0;
        }

        if(paceSec > swimTime[0] && paceSec <= swimTime[1])
        {
            ++z1;
            swimTimezone[1] = z1;
        }

        if(paceSec > swimTime[1] && paceSec <= swimTime[2])
        {
            ++z2;
            swimTimezone[2] = z2;
        }

        if(paceSec > swimTime[2] && paceSec <= swimTime[3])
        {
            ++z3;
            swimTimezone[3] = z3;
        }

        if(paceSec > swimTime[3] && paceSec <= swimTime[4])
        {
            ++z4;
            swimTimezone[4] = z4;
        }

        if(paceSec > swimTime[4] && paceSec <= swimTime[5])
        {
            ++z5;
            swimTimezone[5] = z5;
        }

        if(paceSec > swimTime[5] && paceSec <= swimTime[6])
        {
            ++z6;
            swimTimezone[6] = z6;
        }
        if(paceSec > swimTime[6])
        {
            ++z7;
            swimTimezone[7] = z7;
        }
        */
    }

    for(QHash<QString,int>::const_iterator it =  paceTimeInZone.cbegin(), end = paceTimeInZone.cend(); it != end; ++it)
    {
        qDebug() << it.key() << it.value();
    }

    //Calc Swim Data
    move_time = 0;
    for(int i = 1; i <= 7;i++)
    {
        move_time = move_time + swimTimezone[i];
    }

    swim_pace = ceil(static_cast<double>(move_time) / (sampleModel->data(sampleModel->index(sampleModel->rowCount()-1,1,QModelIndex())).toDouble()*10));

    double goal = sqrt(pow(static_cast<double>(swim_pace),3.0))/10;
    swim_sri = static_cast<double>(pace_cv) / goal;

    //this->calc_HF_TimeInZone();
}

void Activity::calc_HF_TimeInZone()
{
     double hf_factor[] = {0.125,0.25,0.5,0.75};

     //REKOM: Z0*0,125 + Z1 + Z2*0,5
     hfTimezone[0] = ceil((swimTimezone[0]*hf_factor[0]) + swimTimezone[1] + (swimTimezone[2]*hf_factor[2]));

     //END: Z0*0,5 + Z2*0,5 + Z3*0,75
     hfTimezone[1] = ceil((swimTimezone[0]*hf_factor[2]) + (swimTimezone[2]*hf_factor[2]) + (swimTimezone[3]*hf_factor[3]));

     //TEMP: Z0*0,25 + Z3*0,25 + Z4*0,75 + Z5*0,5
     hfTimezone[2] = ceil((swimTimezone[0]*hf_factor[1]) + (swimTimezone[3]*hf_factor[1]) + swimTimezone[4] + (swimTimezone[5]*hf_factor[2]));

     //LT: Z5*0,25 + Z6*0,75 + Z7*0,5
     hfTimezone[3] = ceil((swimTimezone[5]*hf_factor[1]) + (swimTimezone[6]*hf_factor[3]) + (swimTimezone[7]*hf_factor[2]));

     //VO2: Z5*0,25 + Z6*0,125 + Z7*0,25
     hfTimezone[4] = ceil((swimTimezone[5]*hf_factor[1]) + (swimTimezone[6]*hf_factor[0]) + (swimTimezone[7]*hf_factor[1]));

     //AC: Z6*0,125 + Z7*0,125
     hfTimezone[5] = ceil((swimTimezone[6]*hf_factor[0]) + (swimTimezone[7]*hf_factor[0]));

     //NEURO: Z7*0,125
     hfTimezone[6] = ceil(swimTimezone[7]*hf_factor[0]);

    hf_avg = 0;

    //set HF Avg

    //Calc Total Work and Calories
    double totalWork = this->calc_totalWork(tagData.value("Weight").toDouble(),hf_avg,move_time) * swim_sri;
    ride_info.insert("Total Cal",QString::number(ceil((totalWork*4)/4.184)));
    ride_info.insert("Total Work",QString::number(ceil(totalWork)));
    ride_info.insert("AvgHF",QString::number(hf_avg));
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

int Activity::get_int_duration(int row)
{
    int duration;

    duration = intModel->data(intModel->index(row,2,QModelIndex()),Qt::DisplayRole).toInt() - intModel->data(intModel->index(row,1,QModelIndex()),Qt::DisplayRole).toInt();

    return duration;
}

double Activity::get_int_distance(int row)
{
    double dist,dist_start,dist_stop;
    int int_start,int_stop;

    if(row == 0)
    {
        int_stop = intModel->data(intModel->index(row,2,QModelIndex()),Qt::DisplayRole).toInt();
        dist = sampleModel->data(sampleModel->index(int_stop,1,QModelIndex()),Qt::DisplayRole).toDouble();
    }
    else
    {
        int_start = intModel->data(intModel->index(row,1,QModelIndex()),Qt::DisplayRole).toInt();
        int_stop = intModel->data(intModel->index(row,2,QModelIndex()),Qt::DisplayRole).toInt();
        dist_start = sampleModel->data(sampleModel->index(int_start,1,QModelIndex()),Qt::DisplayRole).toDouble();
        dist_stop = sampleModel->data(sampleModel->index(int_stop,1,QModelIndex()),Qt::DisplayRole).toDouble();
        dist = dist_stop - dist_start;
    }

    return dist;
}

int Activity::get_int_pace(int row,QString lapName)
{
    int pace;

    if(isSwim)
    {
        if(lapName.contains(settings::get_generalValue("breakname")))
        {
            pace = 0;
        }
        else
        {
            pace = this->get_int_duration(row) / (this->get_int_distance(row)*10);
        }
    }
    else
    {
        pace = this->get_int_duration(row) / this->get_int_distance(row);
    }

    return pace;
}

int Activity::get_swim_laps(int row)
{
    return round((this->get_int_distance(row)*dist_factor)/swim_track);
}

double Activity::get_int_value(int row,int col)
{
    double value = 0.0;
    int int_start,int_stop;
    int_start = intModel->data(intModel->index(row,1,QModelIndex())).toInt();
    int_stop = intModel->data(intModel->index(row,2,QModelIndex())).toInt();

    for(int i = int_start; i < int_stop; ++i)
    {
        value = value + sampleModel->data(sampleModel->index(i,col,QModelIndex())).toDouble();
    }
    value = value / (int_stop - int_start);

    return value;
}

double Activity::get_int_speed(int row)
{
    double speed,pace;

    if(isSwim)
    {
        pace = this->get_int_duration(row) / (intModel->data(intModel->index(row,3,QModelIndex())).toDouble() / dist_factor);
    }
    else
    {
        pace = this->get_int_duration(row) / (intModel->data(intModel->index(row,3,QModelIndex())).toDouble()); // *10.0
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

void Activity::reset_avgSelection()
{
    avgCounter = 0;
    avgValues.fill(0);

    for(int i = 0; i < avgModel->rowCount();++i)
    {
        avgModel->setData(avgModel->index(i,0),"-");
    }

    for(QHash<int,QModelIndex>::const_iterator it =  avgItems.cbegin(), end = avgItems.cend(); it != end; ++it)
    {
        intTreeModel->setData(it.value(),"-");
        intTreeModel->setData(it.value(),0,Qt::UserRole+1);
    }
    avgItems.clear();
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

void Activity::set_avgValues(int counter,int factor)
{
    avgCounter = counter;

    if(counter != 0)
    {
        if(isSwim)
        {
            avgValues[1] = avgValues[1] + (static_cast<double>(this->get_timesec(intTreeModel->data(selItem.value(4)).toString()))*factor);
            avgValues[2] = avgValues.at(2) + (static_cast<double>(this->get_timesec(intTreeModel->data(selItem.value(6)).toString()))*factor);
            avgValues[3] = avgValues.at(3) + (intTreeModel->data(selItem.value(3)).toDouble()*factor);
        }
        else
        {
            avgValues[1] = avgValues.at(1) + (static_cast<double>(this->get_timesec(intTreeModel->data(selItem.value(1)).toString()))*factor);
            avgValues[2] = avgValues.at(2) + (static_cast<double>(this->get_timesec(intTreeModel->data(selItem.value(5)).toString()))*factor);
            avgValues[3] = avgValues.at(3) + (intTreeModel->data(selItem.value(4)).toDouble()*factor);

            if(curr_sport == settings::isBike)
            {
                avgValues[4] = avgValues.at(4) + (intTreeModel->data(selItem.value(7)).toDouble()*factor);
                avgValues[5] = avgValues.at(5) + (intTreeModel->data(selItem.value(8)).toDouble()*factor);
                avgModel->setData(avgModel->index(4,0),round(avgValues[4]/avgCounter));
                avgModel->setData(avgModel->index(5,0),round(avgValues[5]/avgCounter));
            }
        }

        avgModel->setData(avgModel->index(0,0),avgCounter);
        avgModel->setData(avgModel->index(1,0),this->set_time(round(avgValues[1]/avgCounter)));
        avgModel->setData(avgModel->index(2,0),this->set_time(round(avgValues[2]/avgCounter)));
        avgModel->setData(avgModel->index(3,0),avgValues[3]/avgCounter);
    }
    else
    {
        this->reset_avgSelection();
    }
}
