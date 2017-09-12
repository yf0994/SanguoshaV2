#include "sp.h"
#include "client.h"
#include "general.h"
#include "skill.h"
#include "standard-skillcards.h"
#include "engine.h"
#include "maneuvering.h"
#include "settings.h"
#include "clientstruct.h"
#include "yjcm2013.h"
#include "package.h"
#include "card.h"
#include "standard.h"
#include "ai.h"
#include "clientplayer.h"
#include "assassins.h"
#include "special3v3.h"
#include "special3v3.h"
#include "skill.h"
#include "standard.h"
#include "engine.h"
#include "ai.h"
#include "maneuvering.h"
#include "clientplayer.h"

class SPMoonSpearSkill: public WeaponSkill {
public:
    SPMoonSpearSkill(): WeaponSkill("sp_moonspear") {
        events << CardUsed << CardResponded;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() != Player::NotActive)
            return false;

        const Card *card = NULL;
        if (triggerEvent == CardUsed) {
            CardUseStruct card_use = data.value<CardUseStruct>();
            card = card_use.card;
        } else if (triggerEvent == CardResponded) {
            card = data.value<CardResponseStruct>().m_card;
        }

        if (card == NULL || !card->isBlack()
            || (card->getHandlingMethod() != Card::MethodUse && card->getHandlingMethod() != Card::MethodResponse))
            return false;

        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *tmp, room->getAlivePlayers()) {
            if (player->inMyAttackRange(tmp))
                targets << tmp;
        }
        if (targets.isEmpty()) return false;

        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@sp_moonspear", true, true);
        if (!target) return false;
        room->setEmotion(player, "weapon/moonspear");
        const Card *jink = room->askForCard(target, "jink", "@moon-spear-jink", QVariant(), Card::MethodResponse, player);
        if (!jink || (jink && jink->hasFlag("response_failed"))) {
			if (jink && jink->hasFlag("response_failed")) {
                room->setCardFlag(jink, "-response_failed");
            }
            room->damage(DamageStruct(objectName(), player, target));
        }
        return false;
    }
};

SPMoonSpear::SPMoonSpear(Suit suit, int number)
    : Weapon(suit, number, 3)
{
    setObjectName("sp_moonspear");
}

class Jilei: public TriggerSkill {
public:
    Jilei(): TriggerSkill("jilei") {
        events << Damaged;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *current = room->getCurrent();
        if (!current || current->getPhase() == Player::NotActive || current->isDead() || !damage.from)
            return false;

        if (room->askForSkillInvoke(player, objectName(), data)) {
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.from->objectName());
            room->broadcastSkillInvoke(objectName());
            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                QString choice = room->askForChoice(player, objectName(), "BasicCard+EquipCard+TrickCard");

                LogMessage log;
                log.type = "#Jilei";
                log.from = damage.from;
                log.arg = choice;
                room->sendLog(log);

                QStringList jilei_list = damage.from->tag[objectName()].toStringList();
                if (jilei_list.contains(choice)) return false;
                jilei_list.append(choice);
                damage.from->tag[objectName()] = QVariant::fromValue(jilei_list);
                QString _type = choice + "|.|.|hand"; // Handcards only
                room->setPlayerCardLimitation(damage.from, "use,response,discard", _type, true);

                QString type_name = choice.replace("Card", "").toLower();
                if (damage.from->getMark("@jilei_" + type_name) == 0)
                    room->addPlayerMark(damage.from, "@jilei_" + type_name);
                room->removePlayerMark(player, objectName()+"engine");
            }
        }
        return false;
    }
};

class JileiClear: public TriggerSkill {
public:
    JileiClear(): TriggerSkill("#jilei-clear") {
        events << EventPhaseChanging << Death;
    }

    virtual int getPriority(TriggerEvent) const{
        return 5;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != target || target != room->getCurrent())
                return false;
        }
        QList<ServerPlayer *> players = room->getAllPlayers();
        foreach (ServerPlayer *player, players) {
            QStringList jilei_list = player->tag["jilei"].toStringList();
            if (!jilei_list.isEmpty()) {
                LogMessage log;
                log.type = "#JileiClear";
                log.from = player;
                room->sendLog(log);

                foreach (QString jilei_type, jilei_list) {
                    room->removePlayerCardLimitation(player, "use,response,discard", jilei_type + "|.|.|hand$1");
                    QString type_name = jilei_type.replace("Card", "").toLower();
                    room->setPlayerMark(player, "@jilei_" + type_name, 0);
                }
                player->tag.remove("jilei");
            }
        }

        return false;
    }
};

class Danlao: public TriggerSkill {
public:
    Danlao(): TriggerSkill("danlao") {
        events << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.to.length() <= 1 || !use.to.contains(player)
            || !use.card->isKindOf("TrickCard")
            || !room->askForSkillInvoke(player, objectName(), data))
            return false;

        room->broadcastSkillInvoke(objectName());
        player->setFlags("-DanlaoTarget");
        player->setFlags("DanlaoTarget");
        room->addPlayerMark(player, objectName()+"engine");
        if (player->getMark(objectName()+"engine") > 0) {
            player->drawCards(1, objectName());
            if (player->isAlive() && player->hasFlag("DanlaoTarget")) {
                player->setFlags("-DanlaoTarget");
                use.nullified_list << player->objectName();
                data = QVariant::fromValue(use);
            }
            room->removePlayerMark(player, objectName()+"engine");
        }
        return false;
    }
};

Yongsi::Yongsi(): TriggerSkill("yongsi") {
    events << DrawNCards << EventPhaseStart;
    frequency = Compulsory;
}

int Yongsi::getKingdoms(ServerPlayer *yuanshu) const{
    QSet<QString> kingdom_set;
    Room *room = yuanshu->getRoom();
    foreach (ServerPlayer *p, room->getAlivePlayers())
        kingdom_set << p->getKingdom();

    return kingdom_set.size();
}

bool Yongsi::trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
    if (triggerEvent == DrawNCards) {
        int x = getKingdoms(player);

        LogMessage log;
        log.type = "#YongsiGood";
        log.from = player;
        log.arg = QString::number(x);
        log.arg2 = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(player, objectName());

        room->broadcastSkillInvoke("yongsi", 1);
        room->addPlayerMark(player, objectName()+"engine");
        if (player->getMark(objectName()+"engine") > 0) {
            data = data.toInt() + x;
            room->removePlayerMark(player, objectName()+"engine");
        }
    } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Discard) {
        int x = getKingdoms(player);
        LogMessage log;
        log.type = player->getCardCount() > x ? "#YongsiBad" : "#YongsiWorst";
        log.from = player;
        log.arg = QString::number(log.type == "#YongsiBad" ? x : player->getCardCount());
        log.arg2 = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(player, objectName());

        room->broadcastSkillInvoke("yongsi", 2);

        if (x > 0){
            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                room->askForDiscard(player, "yongsi", x, x, false, true);
                room->removePlayerMark(player, objectName()+"engine");
            }
		}
    }

    return false;
}

class WeidiViewAsSkill: public ViewAsSkill {
public:
    WeidiViewAsSkill(): ViewAsSkill("weidi") {
    }

    static QList<const ViewAsSkill *> getLordViewAsSkills(const Player *player) {
        const Player *lord = NULL;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->isLord()) {
                lord = p;
                break;
            }
        }
        if (!lord) return QList<const ViewAsSkill *>();

        QList<const ViewAsSkill *> vs_skills;
        foreach (const Skill *skill, lord->getVisibleSkillList()) {
            if (skill->isLordSkill() && player->hasLordSkill(skill->objectName())) {
                const ViewAsSkill *vs = ViewAsSkill::parseViewAsSkill(skill);
                if (vs)
                    vs_skills << vs;
            }
        }
        return vs_skills;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        QList<const ViewAsSkill *> vs_skills = getLordViewAsSkills(player);
        foreach (const ViewAsSkill *skill, vs_skills) {
            if (skill->isEnabledAtPlay(player))
                return true;
        }
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        QList<const ViewAsSkill *> vs_skills = getLordViewAsSkills(player);
        foreach (const ViewAsSkill *skill, vs_skills) {
            if (skill->isEnabledAtResponse(player, pattern))
                return true;
        }
        return false;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        QList<const ViewAsSkill *> vs_skills = getLordViewAsSkills(player);
        foreach (const ViewAsSkill *skill, vs_skills) {
            if (skill->isEnabledAtNullification(player))
                return true;
        }
        return false;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        QString skill_name = Self->tag["weidi"].toString();
        if (skill_name.isEmpty()) return false;
        const ViewAsSkill *vs_skill = Sanguosha->getViewAsSkill(skill_name);
        if (vs_skill) return vs_skill->viewFilter(selected, to_select);
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        QString skill_name = Self->tag["weidi"].toString();
        if (skill_name.isEmpty()) return NULL;
        const ViewAsSkill *vs_skill = Sanguosha->getViewAsSkill(skill_name);
        if (vs_skill) return vs_skill->viewAs(cards);
        return NULL;
    }
};

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCommandLinkButton>

WeidiDialog *WeidiDialog::getInstance() {
    static WeidiDialog *instance;
    if (instance == NULL)
        instance = new WeidiDialog();

    return instance;
}

WeidiDialog::WeidiDialog() {
    setObjectName("weidi");
    setWindowTitle(Sanguosha->translate("weidi"));
    group = new QButtonGroup(this);

    button_layout = new QVBoxLayout;
    setLayout(button_layout);
    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectSkill(QAbstractButton *)));
}

void WeidiDialog::popup() {
    Self->tag.remove(objectName());
    foreach (QAbstractButton *button, group->buttons()) {
        button_layout->removeWidget(button);
        group->removeButton(button);
        delete button;
    }

    QList<const ViewAsSkill *> vs_skills = WeidiViewAsSkill::getLordViewAsSkills(Self);
    int count = 0;
    QString name;
    foreach (const ViewAsSkill *skill, vs_skills) {
        QAbstractButton *button = createSkillButton(skill->objectName());
        button->setEnabled(skill->isAvailable(Self, Sanguosha->getCurrentCardUseReason(),
            Sanguosha->getCurrentCardUsePattern()));
        if (button->isEnabled()) {
            ++count;
            name = skill->objectName();
        }
        button_layout->addWidget(button);
    }

    if (count == 0) {
        emit onButtonClick();
        return;
    } else if (count == 1) {
        Self->tag[objectName()] = name;
        emit onButtonClick();
        return;
    }

    exec();
}

void WeidiDialog::selectSkill(QAbstractButton *button) {
    Self->tag[objectName()] = button->objectName();
    emit onButtonClick();
    accept();
}

QAbstractButton *WeidiDialog::createSkillButton(const QString &skill_name) {
    const Skill *skill = Sanguosha->getSkill(skill_name);
    if (!skill) return NULL;

    QCommandLinkButton *button = new QCommandLinkButton(Sanguosha->translate(skill_name));
    button->setObjectName(skill_name);
    button->setToolTip(skill->getDescription());

    group->addButton(button);
    return button;
}

class Weidi: public GameStartSkill {
public:
    Weidi(): GameStartSkill("weidi") {
        frequency = Compulsory;
        view_as_skill = new WeidiViewAsSkill;
    }

    virtual void onGameStart(ServerPlayer *) const{
        return;
    }

    virtual QDialog *getDialog() const{
        return WeidiDialog::getInstance();
    }
};

class Yicong: public DistanceSkill {
public:
    Yicong(): DistanceSkill("yicong") {
    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        int correct = 0;
        if (from->hasSkill(objectName()) && from->getHp() > 2)
            --correct;
        if (to->hasSkill(objectName()) && to->getHp() <= 2)
            ++correct;

        return correct;
    }
};

class YicongEffect: public TriggerSkill {
public:
    YicongEffect(): TriggerSkill("#yicong-effect") {
        events << HpChanged;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        int hp = player->getHp();
        int index = 0;
        int reduce = 0;
        if (data.canConvert<RecoverStruct>()) {
            int rec = data.value<RecoverStruct>().recover;
            if (hp > 2 && hp - rec <= 2)
                index = 1;
        } else {
            if (data.canConvert<DamageStruct>()) {
                DamageStruct damage = data.value<DamageStruct>();
                reduce = damage.damage;
            } else if (!data.isNull()) {
                reduce = data.toInt();
            }
            if (hp <= 2 && hp + reduce > 2)
                index = 2;
        }

        if (index > 0) {
			if (player->getGeneralName() == "gongsunzan" || player->getGeneral2Name() == "gongsunzan")
                index += 2;
            room->broadcastSkillInvoke("yicong", index);
		}
        return false;
    }
};

class Danji: public PhaseChangeSkill {
public:
    Danji(): PhaseChangeSkill("danji") { // What a silly skill!
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Start
               && target->getMark("danji") == 0
               && target->getHandcardNum() > target->getHp();
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        ServerPlayer *the_lord = room->getLord();
        if (the_lord && (the_lord->getGeneralName().contains("caocao") || the_lord->getGeneral2Name().contains("caocao"))) {
            room->notifySkillInvoked(player, objectName());

            LogMessage log;
            log.type = "#DanjiWake";
            log.from = player;
            log.arg = QString::number(player->getHandcardNum());
            log.arg2 = QString::number(player->getHp());
            room->sendLog(log);
            room->broadcastSkillInvoke(objectName());
            //room->doLightbox("$DanjiAnimate", 5000);

            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                room->setPlayerMark(player, "danji", 1);
                if (room->changeMaxHpForAwakenSkill(player) && player->getMark("danji") == 1)
                    room->acquireSkill(player, "mashu");
                room->removePlayerMark(player, objectName()+"engine");
            }
        }

        return false;
    }
};

YuanhuCard::YuanhuCard() {
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool YuanhuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    if (!targets.isEmpty())
        return false;

    const Card *card = Sanguosha->getCard(subcards.first());
    const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
    int equip_index = static_cast<int>(equip->location());
    return to_select->getEquip(equip_index) == NULL;
}

void YuanhuCard::onUse(Room *room, const CardUseStruct &card_use) const{
    int index = -1;
    if (card_use.to.first() == card_use.from)
        index = 5;
    else if (card_use.to.first()->getGeneralName().contains("caocao"))
        index = 4;
    else {
        const Card *card = Sanguosha->getCard(card_use.card->getSubcards().first());
        if (card->isKindOf("Weapon"))
            index = 1;
        else if (card->isKindOf("Armor"))
            index = 2;
        else
            index = 3;
    }
    room->broadcastSkillInvoke("yuanhu", index);
    room->addPlayerMark(card_use.from, "yuanhuengine");
    if (card_use.from->getMark("yuanhuengine") > 0) {
        SkillCard::onUse(room, card_use);
        room->removePlayerMark(card_use.from, "yuanhuengine");
    }
}

void YuanhuCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *caohong = effect.from;
    Room *room = caohong->getRoom();
    room->moveCardTo(this, caohong, effect.to, Player::PlaceEquip,
                     CardMoveReason(CardMoveReason::S_REASON_PUT, caohong->objectName(), "yuanhu", QString()));

    const Card *card = Sanguosha->getCard(subcards.first());

    LogMessage log;
    log.type = "$ZhijianEquip";
    log.from = effect.to;
    log.card_str = QString::number(card->getEffectiveId());
    room->sendLog(log);

    if (card->isKindOf("Weapon")) {
      QList<ServerPlayer *> targets;
      foreach (ServerPlayer *p, room->getAllPlayers()) {
          if (effect.to->distanceTo(p) == 1 && caohong->canDiscard(p, "hej"))
              targets << p;
      }
      if (!targets.isEmpty()) {
          ServerPlayer *to_dismantle = room->askForPlayerChosen(caohong, targets, "yuanhu", "@yuanhu-discard:" + effect.to->objectName());
          int card_id = room->askForCardChosen(caohong, to_dismantle, "hej", "yuanhu", false, Card::MethodDiscard);
          room->throwCard(Sanguosha->getCard(card_id), to_dismantle, caohong);
      }
    } else if (card->isKindOf("Armor")) {
        effect.to->drawCards(1, "yuanhu");
    } else if (card->isKindOf("Horse")) {
        room->recover(effect.to, RecoverStruct(effect.from));
    }
}

class YuanhuViewAsSkill: public OneCardViewAsSkill {
public:
    YuanhuViewAsSkill(): OneCardViewAsSkill("yuanhu") {
        filter_pattern = "EquipCard";
        response_pattern = "@@yuanhu";
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        YuanhuCard *first = new YuanhuCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class Yuanhu: public PhaseChangeSkill {
public:
    Yuanhu(): PhaseChangeSkill("yuanhu") {
        view_as_skill = new YuanhuViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        if (target->getPhase() == Player::Finish && !target->isNude())
            room->askForUseCard(target, "@@yuanhu", "@yuanhu-equip", -1, Card::MethodNone);
        return false;
    }
};

XuejiCard::XuejiCard() {
}

bool XuejiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (targets.length() >= Self->getLostHp())
        return false;

    if (to_select == Self)
        return false;

    int range_fix = 0;
    if (Self->getWeapon() && Self->getWeapon()->getEffectiveId() == getEffectiveId()) {
        const Weapon *weapon = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        range_fix += weapon->getRange() - Self->getAttackRange(false);
    } else if (Self->getOffensiveHorse() && Self->getOffensiveHorse()->getEffectiveId() == getEffectiveId())
        range_fix += 1;

    return Self->inMyAttackRange(to_select, range_fix);
}

void XuejiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->addPlayerMark(source, "xuejiengine");
    if (source->getMark("xuejiengine") > 0) {
        DamageStruct damage;
        damage.from = source;
        damage.reason = "xueji";

        foreach (ServerPlayer *p, targets) {
            damage.to = p;
            room->damage(damage);
        }
        foreach (ServerPlayer *p, targets) {
            if (p->isAlive())
                p->drawCards(1, "xueji");
        }
        room->removePlayerMark(source, "xuejiengine");
    }
}

class Xueji: public OneCardViewAsSkill {
public:
    Xueji(): OneCardViewAsSkill("xueji") {
        filter_pattern = ".|red!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getLostHp() > 0 && player->canDiscard(player, "he") && !player->hasUsed("XuejiCard");
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        XuejiCard *first = new XuejiCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class Huxiao: public TargetModSkill {
public:
    Huxiao(): TargetModSkill("huxiao") {
        frequency = NotCompulsory;
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()))
            return from->getMark(objectName());
        else
            return 0;
    }
};

class HuxiaoCount: public TriggerSkill {
public:
    HuxiaoCount(): TriggerSkill("#huxiao-count") {
        events << SlashMissed << EventPhaseChanging;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == SlashMissed) {
            if (player->getPhase() == Player::Play)
                room->addPlayerMark(player, "huxiao");
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play)
                if (player->getMark("huxiao") > 0)
                    room->setPlayerMark(player, "huxiao", 0);
        }

        return false;
    }
};

class HuxiaoClear: public DetachEffectSkill {
public:
    HuxiaoClear(): DetachEffectSkill("huxiao") {
    }

    virtual void onSkillDetached(Room *room, ServerPlayer *player) const{
        room->setPlayerMark(player, "huxiao", 0);
    }
};



class Wuji: public PhaseChangeSkill {
public:
    Wuji(): PhaseChangeSkill("wuji") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Finish
               && target->getMark("wuji") == 0
               && target->getMark("damage_point_round") >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        room->notifySkillInvoked(player, objectName());

        LogMessage log;
        log.type = "#WujiWake";
        log.from = player;
        log.arg = QString::number(player->getMark("damage_point_round"));
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());
        //room->doLightbox("$WujiAnimate", 4000);

        room->addPlayerMark(player, objectName()+"engine");
        if (player->getMark(objectName()+"engine") > 0) {
            room->setPlayerMark(player, "wuji", 1);
            if (room->changeMaxHpForAwakenSkill(player, 1)) {
                room->recover(player, RecoverStruct(player));
                if (player->getMark("wuji") == 1)
                    room->detachSkillFromPlayer(player, "huxiao");
            }

            room->removePlayerMark(player, objectName()+"engine");
        }

        return false;
    }
};

class Baobian: public TriggerSkill {
public:
    Baobian(): TriggerSkill("baobian") {
        events << GameStart << HpChanged << MaxHpChanged << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventLoseSkill) {
            if (data.toString() == objectName()) {
                QStringList baobian_skills = player->tag["BaobianSkills"].toStringList();
                QStringList detachList;
                foreach (QString skill_name, baobian_skills)
                    detachList.append("-" + skill_name);
                room->handleAcquireDetachSkills(player, detachList);
                player->tag["BaobianSkills"] = QVariant();
            }
            return false;
        } else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() != objectName()) return false;
        }

        if (!player->isAlive() || !player->hasSkill(objectName(), true)) return false;

        acquired_skills.clear();
        detached_skills.clear();
        BaobianChange(room, player, 1, "shensu");
        BaobianChange(room, player, 2, "paoxiao");
        BaobianChange(room, player, 3, "tiaoxin");
        if (!acquired_skills.isEmpty() || !detached_skills.isEmpty())
            room->handleAcquireDetachSkills(player, acquired_skills + detached_skills);
        return false;
    }

private:
    void BaobianChange(Room *room, ServerPlayer *player, int hp, const QString &skill_name) const{
        QStringList baobian_skills = player->tag["BaobianSkills"].toStringList();
        if (player->getHp() <= hp) {
            if (!baobian_skills.contains(skill_name)) {
                room->notifySkillInvoked(player, "baobian");
                if (player->getHp() == hp)
                    room->broadcastSkillInvoke("baobian", 4 - hp);
                acquired_skills.append(skill_name);
                baobian_skills << skill_name;
            }
        } else {
            if (baobian_skills.contains(skill_name)) {
                detached_skills.append("-" + skill_name);
                baobian_skills.removeOne(skill_name);
            }
        }
        player->tag["BaobianSkills"] = QVariant::fromValue(baobian_skills);
    }

    mutable QStringList acquired_skills, detached_skills;
};

BifaCard::BifaCard() {
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool BifaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->getPile("pen").isEmpty() && to_select != Self;
}

void BifaCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->broadcastSkillInvoke("bifa", 1);
    room->addPlayerMark(source, "bifaengine");
    if (source->getMark("bifaengine") > 0) {
        ServerPlayer *target = targets.first();
        target->tag["BifaSource" + QString::number(getEffectiveId())] = QVariant::fromValue(source);
        target->addToPile("pen", this, false);
        room->removePlayerMark(source, "bifaengine");
    }
}

class BifaViewAsSkill: public OneCardViewAsSkill {
public:
    BifaViewAsSkill(): OneCardViewAsSkill("bifa") {
        filter_pattern = ".";
        response_pattern = "@@bifa";
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        Card *card = new BifaCard;
        card->addSubcard(originalcard);
        return card;
    }
};

class Bifa: public TriggerSkill {
public:
    Bifa(): TriggerSkill("bifa") {
        events << EventPhaseStart;
        view_as_skill = new BifaViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish && !player->isKongcheng()) {
            room->askForUseCard(player, "@@bifa", "@bifa-remove", -1, Card::MethodNone);
        } else if (player->getPhase() == Player::RoundStart && player->getPile("pen").length() > 0) {
            int card_id = player->getPile("pen").first();
            ServerPlayer *chenlin = player->tag["BifaSource" + QString::number(card_id)].value<ServerPlayer *>();
            QList<int> ids;
            ids << card_id;

            LogMessage log;
            log.type = "$BifaView";
            log.from = player;
            log.card_str = QString::number(card_id);
            log.arg = "bifa";
            room->sendLog(log, player);

            room->fillAG(ids, player);
            const Card *cd = Sanguosha->getCard(card_id);
            QString pattern;
            if (cd->isKindOf("BasicCard"))
                pattern = "BasicCard";
            else if (cd->isKindOf("TrickCard"))
                pattern = "TrickCard";
            else if (cd->isKindOf("EquipCard"))
                pattern = "EquipCard";
            QVariant data_for_ai = QVariant::fromValue(pattern);
            pattern.append("|.|.|hand");
            const Card *to_give = NULL;
            if (!player->isKongcheng() && chenlin && chenlin->isAlive())
                to_give = room->askForCard(player, pattern, "@bifa-give", data_for_ai, Card::MethodNone, chenlin);
            if (chenlin && to_give) {
                chenlin->obtainCard(to_give, false, CardMoveReason::S_REASON_GIVE);
                CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, player->objectName(), "bifa", QString());
                room->obtainCard(player, cd, reason, false);
            } else {
                room->broadcastSkillInvoke(objectName(), 2);
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
                room->throwCard(cd, reason, NULL);
                room->loseHp(player);
            }
            room->clearAG(player);
            player->tag.remove("BifaSource" + QString::number(card_id));
        }
        return false;
    }
};

SongciCard::SongciCard() {
    mute = true;
}

bool SongciCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->getMark("songci" + Self->objectName()) == 0 && to_select->getHandcardNum() != to_select->getHp();
}

void SongciCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->addPlayerMark(effect.from, "songciengine");
    if (effect.from->getMark("songciengine") > 0) {
        int handcard_num = effect.to->getHandcardNum();
        int hp = effect.to->getHp();
        effect.to->gainMark("@songci");
        room->addPlayerMark(effect.to, "songci" + effect.from->objectName());
        if (handcard_num > hp) {
            room->broadcastSkillInvoke("songci", 2);
            room->askForDiscard(effect.to, "songci", 2, 2, false, true);
        } else if (handcard_num < hp) {
            room->broadcastSkillInvoke("songci", 1);
            effect.to->drawCards(2, "songci");
        }
        room->removePlayerMark(effect.from, "songciengine");
    }
}

class SongciViewAsSkill: public ZeroCardViewAsSkill {
public:
    SongciViewAsSkill(): ZeroCardViewAsSkill("songci") {
    }

    virtual const Card *viewAs() const{
        return new SongciCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if (player->getMark("songci" + player->objectName()) == 0 && player->getHandcardNum() != player->getHp()) return true;
        foreach (const Player *sib, player->getAliveSiblings())
            if (sib->getMark("songci" + player->objectName()) == 0 && sib->getHandcardNum() != sib->getHp())
                return true;
        return false;
    }
};

class Songci: public TriggerSkill {
public:
    Songci(): TriggerSkill("songci") {
        events << Death;
        view_as_skill = new SongciViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player) return false;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("@songci") > 0)
                room->setPlayerMark(p, "@songci", 0);
            if (p->getMark("songci" + player->objectName()) > 0)
                room->setPlayerMark(p, "songci" + player->objectName(), 0);
        }
        return false;
    }
};

class Xiuluo: public PhaseChangeSkill {
public:
    Xiuluo(): PhaseChangeSkill("xiuluo") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Start
               && target->canDiscard(target, "h")
               && hasDelayedTrick(target);
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        while (hasDelayedTrick(target) && target->canDiscard(target, "h")) {
            QStringList suits;
            foreach (const Card *jcard, target->getJudgingArea()) {
                if (!suits.contains(jcard->getSuitString()))
                    suits << jcard->getSuitString();
            }

            const Card *card = room->askForCard(target, QString(".|%1|.|hand").arg(suits.join(",")),
                                                "@xiuluo", QVariant(), objectName());
            if (!card || !hasDelayedTrick(target)) break;
            room->broadcastSkillInvoke(objectName());

            QList<int> avail_list, other_list;
            foreach (const Card *jcard, target->getJudgingArea()) {
                if (jcard->isKindOf("SkillCard")) continue;
                if (jcard->getSuit() == card->getSuit())
                    avail_list << jcard->getEffectiveId();
                else
                    other_list << jcard->getEffectiveId();
            }
            room->fillAG(avail_list + other_list, NULL, other_list);
            int id = room->askForAG(target, avail_list, false, objectName());
            room->clearAG();
            room->addPlayerMark(target, objectName()+"engine");
            if (target->getMark(objectName()+"engine") > 0) {
                room->throwCard(id, NULL);
                room->removePlayerMark(target, objectName()+"engine");
            }
        }

