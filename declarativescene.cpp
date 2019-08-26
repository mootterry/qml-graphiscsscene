#include "declarativescene.h"
#include "declarativescenenode_p.h"
#include <QtGui/QPainter>

#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QGraphicsSceneHoverEvent>
#include <QtWidgets/QApplication>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtQuick/QQuickWindow>
#include <QDebug>
#include <QElapsedTimer>

DeclarativeScene::DeclarativeScene(QQuickItem *parent)
    : QQuickItem(parent)
{
    initScene();
}

// QTBUG-71013
// The symbol resides in qbarmodelmapper.cpp#548 in the C++ module.
// Here, it gets imported and reset to the DeclarativeBarSet allocator
void DeclarativeScene::initScene()
{
    m_sceneImage = 0;
    m_sceneImageDirty = false;
    m_sceneImageNeedsClear = false;
    m_guiThreadId = QThread::currentThreadId();
    m_paintThreadId = 0;
    m_updatePending = false;

    setFlag(ItemHasContents, true);

    m_scene = new QGraphicsScene(this);

    setAntialiasing(QQuickItem::antialiasing());
    connect(m_scene, &QGraphicsScene::changed, this, &DeclarativeScene::sceneChanged);
    connect(this, &DeclarativeScene::needRender, this, &DeclarativeScene::renderScene/*,Qt::QueuedConnection*/);
    connect(this, SIGNAL(antialiasingChanged(bool)), this, SLOT(handleAntialiasingChanged(bool)));

    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
}

DeclarativeScene::~DeclarativeScene()
{
    delete m_sceneImage;
}

void DeclarativeScene::componentComplete()
{
    QQuickItem::componentComplete();
}

void DeclarativeScene::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
   QQuickItem::geometryChanged(newGeometry, oldGeometry);
   m_scene->setSceneRect(0,0,newGeometry.width(),newGeometry.height());
}

QSGNode *DeclarativeScene::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
{
    DeclarativeSceneNode *node = static_cast<DeclarativeSceneNode *>(oldNode);

    if (!node) {
        node =  new DeclarativeSceneNode(window());
        // Ensure that chart is rendered whenever node is recreated
        if (m_sceneImage)
            m_sceneImageDirty = true;
    }

    const QRectF &bRect = boundingRect();
    // Update renderNode data

    m_pendingRenderNodeMouseEvents.clear();
    //qDebug("render updating %s",m_sceneImageDirty ? "true" : "flase");

    // Copy chart (if dirty) to chart node
    if (m_sceneImageDirty) {
        node->createTextureFromImage(*m_sceneImage);
        m_sceneImageDirty = false;
    }

    node->setRect(bRect);

    return node;
}

void DeclarativeScene::sceneChanged(QList<QRectF> region)
{
    const int count = region.size();
    const qreal limitSize = 0.01;
    if (count && !m_updatePending) {
        qreal totalSize = 0.0;
        for (int i = 0; i < count; i++) {
            const QRectF &reg = region.at(i);
            totalSize += (reg.height() * reg.width());
            if (totalSize >= limitSize)
                break;
        }
        // Ignore region updates that change less than small fraction of a pixel, as there is
        // little point regenerating the image in these cases. These are typically cases
        // where OpenGL series are drawn to otherwise static chart.

        if (totalSize >= limitSize) {
            m_updatePending = true;
            // Do async render to avoid some unnecessary renders.
            emit needRender();
        } else {
            // We do want to call update to trigger possible gl series updates.
            update();
        }
    }
}

