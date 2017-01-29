#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    settings::loadSettings();
    avgCounter = 0;
    this->set_speedgraph();
    this->resetPlot();
}

MainWindow::~MainWindow()
{
    delete ui;
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
        curr_activity = new Activity();
        filecontent = file.readAll();
        jsonhandler = new jsonHandler(true,filecontent,curr_activity);
        curr_activity->set_jsonhandler(jsonhandler);
        file.close();

        settings::set_act_isload(true);
        intSelect_del.sport = tree_del.sport = curr_activity->get_sport();
        this->set_activty_infos();
        this->set_activty_intervalls();
    }
}

void MainWindow::set_activty_intervalls()
{
    ui->treeView_intervall->setModel(curr_activity->intTreeModel);
    ui->treeView_intervall->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->treeView_intervall->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->treeView_intervall->header()->setMinimumSectionSize(100);
    ui->treeView_intervall->setItemDelegate(&tree_del);

    treeSelection = ui->treeView_intervall->selectionModel();
    connect(treeSelection,SIGNAL(currentChanged(QModelIndex,QModelIndex)),this,SLOT(setSelectedIntRow(QModelIndex)));

    ui->tableView_selectInt->setModel(curr_activity->selItemModel);
    ui->tableView_selectInt->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView_selectInt->horizontalHeader()->setVisible(false);
    ui->tableView_selectInt->verticalHeader()->setFixedWidth(ui->tableView_selectInt->width()/2);
    ui->tableView_selectInt->setItemDelegate(&intSelect_del);

    ui->tableView_avgValues->setModel(curr_activity->avgModel);
    ui->tableView_avgValues->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView_avgValues->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView_avgValues->horizontalHeader()->setVisible(false);
    ui->tableView_avgValues->verticalHeader()->setFixedWidth(ui->tableView_avgValues->width()/2);
}

void MainWindow::set_activty_infos()
{
    ui->textBrowser_Info->clear();

    QTextCursor cursor = ui->textBrowser_Info->textCursor();
    cursor.beginEditBlock();

    QTextTableFormat tableFormat;
    tableFormat.setCellSpacing(2);
    tableFormat.setCellPadding(2);
    tableFormat.setBorder(0);
    tableFormat.setBackground(QBrush(QColor(220,220,220)));
    QVector<QTextLength> constraints;
        constraints << QTextLength(QTextLength::PercentageLength, 40)
                    << QTextLength(QTextLength::PercentageLength, 60);
    tableFormat.setColumnWidthConstraints(constraints);

    QTextTable *table = cursor.insertTable(curr_activity->ride_info.count(),2,tableFormat);

        QTextFrame *frame = cursor.currentFrame();
        QTextFrameFormat frameFormat = frame->frameFormat();
        frameFormat.setBorder(0);
        frame->setFrameFormat(frameFormat);

        QTextCharFormat format = cursor.charFormat();
        format.setFontPointSize(8);

        QTextCharFormat infoFormat = format;
        infoFormat.setFontWeight(QFont::Bold);

        QTextCharFormat valueFormat = format;

    int i = 0;
    for(QMap<QString,QString>::const_iterator it =  curr_activity->ride_info.cbegin(), end = curr_activity->ride_info.cend(); it != end; ++it,++i)
    {
        QTextTableCell cell = table->cellAt(i,0);
        QTextCursor cellCurser = cell.firstCursorPosition();
        cellCurser.insertText(it.key(),infoFormat);
        cell = table->cellAt(i,1);
        cellCurser = cell.firstCursorPosition();
        cellCurser.insertText(it.value(),valueFormat);
    }
    //table->insertRows(table->rows(),1);

    cursor.endEditBlock();
    cursor.setPosition(0);
    ui->textBrowser_Info->setTextCursor(cursor);
}


void MainWindow::on_actionClear_triggered()
{
    this->resetPlot();
    curr_activity->intModel->clear();
    curr_activity->sampleModel->clear();
    curr_activity->intTreeModel->clear();
    curr_activity->avgModel->clear();
    curr_activity->selItemModel->clear();
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

    for(int i = 0; i < treeSelection->selectedIndexes().count();++i)
    {
        curr_activity->selItem.insert(i,treeSelection->selectedRows(i).at(0));
    }

    if(isSwim)
    {
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
        curr_activity->showInterval(true);
    }

    if(curr_activity->intTreeModel->itemFromIndex(index)->parent() == nullptr)
    {
        ui->horizontalSlider_factor->setValue(0);
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

    for(int i = 0; i < treeSelection->selectedIndexes().count();++i)
    {
        curr_activity->selItem.insert(i,treeSelection->selectedRows(i).at(0));
    }

    if(checkAvg == false)
    {
        curr_activity->intTreeModel->setData(index,"+");
        curr_activity->intTreeModel->setData(index,1,Qt::UserRole+1);
        curr_activity->set_avgValues(++avgCounter,1);
    }
    else
    {
        curr_activity->intTreeModel->setData(index,"-");
        curr_activity->intTreeModel->setData(index,0,Qt::UserRole+1);
        curr_activity->set_avgValues(--avgCounter,-1);
    }

    treeSelection->clearSelection();
}

void MainWindow::on_treeView_intervall_clicked(const QModelIndex &index)
{
    curr_activity->selItem.clear();
    int avgCol = curr_activity->intTreeModel->columnCount()-1;

    if(index.column() == avgCol)
    {
        this->selectAvgValues(index,avgCol);
        ui->treeView_intervall->setItemDelegateForRow(index.row(),&avgSelect_del);
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

    if(curr_activity->get_sport() != settings::isSwim) this->set_polishValues(index,0.0);
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
    subLayout->setMargins(QMargins(50,0,50,5));
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
        curr_activity->updateSwimModel();
    }
    else
    {
        curr_activity->updateIntModel(2,1);
    }
}

void MainWindow::on_actionRecalc_triggered()
{

}

void MainWindow::on_actionUnSelect_triggered()
{

}

void MainWindow::on_toolButton_update_clicked()
{
    curr_activity->updateIntTreeRow(treeSelection);
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
