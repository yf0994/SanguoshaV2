#ifndef _DASHBOARD_H
#define _DASHBOARD_H

#include "QSanSelectableItem.h"
#include "qsanbutton.h"
#include "carditem.h"
#include "player.h"
#include "skill.h"
#include "protocol.h"
#include "TimedProgressBar.h"
#include "GenericCardContainerUI.h"
#include "pixmapanimation.h"
#include "sprite.h"
#include "util.h"

#include <QPushButton>
#include <QComboBox>
#include <QGraphicsLinearLayout>
#include <QLineEdit>
#include <QMutex>
#include <QPropertyAnimation>
#include <QGraphicsProxyWidget>

class Dashboard: public PlayerCardContainer {
    Q_OBJECT
    Q_ENUMS(SortType)

public:
    enum SortType { ByType, BySuit, ByNumber };

    explicit Dashboard(QGraphicsPixmapItem *button_widget);

    virtual QRectF boundingRect() const;
    void setWidth(int width);
    int getMiddleWidth();

    void hideControlButtons();
    virtual void showProgressBar(const QSanProtocol::Countdown &countdown);

    QRectF getProgressBarSceneBoundingRect() const {
        return _m_progressBarItem->sceneBoundingRect();
    }
    QRectF getAvatarAreaSceneBoundingRect() const {
        return _m_rightFrame->sceneBoundingRect();
    }

    QSanSkillButton *removeSkillButton(const QString &skillName);
    QSanSkillButton *addSkillButton(const QString &skillName);
    void removeAllSkillButtons();

    bool isAvatarUnderMouse();

    void highlightEquip(QString skillName, bool hightlight);

    void setTrust(bool trust);
    virtual void killPlayer();

    //����������"�л�ȫ������"�Ĺ��ܣ����dashboardҲ��Ҫ���ش˺������ػ��Լ������ⲿ��
    virtual void repaintAll();

    void selectCard(const QString &pattern, bool forward = true, bool multiple = false);
    void selectEquip(int position);
    void selectOnlyCard(bool need_only = false);
    void useSelected();
    const Card *getSelected() const;
    void unselectAll(const CardItem *except = NULL);

    void disableAllCards();
    void enableCards();
    void enableAllCards();

    void adjustCards(bool playAnimation = true);

    virtual QGraphicsItem *getMouseClickReceiver() { return _m_rightFrame; }

    QList<CardItem *> removeCardItems(const QList<int> &card_ids, Player::Place place);
    virtual QList<CardItem *> cloneCardItems(QList<int> card_ids);

    // pending operations
    void startPending(const ViewAsSkill *skill);
    void stopPending();
    void updatePending();
    void clearPendings();

    void addPending(CardItem *item) { pendings << item; }
    const QList<CardItem *> &getPendings() const { return pendings; }
    bool hasHandCard(CardItem *item) const { return m_handCards.contains(item); }

    const ViewAsSkill *currentSkill() const;
    const Card *pendingCard() const;

    void expandPileCards(const QString &pile_name);
    void retractPileCards(const QString &pile_name);
	void retractAllSkillPileCards();
    inline const QStringList &getPileExpanded() const
    {
        return _m_pile_expanded;
    }

    void selectCard(CardItem *item, bool isSelected);

    int getButtonWidgetWidth() const;
    int getTextureWidth() const;

    int width();
    int height();

    //��ȡ���ƿ���ͷ���ĸ߶Ȳ���������OL����ͷ������
    int middleFrameAndRightFrameHeightDiff() const {
        return m_middleFrameAndRightFrameHeightDiff;
    }

    void showNullificationButton();
    void hideNullificationButton();

    static const int S_PENDING_OFFSET_Y = -25;

    inline void updateSkillButton() {
        if (_m_skillDock)
            _m_skillDock->update();
    }

public slots:
    virtual void updateAvatar();
    void sortCards();
    void beginSorting();
	void changeShefuState();
    void reverseSelection();
    void cancelNullification();
	void setShefuState();
    void skillButtonActivated();
    void skillButtonDeactivated();
    void selectAll();
    void controlNullificationButton(bool show);

protected:
    void _createExtraButtons();
    virtual void _adjustComponentZValues();
    virtual void addHandCards(QList<CardItem *> &cards);
    virtual QList<CardItem *> removeHandCards(const QList<int> &cardIds);

    // initialization of _m_layout is compulsory for children classes.
    inline virtual QGraphicsItem *_getEquipParent() { return _m_leftFrame; }
    inline virtual QGraphicsItem *_getDelayedTrickParent() { return _m_leftFrame; }
    inline virtual QGraphicsItem *_getAvatarParent() { return _m_rightFrame; }
    inline virtual QGraphicsItem *_getMarkParent() { return _m_floatingArea; }
    inline virtual QGraphicsItem *_getPhaseParent() { return _m_floatingArea; }
    inline virtual QGraphicsItem *_getRoleComboBoxParent() { return button_widget; }
    inline virtual QGraphicsItem *_getPileParent() { return _m_rightFrame; }
    inline virtual QGraphicsItem *_getProgressBarParent() { return _m_floatingArea; }
    inline virtual QGraphicsItem *_getFocusFrameParent() { return _m_rightFrame; }
    virtual QGraphicsItem *_getDeathIconParent() { return _m_groupDeath; }