        return false;
    }

private:
    static bool hasDelayedTrick(const ServerPlayer *target) {
        foreach (const Card *card, target->getJudgingArea())
            if (!card->isKindOf("SkillCard")) return true;
        return false;
    }
};

class Shenwei: public DrawCardsSkill {
public:
    Shenwei(): DrawCardsSkill("#shenwei-draw") {
        frequency = Compulsory;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        room->broadcastSkillInvoke("shenwei");
        room->sendCompulsoryTriggerLog(player, "shenwei");

        room->addPlayerMark(player, objectName()+"engine");
        if (player->getMark(objectName()+"engine") > 0) {
            room->removePlayerMark(player, objectName()+"engine");
            return n + 2;
        }
        return n ;
    }
};

class ShenweiKeep: public MaxCardsSkill {
public:
    ShenweiKeep(): MaxCardsSkill("shenwei") {
        frequency = NotCompulsory;
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill(objectName()))
            return 2;
        else
            return 0;
    }
};

class Shenji: public TargetModSkill {
public:
    Shenji(): TargetModSkill("shenji") {
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()) && from->getWeapon() == NULL)
            return 2;
        else
            return 0;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const {
        int index = qrand() % 2 + 1;
        if (!player->hasInnateSkill(objectName()) && player->hasSkill("zhanshen")) {
            index += 2;
        }
        return index;
    }
};

class Xingwu: public TriggerSkill {
public:
    Xingwu(): TriggerSkill("xingwu") {
        events << PreCardUsed << CardResponded << EventPhaseStart << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == PreCardUsed || triggerEvent == CardResponded) {
            const Card *card = NULL;
            if (triggerEvent == PreCardUsed)
                card = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                if (response.m_isUse)
                   card = response.m_card;
            }
            if (card && card->getTypeId() != Card::TypeSkill && card->getHandlingMethod() == Card::MethodUse) {
                int n = player->getMark(objectName());
                if (card->isBlack())
                    n |= 1;
                else if (card->isRed())
                    n |= 2;
                player->setMark(objectName(), n);
            }
        } else if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Discard) {
                int n = player->getMark(objectName());
                bool red_avail = ((n & 2) == 0), black_avail = ((n & 1) == 0);
                if (player->isKongcheng() || (!red_avail && !black_avail))
                    return false;
                QString pattern = ".|.|.|hand";
                if (red_avail != black_avail)
                    pattern = QString(".|%1|.|hand").arg(red_avail ? "red" : "black");
                const Card *card = room->askForCard(player, pattern, "@xingwu", QVariant(), Card::MethodNone);
                if (card) {
                    room->notifySkillInvoked(player, objectName());
                    room->broadcastSkillInvoke(objectName(), 1);

                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = player;
                    log.arg = objectName();
                    room->sendLog(log);

                    room->addPlayerMark(player, "xingwuengine");
                    if (player->getMark("xingwuengine") > 0) {
                        player->addToPile("dance", card);
                        room->removePlayerMark(player, "xingwuengine");
                    }
                }
            } else if (player->getPhase() == Player::RoundStart) {
                player->setMark(objectName(), 0);
            }
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to == player && move.to_place == Player::PlaceSpecial && player->getPile("dance").length() >= 3) {
                player->clearOnePrivatePile("dance");
                QList<ServerPlayer *> males;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->isMale())
                        males << p;
                }
                if (males.isEmpty()) return false;

                ServerPlayer *target = room->askForPlayerChosen(player, males, objectName(), "@xingwu-choose", false, true);
                room->broadcastSkillInvoke(objectName(), 2);
                room->damage(DamageStruct(objectName(), player, target, 2));

                if (!player->isAlive()) return false;
                QList<const Card *> equips = target->getEquips();
                if (!equips.isEmpty()) {
                    DummyCard *dummy = new DummyCard;
                    foreach (const Card *equip, equips) {
                        if (player->canDiscard(target, equip->getEffectiveId()))
                            dummy->addSubcard(equip);
                    }
                    if (dummy->subcardsLength() > 0)
                        room->throwCard(dummy, target, player);
                    delete dummy;
                }
            }
        }
        return false;
    }
};

class Luoyan: public TriggerSkill {
public:
    Luoyan(): TriggerSkill("luoyan") {
        events << CardsMoveOneTime << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventLoseSkill && data.toString() == objectName()) {
            room->handleAcquireDetachSkills(player, "-tianxiang|-liuli", true);
        } else if (triggerEvent == EventAcquireSkill && data.toString() == objectName()) {
            if (!player->getPile("dance").isEmpty()) {
                room->notifySkillInvoked(player, objectName());
                room->handleAcquireDetachSkills(player, "tianxiang|liuli");
            }
        } else if (triggerEvent == CardsMoveOneTime && player->isAlive() && player->hasSkill(objectName(), true)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to == player && move.to_place == Player::PlaceSpecial && move.to_pile_name == "dance") {
                if (player->getPile("dance").length() == 1) {
                    room->notifySkillInvoked(player, objectName());
                    room->handleAcquireDetachSkills(player, "tianxiang|liuli");
                }
            } else if (move.from == player && move.from_places.contains(Player::PlaceSpecial)
                       && move.from_pile_names.contains("dance")) {
                if (player->getPile("dance").isEmpty())
                    room->handleAcquireDetachSkills(player, "-tianxiang|-liuli", true);
            }
        }
        return false;
    }
};

class Yanyu: public TriggerSkill {
public:
    Yanyu(): TriggerSkill("yanyu") {
        events << EventPhaseStart << CardsMoveOneTime << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *xiahou = room->findPlayerBySkillName(objectName());
            if (xiahou && player->getPhase() == Player::Play) {
                if (!xiahou->canDiscard(xiahou, "he")) return false;
                const Card *card = room->askForCard(xiahou, "..", "@yanyu-discard", data, objectName());
                if (card) {
                    room->broadcastSkillInvoke(objectName(), 1);
                    room->addPlayerMark(player, objectName()+"engine");
                    if (player->getMark(objectName()+"engine") > 0) {
                        xiahou->addMark("YanyuDiscard" + QString::number(card->getTypeId()), 3);
                        room->removePlayerMark(player, objectName()+"engine");
                    }
				}
            }
        } else if (triggerEvent == CardsMoveOneTime && TriggerSkill::triggerable(player)) {
            ServerPlayer *current = room->getCurrent();
            if (!current || current->getPhase() != Player::Play) return false;
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to_place == Player::DiscardPile) {
                QList<int> ids, disabled;
                QList<int> all_ids = move.card_ids;
                foreach (int id, move.card_ids) {
                    const Card *card = Sanguosha->getCard(id);
                    if (player->getMark("YanyuDiscard" + QString::number(card->getTypeId())) > 0)
                        ids << id;
                    else
                        disabled << id;
                }
                if (ids.isEmpty()) return false;
                while (!ids.isEmpty()) {
                    room->fillAG(all_ids, player, disabled);
                    bool only = (all_ids.length() == 1);
                    int card_id = -1;
                    if (only)
                        card_id = ids.first();
                    else
                        card_id = room->askForAG(player, ids, true, objectName());
                    room->clearAG(player);
                    if (card_id == -1) break;
                    if (only)
                        player->setMark("YanyuOnlyId", card_id + 1); // For AI
                    const Card *card = Sanguosha->getCard(card_id);
                    ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(),
                                                                    QString("@yanyu-give:::%1:%2\\%3").arg(card->objectName())
                                                                                                      .arg(card->getSuitString() + "_char")
                                                                                                      .arg(card->getNumberString()),
                                                                    only, true);
                    player->setMark("YanyuOnlyId", 0);
                    if (target) {
						room->broadcastSkillInvoke(objectName(), 2);
                        player->removeMark("YanyuDiscard" + QString::number(card->getTypeId()));
                        Player::Place place = move.from_places.at(move.card_ids.indexOf(card_id));
                        QList<int> _card_id;
                        _card_id << card_id;
                        move.removeCardIds(_card_id);
                        data = QVariant::fromValue(move);
                        ids.removeOne(card_id);
                        disabled << card_id;
                        foreach (int id, ids) {
                            const Card *card = Sanguosha->getCard(id);
                            if (player->getMark("YanyuDiscard" + QString::number(card->getTypeId())) == 0) {
                                ids.removeOne(id);
                                disabled << id;
                            }
                        }
                        if (move.from && move.from->objectName() == target->objectName() && place != Player::PlaceTable) {
                            // just indicate which card she chose.
                            LogMessage log;
                            log.type = "$MoveCard";
                            log.from = target;
                            log.to << target;
                            log.card_str = QString::number(card_id);
                            room->sendLog(log);
                        }
                        target->obtainCard(card);
                    } else
                        break;
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    p->setMark("YanyuDiscard1", 0);
                    p->setMark("YanyuDiscard2", 0);
                    p->setMark("YanyuDiscard3", 0);
                }
            }
        }
        return false;
    }
};

class Xiaode: public TriggerSkill {
public:
    Xiaode(): TriggerSkill("xiaode") {
        events << BuryVictim;
    }

    virtual int getPriority(TriggerEvent) const{
        return -2;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &) const{
        ServerPlayer *xiahoushi = room->findPlayerBySkillName(objectName());
        if (!xiahoushi || !xiahoushi->tag["XiaodeSkill"].toString().isEmpty()) return false;
        QStringList skill_list = xiahoushi->tag["XiaodeVictimSkills"].toStringList();
        if (skill_list.isEmpty()) return false;
        if (!room->askForSkillInvoke(xiahoushi, objectName(), QVariant::fromValue(skill_list))) return false;
        room->broadcastSkillInvoke(objectName(), 1);
        room->addPlayerMark(xiahoushi, objectName()+"engine");
        if (xiahoushi->getMark(objectName()+"engine") > 0) {
			QString skill_name = room->askForChoice(xiahoushi, objectName(), skill_list.join("+"));
			xiahoushi->tag["XiaodeSkill"] = skill_name;
			room->acquireSkill(xiahoushi, skill_name);
            room->removePlayerMark(xiahoushi, objectName()+"engine");
        }
        return false;
    }
};

class XiaodeEx: public TriggerSkill {
public:
    XiaodeEx(): TriggerSkill("#xiaode") {
        events << EventPhaseChanging << EventLoseSkill << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                QString skill_name = player->tag["XiaodeSkill"].toString();
                if (!skill_name.isEmpty()) {
                    room->detachSkillFromPlayer(player, skill_name, false, true);
                    player->tag.remove("XiaodeSkill");
                }
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "xiaode") {
            QString skill_name = player->tag["XiaodeSkill"].toString();
            if (!skill_name.isEmpty()) {
                room->detachSkillFromPlayer(player, skill_name, false, true);
                player->tag.remove("XiaodeSkill");
            }
        } else if (triggerEvent == Death && TriggerSkill::triggerable(player)) {
            DeathStruct death = data.value<DeathStruct>();
            QStringList skill_list;
            skill_list.append(addSkillList(death.who->getGeneral()));
            skill_list.append(addSkillList(death.who->getGeneral2()));
            player->tag["XiaodeVictimSkills"] = QVariant::fromValue(skill_list);
        }
        return false;
    }

private:
    QStringList addSkillList(const General *general) const{
        if (!general) return QStringList();
        QStringList skill_list;
        foreach (const Skill *skill, general->getSkillList()) {
            if (skill->isVisible() && !skill->isLordSkill() && skill->getFrequency() != Skill::Wake)
                skill_list.append(skill->objectName());
        }
        return skill_list;
    }
};

ZhoufuCard::ZhoufuCard() {
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool ZhoufuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self && to_select->getPile("incantation").isEmpty();
}

void ZhoufuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->addPlayerMark(source, "zhoufuengine");
    if (source->getMark("zhoufuengine") > 0) {
        ServerPlayer *target = targets.first();
        target->tag["ZhoufuSource" + QString::number(getEffectiveId())] = QVariant::fromValue(source);
        target->addToPile("incantation", this);
        room->removePlayerMark(source, "zhoufuengine");
    }
}

class ZhoufuViewAsSkill: public OneCardViewAsSkill {
public:
    ZhoufuViewAsSkill(): OneCardViewAsSkill("zhoufu") {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ZhoufuCard");
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        Card *card = new ZhoufuCard;
        card->addSubcard(originalcard);
        return card;
    }
	
    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        return 2;
    }
};

class Zhoufu: public TriggerSkill {
public:
    Zhoufu(): TriggerSkill("zhoufu") {
        events << StartJudge << EventPhaseChanging;
        view_as_skill = new ZhoufuViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getPile("incantation").length() > 0;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == StartJudge) {
            int card_id = player->getPile("incantation").first();

            JudgeStruct *judge = data.value<JudgeStruct *>();
            judge->card = Sanguosha->getCard(card_id);

            LogMessage log;
            log.type = "$ZhoufuJudge";
            log.from = player;
            log.arg = objectName();
            log.card_str = QString::number(judge->card->getEffectiveId());
            room->sendLog(log);

            room->moveCardTo(judge->card, NULL, judge->who, Player::PlaceJudge,
                             CardMoveReason(CardMoveReason::S_REASON_JUDGE,
                             judge->who->objectName(),
                             QString(), QString(), judge->reason), true);
            judge->updateResult();
            room->setTag("SkipGameRule", true);
        } else {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                int id = player->getPile("incantation").first();
                ServerPlayer *zhangbao = player->tag["ZhoufuSource" + QString::number(id)].value<ServerPlayer *>();
                if (zhangbao && zhangbao->isAlive()) {
					room->addPlayerMark(zhangbao, objectName()+"engine");
					if (zhangbao->getMark(objectName()+"engine") > 0) {
						zhangbao->obtainCard(Sanguosha->getCard(id));
						room->broadcastSkillInvoke(objectName(), 1);
						room->removePlayerMark(zhangbao, objectName()+"engine");
					}
				}
            }
        }
        return false;
    }
};

class Yingbing: public TriggerSkill {
public:
    Yingbing(): TriggerSkill("yingbing") {
        events << StartJudge;
        frequency = Frequent;
    }

    virtual int getPriority(TriggerEvent) const{
        return -1;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        JudgeStruct *judge = data.value<JudgeStruct *>();
        int id = judge->card->getEffectiveId();
        ServerPlayer *zhangbao = player->tag["ZhoufuSource" + QString::number(id)].value<ServerPlayer *>();
        if (zhangbao && TriggerSkill::triggerable(zhangbao)
            && zhangbao->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            room->addPlayerMark(zhangbao, objectName()+"engine");
            if (zhangbao->getMark(objectName()+"engine") > 0) {
                zhangbao->drawCards(2, "yingbing");
                room->removePlayerMark(zhangbao, objectName()+"engine");
            }
        }
        return false;
    }
};

class Kangkai: public TriggerSkill {
public:
    Kangkai(): TriggerSkill("kangkai") {
        events << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            foreach (ServerPlayer *to, use.to) {
                if (!player->isAlive()) break;
                if (player->distanceTo(to) <= 1 && TriggerSkill::triggerable(player)) {
                    player->tag["KangkaiSlash"] = data;
                    bool will_use = room->askForSkillInvoke(player, objectName(), QVariant::fromValue(to));
                    player->tag.remove("KangkaiSlash");
                    if (!will_use) continue;

					room->broadcastSkillInvoke(objectName(), to == player ? 1 : 2);

                    room->addPlayerMark(player, objectName()+"engine");
                    if (player->getMark(objectName()+"engine") > 0) {

						player->drawCards(1, "kangkai");
						if (!player->isNude() && player != to) {
							const Card *card = NULL;
							if (player->getCardCount() > 1) {
								card = room->askForCard(player, "..!", "@kangkai-give:" + to->objectName(), data, Card::MethodNone);
								if (!card)
									card = player->getCards("he").at(qrand() % player->getCardCount());
							} else {
								Q_ASSERT(player->getCardCount() == 1);
								card = player->getCards("he").first();
							}
							to->obtainCard(card);
							if (card->getTypeId() == Card::TypeEquip && room->getCardOwner(card->getEffectiveId()) == to
								&& !to->isLocked(card)) {
								to->tag["KangkaiSlash"] = data;
								to->tag["KangkaiGivenCard"] = QVariant::fromValue(card);
								bool will_use = room->askForSkillInvoke(to, "kangkai_use", "use");
								to->tag.remove("KangkaiSlash");
								to->tag.remove("KangkaiGivenCard");
								if (will_use)
									room->useCard(CardUseStruct(card, to, to));
							}
						}
                        room->removePlayerMark(player, objectName()+"engine");
                    }
                }
            }
        }
        return false;
    }
};

class Shenxian: public TriggerSkill {
public:
    Shenxian(): TriggerSkill("shenxian") {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (player->getPhase() == Player::NotActive && move.from && move.from->isAlive()
            && move.from->objectName() != player->objectName()
            && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
            && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            foreach (int id, move.card_ids) {
                if (Sanguosha->getCard(id)->getTypeId() == Card::TypeBasic) {
                    if (room->askForSkillInvoke(player, objectName(), data)) {
                        room->broadcastSkillInvoke(objectName());
                        player->drawCards(1, "shenxian");
                    }
                    break;
                }
            }
        }
        return false;
    }
};

QiangwuCard::QiangwuCard() {
    target_fixed = true;
}

void QiangwuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->addPlayerMark(source, "qiangwuengine");
    if (source->getMark("qiangwuengine") > 0) {
		JudgeStruct judge;
		judge.pattern = ".";
		judge.who = source;
		judge.reason = "qiangwu";
		judge.play_animation = false;
		room->judge(judge);

		bool ok = false;
		int num = judge.pattern.toInt(&ok);
		if (ok)
			room->setPlayerMark(source, "qiangwu", num);
        room->removePlayerMark(source, "qiangwuengine");
    }
}

class QiangwuViewAsSkill: public ZeroCardViewAsSkill {
public:
    QiangwuViewAsSkill(): ZeroCardViewAsSkill("qiangwu") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("QiangwuCard");
    }

    virtual const Card *viewAs() const{
        return new QiangwuCard;
    }
};

class Qiangwu: public TriggerSkill {
public:
    Qiangwu(): TriggerSkill("qiangwu") {
        events << EventPhaseChanging << PreCardUsed << FinishJudge;
        view_as_skill = new QiangwuViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->setPlayerMark(player, "qiangwu", 0);
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == "qiangwu")
                judge->pattern = QString::number(judge->card->getNumber());
        } else if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && player->getMark("qiangwu") > 0
                && use.card->getNumber() > player->getMark("qiangwu")) {
                if (use.m_addHistory) {
                    room->addPlayerHistory(player, use.card->getClassName(), -1);
                    use.m_addHistory = false;
                    data = QVariant::fromValue(use);
                }
            }
        }
        return false;
    }
};

class QiangwuTargetMod: public TargetModSkill {
public:
    QiangwuTargetMod(): TargetModSkill("#qiangwu-target") {
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const{
        if (card->getNumber() < from->getMark("qiangwu"))
            return 1000;
        else
            return 0;
    }

    virtual int getResidueNum(const Player *from, const Card *card) const{
        if (from->getMark("qiangwu") > 0
            && (card->getNumber() > from->getMark("qiangwu") || card->hasFlag("Global_SlashAvailabilityChecker")))
            return 1000;
        else
            return 0;
    }
};

class Meibu: public TriggerSkill {
public:
    Meibu(): TriggerSkill("meibu") {
        events << EventPhaseStart << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play) {
            foreach (ServerPlayer *sunluyu, room->getOtherPlayers(player)) {
                if (!player->inMyAttackRange(sunluyu) && TriggerSkill::triggerable(sunluyu) && room->askForSkillInvoke(sunluyu, objectName())) {
                    room->broadcastSkillInvoke(objectName());
                    room->addPlayerMark(sunluyu, objectName()+"engine");
                    if (sunluyu->getMark(objectName()+"engine") > 0) {
                        if (!player->hasSkill("#meibu-filter", true)) {
                            room->acquireSkill(player, "#meibu-filter", false);
                            room->filterCards(player, player->getCards("he"), false);
                        }
                        QVariantList sunluyus = player->tag[objectName()].toList();
                        sunluyus << QVariant::fromValue(sunluyu);
                        player->tag[objectName()] = QVariant::fromValue(sunluyus);
                        room->insertAttackRangePair(player, sunluyu);
                        room->removePlayerMark(sunluyu, objectName()+"engine");
                    }
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive) return false;

            QVariantList sunluyus = player->tag[objectName()].toList();
            foreach (QVariant sunluyu, sunluyus) {
                ServerPlayer *s = sunluyu.value<ServerPlayer *>();
                room->removeAttackRangePair(player, s);
            }

            room->detachSkillFromPlayer(player, "#meibu-filter");

			 player->tag[objectName()] = QVariantList();

            room->filterCards(player, player->getCards("he"), true);
        }
        return false;
    }
	
    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("Slash"))
            return -2;
        
        return -1;
    }
};

class MeibuFilter: public FilterSkill {
public:
    MeibuFilter(const QString &skill_name) : FilterSkill(QString("#%1-filter").arg(skill_name)), n(skill_name){
    }

    virtual bool viewFilter(const Card *to_select) const{
        return to_select->getTypeId() == Card::TypeTrick;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName("_" + n);
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
	private:
    QString n;
};

class Mumu : public TriggerSkill
{
public:
    Mumu() : TriggerSkill("mumu")
    {
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() == Player::Finish && player->getMark("damage_point_play_phase") == 0) {
            QList<ServerPlayer *> weapon_players, armor_players;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->getWeapon() && player->canDiscard(p, p->getWeapon()->getEffectiveId()))
                    weapon_players << p;
                if (p != player && p->getArmor())
                    armor_players << p;
            }
            QStringList choices;
            choices << "cancel";
            if (!armor_players.isEmpty()) choices.prepend("armor");
            if (!weapon_players.isEmpty()) choices.prepend("weapon");
            if (choices.length() == 1) return false;
            QString choice = room->askForChoice(player, objectName(), choices.join("+"));
            if (choice == "cancel") {
                return false;
            } else {
                room->notifySkillInvoked(player, objectName());
                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {
					if (choice == "weapon") {
						room->broadcastSkillInvoke(objectName(), 1);
						ServerPlayer *victim = room->askForPlayerChosen(player, weapon_players, objectName(), "@mumu-weapon");
						room->throwCard(victim->getWeapon(), victim, player);
						player->drawCards(1, objectName());
					} else {
						room->broadcastSkillInvoke(objectName(), 2);
						ServerPlayer *victim = room->askForPlayerChosen(player, armor_players, objectName(), "@mumu-armor");
						int equip = victim->getArmor()->getEffectiveId();
						QList<CardsMoveStruct> exchangeMove;
						CardsMoveStruct move1(equip, player, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_ROB, player->objectName()));
						exchangeMove.push_back(move1);
						if (player->getArmor()) {
							CardsMoveStruct move2(player->getArmor()->getEffectiveId(), NULL, Player::DiscardPile,
								CardMoveReason(CardMoveReason::S_REASON_CHANGE_EQUIP, player->objectName()));
							exchangeMove.push_back(move2);
						}
						room->moveCardsAtomic(exchangeMove, true);
					}
                    room->removePlayerMark(player, objectName()+"engine");
                }
            }
        }
        return false;
    }
};


#include "jsonutils.h"
class AocaiViewAsSkill: public ZeroCardViewAsSkill {
public:
    AocaiViewAsSkill(): ZeroCardViewAsSkill("aocai") {
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (player->getPhase() != Player::NotActive || player->hasFlag("Global_AocaiFailed")) return false;
        if (pattern == "slash")
            return Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
        else if (pattern == "peach")
            return player->getMark("Global_PreventPeach") == 0;
        else if (pattern.contains("analeptic"))
            return true;
        return false;
    }

    virtual const Card *viewAs() const{
        AocaiCard *aocai_card = new AocaiCard;
        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if (pattern == "peach+analeptic" && Self->getMark("Global_PreventPeach") > 0)
            pattern = "analeptic";
        aocai_card->setUserString(pattern);
        return aocai_card;
    }
};

class Aocai: public TriggerSkill {
public:
    Aocai(): TriggerSkill("aocai") {
        events << CardAsked;
        view_as_skill = new AocaiViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QString pattern = data.toStringList().first();
        if (player->getPhase() == Player::NotActive
            && (pattern == "slash" || pattern == "jink")
            && room->askForSkillInvoke(player, objectName(), data)) {
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				QList<int> ids = room->getNCards(2, false);
				QList<int> enabled, disabled;
				foreach (int id, ids) {
					if (Sanguosha->getCard(id)->objectName().contains(pattern))
						enabled << id;
					else
						disabled << id;
				}
				int id = Aocai::view(room, player, ids, enabled, disabled);
				if (id != -1) {
					const Card *card = Sanguosha->getCard(id);
					room->provide(card);
					return true;
				}
				room->removePlayerMark(player, objectName()+"engine");
			}
        }
        return false;
    }

    static int view(Room *room, ServerPlayer *player, QList<int> &ids, QList<int> &enabled, QList<int> &disabled) {
        int result = -1, index = -1;
        LogMessage log;
        log.type = "$ViewDrawPile";
        log.from = player;
        log.card_str = IntList2StringList(ids).join("+");
        room->sendLog(log, player);

        room->broadcastSkillInvoke("aocai");
        room->notifySkillInvoked(player, "aocai");
        if (enabled.isEmpty()) {
            Json::Value arg(Json::arrayValue);
            arg[0] = QSanProtocol::Utils::toJsonString(".");
            arg[1] = false;
            arg[2] = QSanProtocol::Utils::toJsonArray(ids);
            room->doNotify(player, QSanProtocol::S_COMMAND_SHOW_ALL_CARDS, arg);
        } else {
            room->fillAG(ids, player, disabled);
            int id = room->askForAG(player, enabled, true, "aocai");
            if (id != -1) {
                index = ids.indexOf(id);
                ids.removeOne(id);
                result = id;
            }
            room->clearAG(player);
        }

        QList<int> &drawPile = room->getDrawPile();
        for (int i = ids.length() - 1; i >= 0; --i)
            drawPile.prepend(ids.at(i));
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_UPDATE_PILE, Json::Value(drawPile.length()));
        if (result == -1)
            room->setPlayerFlag(player, "Global_AocaiFailed");
        else {
            LogMessage log;
            log.type = "#AocaiUse";
            log.from = player;
            log.arg = "aocai";
            log.arg2 = QString("CAPITAL(%1)").arg(index + 1);
            room->sendLog(log);
        }
        return result;
    }
};

YinbingCard::YinbingCard() {
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
	mute = true;
}

void YinbingCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
	room->broadcastSkillInvoke(objectName(), 1);
    room->addPlayerMark(source, "yinbingengine");
    if (source->getMark("yinbingengine") > 0) {
        source->addToPile("hat", this);
        room->removePlayerMark(source, "yinbingengine");
    }
}

class YinbingViewAsSkill: public ViewAsSkill {
public:
    YinbingViewAsSkill(): ViewAsSkill("yinbing") {
        response_pattern = "@@yinbing";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        return to_select->getTypeId() != Card::TypeBasic;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() == 0) return NULL;

        Card *acard = new YinbingCard;
        acard->addSubcards(cards);
        acard->setSkillName(objectName());
        return acard;
    }
};

class Yinbing: public TriggerSkill {
public:
    Yinbing(): TriggerSkill("yinbing") {
        events << EventPhaseStart << Damaged;
        view_as_skill = new YinbingViewAsSkill;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish && !player->isNude()) {
            room->askForUseCard(player, "@@yinbing", "@yinbing", -1, Card::MethodNone);
        } else if (triggerEvent == Damaged && !player->getPile("hat").isEmpty()) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && (damage.card->isKindOf("Slash") || damage.card->isKindOf("Duel"))) {
                room->sendCompulsoryTriggerLog(player, objectName());
				room->broadcastSkillInvoke(objectName(), 2);

                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {
					QList<int> ids = player->getPile("hat");
					room->fillAG(ids, player);
					int id = room->askForAG(player, ids, false, objectName());
					room->clearAG(player);
					room->throwCard(id, NULL);
                }
            }
        }

        return false;
    }
};

