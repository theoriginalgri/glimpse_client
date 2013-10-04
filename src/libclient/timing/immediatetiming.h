#ifndef IMMEDIATETIMING_H
#define IMMEDIATETIMING_H

#include "timing.h"

class ImmediateTiming : public Timing
{
public:
    ImmediateTiming();
    ~ImmediateTiming();

    // Storage
    static TimingPtr fromVariant(const QVariant &variant);

    // Timing interface
    QString type() const;
    bool reset();
    QDateTime nextRun() const;

    // Serializable interface
    QVariant toVariant() const;

protected:
    class Private;
    Private* d;
};

#endif // IMMEDIATETIMING_H
