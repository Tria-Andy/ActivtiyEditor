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

#include "jsonhandler.h"
#include "settings.h"

jsonHandler::jsonHandler(bool readFlag,QString jsonfile, Activity *p_act)
{   
    hasFile = readFlag;
    hasXdata = false;
    if(readFlag)
    {
        curr_act = p_act;
        this->read_json(jsonfile);
    }
}

void jsonHandler::fill_qmap(QMap<QString, QString> *qmap,QJsonObject *objItem)
{
    QString keyValue;
    for(int i = 0; i < objItem->keys().count(); ++i)
    {
        keyValue = objItem->keys().at(i);
        qmap->insert(keyValue,objItem->value(keyValue).toString().trimmed());
        if(keyValue == "OVERRIDES") hasOverride = true;
    }
}

void jsonHandler::fill_keyList(QStringList *targetList,QMap<int, QString> *map, QStringList *list)
{
    for(int i = 0; i < map->count(); ++i)
    {
        if(list->contains(map->value(i)))
        {
            targetList->insert(i,map->value(i));
        }
    }
    for(int x = 0; x < list->count();++x)
    {
        if(!targetList->contains(list->at(x)))
        {
            (*targetList) << list->at(x);                  
        }
    }
}

void jsonHandler::fill_model(QStandardItemModel *model, QJsonArray *jArray, QStringList *list)
{
    QJsonObject objItem;
    for(int row = 0; row < jArray->count(); ++row)
    {
        objItem = jArray->at(row).toObject();

        for(int col = 0; col < list->count(); ++col)
        {
            model->setData(model->index(row,col,QModelIndex()),objItem[list->at(col)].toVariant());
        }
    }
}

void jsonHandler::fill_list(QJsonArray *jArray,QStringList *list)
{
    for(int i = 0; i < jArray->count(); ++i)
    {
        (*list) << jArray->at(i).toString();
    }
}

QJsonObject jsonHandler::mapToJson(QMap<QString,QString> *map)
{
    QJsonObject item;

    for(QMap<QString,QString>::const_iterator it =  map->cbegin(), end = map->cend(); it != end; ++it)
    {
        if(it.key() == "RECINTSECS")
        {
            item.insert(it.key(),1);
        }
        else
        {
            item.insert(it.key(),it.value()+" ");
        }
    }
    return item;
}

QJsonArray jsonHandler::listToJson(QStringList *list)
{
    QJsonArray jArray;

    for(int i = 0; i < list->count(); ++i)
    {
        jArray.insert(i,list->at(i));
    }
    return jArray;
}

QJsonArray jsonHandler::modelToJson(QStandardItemModel *model, QStringList *list)
{
    QJsonArray jArray;
    QVariant modelValue;
    for(int row = 0; row < model->rowCount(); ++row)
    {
        QJsonObject item_array;
        for(int col = 0; col < list->count(); ++col)
        {
            modelValue = model->data(model->index(row,col,QModelIndex()));
            if(modelValue.isValid())
            {
                item_array.insert(list->at(col),QJsonValue::fromVariant(modelValue));
            }
        }
        jArray.insert(row,item_array);
    }
    return jArray;
}

