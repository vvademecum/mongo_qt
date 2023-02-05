#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QVariant>

#include "MongoStuff.h"
#include "TreeItem.h"

class TreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit TreeModel(QObject *parent = nullptr);
    ~TreeModel();


    bool loadJson(const QString &jsonDoc_str, const QString &collName);
    bool dumpJson(const QString &filepath);

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    bool insertRows(int row, int count,
                    const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count,
                    const QModelIndex &parent = QModelIndex()) override;


private:
    TreeItem *getItem(const QModelIndex &index) const;

    void parseObject(const QString &key,const QJsonObject& obj,TreeItem *&item);
    void parseArray(const QString &key,const QJsonArray& arr,TreeItem *&item);
    void parseValue(const QString &key,const QJsonValue& val,TreeItem *&item);

    QVariantMap dumpObject(TreeItem *&item) const;
    QVariantList dumpArray(TreeItem *&item) const;
    QVariant dumpValue(TreeItem *&item) const;

private:
    TreeItem *theRootItem;
};

#endif
