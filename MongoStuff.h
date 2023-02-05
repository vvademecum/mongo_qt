#ifndef MONGOSTUFF_H
#define MONGOSTUFF_H

#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/types.hpp>
#include <QComboBox>



class MongoStuff
{

public:
    MongoStuff();
    mongocxx::instance instance {};
    mongocxx::database db;
    mongocxx::uri uri;
    mongocxx::client client;
    QString curCollection;
    QString curId;
    QString valueField;
    QString keyField;

    void connectToDb(QString serverIp, QString dbName);
    void insertColl(QString collectionName);
    void removeItem(QString collectionName, QString objectId);
    QString filter(QString collectionName, bsoncxx::types::b_date startDT, bsoncxx::types::b_date endDT, QString word);

    std::vector<std::string> getNamesDb();
    std::vector<std::string> getCollections(QString dbName);
    QString getDocuments(QString collectionName);
    void updateDocument(QString collectionName, QString idDocument, QString keyField, QString valueField);
    void createCollection(std::string collectionName, QString dbName);
    void removeCollection(QString collectionName);

};

#endif // MONGOSTUFF_H