class Juedi: public PhaseChangeSkill {
public:
    Juedi(): PhaseChangeSkill("juedi") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start
            && !target->getPile("hat").isEmpty();
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        if (!room->askForSkillInvoke(target, objectName())) return false;

        QList<ServerPlayer *> playerlist;
        foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
            if (p->getHp() <= target->getHp())
                playerlist << p;
        }
        ServerPlayer *to_give = NULL;
        if (!playerlist.isEmpty())
            to_give = room->askForPlayerChosen(target, playerlist, objectName(), "@juedi", true);
        room->broadcastSkillInvoke(objectName(), to_give == NULL? 1 : 2);
        room->addPlayerMark(target, objectName()+"engine");
        if (target->getMark(objectName()+"engine") > 0) {
			if (to_give) {
				room->recover(to_give, RecoverStruct(target));
				DummyCard *dummy = new DummyCard(target->getPile("hat"));
				room->obtainCard(to_give, dummy);
				delete dummy;
			} else {
				int len = target->getPile("hat").length();
				target->clearOnePrivatePile("hat");
				if (target->isAlive())
					room->drawCards(target, len, objectName());
			}
            room->removePlayerMark(target, objectName()+"engine");
        }
        return false;
    }
};

class Gongao: public TriggerSkill {
public:
    Gongao(): TriggerSkill("gongao") {
        events << BuryVictim;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }
	
    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        room->broadcastSkillInvoke(objectName());
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
			if (p == player) return false;
			room->sendCompulsoryTriggerLog(p, objectName());
			LogMessage log2;
			log2.type = "#GainMaxHp";
			log2.from = p;
			log2.arg = "1";
			room->sendLog(log2);


			room->addPlayerMark(p, objectName()+"engine");
			if (p->getMark(objectName()+"engine") > 0) {
				room->setPlayerProperty(p, "maxhp", p->getMaxHp() + 1);
				if (p->isWounded()) {
					room->recover(p, RecoverStruct(p));
				} else {
					LogMessage log3;
					log3.type = "#GetHp";
					log3.from = p;
					log3.arg = QString::number(p->getHp());
					log3.arg2 = QString::number(p->getMaxHp());
					room->sendLog(log3);
				}
				room->removePlayerMark(p, objectName()+"engine");
			}
        }

        return false;
    }
};

class Juyi: public PhaseChangeSkill {
public:
    Juyi(): PhaseChangeSkill("juyi") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Start
            && target->getMark("juyi") == 0
            && target->isWounded()
            && target->getMaxHp() > target->aliveCount();
    }

    virtual bool onPhaseChange(ServerPlayer *zhugedan) const{
        Room *room = zhugedan->getRoom();

        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(zhugedan, objectName());
        //room->doLightbox("$JuyiAnimate");

        LogMessage log;
        log.type = "#JuyiWake";
        log.from = zhugedan;
        log.arg = QString::number(zhugedan->getMaxHp());
        log.arg2 = QString::number(zhugedan->aliveCount());
        room->sendLog(log);

        room->addPlayerMark(zhugedan, objectName()+"engine");
        if (zhugedan->getMark(objectName()+"engine") > 0) {
			room->setPlayerMark(zhugedan, "juyi", 1);
			room->addPlayerMark(zhugedan, "@waked");
			int diff = zhugedan->getHandcardNum() - zhugedan->getMaxHp();
			if (diff < 0)
				room->drawCards(zhugedan, -diff, objectName());
			if (zhugedan->getMark("juyi") == 1)
				room->handleAcquireDetachSkills(zhugedan, "benghuai|weizhong");

            room->removePlayerMark(zhugedan, objectName()+"engine");
        }
        return false;
    }
};

class Weizhong: public TriggerSkill {
public:
    Weizhong(): TriggerSkill("weizhong") {
        events << MaxHpChanged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        room->addPlayerMark(player, objectName()+"engine");
        if (player->getMark(objectName()+"engine") > 0) {
            player->drawCards(1, objectName());
            room->removePlayerMark(player, objectName()+"engine");
        }
        return false;
    }
};

XiemuCard::XiemuCard() {
    target_fixed = true;
    mute = true;
}

void XiemuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
	room->broadcastSkillInvoke("xiemu", 1);
    room->addPlayerMark(source, "xiemuengine");
    if (source->getMark("xiemuengine") > 0) {
        QString kingdom = room->askForChoice(source, "xiemu", "wei+shu+wu+qun");
        room->setPlayerMark(source, "@xiemu_" + kingdom, 1);
        room->removePlayerMark(source, "xiemuengine");
    }
}

class XiemuViewAsSkill: public OneCardViewAsSkill {
public:
    XiemuViewAsSkill(): OneCardViewAsSkill("xiemu") {
        filter_pattern = "Slash";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("XiemuCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        XiemuCard *card = new XiemuCard;
        card->addSubcard(originalCard);
        return card;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const {
        return 1;
    }
};

class Xiemu: public TriggerSkill {
public:
    Xiemu(): TriggerSkill("xiemu") {
        events << TargetConfirmed << EventPhaseStart;
        view_as_skill = new XiemuViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from && player != use.from && use.card->getTypeId() != Card::TypeSkill
                && use.card->isBlack() && use.to.contains(player)
                && player->getMark("@xiemu_" + use.from->getKingdom()) > 0) {
                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = player;
                    log.arg = objectName();
                    room->sendLog(log);

                    room->notifySkillInvoked(player, objectName());
					room->broadcastSkillInvoke(objectName(), 2);
                    player->drawCards(2, objectName());
            }
        } else {
            if (player->getPhase() == Player::RoundStart) {
                foreach (QString kingdom, Sanguosha->getKingdoms()) {
                    QString markname = "@xiemu_" + kingdom;
                    if (player->getMark(markname) > 0)
                        room->setPlayerMark(player, markname, 0);
                }
            }
        }
        return false;
    }
};

class Naman: public TriggerSkill {
public:
    Naman(): TriggerSkill("naman") {
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to_place != Player::DiscardPile) return false;
        const Card *to_obtain = NULL;
        if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_RESPONSE) {
            if (move.from && player->objectName() == move.from->objectName())
                return false;
            to_obtain = move.reason.m_extraData.value<const Card *>();
            if (!to_obtain || !to_obtain->isKindOf("Slash"))
                return false;
        } else {
            return false;
        }
        if (to_obtain && room->askForSkillInvoke(player, objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                room->obtainCard(player, to_obtain);
                move.removeCardIds(move.card_ids);
                data = QVariant::fromValue(move);
                room->removePlayerMark(player, objectName()+"engine");
            }
        }

        return false;
    }
};

ShefuCard::ShefuCard() {
    will_throw = false;
    target_fixed = true;
    mute = true;
}

#include "jsonutils.h"
void ShefuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->addPlayerMark(source, "shefuengine");
    if (source->getMark("shefuengine") > 0) {
		QString mark = "Shefu_" + user_string;
		source->setMark(mark, getEffectiveId() + 1);

		Json::Value arg(Json::arrayValue);
		arg[0] = QSanProtocol::Utils::toJsonString(source->objectName());
		arg[1] = QSanProtocol::Utils::toJsonString(mark);
		arg[2] = getEffectiveId() + 1;
		room->doNotify(source, QSanProtocol::S_COMMAND_SET_MARK, arg);

		
		room->broadcastSkillInvoke("shefu", 1);
		source->addToPile("ambush", this, false);

		LogMessage log;
		log.type = "$ShefuRecord";
		log.from = source;
		log.card_str = QString::number(getEffectiveId());
		log.arg = user_string;
		room->sendLog(log, source);
    }
}

ShefuDialog *ShefuDialog::getInstance(const QString &object) {
    static ShefuDialog *instance;
    if (instance == NULL || instance->objectName() != object)
        instance = new ShefuDialog(object);

    return instance;
}

ShefuDialog::ShefuDialog(const QString &object)
    : GuhuoDialog(object, true, true, false, true, true)
{
}

bool ShefuDialog::isButtonEnabled(const QString &button_name) const{
    return Self->getMark("Shefu_" + button_name) == 0;
}

class ShefuViewAsSkill: public OneCardViewAsSkill {
public:
    ShefuViewAsSkill(): OneCardViewAsSkill("shefu") {
        filter_pattern = ".|.|.|hand";
        response_pattern = "@@shefu";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        const Card *c = Self->tag.value("shefu").value<const Card *>();
        if (c) {
            QString user_string = c->objectName();
            if (Self->getMark("Shefu_" + user_string) > 0)
                return NULL;

            ShefuCard *card = new ShefuCard;
            card->setUserString(user_string);
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }
};

class Shefu: public PhaseChangeSkill {
public:
    Shefu(): PhaseChangeSkill("shefu") {
        view_as_skill = new ShefuViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        if (target->getPhase() != Player::Finish || target->isKongcheng())
            return false;
        room->askForUseCard(target, "@@shefu", "@shefu-prompt", -1, Card::MethodNone);
        return false;
    }

    virtual QDialog *getDialog() const{
        return ShefuDialog::getInstance("shefu");
    }
};

class ShefuCancel: public TriggerSkill {
public:
    ShefuCancel(): TriggerSkill("#shefu-cancel") {
        events << CardUsed << JinkEffect << NullificationEffect;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == JinkEffect) {
            bool invoked = false;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (ShefuTriggerable(p, player)) {
                    room->setTag("ShefuData", data);
                    if (!room->askForSkillInvoke(p, "shefu_cancel", "data:::jink") || p->getMark("Shefu_jink") == 0)
                        continue;


                    LogMessage log;
                    log.type = "#ShefuEffect";
                    log.from = p;
                    log.to << player;
                    log.arg = "jink";
                    log.arg2 = "shefu";
                    room->sendLog(log);

                    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "shefu", QString());
                    int id = p->getMark("Shefu_jink") - 1;
                    room->setPlayerMark(p, "Shefu_jink", 0);
                    room->throwCard(Sanguosha->getCard(id), reason, NULL);
					room->broadcastSkillInvoke("shefu", 2);
					room->addPlayerMark(player, objectName()+"engine");
					if (player->getMark(objectName()+"engine") > 0) {
						invoked = true;
						room->removePlayerMark(player, objectName()+"engine");
					}
                }
            }
            return invoked;
        } else if (triggerEvent == NullificationEffect) {
            bool invoked = false;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (ShefuTriggerable(p, player)) {
                    room->setTag("ShefuData", data);
                    if (!room->askForSkillInvoke(p, "shefu_cancel", "data:::nullification") || p->getMark("Shefu_nullification") == 0)
                        continue;


                    LogMessage log;
                    log.type = "#ShefuEffect";
                    log.from = p;
                    log.to << player;
                    log.arg = "nullification";
                    log.arg2 = "shefu";
                    room->sendLog(log);

                    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "shefu", QString());
                    int id = p->getMark("Shefu_nullification") - 1;
                    room->setPlayerMark(p, "Shefu_nullification", 0);
                    room->throwCard(Sanguosha->getCard(id), reason, NULL);
				    room->broadcastSkillInvoke("shefu", 2);
					room->addPlayerMark(player, objectName()+"engine");
					if (player->getMark(objectName()+"engine") > 0) {
						invoked = true;
						room->removePlayerMark(player, objectName()+"engine");
					}
                }
            }
            return invoked;
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeBasic && use.card->getTypeId() != Card::TypeTrick)
                return false;
            if (use.card->isKindOf("Nullification"))
                return false;
            QString card_name = use.card->objectName();
            if (card_name.contains("slash")) card_name = "slash";
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (ShefuTriggerable(p, player)) {
                    room->setTag("ShefuData", data);
                    if (!room->askForSkillInvoke(p, "shefu_cancel", "data:::" + card_name) || p->getMark("Shefu_" + card_name) == 0)
                        continue;

                    LogMessage log;
                    log.type = "#ShefuEffect";
                    log.from = p;
                    log.to << player;
                    log.arg = card_name;
                    log.arg2 = "shefu";
                    room->sendLog(log);

                    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "shefu", QString());
                    int id = p->getMark("Shefu_" + card_name) - 1;
                    room->setPlayerMark(p, "Shefu_" + card_name, 0);
                    room->throwCard(Sanguosha->getCard(id), reason, NULL);
				    room->broadcastSkillInvoke("shefu", 2);
                    use.nullified_list << "_ALL_TARGETS";
                }
            }
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				data = QVariant::fromValue(use);
				room->removePlayerMark(player, objectName()+"engine");
			}
        }
        return false;
    }

private:
    bool ShefuTriggerable(ServerPlayer *chengyu, ServerPlayer *user) const{
        return chengyu->getPhase() == Player::NotActive && chengyu != user
               && chengyu->hasSkill("shefu") && !chengyu->getPile("ambush").isEmpty();
    }
};

BenyuCard::BenyuCard() {
    target_fixed = true;
}

void BenyuCard::use(Room *, ServerPlayer *, QList<ServerPlayer *> &) const{
    // dummy
}

class BenyuViewAsSkill: public ViewAsSkill {
public:
    BenyuViewAsSkill(): ViewAsSkill("benyu") {
        response_pattern = "@@benyu";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() < Self->getMark("benyu"))
            return NULL;
        BenyuCard *card = new BenyuCard;
        card->addSubcards(cards);
        return card;
    }
};

class Benyu: public MasochismSkill {
public:
    Benyu(): MasochismSkill("benyu") {
        view_as_skill = new BenyuViewAsSkill;
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const{
        if (!damage.from || damage.from->isDead())
            return;
        Room *room = target->getRoom();
        int from_handcard_num = damage.from->getHandcardNum(), handcard_num = target->getHandcardNum();
        QVariant data = QVariant::fromValue(damage);
        if (handcard_num == from_handcard_num) {
            return;
        } else if (handcard_num < from_handcard_num && handcard_num < 5 && room->askForSkillInvoke(target, objectName(), data)) {
            room->broadcastSkillInvoke(objectName(), 1);
            room->addPlayerMark(target, objectName()+"engine");
            if (target->getMark(objectName()+"engine") > 0) {
                room->drawCards(target, qMin(5, from_handcard_num) - handcard_num, objectName());
                room->removePlayerMark(target, objectName()+"engine");
            }
        } else if (handcard_num > from_handcard_num) {
            room->setPlayerMark(target, objectName(), from_handcard_num + 1);
            if (room->askForUseCard(target, "@@benyu",  // I think we can use askForDiscard here, better than askForUseCard??
                                    QString("@benyu-discard::%1:%2").arg(damage.from->objectName()).arg(from_handcard_num + 1),
                                    -1, Card::MethodDiscard)) {
                room->addPlayerMark(target, objectName()+"engine");
                if (target->getMark(objectName()+"engine") > 0) {
                    room->damage(DamageStruct(objectName(), target, damage.from));
                    room->removePlayerMark(target, objectName()+"engine");
				}
            }
        }
        return;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const {
        return 2;
    }
};

class Fulu: public OneCardViewAsSkill {
public:
    Fulu(): OneCardViewAsSkill("fulu") {
        filter_pattern = "Slash";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE && pattern == "slash";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThunderSlash *acard = new ThunderSlash(originalCard->getSuit(), originalCard->getNumber());
        acard->addSubcard(originalCard->getId());
        acard->setSkillName(objectName());
        return acard;
    }
};

class Zhuji: public TriggerSkill {
public:
    Zhuji(): TriggerSkill("zhuji") {
        events << DamageCaused << FinishJudge;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.nature != DamageStruct::Thunder || !damage.from)
                return false;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (TriggerSkill::triggerable(p) && room->askForSkillInvoke(p, objectName(), data)) {
                    room->broadcastSkillInvoke(objectName());
                    room->addPlayerMark(p, objectName()+"engine");
                    if (p->getMark(objectName()+"engine") > 0) {
						JudgeStruct judge;
						judge.good = true;
						judge.play_animation = false;
						judge.reason = objectName();
						judge.pattern = ".";
						judge.who = damage.from;

						room->judge(judge);
						if (judge.pattern == "black") {
							LogMessage log;
							log.type = "#ZhujiBuff";
							log.from = p;
							log.to << damage.to;
							log.arg = QString::number(damage.damage);
							log.arg2 = QString::number(++damage.damage);
							room->sendLog(log);

							data = QVariant::fromValue(damage);
						}
                        room->removePlayerMark(p, objectName()+"engine");
                    }
                }
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName()) {
                judge->pattern = (judge->card->isRed() ? "red" : "black");
                if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge && judge->card->isRed())
                    player->obtainCard(judge->card);
            }
        }
        return false;
    }
};

class SpZhenwei : public TriggerSkill{
public:
    SpZhenwei() : TriggerSkill("spzhenwei"){
        events << TargetConfirming << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (!p->getPile("zhenweipile").isEmpty()) {
                        DummyCard *dummy = new DummyCard(p->getPile("zhenweipile"));
                        room->obtainCard(p, dummy);
                        delete dummy;
                    }
                }
            }
        } else {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card == NULL || use.to.length() != 1 || !(use.card->isKindOf("Slash") || (use.card->getTypeId() == Card::TypeTrick && use.card->isBlack())))
                return false;
            ServerPlayer *wp = room->findPlayerBySkillName(objectName());
            if (wp == NULL || wp->getHp() <= player->getHp() || wp == use.from)
                return false;
            if (!room->askForCard(wp, "..", QString("@sp_zhenwei:%1").arg(player->objectName()), data, objectName()))
                return false;
            room->addPlayerMark(wp, objectName()+"engine");
            if (wp->getMark(objectName()+"engine") > 0) {
				QString choice = room->askForChoice(wp, objectName(), "draw+null");
				if (choice == "draw"){
					room->broadcastSkillInvoke(objectName(), 1);
					room->drawCards(wp, 1, objectName());

					if (use.card->isKindOf("Slash")) {
						if (!use.from->canSlash(wp, use.card, false))
							return false;
					}

					if (!use.card->isKindOf("DelayedTrick")) {
						if (use.from->isProhibited(wp, use.card))
							return false;

						if (use.card->isKindOf("Collateral")) {
							QList<ServerPlayer *> targets;
							foreach (ServerPlayer *p, room->getOtherPlayers(wp)) {
								if (wp->canSlash(p))
									targets << p;
							}

							if (targets.isEmpty())
								return false;

							use.to.first()->tag.remove("collateralVictim");
							ServerPlayer *target = room->askForPlayerChosen(use.from, targets, objectName(), QString("@dummy-slash2:%1").arg(wp->objectName()));
							wp->tag["collateralVictim"] = QVariant::fromValue(target);

							LogMessage log;
							log.type = "#CollateralSlash";
							log.from = use.from;
							log.to << target;
							room->sendLog(log);
							room->doAnimate(1, wp->objectName(), target->objectName());
						}
						use.to = QList<ServerPlayer *>();
						use.to << wp;
						data = QVariant::fromValue(use);
					} else {
						if (use.from->isProhibited(wp, use.card) || wp->containsTrick(use.card->objectName()))
							return false;
						room->moveCardTo(use.card, wp, Player::PlaceDelayedTrick, true);
					}
				} else {
					room->broadcastSkillInvoke(objectName(), 2);
					room->setCardFlag(use.card, "zhenweinull");
					use.from->addToPile("zhenweipile", use.card);

					use.nullified_list << "_ALL_TARGETS";
					data = QVariant::fromValue(use);
				}
                room->removePlayerMark(wp, objectName()+"engine");
            }
        }
        return false;
    }
};

class Junbing : public TriggerSkill{
public:
    Junbing() : TriggerSkill("junbing"){
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return player && player->isAlive() && player->getPhase() == Player::Finish && player->getHandcardNum() <= 1;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        ServerPlayer *simalang = room->findPlayerBySkillName(objectName());
        if (!simalang || !simalang->isAlive() || (simalang->getMark("simalangengine") == 0 && simalang != player))
            return false;
        if (player->askForSkillInvoke(objectName(), QString("junbing_invoke:%1").arg(simalang->objectName()))) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(simalang, objectName());
            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                player->drawCards(1);
                if (player->objectName() != simalang->objectName()) {
                    DummyCard *cards = player->wholeHandCards();
                    CardMoveReason reason = CardMoveReason(CardMoveReason::S_REASON_GIVE, player->objectName());
                    room->moveCardTo(cards, simalang, Player::PlaceHand, reason);

                    int x = qMin(cards->subcardsLength(), simalang->getHandcardNum());

                    if (x > 0) {
                        const Card *return_cards = room->askForExchange(simalang, objectName(), x, x, false, QString("@junbing-return:%1::%2").arg(player->objectName()).arg(cards->subcardsLength()));
                        CardMoveReason return_reason = CardMoveReason(CardMoveReason::S_REASON_GIVE, simalang->objectName());
                        room->moveCardTo(return_cards, player, Player::PlaceHand, return_reason);
                        delete return_cards;
                    }
                    delete cards;
                }
                room->removePlayerMark(player, objectName()+"engine");
            }
        }
        return false;
    }
};

QujiCard::QujiCard(){
	mute = true;
}

bool QujiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    if (subcardsLength() <= targets.length())
        return false;
    return to_select->isWounded();
}

bool QujiCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    if (targets.length() > 0) {
        foreach (const Player *p, targets) {
            if (!p->isWounded())
                return false;
        }
        return true;
    }
    return false;
}

void QujiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->broadcastSkillInvoke(objectName(), 1);
	room->addPlayerMark(source, "qujiengine");
    if (source->getMark("qujiengine") > 0) {
        foreach(ServerPlayer *p, targets)
            room->cardEffect(this, source, p);

        foreach (int id, getSubcards()) {
            if (Sanguosha->getCard(id)->isBlack()) {
                room->loseHp(source);
                room->broadcastSkillInvoke(objectName(), 2);
                break;
            }
        }
        room->removePlayerMark(source, "qujiengine");
    }
}

void QujiCard::onEffect(const CardEffectStruct &effect) const{
    RecoverStruct recover;
    recover.who = effect.from;
    recover.recover = 1;
    effect.to->getRoom()->recover(effect.to, recover);
}

class Quji : public ViewAsSkill{
public:
    Quji() : ViewAsSkill("quji"){
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *) const{
        return selected.length() < Self->getLostHp();
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->isWounded() && !player->hasUsed("QujiCard");
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() == Self->getLostHp()){
            QujiCard *quji = new QujiCard;
            quji->addSubcards(cards);
            return quji;
        }
        return NULL;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const {
        return 1;
    }
};

AocaiCard::AocaiCard() {
}

bool AocaiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool AocaiCard::targetFixed() const{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetFixed();
}

bool AocaiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetsFeasible(targets, Self);
}

const Card *AocaiCard::validateInResponse(ServerPlayer *user) const{
    Room *room = user->getRoom();
    QList<int> ids = room->getNCards(2, false);
    QStringList names = user_string.split("+");
    if (names.contains("slash")) names << "fire_slash" << "thunder_slash";

    QList<int> enabled, disabled;
    foreach (int id, ids) {
        if (names.contains(Sanguosha->getCard(id)->objectName()))
            enabled << id;
        else
            disabled << id;
    }

    LogMessage log;
    log.type = "#InvokeSkill";
    log.from = user;
    log.arg = "aocai";
    room->sendLog(log);

    room->addPlayerMark(user, "aocaiengine");
    if (user->getMark("aocaiengine") > 0) {
		int id = Aocai::view(room, user, ids, enabled, disabled);
        room->removePlayerMark(user, "aocaiengine");
		return Sanguosha->getCard(id);
    }
	return NULL;
}

const Card *AocaiCard::validate(CardUseStruct &cardUse) const{
    cardUse.m_isOwnerUse = false;
    ServerPlayer *user = cardUse.from;
    Room *room = user->getRoom();
    QList<int> ids = room->getNCards(2, false);
    QStringList names = user_string.split("+");
    if (names.contains("slash")) names << "fire_slash" << "thunder_slash";

    QList<int> enabled, disabled;
    foreach (int id, ids) {
        if (names.contains(Sanguosha->getCard(id)->objectName()))
            enabled << id;
        else
            disabled << id;
    }

    LogMessage log;
    log.type = "#InvokeSkill";
    log.from = user;
    log.arg = "aocai";
    room->sendLog(log);

    room->addPlayerMark(user, "aocaiengine");
    if (user->getMark("aocaiengine") > 0) {
		int id = Aocai::view(room, user, ids, enabled, disabled);
        room->removePlayerMark(user, "aocaiengine");
		return Sanguosha->getCard(id);
    }
	return NULL;
}

DuwuCard::DuwuCard() {
}

bool DuwuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty() || qMax(0, to_select->getHp()) != subcardsLength())
        return false;

    if (Self->getWeapon() && subcards.contains(Self->getWeapon()->getId())) {
        const Weapon *weapon = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        int distance_fix = weapon->getRange() - Self->getAttackRange(false);
        if (Self->getOffensiveHorse() && subcards.contains(Self->getOffensiveHorse()->getId()))
            distance_fix += 1;
        return Self->inMyAttackRange(to_select, distance_fix);
    } else if (Self->getOffensiveHorse() && subcards.contains(Self->getOffensiveHorse()->getId())) {
        return Self->inMyAttackRange(to_select, 1);
    } else
        return Self->inMyAttackRange(to_select);
}

void DuwuCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->addPlayerMark(effect.from, "duwuengine");
    if (effect.from->getMark("duwuengine") > 0) {
		room->damage(DamageStruct("duwu", effect.from, effect.to));
        room->removePlayerMark(effect.from, "duwuengine");
    }
}

class DuwuViewAsSkill: public ViewAsSkill {
public:
    DuwuViewAsSkill(): ViewAsSkill("duwu") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasFlag("DuwuEnterDying");
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        return !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        DuwuCard *duwu = new DuwuCard;
        if (!cards.isEmpty())
            duwu->addSubcards(cards);
        return duwu;
    }
};

class Duwu: public TriggerSkill {
public:
    Duwu(): TriggerSkill("duwu") {
        events << QuitDying;
        view_as_skill = new DuwuViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.damage && dying.damage->getReason() == "duwu" && !dying.damage->chain && !dying.damage->transfer) {
            ServerPlayer *from = dying.damage->from;
            if (from && from->isAlive()) {
                room->setPlayerFlag(from, "DuwuEnterDying");
                room->loseHp(from, 1);
            }
        }
        return false;
    }
};

class Dujin: public DrawCardsSkill {
public:
    Dujin(): DrawCardsSkill("dujin") {
        frequency = Frequent;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
		Room *room = player->getRoom();
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				n = n + player->getEquips().length() / 2 + 1;
				room->removePlayerMark(player, objectName()+"engine");
			}
        }
        return n;
    }
};

QingyiCard::QingyiCard() {
    mute = true;
}

bool QingyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("qingyi");
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}

void QingyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    foreach (ServerPlayer *target, targets) {
        if (!source->canSlash(target, NULL, false))
            targets.removeOne(target);
    }

    if (targets.length() > 0) {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_qingyi");
        room->useCard(CardUseStruct(slash, source, targets));
    }
}

class QingyiViewAsSkill: public ZeroCardViewAsSkill {
public:
    QingyiViewAsSkill(): ZeroCardViewAsSkill("qingyi") {
        response_pattern = "@@qingyi";
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@@qingyi";
    }

    virtual const Card *viewAs() const{
        return new QingyiCard;
    }
};

class Qingyi: public TriggerSkill {
public:
    Qingyi(): TriggerSkill("qingyi") {
        events << EventPhaseChanging;
        view_as_skill = new QingyiViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Judge && !player->isSkipped(Player::Judge)
            && !player->isSkipped(Player::Draw)) {
            if (Slash::IsAvailable(player) && room->askForUseCard(player, "@@qingyi", "@qingyi-slash")) {
                player->skip(Player::Judge, true);
                player->skip(Player::Draw, true);
            }
        }
        return false;
    }
};

