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

#include "CinnamonWindowObserverBackend.hpp"
#include "Cinnamon.hpp"
#include "DBus.hpp"
#include <QDir>
#include <QFile>
#include <QThread>
#include <QElapsedTimer>
#include <iostream>

#include "cinnamon_extension.c"
#include "cinnamon_metadata.c"

namespace Platform {

const QString CinnamonWindowObserverBackend::m_extensionUUID = "shijima-helper@pixelomer.github.io";

// Writes a file only if its contents differ. Returns true if it was written.
static bool writeIfChanged(QString const& path, QByteArray const& data) {
    QFile file { path };
    if (file.open(QFile::ReadOnly) && file.readAll() == data) {
        return false;
    }
    file.close();
    if (!file.open(QFile::WriteOnly | QFile::Truncate) ||
        file.write(data) != data.size())
    {
        throw std::runtime_error("failed to write " + path.toStdString());
    }
    return true;
}

bool CinnamonWindowObserverBackend::installExtension() {
    QDir dir { Cinnamon::extensionsDir() + "/" + m_extensionUUID };
    if (!dir.mkpath(".")) {
        throw std::runtime_error("failed to create " + dir.path().toStdString());
    }
    bool changed = writeIfChanged(dir.filePath("extension.js"),
        QByteArray(cinnamon_extension, cinnamon_extension_len));
    changed = writeIfChanged(dir.filePath("metadata.json"),
        QByteArray(cinnamon_metadata, cinnamon_metadata_len)) || changed;
    return changed;
}

CinnamonWindowObserverBackend::CinnamonWindowObserverBackend():
    WindowObserverBackend()
{
    bool changed = installExtension();
    bool wasRunning = alive();
    auto enabled = Cinnamon::enabledExtensions();
    if (!enabled.contains(m_extensionUUID)) {
        enabled.append(m_extensionUUID);
        Cinnamon::setEnabledExtensions(enabled);
    }
    else if (changed || !wasRunning) {
        // Left enabled by a crash: an older copy may still be loaded, or a
        // copy may have failed to load. ReloadXlet also loads it if needed.
        Cinnamon::reloadExtension(m_extensionUUID);
    }
    else {
        return;
    }
    // Cinnamon loads the extension asynchronously. Wait for it so the first
    // alive() check does not fail.
    QElapsedTimer timer;
    timer.start();
    while (!alive()) {
        if (timer.elapsed() > 5000) {
            throw std::runtime_error("Cinnamon did not load the Shijima "
                "helper extension (" + m_extensionUUID.toStdString() + "). "
                "Check ~/.xsession-errors or Looking Glass for errors.");
        }
        QThread::msleep(50);
    }
}

CinnamonWindowObserverBackend::~CinnamonWindowObserverBackend() {
    try {
        auto enabled = Cinnamon::enabledExtensions();
        if (enabled.removeAll(m_extensionUUID) > 0) {
            Cinnamon::setEnabledExtensions(enabled);
        }
    }
    catch (std::exception &ex) {
        std::cerr << "Could not disable the Shijima Cinnamon helper: "
            << ex.what() << std::endl;
    }
}

bool CinnamonWindowObserverBackend::alive() {
    try {
        return Cinnamon::runningExtensions().contains(m_extensionUUID);
    }
    catch (DBus::DBusCallError &err) {
        // Cinnamon is restarting. It reloads enabled extensions by itself.
        return true;
    }
}

}
