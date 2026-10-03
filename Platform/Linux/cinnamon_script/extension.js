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

// Cinnamon port of the GNOME helper. Cinnamon extensions use the legacy
// init/enable/disable module format and Muffin, whose MetaWindow API matches
// Mutter's for everything used here.

const Meta = imports.gi.Meta;
const Gio = imports.gi.Gio;
const GLib = imports.gi.GLib;

const kServiceName = 'com.pixelomer.ShijimaQt';
const kWindowSignals = [ 'position-changed', 'size-changed', 'unmanaged',
    'notify::minimized', 'workspace-changed' ];

let helper = null;

class ShijimaHelper {
    constructor() {
        this._window = null;
        this._windowSignals = [];
        this._lastMessage = null;
        this._displaySignal = global.display.connect('notify::focus-window',
            () => this._setWindow(global.display.focus_window));
        this._workspaceSignal = global.workspace_manager.connect(
            'active-workspace-changed', () => this._notify());
        // Shijima-Qt may start (or restart) after the helper. Resend the
        // current state whenever its bus name appears.
        this._nameWatch = Gio.bus_watch_name(Gio.BusType.SESSION, kServiceName,
            Gio.BusNameWatcherFlags.NONE,
            () => { this._lastMessage = null; this._notify(); },
            () => { this._lastMessage = null; });
        this._setWindow(global.display.focus_window);
    }

    _isValidWindow(window) {
        return window != null && window.get_window_type() === Meta.WindowType.NORMAL;
    }

    _setWindow(window) {
        if (!this._isValidWindow(window)) {
            window = null;
        }
        if (window !== this._window) {
            this._disconnectWindow();
            this._window = window;
            if (window != null) {
                const notify = () => this._notify();
                for (const name of kWindowSignals) {
                    this._windowSignals.push(window.connect(name, name === 'unmanaged' ?
                        () => this._setWindow(null) : notify));
                }
            }
        }
        this._notify();
    }

    _disconnectWindow() {
        if (this._window != null) {
            for (const id of this._windowSignals) {
                this._window.disconnect(id);
            }
        }
        this._windowSignals = [];
        this._window = null;
    }

    _getMessage() {
        const window = this._window;
        if (window == null || window.minimized || window.is_hidden() ||
            (!window.is_on_all_workspaces() && window.get_workspace() !==
                global.workspace_manager.get_active_workspace()))
        {
            return [ "", -1, -1, -1, -1, -1 ];
        }
        const rect = window.get_frame_rect();
        return [ window.get_id().toString(), window.get_pid(),
            rect.x, rect.y, rect.width, rect.height ];
    }

    _notify() {
        const message = this._getMessage();
        const key = message.join(',');
        if (key === this._lastMessage) {
            return;
        }
        this._lastMessage = key;
        Gio.DBus.session.call(kServiceName, '/', kServiceName,
            'updateActiveWindow', new GLib.Variant('(sidddd)', message),
            null, Gio.DBusCallFlags.NO_AUTO_START, -1, null,
            (connection, result) => {
                try {
                    connection.call_finish(result);
                }
                catch (err) {
                    // Shijima-Qt is not running; the name watch resends later.
                }
            });
    }

    destroy() {
        this._disconnectWindow();
        global.display.disconnect(this._displaySignal);
        global.workspace_manager.disconnect(this._workspaceSignal);
        Gio.bus_unwatch_name(this._nameWatch);
    }
}

function init(metadata) {}

function enable() {
    helper = new ShijimaHelper();
}

function disable() {
    if (helper != null) {
        helper.destroy();
        helper = null;
    }
}
