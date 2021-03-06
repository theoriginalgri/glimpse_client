#ifndef PACKETTRAINSDEFINITION_H
#define PACKETTRAINSDEFINITION_H

#include <QVariant>
#include <QSharedPointer>
#include <time.h>

#include "../measurementdefinition.h"

class PacketTrainsDefinition;

typedef QSharedPointer<PacketTrainsDefinition> PacketTrainsDefinitionPtr;
typedef QList<PacketTrainsDefinitionPtr> PacketTrainsDefinitionList;

class PacketTrainsDefinition : public MeasurementDefinition
{
public:
    PacketTrainsDefinition(QString host, quint16 port, quint16 packetSize, quint16 trainLength, quint8 iterations,
                           quint64 rateMin, quint64 rateMax, quint64 delay);

    QString host;
    quint16 port;
    quint16 packetSize;
    quint16 trainLength;
    quint8 iterations;
    quint64 rateMin;
    quint64 rateMax;
    quint64 delay;

    QVariant toVariant() const;
    static PacketTrainsDefinitionPtr fromVariant(const QVariant &variant);
};

struct msg
{
    quint16 iter;  // train number
    quint8 id;  // packet number (sender)
    quint8 r_id;  // packet number (receiver)
    qint64 otime;  // originate timestamp
    qint64 rtime;  // received timestamp
};

#endif // PACKETTRAINSDEFINITION_H
