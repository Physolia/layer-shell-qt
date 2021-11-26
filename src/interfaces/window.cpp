/*
 *   SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "window.h"
#include "../qwaylandlayersurface_p.h"
#include "../qwaylandlayershellintegration_p.h"
#include <layershellqt_logging.h>
#include <private/qwaylandshellsurface_p.h>
#include <private/qwaylandwindow_p.h>

using namespace LayerShellQt;

static const char *s_interfaceKey = "__kde_layer_window";

class LayerShellQt::WindowPrivate
{
public:
    WindowPrivate(QWindow *window)
        : parentWindow(window)
    {
    }

    QWindow *parentWindow;
    QString scope = QStringLiteral("window");
    Window::Anchors anchors = {Window::AnchorTop | Window::AnchorBottom | Window::AnchorLeft | Window::AnchorRight};
    int32_t exclusionZone = 0;
    Window::KeyboardInteractivity keyboardInteractivity = Window::KeyboardInteractivityExclusive;
    Window::Layer layer = Window::LayerTop;
    QMargins margins;
    QWaylandLayerSurface *getSurface() const;
};

Window::~Window()
{
    d->parentWindow->setProperty(s_interfaceKey, QVariant());
}

void Window::setAnchors(Anchors anchors)
{
    d->anchors = anchors;
    if (auto surface = d->getSurface()) {
        surface->setAnchor(anchors);
    }
}

Window::Anchors Window::anchors() const
{
    return d->anchors;
}

void Window::setExclusiveZone(int32_t zone)
{
    d->exclusionZone = zone;
    if (auto surface = d->getSurface()) {
        surface->setExclusiveZone(zone);
    }
}

int32_t Window::exclusionZone() const
{
    return d->exclusionZone;
}

void Window::setMargins(const QMargins &margins)
{
    d->margins = margins;
    if (auto surface = d->getSurface()) {
        surface->setMargins(margins);
    }
}

QMargins Window::margins() const
{
    return d->margins;
}

void Window::setKeyboardInteractivity(KeyboardInteractivity interactivity)
{
    d->keyboardInteractivity = interactivity;
    if (auto surface = d->getSurface()) {
        surface->setKeyboardInteractivity(interactivity);
    }
}

Window::KeyboardInteractivity Window::keyboardInteractivity() const
{
    return d->keyboardInteractivity;
}

void Window::setLayer(Layer layer)
{
    d->layer = layer;
    if (auto surface = d->getSurface()) {
        surface->setLayer(layer);
    }
}

void Window::setScope(const QString &scope)
{
    d->scope = scope;
    // this is static and must be set before the platform window is created
}

QString Window::scope() const
{
    return d->scope;
}

Window::Layer Window::layer() const
{
    return d->layer;
}

static QWaylandLayerShellIntegration *s_integration = nullptr;

Window::Window(QWindow *window)
    : QObject(window)
    , d(new WindowPrivate(window))
{
    Q_ASSERT(!Window::get(window));
    window->winId(); // create platform window

    QtWaylandClient::QWaylandWindow *waylandWindow = dynamic_cast<QtWaylandClient::QWaylandWindow *>(window->handle());
    if (waylandWindow) {
        if (!s_integration) {
            s_integration = new QWaylandLayerShellIntegration();
        }
        waylandWindow->setShellIntegration(s_integration);
        window->setProperty(s_interfaceKey, QVariant::fromValue<QObject *>(this));
    }
}

Window *Window::get(QWindow *window)
{
    return qobject_cast<Window *>(qvariant_cast<QObject *>(window->property(s_interfaceKey)));
}

QWaylandLayerSurface *WindowPrivate::getSurface() const
{
    if (!parentWindow) {
        return nullptr;
    }
    auto ww = dynamic_cast<QtWaylandClient::QWaylandWindow *>(parentWindow->handle());
    if (!ww) {
        qCDebug(LAYERSHELLQT) << "window not a wayland window" << parentWindow;
        return nullptr;
    }
    QWaylandLayerSurface *s = qobject_cast<QWaylandLayerSurface *>(ww->shellSurface());
    if (!s) {
        qCDebug(LAYERSHELLQT) << "window not using wlr-layer-shell" << parentWindow << ww->shellSurface();
        return nullptr;
    }
    return s;
}