class Shixin: public TriggerSkill {
public:
    Shixin(): TriggerSkill("shixin") {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature == DamageStruct::Fire) {
            room->notifySkillInvoked(player, objectName());
            room->broadcastSkillInvoke(objectName());

            LogMessage log;
            log.type = "#ShixinProtect";
            log.from = player;
            log.arg = QString::number(damage.damage);
            log.arg2 = "fire_nature";
            room->sendLog(log);
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				room->removePlayerMark(player, objectName()+"engine");
				return true;
			}
        }
        return false;
    }
};

SanyaoCard::SanyaoCard() {
}

bool SanyaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty()) return false;
    QList<const Player *> players = Self->getAliveSiblings();
    players << Self;
    int max = -1000;
    foreach (const Player *p, players) {
        if (max < p->getHp()) max = p->getHp();
    }
    return to_select->getHp() == max;
}

void SanyaoCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->addPlayerMark(effect.from, "sanyaoengine");
    if (effect.from->getMark("sanyaoengine") > 0) {
		room->damage(DamageStruct("sanyao", effect.from, effect.to));
        room->removePlayerMark(effect.from, "sanyaoengine");
    }
}

class Sanyao: public OneCardViewAsSkill {
public:
    Sanyao(): OneCardViewAsSkill("sanyao") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("SanyaoCard");
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        SanyaoCard *first = new SanyaoCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class Zhiman: public TriggerSkill {
public:
    Zhiman(): TriggerSkill("zhiman") {
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        if (player->askForSkillInvoke(objectName(), data)) {
            LogMessage log;
            log.type = "#Yishi";
            log.from = player;
            log.arg = objectName();
            log.to << damage.to;
            room->sendLog(log);

			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				if (damage.to->getEquips().isEmpty() && damage.to->getJudgingArea().isEmpty()){
					room->broadcastSkillInvoke(objectName(), 2);
					room->removePlayerMark(player, objectName()+"engine");
					return true;
				}
				room->broadcastSkillInvoke(objectName(), 1);
				int card_id = room->askForCardChosen(player, damage.to, "ej", objectName());
				CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
				room->obtainCard(player, Sanguosha->getCard(card_id), reason);
				room->removePlayerMark(player, objectName()+"engine");
				return true;
			}
        }
        return false;
    }
};

JieyueCard::JieyueCard() {
}

void JieyueCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->addPlayerMark(effect.from, "jieyueengine");
    if (effect.from->getMark("jieyueengine") > 0) {
		if (!effect.to->isNude()) {
			const Card *card = room->askForExchange(effect.to, "jieyue", 1, 1, true, QString("@jieyue_put:%1").arg(effect.from->objectName()), true);

			if (card != NULL)
				effect.from->addToPile("jieyue_pile", card);
			else if (effect.from->canDiscard(effect.to, "he")){
				int id = room->askForCardChosen(effect.from, effect.to, "he", objectName(), false, Card::MethodDiscard);
				room->throwCard(id, effect.to, effect.from);
			}
		}
        room->removePlayerMark(effect.from, "jieyueengine");
    }
}

class JieyueVS : public OneCardViewAsSkill{
public:
    JieyueVS() : OneCardViewAsSkill("jieyue"){
    }

	bool isResponseOrUse() const
    {
        return !Self->getPile("jieyue_pile").isEmpty();
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        if (to_select->isEquipped())
            return false;
        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if (pattern == "@@jieyue") {
            return !Self->isJilei(to_select);
        }

        if (pattern == "jink")
            return to_select->isRed();
        else if (pattern == "nullification")
            return to_select->isBlack();
        return false;
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return (!player->getPile("jieyue_pile").isEmpty() && (pattern == "jink" || pattern == "nullification")) || (pattern == "@@jieyue" && !player->isKongcheng());
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        return !player->getPile("jieyue_pile").isEmpty();
    }

    virtual const Card *viewAs(const Card *card) const{
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "@@jieyue") {
            JieyueCard *jy = new JieyueCard;
            jy->addSubcard(card);
            return jy;
        }

        if (card->isRed()){
            Jink *jink = new Jink(Card::SuitToBeDecided, 0);
            jink->addSubcard(card);
            jink->setSkillName(objectName());
            return jink;
        }
        else if (card->isBlack()){
            Nullification *nulli = new Nullification(Card::SuitToBeDecided, 0);
            nulli->addSubcard(card);
            nulli->setSkillName(objectName());
            return nulli;
        }
        return NULL;
    }
	
    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        int index = 2;
        if (pattern == "@@jieyue") {
            index = 1;
        }
        return index;
    }
};

class Jieyue : public TriggerSkill{
public:
    Jieyue() : TriggerSkill("jieyue"){
        events << EventPhaseStart;
        view_as_skill = new JieyueVS;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() == Player::Start && !player->getPile("jieyue_pile").isEmpty()){
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);
            DummyCard *dummy = new DummyCard(player->getPile("jieyue_pile"));
            player->obtainCard(dummy);
            delete dummy;
        } else if (player->getPhase() == Player::Finish) {
            room->askForUseCard(player, "@@jieyue", "@jieyue", -1, Card::MethodDiscard, false);
        }
        return false;
    }
};

class Liangzhu : public TriggerSkill{
public:
    Liangzhu() : TriggerSkill("liangzhu"){
        events << HpRecover;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive() && target->getPhase() == Player::Play;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
         foreach (ServerPlayer *sun, room->getAllPlayers()) {
            if (TriggerSkill::triggerable(sun)) {
                QString choice = room->askForChoice(sun, objectName(), "draw+letdraw+dismiss", QVariant::fromValue(player));
                if (choice == "dismiss")
                    continue;

                room->broadcastSkillInvoke(objectName(), choice == "letdraw" && player != sun ? 2 : 1);
                room->notifySkillInvoked(sun, objectName());
				room->addPlayerMark(sun, objectName()+"engine");
				if (sun->getMark(objectName()+"engine") > 0) {
					if (choice == "draw") {
						sun->drawCards(1);
						room->setPlayerMark(sun, "@liangzhu_draw", 1);
					} else if (choice == "letdraw") {
						room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, sun->objectName(), player->objectName());
						player->drawCards(2);
						room->setPlayerMark(player, "@liangzhu_draw", 1);
					}
					room->removePlayerMark(sun, objectName()+"engine");
				}
            }
        }
        return false;
    }
};

class Fanxiang : public TriggerSkill{
public:
    Fanxiang() : TriggerSkill("fanxiang"){
        events << EventPhaseStart;
        frequency = Skill::Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL && target->hasSkill(objectName()) && target->getPhase() == Player::Start && target->getMark("fanxiang") == 0);
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        bool flag = false;
        foreach(ServerPlayer *p, room->getAlivePlayers()){
            if (p->getMark("@liangzhu_draw") > 0 && p->isWounded()){
                flag = true;
                break;
            }
        }
        if (flag){
            //room->doLightbox("$fanxiangAnimate", 5000);
            room->notifySkillInvoked(player, objectName());
            room->broadcastSkillInvoke(objectName());
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				room->setPlayerMark(player, "fanxiang", 1);
				if (room->changeMaxHpForAwakenSkill(player, 1) && player->getMark("fanxiang") > 0) {

					room->recover(player, RecoverStruct());
					room->handleAcquireDetachSkills(player, "-liangzhu|xiaoji");
				}
				room->removePlayerMark(player, objectName()+"engine");
			}
        }
        return false;
    }
};



class CihuaiVS : public ZeroCardViewAsSkill
{
public:
    CihuaiVS() : ZeroCardViewAsSkill("cihuai")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player) && player->getMark("@cihuai") > 0;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "slash" && player->getMark("@cihuai") > 0;
    }

    virtual const Card *viewAs() const
    {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_" + objectName());
        return slash;
    }
};

class Cihuai : public TriggerSkill
{
public:
    Cihuai() : TriggerSkill("cihuai")
    {
        events << EventPhaseStart << CardsMoveOneTime << Death;
        view_as_skill = new CihuaiVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive() && (target->hasSkill(objectName()) || target->getMark("ViewAsSkill_cihuaiEffect") > 0);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Play && !player->isKongcheng() && TriggerSkill::triggerable(player) && player->askForSkillInvoke(objectName(), data)) {
				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					room->showAllCards(player);
					bool flag = true;
					foreach (const Card *card, player->getHandcards()) {
						if (card->isKindOf("Slash")) {
							flag = false;
							break;
						}
					}
					room->setPlayerMark(player, "cihuai_handcardnum", player->getHandcardNum());
					if (flag) {
						room->setPlayerMark(player, "@cihuai", 1);
						room->setPlayerMark(player, "ViewAsSkill_cihuaiEffect", 1);
					}
				}
				room->removePlayerMark(player, objectName()+"engine");
			}
        } else if (triggerEvent == CardsMoveOneTime) {
            if (player->getMark("@cihuai") > 0 && player->getHandcardNum() != player->getMark("cihuai_handcardnum")) {
                room->setPlayerMark(player, "@cihuai", 0);
                room->setPlayerMark(player, "ViewAsSkill_cihuaiEffect", 0);
            }
        } else if (triggerEvent == Death) {
            room->setPlayerMark(player, "@cihuai", 0);
            room->setPlayerMark(player, "ViewAsSkill_cihuaiEffect", 0);
        }
        return false;
    }
};

class Canshi : public TriggerSkill
{
public:
    Canshi() : TriggerSkill("canshi")
    {
        events << EventPhaseStart << CardUsed << CardResponded;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Draw) {
                int n = 0;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->isWounded())
                        ++n;
                }

                if (n > 0 && player->askForSkillInvoke(objectName())) {
                    room->broadcastSkillInvoke(objectName());
                    room->addPlayerMark(player, objectName()+"engine");
                    if (player->getMark(objectName()+"engine") > 0) {
                        player->setFlags(objectName());
                        player->drawCards(n, objectName());
                        room->removePlayerMark(player, objectName()+"engine");
                        return true;
                    }
                }
            }
        } else {
            if (player->hasFlag(objectName())) {
                const Card *card = NULL;
                if (triggerEvent == CardUsed)
                    card = data.value<CardUseStruct>().card;
                else {
                    CardResponseStruct resp = data.value<CardResponseStruct>();
                    if (resp.m_isUse)
                        card = resp.m_card;
                }
                if (card != NULL && (card->isKindOf("BasicCard") || card->isKindOf("TrickCard"))) {
                    room->sendCompulsoryTriggerLog(player, objectName());
                    if (!room->askForDiscard(player, objectName(), 1, 1, false, true, "@canshi-discard")) {
                        QList<const Card *> cards = player->getCards("he");
                        const Card *c = cards.at(qrand() % cards.length());
                        room->throwCard(c, player);
                    }
                }
            }
        }
        return false;
    }
};

class Chouhai : public TriggerSkill
{
public:
    Chouhai() : TriggerSkill("chouhai")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->isKongcheng()) {
            room->sendCompulsoryTriggerLog(player, objectName());
            room->broadcastSkillInvoke(objectName());

            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                DamageStruct damage = data.value<DamageStruct>();
                ++damage.damage;
                data = QVariant::fromValue(damage);
                room->removePlayerMark(player, objectName()+"engine");
            }
        }
        return false;
    }
};

class Guiming : public TriggerSkill // play audio effect only. This skill is coupled in Player::isWounded().
{
public:
    Guiming() : TriggerSkill("guiming$")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive() && target->hasLordSkill(objectName()) && target->getPhase() == Player::RoundStart;
    }

    virtual int getPriority(TriggerEvent) const
    {
        return 6;
    }

   virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        foreach (const ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getKingdom() == "wu" && p->isWounded() && p->getHp() == p->getMaxHp()) {
                if (player->hasSkill("weidi"))
                    room->broadcastSkillInvoke("weidi");
                else
                    room->broadcastSkillInvoke(objectName());
                return false;
            }
        }

        return false;
    }
};




class Nuzhan : public TriggerSkill {
public:
    Nuzhan() : TriggerSkill("nuzhan") {
        events << PreCardUsed << CardUsed << ConfirmDamage;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (TriggerSkill::triggerable(use.from)) {
                if (use.card != NULL && use.card->isKindOf("Slash") && use.card->isVirtualCard() && use.card->subcardsLength() == 1 && Sanguosha->getCard(use.card->getSubcards().first())->isKindOf("TrickCard")) {
                    room->addPlayerHistory(use.from, use.card->getClassName(), -1);
                    use.m_addHistory = false;
					room->addPlayerMark(player, objectName()+"engine");
					if (player->getMark(objectName()+"engine") > 0) {
						data = QVariant::fromValue(use);
						room->removePlayerMark(player, objectName()+"engine");
					}
                }
            }
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (TriggerSkill::triggerable(use.from)) {
                if (use.card != NULL && use.card->isKindOf("Slash") && use.card->isVirtualCard() && use.card->subcardsLength() == 1 && Sanguosha->getCard(use.card->getSubcards().first())->isKindOf("EquipCard"))
                    use.card->setFlags("nuzhan_slash");
            }
        } else if (triggerEvent == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card != NULL && damage.card->hasFlag("nuzhan_slash")) {
                if (damage.from != NULL)
                    room->sendCompulsoryTriggerLog(damage.from, objectName(), true);

                ++damage.damage;
				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					data = QVariant::fromValue(damage);
					room->removePlayerMark(player, objectName()+"engine");
				}
            }
        }
        return false;
    }
};

/*
class Nuzhan_tms : public TargetModSkill {
public:
    Nuzhan_tms() : TargetModSkill("#nuzhan") {

    }

    virtual int getResidueNum(const Player *from, const Card *card) const {
        if (from->hasSkill("nuzhan")) {
            if ((card->isVirtualCard() && card->subcardsLength() == 1 && Sanguosha->getCard(card->getSubcards().first())->isKindOf("TrickCard")) || card->hasFlag("Global_SlashAvailabilityChecker"))
                return 1000;
        }

        return 0;
    }
};
*/

class JspDanqi : public PhaseChangeSkill {
public:
    JspDanqi() : PhaseChangeSkill("jspdanqi") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *guanyu, Room *room) const{
        return PhaseChangeSkill::triggerable(guanyu) && guanyu->getMark(objectName()) == 0 && guanyu->getPhase() == Player::Start && guanyu->getHandcardNum() > guanyu->getHp() && !lordIsLiubei(room);
    }

    virtual bool onPhaseChange(ServerPlayer *target) const {
        Room *room = target->getRoom();
        
        //room->doLightbox("$JspdanqiAnimate");
		room->addPlayerMark(target, objectName()+"engine");
		if (target->getMark(objectName()+"engine") > 0) {
			room->setPlayerMark(target, objectName(), 1);
			if (room->changeMaxHpForAwakenSkill(target) && target->getMark(objectName()) > 0)
				room->handleAcquireDetachSkills(target, "mashu|nuzhan");

			room->removePlayerMark(target, objectName()+"engine");
		}
        return false;
    }

private:
    static bool lordIsLiubei(const Room *room) {
        if (room->getLord() != NULL) {
            const ServerPlayer *const lord = room->getLord();
            if (lord->getGeneral() && lord->getGeneralName().contains("liubei"))
                return true;

            if (lord->getGeneral2() && lord->getGeneral2Name().contains("liubei"))
                return true;
        }

        return false;
    }
};

class Kunfen : public PhaseChangeSkill
{
public:
    Kunfen() : PhaseChangeSkill("kunfen")
    {

    }

    virtual Frequency getFrequency(const Player *target) const
    {
        if (target != NULL) {
            return target->getMark("fengliang") > 0 ? NotFrequent : Compulsory;
        }

        return Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Finish;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        if (invoke(target))
            effect(target);

        return false;
    }

private:
    bool invoke(ServerPlayer *target) const
    {
        return getFrequency(target) == Compulsory ? true : target->askForSkillInvoke(objectName());
    }

    void effect(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        room->broadcastSkillInvoke(objectName(), getFrequency(target) == Compulsory? 1 : 2);
		if (getFrequency(target) == Compulsory)
            room->sendCompulsoryTriggerLog(target, objectName());
		room->addPlayerMark(target, objectName()+"engine");
		if (target->getMark(objectName()+"engine") > 0) {
			room->loseHp(target);
			if (target->isAlive())
				target->drawCards(2, objectName());
			room->removePlayerMark(target, objectName()+"engine");
		}
	}
};

class Fengliang : public TriggerSkill
{
public:
    Fengliang() : TriggerSkill("fengliang")
    {
        frequency = Wake;
        events << EnterDying;
    }

     virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getMark(objectName()) == 0;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
		DyingStruct dying = data.value<DyingStruct>();
        if (dying.who != player)
            return false;

        room->broadcastSkillInvoke(objectName());
        //room->doLightbox("$FengliangAnimate");

		room->addPlayerMark(player, objectName()+"engine");
		if (player->getMark(objectName()+"engine") > 0) {
			room->addPlayerMark(player, objectName(), 1);
			if (room->changeMaxHpForAwakenSkill(player) && player->getMark(objectName()) > 0) {
				int recover = 2 - player->getHp();
				room->recover(player, RecoverStruct(player, NULL, recover));
				room->handleAcquireDetachSkills(player, "tiaoxin");

				Json::Value up = QSanProtocol::Utils::toJsonString("kunfen");
				room->doNotify(player, QSanProtocol::S_COMMAND_UPDATE_SKILL, up);
			}
			room->removePlayerMark(player, objectName()+"engine");
		}

        return false;
    }
};

class Chixin : public OneCardViewAsSkill
{  // Slash::isSpecificAssignee
public:
    Chixin() : OneCardViewAsSkill("chixin")
    {
        filter_pattern = ".|diamond";
		response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "jink" || pattern == "slash";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        //CardUseStruct::CardUseReason r = Sanguosha->currentRoomState()->getCurrentCardUseReason();
        QString p = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        Card *c = NULL;
        if (p == "jink")
            c = new Jink(Card::SuitToBeDecided, -1);
        else
            c = new Slash(Card::SuitToBeDecided, -1);

        if (c == NULL)
            return NULL;

        c->setSkillName(objectName());
        c->addSubcard(originalCard);
        return c;
    }
};

class ChixinTrigger : public TriggerSkill
{
public:
    ChixinTrigger() : TriggerSkill("chixin")
    {
        events << PreCardUsed << EventPhaseEnd;
        view_as_skill = new Chixin;
        global = true;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual int getPriority(TriggerEvent) const
    {
        return 8;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->isKindOf("Slash") && player->getPhase() == Player::Play) {
                QSet<QString> s = player->property("chixin").toString().split("+").toSet();
                foreach(ServerPlayer *p, use.to)
                    s.insert(p->objectName());

                QStringList l = s.toList();
                room->setPlayerProperty(player, "chixin", l.join("+"));
            }
        } else if (player->getPhase() == Player::Play)
            room->setPlayerProperty(player, "chixin", QString());

        return false;
    }
};

class Suiren : public PhaseChangeSkill
{
public:
    Suiren() : PhaseChangeSkill("suiren")
    {
        frequency = Limited;
        limit_mark = "@suiren";
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::Start && target->getMark("@suiren") > 0;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
		if (!player->hasSkill("yicong")) return false;
        Room *room = player->getRoom();
        ServerPlayer *p = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@suiren-draw", true);
        if (p == NULL)
            return false;

        LogMessage log;
        log.type = "#InvokeSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(player, objectName());
        room->broadcastSkillInvoke(objectName());
		//room->doLightbox("$SuirenAnimate");
		room->addPlayerMark(player, objectName()+"engine");
		if (player->getMark(objectName()+"engine") > 0) {
			room->setPlayerMark(player, "@suiren", 0);
			
			room->handleAcquireDetachSkills(player, "-yicong");
			int maxhp = player->getMaxHp() + 1;
			room->setPlayerProperty(player, "maxhp", maxhp);
			room->recover(player, RecoverStruct());

			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());
			p->drawCards(3, objectName());
			room->removePlayerMark(player, objectName()+"engine");
		}

        return false;
    }
};

class Yinqin : public PhaseChangeSkill {
public:
    Yinqin() : PhaseChangeSkill("yinqin") {

    }

    virtual bool triggerable(const ServerPlayer *target) const {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const {
        Room *room = target->getRoom();
        QString kingdom = target->getKingdom() == "wei" ? "shu" : target->getKingdom() == "shu" ? "wei" : "wei+shu";
        if (target->askForSkillInvoke(objectName())) {
            kingdom = room->askForChoice(target, objectName(), kingdom);
            room->broadcastSkillInvoke(objectName());
			room->addPlayerMark(target, objectName()+"engine");
			if (target->getMark(objectName()+"engine") > 0) {
				room->setPlayerProperty(target, "kingdom", kingdom);
				room->removePlayerMark(target, objectName()+"engine");
			}
        }

        return false;
    }
};

class TWBaobian : public TriggerSkill {
public:
    TWBaobian() : TriggerSkill("twbaobian") {
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card != NULL && (damage.card->isKindOf("Slash") || damage.card->isKindOf("Duel")) && !damage.chain && !damage.transfer && damage.by_user) {
            if (damage.to->getKingdom() == player->getKingdom()) {
                if (player->askForSkillInvoke(objectName(), QVariant::fromValue(damage.to))) {
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());
                    if (damage.to->getHandcardNum() < damage.to->getMaxHp()) {
                        room->broadcastSkillInvoke(objectName(), 1);
                        int n = damage.to->getMaxHp() - damage.to->getHandcardNum();
						room->addPlayerMark(player, objectName()+"engine");
						if (player->getMark(objectName()+"engine") > 0) {
							room->drawCards(damage.to, n, objectName());
							room->removePlayerMark(player, objectName()+"engine");
						}
                    }
                    return true;
                }
             } else if (damage.to->getHandcardNum() > qMax(damage.to->getHp(), 0) && player->canDiscard(damage.to, "h")) {
                // Seems it is no need to use FakeMoveSkill & Room::askForCardChosen, so we ignore it.
                // If PlayerCardBox has changed for Room::askForCardChosen, please tell me, I will soon fix this.
                if (player->askForSkillInvoke(objectName(), QVariant::fromValue(damage.to))) {
                    room->broadcastSkillInvoke(objectName(), 2);
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());
					room->addPlayerMark(player, objectName()+"engine");
					if (player->getMark(objectName()+"engine") > 0) {
						QList<int> hc = damage.to->handCards();
						qShuffle(hc);
						int n = damage.to->getHandcardNum() - qMax(damage.to->getHp(), 0);
						QList<int> to_discard = hc.mid(0, n - 1);
						DummyCard dc(to_discard);
						room->throwCard(&dc, damage.to, player);
						room->removePlayerMark(player, objectName()+"engine");
					}
                }
            }
        }

        return false;
    }
};

class Tijin : public TriggerSkill {
public:
    Tijin() : TriggerSkill("tijin") {
        events << TargetSpecifying << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == TargetSpecifying) {
            if (use.from != NULL && use.card != NULL && use.card->isKindOf("Slash") && use.to.length() == 1) {
                ServerPlayer *zumao = room->findPlayerBySkillName(objectName());
                if (!TriggerSkill::triggerable(zumao) || use.from == zumao || !use.from->inMyAttackRange(zumao))
                    return false;

                if (!use.from->tag.value("tijin").canConvert(QVariant::Map))
                    use.from->tag["tijin"] = QVariantMap();

                QVariantMap tijin_map = use.from->tag.value("tijin").toMap();
                if (tijin_map.contains(use.card->toString())) {
                    tijin_map.remove(use.card->toString());
                    use.from->tag["tijin"] = tijin_map;
                }

                if (zumao->askForSkillInvoke(objectName(), data)) {
					room->broadcastSkillInvoke(objectName());
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), zumao->objectName());
					room->addPlayerMark(zumao, objectName()+"engine");
					if (zumao->getMark(objectName()+"engine") > 0) {
						use.to.first()->removeQinggangTag(use.card);
						use.to.clear();
						use.to << zumao;

						data = QVariant::fromValue(use);

						tijin_map[use.card->toString()] = QVariant::fromValue(zumao);
						use.from->tag["tijin"] = tijin_map;
						room->removePlayerMark(zumao, objectName()+"engine");
					}
                }
            }
        } else {
            if (use.from != NULL && use.card != NULL) {
                QVariantMap tijin_map = use.from->tag.value("tijin").toMap();
                if (tijin_map.contains(use.card->toString())) {
                    ServerPlayer *zumao = tijin_map.value(use.card->toString()).value<ServerPlayer *>();
                    if (zumao != NULL && zumao->canDiscard(use.from, "he")) {
                        int id = room->askForCardChosen(zumao, use.from, "he", objectName(), false, Card::MethodDiscard);
                        room->throwCard(id, use.from, zumao);
                    }
                }
                tijin_map.remove(use.card->toString());
                use.from->tag["tijin"] = tijin_map;
            }

        }

        return false;
    }
};

class Xiaolian : public TriggerSkill {
public:
    Xiaolian() : TriggerSkill("xiaolian") {
        events << TargetConfirming << Damaged;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->isKindOf("Slash") && use.to.length() == 1) {
                ServerPlayer *caoang = room->findPlayerBySkillName(objectName());
                if (!TriggerSkill::triggerable(caoang) || use.to.first() == caoang)
                    return false;

                if (!caoang->tag.value("xiaolian").canConvert(QVariant::Map))
                    caoang->tag["xiaolian"] = QVariantMap();

                QVariantMap xiaolian_map = caoang->tag.value("xiaolian").toMap();
                if (xiaolian_map.contains(use.card->toString())) {
                    xiaolian_map.remove(use.card->toString());
                    caoang->tag["xiaolian"] = xiaolian_map;
                }

                if (caoang->askForSkillInvoke(objectName(), data)) {
					room->broadcastSkillInvoke(objectName());
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), caoang->objectName());
					room->addPlayerMark(caoang, objectName()+"engine");
					if (caoang->getMark(objectName()+"engine") > 0) {
						ServerPlayer *target = use.to.first();
						use.to.first()->removeQinggangTag(use.card);
						use.to.clear();
						use.to << caoang;

						data = QVariant::fromValue(use);

						xiaolian_map[use.card->toString()] = QVariant::fromValue(target);
						caoang->tag["xiaolian"] = xiaolian_map;
						room->removePlayerMark(caoang, objectName()+"engine");
					}
                }
            }
        } else {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card != NULL) {
                if (!player->tag.value("xiaolian").canConvert(QVariant::Map))
                    return false;

                QVariantMap xiaolian_map = player->tag.value("xiaolian").toMap();
                if (xiaolian_map.contains(damage.card->toString())) {
                    ServerPlayer *target = xiaolian_map.value(damage.card->toString()).value<ServerPlayer *>();
                    if (target != NULL && player->getCardCount(true) > 0) {
                        const Card *c = room->askForExchange(player, objectName(), 1, 1, true, "@xiaolian-put", true);
                        if (c != NULL) 
                            target->addToPile("xlhorse", c);
						 delete c;
                    }
                }
                xiaolian_map.remove(damage.card->toString());
                player->tag["xiaolian"] = xiaolian_map;
            }
        }

        return false;
    }
};

class XiaolianDist : public DistanceSkill {
public:
    XiaolianDist() : DistanceSkill("#xiaolian-dist") {

    }

    virtual int getCorrect(const Player *from, const Player *to) const {
        if (from != to)
            return to->getPile("xlhorse").length();

        return 0;
    }
};

