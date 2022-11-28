#include "TreeItem.h"

TreeItem::TreeItem(TreeItem *parent)
    :theParentItem(parent)
    ,theItemType(TreeItem::None)
    ,theItemDatas({{0,"[Key]"},{1,"[Value]"}})
{
}

TreeItem::TreeItem(const QHash<int, QVariant> &datas, TreeItem::ItemType type, TreeItem *parent)
    :theParentItem(parent)
    ,theItemType(type)
    ,theItemDatas(datas)
{
}

TreeItem::~TreeItem()
{
    deleteAllChild();
}

bool TreeItem::insertChild(int row, TreeItem *child)
{
    if(row<0||row>theChildItems.count())
        return false;
    theChildItems.insert(row,child);
    child->setParentItem(this);
    return true;
}

bool TreeItem::removeChild(int row)
{
    if(row<0||row+1>theChildItems.count())
        return false;
    delete theChildItems.takeAt(row);
    return true;
}

bool TreeItem::insertChildren(int row, int count)
{
    if(row<0||row>theChildItems.count())
        return false;
    for(int i=0;i<count;i++){
        TreeItem *item=new TreeItem(this);
        item->setType(TreeItem::Value);
        theChildItems.insert(row,item);
    }
    return true;
}

bool TreeItem::removeChildren(int row, int count)
{
    if (row<0||row+count>theChildItems.count())
        return false;
    for(int i=0;i<count;i++){
        delete theChildItems.takeAt(row+i);
    }
    return true;
}

void TreeItem::appendChild(TreeItem *child)
{
    theChildItems.append(child);
    child->setParentItem(this);
}

void TreeItem::deleteAllChild()
{
    qDeleteAll(theChildItems);
    theChildItems.clear();
}

TreeItem *TreeItem::childItem(int row)
{
    return theChildItems.value(row);
}

TreeItem *TreeItem::parentItem()
{
    return theParentItem;
}

void TreeItem::setParentItem(TreeItem *parent)
{
    theParentItem=parent;
}

int TreeItem::childCount() const
{
    return theChildItems.count();
}

int TreeItem::columnCount() const
{
    return theItemDatas.count();
}

QVariant TreeItem::data(int column) const
{
    return theItemDatas.value(column,QVariant());
}

void TreeItem::setData(int column, const QVariant &val)
{
    theItemDatas.insert(column,val);
}

int TreeItem::row() const
{
    if(theParentItem)
        return theParentItem->theChildItems.indexOf(const_cast<TreeItem*>(this));
    return 0;
}

bool TreeItem::editable(int column) const
{

    if((!theParentItem||!theParentItem->parentItem())||
            ((0==column)&&(theParentItem->type()==TreeItem::Array))||
            ((1==column)&&((type()==TreeItem::Array)||(type()==TreeItem::Object))))
        return false;
    return true;
}

QString TreeItem::key() const
{
    return theItemDatas.value(0,"").toString();
}

void TreeItem::setKey(const QString &key)
{
    theItemDatas[0]=key;
}

QVariant TreeItem::value() const
{
    return theItemDatas.value(1,0);
}

void TreeItem::setValue(const QVariant &value)
{
    theItemDatas[1]=value;
}

TreeItem::ItemType TreeItem::type() const
{
    return theItemType;
}

void TreeItem::setType(TreeItem::ItemType type)
{
    theItemType=type;
}

