/* QDataflowCanvas - a dataflow widget for Qt
 * Copyright (C) 2017-2018 Federico Ferri
 * Copyright (C) 2018 Kuba Ober
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef QDATAFLOWMODEL_H
#define QDATAFLOWMODEL_H

#include <QObject>
#include <QSet>
#include <QList>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <initializer_list>

class QDataflowModelNode;
class QDataflowModelIOlet;
class QDataflowModelInlet;
class QDataflowModelOutlet;
class QDataflowModelConnection;
class QDataflowMetaObject;

class QDataflowModel : public QObject
{
    Q_OBJECT
public:
    explicit QDataflowModel(QObject *parent = {});

protected:
    virtual QDataflowModelNode * newNode(const QPoint &pos, const QString &text, int inletCount, int outletCount);
    virtual QDataflowModelConnection * newConnection(QDataflowModelNode *sourceNode, int sourceOutlet, QDataflowModelNode *destNode, int destInlet);

public:
    virtual QDataflowModelNode * create(const QPoint &pos, const QString &text, int inletCount, int outletCount);
    virtual void remove(QDataflowModelNode *node);
    virtual QDataflowModelConnection * connect(QDataflowModelConnection *conn);
    virtual QDataflowModelConnection * connect(QDataflowModelNode *sourceNode, int sourceOutlet, QDataflowModelNode *destNode, int destInlet);
    virtual void disconnect(QDataflowModelConnection *conn);
    virtual void disconnect(QDataflowModelNode *sourceNode, int sourceOutlet, QDataflowModelNode *destNode, int destInlet);

    QSet<QDataflowModelNode*> nodes();
    QSet<QDataflowModelConnection*> connections();

protected:
    virtual void addConnection(QDataflowModelConnection *conn);
    virtual void removeConnection(QDataflowModelConnection *conn);
    virtual QList<QDataflowModelConnection*> findConnections(QDataflowModelConnection *conn) const;
    virtual QList<QDataflowModelConnection*> findConnections(QDataflowModelNode *sourceNode, int sourceOutlet, QDataflowModelNode *destNode, int destInlet) const;
    virtual QList<QDataflowModelConnection*> findConnections(QDataflowModelOutlet *source, QDataflowModelInlet *dest) const;

Q_SIGNALS:
    void nodeAdded(QDataflowModelNode *node);
    void nodeRemoved(QDataflowModelNode *node);
    void nodeValidChanged(QDataflowModelNode *node, bool valid);
    void nodePosChanged(QDataflowModelNode *node, const QPoint &pos);
    void nodeTextChanged(QDataflowModelNode *node, const QString &text);
    void nodeInletCountChanged(QDataflowModelNode *node, int count);
    void nodeOutletCountChanged(QDataflowModelNode *node, int count);
    void connectionAdded(QDataflowModelConnection *conn);
    void connectionRemoved(QDataflowModelConnection *conn);

private Q_SLOTS:
    virtual void onValidChanged(bool valid);
    virtual void onPosChanged(const QPoint &pos);
    virtual void onTextChanged(const QString &text);
    virtual void onInletCountChanged(int count);
    virtual void onOutletCountChanged(int count);

private:
    QSet<QDataflowModelNode*> nodes_;
    QSet<QDataflowModelConnection*> connections_;
};

class QDataflowModelNode : public QObject
{
    Q_OBJECT
protected:
    explicit QDataflowModelNode(QDataflowModel *parent, const QPoint &pos, const QString &text, int inletCount, int outletCount);
    explicit QDataflowModelNode(QDataflowModel *parent, const QPoint &pos, const QString &text, const QStringList &inletTypes, const QStringList &outletTypes);

public:
    QDataflowModel * model();

    QDataflowMetaObject * dataflowMetaObject() const;
    void setDataflowMetaObject(QDataflowMetaObject *dataflowMetaObject);

    bool isValid() const;
    void setValid(bool valid);
    QPoint pos() const;
    QString text() const;
    QList<QDataflowModelInlet*> inlets() const;
    QDataflowModelInlet * inlet(int index) const;
    int inletCount() const;
    QList<QDataflowModelOutlet*> outlets() const;
    QDataflowModelOutlet * outlet(int index) const;
    int outletCount() const;

Q_SIGNALS:
    void validChanged(bool valid);
    void posChanged(const QPoint &pos);
    void textChanged(const QString &text);
    void inletCountChanged(int count);
    void outletCountChanged(int count);

public Q_SLOTS:
    void setPos(const QPoint &pos);
    void setText(const QString &text);
    void addInlet(const QString &name = {}, const QString &type = QStringLiteral("*"));
    void removeLastInlet();
    void setInletCount(int count);
    void setInletTypes(const QStringList &types);
    void setInletTypes(std::initializer_list<const char *> types);
    void addOutlet(const QString name = {}, const QString type = QStringLiteral("*"));
    void removeLastOutlet();
    void setOutletCount(int count);
    void setOutletTypes(const QStringList &types);
    void setOutletTypes(std::initializer_list<const char *> types);

protected Q_SLOTS:
    void addInlet(QDataflowModelInlet *inlet);
    void addOutlet(QDataflowModelOutlet *outlet);

private:
    bool valid_;
    QPoint pos_;
    QString text_;
    QList<QDataflowModelInlet*> inlets_;
    QList<QDataflowModelOutlet*> outlets_;
    QDataflowMetaObject *dataflowMetaObject_;

    friend class QDataflowModel;
};

QDebug operator<<(QDebug debug, const QDataflowModelNode &node);
QDebug operator<<(QDebug debug, const QDataflowModelNode *node);

class QDataflowModelIOlet : public QObject
{
    Q_OBJECT
protected:
    explicit QDataflowModelIOlet(QDataflowModelNode *parent, int index, const QString &name = {}, const QString &type = QStringLiteral("*"));

public:
    QDataflowModel * model();

    QDataflowModelNode * node() const;
    int index() const;
    QString name() const;
    QString type() const;

    void addConnection(QDataflowModelConnection *conn);
    void removeConnection(QDataflowModelConnection *conn);
    QList<QDataflowModelConnection*> connections() const;

private:
    QList<QDataflowModelConnection*> connections_;
    QDataflowModelNode *node_;
    int index_;
    QString name_;
    QString type_;
};

class QDataflowModelInlet : public QDataflowModelIOlet
{
    Q_OBJECT
protected:
    explicit QDataflowModelInlet(QDataflowModelNode *parent, int index, const QString &name = {}, const QString &type = QStringLiteral("*"));

public:
    bool canAcceptConnectionFrom(QDataflowModelOutlet *outlet);

private:
    friend class QDataflowModelNode;
};

QDebug operator<<(QDebug debug, const QDataflowModelInlet &inlet);
QDebug operator<<(QDebug debug, const QDataflowModelInlet *inlet);

class QDataflowModelOutlet : public QDataflowModelIOlet
{
    Q_OBJECT
protected:
    explicit QDataflowModelOutlet(QDataflowModelNode *parent, int index, const QString &name = {}, const QString &type = QStringLiteral("*"));

public:
    bool canMakeConnectionTo(QDataflowModelInlet *inlet);

private:
    friend class QDataflowModelNode;
};

QDebug operator<<(QDebug debug, const QDataflowModelOutlet &outlet);
QDebug operator<<(QDebug debug, const QDataflowModelOutlet *outlet);

class QDataflowModelConnection : public QObject
{
    Q_OBJECT
protected:
    explicit QDataflowModelConnection(QDataflowModel *parent, QDataflowModelOutlet *source, QDataflowModelInlet *dest);

public:
    QDataflowModel * model();

    QDataflowModelOutlet * source() const;
    QDataflowModelInlet * dest() const;

private:
    QDataflowModelOutlet *source_;
    QDataflowModelInlet *dest_;

    friend class QDataflowModel;
};

QDebug operator<<(QDebug debug, const QDataflowModelConnection &conn);
QDebug operator<<(QDebug debug, const QDataflowModelConnection *conn);

class QDataflowMetaObject
{
public:
    QDataflowMetaObject(QDataflowModelNode *node);
    virtual ~QDataflowMetaObject() = default;

    QDataflowModelNode * node() {return node_;}
    void setNode(QDataflowModelNode *node) {node_ = node;}
    QDataflowModelInlet * inlet(int index) {return node_->inlet(index);}
    QDataflowModelOutlet * outlet(int index) {return node_->outlet(index);}
    int inletCount() {return node_->inletCount();}
    void setInletCount(int c) {node_->setInletCount(c);}
    void setInletTypes(std::initializer_list<const char*> types) {node_->setInletTypes(types);}
    int outletCount() {return node_->outletCount();}
    void setOutletCount(int c) {node_->setOutletCount(c);}
    void setOutletTypes(std::initializer_list<const char*> types) {node_->setOutletTypes(types);}
    virtual void onDataReceved(int inlet, void *data);
    void sendData(int outlet, void *data);

private:
    QDataflowModelNode *node_;

    friend class QDataflowModelNode;
};

class QDataflowModelDebugSignals : public QObject
{
    Q_OBJECT
public:
    explicit QDataflowModelDebugSignals(QDataflowModel *parent);

private:
    QDebug debug() const;

private Q_SLOTS:
    void onNodeAdded(QDataflowModelNode *node);
    void onNodeRemoved(QDataflowModelNode *node);
    void onNodePosChanged(QDataflowModelNode *node, const QPoint &pos);
    void onNodeTextChanged(QDataflowModelNode *node, const QString &text);
    void onNodeInletCountChanged(QDataflowModelNode *node, int count);
    void onNodeOutletCountChanged(QDataflowModelNode *node, int count);
    void onConnectionAdded(QDataflowModelConnection *conn);
    void onConnectionRemoved(QDataflowModelConnection *conn);
};

#endif // QDATAFLOWMODEL_H
