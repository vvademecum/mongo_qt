#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cstdint>
#include <iostream>
#include <vector>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <QMessageBox>
#include <QListWidget>
#include <QComboBox>
#include <QDebug>
#include <QDateTime>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/exception/exception.hpp>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::basic::kvp;



mongocxx::database db;
//mongocxx::client conn{mongocxx::uri ("mongodb://127.0.0.1:27017")};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);

    connect(ui->listWidget, SIGNAL(itemClicked(QListWidgetItem*)),
                this, SLOT(on_listWidget_itemDoubleClicked(QListWidgetItem*)));

    jsonModel = new TreeModel(this);
    ui->treeView->setModel(jsonModel);


}


MainWindow::~MainWindow()
{

    delete ui;
}

void MainWindow::on_pushButton_2_clicked() //connect
{
    mongocxx::instance inst{};
    mongocxx::uri uri("mongodb://127.0.0.1:27017");
    mongocxx::client client{uri};
    db = client["test"];

    auto names = client.list_database_names();

    for(std::string n : names){
        const char * c = n.data();
        ui->comboBox->addItem(tr(c));
    }
}

void MainWindow::on_pushButton_clicked() // insert one collection
{
    mongocxx::uri uri("mongodb://127.0.0.1:27017");
    mongocxx::client client{uri};
    db = client["test"];
    bsoncxx::document::value record_doc =
        document{} << "name" << "Vladimir"
                   << "age" << 23
                   << "date" << bsoncxx::types::b_date{std::chrono::system_clock::now()}
                   << finalize;
    auto res = db["record"].insert_one(std::move(record_doc));

    QMessageBox::warning(this, "Внимание", "ok");
}

void MainWindow::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    mongocxx::uri uri("mongodb://127.0.0.1:27017");
    mongocxx::client client{uri};

    const QString& currentColl = ui->listWidget->currentItem()->text();

    const QString& currentBase = ui->comboBox->currentText();
    QByteArray currentBase_ba = currentBase.toLocal8Bit();
    const char *currentBase_c = currentBase_ba.data();

    db = client[currentBase_c];

    //QMessageBox::warning(this, "Текущая коллекция", currentColl);

    QByteArray currentColl_ba = currentColl.toLocal8Bit();
    const char *currentColl_c = currentColl_ba.data();

    QString json_doc = "[";
    mongocxx::cursor cursor = db[currentColl_c].find({});
    for(auto doc : cursor) {
      json_doc.append(QString::fromStdString(bsoncxx::to_json(doc))).append(", ");
    }
    json_doc.remove(json_doc.length()-2, 2);
    json_doc.append(']');

    //qDebug() << json_doc;

    if(json_doc.isEmpty() || json_doc.length() < 5) {
        QModelIndex index = ui->treeView->selectionModel()->currentIndex().parent().parent();
        QAbstractItemModel *model = ui->treeView->model();
        if (model->removeRow(index.row(), index.parent()))
            updateIndex();
        qDebug() << "Коллекция пуста!";
        return;
    };
    jsonModel->loadJson(json_doc, currentColl);
    ui->treeView->expandAll();
}

void MainWindow::on_comboBox_currentIndexChanged(const QString &arg1)
{
    mongocxx::uri uri("mongodb://127.0.0.1:27017");
    mongocxx::client client{uri};

    const QString& currentBase = ui->comboBox->currentText();
    QByteArray currentBase_ba = currentBase.toLocal8Bit();
    const char *currentBase_c = currentBase_ba.data();

    db = client[currentBase_c];
    auto collections = db.list_collection_names();
    ui->listWidget->clear();
    for(std::string col : collections){
        const char * c = col.data();
        new QListWidgetItem(tr(c), ui->listWidget);
    }
}

void MainWindow::initEdit()
{

//    connect(ui->btnInsert ,&QPushButton::clicked,this,[this](){


//    });

//    connect(ui->btnInsertChild,&QPushButton::clicked,this,[this](){
//        QModelIndex index = ui->treeView->selectionModel()->currentIndex();
//        QAbstractItemModel *model = ui->treeView->model();

//        if (model->columnCount(index) == 0) {
//            if (!model->insertColumn(0, index))
//                return;
//        }

//        if (!model->insertRow(0, index))
//            return;

//        ui->treeView->selectionModel()->setCurrentIndex(model->index(0, 0, index),
//                                                        QItemSelectionModel::ClearAndSelect);
//        updateIndex();
//    });

//    connect(ui->btnRemove,&QPushButton::clicked,this,[this](){
//        QModelIndex index = ui->treeView->selectionModel()->currentIndex();
//        QAbstractItemModel *model = ui->treeView->model();
//        if (model->removeRow(index.row(), index.parent()))
//            updateIndex();
//    });
}

