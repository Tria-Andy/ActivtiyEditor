#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    settings::loadSettings();
    fileModel = new QStandardItemModel;
    fileModel->setColumnCount(2);
    avgCounter = 0;
    actLoaded = false;

    ui->tableView_actInfo->setStyleSheet("background-color: #e6e6e6");
    ui->treeView_intervall->setStyleSheet("background-color: #e6e6e6");
    ui->listView_files->setStyleSheet("background-color: #e6e6e6");

    this->set_speedgraph();
    this->resetPlot();
    this->read_activityFiles();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::read_activityFiles()
{
    Activity *readFile;
    QFile file;
    QString filePath,actString;
    int jsonMaxFiles = settings::get_generalValue("filecount").toInt();
    QStringList infoHeader = settings::get_listValues("JsonFile");
    QDir directory(settings::get_gcInfo("gcpath"));
    directory.setSorting(QDir::Name | QDir::Reversed);
    directory.setFilter(QDir::Files);
    QFileInfoList fileList = directory.entryInfoList();
    int fileCount = fileList.count() > jsonMaxFiles ? jsonMaxFiles : fileList.count();
    QDateTime workDateTime;
    workDateTime.setTimeSpec(Qt::TimeZone);
    fileModel->setRowCount(fileCount);

    for(int i = 0; i < fileCount; ++i)
    {
        filePath = fileList.at(i).path()+QDir::separator()+fileList.at(i).fileName();
        file.setFileName(filePath);
        file.open(QFile::ReadOnly | QFile::Text);
        readFile = new Activity(file.readAll(),false);
        workDateTime = QDateTime::fromString(readFile->ride_info.value("Date"),"yyyy/MM/dd hh:mm:ss UTC").addSecs(workDateTime.offsetFromUtc());
        actString = QDate().shortDayName(workDateTime.date().dayOfWeek())+" - " +workDateTime.toString("dd.MM.yyyy hh:mm:ss")+" - "+readFile->ride_info.value("Sport") + " - " + readFile->ride_info.value("Workout Code");
        fileModel->setData(fileModel->index(i,0),actString);
        fileModel->setData(fileModel->index(i,1),filePath);
        ui->progressBar_save->setValue((100/fileCount)*i);
        file.close();
        delete readFile;
    }
    ui->progressBar_save->reset();

    ui->listView_files->setModel(fileModel);
    infoModel = new QStandardItemModel(infoHeader.count(),1);
    infoModel->setVerticalHeaderLabels(infoHeader);
    ui->tableView_actInfo->setModel(infoModel);
    ui->tableView_actInfo->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView_actInfo->horizontalHeader()->setStretchLastSection(true);
    ui->tableView_actInfo->horizontalHeader()->setVisible(false);
    ui->tableView_actInfo->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView_actInfo->verticalHeader()->setSectionsClickable(false);
    ui->tableView_actInfo->setFixedHeight(infoHeader.count()*25);
    this->loadfile(fileList.at(0).path()+QDir::separator()+fileList.at(0).fileName());
    actLoaded = true;
}

void MainWindow::clearActivtiy()
{
    this->resetPlot();

    for(int i = 0; i < infoModel->rowCount(); ++i)
    {
        infoModel->setData(infoModel->index(i,0),"-");
    }

    delete curr_activity->intModel;
    delete curr_activity->sampleModel;
    delete curr_activity->intTreeModel;
    delete curr_activity->avgModel;
    delete curr_activity->selItemModel;
    if(curr_activity->get_sport() == settings::isSwim) delete curr_activity->swimModel;
    delete curr_activity;

    actLoaded = false;
}


void MainWindow::select_activity_file()
{
    QMessageBox::StandardButton reply;
    QString filename = QFileDialog::getOpenFileName(
                this,
                tr("Select GC JSON File"),
                settings::get_gcInfo("gcpath"),
                "JSON Files (*.json)"
                );

    if(filename != "")
    {
        reply = QMessageBox::question(this,
                                  tr("Open Selected File!"),
                                  filename,
                                  QMessageBox::Yes|QMessageBox::No
                                  );
        if (reply == QMessageBox::Yes)
        {
            this->loadfile(filename);
        }
    }
}

