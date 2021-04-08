#include "resizeablerectitem.hpp"
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneMoveEvent>
#include <QPainter>
#include <QTimer>
#include <QPointF>
#include <QCursor>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPixmapCache>
#include "model.hpp"

const quint32 ResizeableRectItem::kMinRectSize = 10;

ResizeableRectItem::ResizeableRectItem(QGraphicsView* pView, MapObject* pMapObject)
      : QGraphicsRectItem( pMapObject->mXPos, pMapObject->mYPos, pMapObject->mWidth, pMapObject->mHeight, nullptr ), mView(pView), mMapObject(pMapObject)
{
    Init();
    setZValue(3.0 + CalcZPos());
}

qreal ResizeableRectItem::CalcZPos() const
{
    // Why isn't area == 1 ?
    QRectF ourRect = rect();

    float area = ((float)ourRect.width() * (float)ourRect.height()) / 4294836225.0f;
    float percentArea = area * 100.0f;

    qreal zpos = 999999.0f - (percentArea*1000.0f);
    // Negative zpos underflows and breaks resize/selection.
    if (zpos < 0)
    {
        zpos = 0;
    }
    return zpos;
}

void ResizeableRectItem::mousePressEvent( QGraphicsSceneMouseEvent* aEvent )
{
    if ( aEvent->button() == Qt::LeftButton )
    {
        m_ResizeMode = getResizeLocation( aEvent->pos(), boundingRect() );
        if ( m_ResizeMode == eResize_None )
        {
            SetViewCursor( Qt::ClosedHandCursor );
        }
    }
    QGraphicsRectItem::mousePressEvent( aEvent );
}

void ResizeableRectItem::mouseMoveEvent( QGraphicsSceneMouseEvent* aEvent )
{
    if ( m_ResizeMode != eResize_None )
    {
        onResize( aEvent->pos() );
        return;
    }
    QGraphicsRectItem::mouseMoveEvent( aEvent );
}

void ResizeableRectItem::mouseReleaseEvent( QGraphicsSceneMouseEvent* aEvent )
{
    if ( aEvent->button() == Qt::LeftButton )
    {
        m_ResizeMode = eResize_None;

        // TODO: Calc new parent if moved


    }
    QGraphicsRectItem::mouseReleaseEvent( aEvent );
}

void ResizeableRectItem::paint( QPainter* aPainter, const QStyleOptionGraphicsItem* aOption, QWidget* aWidget /*= nullptr*/ )
{
    Q_UNUSED( aWidget );

    //aPainter->setClipRect( aOption->exposedRect );

    QRectF cRect = boundingRect();
    /*
    cRect.setX( cRect.x() + 1 );
    cRect.setY( cRect.y() + 1 );
    cRect.setWidth( cRect.width() - 1 );
    cRect.setHeight( cRect.height() - 1 );
*/
    // Draw the rect middle
   // aPainter->fillRect( cRect, Qt::SolidPattern );

    if ( isSelected() )
    {
        aPainter->setPen( QPen ( Qt::red, 2, Qt::DashLine ) );
    }
    else
    {
        // Draw normal rect
        aPainter->setPen( QPen( Qt::black, 2, Qt::SolidLine ) );
    }

    if ( m_Pixmap.isNull() )
    {
        aPainter->setBrush( Qt::lightGray );
    }
    else
    {
       // aPainter->setBrush( QBrush() );
        aPainter->drawPixmap( cRect.x(), cRect.y(), cRect.width(), cRect.height(), m_Pixmap );
    }

    // Draw the rect outline.
    aPainter->drawRect( cRect );
}

void ResizeableRectItem::hoverMoveEvent( QGraphicsSceneHoverEvent* aEvent )
{ 
    //qDebug("Resize mode = %d", m_ResizeMode);
    if ( !( flags() & QGraphicsItem::ItemIsSelectable ) )
    {
        SetViewCursor( Qt::OpenHandCursor );
        return;
    }

    const eResize resizeLocation = getResizeLocation( aEvent->pos(), boundingRect() );
    switch ( resizeLocation )
    {
    case eResize_None:
        SetViewCursor( Qt::OpenHandCursor );
        break;

    case eResize_TopLeftCorner:
        SetViewCursor( Qt::SizeFDiagCursor );
        break;

    case eResize_TopRightCorner:
        SetViewCursor( Qt::SizeBDiagCursor );
        break;

    case eResize_BottomLeftCorner:
        SetViewCursor( Qt::SizeBDiagCursor );
        break;

    case eResize_BottomRightCorner:
        SetViewCursor( Qt::SizeFDiagCursor );
        break;

    case eResize_Top:
        SetViewCursor( Qt::SizeVerCursor );
        break;

    case eResize_Left:
        SetViewCursor( Qt::SizeHorCursor );
        break;

    case eResize_Right:
        SetViewCursor( Qt::SizeHorCursor );
        break;

    case eResize_Bottom:
        SetViewCursor( Qt::SizeVerCursor );
        break;
    }
}

void ResizeableRectItem::hoverLeaveEvent( QGraphicsSceneHoverEvent* aEvent )
{
    SetViewCursor( Qt::ArrowCursor );
    QGraphicsItem::hoverLeaveEvent( aEvent );
}

