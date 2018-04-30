#pragma once

#include <QMainWindow>

class ViewerWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget* parent=nullptr);

protected slots:

    void runSimulation(bool);
    void about();

protected:

    ViewerWidget* _viewer;
};
