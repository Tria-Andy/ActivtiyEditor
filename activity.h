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

class jsonHandler;

class Activity : public calculation
{
private:
    jsonHandler *jsonhandler;
    QList<QStandardItem*> setIntRow(int);
    QList<QStandardItem*> setSwimLap(int,QString);
    QSortFilterProxyModel *swimProxy;
    QString v_date,curr_sport;
    QStringList ride_items,swimType;
    QMap<int,QStringList> itemHeader;
    QVector<double> calc_speed,calc_cadence,p_swim_time,new_dist;
    QVector<int> p_swim_timezone,p_hf_timezone,hf_zone_avg;
    double swim_track,swim_cv,swim_sri,polishFactor,hf_threshold,hf_max;
    int dist_factor,avgCounter,pace_cv,zone_count,move_time,swim_pace,hf_avg;
    bool changeRowCount,isUpdated,selectInt;

    //Functions    
    void set_time_in_zones(bool);
    void recalcIntTree();
    void set_edit_samp_model(int);
    void updateSwimInt(QModelIndex,QItemSelectionModel*);
    void updateSwimBreak(QModelIndex,QItemSelectionModel*,int);
    void updateInterval();
    void updateIntModel();
    void calcAvgValues();
    double get_int_value(int,int,bool);
    double interpolate_speed(int,int,double);

    int get_swim_laps(int);
    int check_is_intervall(int);
    int get_zone_values(double,int,bool);

public:
    explicit Activity();
    void setUpdateFlag(bool value) {isUpdated = value;}
    void prepareData();
    void build_intTree();
    void showSwimLap(bool);
    void showInterval(bool);

    void updateSwimLap();
    QHash<int,QModelIndex> selItem;
    void set_additional_ride_info();
    void act_reset();
    QStandardItemModel *int_model,*samp_model,*curr_act_model,*edit_int_model,*xdata_model,*swim_xdata,*edit_samp_model;
    QStandardItemModel *swim_pace_model, *swim_hf_model,*intTreeModel,*selItemModel,*avgModel;
    QMap<QString,QString> ride_info;
    QVector<double> sampSpeed,avgValues;

    //Recalculation
    QStandardItemModel * set_int_model_pointer(bool);
    QStandardItemModel * set_samp_model_pointer(bool);
    void updateIntTreeRow(QItemSelectionModel *);
    double get_int_distance(int);
    int get_int_duration(int);
    int get_int_pace(int);
    double get_int_speed(int);
    double polish_SpeedValues(double,double,double,bool);

    //Value Getter and Setter
    void set_polishFactor(double vFactor) {polishFactor = vFactor;}
    int get_header_num();
    void set_sport(QString a_sport) {curr_sport = a_sport;}
    QString get_sport() {return curr_sport;}


    //Averages
    void reset_avg();
    void set_avgValues(int,int);
    int get_dist_factor() {return dist_factor;}

    //Swim Calculations
    void set_swim_data();
    int get_move_time() {return move_time;}
    int get_swim_pace() {return swim_pace;}
    double get_swim_sri() { return swim_sri;}
    void set_hf_zone_avg(double,double,int);
    double get_hf_zone_avg();
    int get_hf_avg() {return hf_avg;}
    void set_hf_time_in_zone();
    void set_swim_track(double trackLen) {swim_track = trackLen;}
    double get_swim_track() {return swim_track;}   
};

#endif // ACTIVITY_H
