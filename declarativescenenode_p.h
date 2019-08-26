#ifndef DECLARATIVESCENENODE_P_H
#define DECLARATIVESCENENODE_P_H

#include <QtCharts/QChartGlobal>
#include <QtQuick/QSGNode>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGImageNode>

class DeclarativeAbstractRenderNode;
class DeclarativeSceneNode : public QSGRootNode
{
public:
    DeclarativeSceneNode(QQuickWindow *window);
    ~DeclarativeSceneNode();

    void createTextureFromImage(const QImage &chartImage);
    DeclarativeAbstractRenderNode *renderNode() const { return m_renderNode; }

    void setRect(const QRectF &rect);

private:
    QRectF m_rect;
    QQuickWindow *m_window;
    DeclarativeAbstractRenderNode *m_renderNode;
    QSGImageNode *m_imageNode;
};

#endif // DECLARATIVESCENENODE_P_H
