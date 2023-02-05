#include "TreeModel.h"
#include <QFile>
#include <QDebug>
#include <QDate>
#include <ctime>
#include <iomanip>

#include <iostream>
#include "mainwindow.h"


extern MainWindow *theMainWin;

TreeModel::TreeModel(QObject *parent)
    : QAbstractItemModel(parent),theRootItem(new TreeItem(nullptr))
{

}

TreeModel::~TreeModel()
{
    delete theRootItem;
}

bool TreeModel::loadJson(const QString &jsonDoc_str, const QString &collName)
{
    QByteArray jsonDoc = jsonDoc_str.toLocal8Bit();

    const QByteArray raw_data = jsonDoc;

    QJsonParseError json_error;
    QJsonDocument json_doc=QJsonDocument::fromJson(raw_data,&json_error);

    if(json_doc.isNull()||json_doc.isEmpty()||json_error.error!=QJsonParseError::NoError)
        return false;

    emit beginResetModel();
    theRootItem->deleteAllChild();

    if(json_doc.isObject()){
        parseObject(collName, json_doc.object(),theRootItem);

    }else if(json_doc.isArray()){
        parseArray(collName, json_doc.array(),theRootItem);

    }
    emit endResetModel();

    qDebug()<<"json получен";
    return true;
}

bool TreeModel::dumpJson(const QString &filepath)
{
    if(filepath.isEmpty())
        return false;

    if(!theRootItem||theRootItem->childCount()==0)
        return false;
    TreeItem *top_level_item=theRootItem->childItem(0);
    if(!top_level_item)
        return false;

    QJsonDocument json_doc;
    switch (top_level_item->type()) {
    case TreeItem::Object:
        json_doc=QJsonDocument::fromVariant(dumpObject(top_level_item));
        break;
    case TreeItem::Array:
        json_doc=QJsonDocument::fromVariant(dumpArray(top_level_item));
        break;
    default:
        break;
    }

    QFile file(filepath);
    if(!file.open(QIODevice::WriteOnly|QIODevice::Text))
        return false;

    file.write(json_doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *parentItem=getItem(parent);
    TreeItem *childItem = parentItem->childItem(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = getItem(index);
    TreeItem *parentItem = childItem->parentItem();

    if (parentItem == theRootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem = getItem(parent);
    return parentItem->childCount();
}

int TreeModel::columnCount(const QModelIndex &parent) const
{
    TreeItem *parentItem = getItem(parent);
    return parentItem->columnCount();
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    TreeItem *item = getItem(index);
    return item->data(index.column());
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    TreeItem *item = getItem(index);
    return (item->editable(index.column())?Qt::ItemIsEditable:Qt::NoItemFlags)
            |QAbstractItemModel::flags(index);
}

bool TreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    TreeItem *item = getItem(index);
    item->setData(index.column(), value);
    emit dataChanged(index, index, {role});

    if (theMainWin)
        theMainWin->updateDocument();

    return true;
}

bool TreeModel::insertRows(int row, int count, const QModelIndex &parent)
{
    TreeItem *parentItem=getItem(parent);

    beginInsertRows(parent, row, row+count-1);
    const bool result=parentItem->insertChildren(row,count);
    endInsertRows();

    return result;
}

bool TreeModel::removeRows(int row, int count, const QModelIndex &parent)
{
    TreeItem *parentItem=getItem(parent);

    beginRemoveRows(parent, row, row+count-1);
    const bool result=parentItem->removeChildren(row,count);
    endRemoveRows();

    return result;
}

TreeItem *TreeModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return theRootItem;
}

void TreeModel::parseObject(const QString &key, const QJsonObject &obj, TreeItem *&item)
{
    if(obj.contains("$oid")) {
        TreeItem *child=new TreeItem({{0,key},{1,obj.find("$oid").value().toString()}},TreeItem::Value,item);
        item->appendChild(child);
    }
    else
    if (obj.contains("$date")) {
        long duration = obj.find("$date").value().toVariant().toLongLong();

        std::chrono::milliseconds msDuration(duration);
        std::chrono::time_point<std::chrono::system_clock> dt(msDuration);
        auto convertedDateTime = std::chrono::system_clock::to_time_t(dt);

        //Mon Dec 31 23:34:34 2019
//        TreeItem *child=new TreeItem({{0,key},{1, std::ctime(&convertedDateTime)}},TreeItem::Value,item);

        std::string datetime(100,0);
        datetime.resize(std::strftime(&datetime[0], datetime.size(),
            "%F %T", std::localtime(&convertedDateTime)));

        TreeItem *child=new TreeItem({{0,key},{1, QString::fromStdString(datetime)}},TreeItem::Value,item);

        item->appendChild(child);
    }
    else {
        TreeItem *child=new TreeItem({{0,key},{1,"[Object]"}},TreeItem::Object,item);
        item->appendChild(child);

        const QStringList keys=obj.keys();
        for(const QString &item_key:keys){
            parseValue(item_key,obj.value(item_key),child);
        }
    }
}

void TreeModel::parseArray(const QString &key, const QJsonArray &arr, TreeItem *&item)
{
    TreeItem *child=new TreeItem({{0,key},{1,"[Array]"}},TreeItem::Array,item);
    item->appendChild(child);

    for(int i=0;i<arr.count();i++){
        std::string num = "[" + std::to_string(i) + "]";
        parseValue(QString::fromStdString(num), arr.at(i), child);
    }
}

void TreeModel::parseValue(const QString &key, const QJsonValue &val, TreeItem *&item)
{
    QVariant the_val;

    switch (val.type()) {
    case QJsonValue::Object:
        parseObject(key,val.toObject(),item);
        return;
        break;
    case QJsonValue::Array:
        parseArray(key,val.toArray(),item);
        return;
        break;
    case QJsonValue::Bool:
        the_val=val.toBool();
        break;
    case QJsonValue::Double:
        the_val=val.toDouble();
        break;
    case QJsonValue::String:
        the_val=val.toString();
        break;
    case QJsonValue::Null: break;
    case QJsonValue::Undefined: break;
    default: break;
    }

    TreeItem *child=new TreeItem({{0,key},{1,the_val}},TreeItem::Value,item);
    item->appendChild(child);
}

QVariantMap TreeModel::dumpObject(TreeItem *&item) const
{
    QVariantMap json_obj;
    const int child_count=item->childCount();
    for(int i=0;i<child_count;i++){
        TreeItem *child=item->childItem(i);
        if(!child) continue;

        switch (child->type()) {
        case TreeItem::Object:
            json_obj.insert(child->key(),dumpObject(child));
            break;
        case TreeItem::Array:
            json_obj.insert(child->key(),dumpArray(child));
            break;
        case TreeItem::Value:
            json_obj.insert(child->key(),dumpValue(child));
            break;
        default:
            break;
        }
    }
    return json_obj;
}

QVariantList TreeModel::dumpArray(TreeItem *&item) const
{
    QVariantList json_arr;
    const int child_count=item->childCount();
    for(int i=0;i<child_count;i++){
        TreeItem *child=item->childItem(i);
        if(!child) continue;
        switch (child->type()) {
        case TreeItem::Object:
            json_arr.append(dumpObject(child));
            break;
        case TreeItem::Array:
            json_arr.append(dumpArray(child));
            break;
        case TreeItem::Value:
            json_arr.append(dumpValue(child));
            break;
        default:
            break;
        }
    }
    return json_arr;
}

QVariant TreeModel::dumpValue(TreeItem *&item) const
{
    return item->value();
}