void MainWindow::on_btnInsert_clicked()
{
    QModelIndex index = ui->treeView->selectionModel()->currentIndex();
    if(!index.isValid())
        return;
    QAbstractItemModel *model = ui->treeView->model();

    try {
        mongocxx::uri uri("mongodb://127.0.0.1:27017");
        mongocxx::client client{uri};

        const QString& currentBase = ui->comboBox->currentText();
        QByteArray currentBase_ba = currentBase.toLocal8Bit();
        const char *currentBase_c = currentBase_ba.data();

        db = client[currentBase_c];

        const QString& currentColl = ui->listWidget->currentItem()->text();
        QByteArray currentColl_ba = currentColl.toLocal8Bit();
        const char *currentColl_c = currentColl_ba.data();

        std::string keyName = "name";
        std::string valueName = "[name_value]";
        std::string keyAge = "age";
        std::string valueAge = "[age_value]";
        std::string keyDate = "date";
        std::string valueDate = "[date_value]";
        bsoncxx::builder::basic::document basic_builder{};
        basic_builder.append(kvp(keyName, valueName), kvp(keyAge, valueAge), kvp(keyDate, valueDate));
        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = db[currentColl_c].insert_one(basic_builder.view());

        QString json_doc = "[";
        mongocxx::cursor cursor = db[currentColl_c].find({});
        for(auto doc : cursor) {
          json_doc.append(QString::fromStdString(bsoncxx::to_json(doc))).append(", ");
        }
        json_doc.remove(json_doc.length()-2, 2);
        json_doc.append(']');

        if(json_doc.isEmpty() || json_doc.length() < 5) {
            qDebug() << "Коллекция пуста!";

            QModelIndex index = ui->treeView->selectionModel()->currentIndex().parent().parent();
            QAbstractItemModel *model = ui->treeView->model();
            if (model->removeRow(index.row(), index.parent()))
                updateIndex();
        };

        jsonModel->loadJson(json_doc, currentColl);
        ui->treeView->expandAll();
    } catch (const mongocxx::exception &ex) {
        qWarning() << "Ошибка при удалении: " << ex.what();
    }
}


void MainWindow::on_btnRemove_clicked()
{
    QModelIndex index = ui->treeView->selectionModel()->currentIndex();
    QAbstractItemModel *model = ui->treeView->model();

    mongocxx::uri uri("mongodb://127.0.0.1:27017");
    mongocxx::client client{uri};

    const QString& currentBase = ui->comboBox->currentText();
    QByteArray currentBase_ba = currentBase.toLocal8Bit();
    const char *currentBase_c = currentBase_ba.data();

    db = client[currentBase_c];

    const QString& currentColl = ui->listWidget->currentItem()->text();
    QByteArray currentColl_ba = currentColl.toLocal8Bit();
    const char *currentColl_c = currentColl_ba.data();


    qDebug() << model->index(index.row(), 0, index.parent()).data().value<QString>();
    const QString& nameField = model->index(index.row(), 0, index.parent()).data().value<QString>();
    std::string namef = nameField.toStdString();

    qDebug() << model->index(index.row(), 1, index.parent()).data().value<QString>();
    const QString& valueField = model->index(index.row(), 1, index.parent()).data().value<QString>();
    std::string valf = valueField.toStdString();

    try {
        bsoncxx::builder::basic::document basic_builder{};
        basic_builder.append(kvp(namef, valf));
        bsoncxx::stdx::optional<mongocxx::result::delete_result> result = db[currentColl_c].delete_one(basic_builder.view());

        QString json_doc = "[";
        mongocxx::cursor cursor = db[currentColl_c].find({});
        for(auto doc : cursor) {
          json_doc.append(QString::fromStdString(bsoncxx::to_json(doc))).append(", ");
        }
        json_doc.remove(json_doc.length()-2, 2);
        json_doc.append(']');

        if(json_doc.isEmpty() || json_doc.length() < 5) {
            qDebug() << "Коллекция пуста!";

            QModelIndex index = ui->treeView->selectionModel()->currentIndex().parent().parent();
            QAbstractItemModel *model = ui->treeView->model();
            if (model->removeRow(index.row(), index.parent()))
                updateIndex();
        };

        jsonModel->loadJson(json_doc, currentColl);
        ui->treeView->expandAll();
    } catch (const mongocxx::exception &ex) {
        qWarning() << "Ошибка при удалении: " << ex.what();
    }
}

void MainWindow::updateIndex()
{

    bool hasCurrent = ui->treeView->selectionModel()->currentIndex().isValid();

    if (hasCurrent) {
        ui->treeView->closePersistentEditor(ui->treeView->selectionModel()->currentIndex());

        int row = ui->treeView->selectionModel()->currentIndex().row();
        int column = ui->treeView->selectionModel()->currentIndex().column();
        if (ui->treeView->selectionModel()->currentIndex().parent().isValid())
            qDebug()<<tr("Position: (%1,%2)").arg(row).arg(column);
        else
            qDebug()<<tr("Position: (%1,%2) in top level").arg(row).arg(column);
    }
}


