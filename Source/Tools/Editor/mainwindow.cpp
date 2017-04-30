#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "rendererwidget.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    auto renderWidget = new RendererWidget(this);
    ui->centralWidget = renderWidget;
    renderWidget->init();
}

MainWindow::~MainWindow()
{
    delete ui;
}
