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
#include <bsoncxx/oid.hpp>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::basic::kvp;

mongocxx::database db;
MainWindow * theMainWin = nullptr;

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    theMainWin = this;
    mongoStuff = make_unique<MongoStuff>();
    ui->textEdit->setText("127.0.0.1:27017");
    ui->endDate->setDateTime(QDateTime::currentDateTime());

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
    mongoStuff->connectToDb(ui->textEdit->toPlainText(), "admin");

    auto names = mongoStuff->getNamesDb();
    for(std::string n : names){
        const char * c = n.data();
        ui->comboBox->addItem(tr(c));
    }
}

void MainWindow::on_comboBox_currentIndexChanged(const QString &arg1)
{
    collectionsUpdate();
}

void MainWindow::collectionsUpdate(){
    auto collections = mongoStuff->getCollections(ui->comboBox->currentText());
    ui->listWidget->clear();
    for(std::string col : collections){
        const char * c = col.data();
        new QListWidgetItem(tr(c), ui->listWidget);
    }
}

void MainWindow::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    QString json_doc = mongoStuff->getDocuments(ui->listWidget->currentItem()->text());
    fillTree(json_doc);
}

void MainWindow::on_btnInsert_clicked()
{
    QAbstractItemModel *model = ui->treeView->model();
    if(ui->listWidget->currentItem() == nullptr)
        return;
    try {
        mongoStuff->insertColl(ui->listWidget->currentItem()->text());

        QString json_doc = mongoStuff->getDocuments(ui->listWidget->currentItem()->text());
        fillTree(json_doc);
    } catch (const mongocxx::exception &ex) {
        qWarning() << "Ошибка при добавлении новой записи: " << ex.what();
    }
}

void MainWindow::on_btnRemove_clicked()
{

    QModelIndex index = ui->treeView->selectionModel()->currentIndex();
    QAbstractItemModel *model = ui->treeView->model();

    if(!index.isValid())
        return;
    if(ui->listWidget->currentItem() == nullptr)
        return;

    QString curValue = model->index(index.row(), 1, index.parent()).data().value<QString>();
    if (curValue == "[Array]")
        return; //remove collection

    try {
        mongoStuff->removeItem(ui->listWidget->currentItem()->text().toUtf8().constData(), getIdDocument());

        QString json_doc = mongoStuff->getDocuments(ui->listWidget->currentItem()->text());
        fillTree(json_doc);
    } catch (const mongocxx::exception &ex) {
        qWarning() << "Ошибка при удалении записи: " << ex.what();

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

QString MainWindow::getIdDocument(){
    QAbstractItemModel *model = ui->treeView->model();
    QModelIndex index = ui->treeView->selectionModel()->currentIndex();

    QString objectId;
    if(model->index(index.row(), 0, index.parent()).data().value<QString>() == "_id")
        objectId = model->index(index.row(), 1, index.parent()).data().value<QString>();
    else if (model->index(index.row(), 1, index.parent()).data().value<QString>() == "[Object]")
        objectId = model->index(0, 1, index).data().value<QString>();
    else
        objectId = model->index(0, 1, index.parent()).data().value<QString>();
    return objectId;

}

void MainWindow::updateDocument(){
    QAbstractItemModel *model = ui->treeView->model();
    QModelIndex index = ui->treeView->selectionModel()->currentIndex();

    if(ui->listWidget->currentItem() == nullptr)
        return;
    QString curKey = model->index(index.row(), 0, index.parent()).data().value<QString>();
    QString curValue = model->index(index.row(), 1, index.parent()).data().value<QString>();
    if (curValue == "[Object]" || curValue == "[Array]" || curKey == "_id")
        return;

    mongoStuff->updateDocument(ui->listWidget->currentItem()->text(), getIdDocument(),
                               model->index(index.row(), 0, index.parent()).data().value<QString>(),
                               model->index(index.row(), 1, index.parent()).data().value<QString>());

    QString json_doc = mongoStuff->getDocuments(ui->listWidget->currentItem()->text());
    fillTree(json_doc);
}


void MainWindow::on_searchBtn_clicked()
{
    qint64 millisStart = ui->startDate->dateTime().toMSecsSinceEpoch();
    std::chrono::milliseconds chronoMillisStart(millisStart);
    bsoncxx::types::b_date startDT(chronoMillisStart);

    qint64 millisEnd= ui->endDate->dateTime().toMSecsSinceEpoch();
    std::chrono::milliseconds chronoMillisEnd(millisEnd);
    bsoncxx::types::b_date endDT(chronoMillisEnd);

    if(ui->listWidget->currentItem() == nullptr)
        return;

    QString json_doc = mongoStuff->filter(ui->listWidget->currentItem()->text(), startDT, endDT, ui->regexWrd->toPlainText());

    fillTree(json_doc);
}

void MainWindow::fillTree(QString json_doc){
    if(json_doc.isEmpty() || json_doc.length() < 5) {
        ui->treeView->setModel(nullptr);
        jsonModel = new TreeModel(this);
        ui->treeView->setModel(jsonModel);
        return;
    };
    jsonModel->loadJson(json_doc, ui->listWidget->currentItem()->text());
    ui->treeView->expandAll();
}

void MainWindow::on_addCollectionBtn_clicked()
{
    if(ui->collectionNameEdit->toPlainText() == nullptr || ui->collectionNameEdit->toPlainText().trimmed() == "" || ui->comboBox->currentText() == nullptr)
        return;

    mongoStuff->createCollection(ui->collectionNameEdit->toPlainText().toStdString(), ui->comboBox->currentText());
    collectionsUpdate();
}

void MainWindow::on_removeCollectionBtn_clicked()
{
    if(ui->listWidget->currentItem() == nullptr)
        return;

    mongoStuff->removeCollection(ui->listWidget->currentItem()->text());
    collectionsUpdate();
    fillTree("");
}
