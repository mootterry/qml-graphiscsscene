#ifndef DECLARATIVESCENE_H
#define DECLARATIVESCENE_H

#include <QtCore/QtGlobal>
#include <QtQuick/QQuickItem>
#include <QtWidgets/QGraphicsScene>

#include <QtCharts/QChart>
#include <QtCore/QLocale>
#include <QQmlComponent>


class DeclarativeScene : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QObject* scene READ scene )

public:
    DeclarativeScene(QQuickItem *parent = 0);
    ~DeclarativeScene();

public: // From parent classes
    Q_INVOKABLE QGraphicsScene *scene() const;
    Q_INVOKABLE void setScene(QGraphicsScene *scene);

    void componentComplete();
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);


protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void hoverMoveEvent(QHoverEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event);
private Q_SLOTS:
    void handleAntialiasingChanged(bool enable);
    void sceneChanged(QList<QRectF> region);
    void renderScene();

Q_SIGNALS:
    void backgroundColorChanged();
    void dropShadowEnabledChanged(bool enabled);
    void needRender();
    void pendingRenderNodeMouseEventResponses();


private:
    void initScene();
    void queueRendererMouseEvent(QMouseEvent *event);

    QGraphicsScene *m_scene;
    QPointF m_mousePressScenePoint;
    QPoint m_mousePressScreenPoint;
    QPointF m_lastMouseMoveScenePoint;
    QPoint m_lastMouseMoveScreenPoint;
    Qt::MouseButton m_mousePressButton;
    Qt::MouseButtons m_mousePressButtons;
    QImage *m_sceneImage;
    bool m_sceneImageDirty;
    bool m_updatePending;
    Qt::HANDLE m_paintThreadId;
    Qt::HANDLE m_guiThreadId;
    bool m_sceneImageNeedsClear;
    QVector<QMouseEvent *> m_pendingRenderNodeMouseEvents;
    QRectF m_adjustedPlotArea;
};

#endif // DECLARATIVESCENE_H
