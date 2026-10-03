#pragma once

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

#include "WindowObserverBackend.hpp"
#include <QString>

namespace Platform {

// Installs a small Cinnamon extension that reports the focused window over
// DBus, the same way the GNOME backend does. Cinnamon loads and unloads
// extensions live when org.cinnamon enabled-extensions changes, so unlike
// GNOME no re-login is needed.
class CinnamonWindowObserverBackend : public WindowObserverBackend {
private:
    static const QString m_extensionUUID;
    bool installExtension();
public:
    explicit CinnamonWindowObserverBackend();
    ~CinnamonWindowObserverBackend();
    bool alive() override;
};

}