void DeclarativeScene::renderScene()
{
    m_updatePending = false;
    QElapsedTimer timer;
    timer.start();

    QSize chartSize(width(),height());

    if (!m_sceneImage || chartSize != m_sceneImage->size()) {
        delete m_sceneImage;
        qreal dpr = window() ? window()->devicePixelRatio() : 1.0;
        qDebug()<<chartSize;
        m_sceneImage = new QImage(chartSize * dpr, QImage::Format_ARGB32);
        m_sceneImage->setDevicePixelRatio(dpr);
    }

    m_sceneImageNeedsClear = true;
    if (m_sceneImageNeedsClear) {
        m_sceneImage->fill(Qt::transparent);
        m_sceneImageNeedsClear = false;
    }

    QPainter painter(m_sceneImage);
    if (antialiasing()) {
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing
                               | QPainter::SmoothPixmapTransform);
    }

    QRect renderRect(QPoint(0, 0), chartSize);
    m_scene->render(&painter, renderRect, renderRect);
    //qDebug("render %lld",timer.restart());
    m_sceneImageDirty = true;
    update();
}

void DeclarativeScene::mousePressEvent(QMouseEvent *event)
{
    m_mousePressScenePoint = event->pos();
    m_mousePressScreenPoint = event->globalPos();
    m_lastMouseMoveScenePoint = m_mousePressScenePoint;
    m_lastMouseMoveScreenPoint = m_mousePressScreenPoint;
    m_mousePressButton = event->button();
    m_mousePressButtons = event->buttons();

    QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMousePress);
    mouseEvent.setWidget(0);
    mouseEvent.setButtonDownScenePos(m_mousePressButton, m_mousePressScenePoint);
    mouseEvent.setButtonDownScreenPos(m_mousePressButton, m_mousePressScreenPoint);
    mouseEvent.setScenePos(m_mousePressScenePoint);
    mouseEvent.setScreenPos(m_mousePressScreenPoint);
    mouseEvent.setLastScenePos(m_lastMouseMoveScenePoint);
    mouseEvent.setLastScreenPos(m_lastMouseMoveScreenPoint);
    mouseEvent.setButtons(m_mousePressButtons);
    mouseEvent.setButton(m_mousePressButton);
    mouseEvent.setModifiers(event->modifiers());
    mouseEvent.setAccepted(false);

    QApplication::sendEvent(m_scene, &mouseEvent);

    queueRendererMouseEvent(event);
}

void DeclarativeScene::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseRelease);
    mouseEvent.setWidget(0);
    mouseEvent.setButtonDownScenePos(m_mousePressButton, m_mousePressScenePoint);
    mouseEvent.setButtonDownScreenPos(m_mousePressButton, m_mousePressScreenPoint);
    mouseEvent.setScenePos(event->pos());
    mouseEvent.setScreenPos(event->globalPos());
    mouseEvent.setLastScenePos(m_lastMouseMoveScenePoint);
    mouseEvent.setLastScreenPos(m_lastMouseMoveScreenPoint);
    mouseEvent.setButtons(event->buttons());
    mouseEvent.setButton(event->button());
    mouseEvent.setModifiers(event->modifiers());
    mouseEvent.setAccepted(false);

    QApplication::sendEvent(m_scene, &mouseEvent);

    m_mousePressButtons = event->buttons();
    m_mousePressButton = Qt::NoButton;

    queueRendererMouseEvent(event);
}

void DeclarativeScene::hoverMoveEvent(QHoverEvent *event)
{
    QPointF previousLastScenePoint = m_lastMouseMoveScenePoint;

    //qDebug() << "mouse move"<<event->pos();
    // Convert hover move to mouse move, since we don't seem to get actual mouse move events.
    // QGraphicsScene generates hover events from mouse move events, so we don't need
    // to pass hover events there.
    QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseMove);
    mouseEvent.setWidget(0);
    mouseEvent.setButtonDownScenePos(m_mousePressButton, m_mousePressScenePoint);
    mouseEvent.setButtonDownScreenPos(m_mousePressButton, m_mousePressScreenPoint);
    mouseEvent.setScenePos(event->pos());
    // Hover events do not have global pos in them, and the screen position doesn't seem to
    // matter anyway in this use case, so just pass event pos instead of trying to
    // calculate the real screen position.
    mouseEvent.setScreenPos(event->pos());
    mouseEvent.setLastScenePos(m_lastMouseMoveScenePoint);
    mouseEvent.setLastScreenPos(m_lastMouseMoveScreenPoint);
    mouseEvent.setButtons(m_mousePressButtons);
    mouseEvent.setButton(m_mousePressButton);
    mouseEvent.setModifiers(event->modifiers());
    m_lastMouseMoveScenePoint = mouseEvent.scenePos();
    m_lastMouseMoveScreenPoint = mouseEvent.screenPos();
    mouseEvent.setAccepted(false);

    QApplication::sendEvent(m_scene, &mouseEvent);
}