void MainWindow::loadfile(const QString &filename)
{
    QFile file(filename);
    QFileInfo fileinfo(filename);
    QString filecontent;

    if(fileinfo.suffix() == "json")
    {
        if (!file.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox::warning(this, tr("Application"),
                                     tr("Cannot read file %1:\n%2.")
                                     .arg(filename)
                                     .arg(file.errorString()));
           return;
        }
        filecontent = file.readAll();
        curr_activity = new Activity(filecontent,true);
        actLoaded = true;
        file.close();

        settings::set_act_isload(true);
        intSelect_del.sport = tree_del.sport = curr_activity->get_sport();
        this->update_infoModel();
        this->init_editorViews();
    }
}

void MainWindow::init_editorViews()
{
    ui->treeView_intervall->setModel(curr_activity->intTreeModel);
    ui->treeView_intervall->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->treeView_intervall->header()->setSectionResizeMode(QHeaderView::Stretch);
    ui->treeView_intervall->header()->setMaximumSectionSize(ui->treeView_intervall->width()/curr_activity->intTreeModel->columnCount());
    ui->treeView_intervall->setItemDelegate(&tree_del);

    treeSelection = ui->treeView_intervall->selectionModel();
    connect(treeSelection,SIGNAL(currentChanged(QModelIndex,QModelIndex)),this,SLOT(setSelectedIntRow(QModelIndex)));

    ui->tableView_selectInt->setModel(curr_activity->selItemModel);
    ui->tableView_selectInt->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView_selectInt->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableView_selectInt->verticalHeader()->setSectionsClickable(false);
    ui->tableView_selectInt->horizontalHeader()->setVisible(false);
    ui->tableView_selectInt->setItemDelegate(&intSelect_del);

    ui->tableView_avgValues->setModel(curr_activity->avgModel);
    ui->tableView_avgValues->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView_avgValues->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView_avgValues->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableView_avgValues->verticalHeader()->setFixedWidth(ui->tableView_selectInt->verticalHeader()->width());
    ui->tableView_avgValues->verticalHeader()->setSectionsClickable(false);
    ui->tableView_avgValues->horizontalHeader()->setVisible(false);
}

void MainWindow::update_infoModel()
{
    for(int i = 0; i < infoModel->rowCount();++i)
    {
        infoModel->setData(infoModel->index(i,0),curr_activity->ride_info.value(settings::get_listValues("JsonFile").at(i)));
    }
}

