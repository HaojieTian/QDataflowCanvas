/* QDataflowCanvas - a dataflow widget for Qt
 * Copyright (C) 2017 Federico Ferri
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
#include "qdataflowmodel.h"
#include "qdataflowcanvas.h"

QDataflowModel::QDataflowModel(QObject *parent)
    : QObject(parent)
{

}

QDataflowModelNode * QDataflowModel::newNode(const QPoint &pos, const QString &text, int inletCount, int outletCount)
{
    QDataflowModelNode *node = new QDataflowModelNode({}, pos, text, inletCount, outletCount);
    node->moveToThread(thread());
    node->setParent(this);
    return node;
}

QDataflowModelConnection * QDataflowModel::newConnection(QDataflowModelNode *sourceNode, int sourceOutlet, QDataflowModelNode *destNode, int destInlet)
{
    QDataflowModelConnection *conn = new QDataflowModelConnection({}, sourceNode->outlet(sourceOutlet), destNode->inlet(destInlet));
    conn->moveToThread(thread());
    conn->setParent(this);
    return conn;
}

QDataflowModelNode * QDataflowModel::create(const QPoint &pos, const QString &text, int inletCount, int outletCount)
{
    QDataflowModelNode *node = newNode(pos, text, inletCount, outletCount);
    nodes_.insert(node);
    QObject::connect(node, &QDataflowModelNode::validChanged, this, &QDataflowModel::onValidChanged);
    QObject::connect(node, &QDataflowModelNode::posChanged, this, &QDataflowModel::onPosChanged);
    QObject::connect(node, &QDataflowModelNode::textChanged, this, &QDataflowModel::onTextChanged);
    QObject::connect(node, &QDataflowModelNode::inletCountChanged, this, &QDataflowModel::onInletCountChanged);
    QObject::connect(node, &QDataflowModelNode::outletCountChanged, this, &QDataflowModel::onOutletCountChanged);
    emit nodeAdded(node);
    return node;
}

void QDataflowModel::remove(QDataflowModelNode *node)
{
    if(!node) return;
    if(!nodes_.contains(node)) return;
    foreach(QDataflowModelInlet *inlet, node->inlets())
        foreach(QDataflowModelConnection *conn, inlet->connections())
            removeConnection(conn);
    foreach(QDataflowModelOutlet *outlet, node->outlets())
        foreach(QDataflowModelConnection *conn, outlet->connections())
            removeConnection(conn);
    QObject::disconnect(node, &QDataflowModelNode::validChanged, this, &QDataflowModel::onValidChanged);
    QObject::disconnect(node, &QDataflowModelNode::posChanged, this, &QDataflowModel::onPosChanged);
    QObject::disconnect(node, &QDataflowModelNode::textChanged, this, &QDataflowModel::onTextChanged);
    QObject::disconnect(node, &QDataflowModelNode::inletCountChanged, this, &QDataflowModel::onInletCountChanged);
    QObject::disconnect(node, &QDataflowModelNode::outletCountChanged, this, &QDataflowModel::onOutletCountChanged);
    nodes_.remove(node);
    emit nodeRemoved(node);
}

QDataflowModelConnection * QDataflowModel::connect(QDataflowModelConnection *conn)
{
    if(!conn) return {};
    if(!findConnections(conn).isEmpty()) return {};
    addConnection(conn);
    return conn;
}

QDataflowModelConnection * QDataflowModel::connect(QDataflowModelNode *sourceNode, int sourceOutlet, QDataflowModelNode *destNode, int destInlet)
{
    if(!sourceNode || !destNode) return {};
    if(!findConnections(sourceNode, sourceOutlet, destNode, destInlet).isEmpty()) return {};
    QDataflowModelConnection *conn = newConnection(sourceNode, sourceOutlet, destNode, destInlet);
    addConnection(conn);
    return conn;
}

void QDataflowModel::disconnect(QDataflowModelConnection *conn)
{
    if(!conn) return;
    foreach(QDataflowModelConnection *conn, findConnections(conn))
    {
        removeConnection(conn);
    }
}

void QDataflowModel::disconnect(QDataflowModelNode *sourceNode, int sourceOutlet, QDataflowModelNode *destNode, int destInlet)
{
    if(!sourceNode || !destNode) return;
    foreach(QDataflowModelConnection *conn, findConnections(sourceNode, sourceOutlet, destNode, destInlet))
    {
        removeConnection(conn);
    }
}

QSet<QDataflowModelNode*> QDataflowModel::nodes()
{
    return nodes_;
}

QSet<QDataflowModelConnection*> QDataflowModel::connections()
{
    return connections_;
}

void QDataflowModel::addConnection(QDataflowModelConnection *conn)
{
    if(!conn) return;
    if(!findConnections(conn).isEmpty()) return;
    if(!conn->source()->canMakeConnectionTo(conn->dest()) || !conn->dest()->canAcceptConnectionFrom(conn->source()))
    {
        qDebug() << "cannoct connect outlet" << conn->source() << "to inlet" << conn->dest();
        return;
    }
    conn->setParent(this);
    connections_.insert(conn);
    conn->source()->addConnection(conn);
    conn->dest()->addConnection(conn);
    emit connectionAdded(conn);
}

void QDataflowModel::removeConnection(QDataflowModelConnection *conn)
{
    if(!conn) return;
    if(!connections_.contains(conn)) return;
    conn->source()->removeConnection(conn);
    conn->dest()->removeConnection(conn);
    connections_.remove(conn);
    emit connectionRemoved(conn);
}

QList<QDataflowModelConnection*> QDataflowModel::findConnections(QDataflowModelConnection *conn) const
{
    if(!conn) return QList<QDataflowModelConnection*>();
    return findConnections(conn->source(), conn->dest());
}

QList<QDataflowModelConnection*> QDataflowModel::findConnections(QDataflowModelOutlet *source, QDataflowModelInlet *dest) const
{
    if(!source || !dest) return QList<QDataflowModelConnection*>();
    return findConnections(source->node(), source->index(), dest->node(), dest->index());
}

QList<QDataflowModelConnection*> QDataflowModel::findConnections(QDataflowModelNode *sourceNode, int sourceOutlet, QDataflowModelNode *destNode, int destInlet) const
{
    if(!sourceNode || !destNode) return QList<QDataflowModelConnection*>();
    QList<QDataflowModelConnection*> ret;
    foreach(QDataflowModelConnection *conn, connections_)
    {
        QDataflowModelOutlet *src = conn->source();
        QDataflowModelInlet *dst = conn->dest();
        if(src->node() == sourceNode && src->index() == sourceOutlet && dst->node() == destNode && dst->index() == destInlet)
            ret.push_back(conn);
    }
    return ret;
}

void QDataflowModel::onValidChanged(bool valid)
{
    if(QDataflowModelNode *node = dynamic_cast<QDataflowModelNode*>(sender()))
        emit nodeValidChanged(node, valid);
}

void QDataflowModel::onPosChanged(const QPoint &pos)
{
    if(QDataflowModelNode *node = dynamic_cast<QDataflowModelNode*>(sender()))
        emit nodePosChanged(node, pos);
}

void QDataflowModel::onTextChanged(const QString &text)
{
    if(QDataflowModelNode *node = dynamic_cast<QDataflowModelNode*>(sender()))
        emit nodeTextChanged(node, text);
}

void QDataflowModel::onInletCountChanged(int count)
{
    if(QDataflowModelNode *node = dynamic_cast<QDataflowModelNode*>(sender()))
        emit nodeInletCountChanged(node, count);
}

void QDataflowModel::onOutletCountChanged(int count)
{
    if(QDataflowModelNode *node = dynamic_cast<QDataflowModelNode*>(sender()))
        emit nodeOutletCountChanged(node, count);
}

QDataflowModelNode::QDataflowModelNode(QDataflowModel *parent, const QPoint &pos, const QString &text, int inletCount, int outletCount)
    : QObject(parent), valid_(false), pos_(pos), text_(text), dataflowMetaObject_()
{
    for(int i = 0; i < inletCount; i++) addInlet();
    for(int i = 0; i < outletCount; i++) addOutlet();
}

QDataflowModelNode::QDataflowModelNode(QDataflowModel *parent, const QPoint &pos, const QString &text, const QStringList &inletTypes, const QStringList &outletTypes)
    : QObject(parent), valid_(false), pos_(pos), text_(text), dataflowMetaObject_()
{
    foreach(const QString &inletType, inletTypes) addInlet(inletType);
    foreach(const QString &outletType, outletTypes) addOutlet(outletType);
}

QDataflowModel * QDataflowModelNode::model()
{
    return static_cast<QDataflowModel*>(parent());
}

QDataflowMetaObject * QDataflowModelNode::dataflowMetaObject() const
{
    return dataflowMetaObject_;
}

void QDataflowModelNode::setDataflowMetaObject(QDataflowMetaObject *dataflowMetaObject)
{
    if(dataflowMetaObject_)
        delete dataflowMetaObject_;

    dataflowMetaObject_ = dataflowMetaObject;

    if(dataflowMetaObject_)
        dataflowMetaObject_->node_ = this;
}

bool QDataflowModelNode::isValid() const
{
    return valid_;
}

QPoint QDataflowModelNode::pos() const
{
    return pos_;
}

QString QDataflowModelNode::text() const
{
    return text_;
}

QList<QDataflowModelInlet*> QDataflowModelNode::inlets() const
{
    return inlets_;
}

QDataflowModelInlet * QDataflowModelNode::inlet(int index) const
{
    if(index >= 0 && index < inlets_.length())
        return inlets_[index];
    else
        return {};
}

int QDataflowModelNode::inletCount() const
{
    return inlets_.length();
}

QList<QDataflowModelOutlet*> QDataflowModelNode::outlets() const
{
    return outlets_;
}

QDataflowModelOutlet * QDataflowModelNode::outlet(int index) const
{
    if(index >= 0 && index < outlets_.length())
        return outlets_[index];
    else
        return {};
}

int QDataflowModelNode::outletCount() const
{
    return outlets_.length();
}

void QDataflowModelNode::setValid(bool valid)
{
    if(valid_ == valid) return;
    valid_ = valid;
    emit validChanged(valid);
}

void QDataflowModelNode::setPos(const QPoint &pos)
{
    if(pos_ == pos) return;
    pos_ = pos;
    emit posChanged(pos);
}

void QDataflowModelNode::setText(const QString &text)
{
    if(text_ == text) return;
    text_ = text;
    emit textChanged(text);
}

void QDataflowModelNode::addInlet(const QString &name, const QString &type)
{
    addInlet(new QDataflowModelInlet(this, inletCount(), name, type));
}

void QDataflowModelNode::removeLastInlet()
{
    if(inlets_.isEmpty()) return;
    QDataflowModelInlet *inlet = inlets_.back();
    foreach(QDataflowModelConnection *conn, inlet->connections())
        model()->disconnect(conn);
    inlets_.pop_back();
    emit inletCountChanged(inletCount());
}

void QDataflowModelNode::setInletCount(int count)
{
    if(inletCount() == count) return;

    bool shouldBlockSignals = blockSignals(true);

    while(inletCount() < count)
        addInlet();

    while(inletCount() > count)
        removeLastInlet();

    blockSignals(shouldBlockSignals);

    emit inletCountChanged(count);
}

void QDataflowModelNode::setInletTypes(const QStringList &types)
{
    int oldCount = inletCount();

    bool shouldBlockSignals = blockSignals(true);

    while(inletCount() > 0)
        removeLastInlet();

    foreach(const QString &type, types)
        addInlet("", type);

    blockSignals(shouldBlockSignals);

    int newCount = inletCount();
    if(oldCount != newCount)
        emit inletCountChanged(newCount);
}

void QDataflowModelNode::setInletTypes(std::initializer_list<const char*> types_)
{
    QStringList types;
    types.reserve(types_.size());

    for(const char *type : types_)
    {
        types << QString::fromUtf8(type);
    }

    setInletTypes(types);
}

void QDataflowModelNode::addOutlet(QString name, QString type)
{
    addOutlet(new QDataflowModelOutlet(this, outletCount(), name, type));
}

void QDataflowModelNode::removeLastOutlet()
{
    if(outlets_.isEmpty()) return;
    QDataflowModelOutlet *outlet = outlets_.back();
    foreach(QDataflowModelConnection *conn, outlet->connections())
        model()->disconnect(conn);
    outlets_.pop_back();
    emit outletCountChanged(outletCount());
}

void QDataflowModelNode::setOutletCount(int count)
{
    if(outletCount() == count) return;

    bool shouldBlockSignals = blockSignals(true);

    while(outletCount() < count)
        addOutlet();

    while(outletCount() > count)
        removeLastOutlet();

    blockSignals(shouldBlockSignals);

    emit outletCountChanged(count);
}

void QDataflowModelNode::setOutletTypes(const QStringList &types)
{
    int oldCount = outletCount();

    bool shouldBlockSignals = blockSignals(true);

    while(outletCount() > 0)
        removeLastOutlet();

    foreach(const QString &type, types)
        addOutlet("", type);

    blockSignals(shouldBlockSignals);

    int newCount = outletCount();
    if(oldCount != newCount)
        emit outletCountChanged(newCount);
}

void QDataflowModelNode::setOutletTypes(std::initializer_list<const char *> types_)
{
    QStringList types;
    types.reserve(types_.size());

    for(const char *type : types_)
    {
        types << QString::fromUtf8(type);
    }

    setOutletTypes(types);
}

void QDataflowModelNode::addInlet(QDataflowModelInlet *inlet)
{
    if(!inlet) return;
    inlet->setParent(this);
    inlets_.append(inlet);
    emit inletCountChanged(inletCount());
}

void QDataflowModelNode::addOutlet(QDataflowModelOutlet *outlet)
{
    if(!outlet) return;
    outlet->setParent(this);
    outlets_.append(outlet);
    emit outletCountChanged(outletCount());
}

QDebug operator<<(QDebug debug, const QDataflowModelNode &node)
{
    QDebugStateSaver stateSaver(debug);
    debug.nospace() << "QDataflowModelNode";
    debug.nospace() << "(" << reinterpret_cast<const void*>(&node) <<
                       ", text=" << node.text() << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const QDataflowModelNode *node)
{
    return debug << *node;
}

QDataflowModelIOlet::QDataflowModelIOlet(QDataflowModelNode *parent, int index, const QString &name, const QString &type)
    : QObject(parent), node_(parent), index_(index), name_(name), type_(type)
{

}

QDataflowModel * QDataflowModelIOlet::model()
{
    return node_->model();
}

QDataflowModelNode * QDataflowModelIOlet::node() const
{
    return node_;
}

int QDataflowModelIOlet::index() const
{
    return index_;
}

QString QDataflowModelIOlet::type() const
{
    return type_;
}

void QDataflowModelIOlet::addConnection(QDataflowModelConnection *conn)
{
    connections_.push_back(conn);
}

void QDataflowModelIOlet::removeConnection(QDataflowModelConnection *conn)
{
    connections_.removeAll(conn);
}

QList<QDataflowModelConnection*> QDataflowModelIOlet::connections() const
{
    return connections_;
}

QDataflowModelInlet::QDataflowModelInlet(QDataflowModelNode *parent, int index, const QString &name, const QString &type)
    : QDataflowModelIOlet(parent, index, name, type)
{

}

bool QDataflowModelInlet::canAcceptConnectionFrom(QDataflowModelOutlet *outlet)
{
    if(type() == "*") return true;
    return type() == outlet->type();
}

QDebug operator<<(QDebug debug, const QDataflowModelInlet &inlet)
{
    QDebugStateSaver stateSaver(debug);
    debug.nospace() << "QDataflowModelInlet";
    debug.nospace() << "(" << reinterpret_cast<const void*>(&inlet) <<
                       ", node=" << inlet.node() << ", index=" << inlet.index() << ", type=" << inlet.type() << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const QDataflowModelInlet *inlet)
{
    return debug << *inlet;
}

QDataflowModelOutlet::QDataflowModelOutlet(QDataflowModelNode *parent, int index, const QString &name, const QString &type)
    : QDataflowModelIOlet(parent, index, name, type)
{

}

bool QDataflowModelOutlet::canMakeConnectionTo(QDataflowModelInlet *inlet)
{
    if(inlet->type() == "*") return true;
    return type() == inlet->type();
}

QDebug operator<<(QDebug debug, const QDataflowModelOutlet &outlet)
{
    QDebugStateSaver stateSaver(debug);
    debug.nospace() << "QDataflowModelOutlet";
    debug.nospace() << "(" << reinterpret_cast<const void*>(&outlet) <<
                       ", node=" << outlet.node() << ", index=" << outlet.index() << ", type=" << outlet.type() << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const QDataflowModelOutlet *outlet)
{
    return debug << *outlet;
}

QDataflowModelConnection::QDataflowModelConnection(QDataflowModel *parent, QDataflowModelOutlet *source, QDataflowModelInlet *dest)
    : QObject(parent), source_(source), dest_(dest)
{
}

QDataflowModel * QDataflowModelConnection::model()
{
    return static_cast<QDataflowModel*>(parent());
}

QDataflowModelOutlet * QDataflowModelConnection::source() const
{
    return source_;
}

QDataflowModelInlet * QDataflowModelConnection::dest() const
{
    return dest_;
}

QDebug operator<<(QDebug debug, const QDataflowModelConnection &conn)
{
    QDebugStateSaver stateSaver(debug);
    debug.nospace() << "QDataflowModelConnection";
    debug.nospace() << "(" << reinterpret_cast<const void*>(&conn) <<
                       ", src=" << conn.source() << ", dst=" << conn.dest() << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const QDataflowModelConnection *conn)
{
    return debug << *conn;
}

QDataflowMetaObject::QDataflowMetaObject(QDataflowModelNode *node)
    : node_(node)
{
}

void QDataflowMetaObject::onDataReceved(int inlet, void *data)
{
    Q_UNUSED(inlet);
    Q_UNUSED(data);
}

void QDataflowMetaObject::sendData(int outletIndex, void *data)
{
    foreach(QDataflowModelConnection *conn, outlet(outletIndex)->connections())
    {
        QDataflowMetaObject *mo = conn->dest()->node()->dataflowMetaObject();
        if(mo)
            mo->onDataReceved(conn->dest()->index(), data);
    }
}

QDataflowModelDebugSignals::QDataflowModelDebugSignals(QDataflowModel *parent)
    : QObject(parent)
{
    QObject::connect(parent, &QDataflowModel::nodeAdded, this, &QDataflowModelDebugSignals::onNodeAdded);
    QObject::connect(parent, &QDataflowModel::nodeRemoved, this, &QDataflowModelDebugSignals::onNodeRemoved);
    QObject::connect(parent, &QDataflowModel::nodePosChanged, this, &QDataflowModelDebugSignals::onNodePosChanged);
    QObject::connect(parent, &QDataflowModel::nodeTextChanged, this, &QDataflowModelDebugSignals::onNodeTextChanged);
    QObject::connect(parent, &QDataflowModel::nodeInletCountChanged, this, &QDataflowModelDebugSignals::onNodeInletCountChanged);
    QObject::connect(parent, &QDataflowModel::nodeOutletCountChanged, this, &QDataflowModelDebugSignals::onNodeOutletCountChanged);
    QObject::connect(parent, &QDataflowModel::connectionAdded, this, &QDataflowModelDebugSignals::onConnectionAdded);
    QObject::connect(parent, &QDataflowModel::connectionRemoved, this, &QDataflowModelDebugSignals::onConnectionRemoved);
}

QDebug QDataflowModelDebugSignals::debug() const
{
    return qDebug() << parent();
}

void QDataflowModelDebugSignals::onNodeAdded(QDataflowModelNode *node)
{
    debug() << "nodeAdded" << node;
}

void QDataflowModelDebugSignals::onNodeRemoved(QDataflowModelNode *node)
{
    debug() << "nodeRemoved" << node;
}

void QDataflowModelDebugSignals::onNodePosChanged(QDataflowModelNode *node, const QPoint &pos)
{
    debug() << "nodePosChanged" << node << pos;
}

void QDataflowModelDebugSignals::onNodeTextChanged(QDataflowModelNode *node, const QString &text)
{
    debug() << "nodeTextChanged" << node << text;
}

void QDataflowModelDebugSignals::onNodeInletCountChanged(QDataflowModelNode *node, int count)
{
    debug() << "nodeInletCountChanged" << node << count;
}

void QDataflowModelDebugSignals::onNodeOutletCountChanged(QDataflowModelNode *node, int count)
{
    debug() << "nodeOutletCountChanged" << node << count;
}

void QDataflowModelDebugSignals::onConnectionAdded(QDataflowModelConnection *conn)
{
    debug() << "connectionAdded" << conn;
}

void QDataflowModelDebugSignals::onConnectionRemoved(QDataflowModelConnection *conn)
{
    debug() << "connectionRemoved" << conn;
}
