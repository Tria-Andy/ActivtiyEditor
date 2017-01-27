#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    settings::loadSettings();
    avgCounter = 0;
    ui->frame_avg->setVisible(false);
    ui->frame_editInt->setVisible(true);
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
        file.close();

        settings::set_act_isload(true);
        intSelect_del.sport = curr_activity->get_sport();

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
    treeSelection = ui->treeView_intervall->selectionModel();

    ui->tableView_selectInt->setModel(curr_activity->selItemModel);
    ui->tableView_selectInt->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView_selectInt->verticalHeader()->setVisible(false);
    ui->tableView_selectInt->horizontalHeader()->setVisible(false);
    ui->tableView_selectInt->setItemDelegate(&intSelect_del);

    ui->tableView_avgValues->setModel(curr_activity->avgModel);
    ui->tableView_avgValues->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView_avgValues->verticalHeader()->setVisible(false);
    ui->tableView_avgValues->horizontalHeader()->setVisible(false);
    ui->tableView_avgValues->setShowGrid(false);
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
    curr_activity->int_model->clear();
    curr_activity->samp_model->clear();
    curr_activity->intTreeModel->clear();
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
    intSelect_del.intType = isInt;
    ui->label_lapType->setText(intLabel.at(isInt));
}

void MainWindow::selectAvgValues(QModelIndex index, int avgCol)
{
    treeSelection->select(index,QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    QStandardItem *avgItem = curr_activity->intTreeModel->itemFromIndex(treeSelection->selectedRows(avgCol).at(0));

    bool checkAvg = avgItem->data().toBool();
    //curr_activity->intTreeModel->itemFromIndex(index)->setBackground(QBrush(Qt::green));
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
        ui->frame_editInt->setVisible(false);
        ui->frame_avg->setVisible(true);
        this->selectAvgValues(index,avgCol);
    }
    else
    {
        ui->frame_editInt->setVisible(true);
        ui->frame_avg->setVisible(false);
        this->setSelectedIntRow(index);
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

    this->setSelectedIntRow(index);
    ui->treeView_intervall->setCurrentIndex(index);
}

void MainWindow::on_toolButton_upInt_clicked()
{
    this->setCurrentTreeIndex(true);
}

void MainWindow::on_toolButton_downInt_clicked()
{
    this->setCurrentTreeIndex(false);
}
