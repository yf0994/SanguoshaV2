#include "carditem.h"
#include "engine.h"
#include "SkinBank.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QApplication>

CardItem::CardItem(const Card *card)
    : m_backgroundPixmap(G_COMMON_LAYOUT.m_cardMainArea.size()),
    m_canvas(G_COMMON_LAYOUT.m_cardMainArea.size())
{
    initialize();
    setCard(card);
    setAcceptHoverEvents(true);
}

CardItem::CardItem(const QString &generalName)
    : m_backgroundPixmap(G_COMMON_LAYOUT.m_cardMainArea.size()),
    m_canvas(G_COMMON_LAYOUT.m_cardMainArea.size())
{
    m_cardId = Card::S_UNKNOWN_CARD_ID;
    initialize();
    changeGeneral(generalName);
}

CardItem::~CardItem()
{
    QMutexLocker locker(&m_animationMutex);
    stopAnimation();
}

void CardItem::initialize()
{
    //����Ĭ�ϲ�֧���κ���갴�����Ա�����������������¿��������ڽ������޷���ʧ������
    setAcceptedMouseButtons(0);

    setFlag(QGraphicsItem::ItemIsMovable);

    m_opacityAtHome = 1.0;
    m_currentAnimation = NULL;
    _m_width = G_COMMON_LAYOUT.m_cardNormalWidth;
    _m_height = G_COMMON_LAYOUT.m_cardNormalHeight;
    m_showFootnote = true;
    m_isSelected = false;
    m_autoBack = true;
    m_mouseDoubleclicked = false;

    resetTransform();
    setTransform(QTransform::fromTranslate(-_m_width / 2, -_m_height / 2), true);

    m_backgroundPixmap.fill(Qt::transparent);
    m_canvas.fill(Qt::transparent);

    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

QRectF CardItem::boundingRect() const
{
    return G_COMMON_LAYOUT.m_cardMainArea;
}

void CardItem::setCard(const Card *card)
{
    m_cardId = (card ? card->getId() : Card::S_UNKNOWN_CARD_ID);

    const Card *engineCard = Sanguosha->getEngineCard(m_cardId);
    if (engineCard) {
        setObjectName(engineCard->objectName());
        setToolTip(engineCard->getDescription());
    }
    else {
        setObjectName("unknown");
    }

    QPainter painter(&m_backgroundPixmap);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    painter.drawPixmap(G_COMMON_LAYOUT.m_cardMainArea,
        G_ROOM_SKIN.getCardMainPixmap(objectName(), true));

    if (engineCard) {
        painter.drawPixmap(G_COMMON_LAYOUT.m_cardSuitArea,
            G_ROOM_SKIN.getCardSuitPixmap(engineCard->getSuit()));

        painter.drawPixmap(G_COMMON_LAYOUT.m_cardNumberArea,
            G_ROOM_SKIN.getCardNumberPixmap(engineCard->getNumber(), engineCard->isBlack()));
    }
}

void CardItem::changeGeneral(const QString &generalName)
{
    setObjectName(generalName);

    QPainter painter(&m_backgroundPixmap);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    const General *general = Sanguosha->getGeneral(generalName);
    if (general) {
        painter.drawPixmap(G_COMMON_LAYOUT.m_cardMainArea,
            G_ROOM_SKIN.getCardMainPixmap(objectName(), true));

        setToolTip(general->getSkillDescription(true));
    }
    else {
        painter.drawPixmap(G_COMMON_LAYOUT.m_cardMainArea,
            G_ROOM_SKIN.getPixmap("generalCardBack", QString(), true));

        setToolTip(QString());
    }
}

const Card *CardItem::getCard() const
{
    return Sanguosha->getCard(m_cardId);
}

void CardItem::goBack(bool playAnimation, bool doFade)
{
    if (playAnimation) {
        getGoBackAnimation(doFade);
        if (m_currentAnimation != NULL) {
            m_currentAnimation->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }
    else {
        QMutexLocker locker(&m_animationMutex);
        stopAnimation();
        setPos(homePos());
    }
}

QAbstractAnimation *CardItem::getGoBackAnimation(bool doFade, bool smoothTransition, int duration)
{
    QMutexLocker locker(&m_animationMutex);
    stopAnimation();

    QPropertyAnimation *goback = new QPropertyAnimation(this, "pos", this);
    goback->setEndValue(m_homePos);
    goback->setEasingCurve(QEasingCurve::OutQuad);
    goback->setDuration(duration);

    if (doFade) {
        QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
        QPropertyAnimation *disappear = new QPropertyAnimation(this, "opacity", this);
        double middleOpacity = qMax(opacity(), m_opacityAtHome);
        if (middleOpacity == 0) middleOpacity = 1.0;
        disappear->setEndValue(m_opacityAtHome);
        if (!smoothTransition) {
            disappear->setKeyValueAt(0.2, middleOpacity);
            disappear->setKeyValueAt(0.8, middleOpacity);
            disappear->setDuration(duration);
        }

        group->addAnimation(goback);
        group->addAnimation(disappear);

        m_currentAnimation = group;
    }
    else {
        m_currentAnimation = goback;
    }

    connect(m_currentAnimation, SIGNAL(finished()), this, SIGNAL(movement_animation_finished()));
    connect(m_currentAnimation, SIGNAL(finished()),
        this, SLOT(animationFinished()));

    return m_currentAnimation;
}

void CardItem::showAvatar(const General *general)
{
    m_avatarName = general->objectName();
}

CardItem *CardItem::FindItem(const QList<CardItem *> &items, int card_id)
{
    foreach (CardItem *item, items) {
        if (item->getCard() == NULL) {
            if (card_id == Card::S_UNKNOWN_CARD_ID) {
                return item;
            }
        }
        else if (item->getCard()->getId() == card_id) {
            return item;
        }
    }

    return NULL;
}

void CardItem::mousePressEvent(QGraphicsSceneMouseEvent *)
{
    //���п���������һ�ζ���֮ǰ����ֹ֮ͣǰ�п��ܻ�δ�����Ķ���
    //�Ա����������Ҫ����Ч��
    QMutexLocker locker(&m_animationMutex);
    stopAnimation();

    m_mouseDoubleclicked = false;
}

void CardItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *)
{
    //���������˫���¼�������˫���¼����ֻ���Ų���mouseReleaseEvent��
    //�����Ҫ�������release�¼��������ƻ���������ɵ��ƶ�Ч��
    if (m_mouseDoubleclicked) {
        return;
    }

    emit released();

    if (m_autoBack) {
        goBack(true, false);
    }
}

void CardItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!(flags() & QGraphicsItem::ItemIsMovable)
        || QLineF(event->screenPos(), event->buttonDownScreenPos(Qt::LeftButton)).length()
        < QApplication::startDragDistance()) {
        return;
    }

    QPointF newPos = mapToParent(event->pos());
    QPointF downPos = event->buttonDownPos(Qt::LeftButton);
    setPos(newPos - transform().map(downPos));
}

void CardItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *)
{
    m_mouseDoubleclicked = true;

    if (hasFocus()) {
        emit double_clicked();
    }
}

void CardItem::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    emit enter_hover();
}

void CardItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    emit leave_hover();
}

void CardItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    //������֧������Ϸ����ʱ�˳��Ĺ��ܣ������Է��֣�
    //��ʱpainter������Ϊ�գ��Ӷ����³��������
    //�����������жϣ��Ա���������
    if (NULL == painter) {
        return;
    }

    QPainter tempPainter(&m_canvas);
    tempPainter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    if (!isEnabled()) {
        tempPainter.fillRect(G_COMMON_LAYOUT.m_cardMainArea, QColor(100, 100, 100, 255));
        tempPainter.setOpacity(0.7);
    }

    tempPainter.drawPixmap(G_COMMON_LAYOUT.m_cardMainArea, m_backgroundPixmap);

    if (m_showFootnote) {
        //��ע����ʼ�ղ��䰵
        tempPainter.setOpacity(1);

        tempPainter.drawImage(G_COMMON_LAYOUT.m_cardFootnoteArea,
            m_footnoteImage);
    }

    if (!m_avatarName.isEmpty()) {
        //Сͼ��ʼ�ղ��䰵
        tempPainter.setOpacity(1);

        //Բ�ǻ�Сͼ��
        QPainterPath roundRectPath;
        roundRectPath.addRoundedRect(G_COMMON_LAYOUT.m_cardAvatarArea, 4, 4);
        tempPainter.setClipPath(roundRectPath);

        tempPainter.drawPixmap(G_COMMON_LAYOUT.m_cardAvatarArea,
            G_ROOM_SKIN.getCardAvatarPixmap(m_avatarName));
    }

    tempPainter.end();

    painter->drawPixmap(G_COMMON_LAYOUT.m_cardMainArea, m_canvas);
}

void CardItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *)
{
    emit context_menu_event_triggered();
}

void CardItem::setFootnote(const QString &desc)
{
    if (!desc.isEmpty()) {
        paintFootnoteImage(G_COMMON_LAYOUT.m_cardFootnoteFont, desc);
    }
}

void CardItem::setDoubleClickedFootnote(const QString &desc)
{
    //����佫���ƹ��������ᵼ�½�ע���ֱ�Сͼ������ס��
    //����޶�������佫���Ʋ�����6���֣�������ʵ����ӽ�ע����Ĵ�С���Ա�������ڿ������ע���֣�
    //������ʹ��ԭ��ע�����С
    if (desc.size() < 6) {
        SanShadowTextFont font = G_COMMON_LAYOUT.m_cardFootnoteFont;
        QSize fontNewSize = font.size();
        fontNewSize.rwidth() += 2;
        fontNewSize.rheight() += 2;
        font.setSize(fontNewSize);

        paintFootnoteImage(font, desc);
    }
    else {
        paintFootnoteImage(G_COMMON_LAYOUT.m_cardFootnoteFont, desc);
    }
}

void CardItem::paintFootnoteImage(const SanShadowTextFont &footnoteFont,
    const QString &desc)
{
    QRect rect = G_COMMON_LAYOUT.m_cardFootnoteArea;
    rect.moveTopLeft(QPoint(0, 0));
    m_footnoteImage = QImage(rect.size(), QImage::Format_ARGB32);
    m_footnoteImage.fill(Qt::transparent);
    QPainter painter(&m_footnoteImage);
    footnoteFont.paintText(&painter, QRect(QPoint(0, 0), rect.size()),
        (Qt::AlignmentFlag)((int)Qt::AlignHCenter | Qt::AlignBottom | Qt::TextWrapAnywhere), desc);
}

void CardItem::animationFinished()
{
    QMutexLocker locker(&m_animationMutex);
    m_currentAnimation = NULL;
}

void CardItem::stopAnimation()
{
    if (m_currentAnimation != NULL) {
        m_currentAnimation->stop();
        delete m_currentAnimation;
        m_currentAnimation = NULL;
    }
}