void MainWindow::on_actionClear_triggered()
{
    this->clearActivtiy();
    actLoaded = false;
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::on_actionLoad_triggered()
{
    this->select_activity_file();
}

void MainWindow::setSelectedIntRow(QModelIndex index)
{
    QStringList intLabel;
    intLabel << "Swim Lap" << "Interval";
    bool isInt = true;
    bool isSwim = false;
    if(curr_activity->get_sport() == settings::isSwim) isSwim = true;

    treeSelection->select(index,QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    QString lapIdent = treeSelection->selectedRows(0).at(0).data().toString().trimmed();
    curr_activity->set_selectedItem(treeSelection);

    if(isSwim)
    {
        ui->horizontalSlider_factor->setEnabled(false);
        if(treeSelection->selectedRows(2).at(0).data().toInt() == 1)
        {
            isInt = false;
        }
        else if(curr_activity->intTreeModel->itemFromIndex(index)->parent() == nullptr || lapIdent.contains(settings::get_generalValue("breakname")))
        {
            isInt = true;
        }
        else
        {
            isInt = false;
        }
        curr_activity->showSwimLap(isInt);
    }
    else
    {
        ui->horizontalSlider_factor->setEnabled(true);
        curr_activity->showInterval(true);
    }

    if(curr_activity->intTreeModel->itemFromIndex(index)->parent() == nullptr)
    {
        this->set_speedValues(index.row());
    }

    intSelect_del.intType = isInt;
    ui->label_lapType->setText(intLabel.at(isInt));
}

void MainWindow::selectAvgValues(QModelIndex index, int avgCol)
{
    treeSelection->select(index,QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    QStandardItem *avgItem = curr_activity->intTreeModel->itemFromIndex(treeSelection->selectedRows(avgCol).at(0));
    bool checkAvg = avgItem->data().toBool();
    curr_activity->set_selectedItem(treeSelection);

    if(checkAvg == false)
    {
        curr_activity->avgItems.insert(index.row(),index);
        curr_activity->intTreeModel->setData(index,"+");
        curr_activity->intTreeModel->setData(index,1,Qt::UserRole+1);
        curr_activity->set_avgValues(++avgCounter,1);
    }
    else
    {
        curr_activity->avgItems.remove(index.row());
        curr_activity->intTreeModel->setData(index,"-");
        curr_activity->intTreeModel->setData(index,0,Qt::UserRole+1);
        curr_activity->set_avgValues(--avgCounter,-1);
    }

    treeSelection->clearSelection();
}

void MainWindow::on_treeView_intervall_clicked(const QModelIndex &index)
{
    int avgCol = curr_activity->intTreeModel->columnCount()-1;

    if(index.column() == avgCol)
    {
        this->selectAvgValues(index,avgCol);
    }
    else
    {
        treeSelection->setCurrentIndex(index,QItemSelectionModel::Select);
    }
}

void MainWindow::on_horizontalSlider_factor_valueChanged(int value)
{
    ui->label_factorValue->setText(QString::number(10-value) + "%");
    double factor = static_cast<double>(value)/100;
    curr_activity->set_polishFactor(0.1-factor);
    int indexRow = ui->treeView_intervall->currentIndex().row();
    this->set_polishValues(indexRow,factor);
    rangeMinMax[0] = curr_activity->polish_SpeedValues(1.0,curr_activity->get_int_speed(indexRow),0.1-factor,false);
    rangeMinMax[1] = curr_activity->polish_SpeedValues(50.0,curr_activity->get_int_speed(indexRow),0.1-factor,false);
    ui->lineEdit_polMin->setText(QString::number(rangeMinMax[0]));
    ui->lineEdit_polMax->setText(QString::number(rangeMinMax[1]));
}

void MainWindow::set_polishValues(int lap,double factor)
{
    double avg = curr_activity->get_int_speed(lap);
    double intdist = curr_activity->get_int_distance(lap);
    for(int i = 0; i < speedValues.count(); ++i)
    {
        if(lap == 0 && i < 5)
        {
            polishValues[i] = speedValues[i];
        }
        else
        {
            polishValues[i] = curr_activity->polish_SpeedValues(speedValues[i],avg,0.10-factor,true);
        }
        //if(speedMinMax[0] > polishValues[i]) rangeMinMax[0] = speedMinMax[0] = polishValues[i];
        //if(speedMinMax[1] < polishValues[i]) rangeMinMax[1] = speedMinMax[1] = polishValues[i];
    }
    this->set_speedPlot(avg,intdist);
}

void MainWindow::set_speedValues(int index)
{
    int lapLen;
    double current = 0;
    double avg = curr_activity->get_int_speed(index);
    double intdist = curr_activity->get_int_distance(index);

    int start = curr_activity->intModel->data(curr_activity->intModel->index(index,1,QModelIndex())).toInt();
    int stop = curr_activity->intModel->data(curr_activity->intModel->index(index,2,QModelIndex())).toInt();
    speedMinMax.resize(2);
    rangeMinMax.resize(2);
    speedMinMax[0] = 40.0;
    speedMinMax[1] = 0.0;
    lapLen = stop-start;

    speedValues.resize(lapLen+1);
    polishValues.resize(lapLen+1);
    secTicker.resize(lapLen+1);

    for(int i = start, pos=0; i <= stop; ++i,++pos)
    {
        current = curr_activity->sampSpeed[i];
        secTicker[pos] = pos;
        speedValues[pos] = current;
        if(speedMinMax[0] > current) rangeMinMax[0] = speedMinMax[0] = current;
        if(speedMinMax[1] < current) rangeMinMax[1] = speedMinMax[1] = current;
    }

    if(curr_activity->get_sport() != settings::isSwim)
    {
        double factor = static_cast<double>(ui->horizontalSlider_factor->value())/100;
        this->set_polishValues(index,factor);
    }

    this->set_speedPlot(avg,intdist);
}

void MainWindow::resetPlot()
{
    ui->widget_plot->clearPlottables();
    ui->widget_plot->clearItems();
    speedValues.resize(100);
    secTicker.resize(100);
    secTicker.fill(0);
    speedValues.fill(0);
    QCPGraph *resetLine = ui->widget_plot->addGraph();
    resetLine->setPen(QPen(QColor(255,255,255),2));
    resetLine->setData(secTicker,speedValues);
    resetLine->setName("-");
    ui->widget_plot->xAxis->setRange(0,100);
    ui->widget_plot->xAxis2->setRange(0,1);
    ui->widget_plot->yAxis->setRange(0,5);
    ui->widget_plot->plotLayout()->setRowStretchFactor(1,0.0001);
    ui->widget_plot->replot();
}

void MainWindow::set_speedgraph()
{
    QFont plotFont;
    plotFont.setBold(true);
    plotFont.setPointSize(8);

    ui->widget_plot->xAxis->setLabel("Seconds");
    ui->widget_plot->xAxis->setLabelFont(plotFont);
    ui->widget_plot->xAxis2->setVisible(true);
    ui->widget_plot->xAxis2->setLabelFont(plotFont);
    ui->widget_plot->xAxis2->setLabel("Distance");
    ui->widget_plot->xAxis2->setTickLabels(true);
    ui->widget_plot->yAxis->setLabel("Speed");
    ui->widget_plot->yAxis->setLabelFont(plotFont);
    ui->widget_plot->yAxis2->setVisible(true);
    ui->widget_plot->yAxis2->setLabelFont(plotFont);
    ui->widget_plot->legend->setVisible(true);
    ui->widget_plot->legend->setFont(plotFont);

    QCPLayoutGrid *subLayout = new QCPLayoutGrid;
    ui->widget_plot->plotLayout()->addElement(1,0,subLayout);
    subLayout->setMargins(QMargins(200,0,200,5));
    subLayout->addElement(0,0,ui->widget_plot->legend);
}

void MainWindow::set_speedPlot(double avgSpeed,double intdist)
{
    ui->widget_plot->clearPlottables();
    ui->widget_plot->clearItems();
    ui->widget_plot->legend->setFillOrder(QCPLegend::foColumnsFirst);
    ui->widget_plot->plotLayout()->setRowStretchFactor(1,0.0001);

    QCPGraph *speedLine = ui->widget_plot->addGraph();
    speedLine->setName("Speed");
    speedLine->setLineStyle(QCPGraph::lsLine);
    speedLine->setData(secTicker,speedValues);
    speedLine->setPen(QPen(QColor(0,255,0),2));

    QCPItemLine *avgLine = new QCPItemLine(ui->widget_plot);
    avgLine->start->setCoords(0,avgSpeed);
    avgLine->end->setCoords(speedValues.count(),avgSpeed);
    avgLine->setPen(QPen(QColor(0,0,255),2));

    QCPGraph *avgLineP = ui->widget_plot->addGraph();
    avgLineP->setName("Avg Speed");
    avgLineP->setPen(QPen(QColor(0,0,255),2));

    if(curr_activity->get_sport() != settings::isSwim)
    {
        QCPGraph *polishLine = ui->widget_plot->addGraph();
        polishLine->setName("Polished Speed");
        polishLine->setLineStyle(QCPGraph::lsLine);
        polishLine->setData(secTicker,polishValues);
        polishLine->setPen(QPen(QColor(255,0,0),2));

        QCPGraph *polishRangeP = ui->widget_plot->addGraph();
        polishRangeP->setName("Polish Range");
        polishRangeP->setPen(QPen(QColor(225,225,0),2));

        QCPItemRect *polishRange = new QCPItemRect(ui->widget_plot);
        polishRange->topLeft->setCoords(0,rangeMinMax[1]);
        polishRange->bottomRight->setCoords(speedValues.count(),rangeMinMax[0]);
        polishRange->setPen(QPen(QColor(225,225,0),2));
        polishRange->setBrush(QBrush(QColor(255,255,0,50)));
    }

    double yMin = 0,yMax = 0;
    if(speedMinMax[0] > 0)
    {
        yMin = speedMinMax[0]*0.1;
    }
    yMax =  speedMinMax[1]*0.1;

    ui->widget_plot->xAxis->setRange(0,speedValues.count());
    ui->widget_plot->xAxis2->setRange(0,intdist);
    ui->widget_plot->yAxis->setRange(speedMinMax[0]-yMin,speedMinMax[1]+yMax);
    ui->widget_plot->yAxis2->setRange(speedMinMax[0]-yMin,speedMinMax[1]+yMax);

    ui->widget_plot->replot();
}

void MainWindow::on_actionSave_triggered()
{
    if(curr_activity->get_sport() == settings::isSwim)
    {
        curr_activity->updateXDataModel();
    }
    else
    {
        curr_activity->updateIntModel(2,1);
    }
    curr_activity->writeChangedData();
}

void MainWindow::on_actionUnSelect_triggered()
{
    curr_activity->reset_avgSelection();
    avgCounter = 0;
}

void MainWindow::on_toolButton_update_clicked()
{
    ui->tableView_selectInt->setCurrentIndex(curr_activity->selItemModel->index(0,0));
    curr_activity->updateRow_intTree(treeSelection);
    this->update_infoModel();
}

void MainWindow::on_toolButton_delete_clicked()
{
    curr_activity->removeRow_intTree(treeSelection);
    this->update_infoModel();
}

void MainWindow::on_toolButton_add_clicked()
{
    curr_activity->addRow_intTree(treeSelection);
    treeSelection->setCurrentIndex(ui->treeView_intervall->indexAbove(treeSelection->currentIndex()),QItemSelectionModel::Select);
}
void MainWindow::setCurrentTreeIndex(bool up)
{
    QModelIndex index;

    if(up)
    {
        index = ui->treeView_intervall->indexAbove(treeSelection->currentIndex());
    }
    else
    {
        index = ui->treeView_intervall->indexBelow(treeSelection->currentIndex());
    }

    int currRow = index.row();

    if(currRow == 0)
    {
        ui->toolButton_upInt->setEnabled(false);
    }
    else if(currRow == curr_activity->intTreeModel->rowCount()-1)
    {
        ui->toolButton_downInt->setEnabled(false);
    }
    else
    {
        ui->toolButton_upInt->setEnabled(true);
        ui->toolButton_downInt->setEnabled(true);
    }

    treeSelection->setCurrentIndex(index,QItemSelectionModel::Select);
}

void MainWindow::on_toolButton_upInt_clicked()
{
    this->setCurrentTreeIndex(true);
}

void MainWindow::on_toolButton_downInt_clicked()
{
    this->setCurrentTreeIndex(false);
}

void MainWindow::on_listView_files_clicked(const QModelIndex &index)
{
    this->clearActivtiy();
    this->loadfile(fileModel->data(fileModel->index(index.row(),1)).toString());
}
