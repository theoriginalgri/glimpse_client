#include "schedulermodel.h"
#include "scheduler.h"

#include <QPointer>
#include <QTimer>

class SchedulerModel::Private : public QObject
{
    Q_OBJECT

public:
    Private(SchedulerModel *q)
    : q(q)
    {
        connect(&updateTimer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    }

    SchedulerModel *q;

    QPointer<Scheduler> scheduler;
    ScheduleDefinitionList tests;

    QTimer updateTimer;

public slots:
    void testAdded(const ScheduleDefinition &test, int position);
    void testRemoved(const ScheduleDefinition &test, int position);
    void testMoved(const ScheduleDefinition &test, int from, int to);

    void onTimeout();
};

void SchedulerModel::Private::testAdded(const ScheduleDefinition &test, int position)
{
    q->beginInsertRows(QModelIndex(), position, position);
    tests.insert(position, test);
    q->endInsertRows();
}

void SchedulerModel::Private::testRemoved(const ScheduleDefinition &test, int position)
{
    Q_UNUSED(test);

    q->beginRemoveRows(QModelIndex(), position, position);
    tests.removeAt(position);
    q->endRemoveRows();
}

void SchedulerModel::Private::testMoved(const ScheduleDefinition &test, int from, int to)
{
    Q_UNUSED(test);

    q->beginMoveRows(QModelIndex(), from, from,
                     QModelIndex(), to + 1);
    tests.move(from, to);
    q->endMoveRows();
}

void SchedulerModel::Private::onTimeout()
{
    QModelIndex topLeft = q->index(0,0);
    QModelIndex bottomRight = q->index(q->rowCount(QModelIndex()), q->columnCount(QModelIndex()));

    emit q->dataChanged(topLeft, bottomRight);
}

SchedulerModel::SchedulerModel(QObject *parent)
: QAbstractTableModel(parent)
, d(new Private(this))
{
    setUpdateInterval(1000);
}

SchedulerModel::~SchedulerModel()
{
    delete d;
}

void SchedulerModel::setScheduler(Scheduler *scheduler)
{
    if (d->scheduler == scheduler)
    {
        return;
    }

    if (d->scheduler)
    {
        disconnect(d->scheduler.data(), SIGNAL(testAdded(ScheduleDefinition, int)), d, SLOT(testAdded(ScheduleDefinition, int)));
        disconnect(d->scheduler.data(), SIGNAL(testRemoved(ScheduleDefinition, int)), d, SLOT(testRemoved(ScheduleDefinition,
                                                                                                         int)));
        disconnect(d->scheduler.data(), SIGNAL(testMoved(ScheduleDefinition, int, int)), d, SLOT(testMoved(ScheduleDefinition,
                                                                                                          int, int)));
    }

    d->scheduler = scheduler;

    if (d->scheduler)
    {
        connect(d->scheduler.data(), SIGNAL(testAdded(ScheduleDefinition, int)), d, SLOT(testAdded(ScheduleDefinition, int)));
        connect(d->scheduler.data(), SIGNAL(testRemoved(ScheduleDefinition, int)), d, SLOT(testRemoved(ScheduleDefinition, int)));
        connect(d->scheduler.data(), SIGNAL(testMoved(ScheduleDefinition, int, int)), d, SLOT(testMoved(ScheduleDefinition, int,
                                                                                                       int)));
    }

    emit schedulerChanged();
    reset();
}

Scheduler *SchedulerModel::scheduler() const
{
    return d->scheduler;
}

void SchedulerModel::setUpdateInterval(int ms)
{
    if (ms == 0)
    {
        d->updateTimer.stop();
    }
    else
    {
        d->updateTimer.setInterval(ms);
        d->updateTimer.start();
    }
}

int SchedulerModel::updateInterval() const
{
    if (d->updateTimer.isActive())
    {
        return d->updateTimer.interval();
    }

    return 0;
}

void SchedulerModel::reset()
{
    beginResetModel();

    if (!d->scheduler.isNull())
    {
        d->tests = d->scheduler->tests();
    }

    endResetModel();
}

QVariant SchedulerModel::get(int index) const
{
    QModelIndex idx = this->index(index, 0);

    if (!idx.isValid())
    {
        return QVariant();
    }

    QVariantMap hash;
    QHashIterator<int, QByteArray> iter(roleNames());

    while (iter.hasNext())
    {
        iter.next();
        hash.insert(iter.value(), data(idx, iter.key()));
    }

    return hash;
}

QHash<int, QByteArray> SchedulerModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(NameRole, "name");
    roleNames.insert(NextRunRole, "nextRun");
    roleNames.insert(TimeLeftRole, "timeLeft");
    roleNames.insert(TypeRole, "type");
    roleNames.insert(IdRole, "id");
    return roleNames;
}

int SchedulerModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return d->tests.size();
}

int SchedulerModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 5;
}

QVariant SchedulerModel::data(const QModelIndex &index, int role) const
{
    const ScheduleDefinition &testDefinition = d->tests.at(index.row());

    if (role == Qt::DisplayRole)
    {
        role = NameRole + index.column();
    }

    switch (role)
    {
    case NameRole:
        return testDefinition.name();

    case NextRunRole:
        return testDefinition.timing()->nextRun();

    case TimeLeftRole:
        return testDefinition.timing()->timeLeft();

    case TypeRole:
        return testDefinition.timing()->type();

    case IdRole:
        return testDefinition.id().toInt();
    }

    return QVariant();
}

QVariant SchedulerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
    {
        return QVariant();
    }

    section += NameRole;

    switch (section)
    {
    case NameRole:
        return tr("Name");

    case NextRunRole:
        return tr("Next Run");

    case TimeLeftRole:
        return tr("Time left");

    case TypeRole:
        return tr("Type");

    case IdRole:
        return tr("Id");

    default:
        return QVariant();
    }
}

#include "schedulermodel.moc"
