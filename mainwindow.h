#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QFile>
#include <QFileDialog>
#include <QListWidgetItem>
#include "TreeModel.h"
#include "MongoStuff.h"

#include <QStandardItemModel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    std::unique_ptr<MongoStuff> mongoStuff;

    QJsonDocument doc;
    QJsonArray docArr;
    QJsonParseError docError;
    QFile file;
    QStandardItem* name;
    QStandardItem* age;

private:
    void initLoadDump();
    void initEdit();
    void updateIndex();

private slots:

    void on_pushButton_2_clicked();

    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

    void on_comboBox_currentIndexChanged(const QString &arg1);

    void on_btnRemove_clicked();

    void on_btnInsert_clicked();

    QString getIdDocument();

    void on_searchBtn_clicked();

    void fillTree(QString json_doc);

public slots:

    void updateDocument();


private:
    Ui::MainWindow *ui;
    TreeModel *jsonModel;
};
#endif // MAINWINDOW_H