class Conqueror : public TriggerSkill {
public:
    Conqueror() : TriggerSkill("conqueror") {
        events << TargetSpecified;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card != NULL && use.card->isKindOf("Slash")) {
            int n = 0;
            foreach (ServerPlayer *target, use.to) {
                if (player->askForSkillInvoke(objectName(), QVariant::fromValue(target))) {
                    QString choice = room->askForChoice(player, objectName(), "BasicCard+EquipCard+TrickCard", QVariant::fromValue(target));
                    const Card *c = room->askForCard(target, choice, QString("@conqueror-exchange:%1::%2").arg(player->objectName()).arg(choice), choice, Card::MethodNone);
                    if (c != NULL) {
                        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), player->objectName(), objectName(), QString());
                        room->obtainCard(player, c, reason);
                        use.nullified_list << target->objectName();
                        data = QVariant::fromValue(use);
                    } else {
                        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
                        jink_list[n] = 0;
                        player->tag["Jink_" + use.card->toString()] = jink_list;
                        LogMessage log;
                        log.type = "#NoJink";
                        log.from = target;
                        room->sendLog(log);
                    }
                }
                ++n;
            }
        }
        return false;
    }
};





class OlZishou : public DrawCardsSkill
{
public:
    OlZishou() : DrawCardsSkill("olzishou")
    {

    }

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        Room *room = player->getRoom();
        if (player->askForSkillInvoke(objectName())) {
			room->broadcastSkillInvoke(objectName());
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				room->setPlayerFlag(player, "olzishou");

				QSet<QString> kingdomSet;
				foreach (ServerPlayer *p, room->getAlivePlayers())
					kingdomSet.insert(p->getKingdom());

				room->removePlayerMark(player, objectName()+"engine");
				n = n + kingdomSet.count();
			}
        }

        return n;
    }
};

class OlZishouProhibit : public ProhibitSkill
{
public:
    OlZishouProhibit() : ProhibitSkill("#olzishou")
    {

    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> & /* = QList<const Player *>() */) const
    {
        if (card->isKindOf("SkillCard"))
            return false;

        if (from->hasFlag("olzishou") && from->getPhase() == Player::Play)
            return from != to;

        return false;
    }
};

class OlShenxian : public TriggerSkill
{
public:
    OlShenxian() : TriggerSkill("olshenxian")
    {
        events << CardsMoveOneTime << EventPhaseStart;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->getMark(objectName()) > 0)
                        p->setMark(objectName(), 0);
                }
            }
        } else {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::NotActive && player->getMark(objectName()) == 0 && move.from && move.from->isAlive()
                && move.from->objectName() != player->objectName() && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                foreach (int id, move.card_ids) {
                    if (Sanguosha->getCard(id)->getTypeId() == Card::TypeBasic) {
                        if (room->askForSkillInvoke(player, objectName(), data)) {
                            room->broadcastSkillInvoke(objectName());
							room->addPlayerMark(player, objectName()+"engine");
							if (player->getMark(objectName()+"engine") > 0) {
								player->drawCards(1, "olshenxian");
								player->setMark(objectName(), 1);
								room->removePlayerMark(player, objectName()+"engine");
							}
                        }
                        break;
                    }
                }
            }
        }
        return false;
    }
};

class OlMeibu : public TriggerSkill
{
public:
    OlMeibu() : TriggerSkill("olmeibu")
    {
        events << EventPhaseStart << EventPhaseChanging << CardUsed;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    int getPriority(TriggerEvent triggerEvent) const
    {
        if (triggerEvent == CardUsed)
            return 6;

        return TriggerSkill::getPriority(triggerEvent);
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play) {
            foreach (ServerPlayer *sunluyu, room->getOtherPlayers(player)) {
                if (!player->inMyAttackRange(sunluyu) && TriggerSkill::triggerable(sunluyu) && sunluyu->askForSkillInvoke(objectName())) {
                    room->broadcastSkillInvoke(objectName());
					room->addPlayerMark(player, objectName()+"engine");
					if (player->getMark(objectName()+"engine") > 0) {
						if (!player->hasSkill("#olmeibu-filter", true)) {
							room->acquireSkill(player, "#olmeibu-filter", false);
							room->filterCards(player, player->getCards("he"), false);
						}
						QVariantList sunluyus = player->tag[objectName()].toList();
						sunluyus << QVariant::fromValue(sunluyu);
						player->tag[objectName()] = QVariant::fromValue(sunluyus);
						room->insertAttackRangePair(player, sunluyu);
						room->removePlayerMark(player, objectName()+"engine");
					}
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive) return false;

            QVariantList sunluyus = player->tag[objectName()].toList();
            foreach (const QVariant &sunluyu, sunluyus) {
                ServerPlayer *s = sunluyu.value<ServerPlayer *>();
                room->removeAttackRangePair(player, s);
            }

            player->tag[objectName()] = QVariantList();

            if (player->hasSkill("#olmeibu-filter", true)) {
                room->detachSkillFromPlayer(player, "#olmeibu-filter");
                room->filterCards(player, player->getCards("he"), true);
            }
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (player->hasSkill("#olmeibu-filter", true) && use.card != NULL && use.card->getSkillName() == "olmeibu") {
                room->detachSkillFromPlayer(player, "#olmeibu-filter");
                room->filterCards(player, player->getCards("he"), true);
            }
        }
        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("Slash"))
            return -2;

        return -1;
    }
};

OlMumuCard::OlMumuCard()
{

}

bool OlMumuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (targets.isEmpty()) {
        if (to_select->getWeapon() && Self->canDiscard(to_select, to_select->getWeapon()->getEffectiveId()))
            return true;
        if (to_select != Self && to_select->getArmor())
            return true;
    }

    return false;
}

void OlMumuCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    ServerPlayer *target = effect.to;
    ServerPlayer *player = effect.from;

    QStringList choices;
    if (target->getWeapon() && player->canDiscard(target, target->getWeapon()->getEffectiveId()))
        choices << "weapon";
    if (target != player && target->getArmor())
        choices << "armor";

    if (choices.length() == 0)
        return;

    Room *r = target->getRoom();
    QString choice = choices.length() == 1 ? choices.first() : r->askForChoice(player, "olmumu", choices.join("+"), QVariant::fromValue(target));

    room->addPlayerMark(effect.from, "olmumuengine");
    if (effect.from->getMark("olmumuengine") > 0) {
		if (choice == "weapon") {
			r->throwCard(target->getWeapon(), target, player == target ? NULL : player);
			player->drawCards(1, "olmumu");
		} else {
			int equip = target->getArmor()->getEffectiveId();
			QList<CardsMoveStruct> exchangeMove;
			CardsMoveStruct move1(equip, player, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_ROB, player->objectName()));
			exchangeMove.push_back(move1);
			if (player->getArmor()) {
				CardsMoveStruct move2(player->getArmor()->getEffectiveId(), NULL, Player::DiscardPile, CardMoveReason(CardMoveReason::S_REASON_CHANGE_EQUIP, player->objectName()));
				exchangeMove.push_back(move2);
			}
			r->moveCardsAtomic(exchangeMove, true);
		}
        room->removePlayerMark(effect.from, "olmumuengine");
    }
}

class OlMumu : public OneCardViewAsSkill
{
public:
    OlMumu() : OneCardViewAsSkill("olmumu")
    {
        filter_pattern = "Slash#TrickCard|black!";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        OlMumuCard *mm = new OlMumuCard;
        mm->addSubcard(originalCard);
        return mm;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("OlMumuCard");
    }
};

class OlPojun : public TriggerSkill
{
public:
    OlPojun() : TriggerSkill("olpojun")
    {
        events << TargetSpecified << EventPhaseStart << Death;;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
         if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->isKindOf("Slash") && TriggerSkill::triggerable(player) && player->getPhase() == Player::Play) {
                foreach (ServerPlayer *t, use.to) {
                    int n = qMin(t->getCards("he").length(), t->getHp());
                    if (n > 0 && player->askForSkillInvoke(objectName(), QVariant::fromValue(t))) {
                        room->broadcastSkillInvoke(objectName());
						room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), t->objectName());
						room->addPlayerMark(player, objectName()+"engine");
						if (player->getMark(objectName()+"engine") > 0) {
							QStringList dis_num;
							for (int i = 1; i <= n; ++i)
								dis_num << QString::number(i);

							int ad = Config.AIDelay;
							Config.AIDelay = 0;

							bool ok = false;
							int discard_n = room->askForChoice(player, objectName() + "_num", dis_num.join("+")).toInt(&ok);
							if (!ok || discard_n == 0) {
								Config.AIDelay = ad;
								continue;
							}

							QList<Player::Place> orig_places;
							QList<int> cards;
							// fake move skill needed!!!
							t->setFlags("olpojun_InTempMoving");

							for (int i = 0; i < discard_n; ++i) {
								int id = room->askForCardChosen(player, t, "he", objectName() + "_dis", false, Card::MethodNone);
								Player::Place place = room->getCardPlace(id);
								orig_places << place;
								cards << id;
								t->addToPile("#olpojun", id, false);
							}

							for (int i = 0; i < discard_n; ++i)
								room->moveCardTo(Sanguosha->getCard(cards.value(i)), t, orig_places.value(i), false);

							t->setFlags("-olpojun_InTempMoving");
							Config.AIDelay = ad;

							DummyCard dummy(cards);
							t->addToPile("olpojun", &dummy, false, QList<ServerPlayer *>() << t);

							// for record
							if (!t->tag.contains("olpojun") || !t->tag.value("olpojun").canConvert(QVariant::Map))
								t->tag["olpojun"] = QVariantMap();

							QVariantMap vm = t->tag["olpojun"].toMap();
							foreach (int id, cards)
								vm[QString::number(id)] = player->objectName();

							t->tag["olpojun"] = vm;
							room->removePlayerMark(player, objectName()+"engine");
						}
                    }
                }
            }
        } else if (cardGoBack(triggerEvent, player, data)) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->tag.contains("olpojun")) {
                    QVariantMap vm = p->tag.value("olpojun", QVariantMap()).toMap();
                    if (vm.values().contains(player->objectName())) {
                        QList<int> to_obtain;
                        foreach (const QString &key, vm.keys()) {
                            if (vm.value(key) == player->objectName())
                                to_obtain << key.toInt();
                        }

                        DummyCard dummy(to_obtain);
                        room->obtainCard(p, &dummy, false);

                        foreach (int id, to_obtain)
                            vm.remove(QString::number(id));

                        p->tag["olpojun"] = vm;
                    }
                }
            }
        }

        return false;
    }

private:
    static bool cardGoBack(TriggerEvent triggerEvent, ServerPlayer *player, const QVariant &data)
    {
        if (triggerEvent == EventPhaseStart)
            return player->getPhase() == Player::Finish;
        else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            return death.who == player;
        }

        return false;
    }
};

class OlZhixi : public TriggerSkill
{
public:
    OlZhixi() : TriggerSkill("olzhixi")
    {
        events << CardUsed << EventLoseSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    int getPriority(TriggerEvent triggerEvent) const
    {
        if (triggerEvent == CardUsed)
            return 6;

        return TriggerSkill::getPriority(triggerEvent);
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->isKindOf("TrickCard") && TriggerSkill::triggerable(player)) {
                if (!player->hasSkill("#olzhixi-filter", true)) {
                    room->acquireSkill(player, "#olzhixi-filter", false);
                    room->filterCards(player, player->getCards("he"), true);
                }
            }
        } else if (triggerEvent == EventLoseSkill) {
            QString name = data.toString();
            if (name == objectName()) {
                if (player->hasSkill("#olzhixi-filter", true)) {
                    room->detachSkillFromPlayer(player, "#olzhixi-filter");
                    room->filterCards(player, player->getCards("he"), true);
                }
            }
        }
        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *card)
    {
        if (card->isKindOf("Slash"))
            return -2;

        return -1;
    }
};

class OlMeibu2 : public TriggerSkill
{
public:
    OlMeibu2() : TriggerSkill("olmeibu2")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play) {
            foreach (ServerPlayer *sunluyu, room->getOtherPlayers(player)) {
                if (!player->inMyAttackRange(sunluyu) && TriggerSkill::triggerable(sunluyu) && sunluyu->askForSkillInvoke(objectName())) {
                    room->broadcastSkillInvoke(objectName());
                    if (!player->hasSkill("olzhixi", true))
                        room->acquireSkill(player, "olzhixi");
                    if (sunluyu->getMark("olmumu2") == 0) {
                        QVariantList sunluyus = player->tag[objectName()].toList();
                        sunluyus << QVariant::fromValue(sunluyu);
                        player->tag[objectName()] = QVariant::fromValue(sunluyus);
                        room->insertAttackRangePair(player, sunluyu);
                    }
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive) return false;

            QVariantList sunluyus = player->tag[objectName()].toList();
            foreach (const QVariant &sunluyu, sunluyus) {
                ServerPlayer *s = sunluyu.value<ServerPlayer *>();
                room->removeAttackRangePair(player, s);
            }

            player->tag[objectName()] = QVariantList();

            if (player->hasSkill("olzhixi", true))
                room->detachSkillFromPlayer(player, "olzhixi");
        }
        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("Slash"))
            return 2;

        return 1;
    }
};


OlMumu2Card::OlMumu2Card()
{

}

bool OlMumu2Card::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (targets.isEmpty() && !to_select->getEquips().isEmpty()) {
        QList<const Card *> equips = to_select->getEquips();
        foreach (const Card *e, equips) {
             if (to_select->getArmor() != NULL && to_select->getArmor()->getRealCard() == e->getRealCard())
                return true;

            if (Self->canDiscard(to_select, e->getEffectiveId()))
                return true;
        }
    }

    return false;
}

void OlMumu2Card::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *target = effect.to;
    ServerPlayer *player = effect.from;

    Room *r = target->getRoom();

    QList<int> disabled;
    foreach (const Card *e, target->getEquips()) {
         if (target->getArmor() != NULL && target->getArmor()->getRealCard() == e->getRealCard())
            continue;

        if (!player->canDiscard(target, e->getEffectiveId()))
            disabled << e->getEffectiveId();
    }

    int id = r->askForCardChosen(player, target, "e", "olmumu2", false, Card::MethodNone, disabled);

    QString choice = "discard";
   if (target->getArmor() != NULL && id == target->getArmor()->getEffectiveId()) {
        if (!player->canDiscard(target, id))
            choice = "obtain";
        else
            choice = r->askForChoice(player, "olmumu2", "discard+obtain", id);
    }

    r->addPlayerMark(effect.from, "olmumu2engine");
    if (effect.from->getMark("olmumu2engine") > 0) {
		if (choice == "discard") {
			r->throwCard(Sanguosha->getCard(id), target, player == target ? NULL : player);
			player->drawCards(1, "olmumu2");
		} else
			r->obtainCard(player, id);


		int used_id = subcards.first();
		const Card *c = Sanguosha->getCard(used_id);
		if (c->isKindOf("Slash") || (c->isBlack() && c->isKindOf("TrickCard")))
			player->addMark("olmumu2");
        r->removePlayerMark(effect.from, "olmumu2engine");
    }
}

class OlMumu2VS : public OneCardViewAsSkill
{
public:
    OlMumu2VS() : OneCardViewAsSkill("olmumu2")
    {
        filter_pattern = ".!";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("OlMumu2Card");
    }

    const Card *viewAs(const Card *originalCard) const
    {
        OlMumu2Card *mm = new OlMumu2Card;
        mm->addSubcard(originalCard);
        return mm;
    }
};

class OlMumu2 : public PhaseChangeSkill
{
public:
    OlMumu2() : PhaseChangeSkill("olmumu2")
    {
        view_as_skill = new OlMumu2VS;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getPhase() == Player::RoundStart;
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        target->setMark("olmumu2", 0);

        return false;
    }
};

class Biluan : public PhaseChangeSkill
{
public:
    Biluan() : PhaseChangeSkill("biluan")
    {

    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
		Room *room = target->getRoom();
        switch (target->getPhase()) {
        case (Player::Draw) :
            if (PhaseChangeSkill::triggerable(target) && target->askForSkillInvoke(objectName())) {
				room->addPlayerMark(target, objectName()+"engine");
				if (target->getMark(objectName()+"engine") > 0) {
                    room->broadcastSkillInvoke(objectName());
					room->setPlayerMark(target, "biluan", 1);
					room->removePlayerMark(target, objectName()+"engine");
				}
                return true;
            }
            break;
        case (Player::RoundStart) :
            room->setPlayerMark(target, "biluan", 0);
            break;
        default:

            break;
        }

        return false;
    }
};

class BiluanDist : public DistanceSkill
{
public:
    BiluanDist() : DistanceSkill("#biluan-dist")
    {

    }

    int getCorrect(const Player *, const Player *to) const
    {
        if (to->getMark("biluan") == 1) {
            QList<const Player *> sib = to->getAliveSiblings();
            sib << to;
            QSet<QString> kingdoms;
            foreach (const Player *p, sib)
                kingdoms.insert(p->getKingdom());

            return kingdoms.count();
        }

        return 0;
    }
};

class Lixia : public PhaseChangeSkill
{
public:
    Lixia() : PhaseChangeSkill("lixia")
    {

    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        Room *r = target->getRoom();
        switch (target->getPhase()) {
        case (Player::Finish) : {
            QList<ServerPlayer *> misterious1s = r->getOtherPlayers(target);
            foreach (ServerPlayer *misterious1, misterious1s) {
                if (TriggerSkill::triggerable(misterious1) && !target->inMyAttackRange(misterious1) && misterious1->askForSkillInvoke(objectName(), QVariant::fromValue(target))) {
                    r->broadcastSkillInvoke(objectName());
					r->addPlayerMark(target, objectName()+"engine");
					if (target->getMark(objectName()+"engine") > 0) {
						misterious1->drawCards(1, objectName());
						r->addPlayerMark(misterious1, "lixia", 1);
						r->removePlayerMark(target, objectName()+"engine");
					}
                }
            }
        }
            break;
        case (Player::RoundStart) :
            target->getRoom()->setPlayerMark(target, "lixia", 0);
            break;
        default:

            break;
        }

        return false;
    }
};

class LixiaDist : public DistanceSkill
{
public:
    LixiaDist() : DistanceSkill("#lixia-dist")
    {

    }

    int getCorrect(const Player *, const Player *to) const
    {
        return -to->getMark("lixia");
    }
};

class Yishe : public TriggerSkill
{
public:
    Yishe() : TriggerSkill("yishe")
    {
       events << EventPhaseStart << CardsMoveOneTime;
        frequency = Frequent;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPile("rice").isEmpty() && player->getPhase() == Player::Finish) {
                if (player->askForSkillInvoke(objectName())) {
					room->broadcastSkillInvoke(objectName());
					room->addPlayerMark(player, objectName()+"engine");
					if (player->getMark(objectName()+"engine") > 0) {
						player->drawCards(2, objectName());
						if (!player->isNude()) {
							const Card *dummy = NULL;
							if (player->getCardCount(true) <= 2) {
								DummyCard *dummy2 = new DummyCard;
								dummy2->addSubcards(player->getCards("he"));
								dummy2->deleteLater();
								dummy = dummy2;
							} else
								dummy = room->askForExchange(player, objectName(), 2, 2, true, "@yishe");

							player->addToPile("rice", dummy);
							delete dummy;
						}
						room->removePlayerMark(player, objectName()+"engine");
					}
                }
            }
        } else {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && move.from_pile_names.contains("rice") && move.from->getPile("rice").isEmpty()) {
				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					room->recover(player, RecoverStruct(player));
					room->removePlayerMark(player, objectName()+"engine");
				}
			}
        }

        return false;
    }
};

BushiCard::BushiCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
    mute = true;
}

void BushiCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    // log

    room->addPlayerMark(card_use.from, "engine");
    if (card_use.from->getMark("engine") > 0) {
		card_use.from->obtainCard(this);
        room->removePlayerMark(card_use.from, "engine");
    }
}

class BushiVS : public OneCardViewAsSkill
{
public:
    BushiVS() : OneCardViewAsSkill("bushi")
    {
        response_pattern = "@@bushi";
        filter_pattern = ".|.|.|rice,%rice";
        expand_pile = "rice,%rice";
    }

    const Card *viewAs(const Card *card) const
    {
        BushiCard *bs = new BushiCard;
        bs->addSubcard(card);
        return bs;
    }
};

class Bushi : public TriggerSkill
{
public:
    Bushi() : TriggerSkill("bushi")
    {
        events << Damage << Damaged;
        view_as_skill = new BushiVS;
    }

    bool trigger(TriggerEvent e, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getPile("rice").isEmpty())
            return false;

        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *p = damage.to;

        if (damage.from->isDead() || damage.to->isDead())
            return false;

        if (e == Damage && damage.to == player)
            return false;
            
        for (int i = 0; i < damage.damage; ++i) {
            if (!room->askForUseCard(p, "@@bushi", "@bushi", -1, Card::MethodNone))
                break;
			if (p == damage.from)
				room->broadcastSkillInvoke(objectName(), 2);
			else
				room->broadcastSkillInvoke(objectName(), 1);
            if (p->isDead() || player->getPile("rice").isEmpty())
                break;
        }

        return false;
    }
};



MidaoCard::MidaoCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void MidaoCard::onUse(Room *, const CardUseStruct &card_use) const
{
    card_use.from->tag["midao"] = subcards.first();
}

class MidaoVS : public OneCardViewAsSkill
{
public:
    MidaoVS() : OneCardViewAsSkill("midao")
    {
        response_pattern = "@@midao";
        filter_pattern = ".|.|.|rice";
        expand_pile = "rice";
    }

    const Card *viewAs(const Card *card) const
    {
        MidaoCard *bs = new MidaoCard;
        bs->addSubcard(card);
        return bs;
    }
};

class Midao : public RetrialSkill
{
public:
    Midao() : RetrialSkill("midao", false)
    {
        view_as_skill = new MidaoVS;
    }

    const Card *onRetrial(ServerPlayer *player, JudgeStruct *judge) const
    {
        if (player->getPile("rice").isEmpty())
            return NULL;

        QStringList prompt_list;
        prompt_list << "@midao-card" << judge->who->objectName()
            << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");

        player->tag.remove("midao");
        player->tag["judgeData"] = QVariant::fromValue(judge);
        Room *room = player->getRoom();
        bool invoke = room->askForUseCard(player, "@@midao", prompt, -1, Card::MethodNone);
        player->tag.remove("judgeData");
        if (invoke && player->tag.contains("midao")) {
			room->broadcastSkillInvoke(objectName());
            int id = player->tag.value("midao", player->getPile("rice").first()).toInt();
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				room->removePlayerMark(player, objectName()+"engine");
				return Sanguosha->getCard(id);
			}
        }

        return NULL;
    }
};

class Chenqing : public TriggerSkill
{
public:
    Chenqing() : TriggerSkill("chenqing")
    {
        events << AskForPeaches;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        ServerPlayer *current = room->getCurrent();
        if (current == NULL || current->getPhase() == Player::NotActive || current->isDead())
            return false;
        if (current->getMark("chenqing-Clear") > 0 || player->getMark("chenqing_start") > 0) return false;

        DyingStruct dying = data.value<DyingStruct>();

        QList<ServerPlayer *> players = room->getOtherPlayers(player);
        players.removeAll(dying.who);
        ServerPlayer *target = room->askForPlayerChosen(player, players, objectName(), "ChenqingAsk", true, true);
        if (target && target->isAlive()) {
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				if (Config.value("chenqing_down", true).toBool())
					room->addPlayerMark(player, "chenqing_start");
				else
					room->addPlayerMark(current, "chenqing-Clear");
				target->drawCards(4, objectName());
				const Card *card = NULL;
				if (target->getCardCount() < 4) { // for limit broken xiahoudun && trashbin!!!!!!!!!!!!!
					DummyCard *dm = new DummyCard;
					dm->addSubcards(target->getCards("he"));
					card = dm;
				} else
					card = room->askForExchange(target, "Chenqing", 4, 4, false, "ChenqingDiscard");
				QSet<Card::Suit> suit;
				foreach (int id, card->getSubcards()) {
					const Card *c = Sanguosha->getCard(id);
					if (c == NULL) continue;
					suit.insert(c->getSuit());
				}
				room->throwCard(card, target);
				delete card;

				if (suit.count() == 4 && room->getCurrentDyingPlayer() == dying.who)
					room->useCard(CardUseStruct(Sanguosha->cloneCard("peach"), target, dying.who, false), true);

				room->removePlayerMark(player, objectName()+"engine");
			}
        }
        return false;
    }
};

class OldMoshiViewAsSkill : public OneCardViewAsSkill
{
public:
    OldMoshiViewAsSkill() : OneCardViewAsSkill("Omoshi")
    {
        filter_pattern = ".";
        response_or_use = true;
        response_pattern = "@@Omoshi";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        const Card *ori = Self->tag[objectName()].value<const Card *>();
        if (ori == NULL) return NULL;
        Card *a = Sanguosha->cloneCard(ori->objectName());
        a->addSubcard(originalCard);
        a->setSkillName(objectName());
        return a;

    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }
};

class OldMoshi : public TriggerSkill  //a souvenir for Xusine......
{
public:
    OldMoshi() : TriggerSkill("Omoshi")
    {
        view_as_skill = new OldMoshiViewAsSkill;
        events << EventPhaseStart << CardUsed;
    }

    QDialog *getDialog() const
    {
        return GuhuoDialog::getInstance(objectName(), true, true, false, false, true);
    }

    bool trigger(TriggerEvent e, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (e == CardUsed && player->getPhase() == Player::Play) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("SkillCard") || use.card->isKindOf("EquipCard")) return false;
            QStringList list = player->tag[objectName()].toStringList();
            list.append(use.card->objectName());
            player->tag[objectName()] = list;
        } else if (e == EventPhaseStart && player->getPhase() == Player::Finish) {
            QStringList list = player->tag[objectName()].toStringList();
            player->tag.remove(objectName());
            if (list.isEmpty()) return false;
            room->setPlayerProperty(player, "allowed_guhuo_dialog_buttons", list.join("+"));
            try {
                const Card *first = room->askForUseCard(player, "@@Omoshi", "@moshi_ask_first");
                if (first) {
                    list.removeOne(first->objectName());
                    room->setPlayerProperty(player, "allowed_guhuo_dialog_buttons", list.join("+"));
                    room->askForUseCard(player, "@@Omoshi", "@moshi_ask_second");
                }
            }
            catch (TriggerEvent e) {
                if (e == TurnBroken || e == StageChange) {
                    room->setPlayerProperty(player, "allowed_guhuo_dialog_buttons", QString());
                }
                throw e;
            }
        }
        return false;
    }
};


class MoshiViewAsSkill : public OneCardViewAsSkill
{
public:
    MoshiViewAsSkill() : OneCardViewAsSkill("moshi")
    {
        response_or_use = true;
        response_pattern = "@@moshi";
    }

    bool viewFilter(const Card *to_select) const
    {
        if (to_select->isEquipped()) return false;
        QString ori = Self->property("moshi").toString();
        if (ori.isEmpty()) return NULL;
        Card *a = Sanguosha->cloneCard(ori);
        a->addSubcard(to_select);
        return a->isAvailable(Self);
    }

    const Card *viewAs(const Card *originalCard) const
    {
        QString ori = Self->property("moshi").toString();
        if (ori.isEmpty()) return NULL;
        Card *a = Sanguosha->cloneCard(ori);
        a->addSubcard(originalCard);
        a->setSkillName(objectName());
        return a;
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }
};


