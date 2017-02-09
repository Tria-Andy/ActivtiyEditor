#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QItemSelectionModel>
#include "settings.h"
#include "jsonhandler.h"
#include "activity.h"
#include "calculation.h"
#include "del_treeview.h"
#include "del_intselect.h"
#include "del_avgselect.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow,public calculation
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void setSelectedIntRow(QModelIndex);
    void on_actionClear_triggered();
    void on_actionExit_triggered();
    void on_actionLoad_triggered();
    void on_treeView_intervall_clicked(const QModelIndex &index);
    void on_actionUnSelect_triggered();
    void on_toolButton_update_clicked();
    void on_toolButton_upInt_clicked();
    void on_toolButton_downInt_clicked();
    void on_actionSave_triggered();
    void on_horizontalSlider_factor_valueChanged(int value);
    void on_listView_files_clicked(const QModelIndex &index);
    void on_toolButton_delete_clicked();
    void on_toolButton_add_clicked();

private:
    Ui::MainWindow *ui;
    Activity *curr_activity;
    QStandardItemModel *fileModel,*infoModel;
    QItemSelectionModel *treeSelection;
    del_treeview tree_del;
    del_intselect intSelect_del;
    del_avgselect avgSelect_del;
    int avgCounter;
    bool actLoaded;

    //Intervall Chart
    QVector<double> secTicker,speedValues,polishValues,speedMinMax,rangeMinMax;
    void set_speedValues(int);
    void set_speedgraph();
    void set_speedPlot(double,double);
    void set_polishValues(int,double);
    void resetPlot();

    //Editor
    void read_activityFiles();
    void clearActivtiy();
    void select_activity_file();
    void loadfile(const QString &filename);
    void selectAvgValues(QModelIndex,int);
    void setCurrentTreeIndex(bool);
    void init_editorViews();
    void update_infoModel();
    void fill_WorkoutContent();
    void unselect_intRow();
    void set_menuItems(bool,bool);
    void reset_jsontext();
    void freeMem();
};

#endif // MAINWINDOW_H
