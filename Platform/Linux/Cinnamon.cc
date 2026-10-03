// 
// Shijima-Qt - Cross-platform shimeji simulation app for desktop
// Copyright (C) 2025 pixelomer
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include "Cinnamon.hpp"
#include "DBus.hpp"
#include <QDBusMessage>
#include <QDir>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>

using namespace Platform::DBus;

namespace Platform {
namespace Cinnamon {

static QString gsettings(QStringList const& args) {
    QProcess process;
    process.setProcessChannelMode(QProcess::ProcessChannelMode::ForwardedErrorChannel);
    process.start("gsettings", args);
    process.waitForFinished(-1);
    int exitCode = process.exitCode();
    if (process.exitStatus() != QProcess::NormalExit || exitCode != 0) {
        throw std::runtime_error(std::string("gsettings ") +
            args.join(' ').toStdString() + " failed (" +
            std::to_string(exitCode) + ")");
    }
    return QString::fromUtf8(process.readAllStandardOutput());
}

QStringList runningExtensions() {
    auto msg = QDBusMessage::createMethodCall("org.Cinnamon",
        "/org/Cinnamon", "org.Cinnamon", "GetRunningXletUUIDs");
    msg << QString("extension");
    auto res = callDBus(msg, "as");
    return res[0].toStringList();
}

void reloadExtension(QString const& uuid) {
    auto msg = QDBusMessage::createMethodCall("org.Cinnamon",
        "/org/Cinnamon", "org.Cinnamon", "ReloadXlet");
    msg << uuid << QString("EXTENSION");
    callDBus(msg);
}

QStringList enabledExtensions() {
    // Output is a GVariant string array, e.g. "@as []" or "['a', 'b']".
    // Extension UUIDs never contain quotes.
    auto output = gsettings({ "get", "org.cinnamon", "enabled-extensions" });
    static const QRegularExpression item { "'([^']*)'" };
    QStringList uuids;
    for (auto const& match : item.globalMatch(output)) {
        uuids.append(match.captured(1));
    }
    return uuids;
}

void setEnabledExtensions(QStringList const& uuids) {
    QString value = uuids.isEmpty() ? "@as []" :
        "['" + uuids.join("', '") + "']";
    gsettings({ "set", "org.cinnamon", "enabled-extensions", value });
}

QString extensionsDir() {
    auto dataHome = QProcessEnvironment::systemEnvironment()
        .value("XDG_DATA_HOME");
    if (dataHome.isEmpty()) {
        dataHome = QDir::homePath() + "/.local/share";
    }
    return dataHome + "/cinnamon/extensions";
}

bool running() {
    try {
        runningExtensions();
        return true;
    }
    catch (DBusCallError &err) {
        return false;
    }
}

}
}