class Moshi : public TriggerSkill
{
public:
    Moshi() : TriggerSkill("moshi")
    {
        view_as_skill = new MoshiViewAsSkill;
        events << EventPhaseStart << CardUsed;
    }
    bool trigger(TriggerEvent e, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (e == CardUsed && player->getPhase() == Player::Play) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("SkillCard") || use.card->isKindOf("EquipCard") || use.card->isKindOf("DelayedTrick")) return false;
            QStringList list = player->tag[objectName()].toStringList();
            if (list.length() == 2) return false;
            list.append(use.card->objectName());
            player->tag[objectName()] = list;
        } else if (e == EventPhaseStart && player->getPhase() == Player::Finish) {
            QStringList list = player->tag[objectName()].toStringList();
            player->tag.remove(objectName());
            if (list.isEmpty()) return false;
            room->setPlayerProperty(player, "moshi", list.first());
            try {
                const Card *first = room->askForUseCard(player, "@@moshi", QString("@moshi_ask:::%1").arg(list.takeFirst()));
                if (first != NULL && !list.isEmpty() && !(player->isKongcheng() && player->getHandPile().isEmpty())) {
                    room->setPlayerProperty(player, "moshi", list.first());
                    Q_ASSERT(list.length() == 1);
                    room->askForUseCard(player, "@@moshi", QString("@moshi_ask:::%1").arg(list.takeFirst()));
                }
            }
            catch (TriggerEvent e) {
                if (e == TurnBroken || e == StageChange) {
                    room->setPlayerProperty(player, "moshi", QString());
                }
                throw e;
            }
        }
        return false;
    }
};



class FengpoRecord : public TriggerSkill
{
public:
    FengpoRecord() : TriggerSkill("#fengpo-record")
    {
        events << EventPhaseChanging << PreCardUsed << CardResponded;
        global = true;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::Play)
                room->setPlayerFlag(player, "-fengporec");
        } else {
            if (player->getPhase() != Player::Play)
                return false;

            const Card *c = NULL;
            if (triggerEvent == PreCardUsed)
                c = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    c = resp.m_card;
            }

            if (c == NULL || c->isKindOf("SkillCard"))
                return false;
            
            if (triggerEvent == PreCardUsed && !player->hasFlag("fengporec"))
                room->setCardFlag(c, "fengporecc");


            if (!Config.value("fengpo_down", true).toBool() || c->isKindOf("Slash") || c->isKindOf("Duel"))
                room->setPlayerFlag(player, "fengporec");
        }

        return false;
    }
};

class Fengpo : public TriggerSkill
{
public:
    Fengpo() : TriggerSkill("fengpo")
    {
        events << TargetSpecified << ConfirmDamage << CardFinished;
    }

    bool trigger(TriggerEvent e, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (e == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.to.length() != 1) return false;
            if (use.to.first()->isKongcheng()) return false;
            if (use.card == NULL) return false;
            if (!use.card->isKindOf("Slash") && !use.card->isKindOf("Duel")) return false;
            if (!use.card->hasFlag("fengporecc")) return false;
            QStringList choices;
            int n = 0;
            foreach (const Card *card, use.to.first()->getHandcards())
                if (card->getSuit() == Card::Diamond)
                    ++n;
            choices << "drawCards" << "addDamage" << "cancel";
            QString choice = room->askForChoice(player, objectName(), choices.join("+"));
            if (choice != "cancel") {
                room->notifySkillInvoked(player, objectName());
                room->broadcastSkillInvoke(objectName(), choice == "drawCards" ? 1 : 2);

                LogMessage log;
                log.type = "#InvokeSkill";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), use.to.first()->objectName());
				if (choice == "drawCards")
					player->drawCards(n);
				else if (choice == "addDamage")
					player->tag["fengpoaddDamage" + use.card->toString()] = n;
			}
       } else if (e == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card == NULL || damage.from == NULL)
                return false;
            if (damage.from->tag.contains("fengpoaddDamage" + damage.card->toString()) && (damage.card->isKindOf("Slash") || damage.card->isKindOf("Duel"))) {
                damage.damage += damage.from->tag.value("fengpoaddDamage" + damage.card->toString()).toInt();
                data = QVariant::fromValue(damage);
                damage.from->tag.remove("fengpoaddDamage" + damage.card->toString());
            }
        } else if (e == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.to.length() != 1) return false;
            if (use.to.first()->isKongcheng()) return false;
            if (!use.card->isKindOf("Slash") || !use.card->isKindOf("Duel")) return false;
            if (player->tag.contains("fengpoaddDamage" + use.card->toString()))
                player->tag.remove("fengpoaddDamage" + use.card->toString());
        }
        return false;
    }
};
JiqiaoCard::JiqiaoCard()
{
    target_fixed = true;
}

void JiqiaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->addPlayerMark(source, "engine");
    if (source->getMark("engine") > 0) {
		int n = subcardsLength() * 2;
		QList<int> card_ids = room->getNCards(n, false);
		CardMoveReason reason1(CardMoveReason::S_REASON_TURNOVER, source->objectName(), "jiqiao", QString());
		CardsMoveStruct move(card_ids, NULL, Player::PlaceTable, reason1);
		room->moveCardsAtomic(move, true);
		room->getThread()->delay();
		room->getThread()->delay();
		
		DummyCard get;
		DummyCard thro;

		foreach (int id, card_ids) {
			const Card *c = Sanguosha->getCard(id);
			if (c == NULL)
				continue;

			if (c->isKindOf("TrickCard"))
				get.addSubcard(c);
			else
				thro.addSubcard(c);
		}

		if (get.subcardsLength() > 0)
			source->obtainCard(&get);

		if (thro.subcardsLength() > 0) {
			CardMoveReason reason2(CardMoveReason::S_REASON_NATURAL_ENTER, QString(), "jiqiao", QString());
			room->throwCard(&thro, reason2, NULL);
		}
        room->removePlayerMark(source, "engine");
    }
}

class JiqiaoVS : public ViewAsSkill
{
public:
    JiqiaoVS() : ViewAsSkill("jiqiao")
    {
        response_pattern = "@@jiqiao";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return to_select->isKindOf("EquipCard") && !Self->isJilei(to_select);
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 0)
            return NULL;

        JiqiaoCard *jq = new JiqiaoCard;
        jq->addSubcards(cards);
        return jq;
    }
};

class Jiqiao : public PhaseChangeSkill
{
public:
    Jiqiao() : PhaseChangeSkill("jiqiao")
    {
        view_as_skill = new JiqiaoVS;
    }

    bool onPhaseChange(ServerPlayer *player) const
    {
        if (player->getPhase() != Player::Play)
            return false;

        if (!player->canDiscard(player, "he"))
            return false;

        player->getRoom()->askForUseCard(player, "@@jiqiao", "@jiqiao", -1, Card::MethodDiscard);

        return false;
    }
};

class Linglong : public TriggerSkill
{
public:
    Linglong() : TriggerSkill("linglong")
    {
        frequency = Compulsory;
        events << CardAsked;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getArmor() == NULL && target->hasArmorEffect("eight_diagram");
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        QString pattern = data.toStringList().first();

        if (pattern != "jink")
            return false;

        if (player->askForSkillInvoke("eight_diagram")) {
            room->broadcastSkillInvoke(objectName());
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				JudgeStruct judge;
				judge.pattern = ".|red";
				judge.good = true;
				judge.reason = "eight_diagram";
				judge.who = player;

				room->judge(judge);

				if (judge.isGood()) {
					room->setEmotion(player, "armor/eight_diagram");
					Jink *jink = new Jink(Card::NoSuit, 0);
					jink->setSkillName("eight_diagram");
					room->provide(jink);
					return true;
				}
				room->removePlayerMark(player, objectName()+"engine");
			}
        }

        return false;
    }
};

class LinglongMax : public MaxCardsSkill
{
public:
    LinglongMax() : MaxCardsSkill("#linglong-horse")
    {

    }

    int getExtra(const Player *target) const
    {
        if (target->hasSkill("linglong") && target->getDefensiveHorse() == NULL && target->getOffensiveHorse() == NULL)
            return 1;

        return 0;
    }
};

class LinglongTreasure : public TriggerSkill
{
public:
    LinglongTreasure() : TriggerSkill("#linglong-treasure")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << CardsMoveOneTime;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventLoseSkill && data.toString() == "linglong") {
            room->handleAcquireDetachSkills(player, "-qicai", true);
            player->setMark("linglong_qicai", 0);
        } else if ((triggerEvent == EventAcquireSkill && data.toString() == "linglong") || (triggerEvent == GameStart && TriggerSkill::triggerable(player))) {
            if (player->getTreasure() == NULL) {
                room->notifySkillInvoked(player, objectName());
                room->handleAcquireDetachSkills(player, "qicai");
                player->setMark("linglong_qicai", 1);
            }
        } else if (triggerEvent == CardsMoveOneTime && player->isAlive() && player->hasSkill("linglong", true)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && move.from_places.contains(Player::PlaceEquip)) {
                if (player->getTreasure() == NULL && player->getMark("linglong_qicai") == 0) {
                    room->notifySkillInvoked(player, objectName());
                    room->handleAcquireDetachSkills(player, "qicai");
                    player->setMark("linglong_qicai", 1);
                }
            } else if (move.to == player && move.to_place == Player::PlaceEquip) {
                if (player->getTreasure() != NULL && player->getMark("linglong_qicai") == 1) {
                    room->handleAcquireDetachSkills(player, "-qicai", true);
                    player->setMark("linglong_qicai", 0);
                }
            }
        }
        return false;
    }
};

class OlMiji : public TriggerSkill
{
public:
    OlMiji() : TriggerSkill("olmiji")
    {
        events << EventPhaseStart << ChoiceMade;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        if (TriggerSkill::triggerable(target) && triggerEvent == EventPhaseStart
            && target->getPhase() == Player::Finish && target->isWounded() && target->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
			room->addPlayerMark(target, objectName()+"engine");
			if (target->getMark(objectName()+"engine") > 0) {
				int num = target->getLostHp();
				target->drawCards(target->getLostHp(), objectName());
				target->setMark(objectName(), 0);
				if (!target->isKongcheng()) {
					forever {
						int n = target->getMark(objectName());
						if (n < num && !target->isKongcheng()) {
							QList<int> handcards = target->handCards();
							if (!room->askForYiji(target, handcards, objectName(), false, false, n == 0, num - n)) {
								if (n == 0)
									return false; // User select cancel at the first time of askForYiji, it can be treated as canceling the distribution of the cards

								break;
							}
						} else {
							break;
						}
					}
					// give the rest cards randomly
					if (target->getMark(objectName()) < num && !target->isKongcheng()) {
						int rest_num = num - target->getMark(objectName());
						forever {
							QList<int> handcard_list = target->handCards();
							qShuffle(handcard_list);
							int give = qrand() % rest_num + 1;
							rest_num -= give;
							QList<int> to_give = handcard_list.length() < give ? handcard_list : handcard_list.mid(0, give);
							ServerPlayer *receiver = room->getOtherPlayers(target).at(qrand() % (target->aliveCount() - 1));
							DummyCard *dummy = new DummyCard(to_give);
							room->obtainCard(receiver, dummy, false);
							delete dummy;
							if (rest_num == 0 || target->isKongcheng())
								break;
						}
					}
				}
				room->removePlayerMark(target, objectName()+"engine");
			}
        } else if (triggerEvent == ChoiceMade) {
            QString str = data.toString();
            if (str.startsWith("Yiji:" + objectName()))
                target->addMark(objectName(), str.split(":").last().split("+").length());
        }
        return false;
    }
};


class OlQianxi : public PhaseChangeSkill
{
public:
    OlQianxi() : PhaseChangeSkill("olqianxi")
    {
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        if (target->getPhase() == Player::Start && target->askForSkillInvoke(objectName())) {
            Room *room = target->getRoom();

            room->broadcastSkillInvoke(objectName());

			room->addPlayerMark(target, objectName()+"engine");
			if (target->getMark(objectName()+"engine") > 0) {
				target->drawCards(1, objectName());

				if (target->isNude())
					return false;

				const Card *c = room->askForCard(target, "..!", "@olqianxi");
				if (c == NULL) {
					c = target->getCards("he").at(qrand() % target->getCardCount());
					room->throwCard(c, target);
				}

				if (target->isDead())
					return false;

				QString color;
				if (c->isBlack())
					color = "black";
				else if (c->isRed())
					color = "red";
				else
					return false;
				QList<ServerPlayer *> to_choose;
				foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
					if (target->distanceTo(p) == 1)
						to_choose << p;
				}
				if (to_choose.isEmpty())
					return false;

				ServerPlayer *victim = room->askForPlayerChosen(target, to_choose, objectName());
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, target->objectName(), victim->objectName());
				QString pattern = QString(".|%1|.|hand$0").arg(color);
				target->tag[objectName()] = QVariant::fromValue(color);

				room->setPlayerFlag(victim, "OlQianxiTarget");
				room->addPlayerMark(victim, QString("@qianxi_%1").arg(color));
				room->setPlayerCardLimitation(victim, "use,response", pattern, false);

				LogMessage log;
				log.type = "#Qianxi";
				log.from = victim;
				log.arg = QString("no_suit_%1").arg(color);
				room->sendLog(log);
				room->removePlayerMark(target, objectName()+"engine");
			}
        }
        return false;
    }
};

class OlQianxiClear : public TriggerSkill
{
public:
    OlQianxiClear() : TriggerSkill("#olqianxi-clear")
    {
        events << EventPhaseChanging << Death;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return !target->tag["olqianxi"].toString().isNull();
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return false;
        }

        QString color = player->tag["olqianxi"].toString();
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->hasFlag("OlQianxiTarget")) {
                room->removePlayerCardLimitation(p, "use,response", QString(".|%1|.|hand$0").arg(color));
                room->setPlayerMark(p, QString("@qianxi_%1").arg(color), 0);
            }
        }
        return false;
    }
};

OlRendeCard::OlRendeCard()
{
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool OlRendeCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    QStringList rende_prop = Self->property("olrende").toString().split("+");
    if (rende_prop.contains(to_select->objectName()))
        return false;

    return targets.isEmpty() && to_select != Self;
}

void OlRendeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();

    QDateTime dtbefore = source->tag.value("olrende", QDateTime(QDate::currentDate(), QTime(0, 0, 0))).toDateTime();
    QDateTime dtafter = QDateTime::currentDateTime();

    if (dtbefore.secsTo(dtafter) > 3 * Config.AIDelay / 1000)
        room->broadcastSkillInvoke("rende");

    source->tag["olrende"] = QDateTime::currentDateTime();

    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "olrende", QString());
    room->obtainCard(target, this, reason, false);

    int old_value = source->getMark("olrende");
    int new_value = old_value + subcards.length();
    room->setPlayerMark(source, "olrende", new_value);

    if (old_value < 2 && new_value >= 2)
        room->recover(source, RecoverStruct(source));

    QSet<QString> rende_prop = source->property("olrende").toString().split("+").toSet();
    rende_prop.insert(target->objectName());
    room->setPlayerProperty(source, "olrende", QStringList(rende_prop.toList()).join("+"));
}

class OlRendeVS : public ViewAsSkill
{
public:
    OlRendeVS() : ViewAsSkill("olrende")
    {
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (ServerInfo.GameMode == "04_1v3" && selected.length() + Self->getMark("olrende") >= 2)
            return false;
        else
            return !to_select->isEquipped();
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        if (ServerInfo.GameMode == "04_1v3" && player->getMark("olrende") >= 2)
            return false;

        bool f = false;
        QStringList rende_prop = player->property("olrende").toString().split("+");
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p == player)
                continue;

            if (!rende_prop.contains(p->objectName())) {
                f = true;
                break;
            }
        }

        if (!f)
            return false;

        return !player->isKongcheng();
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        OlRendeCard *rende_card = new OlRendeCard;
        rende_card->addSubcards(cards);
        return rende_card;
    }
};

class OlRende : public TriggerSkill
{
public:
    OlRende() : TriggerSkill("olrende")
    {
        events << EventPhaseChanging;
        view_as_skill = new OlRendeVS;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getMark("olrende") > 0;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return false;
        room->setPlayerMark(player, "olrende", 0);

        room->setPlayerProperty(player, "olrende", QVariant());
        return false;
    }
};

OlQingjianCard::OlQingjianCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void OlQingjianCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    room->addPlayerMark(card_use.from, "olqingjianengine");
    if (card_use.from->getMark("olqingjianengine") > 0) {
		card_use.to.first()->obtainCard(this, false);
        room->removePlayerMark(card_use.from, "olqingjianengine");
    }
}

class OlQingjianVS : public ViewAsSkill
{
public:
    OlQingjianVS() : ViewAsSkill("olqingjian")
    {
        expand_pile = "olqingjian";
        response_pattern = "@@olqingjian!";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return Self->getPile("olqingjian").contains(to_select->getId());
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        OlQingjianCard *qj = new OlQingjianCard;
        qj->addSubcards(cards);
        return qj;
    }
};

class OlQingjian : public TriggerSkill
{
public:
    OlQingjian() : TriggerSkill("olqingjian")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
        view_as_skill = new OlQingjianVS;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            if (!TriggerSkill::triggerable(player))
                return false;

            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (!room->getTag("FirstRound").toBool() && player->getPhase() != Player::Draw && move.to == player && move.to_place == Player::PlaceHand) {
                QList<int> ids;
                foreach (int id, move.card_ids) {
                    if (room->getCardOwner(id) == player && room->getCardPlace(id) == Player::PlaceHand)
                        ids << id;
                }
                if (ids.isEmpty())
                    return false;

                player->tag["olqingjian"] = IntList2VariantList(ids);
                const Card *c = room->askForExchange(player, "olqingjian", ids.length(), 1, false, "@olqingjian", true, IntList2StringList(ids).join("#"));
                if (c == NULL)
                    return false;

                player->addToPile("olqingjian", c, false);
                ServerPlayer *current = room->getCurrent();
                if (!(current == NULL || current->isDead() || current->getPhase() == Player::NotActive))
                    player->setFlags("olqingjian");
            }
        } else {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("olqingjian")) {
                        while (!p->getPile("olqingjian").isEmpty()) { // cannot cancel!!!!!!!! must have AI to make program continue
                            if (room->askForUseCard(p, "@@olqingjian!", "@olqingjian-distribute", -1, Card::MethodNone)) {
                                if (p->getPile("olqingjian").isEmpty())
                                    break;
                                if (p->isDead())
                                    break;
                            }
                        }
                        p->setFlags("-olqingjian");
                    }
                }
            }
        }
        return false;
    }
};

class OlLeiji : public TriggerSkill
{
public:
    OlLeiji() : TriggerSkill("olleiji")
    {
        events << CardResponded;
		
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        const Card *card_star = data.value<CardResponseStruct>().m_card;
        if (card_star->isKindOf("Jink")) {
            ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "leiji-invoke", true, true);
            if (target) {
                room->broadcastSkillInvoke(objectName());

				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					JudgeStruct judge;
					judge.pattern = ".|black";
					judge.good = false;
					judge.negative = true;
					judge.reason = objectName();
					judge.who = target;

					room->judge(judge);

					if (judge.isBad()){
						if (judge.card->getSuit() == Card::Spade)
							room->damage(DamageStruct(objectName(), player, target, 2, DamageStruct::Thunder));
						else{
							if (player->isWounded())
								room->recover(player, RecoverStruct(player));
							room->damage(DamageStruct(objectName(), player, target, 1, DamageStruct::Thunder));
						}
					}
					room->removePlayerMark(player, objectName()+"engine");
				}
			}
        }
        return false;
    }
};

class OlYongsi : public PhaseChangeSkill
{
public:
    OlYongsi() : PhaseChangeSkill("olyongsi")
    {
		frequency = Compulsory;
    }

    virtual bool onPhaseChange(ServerPlayer *yuanshu) const
    {
        Room *room = yuanshu->getRoom();
        if (yuanshu->getPhase() == Player::Draw) {
            room->sendCompulsoryTriggerLog(yuanshu, objectName());
			room->broadcastSkillInvoke(objectName());

			QSet<QString> kingdom_set;
            foreach(ServerPlayer *p, room->getAlivePlayers())
                kingdom_set << p->getKingdom();

            int x = kingdom_set.size();

			room->addPlayerMark(yuanshu, objectName()+"engine");
			if (yuanshu->getMark(objectName()+"engine") > 0) {
				yuanshu->drawCards(x);
				room->removePlayerMark(yuanshu, objectName()+"engine");
			}

            return true;
		} else if (yuanshu->getPhase() == Player::Discard) {
			room->sendCompulsoryTriggerLog(yuanshu, objectName());
			room->broadcastSkillInvoke(objectName());

			room->addPlayerMark(yuanshu, objectName()+"engine");
			if (yuanshu->getMark(objectName()+"engine") > 0) {
				if (!room->askForDiscard(yuanshu, objectName(), 1, 1, true, true, "@olyongsi-discard"))
					room->loseHp(yuanshu);
				room->removePlayerMark(yuanshu, objectName()+"engine");
			}

		}
		return false;
    }
};

class OlJixi : public TriggerSkill
{
public:
    OlJixi() : TriggerSkill("oljixi")
    {
        events << EventPhaseChanging << HpLost << GameStart;
		frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target);
    }

	virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
			if (change.to != Player::NotActive || player->getMark("@jixi_turns") == 0)
                return false;
			room->removePlayerMark(player, "@jixi_turns");
            if (player->getMark("@jixi_turns") == 0){
				room->sendCompulsoryTriggerLog(player, objectName());
			    room->broadcastSkillInvoke(objectName());
				//room->doLightbox("$RejixiAnimate", 4000);
				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {

					room->setPlayerMark(player, "oljixi", 1);
					if (room->changeMaxHpForAwakenSkill(player, 1)) {
						room->recover(player, RecoverStruct(player));
                    QStringList choices;
					choices << "lordskill";
                    if (Sanguosha->getSkill("wangzun"))
                        choices << "wangzun";
						QStringList lordskills;
						ServerPlayer *lord = room->getLord();
						if (lord) {
							QList<const Skill *> skills = lord->getVisibleSkillList();
							foreach(const Skill *skill, skills) {
								if (skill->isLordSkill() && lord->hasLordSkill(skill->objectName(), true)) {
									lordskills.append(skill->objectName());
								}
							}
						}
						QString choice = room->askForChoice(player, "oljixi", choices.join("+"));
						if (choice == "wangzun")
							room->handleAcquireDetachSkills(player, "wangzun");
						else {
							room->drawCards(player, 2, "oljixi");
							if (!lordskills.isEmpty())
								room->handleAcquireDetachSkills(player, lordskills);
						}
					}
					room->removePlayerMark(player, objectName()+"engine");
				}
			}
		} else {
			if (triggerEvent == GameStart || (triggerEvent == HpLost && player->getPhase() != Player::NotActive)) {
				room->setPlayerMark(player, "@jixi_turns", 4);
			}
        }
        return false;
    }
};

OlAnxuCard::OlAnxuCard()
{
}

bool OlAnxuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (to_select == Self)
        return false;
    if (targets.isEmpty())
        return true;
    else if (targets.length() == 1)
        return to_select->getHandcardNum() != targets.first()->getHandcardNum();
    else
        return false;
}

bool OlAnxuCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

void OlAnxuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->addPlayerMark(source, "olanxuengine");
    if (source->getMark("olanxuengine") > 0) {
		QList<ServerPlayer *> selecteds = targets;
		ServerPlayer *from = selecteds.first()->getHandcardNum() < selecteds.last()->getHandcardNum() ? selecteds.takeFirst() : selecteds.takeLast();
		ServerPlayer *to = selecteds.takeFirst();
		if (!to->isKongcheng()){
			const Card *card = card = room->askForExchange(to, "olanxu", 1, 1, false, "@anxu-give:" + source->objectName() + ":" + from->objectName());
			CardMoveReason reason(CardMoveReason::S_REASON_GIVE, to->objectName(),
				from->objectName(), "olanxu", QString());
			reason.m_playerId = from->objectName();
			room->moveCardTo(card, to, from, Player::PlaceHand, reason);
			delete card;
		} 
		if (from->getHandcardNum() == to->getHandcardNum()){
			if (source->isWounded() && room->askForChoice(source, "olanxu", "recover+draw") == "recover")
					room->recover(source, RecoverStruct(source));
				else
					source->drawCards(1, "olanxu");
		}
        room->removePlayerMark(source, "olanxuengine");
    }
}

class OlAnxu : public ZeroCardViewAsSkill
{
public:
    OlAnxu() : ZeroCardViewAsSkill("olanxu")
    {
    }

    virtual const Card *viewAs() const
    {
        return new OlAnxuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("OlAnxuCard");
    }
};

class Fenyin : public TriggerSkill
{
public:
    Fenyin() : TriggerSkill("fenyin")
    {
        events << EventPhaseStart << CardUsed << CardResponded;
        global = true;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    virtual int getPriority(TriggerEvent triggerEvent) const
    {
        if (triggerEvent == EventPhaseStart)
            return 6;

        return TriggerSkill::getPriority(triggerEvent);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::RoundStart)
                player->setMark(objectName(), 0);

            return false;
        }

        const Card *c = NULL;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (player == use.from)
                c = use.card;
        } else {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse)
                c = resp.m_card;
        }

        if (c == NULL || c->isKindOf("SkillCard") || player->getPhase() == Player::NotActive)
            return false;

        if (player->getMark(objectName()) != 0) {
            Card::Color old_color = static_cast<Card::Color>(player->getMark(objectName()) - 1);
            if (old_color != c->getColor() && player->hasSkill(objectName()) && player->askForSkillInvoke(objectName(), QVariant::fromValue(c))) {
                room->broadcastSkillInvoke(objectName());
				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					player->drawCards(1);
					room->removePlayerMark(player, objectName()+"engine");
				}
            }
        }

        player->setMark(objectName(), static_cast<int>(c->getColor()) + 1);

        return false;
    }
};

class TunchuDraw : public DrawCardsSkill
{
public:
    TunchuDraw() : DrawCardsSkill("tunchu")
    {

    }

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
		Room *room = player->getRoom();
        if (player->askForSkillInvoke("tunchu")) {
            player->setFlags("tunchu");
            room->broadcastSkillInvoke("tunchu");
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				n = n + 2;
				room->removePlayerMark(player, objectName()+"engine");
			}
        }

        return n;
    }
};

class TunchuEffect : public TriggerSkill
{
public:
    TunchuEffect() : TriggerSkill("#tunchu-effect")
    {
        events << AfterDrawNCards;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->hasFlag("tunchu") && !player->isKongcheng()) {
            const Card *c = room->askForExchange(player, "tunchu", 1, 1, false, "@tunchu-put");
            if (c != NULL)
                player->addToPile("food", c);
			delete c;
        }

        return false;
    }
};

class Tunchu : public TriggerSkill
{
public:
    Tunchu() : TriggerSkill("#tunchu-disable")
    {
        events << EventLoseSkill << EventAcquireSkill << CardsMoveOneTime;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventLoseSkill && data.toString() == "tunchu") {
            room->removePlayerCardLimitation(player, "use", "Slash,Duel$0");
        } else if (triggerEvent == EventAcquireSkill && data.toString() == "tunchu") {
            if (!player->getPile("food").isEmpty())
                room->setPlayerCardLimitation(player, "use", "Slash,Duel", false);
        } else if (triggerEvent == CardsMoveOneTime && player->isAlive() && player->hasSkill("tunchu", true)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to == player && move.to_place == Player::PlaceSpecial && move.to_pile_name == "food") {
                if (player->getPile("food").length() == 1)
                    room->setPlayerCardLimitation(player, "use", "Slash,Duel", false);
            } else if (move.from == player && move.from_places.contains(Player::PlaceSpecial)
                && move.from_pile_names.contains("food")) {
                if (player->getPile("food").isEmpty())
                    room->removePlayerCardLimitation(player, "use", "Slash,Duel$0");
            }
        }
        return false;
    }
};

ShuliangCard::ShuliangCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void ShuliangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    CardMoveReason r(CardMoveReason::S_REASON_REMOVE_FROM_PILE, source->objectName(), objectName(), QString());
    room->moveCardTo(this, NULL, Player::DiscardPile, r, true);
}

class ShuliangVS : public OneCardViewAsSkill
{
public:
    ShuliangVS() : OneCardViewAsSkill("shuliang")
    {
        response_pattern = "@@shuliang";
        filter_pattern = ".|.|.|food";
        expand_pile = "food";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ShuliangCard *c = new ShuliangCard;
        c->addSubcard(originalCard);
        return c;
    }
};

