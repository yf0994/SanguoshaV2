#include <magatamasItem.h>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include "SkinBank.h"

MagatamasBoxItem::MagatamasBoxItem(QGraphicsItem *parent/* = 0*/)
    : QGraphicsObject(parent)
{
    m_hp = 0;
    m_maxHp = 0;
}

void MagatamasBoxItem::setOrientation(Qt::Orientation orientation) {
    m_orientation = orientation;
    _updateLayout();
}

void MagatamasBoxItem::_updateLayout() {
    int xStep, yStep;
    if (this->m_orientation == Qt::Horizontal) {
        xStep = m_iconSize.width();
        yStep = 0;
    } else {
        xStep = 0;
        yStep = m_iconSize.height();
    }

    for (int i = 0; i < 6; ++i) {
        _icons[i] = G_ROOM_SKIN.getPixmap(QString(QSanRoomSkin::S_SKIN_KEY_MAGATAMAS).arg(QString::number(i)), QString(), true)
                                          .scaled(m_iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    for (int i = 1; i < 6; ++i) {
        QSize bgSize;
        if (this->m_orientation == Qt::Horizontal) {
            bgSize.setWidth((xStep + 1) * i);
            bgSize.setHeight(m_iconSize.height());
        } else {
            bgSize.setWidth((yStep + 1) * i);
            bgSize.setHeight(m_iconSize.width());
        }
        _bgImages[i] = G_ROOM_SKIN.getPixmap(QString(QSanRoomSkin::S_SKIN_KEY_MAGATAMAS_BG).arg(QString::number(i)))
                                             .scaled(bgSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
}

void MagatamasBoxItem::setIconSize(QSize size) {
    m_iconSize = size;
    _updateLayout();
}

QRectF MagatamasBoxItem::boundingRect() const{
    int buckets = (m_maxHp > 5 ? 4 : qMin(m_maxHp, 5)) + G_COMMON_LAYOUT.m_hpExtraSpaceHolder;

    if (m_orientation == Qt::Horizontal)
        return QRectF(0, 0, buckets * m_iconSize.width(), m_iconSize.height());
    else
        return QRectF(0, 0, m_iconSize.width(), buckets * m_iconSize.height());
}

void MagatamasBoxItem::setHp(int hp) {
    _doHpChangeAnimation(hp);
    m_hp = hp;
    update();
}

void MagatamasBoxItem::setAnchor(QPoint anchor, Qt::Alignment align) {
    m_anchor = anchor;
    m_align = align;
}

void MagatamasBoxItem::setMaxHp(int maxHp) {
    m_maxHp = maxHp;
    _autoAdjustPos();
}

void MagatamasBoxItem::_autoAdjustPos() {
    if (!anchorEnabled) return;
    QRectF rect = boundingRect();
    Qt::Alignment hAlign = m_align & Qt::AlignHorizontal_Mask;
    if (hAlign == Qt::AlignRight)
        setX(m_anchor.x() - rect.width());
    else if (hAlign == Qt::AlignHCenter)
        setX(m_anchor.x() - rect.width() / 2);
    else
        setX(m_anchor.x());
    Qt::Alignment vAlign = m_align & Qt::AlignVertical_Mask;
    if (vAlign == Qt::AlignBottom)
        setY(m_anchor.y() - rect.height());
    else if (vAlign == Qt::AlignVCenter)
        setY(m_anchor.y() - rect.height() / 2);
    else
        setY(m_anchor.y());
}

void MagatamasBoxItem::update() {
    _updateLayout();
    _autoAdjustPos();
    QGraphicsItem::update();
}

#include "sprite.h"
#include "pixmapanimation.h"

void MagatamasBoxItem::_doHpChangeAnimation(int newHp)
{
    if (newHp >= m_hp) return;

	int width = m_iconSize.width();
    int height = m_iconSize.height();
    int xStep, yStep;
    if (this->m_orientation == Qt::Horizontal) {
        xStep = width;
        yStep = 0;
    } else {
        xStep = 0;
        yStep = height;
    }

    if (newHp < 0)
        newHp = 0;

	PixmapAnimation *pma = PixmapAnimation::GetPixmapAnimation(this, "destroy");
	
	if (m_showBackground) {
	    pma->moveBy((boundingRect().width() - pma->boundingRect().width()) / 2,
                (boundingRect().height() - pma->boundingRect().height()));
	    pma->moveBy(45, 55);
	
	    int pos = m_maxHp > 5 ? 4 : newHp;
	
	    pma->moveBy(- xStep * pos, - yStep * pos);
		
	} else {
		pma->moveBy((width - pma->boundingRect().width()) / 2,
                (height - pma->boundingRect().height()));

		pma->moveBy(5, 5);
		
		
	    int pos = m_maxHp > 5 ? 0 : (m_maxHp - 1 - newHp);
	
	    pma->moveBy(xStep * pos, yStep * pos);
	}
}


void MagatamasBoxItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    if (m_maxHp <= 0) return;
    int imageIndex = qBound(0, m_hp, 5);
    if (m_hp == m_maxHp) imageIndex = 5;

    int xStep, yStep;
    if (this->m_orientation == Qt::Horizontal) {
        xStep = m_iconSize.width();
        yStep = 0;
    } else {
        xStep = 0;
        yStep = m_iconSize.height();
    }

    if (m_showBackground) {
        if (this->m_orientation == Qt::Vertical) {
            painter->save();
            painter->translate(m_iconSize.width(), 0);
            painter->rotate(90);
        }

        int bgImageIndex = m_maxHp > 5 ? 4 : qMin(m_maxHp, 5);
        QPoint bgImagePos = this->m_orientation == Qt::Vertical ? QPoint(0, 0)
            : m_imageArea.topLeft();
        painter->drawPixmap(bgImagePos, _bgImages[bgImageIndex]);

        if (this->m_orientation == Qt::Vertical)
            painter->restore();
    }

    if (m_maxHp <= 5) {
        int i;
        int lostHp = qMin(m_maxHp - m_hp, m_maxHp);
        for (i = 0; i < lostHp; ++i) {
            QRect rect(xStep * i, yStep * i, m_imageArea.width(), m_imageArea.height());
            rect.translate(m_imageArea.topLeft());
            painter->drawPixmap(rect, _icons[0]);
        }
        for (; i < m_maxHp; ++i) {
            QRect rect(xStep * i, yStep * i, m_imageArea.width(), m_imageArea.height());
            rect.translate(m_imageArea.topLeft());
            painter->drawPixmap(rect, _icons[imageIndex]);
        }
    } else {
        painter->drawPixmap(m_imageArea, _icons[imageIndex]);

        //��ȼ�2��Ϊ�˱�����λ������ʾ��ȫ
        QRect rect(xStep - 2, yStep, m_imageArea.width() + 2, m_imageArea.height());
        rect.translate(m_imageArea.topLeft());

        if (this->m_orientation == Qt::Horizontal)
            rect.translate(xStep * 0.5, yStep * 0.5);
        G_COMMON_LAYOUT.m_hpFont[imageIndex].paintText(painter, rect, Qt::AlignCenter, QString::number(m_hp));
        rect.translate(xStep, yStep);
        G_COMMON_LAYOUT.m_hpFont[imageIndex].paintText(painter, rect, Qt::AlignCenter, "/");
        rect.translate(xStep, yStep);
        G_COMMON_LAYOUT.m_hpFont[imageIndex].paintText(painter, rect, Qt::AlignCenter, QString::number(m_maxHp));
    }
}
