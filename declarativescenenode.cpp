#include "declarativescenenode_p.h"
#include "declarativeabstractrendernode_p.h"

#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGImageNode>
#include <QtQuick/QSGRendererInterface>


// This node handles displaying of the chart itself
DeclarativeSceneNode::DeclarativeSceneNode(QQuickWindow *window) :
    QSGRootNode(),
    m_window(window),
    m_renderNode(nullptr),
    m_imageNode(nullptr)
{
    // Create a DeclarativeRenderNode for correct QtQuick Backend
}

DeclarativeSceneNode::~DeclarativeSceneNode()
{
}

// Must be called on render thread and in context
void DeclarativeSceneNode::createTextureFromImage(const QImage &chartImage)
{
    static auto const defaultTextureOptions = QQuickWindow::CreateTextureOptions(QQuickWindow::TextureHasAlphaChannel |
                                                                                 QQuickWindow::TextureOwnsGLTexture);

    auto texture = m_window->createTextureFromImage(chartImage, defaultTextureOptions);
    // Create Image node if needed
    if (!m_imageNode) {
        m_imageNode = m_window->createImageNode();
        m_imageNode->setFlag(OwnedByParent);
        m_imageNode->setOwnsTexture(true);
        m_imageNode->setTexture(texture);
        prependChildNode(m_imageNode);
    } else {
        m_imageNode->setTexture(texture);
    }
    if (!m_rect.isEmpty())
        m_imageNode->setRect(m_rect);
}

void DeclarativeSceneNode::setRect(const QRectF &rect)
{
    m_rect = rect;

    if (m_imageNode)
        m_imageNode->setRect(rect);
}