void jsonHandler::read_json(QString jsonfile)
{
    hasOverride = false;
    QJsonObject itemObject;
    QJsonArray itemArray;
    QStringList valueList;
    QString stgValue;
    QMap<int,QString> mapValues;
    QJsonDocument d = QJsonDocument::fromJson(jsonfile.toUtf8());
    QJsonObject jsonobj = d.object();

    QJsonObject item_ride = jsonobj.value(QString("RIDE")).toObject();
    this->fill_qmap(&rideData,&item_ride);

    if(hasOverride)
    {
        QJsonObject objOverride,objValue;
        itemArray = item_ride["OVERRIDES"].toArray();
        for(int i = 0; i < itemArray.count(); ++i)
        {
            objOverride = itemArray.at(i).toObject();
            objValue = objOverride[objOverride.keys().first()].toObject();
            overrideData.insert(objOverride.keys().first(),objValue["value"].toString());
        }
        curr_act->setUpdateFlag(hasOverride);
    }

    itemObject = item_ride.value(QString("TAGS")).toObject();
    this->fill_qmap(&tagData,&itemObject);
    curr_act->set_sport(tagData.value("Sport"));
    valueList = settings::get_listValues("JsonFile");

    curr_act->ride_info.insert("Date:",rideData.value("STARTTIME"));
    fileName = tagData.value("Filename").trimmed();

    for(int i = 0; i < valueList.count();++i)
    {
        stgValue = valueList.at(i);
        curr_act->ride_info.insert(stgValue+":",itemObject[stgValue].toString());
    }
    stgValue = QString();
    valueList = QStringList();
    itemArray = item_ride["INTERVALS"].toArray();
    valueList << itemArray.at(0).toObject().keys();
    mapValues = settings::get_intList();
    this->fill_keyList(&intList,&mapValues,&valueList);
    curr_act->intModel = new QStandardItemModel(itemArray.count(),intList.count()+3);
    this->fill_model(curr_act->intModel,&itemArray,&intList);

    valueList = QStringList();

    itemArray = item_ride["SAMPLES"].toArray();
    valueList << itemArray.at(0).toObject().keys();
    mapValues = settings::get_sampList();
    this->fill_keyList(&sampList,&mapValues,&valueList);
    curr_act->sampleModel = new QStandardItemModel(itemArray.count(),sampList.count());
    this->fill_model(curr_act->sampleModel,&itemArray,&sampList);

    if(curr_act->get_sport() == settings::isRun)
    {
        int avgHF = 0;
        int posHF = sampList.indexOf("HR");
        int sampCount = curr_act->sampleModel->rowCount();
        for(int i = 0;i < sampCount; ++i)
        {
            avgHF = avgHF + curr_act->sampleModel->data(curr_act->sampleModel->index(i,posHF,QModelIndex())).toInt();
        }
        avgHF = (avgHF / sampCount);
        overrideData.insert("total_work",QString::number(this->calc_totalWork(tagData.value("Weight").toDouble(),avgHF,sampCount)));
        curr_act->ride_info.insert("Total Work:",overrideData.value("total_work"));
        hasOverride = true;
    }

    if(item_ride.contains("XDATA"))
    {
        hasXdata = true;
        if(curr_act->get_sport() == settings::isSwim)
        {
            QJsonObject item_xdata = item_ride["XDATA"].toArray().at(0).toObject();
            this->fill_qmap(&xData,&item_xdata);

            itemArray = QJsonArray();
            itemArray = item_xdata["UNITS"].toArray();
            this->fill_list(&itemArray,&xdataUnits);

            itemArray = QJsonArray();
            itemArray = item_xdata["VALUES"].toArray();
            this->fill_list(&itemArray,&xdataValues);

            itemArray = QJsonArray();
            itemArray = item_xdata["SAMPLES"].toArray();
            QJsonObject obj_xdata = itemArray.at(0).toObject();

            curr_act->xdata_model = new QStandardItemModel(itemArray.count(),(obj_xdata.keys().count()+xdataValues.count()));

            double swim_track = tagData.value("Pool Length").toDouble();
            curr_act->set_swim_track(swim_track);

            for(int i = 0; i < itemArray.count(); ++i)
            {
                obj_xdata = itemArray.at(i).toObject();
                QJsonArray arrValues = obj_xdata["VALUES"].toArray();
                curr_act->xdata_model->setData(curr_act->xdata_model->index(i,0,QModelIndex()),obj_xdata["SECS"].toInt());
                curr_act->xdata_model->setData(curr_act->xdata_model->index(i,1,QModelIndex()),obj_xdata["KM"].toDouble());
                curr_act->xdata_model->setData(curr_act->xdata_model->index(i,2,QModelIndex()),arrValues.at(0).toInt());
                curr_act->xdata_model->setData(curr_act->xdata_model->index(i,3,QModelIndex()),arrValues.at(1).toDouble());
                curr_act->xdata_model->setData(curr_act->xdata_model->index(i,4,QModelIndex()),arrValues.at(2).toInt());
            }
        }
    }
    curr_act->prepareData();
}

void jsonHandler::write_json()
{
    QJsonDocument jsonDoc;
    QJsonObject rideFile,item_ride;
    QJsonArray intArray;

    item_ride = mapToJson(&rideData);
    item_ride["TAGS"] = mapToJson(&tagData);

    if(hasFile)
    {
        p_int = curr_act->intModel;
        p_samp = curr_act->sampleModel;
        item_ride["INTERVALS"] = modelToJson(p_int,&intList);
        item_ride["SAMPLES"] = modelToJson(p_samp,&sampList);
    }

    if(hasOverride)
    {
        int i = 0;
        for(QMap<QString,QString>::const_iterator it =  overrideData.cbegin(), end = overrideData.cend(); it != end; ++it,++i)
        {
            QJsonObject objOverride,objValue;
            objValue.insert("value",it.value());
            objOverride.insert(it.key(),objValue);
            intArray.insert(i,objOverride);
        }
        item_ride["OVERRIDES"] = intArray;
    }

    if(hasXdata)
    {
        QJsonArray item_xdata;
        QJsonObject xdataObj;

        xdataObj.insert("NAME",xData.value("NAME"));
        xdataObj.insert("UNITS",listToJson(&xdataUnits));
        xdataObj.insert("VALUES",listToJson(&xdataValues));

        intArray = QJsonArray();
        for(int i = 0; i < curr_act->swimModel->rowCount(); ++i)
        {
            QJsonObject item_array;
            QJsonArray value_array;
            item_array.insert("SECS",QJsonValue::fromVariant(curr_act->swimModel->data(curr_act->swimModel->index(i,1,QModelIndex()))));
            item_array.insert("KM",QJsonValue::fromVariant(curr_act->swimModel->data(curr_act->swimModel->index(i,2,QModelIndex()))));
            value_array.insert(0,QJsonValue::fromVariant(curr_act->swimModel->data(curr_act->swimModel->index(i,3,QModelIndex()))));
            value_array.insert(1,QJsonValue::fromVariant(curr_act->swimModel->data(curr_act->swimModel->index(i,5,QModelIndex()))));
            value_array.insert(2,QJsonValue::fromVariant(curr_act->swimModel->data(curr_act->swimModel->index(i,8,QModelIndex()))));
            item_array["VALUES"] = value_array;
            intArray.insert(i,item_array);
        }
        xdataObj.insert("SAMPLES",intArray);
        item_xdata.insert(0,xdataObj);
        item_ride["XDATA"] = item_xdata;
    }

    rideFile["RIDE"] = item_ride;
    jsonDoc.setObject(rideFile);
    this->write_file(jsonDoc);
}

void jsonHandler::write_file(QJsonDocument jsondoc)
{
    //QFile file(settings::get_gcInfo("gcpath") + QDir::separator() + fileName);
    QFile file(QCoreApplication::applicationDirPath() + QDir::separator() + fileName);
    if(!file.open(QFile::WriteOnly))
    {
        qDebug() << "File not open!";
        return;
    }

    //file.write(jsondoc.toJson(QJsonDocument::Compact));
    file.write(jsondoc.toJson());
    file.flush();
    file.close();
}
