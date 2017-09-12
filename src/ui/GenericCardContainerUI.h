#ifndef _GENERAL_CARD_CONTAINER_UI_H
#define _GENERAL_CARD_CONTAINER_UI_H

#include "carditem.h"
#include "player.h"
#include "QSanSelectableItem.h"
#include "SkinBank.h"
#include "TimedProgressBar.h"
#include "magatamasItem.h"
#include "rolecombobox.h"

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QMutex>

#include <qparallelanimationgroup.h>
#include <qgraphicseffect.h>
#include <qvariant.h>
#include <qlabel.h>

class GenericCardContainer: public QGraphicsObject {
    Q_OBJECT

public:
    inline GenericCardContainer() { _m_highestZ = 10000; }
    virtual QList<CardItem *> removeCardItems(const QList<int> &card_ids, Player::Place place) = 0;
    virtual void addCardItems(QList<CardItem *> &card_items, const CardsMoveStruct &moveInfo);
    virtual QList<CardItem *> cloneCardItems(QList<int> card_ids);

protected:
    // @return Whether the card items should be destroyed after animation
    virtual bool _addCardItems(QList<CardItem *> &card_items, const CardsMoveStruct &moveInfo) = 0;
    QList<CardItem *> _createCards(QList<int> card_ids);
    CardItem *_createCard(int card_id);
    void _disperseCards(QList<CardItem *> &cards, QRectF fillRegion, Qt::Alignment align, bool useHomePos, bool keepOrder);
    void _playMoveCardsAnimation(QList<CardItem *> &cards, bool destroyCards);
    int _m_highestZ;

protected slots:
    virtual void onAnimationFinished();

private slots:
    void _doUpdate();
    void _destroyCard();

private:
    static bool _horizontalPosLessThan(const CardItem *card1, const CardItem *card2);

signals:
    void animation_finished();
};

class GraphicsPixmapHoverItem;
class GraphicsWidgetHoverItem;
class HeroSkinContainer;

class PlayerCardContainer: public GenericCardContainer {
    Q_OBJECT

public:
    PlayerCardContainer();

    virtual void showProgressBar(const QSanProtocol::Countdown &countdown);
    void hideProgressBar();
    bool isProgressBarVisible() const {
        if (_m_progressBar != NULL) {
            return _m_progressBar->isVisible();
        }
        return false;
    }

    void setReadyIconVisible(bool visible);

    const QGraphicsItem *getDoubleClickedItem() const { return m_doubleClickedItem; }
    void clearDoubleClickedItem() { m_doubleClickedItem = NULL; }

    void hideAvatars();

    const ClientPlayer *getPlayer() const { return m_player; }
    void setPlayer(ClientPlayer *player);

    inline int getVotes() { return _m_votesGot; }
    inline void setMaxVotes(int maxVotes) { _m_maxVotes = maxVotes; }

    // See _m_floatingArea for more information
    inline QRect getFloatingArea() const{ return _m_floatingAreaRect; }
    void setFloatingArea(const QRect &rect);

    // repaintAll is different from refresh in that it recreates all controls and is
    // very costly. Avoid calling this except for changing skins or only once during
    // the initialization. If you just want to update the information displayed, call
    // refresh instead.
    virtual void repaintAll();

    virtual void killPlayer();
    virtual void revivePlayer();

    virtual QGraphicsItem *getMouseClickReceiver() = 0;

    virtual void startHuaShen(const QString &generalName, const QString &skillName);
    virtual void stopHuaShen();
    const QString &getHuaShenGeneralName() const { return _m_huashenGeneralName; }

    virtual void updateAvatarTooltip();

    QPixmap paintByMask(const QPixmap &source);

    bool canBeSelected();

    void showSkillName(const QString &skill_name);

    QPixmap _getAvatarIcon(const QString &generalName);
    QPixmap getSmallAvatarIcon(const QString &generalName);
    GraphicsPixmapHoverItem *getAvartarItem() const { return _m_avatarIcon; }
    GraphicsPixmapHoverItem *getSmallAvartarItem() const { return _m_smallAvatarIcon; }

    static void _paintPixmap(QGraphicsPixmapItem *&item, const QRect &rect,
        const QPixmap &pixmap, QGraphicsItem *parent);

    void showSkillPromptInfo(const QString &skillPromptInfo);