QVariant ResizeableRectItem::itemChange( GraphicsItemChange aChange, const QVariant& aValue )
{
    if ( aChange == ItemPositionHasChanged )
    {
        // TODO: Calc new parent if mouse not down?
    }
    return QGraphicsRectItem::itemChange( aChange, aValue );
}

void ResizeableRectItem::Init()
{
    m_ResizeMode = eResize_None;

    // Must set pen width for bounding rect calcs
    setPen( QPen( Qt::black, 2, Qt::SolidLine ) );

    setAcceptHoverEvents( true );

    // Change mouse cursor on hover so its easy to see when you can click to move the item
    SetViewCursor( Qt::PointingHandCursor );

    // Allow select and move.
    setFlags( ItemSendsScenePositionChanges |  ItemSendsGeometryChanges | ItemIsMovable | ItemIsSelectable );

    // Test image - TODO: Memory for these should be managed better, don't load same pixamp N times
    /*
    if ( !QPixmapCache::find( "019_Security_eye.bmp", m_Pixmap ) )
    {

        QPixmapCache::insert( "019_Security_eye.bmp", m_Pixmap );
    }
    */
     //m_Pixmap = QPixmap("019_Security_eye.bmp");

     // TODO: Use QPixmapCache instead
   //  setCacheMode( ItemCoordinateCache );

     this->setOpacity( 0.7 );
}


ResizeableRectItem::eResize ResizeableRectItem::getResizeLocation( QPointF aPos, QRectF aRect )
{
    const auto x = aPos.x();
    const auto y = aPos.y();
    const auto& bRect = aRect;

    bool xPosNearRectX = IsNear( x, bRect.x() );
    bool yPosNearRectY = IsNear( y, bRect.y() );
    bool xPosNearRectW = IsNear( x, bRect.x() + bRect.width() );
    bool yPosNearRectH = IsNear( y, bRect.y() + bRect.height() );

    // Top right corner
    if ( xPosNearRectW && yPosNearRectY )
    {
        return eResize_TopRightCorner;
    }

    // Bottom left corner
    if ( xPosNearRectX && yPosNearRectH )
    {
        return eResize_BottomLeftCorner;
    }

    // Top left corner
    if ( xPosNearRectX && yPosNearRectY )
    {
        return eResize_TopLeftCorner;
    }

    // Bottom right corner
    if ( xPosNearRectW && yPosNearRectH )
    {
        return eResize_BottomRightCorner;
    }

    // Left edge
    if ( xPosNearRectX && !yPosNearRectY )
    {
        return eResize_Left;
    }

    // Top edge
    if ( !xPosNearRectX && yPosNearRectY )
    {
        return eResize_Top;
    }

    // Right edge
    if ( xPosNearRectW && !yPosNearRectH )
    {
        return eResize_Right;
    }

    // Bottom edge
    if ( !xPosNearRectW && yPosNearRectH )
    {
        return eResize_Bottom;
    }

    return eResize_None;
}

bool ResizeableRectItem::IsNear( qreal xP1, qreal xP2 )
{
    qreal tolerance = 8; // aka epsilon
    if ( rect().width() <= kMinRectSize || rect().height() <= kMinRectSize )
    {
        tolerance = 1;
    }
    if ( abs( xP1-xP2 ) <= tolerance )
    {
        return true;
    }
    return false;
}

void ResizeableRectItem::onResize( QPointF aPos )
{
    QRectF curRect = rect();
    const bool isLeft = ( m_ResizeMode == eResize_Left )     || ( m_ResizeMode == eResize_TopLeftCorner )    || ( m_ResizeMode == eResize_BottomLeftCorner );
    const bool isRight = ( m_ResizeMode == eResize_Right )   || ( m_ResizeMode == eResize_TopRightCorner )   || ( m_ResizeMode == eResize_BottomRightCorner );
    const bool isTop = ( m_ResizeMode == eResize_Top )       || ( m_ResizeMode == eResize_TopLeftCorner )    || ( m_ResizeMode == eResize_TopRightCorner );
    const bool isBottom = ( m_ResizeMode == eResize_Bottom ) || ( m_ResizeMode == eResize_BottomLeftCorner ) || ( m_ResizeMode == eResize_BottomRightCorner );

    if ( isRight )
    {
        qreal newWidth = aPos.x() - curRect.x();
        if ( newWidth < kMinRectSize )
        {
            newWidth = kMinRectSize;
        }
        curRect.setWidth( newWidth );
    }
    else if ( isLeft )
    {
        qreal newx = aPos.x();
        if ( newx > (curRect.x()+curRect.width())-kMinRectSize )
        {
            newx = (curRect.x()+curRect.width())-kMinRectSize;
        }
        curRect.setX( newx );
    }

    if ( isTop )
    {
        qreal newy = aPos.y();
        if ( newy > (curRect.y()+curRect.height()-kMinRectSize))
        {
            newy = curRect.y()+curRect.height()-kMinRectSize;
        }
        curRect.setY( newy );
    }
    else if ( isBottom )
    {
        qreal newHeight = aPos.y() - curRect.y();
        if ( newHeight < kMinRectSize )
        {
            newHeight = kMinRectSize;
        }
        curRect.setHeight( newHeight );
    }

    prepareGeometryChange();
    setRect( curRect );
}

void ResizeableRectItem::SetViewCursor(Qt::CursorShape cursor)
{
    mView->setCursor(cursor);
}
