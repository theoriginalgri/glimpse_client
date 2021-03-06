#include "configcontroller.h"
#include "../network/networkmanager.h"
#include "../settings.h"
#include "../log/logger.h"

#include "../webrequester.h"
#include "../network/requests/getconfigrequest.h"
#include "../timing/periodictiming.h"
#include "../client.h"

#include <QPointer>
#include <QTimer>

LOGGER(ConfigController);

class ConfigController::Private : public QObject
{
    Q_OBJECT

public:
    Private(ConfigController *q)
    : q(q)
    {
        connect(&timer, SIGNAL(timeout()), q, SLOT(update()));
        connect(&requester, SIGNAL(statusChanged(WebRequester::Status)), q, SIGNAL(statusChanged()));
        connect(&requester, SIGNAL(finished()), this, SLOT(onFinished()));
        connect(&requester, SIGNAL(error()), this, SLOT(onError()));

        request.setPath("/supervisor/api/v1/configuration/1/");
        requester.setRequest(&request);
    }

    ConfigController *q;

    // Properties
    QPointer<NetworkManager> networkManager;
    QPointer<Settings> settings;

    WebRequester requester;
    GetConfigRequest request;
    GetConfigResponse *response;

    QTimer timer;

    // Functions
public slots:
    void updateTimer();
    void onFinished();
    void onError();
};

void ConfigController::Private::updateTimer()
{
    // Set the new url
    QString newUrl = QString("https://%1").arg(Client::instance()->settings()->config()->configAddress());

    if (requester.url() != newUrl)
    {
        LOG_DEBUG(QString("Config url set to %1").arg(newUrl));
        requester.setUrl(newUrl);
    }

    TimingPtr timing = settings->config()->configTiming();

    int period = 0;

    if (timing.isNull())
    {
        period = 60 * 1 * 1000; // backup if no timing is set
    }
    else
    {
        QSharedPointer<PeriodicTiming> periodicTiming = timing.dynamicCast<PeriodicTiming>();
        Q_ASSERT(periodicTiming);

        period = periodicTiming->interval();
    }

    if (timer.interval() != period)
    {
        LOG_DEBUG(QString("Config schedule set to %1 sec.").arg(period / 1000));
        timer.setInterval(period);
    }

    timer.start();
}

void ConfigController::Private::onFinished()
{
    emit q->finished();
}

void ConfigController::Private::onError()
{
    LOG_ERROR(QString("Could not get config from server, trying again later"));
}

ConfigController::ConfigController(QObject *parent)
: Controller(parent)
, d(new Private(this))
{
}

ConfigController::~ConfigController()
{
    delete d;
}

void ConfigController::update()
{
    d->requester.start();
}

bool ConfigController::init(NetworkManager *networkManager, Settings *settings)
{
    d->networkManager = networkManager;
    d->settings = settings;

    // TODO: This is a evil hack
    d->response = settings->config();
    d->requester.setResponse(d->response);
    connect(d->response, SIGNAL(responseChanged()), d, SLOT(updateTimer()));

    d->updateTimer();
    update();

    return true;
}

Controller::Status ConfigController::status() const
{
    return (Status)d->requester.status();
}

QString ConfigController::errorString() const
{
    return d->requester.errorString();
}

#include "configcontroller.moc"