class Shuliang : public TriggerSkill
{
public:
    Shuliang() : TriggerSkill("shuliang")
    {
        events << EventPhaseStart;
        view_as_skill = new ShuliangVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive() && target->getPhase() == Player::Finish && target->isKongcheng();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        foreach (ServerPlayer *const &p, room->getAllPlayers()) {
            if (!TriggerSkill::triggerable(p))
                continue;

       if (!p->getPile("food").isEmpty()) {
                if (room->askForUseCard(p, "@@shuliang", "@shuliang", -1, Card::MethodNone))
                    player->drawCards(2, objectName());
            }
        }

        return false;
    }
};


ZhanyiViewAsBasicCard::ZhanyiViewAsBasicCard()
{
    m_skillName = "_zhanyi";
    will_throw = false;
}

bool ZhanyiViewAsBasicCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    const Card *card = Self->tag.value("zhanyi").value<const Card *>();
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool ZhanyiViewAsBasicCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFixed();
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("zhanyi").value<const Card *>();
    return card && card->targetFixed();
}

bool ZhanyiViewAsBasicCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("zhanyi").value<const Card *>();
    return card && card->targetsFeasible(targets, Self);
}

const Card *ZhanyiViewAsBasicCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *zhuling = card_use.from;
    Room *room = zhuling->getRoom();

    QString to_zhanyi = user_string;
    if (user_string == "slash" && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        QStringList guhuo_list;
        guhuo_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            guhuo_list << "normal_slash" << "thunder_slash" << "fire_slash";
        to_zhanyi = room->askForChoice(zhuling, "zhanyi_slash", guhuo_list.join("+"));
    }

    const Card *card = Sanguosha->getCard(subcards.first());
    QString user_str;
    if (to_zhanyi == "slash") {
        if (card->isKindOf("Slash"))
            user_str = card->objectName();
        else
            user_str = "slash";
    } else if (to_zhanyi == "normal_slash")
        user_str = "slash";
    else
        user_str = to_zhanyi;
    Card *use_card = Sanguosha->cloneCard(user_str, card->getSuit(), card->getNumber());
    use_card->setSkillName("_zhanyi");
    use_card->addSubcard(subcards.first());
    use_card->deleteLater();
    return use_card;
}

const Card *ZhanyiViewAsBasicCard::validateInResponse(ServerPlayer *zhuling) const
{
    Room *room = zhuling->getRoom();

    QString to_zhanyi;
    if (user_string == "peach+analeptic") {
        QStringList guhuo_list;
        guhuo_list << "peach";
        if (!Config.BanPackages.contains("maneuvering"))
            guhuo_list << "analeptic";
        to_zhanyi = room->askForChoice(zhuling, "zhanyi_saveself", guhuo_list.join("+"));
    } else if (user_string == "slash") {
        QStringList guhuo_list;
        guhuo_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            guhuo_list << "normal_slash" << "thunder_slash" << "fire_slash";
        to_zhanyi = room->askForChoice(zhuling, "zhanyi_slash", guhuo_list.join("+"));
    } else
        to_zhanyi = user_string;

    const Card *card = Sanguosha->getCard(subcards.first());
    QString user_str;
    if (to_zhanyi == "slash") {
        if (card->isKindOf("Slash"))
            user_str = card->objectName();
        else
            user_str = "slash";
    } else if (to_zhanyi == "normal_slash")
        user_str = "slash";
    else
        user_str = to_zhanyi;
    Card *use_card = Sanguosha->cloneCard(user_str, card->getSuit(), card->getNumber());
    use_card->setSkillName("_zhanyi");
    use_card->addSubcard(subcards.first());
    use_card->deleteLater();
    return use_card;
}

ZhanyiCard::ZhanyiCard()
{
    target_fixed = true;
}

void ZhanyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->addPlayerMark(source, "engine");
    if (source->getMark("engine") > 0) {
		room->loseHp(source);	
		if (source->isAlive()) {
			const Card *c = Sanguosha->getCard(subcards.first());
			if (c->getTypeId() == Card::TypeBasic) {
				room->setPlayerMark(source, "ViewAsSkill_zhanyiEffect", 1);
			} else if (c->getTypeId() == Card::TypeEquip)
				source->setFlags("zhanyiEquip");
			else if (c->getTypeId() == Card::TypeTrick) {
				source->drawCards(2, "zhanyi");
				room->setPlayerFlag(source, "zhanyiTrick");
			}
		}
        room->removePlayerMark(source, "engine");
    }
}

class ZhanyiNoDistanceLimit : public TargetModSkill
{
public:
    ZhanyiNoDistanceLimit() : TargetModSkill("#zhanyi-trick")
    {
        pattern = "^SkillCard";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const
    {
        return from->hasFlag("zhanyiTrick") ? 1000 : 0;
    }
};

class ZhanyiDiscard2 : public TriggerSkill
{
public:
    ZhanyiDiscard2() : TriggerSkill("#zhanyi-equip")
    {
        events << TargetSpecified;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive() && target->hasFlag("zhanyiEquip");
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card == NULL || !use.card->isKindOf("Slash"))
            return false;


        foreach (ServerPlayer *p, use.to) {
            if (p->isNude())
                continue;

            if (p->getCardCount() <= 2) {
                DummyCard dummy;
                dummy.addSubcards(p->getCards("he"));
                room->throwCard(&dummy, p);
            } else
                room->askForDiscard(p, "zhanyi_equip", 2, 2, false, true, "@zhanyiequip_discard");
        }
        return false;
    }
};

class Zhanyi : public OneCardViewAsSkill
{
public:
    Zhanyi() : OneCardViewAsSkill("zhanyi")
    {
		response_or_use = true;
    }

    virtual bool isResponseOrUse() const
    {
        return Self->getMark("ViewAsSkill_zhanyiEffect") > 0;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (!player->hasUsed("ZhanyiCard"))
            return true;

        if (player->getMark("ViewAsSkill_zhanyiEffect") > 0)
            return true;

        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getMark("ViewAsSkill_zhanyiEffect") == 0) return false;
        if (pattern.startsWith(".") || pattern.startsWith("@")) return false;
        if (pattern == "peach" && player->getMark("Global_PreventPeach") > 0) return false;
        for (int i = 0; i < pattern.length(); i++) {
            QChar ch = pattern[i];
            if (ch.isUpper() || ch.isDigit()) return false; // This is an extremely dirty hack!! For we need to prevent patterns like 'BasicCard'
        }
        return !(pattern == "nullification");
    }

    virtual QDialog *getDialog() const
    {
        return GuhuoDialog::getInstance("zhanyi", true, false);
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        if (Self->getMark("ViewAsSkill_zhanyiEffect") > 0)
            return to_select->isKindOf("BasicCard");
        else
            return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (Self->getMark("ViewAsSkill_zhanyiEffect") == 0) {
            ZhanyiCard *zy = new ZhanyiCard;
            zy->addSubcard(originalCard);
            return zy;
        }

        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE
            || Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            ZhanyiViewAsBasicCard *card = new ZhanyiViewAsBasicCard;
            card->setUserString(Sanguosha->getCurrentCardUsePattern());
            card->addSubcard(originalCard);
            return card;
        }

        const Card *c = Self->tag.value("zhanyi").value<const Card *>();
        if (c) {
            ZhanyiViewAsBasicCard *card = new ZhanyiViewAsBasicCard;
            card->setUserString(c->objectName());
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }
};

class ZhanyiRemove : public TriggerSkill
{
public:
    ZhanyiRemove() : TriggerSkill("#zhanyi-basic")
    {
        events << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive() && target->getMark("ViewAsSkill_zhanyiEffect") > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive)
            room->setPlayerMark(player, "ViewAsSkill_zhanyiEffect", 0);

        return false;
    }
};


class Yuhua : public TriggerSkill
{
public:
    Yuhua() : TriggerSkill("yuhua")
    {
        events << EventPhaseStart << EventPhaseEnd;
		frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Discard && TriggerSkill::triggerable(player)) {
            room->sendCompulsoryTriggerLog(player, objectName());
			room->broadcastSkillInvoke(objectName());
			room->setPlayerFlag(player, objectName());
			room->setPlayerCardLimitation(player, "discard", "EquipCard,TrickCard|.|.|hand", true);
        } else if (triggerEvent == EventPhaseEnd && player->getPhase() == Player::Discard && player->hasFlag(objectName())) {
            room->removePlayerCardLimitation(player, "discard", "EquipCard,TrickCard|.|.|hand");
        }

        return false;
    }
};

class YuhuaKeep : public MaxCardsSkill
{
public:
    YuhuaKeep() : MaxCardsSkill("#yuhua")
    {
    }

    virtual int getExtra(const Player *target) const
    {
		int n = 0;
        if (target->hasSkill(objectName()) && target->getPhase() == Player::Discard){
            foreach (const Card *card, target->getHandcards())
                if (card->getTypeId() != Card::TypeBasic)
                    ++n;
		}
        
        return n;
    }
};

class Qirang : public TriggerSkill
{
public:
    Qirang() : TriggerSkill("qirang")
    {
        events << CardsMoveOneTime;
		frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to == player && move.to_place == Player::PlaceEquip) {
            for (int i = 0; i < move.card_ids.size(); i++) {
                if (!player->isAlive())
                    return false;
                if (room->askForSkillInvoke(player, objectName())) {
				    room->broadcastSkillInvoke(objectName());
					QList<int> tricks;
                    foreach (int card_id, room->getDrawPile()) {
						if (Sanguosha->getCard(card_id)->getTypeId() == Card::TypeTrick)
							tricks << card_id;
					}
					if (tricks.isEmpty()){
                        LogMessage log;
                        log.type = "$SearchFailed";
                        log.from = player;
						log.arg = "trick";
                        room->sendLog(log);
						break;
					}
					int index = qrand() % tricks.length();
					int id = tricks.at(index);
					room->addPlayerMark(player, objectName()+"engine");
					if (player->getMark(objectName()+"engine") > 0) {
						player->obtainCard(Sanguosha->getCard(id));
						room->removePlayerMark(player, objectName()+"engine");
					}
                } else {
                    break;
                }
                
            }
        }
        return false;
    }
};


class Moukui: public TriggerSkill {
public:
    Moukui(): TriggerSkill("moukui") {
        events << TargetSpecified << SlashMissed << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetSpecified && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return false;
            foreach (ServerPlayer *p, use.to) {
                if (player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                    QString choice;
					room->addPlayerMark(player, objectName()+"engine");
					if (player->getMark(objectName()+"engine") > 0) {
						if (!player->canDiscard(p, "he"))
							choice = "draw";
						else
							choice = room->askForChoice(player, objectName(), "draw+discard", QVariant::fromValue(p));
						if (choice == "draw") {
							room->broadcastSkillInvoke(objectName(), 1);
							player->drawCards(1, objectName());
						} else {
							room->broadcastSkillInvoke(objectName(), 2);
							room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());
							room->setTag("MoukuiDiscard", data);
							int disc = room->askForCardChosen(player, p, "he", objectName(), false, Card::MethodDiscard);
							room->removeTag("MoukuiDiscard");
							room->throwCard(disc, p, player);
						}
						room->addPlayerMark(p, objectName() + use.card->toString());
						room->removePlayerMark(player, objectName()+"engine");
					}
                }
            }
        } else if (triggerEvent == SlashMissed) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.to->isDead() || effect.to->getMark(objectName() + effect.slash->toString()) <= 0)
                return false;
            if (!effect.from->isAlive() || !effect.to->isAlive() || !effect.to->canDiscard(effect.from, "he"))
                return false;
            int disc = room->askForCardChosen(effect.to, effect.from, "he", objectName(), false, Card::MethodDiscard);
            room->broadcastSkillInvoke(objectName(), 3);
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, effect.to->objectName(), effect.from->objectName());
            room->throwCard(disc, effect.from, effect.to);
            room->removePlayerMark(effect.to, objectName() + effect.slash->toString());
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return false;
            foreach (ServerPlayer *p, room->getAllPlayers())
                room->setPlayerMark(p, objectName() + use.card->toString(), 0);
        }

        return false;
    }
};

class Tianming: public TriggerSkill {
public:
    Tianming(): TriggerSkill("tianming") {
        events << TargetConfirming;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && room->askForSkillInvoke(player, objectName())) {
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				room->broadcastSkillInvoke(objectName(), 1);
				room->askForDiscard(player, objectName(), 2, 2, false, true);
				player->drawCards(2, objectName());

				int max = -1000;
				foreach (ServerPlayer *p, room->getAllPlayers())
					if (p->getHp() > max)
						max = p->getHp();
				if (player->getHp() == max)
					return false;

				QList<ServerPlayer *> maxs;
				foreach (ServerPlayer *p, room->getAllPlayers()) {
					if (p->getHp() == max)
						maxs << p;
					if (maxs.size() > 1)
						return false;
				}
				ServerPlayer *mosthp = maxs.first();
				if (room->askForSkillInvoke(mosthp, objectName())) {
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), mosthp->objectName());
					int index = 2;
					if (mosthp->isFemale())
						index = 3;
					room->broadcastSkillInvoke(objectName(), index);
					room->askForDiscard(mosthp, objectName(), 2, 2, false, true);
					mosthp->drawCards(2, objectName());
				}
				room->removePlayerMark(player, objectName()+"engine");
			}
        }

        return false;
    }
};

MizhaoCard::MizhaoCard() {
    mute = true;
}

bool MizhaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self;
}

void MizhaoCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->addPlayerMark(effect.from, "mizhaoengine");
    if (effect.from->getMark("mizhaoengine") > 0) {
		DummyCard *handcards = effect.from->wholeHandCards();
		effect.to->obtainCard(handcards, false, CardMoveReason::S_REASON_GIVE);
		delete handcards;
		if (effect.to->isKongcheng()) return;

		room->broadcastSkillInvoke("mizhao", effect.to->getGeneralName().contains("liubei") ? 2 : 1);

		QList<ServerPlayer *> targets;
		foreach (ServerPlayer *p, room->getOtherPlayers(effect.to))
			if (!p->isKongcheng())
				targets << p;

		if (!targets.isEmpty()) {
			ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "mizhao", "@mizhao-pindian:" + effect.to->objectName());
			target->setFlags("MizhaoPindianTarget");
			effect.to->pindian(target, "mizhao", NULL);
			target->setFlags("-MizhaoPindianTarget");
		}
        room->removePlayerMark(effect.from, "mizhaoengine");
    }
}

class MizhaoViewAsSkill: public ZeroCardViewAsSkill {
public:
    MizhaoViewAsSkill(): ZeroCardViewAsSkill("mizhao") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && !player->hasUsed("MizhaoCard");
    }

    virtual const Card *viewAs() const{
        return new MizhaoCard;
    }
};

class Mizhao: public TriggerSkill {
public:
    Mizhao(): TriggerSkill("mizhao") {
        events << Pindian;
        view_as_skill = new MizhaoViewAsSkill;
    }

    virtual int getPriority(TriggerEvent) const{
        return -1;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (pindian->reason != objectName() || pindian->from_number == pindian->to_number)
            return false;

        ServerPlayer *winner = pindian->isSuccess() ? pindian->from : pindian->to;
        ServerPlayer *loser = pindian->isSuccess() ? pindian->to : pindian->from;
        if (winner->canSlash(loser, NULL, false)) {
            Slash *slash = new Slash(Card::NoSuit, 0);
            slash->setSkillName("_mizhao");
            room->useCard(CardUseStruct(slash, winner, loser));
        }

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return -2;
    }
};

class MizhaoSlashNoDistanceLimit: public TargetModSkill {
public:
    MizhaoSlashNoDistanceLimit(): TargetModSkill("#mizhao-slash-ndl") {
    }

    virtual int getDistanceLimit(const Player *, const Card *card) const{
        if (card->isKindOf("Slash") && card->getSkillName() == "mizhao")
            return 1000;
        else
            return 0;
    }
};

class Jieyuan: public TriggerSkill {
public:
    Jieyuan(): TriggerSkill("jieyuan") {
        events << DamageCaused << DamageInflicted;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == DamageCaused) {
            if (damage.to && damage.to->isAlive()
                && damage.to->getHp() >= player->getHp() && damage.to != player && player->canDiscard(player, "h")
                && room->askForCard(player, ".black", "@jieyuan-increase:" + damage.to->objectName(), data, objectName())) {
                room->broadcastSkillInvoke(objectName(), 1);
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());

                LogMessage log;
                log.type = "#JieyuanIncrease";
                log.from = player;
                log.arg = QString::number(damage.damage);
                log.arg2 = QString::number(++damage.damage);
                room->sendLog(log);

				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					data = QVariant::fromValue(damage);
					room->removePlayerMark(player, objectName()+"engine");
				}
            }
        } else if (triggerEvent == DamageInflicted) {
            if (damage.from && damage.from->isAlive()
                && damage.from->getHp() >= player->getHp() && damage.from != player && player->canDiscard(player, "h")
                && room->askForCard(player, ".red", "@jieyuan-decrease:" + damage.from->objectName(), data, objectName())) {
                room->broadcastSkillInvoke(objectName(), 2);

                LogMessage log;
                log.type = "#JieyuanDecrease";
                log.from = player;
                log.arg = QString::number(damage.damage);
                log.arg2 = QString::number(--damage.damage);
                room->sendLog(log);

				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					data = QVariant::fromValue(damage);
					if (damage.damage < 1) {
						room->removePlayerMark(player, objectName()+"engine");
						return true;
					}
					room->removePlayerMark(player, objectName()+"engine");
				}
            }
        }

        return false;
    }
};

class Fenxin: public TriggerSkill {
public:
    Fenxin(): TriggerSkill("fenxin") {
        events << BeforeGameOverJudge;
        frequency = Limited;
        limit_mark = "@burnheart";
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (!isNormalGameMode(room->getMode()))
            return false;
        DeathStruct death = data.value<DeathStruct>();
        if (death.damage == NULL)
            return false;
        ServerPlayer *killer = death.damage->from;
        if (killer == NULL || killer->isLord() || player->isLord() || player->getHp() > 0)
            return false;
        if (!TriggerSkill::triggerable(killer) || killer->getMark("@burnheart") == 0)
            return false;
        player->setFlags("FenxinTarget");
        bool invoke = room->askForSkillInvoke(killer, objectName(), QVariant::fromValue(player));
        player->setFlags("-FenxinTarget");
        if (invoke) {
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, killer->objectName(), player->objectName());
            room->broadcastSkillInvoke(objectName());
            //room->doLightbox("$FenxinAnimate");
			room->addPlayerMark(killer, objectName()+"engine");
			if (killer->getMark(objectName()+"engine") > 0) {
				room->removePlayerMark(killer, "@burnheart");
				QString role1 = killer->getRole();
				killer->setRole(player->getRole());
				if (player == room->getTag("Zhaoyun_Buff").value<ServerPlayer *>())
					room->setTag("Zhaoyun_Buff", QVariant::fromValue(killer));
				else if (player == room->getTag("Zhouyu_Buff").value<ServerPlayer *>())
					room->setTag("Zhouyu_Buff", QVariant::fromValue(killer));
				else if (player == room->getTag("Zhangyi_Buff").value<ServerPlayer *>())
					room->setTag("Zhangyi_Buff", QVariant::fromValue(killer));
				room->notifyProperty(killer, killer, "role", player->getRole());
				room->setPlayerProperty(player, "role", role1);
				if (killer == room->getTag("Zhaoyun_Buff").value<ServerPlayer *>())
					room->setTag("Zhaoyun_Buff", QVariant::fromValue(player));
				else if (killer == room->getTag("Zhouyu_Buff").value<ServerPlayer *>())
					room->setTag("Zhouyu_Buff", QVariant::fromValue(player));
				else if (killer == room->getTag("Zhangyi_Buff").value<ServerPlayer *>())
					room->setTag("Zhangyi_Buff", QVariant::fromValue(player));
				room->removePlayerMark(killer, objectName()+"engine");
			}
        }
        return false;
    }
};

HongyuanCard::HongyuanCard() {
    mute = true;
}

bool HongyuanCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() <= 2 && !targets.contains(Self);
}

bool HongyuanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return to_select != Self && targets.length() < 2;
}

void HongyuanCard::onEffect(const CardEffectStruct &effect) const{
   effect.to->setFlags("HongyuanTarget");
}

class HongyuanViewAsSkill: public ZeroCardViewAsSkill {
public:
    HongyuanViewAsSkill(): ZeroCardViewAsSkill("hongyuan") {
        response_pattern = "@@hongyuan";
    }

    virtual const Card *viewAs() const{
        return new HongyuanCard;
    }
};

class Hongyuan: public DrawCardsSkill {
public:
    Hongyuan(): DrawCardsSkill("hongyuan") {
        frequency = NotFrequent;
        view_as_skill = new HongyuanViewAsSkill;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
		if (n == 0)
           return false;
        Room *room = player->getRoom();
        bool invoke = false;
        if (room->getMode().startsWith("06_"))
            invoke = room->askForSkillInvoke(player, objectName());
        else
            invoke = room->askForUseCard(player, "@@hongyuan", "@hongyuan");
        if (invoke) {
            room->broadcastSkillInvoke(objectName());
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				player->setFlags("hongyuan");
				room->removePlayerMark(player, objectName()+"engine");
			}
            n = n - 1;
        }
        return n;
    }
};

class HongyuanDraw: public TriggerSkill {
public:
    HongyuanDraw(): TriggerSkill("#hongyuan") {
        events << AfterDrawNCards;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (!player->hasFlag("hongyuan"))
            return false;
        player->setFlags("-hongyuan");

        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (room->getMode().startsWith("06_") || room->getMode().startsWith("04_")) {
                if (AI::GetRelation3v3(player, p) == AI::Friend)
                    targets << p;
            } else if (p->hasFlag("HongyuanTarget")) {
                p->setFlags("-HongyuanTarget");
                targets << p;
            }
        }

        if (targets.isEmpty()) return false;
        room->drawCards(targets, 1, "hongyuan");
        return false;
    }
};

class Huanshi: public TriggerSkill {
public:
    Huanshi(): TriggerSkill("huanshi") {
        events << AskForRetrial;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && !target->isNude();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        JudgeStruct *judge = data.value<JudgeStruct *>();
        const Card *card = NULL;
        if (room->getMode().startsWith("06_") || room->getMode().startsWith("04_")) {
            if (AI::GetRelation3v3(player, judge->who) != AI::Friend) return false;
            QStringList prompt_list;
            prompt_list << "@huanshi-card" << judge->who->objectName()
                        << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
            QString prompt = prompt_list.join(":");

            card = room->askForCard(player, "..", prompt, data, Card::MethodResponse, judge->who, true);
        } else if (!player->isNude()) {
            QList<int> ids, disabled_ids;
            foreach (const Card *card, player->getCards("he")) {
                if (player->isCardLimited(card, Card::MethodResponse))
                    disabled_ids << card->getEffectiveId();
                else
                    ids << card->getEffectiveId();
            }
            if (!ids.isEmpty() && room->askForSkillInvoke(player, objectName(), data)) {
                if (judge->who != player && !player->isKongcheng()) {
                    LogMessage log;
                    log.type = "$ViewAllCards";
                    log.from = judge->who;
                    log.to << player;
                    log.card_str = IntList2StringList(player->handCards()).join("+");
                    room->sendLog(log, judge->who);
                }
                judge->who->tag["HuanshiJudge"] = data;
                room->fillAG(ids + disabled_ids, judge->who, disabled_ids);
                int card_id = room->askForAG(judge->who, ids, false, objectName());
                room->clearAG(judge->who);
                judge->who->tag.remove("HuanshiJudge");
                card = Sanguosha->getCard(card_id);
            }
        }
        if (card != NULL) {
            room->broadcastSkillInvoke(objectName());
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				room->retrial(card, player, judge, objectName());
				room->removePlayerMark(player, objectName()+"engine");
			}
        }

        return false;
    }
};

class Mingzhe: public TriggerSkill {
public:
    Mingzhe(): TriggerSkill("mingzhe") {
        events << BeforeCardsMove << CardsMoveOneTime << CardUsed << CardResponded;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() != Player::NotActive) return false;
        if (triggerEvent == BeforeCardsMove || triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != player) return false;

            if (triggerEvent == BeforeCardsMove) {
                CardMoveReason reason = move.reason;
                if ((reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                    const Card *card;
                    int i = 0;
                    foreach (int card_id, move.card_ids) {
                        card = Sanguosha->getCard(card_id);
                        if (room->getCardOwner(card_id) == player && card->isRed()
                            && (move.from_places[i] == Player::PlaceHand
                                || move.from_places[i] == Player::PlaceEquip)) {
                            player->addMark(objectName());
                        }
                        ++i;
                    }
                }
            } else {
                int n = player->getMark(objectName());
                try {
                    for (int i = 0; i < n; ++i) {
                        player->removeMark(objectName());
                        if (player->isAlive() && player->askForSkillInvoke(objectName(), data)) {
                            room->broadcastSkillInvoke(objectName());
							room->addPlayerMark(player, objectName()+"engine");
							if (player->getMark(objectName()+"engine") > 0) {
								player->drawCards(1, objectName());
								room->removePlayerMark(player, objectName()+"engine");
							}
                        } else {
                            break;
                        }
                    }
                    player->setMark(objectName(), 0);
                }
                catch (TriggerEvent triggerEvent) {
                    if (triggerEvent == TurnBroken || triggerEvent == StageChange)
                        player->setMark(objectName(), 0);
                    throw triggerEvent;
                }
            }
        } else {
            const Card *card = NULL;
            if (triggerEvent == CardUsed) {
                CardUseStruct use = data.value<CardUseStruct>();
                card = use.card;
            } else if (triggerEvent == CardResponded) {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                card = resp.m_card;
            }
            if (card && card->isRed() && player->askForSkillInvoke(objectName(), data)) {
                room->broadcastSkillInvoke(objectName());
				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					player->drawCards(1, objectName());
					room->removePlayerMark(player, objectName()+"engine");
				}
            }
        }
        return false;
    }
};

class Xiaoguo: public TriggerSkill {
public:
    Xiaoguo(): TriggerSkill("xiaoguo") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() != Player::Finish)
            return false;
        ServerPlayer *yuejin = room->findPlayerBySkillName(objectName());
        if (!yuejin || yuejin == player)
            return false;
        if (yuejin->canDiscard(yuejin, "h") && room->askForCard(yuejin, ".Basic", "@xiaoguo", QVariant(), objectName())) {
            room->broadcastSkillInvoke(objectName(), 1);
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, yuejin->objectName(), player->objectName());
			room->addPlayerMark(yuejin, objectName()+"engine");
			if (yuejin->getMark(objectName()+"engine") > 0) {
				if (!room->askForCard(player, ".Equip", "@xiaoguo-discard", QVariant())) {
					room->broadcastSkillInvoke(objectName(), 2);
					room->damage(DamageStruct("xiaoguo", yuejin, player));
				} else {
					room->broadcastSkillInvoke(objectName(), 3);
					if (yuejin->isAlive() && !Config.value("heg_skill", true).toBool())
						yuejin->drawCards(1, objectName());
				}
				room->removePlayerMark(yuejin, objectName()+"engine");
			}
        }
        return false;
    }
};

class Shushen: public TriggerSkill {
public:
    Shushen(): TriggerSkill("shushen") {
        events << PreHpRecover;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        RecoverStruct recover_struct = data.value<RecoverStruct>();
        int recover = recover_struct.recover;
        for (int i = 0; i < recover; ++i) {
            ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "shushen-invoke", true, true);
            if (target) {
                room->broadcastSkillInvoke(objectName(), target->getGeneralName().contains("liubei") ? 2 : 1);
				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					if (!Config.value("heg_skill", true).toBool()) {
						if (target->isWounded() && room->askForChoice(player, objectName(), "recover+draw", QVariant::fromValue(target)) == "recover")
							room->recover(target, RecoverStruct(player));
						else
							target->drawCards(2, objectName());
					} else
						target->drawCards(1, objectName());
					room->removePlayerMark(player, objectName()+"engine");
				}
            } else {
                break;
            }
        }
        return false;
    }
};

