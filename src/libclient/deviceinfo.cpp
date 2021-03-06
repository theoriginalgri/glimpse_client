#include "deviceinfo.h"
#include "log/logger.h"
#include "client.h"
#include "network/networkmanager.h"

#include <QCryptographicHash>
#include <QProcess>
#include <QTextStream>
#include <Windows.h>
#include <qbatteryinfo.h>
#include <qnetworkinfo.h>
#include <qdeviceinfo.h>
#include <qstorageinfo.h>

LOGGER(DeviceInfo);

static QStringList execWmicCommand(const QStringList &parameters, bool *ok = NULL)
{
    QStringList values;

    QProcess process;
    QTextStream stream(&process);
    process.start("wmic", parameters, QIODevice::ReadOnly);
    process.waitForFinished();

    /* Example output:
     *
     * UUID
     * 00000000-0000-0000-0000-D43D7EEA6FD8
     */

    if (process.exitCode() == 0)
    {
        int lineNumber = 0;
        QString line;

        do
        {
            // We need trimmed(), windows has '\r' at the beginning of some lines
            line = stream.readLine().trimmed();

            // Ignore the first line
            if (lineNumber++ == 0)
            {
                continue;
            }

            if (!line.isEmpty())
            {
                values.append(line);
            }
        }
        while (!line.isNull());
    }

    if (ok)
    {
        *ok = (process.exitCode() == 0);
    }

    return values;
}

static QByteArray cryptSome()
{
    DATA_BLOB outData;
    DATA_BLOB inData;
    inData.pbData = (BYTE *)"Glimpse crypted some random values on windows";
    inData.cbData = (DWORD)strlen((const char *)inData.pbData) + 1;

    if (::CryptProtectData(&inData, NULL, NULL, NULL, NULL, CRYPTPROTECT_LOCAL_MACHINE, &outData))
    {
        QByteArray data((const char *)outData.pbData, outData.cbData);
        ::LocalFree(outData.pbData);
        return data;
    }
    else
    {
        LOG_ERROR(QString("CryptProtectedData failed. LastError=%1").arg(::GetLastError()));
        return QByteArray();
    }
}

DeviceInfo::DeviceInfo()
: d(NULL)
{
}

DeviceInfo::~DeviceInfo()
{
}

QString DeviceInfo::deviceId() const
{
    DWORD serialNumber = 0;

    // Is it clever to check C:? Otherwise we may read a id of an usb stick
    if (::GetVolumeInformationW(L"C:\\", NULL, 0, &serialNumber, NULL, NULL, NULL, 0) == FALSE)
    {
        LOG_DEBUG("Unable to get partition serial number");
        return QString();
    }

    QStringList productUuid = execWmicCommand(QStringList() << "csproduct" << "get" << "UUID");
    QStringList cpu = execWmicCommand(QStringList() << "cpu" << "get" << "name");
    QStringList bios = execWmicCommand(QStringList() << "bios" << "get" << "manufacturer");

    QCryptographicHash hash(QCryptographicHash::Sha224);
    hash.addData(QString::number(serialNumber).toUtf8());
    hash.addData(productUuid.join(';').toUtf8());
    hash.addData(cpu.join(';').toUtf8());
    hash.addData(bios.join(';').toUtf8());
    //hash.addData(cryptSome());

    return QString::fromLatin1(hash.result().toHex());
}

qreal DeviceInfo::cpuUsage() const
{
    return -1.0;
}

quint32 DeviceInfo::freeMemory() const
{
    return 0;
}

qint32 DeviceInfo::signalStrength() const
{
    return QNetworkInfo().networkSignalStrength(Client::instance()->networkManager()->connectionMode(), 0);
}

qint8 DeviceInfo::batteryLevel() const
{
    return QBatteryInfo().level();
}

QString DeviceInfo::platform() const
{
    return "windows";
}

QString DeviceInfo::OSName() const
{
    return QDeviceInfo().operatingSystemName();
}

QString DeviceInfo::OSVersion() const
{
    return QDeviceInfo().version(QDeviceInfo::Os);
}

QString DeviceInfo::firmwareVersion() const
{
    return QDeviceInfo().version(QDeviceInfo::Firmware);
}

QString DeviceInfo::board() const
{
    return QDeviceInfo().boardName();
}

QString DeviceInfo::manufacturer() const
{
    return QDeviceInfo().manufacturer();
}

QString DeviceInfo::model() const
{
    return QDeviceInfo().model();
}

qlonglong DeviceInfo::availableDiskSpace() const
{
    QStorageInfo info;
    qlonglong diskSpace = 0;
    QStringList drives = info.allLogicalDrives();

    drives.removeDuplicates();

    foreach (const QString &drive, drives)
    {
        if (info.driveType(drive) == QStorageInfo::InternalDrive)
        {
            diskSpace += info.availableDiskSpace(drive);
        }
    }

    return diskSpace;
}
