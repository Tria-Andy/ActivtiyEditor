#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "settings.h"
#include "jsonhandler.h"
#include "activity.h"
#include "calculation.h"

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
    void on_actionClear_triggered();
    void on_actionExit_triggered();
    void on_actionLoad_triggered();

    void on_treeView_intervall_clicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;

    Activity *curr_activity;
    jsonHandler *jsonhandler;

    //Intervall Chart
    QVector<double> secTicker,speedValues,polishValues,speedMinMax,rangeMinMax;
    void set_speedValues(int);
    void set_speedgraph(double,double);
    void set_speedPlot(double,double);
    void set_polishValues(int,double);

    //Editor
    void select_activity_file();
    void loadfile(const QString &filename);
    void set_activty_infos();
    void set_activty_intervalls();
    void set_avg_fields();
    void write_hf_infos();
    void fill_WorkoutContent();
    void unselect_intRow();
    void set_menuItems(bool,bool);
    void reset_jsontext();
    void freeMem();



};

#endif // MAINWINDOW_H