    void stopHeroSkinChangingAnimation();

public slots:
    virtual void updateAvatar();
    void updateSmallAvatar();
    void updateReadyItem(bool visible);
    void updatePhase();
    void updateHp();
    void updateHandcardNum();
    void updateDrankState();
    virtual void updateDuanchang();
    void updatePile(const QString &pile_name);
    void updateMarks();
    void updateVotes(bool need_select = true, bool display_1 = false);
    void updateReformState();
    void showDistance();
    virtual void refresh();
    void updateSaveMeIcon(bool visible);
    void updateOwnerIcon(bool owner);
    void updateScreenName(const QString &screenName);
    void hideSkillName();

protected:
    // overrider parent functions
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);

    // initialization of _m_layout is compulsory for children classes.
    virtual QGraphicsItem *_getEquipParent() = 0;
    virtual QGraphicsItem *_getDelayedTrickParent() = 0;
    virtual QGraphicsItem *_getAvatarParent() = 0;
    virtual QGraphicsItem *_getMarkParent() = 0;
    virtual QGraphicsItem *_getPhaseParent() = 0;
    virtual QGraphicsItem *_getRoleComboBoxParent() = 0;
    virtual QGraphicsItem *_getPileParent() = 0;
    virtual QGraphicsItem *_getFocusFrameParent() = 0;
    virtual QGraphicsItem *_getProgressBarParent() = 0;
    virtual QGraphicsItem *_getDeathIconParent() = 0;
    virtual QString getResourceKeyName() = 0;
    virtual const SanShadowTextFont &getSkillNameFont() const = 0;
    virtual const QRect &getSkillNameArea() const = 0;
    virtual QGraphicsItem *_getHandCardNumParent() const = 0;
    virtual QPointF getHeroSkinContainerPosition() const = 0;

    void _createRoleComboBox();
    void _updateProgressBar(); // a dirty function used by the class itself only.
    void _updateDeathIcon();
    void _updateEquips();
    void _updatePrivatePilesGeometry();

    void _paintPixmap(QGraphicsPixmapItem *&item, const QRect &rect, const QString &key);
    void _paintPixmap(QGraphicsPixmapItem *&item, const QRect &rect, const QString &key, QGraphicsItem *parent);
    void _paintPixmap(QGraphicsPixmapItem *&item, const QRect &rect, const QPixmap &pixmap);
    void _clearPixmap(QGraphicsPixmapItem *item);
    QPixmap _getPixmap(const QString &key, bool cache = false);
    QPixmap _getPixmap(const QString &key, const QString &arg, bool cache = false);
    QPixmap _getEquipPixmap(const EquipCard *equip);
    virtual void _adjustComponentZValues();
    void _updateFloatingArea();

    // We use QList of cards instead of a single card as parameter here, just in case
    // we need to do group animation in the future.
    virtual void addEquips(QList<CardItem *> &equips);
    virtual QList<CardItem *> removeEquips(const QList<int> &cardIds);
    virtual void addDelayedTricks(QList<CardItem *> &judges);
    virtual QList<CardItem *> removeDelayedTricks(const QList<int> &cardIds);
    virtual void updateDelayedTricks();

    // This is a dirty but easy design, we require children class to call create controls after
    // everything specific to the children has been setup (such as the frames that we attach
    // the controls. Consider revise this in the future.
    void _createControls();
    void _layBetween(QGraphicsItem *middle, QGraphicsItem *item1, QGraphicsItem *item2);
    void _layUnder(QGraphicsItem *item);

    bool isUseFullSkin() const {
        return QSanSkinFactory::getInstance().getCurrentSkinName().contains("full");
    }

    void _onMouseClicked(Qt::MouseButton button);

    void updatePhaseAppearance();

    // layout
    const QSanRoomSkin::PlayerCardContainerLayout *_m_layout;
    QGraphicsRectItem *_m_avatarArea, *_m_smallAvatarArea;

    // icons;
    // painting large shadowtext every frame is very costly, so we use a
    // graphicsitem to cache the result
    QGraphicsPixmapItem *_m_avatarNameItem, *_m_smallAvatarNameItem;
    GraphicsPixmapHoverItem *_m_avatarIcon, *_m_smallAvatarIcon;
    QGraphicsPixmapItem *_m_circleItem;

    QGraphicsPixmapItem *_m_screenNameItem;
    QGraphicsPixmapItem *_m_chainIcon, *_m_faceTurnedIcon;
    QGraphicsPixmapItem *_m_handCardBg, *_m_handCardNumText;
    QGraphicsPixmapItem *_m_kingdomColorMaskIcon;
    QGraphicsPixmapItem *_m_deathIcon;
    QGraphicsPixmapItem *_m_readyIcon, *_m_actionIcon;
    QGraphicsPixmapItem *_m_kingdomIcon;
    QGraphicsPixmapItem *_m_saveMeIcon;
    QGraphicsPixmapItem *_m_phaseIcon;
    QGraphicsPixmapItem *_m_extraSkillBg;
    QGraphicsPixmapItem *_m_extraSkillText;
    QGraphicsTextItem *_m_markItem;
    QGraphicsPixmapItem *_m_selectedFrame;
    QMap<QString, QGraphicsProxyWidget *> _m_privatePiles;

    // The frame that is maintained by roomscene. Items in this area has positions
    // or contents that cannot be decided based on the information of PlayerCardContainer
    // alone. It is relative to other components in the roomscene. One use case is
    // phase area of dashboard;
    QRect _m_floatingAreaRect;
    QGraphicsPixmapItem *_m_floatingArea;

    QList<QGraphicsPixmapItem *> _m_judgeIcons;
    QList<CardItem *> _m_judgeCards;

    QGraphicsProxyWidget *_m_equipRegions[S_EQUIP_AREA_LENGTH];
    CardItem *_m_equipCards[S_EQUIP_AREA_LENGTH];
    QLabel *_m_equipLabel[S_EQUIP_AREA_LENGTH];
    QParallelAnimationGroup *_m_equipAnim[S_EQUIP_AREA_LENGTH];
    QMutex _mutexEquipAnim;

    // controls
    MagatamasBoxItem *_m_hpBox;
    RoleComboBox *_m_roleComboBox;
    QSanCommandProgressBar *_m_progressBar;
    QGraphicsProxyWidget *_m_progressBarItem;

    // in order to apply different graphics effect;
    QGraphicsPixmapItem *_m_groupMain;
    QGraphicsPixmapItem *_m_groupDeath;

    // now, logic
    ClientPlayer *m_player;

    // The following stuffs for mulitple votes required for yeyan
    int _m_votesGot, _m_maxVotes;
    QGraphicsPixmapItem *_m_votesItem;

    // The following stuffs for showing distance
    QGraphicsPixmapItem *_m_distanceItem;

    // animations
    QAbstractAnimation *_m_huashenAnimation;
    GraphicsWidgetHoverItem *_m_huashenItem;
    QString _m_huashenGeneralName;
    QString _m_huashenSkillName;

    QGraphicsPixmapItem *_m_skillNameItem;

    bool _m_mouse_doubleclicked;

    QGraphicsItem *m_doubleClickedItem;

    QSanButton *m_changePrimaryHeroSKinBtn;
    QSanButton *m_changeSecondaryHeroSkinBtn;
    HeroSkinContainer *m_primaryHeroSkinContainer;
    HeroSkinContainer *m_secondaryHeroSkinContainer;

protected slots:
    virtual void _onEquipSelectChanged();

    void showHeroSkinList();
    void heroSkinBtnMouseOutsideClicked();

    virtual void onAvatarHoverEnter();
    virtual void onAvatarHoverLeave();
    virtual void onSkinChangingStart();
    virtual void onSkinChangingFinished();

    virtual void doAvatarHoverLeave() {}
    virtual bool isItemUnderMouse(QGraphicsItem *item) const {
        return item->isUnderMouse();
    }

private:
    bool _startLaying();
    void clearVotes();
    int _lastZ;
    bool _allZAdjusted;

    QString m_treasureName;

    void attemptChangeTargetUseCard();

    void showHeroSkinListHelper(const General *general,
        GraphicsPixmapHoverItem *avatarIcon,
        HeroSkinContainer *&heroSkinContainer);

signals:
    void selected_changed();
    void enable_changed();
    void add_equip_skill(const Skill *skill, bool from_left);
    void remove_equip_skill(const QString &skill_name);
    void card_to_use();
};

#endif
