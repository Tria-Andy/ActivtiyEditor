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

#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <QFile>
#include <QFileDialog>
#include <QStandardItemModel>
#include "settings.h"
#include "jsonhandler.h"
#include "calculation.h"

class Activity : public jsonHandler
{
private:
    QList<QStandardItem*> setIntRow(int);
    QList<QStandardItem*> setSwimLap(int,QString);
    QSortFilterProxyModel *swimProxy;
    QString v_date,curr_sport;
    QMap<int,QStringList> itemHeader,avgHeader;
    QHash<QString,int> paceTimeInZone,hfTimeInZone,hfZoneAvg;
    QHash<QString,QPair<double,double>> swimSpeed;
    QStringList ride_items,swimType,levels;
    QVector<double> calc_speed,calc_cadence,swimTime,new_dist;
    QVector<int> swimTimezone,hfTimezone;
    double swim_track,swim_cv,swim_sri,polishFactor,hf_threshold,hf_max;
    int dist_factor,avgCounter,pace_cv,zone_count,move_time,swim_pace,hf_avg;
    bool isSwim,changeRowCount,isUpdated,selectInt;

    //Functions    
    void readJsonFile(QString,bool);
    void recalcIntTree();
    void updateSampleModel(int);
    int build_swimModel(bool,QString,int,int,int,int,double);
    void updateSwimLap();
    void updateSwimInt(QModelIndex,QItemSelectionModel*);
    void updateSwimBreak(QModelIndex,QItemSelectionModel*,int);
    void updateInterval();
    void calcAvgValues();
    double get_int_value(int,int);
    double interpolate_speed(int,int,double);

    //Swim Calculations
    void calc_HF_TimeInZone();
    void calc_SwimTimeInZones();
    int get_swim_laps(int);
    int get_zone_values(double,int,bool);

public:
    explicit Activity(QString jsonfile = QString(),bool intAct = false);
    void set_selectedItem(QItemSelectionModel*);
    void setUpdateFlag(bool value) {isUpdated = value;}
    void prepareData();
    void build_intTree();
    void showSwimLap(bool);
    void showInterval(bool);
    void updateIntModel(int,int);
    void updateSwimModel();
    void writeChangedData();
    QHash<int,QModelIndex> selItem,avgItems;
    QHash<QString,int> swimPace,swimHF;
    void act_reset();
    QStandardItemModel *intModel,*sampleModel,*xdata_model,*swimModel,*intTreeModel,*selItemModel,*avgModel;
    QMap<QString,QString> ride_info;
    QVector<double> sampSpeed,avgValues;

    //Recalculation
    void updateRow_intTree(QItemSelectionModel *);
    void addRow_intTree(QItemSelectionModel *);
    void removeRow_intTree(QItemSelectionModel *);
    double get_int_distance(int);
    int get_int_duration(int);
    int get_int_pace(int,QString);
    double get_int_speed(int);
    double polish_SpeedValues(double,double,double,bool);

    //Value Getter and Setter
    void set_polishFactor(double vFactor) {polishFactor = vFactor;}
    void set_sport(QString a_sport) {curr_sport = a_sport;}
    QString get_sport() {return curr_sport;}


    //Averages
    void reset_avgSelection();
    void set_avgValues(int,int);
    int get_dist_factor() {return dist_factor;}

    int get_move_time() {return move_time;}
    int get_swim_pace() {return swim_pace;}
    double get_swim_sri() { return swim_sri;}
    double get_hf_zone_avg();
    int get_hf_avg() {return hf_avg;}

    void set_swim_track(double trackLen) {swim_track = trackLen;}
    double get_swim_track() {return swim_track;}   
};

#endif // ACTIVITY_H
