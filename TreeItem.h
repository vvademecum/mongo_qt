#ifndef TREEITEM_H
#define TREEITEM_H

#include <QList>

class TreeItem
{
public:

    enum ItemType{
        None,
        Object,
        Array,
        Value
    };
public:
    explicit TreeItem(TreeItem *parent=nullptr);
    explicit TreeItem(const QHash<int,QVariant> &datas,TreeItem::ItemType type,TreeItem *parent=nullptr);
    ~TreeItem();

    bool insertChild(int row,TreeItem *child);
    bool removeChild(int row);
    bool insertChildren(int row,int count);
    bool removeChildren(int row,int count);
    void appendChild(TreeItem *child);
    void deleteAllChild();

    TreeItem *childItem(int row);
    TreeItem *parentItem();
    void setParentItem(TreeItem *parent);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    void setData(int column,const QVariant &val);
    int row() const;
    bool editable(int column) const;

    QString key() const;
    void setKey(const QString &key);
    QVariant value() const;
    void setValue(const QVariant &value);
    TreeItem::ItemType type() const;
    void setType(TreeItem::ItemType type);

private:
    TreeItem *theParentItem;
    QList<TreeItem*> theChildItems;
    ItemType theItemType;

    QHash<int,QVariant> theItemDatas;
};

#endif
