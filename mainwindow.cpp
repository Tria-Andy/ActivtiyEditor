#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    settings::loadSettings();
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

        this->set_activty_infos();
        this->set_activty_intervalls();
     }
}


void MainWindow::set_activty_intervalls()
{
    ui->treeView_intervall->setModel(curr_activity->intTreeModel);
    ui->treeView_intervall->setEditTriggers(QAbstractItemView::NoEditTriggers);
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
    curr_activity->curr_act_model->clear();
    curr_activity->int_model->clear();
    curr_activity->samp_model->clear();
    curr_activity->edit_int_model->clear();
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

void MainWindow::on_treeView_intervall_clicked(const QModelIndex &index)
{
    int avgCol = curr_activity->intTreeModel->columnCount() -1;

    if(index.column() == avgCol)
    {
        qDebug() << "Set Avg";
    }
    else
    {
        qDebug() << "Load Lap";
    }
}