    inline virtual QString getResourceKeyName() { return QSanRoomSkin::S_SKIN_KEY_DASHBOARD; }

    virtual const SanShadowTextFont &getSkillNameFont() const {
        return G_DASHBOARD_LAYOUT.m_skillNameFont;
    }
    virtual const QRect &getSkillNameArea() const { return G_DASHBOARD_LAYOUT.m_skillNameArea; }
    virtual QGraphicsItem *_getHandCardNumParent() const { return button_widget; }
    virtual QPointF getHeroSkinContainerPosition() const;

    bool _addCardItems(QList<CardItem *> &card_items, const CardsMoveStruct &moveInfo);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);

    //װ��������֧��˫���¼�
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);

    //֧�ּ�ʹ���Ʊ�disabled�ˣ������ʱȷ����ť���ã�Ҳ��˫������Ĺ���
    //���Ӵ˹��ܵ������ڻ��ι�ģʽ����ѯ���Ƿ񷢶�"��������"����ʱ���������ѱ�disabled�ˣ���
    //ϣ���ܼ������˫����ʾ�������������Ҽ�˫����ʾ����������װ����������
    virtual bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);

    void _onCardItemDoubleClicked(CardItem *card_item);

    void _addHandCard(CardItem *card_item, bool prepend = false, const QString &footnote = QString());
    void _adjustCards();
    void _adjustCards(const QList<CardItem *> &list, int y);

    int _m_width;
    // sync objects
    QMutex m_mutex;
    QMutex m_mutexEnableCards;

    QSanButton *m_btnReverseSelection;
    QSanButton *m_btnSortHandcard;
    QSanButton *m_btnNoNullification;
	QSanButton *m_btnShefu;
    QGraphicsPixmapItem *_m_leftFrame, *_m_middleFrame, *_m_rightFrame;
    QGraphicsPixmapItem *button_widget;

    CardItem *selected;
    QList<CardItem *> m_handCards;

    QGraphicsPathItem *trusting_item;
    QGraphicsSimpleTextItem *trusting_text;

    QSanInvokeSkillDock* _m_skillDock;
    const QSanRoomSkin::DashboardLayout *_dlayout;

    //for animated effects
    EffectAnimation *animations;

    // for parts creation
    void _createLeft();
    void _createRight();
    void _createMiddle();
    void _updateFrames();

    void _paintLeftFrame();
    void _paintMiddleFrame(const QRect &rect);
    void _paintRightFrame();

    // for pendings
    QList<CardItem *> pendings;
    const Card *pending_card;
    const ViewAsSkill *view_as_skill;
    const FilterSkill *filter;
    QStringList _m_pile_expanded;

    // for equip skill/selections
    PixmapAnimation *_m_equipBorders[S_EQUIP_AREA_LENGTH];
    QSanSkillButton *_m_equipSkillBtns[S_EQUIP_AREA_LENGTH];
    bool _m_isEquipsAnimOn[S_EQUIP_AREA_LENGTH];
    QList<QSanSkillButton *> _m_button_recycle;

    void _createEquipBorderAnimations();
    void _setEquipBorderAnimation(int index, bool turnOn);

    void drawEquip(QPainter *painter, const CardItem *equip, int order);
    void setSelectedItem(CardItem *card_item);

    QMenu *_m_sort_menu;
	QMenu *_m_shefu_menu;
    QMenu *_m_carditem_context_menu;

    //���浱ǰ�ƽ�Dashboard��ʹ�õĿ���
    QList<CardItem *> _m_cardItemsAnimationFinished;
    QMutex m_mutexCardItemsAnimationFinished;

    CardItem *m_selectedEquipCardItem;
    int m_middleFrameAndRightFrameHeightDiff;

protected slots:
    virtual void _onEquipSelectChanged();

    //�ڲ��ſ����ƽ�Dashboard�Ķ��������У���������ʱ�ɲ�����ס���ƺ������ƶ���Bug��
    //Ϊ��������⣬Dashboard��Ҫ��д����۷���
    virtual void onAnimationFinished();

    //�Լ���ͷ�������̶���ʾ�ǳƣ������������ͣ������ʱ����ʾ
    virtual void onAvatarHoverEnter();

    virtual void doAvatarHoverLeave() { _m_screenNameItem->hide(); }
    virtual bool isItemUnderMouse(QGraphicsItem *item);

private slots:
    void onCardItemClicked();

    //���������Ƴ���"��ѡ"��"��������"���ܣ�ת�Ƶ��Ҽ��˵���ʵ��
    void onCardItemContextMenu();

    //����˫������ֱ�ӳ��ƵĹ���
    void onCardItemDoubleClicked();

    void onCardItemHover();
    void onCardItemLeaveHover();
    void onMarkChanged();

    void updateRolesForHegemony(const QString &generalName);

signals:
    void card_selected(const Card *card);
    void progressBarTimedOut();
};

#endif