class Shenzhi: public PhaseChangeSkill {
public:
    Shenzhi(): PhaseChangeSkill("shenzhi") {
    }

    virtual bool onPhaseChange(ServerPlayer *ganfuren) const{
        Room *room = ganfuren->getRoom();
        if (ganfuren->getPhase() != Player::Start || ganfuren->isKongcheng())
            return false;
        // As the cost, if one of her handcards cannot be throwed, the skill is unable to invoke
        foreach (const Card *card, ganfuren->getHandcards()) {
            if (ganfuren->isJilei(card))
                return false;
        }
        //==================================
        if (room->askForSkillInvoke(ganfuren, objectName())) {
            int handcard_num = ganfuren->getHandcardNum();
            room->broadcastSkillInvoke(objectName());
			room->addPlayerMark(ganfuren, objectName()+"engine");
			if (ganfuren->getMark(objectName()+"engine") > 0) {
				ganfuren->throwAllHandCards();
				if (handcard_num >= ganfuren->getHp())
					room->recover(ganfuren, RecoverStruct(ganfuren));
				room->removePlayerMark(ganfuren, objectName()+"engine");
			}
        }
        return false;
    }
};

FenxunCard::FenxunCard() {
}

bool FenxunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self;
}

void FenxunCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->addPlayerMark(effect.from, "fenxunengine");
    if (effect.from->getMark("fenxunengine") > 0) {
		effect.from->tag["FenxunTarget"] = QVariant::fromValue(effect.to);
		room->setFixedDistance(effect.from, effect.to, 1);
        room->removePlayerMark(effect.from, "fenxunengine");
    }
}

class FenxunViewAsSkill: public OneCardViewAsSkill {
public:
    FenxunViewAsSkill(): OneCardViewAsSkill("fenxun") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("FenxunCard");
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        FenxunCard *first = new FenxunCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class Fenxun: public TriggerSkill {
public:
    Fenxun(): TriggerSkill("fenxun") {
        events << EventPhaseChanging << Death;
        view_as_skill = new FenxunViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->tag["FenxunTarget"].value<ServerPlayer *>() != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *dingfeng, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != dingfeng)
                return false;
        }
        ServerPlayer *target = dingfeng->tag["FenxunTarget"].value<ServerPlayer *>();

        if (target) {
            room->removeFixedDistance(dingfeng, target, 1);
            dingfeng->tag.remove("FenxunTarget");
        }
        return false;
    }
};

class Kuangfu: public TriggerSkill {
public:
    Kuangfu(): TriggerSkill("kuangfu") {
        events << Damage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *panfeng, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        if (damage.card && damage.card->isKindOf("Slash") && target->hasEquip()
            && !target->hasFlag("Global_DebutFlag") && !damage.chain && !damage.transfer) {
            QStringList equiplist;
            for (int i = 0; i < S_EQUIP_AREA_LENGTH; ++i) {
                if (!target->getEquip(i)) continue;
                if (panfeng->canDiscard(target, target->getEquip(i)->getEffectiveId()) || panfeng->getEquip(i) == NULL)
                    equiplist << QString::number(i);
            }
            if (equiplist.isEmpty() || !panfeng->askForSkillInvoke(objectName(), data))
                return false;
			
			room->addPlayerMark(panfeng, objectName()+"engine");
			if (panfeng->getMark(objectName()+"engine") > 0) {
				int equip_index = room->askForChoice(panfeng, "kuangfu_equip", equiplist.join("+"), QVariant::fromValue(target)).toInt();
				const Card *card = target->getEquip(equip_index);
				int card_id = card->getEffectiveId();

				QStringList choicelist;
				if (panfeng->canDiscard(target, card_id))
					choicelist << "throw";
				if (equip_index > -1 && panfeng->getEquip(equip_index) == NULL)
					choicelist << "move";

				QString choice = room->askForChoice(panfeng, "kuangfu", choicelist.join("+"));

				if (choice == "move") {
					room->broadcastSkillInvoke(objectName(), 1);
					room->moveCardTo(card, panfeng, Player::PlaceEquip);
				} else {
					room->broadcastSkillInvoke(objectName(), 2);
					room->throwCard(card, target, panfeng);
				}
				room->removePlayerMark(panfeng, objectName()+"engine");
			}
        }

        return false;
    }
};

class Zhendu: public TriggerSkill {
public:
    Zhendu(): TriggerSkill("zhendu") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() != Player::Play)
            return false;
         foreach (ServerPlayer *hetaihou, room->getOtherPlayers(player)) {
            if (!TriggerSkill::triggerable(hetaihou))
                continue;
            if (!hetaihou->isAlive() || !hetaihou->canDiscard(hetaihou, "h") || hetaihou->getPhase() == Player::Play)
                continue;
			if (room->askForCard(hetaihou, ".", "@zhendu-discard", QVariant(), objectName())) {
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, hetaihou->objectName(), player->objectName());
				room->addPlayerMark(hetaihou, objectName()+"engine");
				if (hetaihou->getMark(objectName()+"engine") > 0) {
					Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
					analeptic->setSkillName("_zhendu");
					room->useCard(CardUseStruct(analeptic, player, QList<ServerPlayer *>()), true);
					if (player->isAlive())
						room->damage(DamageStruct(objectName(), hetaihou, player));
					room->removePlayerMark(hetaihou, objectName()+"engine");
				}
            }
        }
        return false;
    }
};

class Qiluan : public TriggerSkill
{
public:
    Qiluan() : TriggerSkill("qiluan") {
        events << Death << EventPhaseChanging;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player) {
                return false;
            }
            ServerPlayer *killer = death.damage ? death.damage->from : NULL;
            ServerPlayer *current = room->getCurrent();

            if (killer && current && (current->isAlive() || death.who == current)
                && current->getPhase() != Player::NotActive) {
                killer->addMark(objectName());
            }
        }
        else {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                QList<ServerPlayer *> hetaihous;
                QList<int> mark_count;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getMark(objectName()) > 0 && TriggerSkill::triggerable(p)) {
                        hetaihous << p;
                        mark_count << p->getMark(objectName());
                    }
                    p->setMark(objectName(), 0);
                }

                for (int i = 0; i < hetaihous.length(); ++i) {
                    ServerPlayer *p = hetaihous.at(i);
					if (Config.value("heg_skill", true).toBool()) {
						if (p->isAlive() && room->askForSkillInvoke(p, objectName())) {
							room->addPlayerMark(p, objectName()+"engine");
							if (p->getMark(objectName()+"engine") > 0) {
								p->drawCards(3, objectName());
								room->removePlayerMark(p, objectName()+"engine");
							}
						}
					} else {
						for (int j = 0; j < mark_count.at(i); ++j) {
							if (p->isDead() || !room->askForSkillInvoke(p, objectName())) {
								break;
							}
							room->addPlayerMark(p, objectName()+"engine");
							if (p->getMark(objectName()+"engine") > 0) {
								p->drawCards(3, objectName());
								room->removePlayerMark(p, objectName()+"engine");
							}
						}	
					}
                }
            }
        }
        return false;
    }
};

SPCardPackage::SPCardPackage()
    : Package("sp_cards")
{
    (new SPMoonSpear)->setParent(this);
    skills << new SPMoonSpearSkill;

    type = CardPack;
}

ADD_PACKAGE(SPCard)

SPPackage::SPPackage()
    : Package("sp")
{
	//SP 010 SP 015 SP 022 太监了
    General *yangxiu = new General(this, "yangxiu", "wei", 3); // SP 001
    yangxiu->addSkill(new Jilei);
    yangxiu->addSkill(new JileiClear);
    yangxiu->addSkill(new Danlao);
    related_skills.insertMulti("jilei", "#jilei-clear");

    General *gongsunzan = new General(this, "gongsunzan", "qun", 4, true, Config.value("EnableHidden", true).toBool()); // SP 003
    gongsunzan->addSkill(new Yicong);
    gongsunzan->addSkill(new YicongEffect);
    related_skills.insertMulti("yicong", "#yicong-effect");

    General *yuanshu = new General(this, "yuanshu", "qun", 4, true, Config.value("EnableHidden", true).toBool()); // SP 004
    yuanshu->addSkill(new Yongsi);
    yuanshu->addSkill(new Weidi);

    General *sp_sunshangxiang = new General(this, "sp_sunshangxiang", "shu", 3, false, true); // SP 005
    sp_sunshangxiang->addSkill("jieyin");
    sp_sunshangxiang->addSkill("xiaoji");

    General *sp_pangde = new General(this, "sp_pangde", "wei", 4, true, true); // SP 006
    sp_pangde->addSkill("mashu");
    sp_pangde->addSkill("mengjin");

    General *sp_guanyu = new General(this, "sp_guanyu", "wei", 4, true, Config.value("EnableHidden", true).toBool()); // SP 007
    sp_guanyu->addSkill("wusheng");
    sp_guanyu->addSkill(new Danji);

    General *shenlvbu1 = new General(this, "shenlvbu1", "god", 8, true, true); // SP 008 (2-1)
    shenlvbu1->addSkill("mashu");
    shenlvbu1->addSkill("wushuang");

    General *shenlvbu2 = new General(this, "shenlvbu2", "god", 4, true, true); // SP 008 (2-2)
    shenlvbu2->addSkill("mashu");
    shenlvbu2->addSkill("wushuang");
    shenlvbu2->addSkill(new Xiuluo);
    shenlvbu2->addSkill(new ShenweiKeep);
    shenlvbu2->addSkill(new Shenwei);
    shenlvbu2->addSkill(new Shenji);
    related_skills.insertMulti("shenwei", "#shenwei-draw");

    General *sp_caiwenji = new General(this, "sp_caiwenji", "wei", 3, false, true); // SP 009
    sp_caiwenji->addSkill("beige");
    sp_caiwenji->addSkill("duanchang");

    General *sp_machao = new General(this, "sp_machao", "qun", 4, true, true); // SP 011
    sp_machao->addSkill("mashu");
    sp_machao->addSkill("nostieji");

    General *sp_jiaxu = new General(this, "sp_jiaxu", "wei", 3, true, true); // SP 012
    sp_jiaxu->addSkill("wansha");
    sp_jiaxu->addSkill("luanwu");
    sp_jiaxu->addSkill("weimu");

    General *caohong = new General(this, "caohong", "wei"); // SP 013
    caohong->addSkill(new Yuanhu);

    General *guanyinping = new General(this, "guanyinping", "shu", 3, false); // SP 014
    guanyinping->addSkill(new Xueji);
    guanyinping->addSkill(new Huxiao);
    guanyinping->addSkill(new HuxiaoCount);
    guanyinping->addSkill(new HuxiaoClear);
    guanyinping->addSkill(new Wuji);
    related_skills.insertMulti("huxiao", "#huxiao-count");
    related_skills.insertMulti("huxiao", "#huxiao-clear");

    General *liuxie = new General(this, "liuxie", "qun", 3); // SP 016
    liuxie->addSkill(new Tianming);
    liuxie->addSkill(new Mizhao);

    General *lingju = new General(this, "lingju", "qun", 3, false);// SP 017
    lingju->addSkill(new Jieyuan);
    lingju->addSkill(new Fenxin);

    General *fuwan = new General(this, "fuwan", "qun", 4);// SP 018
    fuwan->addSkill(new Moukui);

    General *xiahouba = new General(this, "xiahouba", "shu"); // SP 019
    xiahouba->addSkill(new Baobian);

    General *chenlin = new General(this, "chenlin", "wei", 3); // SP 020
    chenlin->addSkill(new Bifa);
    chenlin->addSkill(new Songci);

    General *erqiao = new General(this, "erqiao", "wu", 3, false); // SP 021
    erqiao->addSkill(new Xingwu);
    erqiao->addSkill(new Luoyan);

    General *xiahoushi = new General(this, "xiahoushi", "shu", 3, false); // SP 023
    xiahoushi->addSkill(new Yanyu);
    xiahoushi->addSkill(new Xiaode);
    xiahoushi->addSkill(new XiaodeEx);
    related_skills.insertMulti("xiaode", "#xiaode");

    General *yuejin = new General(this, "yuejin", "wei"); // SP 024
    yuejin->addSkill(new Xiaoguo);

    General *zhangbao = new General(this, "zhangbao", "qun", 3); // SP 025
    zhangbao->addSkill(new Zhoufu);
    zhangbao->addSkill(new Yingbing);

    General *caoang = new General(this, "caoang", "wei"); // SP 026
    caoang->addSkill(new Kangkai);

    General *zhugejin = new General(this, "zhugejin", "wu", 3); // SP 027
    zhugejin->addSkill(new Hongyuan);
    zhugejin->addSkill(new HongyuanDraw);
    zhugejin->addSkill(new Huanshi);
    zhugejin->addSkill(new Mingzhe);
    related_skills.insertMulti("hongyuan", "#hongyuan");

    General *xingcai = new General(this, "xingcai", "shu", 3, false); // SP 028
    xingcai->addSkill(new Shenxian);
    xingcai->addSkill(new Qiangwu);
    xingcai->addSkill(new QiangwuTargetMod);
    related_skills.insertMulti("qiangwu", "#qiangwu-target");

    General *panfeng = new General(this, "panfeng", "qun"); // SP 029
    panfeng->addSkill(new Kuangfu);

    General *zumao = new General(this, "zumao", "wu"); // SP 030
    zumao->addSkill(new Yinbing);
    zumao->addSkill(new Juedi);

    General *dingfeng = new General(this, "dingfeng", "wu"); // SP 031
    dingfeng->addSkill(new Skill("duanbing", Skill::Compulsory));
    dingfeng->addSkill(new Fenxun);

    General *zhugedan = new General(this, "zhugedan", "wei", 4); // SP 032
    zhugedan->addSkill(new Gongao);
    zhugedan->addSkill(new Juyi);
    zhugedan->addRelateSkill("weizhong");
    zhugedan->addRelateSkill("benghuai");

    General *hetaihou = new General(this, "hetaihou", "qun", 3, false); // SP 033
    hetaihou->addSkill(new Zhendu);
    hetaihou->addSkill(new Qiluan);

    General *sunluyu = new General(this, "sunluyu", "wu", 3, false); // SP 034
    sunluyu->addSkill(new Meibu);
    sunluyu->addSkill(new Mumu);

    General *maliang = new General(this, "maliang", "shu", 3); // SP 035
    maliang->addSkill(new Xiemu);
    maliang->addSkill(new Naman);

    General *chengyu = new General(this, "chengyu", "wei", 3); // SP 036
    chengyu->addSkill(new Shefu);
    chengyu->addSkill(new ShefuCancel);
    chengyu->addSkill(new Benyu);
    related_skills.insertMulti("shefu", "#shefu-cancel");

    General *ganfuren = new General(this, "ganfuren", "shu", 3, false); // SP 037
    ganfuren->addSkill(new Shushen);
    ganfuren->addSkill(new Shenzhi);

    General *huangjinleishi = new General(this, "huangjinleishi", "qun", 3, false); // SP 038
    huangjinleishi->addSkill(new Fulu);
    huangjinleishi->addSkill(new Zhuji);

	General *sp_wenpin = new General(this, "sp_wenpin", "wei"); // SP 039
    sp_wenpin->addSkill(new SpZhenwei);

    General *simalang = new General(this, "simalang", "wei", 3); // SP 040
    simalang->addSkill(new Quji);
    simalang->addSkill(new Junbing);

    General *sunhao = new General(this, "sunhao$", "wu", 5); // SP 041
    sunhao->addSkill(new Canshi);
    sunhao->addSkill(new Chouhai);
    sunhao->addSkill(new Guiming);

    addMetaObject<YuanhuCard>();
    addMetaObject<XuejiCard>();
    addMetaObject<BifaCard>();
    addMetaObject<SongciCard>();
    addMetaObject<ZhoufuCard>();
    addMetaObject<QiangwuCard>();
    addMetaObject<YinbingCard>();
    addMetaObject<XiemuCard>();
    addMetaObject<ShefuCard>();
    addMetaObject<BenyuCard>();
	addMetaObject<QujiCard>();
    addMetaObject<HongyuanCard>();
    addMetaObject<FenxunCard>();
    addMetaObject<MizhaoCard>();

    skills << new Weizhong << new MeibuFilter("meibu");
}

ADD_PACKAGE(SP)

OLPackage::OLPackage()
    : Package("OL")
{
    General *zhugeke = new General(this, "zhugeke", "wu", 3); // OL 002
    zhugeke->addSkill(new Aocai);
    zhugeke->addSkill(new Duwu);

    General *lingcao = new General(this, "lingcao", "wu", 4);
    lingcao->addSkill(new Dujin);

    General *sunru = new General(this, "sunru", "wu", 3, false);
    sunru->addSkill(new Qingyi);
    sunru->addSkill(new SlashNoDistanceLimitSkill("qingyi"));
    sunru->addSkill(new Shixin);
    related_skills.insertMulti("qingyi", "#qingyi-slash-ndl");

	General *liuzan = new General(this, "liuzan", "wu");
    liuzan->addSkill(new Fenyin);

    General *lifeng = new General(this, "lifeng", "shu", 3);
    lifeng->addSkill(new TunchuDraw);
    lifeng->addSkill(new TunchuEffect);
    lifeng->addSkill(new Tunchu);
    lifeng->addSkill(new Shuliang);
    related_skills.insertMulti("tunchu", "#tunchu-effect");
    related_skills.insertMulti("tunchu", "#tunchu-disable");

    General *zhuling = new General(this, "zhuling", "wei");
    zhuling->addSkill(new Zhanyi);
    zhuling->addSkill(new ZhanyiDiscard2);
    zhuling->addSkill(new ZhanyiNoDistanceLimit);
    zhuling->addSkill(new ZhanyiRemove);
    related_skills.insertMulti("zhanyi", "#zhanyi-basic");
    related_skills.insertMulti("zhanyi", "#zhanyi-equip");
    related_skills.insertMulti("zhanyi", "#zhanyi-trick");

	General *zhugeguo = new General(this, "zhugeguo", "shu", 3, false);
	zhugeguo->addSkill(new Yuhua);
    zhugeguo->addSkill(new YuhuaKeep);
	zhugeguo->addSkill(new Qirang);
	related_skills.insertMulti("yuhua", "#yuhua");

    General *ol_masu = new General(this, "ol_masu", "shu", 3, true, Config.value("EnableHidden", true).toBool());
    ol_masu->addSkill(new Sanyao);
    ol_masu->addSkill(new Zhiman);

    General *ol_madai = new General(this, "ol_madai", "shu", 4, true, Config.value("EnableHidden", true).toBool());
    ol_madai->addSkill("mashu");
    ol_madai->addSkill(new OlQianxi);
    ol_madai->addSkill(new OlQianxiClear);
    related_skills.insertMulti("olqianxi", "#olqianxi-clear");

    General *ol_wangyi = new General(this, "ol_wangyi", "wei", 3, false, Config.value("EnableHidden", true).toBool());
    ol_wangyi->addSkill("zhenlie");
    ol_wangyi->addSkill(new OlMiji);

	General *ol_yujin = new General(this, "ol_yujin", "wei", 4, true, Config.value("EnableHidden", true).toBool());
    ol_yujin->addSkill(new Jieyue);

	General *ol_liubiao = new General(this, "ol_liubiao", "qun", 3, true, Config.value("EnableHidden", true).toBool());
    ol_liubiao->addSkill(new OlZishou);
    ol_liubiao->addSkill(new OlZishouProhibit);
    ol_liubiao->addSkill("zongshi");

	General *ol_xingcai = new General(this, "ol_xingcai", "shu", 3, false, Config.value("EnableHidden", true).toBool());
    ol_xingcai->addSkill(new OlShenxian);
    ol_xingcai->addSkill("qiangwu");

	General *ol_xiaohu = new General(this, "ol_sunluyu", "wu", 3, false, Config.value("EnableHidden", true).toBool());
    ol_xiaohu->addSkill(new OlMeibu);
    ol_xiaohu->addSkill(new OlMumu);

	General *ol_xusheng = new General(this, "ol_xusheng", "wu", 4, true, Config.value("EnableHidden", true).toBool());
    ol_xusheng->addSkill(new OlPojun);
    ol_xusheng->addSkill(new FakeMoveSkill("olpojun"));
    related_skills.insertMulti("olpojun", "#olpojun-fake-move");

    General *ol_x1aohu = new General(this, "ol_sun1uyu", "wu", 3, false, Config.value("EnableHidden", true).toBool());
    ol_x1aohu->addSkill(new OlMeibu2);
    ol_x1aohu->addSkill(new OlMumu2);

    General *shixie = new General(this, "shixie", "qun", 3);
    shixie->addSkill(new Biluan);
    shixie->addSkill(new BiluanDist);
    shixie->addSkill(new Lixia);
    shixie->addSkill(new LixiaDist);
    related_skills.insertMulti("biluan", "#biluan-dist");
    related_skills.insertMulti("lixia", "#lixia-dist");

    General *zhanglu = new General(this, "zhanglu", "qun", 3);
    zhanglu->addSkill(new Yishe);
    zhanglu->addSkill(new Bushi);
    zhanglu->addSkill(new Midao);

    General *mayl = new General(this, "mayunlu", "shu", 4, false);
    mayl->addSkill("mashu");
    mayl->addSkill(new Fengpo);
    mayl->addSkill(new FengpoRecord);
    related_skills.insertMulti("fengpo", "#fengpo-record");


    General *olDB = new General(this, "ol_caiwenji", "wei", 3, false, Config.value("EnableHidden", true).toBool());
    olDB->addSkill(new Chenqing);
    olDB->addSkill(new Moshi);

	 General *olliubei = new General(this, "ol_liubei$", "shu", 4, true, Config.value("EnableHidden", true).toBool());
    olliubei->addSkill(new OlRende);
    olliubei->addSkill("jijiang");

    General *ol_xiahd = new General(this, "ol_xiahoudun", "wei", 4, true, Config.value("EnableHidden", true).toBool());
    ol_xiahd->addSkill("ganglie");
    ol_xiahd->addSkill(new OlQingjian);

		General *ol_zhangjiao = new General(this, "ol_zhangjiao$", "qun", 3, true, Config.value("EnableHidden", true).toBool());
    ol_zhangjiao->addSkill(new OlLeiji);
	ol_zhangjiao->addSkill("guidao");
	ol_zhangjiao->addSkill("huangtian");

	General *ol_yuanshu = new General(this, "ol_yuanshu", "qun", 4, true, Config.value("EnableHidden", true).toBool());
    ol_yuanshu->addSkill(new OlYongsi);
	ol_yuanshu->addSkill(new OlJixi);

	General *ol_bulianshi = new General(this, "ol_bulianshi", "wu", 3, false, Config.value("EnableHidden", true).toBool());
	ol_bulianshi->addSkill(new OlAnxu);
	ol_bulianshi->addSkill("zhuiyi");

    addMetaObject<AocaiCard>();
    addMetaObject<DuwuCard>();
    addMetaObject<QingyiCard>();
    addMetaObject<SanyaoCard>();
    addMetaObject<JieyueCard>();
	addMetaObject<ShuliangCard>();
	addMetaObject<OlMumuCard>();
    addMetaObject<ZhanyiCard>();
    addMetaObject<ZhanyiViewAsBasicCard>();
	addMetaObject<OlMumu2Card>();
	addMetaObject<MidaoCard>();
    addMetaObject<BushiCard>();
	addMetaObject<OlRendeCard>();
    addMetaObject<OlQingjianCard>();
	addMetaObject<OlAnxuCard>();

	skills << new MeibuFilter("olmeibu") << new MeibuFilter("olzhixi") << new OlZhixi << new OldMoshi;
}

ADD_PACKAGE(OL)

TaiwanYJCMPackage::TaiwanYJCMPackage()
    : Package("Taiwan_yjcm")
{
    General *xiahb = new General(this, "twyj_xiahouba", "shu", 4, true, Config.value("EnableHidden", true).toBool()); // TAI 001
    xiahb->addSkill(new Yinqin);
    xiahb->addSkill(new TWBaobian);

    General *zumao = new General(this, "twyj_zumao", "wu", 4, true, Config.value("EnableHidden", true).toBool()); // TAI 002
    zumao->addSkill(new Tijin);

    General *caoang = new General(this, "twyj_caoang", "wei", 4, true, Config.value("EnableHidden", true).toBool()); // TAI 003
    caoang->addSkill(new XiaolianDist);
    caoang->addSkill(new Xiaolian);
    related_skills.insertMulti("xiaolian", "#xiaolian-dist");
}

ADD_PACKAGE(TaiwanYJCM)

MiscellaneousPackage::MiscellaneousPackage()
    : Package("miscellaneous")
{
	General *Caesar = new General(this, "caesar", "god", 4); // E.SP 001
    Caesar->addSkill(new Conqueror);
}

ADD_PACKAGE(Miscellaneous)

JSPPackage::JSPPackage()
    : Package("jiexian_sp")
{
    General *jsp_sunshangxiang = new General(this, "jsp_sunshangxiang", "shu", Config.value("heg_skill", true).toBool() ? 4 : 3, false, Config.value("EnableHidden", true).toBool()); // JSP 001
	if (Config.value("heg_skill", true).toBool())
		jsp_sunshangxiang->addSkill("xiaoji");
	else{
		jsp_sunshangxiang->addSkill(new Fanxiang);
		jsp_sunshangxiang->addSkill(new Liangzhu);
	}
	
    General *jsp_machao = new General(this, "jsp_machao", "qun", 4, true, Config.value("EnableHidden", true).toBool()); // JSP 002
    jsp_machao->addSkill(new Skill("zhuiji", Skill::Compulsory));
    jsp_machao->addSkill(new Cihuai);

    General *jsp_guanyu = new General(this, "jsp_guanyu", "wei", Config.value("heg_skill", true).toBool() ? 3 : 4, true, Config.value("EnableHidden", true).toBool()); // JSP 003
    jsp_guanyu->addSkill("wusheng");
	if (Config.value("heg_skill", true).toBool())
		jsp_guanyu->addSkill("nuzhan");
	else{
		jsp_guanyu->addSkill(new JspDanqi);
		jsp_guanyu->addRelateSkill("nuzhan");
	}

    General *jsp_jiangwei = new General(this, "jsp_jiangwei", "wei", Config.value("heg_skill", true).toBool() ? 3 : 4, true, Config.value("EnableHidden", true).toBool()); // JSP 004
    jsp_jiangwei->addSkill(new Kunfen);
	if (Config.value("heg_skill", true).toBool())
		jsp_jiangwei->addSkill("tiaoxin");
	else
		jsp_jiangwei->addSkill(new Fengliang);

	General *jsp_zhaoyun = new General(this, "jsp_zhaoyun", "qun", 3, true, Config.value("EnableHidden", true).toBool());
    jsp_zhaoyun->addSkill(new ChixinTrigger);
    jsp_zhaoyun->addSkill(new Suiren);
    jsp_zhaoyun->addSkill("yicong");

	General *jsp_huangyy = new General(this, "jsp_huangyueying", "qun", 3, false, Config.value("EnableHidden", true).toBool());
    jsp_huangyy->addSkill(new Jiqiao);
    jsp_huangyy->addSkill(new Linglong);
    jsp_huangyy->addSkill(new LinglongTreasure);
    jsp_huangyy->addSkill(new LinglongMax);
    related_skills.insertMulti("linglong", "#linglong-horse");
    related_skills.insertMulti("linglong", "#linglong-treasure");


    skills << new Nuzhan;
	addMetaObject<JiqiaoCard>();
}

ADD_PACKAGE(JSP)