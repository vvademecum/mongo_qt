#include "MongoStuff.h"
#include <QDebug>
#include <QDateTime>
#include "mongocxx/instance.hpp"
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/client.hpp>
#include <bsoncxx/view_or_value.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/types.hpp>
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::make_array;


MongoStuff::MongoStuff()
{

}

void MongoStuff::connectToDb(QString serverIp, QString dbName) {
    try {
        serverIp = "mongodb://" + serverIp;
        uri = mongocxx::uri(serverIp.toUtf8().constData());
        client = mongocxx::client{uri};

        if(dbName.isNull())
            db = client["local"];
        else
            db = client[dbName.toUtf8().constData()];
        qWarning() << "Connected!";
    }  catch (const mongocxx::exception &ex) {
        qWarning() << "Hit an exception: " << ex.what();
    }
}

std::vector<std::string> MongoStuff::getNamesDb(){
    auto names = client.list_database_names();
    return names;
}

std::vector<std::string> MongoStuff::getCollections(QString dbName){
    db = client[dbName.toUtf8().constData()];
    auto collections = db.list_collection_names();
    return collections;
}

QString MongoStuff::getDocuments(QString collectionName){

    QString jsonDocuments = "[";
    mongocxx::cursor cursor = db[collectionName.toUtf8().constData()].find({});
    for(auto doc : cursor) {
      jsonDocuments.append(QString::fromStdString(bsoncxx::to_json(doc))).append(", ");
    }
    jsonDocuments.remove(jsonDocuments.length()-2, 2);
    jsonDocuments.append(']');

    return jsonDocuments;
}

void MongoStuff::insertColl(QString collectionName){
    try {
        std::string keyName = "info";
        std::string valueName = "[string_value]";
        std::string keyAge = "number";
        int valueAge = 0;
        std::string keyDate = "date";

        bsoncxx::builder::basic::document basic_builder{};
        basic_builder.append(kvp(keyName, valueName), kvp(keyAge, valueAge), kvp(keyDate, bsoncxx::types::b_date(std::chrono::system_clock::now())));
        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = db[collectionName.toUtf8().constData()].insert_one(basic_builder.view());

    }  catch (const mongocxx::exception &ex) {
        qWarning() << "Ошибка при создании документа: " << ex.what();
    }
}

void MongoStuff::removeItem(QString collectionName, QString objectId){

    bsoncxx::builder::basic::document basic_builder{};
    basic_builder.append(kvp("_id", bsoncxx::oid(objectId.toUtf8().constData())));

    bsoncxx::stdx::optional<mongocxx::result::delete_result> result = db[collectionName.toUtf8().constData()].delete_one(basic_builder.view());   
}

void MongoStuff::updateDocument(QString collectionName, QString idDocument, QString keyField, QString valueField){

    try {
        if(keyField == "number")
            db[collectionName.toUtf8().constData()].update_one(make_document(kvp("_id", bsoncxx::oid(idDocument.toUtf8().constData()))),
                                                               make_document(kvp("$set", make_document(kvp(keyField.toStdString(), valueField.toDouble())))));
        else if(keyField == "date"){
            QDateTime dt = QDateTime::fromString(valueField, Qt::ISODate);
            time_t t = dt.toTime_t();
            auto timepoint = std::chrono::system_clock::from_time_t(t);
            bsoncxx::types::b_date date(timepoint);

            db[collectionName.toUtf8().constData()].update_one(make_document(kvp("_id", bsoncxx::oid(idDocument.toUtf8().constData()))),
                                                               make_document(kvp("$set", make_document(kvp(keyField.toStdString(), date)))));
        }
        else
            db[collectionName.toUtf8().constData()].update_one(make_document(kvp("_id", bsoncxx::oid(idDocument.toUtf8().constData()))),
                                                               make_document(kvp("$set", make_document(kvp(keyField.toStdString(), valueField.toStdString())))));
    }  catch (const mongocxx::exception &ex) {
        qWarning() << "Ошибка при создании документа: " << ex.what();
    }

}


QString MongoStuff::filter(QString collectionName, bsoncxx::types::b_date startDT, bsoncxx::types::b_date endDT, QString word){

    QString jsonDocuments = "[";

    mongocxx::cursor cursor = db[collectionName.toUtf8().constData()].find(
                make_document(kvp("$and", make_array(
                                            make_document(
                                             kvp("date", make_document(kvp("$gt", startDT), kvp("$lte", endDT)))
                                            ),
                                            make_document(
                                             kvp("info",make_document(kvp("$regex", word.toStdString())))
                                            )
                                      ))));
    for(auto doc : cursor) {
      jsonDocuments.append(QString::fromStdString(bsoncxx::to_json(doc))).append(", ");
    }
    jsonDocuments.remove(jsonDocuments.length()-2, 2);
    jsonDocuments.append(']');

    return jsonDocuments;
}

void MongoStuff::createCollection(std::string collectionName, QString dbName){

    auto collections = getCollections(dbName);

    if(std::find(collections.begin(), collections.end(), collectionName) != collections.end())
        return;

    db.create_collection(collectionName);
}

void MongoStuff::removeCollection(QString collectionName){
    db[collectionName.toUtf8().constData()].drop();
}