void DeclarativeScene::mouseMoveEvent(QMouseEvent *event)
{
    QPointF previousLastScenePoint = m_lastMouseMoveScenePoint;

    if(m_sceneImageDirty)
        return;
    //qDebug() << "mouse move"<<event->pos();
    // Convert hover move to mouse move, since we don't seem to get actual mouse move events.
    // QGraphicsScene generates hover events from mouse move events, so we don't need
    // to pass hover events there.
    QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseMove);
    mouseEvent.setWidget(0);
    mouseEvent.setButtonDownScenePos(m_mousePressButton, m_mousePressScenePoint);
    mouseEvent.setButtonDownScreenPos(m_mousePressButton, m_mousePressScreenPoint);
    mouseEvent.setScenePos(event->pos());
    // Hover events do not have global pos in them, and the screen position doesn't seem to
    // matter anyway in this use case, so just pass event pos instead of trying to
    // calculate the real screen position.
    mouseEvent.setScreenPos(event->pos());
    mouseEvent.setLastScenePos(m_lastMouseMoveScenePoint);
    mouseEvent.setLastScreenPos(m_lastMouseMoveScreenPoint);
    mouseEvent.setButtons(m_mousePressButtons);
    mouseEvent.setButton(m_mousePressButton);
    mouseEvent.setModifiers(event->modifiers());
    m_lastMouseMoveScenePoint = mouseEvent.scenePos();
    m_lastMouseMoveScreenPoint = mouseEvent.screenPos();
    mouseEvent.setAccepted(false);

    QApplication::sendEvent(m_scene, &mouseEvent);
}

void DeclarativeScene::mouseDoubleClickEvent(QMouseEvent *event)
{
    m_mousePressScenePoint = event->pos();
    m_mousePressScreenPoint = event->globalPos();
    m_lastMouseMoveScenePoint = m_mousePressScenePoint;
    m_lastMouseMoveScreenPoint = m_mousePressScreenPoint;
    m_mousePressButton = event->button();
    m_mousePressButtons = event->buttons();

    QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseDoubleClick);
    mouseEvent.setWidget(0);
    mouseEvent.setButtonDownScenePos(m_mousePressButton, m_mousePressScenePoint);
    mouseEvent.setButtonDownScreenPos(m_mousePressButton, m_mousePressScreenPoint);
    mouseEvent.setScenePos(m_mousePressScenePoint);
    mouseEvent.setScreenPos(m_mousePressScreenPoint);
    mouseEvent.setLastScenePos(m_lastMouseMoveScenePoint);
    mouseEvent.setLastScreenPos(m_lastMouseMoveScreenPoint);
    mouseEvent.setButtons(m_mousePressButtons);
    mouseEvent.setButton(m_mousePressButton);
    mouseEvent.setModifiers(event->modifiers());
    mouseEvent.setAccepted(false);

    QApplication::sendEvent(m_scene, &mouseEvent);

    queueRendererMouseEvent(event);
}

void DeclarativeScene::handleAntialiasingChanged(bool enable)
{
    setAntialiasing(enable);
    emit needRender();
}

void DeclarativeScene::queueRendererMouseEvent(QMouseEvent *event)
{
//                update();
}

QGraphicsScene *DeclarativeScene::scene() const
{
    return m_scene;
}

void DeclarativeScene::setScene(QGraphicsScene *scene)
{
    m_scene = scene;
}

