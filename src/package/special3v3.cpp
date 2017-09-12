#include "special3v3.h"
#include "skill.h"
#include "standard.h"
#include "engine.h"
#include "ai.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "settings.h"

class VsGanglie: public MasochismSkill {
public:
    VsGanglie(): MasochismSkill("vsganglie") {
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &) const{
        Room *room = player->getRoom();
        QString mode = room->getMode();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if ((!mode.startsWith("06_") && !mode.startsWith("04_")) || AI::GetRelation3v3(player, p) == AI::Enemy)
                targets << p;
        }

        ServerPlayer *from = room->askForPlayerChosen(player, targets, objectName(), "vsganglie-invoke", true, true);
        if (!from) return;

       room->broadcastSkillInvoke(objectName());

		room->addPlayerMark(player, objectName()+"engine");
		if (player->getMark(objectName()+"engine") > 0) {
			JudgeStruct judge;
			judge.pattern = ".|heart";
			judge.good = false;
			judge.reason = objectName();
			judge.who = player;

			room->judge(judge);
			if (from->isDead()) return;
			if (judge.isGood()) {
				if (from->getHandcardNum() < 2 || !room->askForDiscard(from, objectName(), 2, 2, true))
					room->damage(DamageStruct(objectName(), player, from));
			}
		}
        room->removePlayerMark(player, objectName()+"engine");
    }
};

ZhongyiCard::ZhongyiCard() {
    mute = true;
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void ZhongyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->broadcastSkillInvoke("zhongyi");
    //room->doLightbox("$ZhongyiAnimate");
    room->addPlayerMark(source, "zhongyiengine");
    if (source->getMark("zhongyiengine") > 0) {
		room->removePlayerMark(source, "@loyal");
		source->addToPile("loyal", this);
        room->removePlayerMark(source, "zhongyiengine");
    }
}

class Zhongyi: public OneCardViewAsSkill {
public:
    Zhongyi(): OneCardViewAsSkill("zhongyi") {
        frequency = Limited;
        limit_mark = "@loyal";
        filter_pattern = ".|red|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && player->getMark("@loyal") > 0;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ZhongyiCard *card = new ZhongyiCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ZhongyiAction: public TriggerSkill {
public:
    ZhongyiAction(): TriggerSkill("#zhongyi-action") {
        events << DamageCaused << EventPhaseStart << ActionedReset;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QString mode = room->getMode();
        if (triggerEvent == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.chain || damage.transfer || !damage.by_user) return false;
            if (damage.card && damage.card->isKindOf("Slash")) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getPile("loyal").isEmpty()) continue;
                    bool on_effect = false;
                    if (room->getMode().startsWith("06_") || room->getMode().startsWith("04_"))
                        on_effect = (AI::GetRelation3v3(player, p) == AI::Friend);
                    else
                        on_effect = (room->askForSkillInvoke(p, "zhongyi", data));
                    if (on_effect) {
                        LogMessage log;
                        log.type = "#ZhongyiBuff";
                        log.from = p;
                        log.to << damage.to;
                        log.arg = QString::number(damage.damage);
                        log.arg2 = QString::number(++damage.damage);
                        room->sendLog(log);
                    }
                }
            }
            data = QVariant::fromValue(damage);
        } else if ((mode == "06_3v3" && triggerEvent == ActionedReset) || (mode != "06_3v3" && triggerEvent == EventPhaseStart)) {
            if (triggerEvent == EventPhaseStart && player->getPhase() != Player::RoundStart)
                return false;
            if (player->getPile("loyal").length() > 0)
                player->clearOnePrivatePile("loyal");
        }
        return false;
    }
};

JiuzhuCard::JiuzhuCard() {
    target_fixed = true;
}

void JiuzhuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    ServerPlayer *who = room->getCurrentDyingPlayer();
    if (!who) return;

	room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), who->objectName());
    room->loseHp(source);
    room->addPlayerMark(source, "jiuzhuengine");
    if (source->getMark("jiuzhuengine") > 0) {
		room->recover(who, RecoverStruct(source));
        room->removePlayerMark(source, "jiuzhuengine");
    }
}

class Jiuzhu: public OneCardViewAsSkill {
public:
    Jiuzhu(): OneCardViewAsSkill("jiuzhu") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (pattern != "peach" || !player->canDiscard(player, "he") || player->getHp() <= 1) return false;
        QString dyingobj = player->property("currentdying").toString();
        const Player *who = NULL;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->objectName() == dyingobj) {
                who = p;
                break;
            }
        }
        if (!who) return false;
        if (who == Self) return false;
        if (ServerInfo.GameMode.startsWith("06_"))
            return player->getRole().at(0) == who->getRole().at(0);
        else
            return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        JiuzhuCard *card = new JiuzhuCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Zhanshen: public TriggerSkill {
public:
    Zhanshen(): TriggerSkill("zhanshen") {
        events << Death << EventPhaseStart;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return false;
            foreach (ServerPlayer *lvbu, room->getAllPlayers()) {
                if (!TriggerSkill::triggerable(lvbu)) continue;
                if (room->getMode().startsWith("06_") || room->getMode().startsWith("04_")) {
                    if (lvbu->getMark(objectName()) == 0 && lvbu->getMark("zhanshen_fight") == 0
                        && AI::GetRelation3v3(lvbu, player) == AI::Friend)
                        lvbu->addMark("zhanshen_fight");
                } else {
                    if (lvbu->getMark(objectName()) == 0 && lvbu->getMark("@fight") == 0
                        && room->askForSkillInvoke(player, objectName(), "mark:" + lvbu->objectName()))
                        room->addPlayerMark(lvbu, "@fight");
                }
            }
        } else if (TriggerSkill::triggerable(player)
                   && player->getPhase() == Player::Start
                   && player->getMark(objectName()) == 0
                   && player->isWounded()
                   && (player->getMark("zhanshen_fight") > 0 || player->getMark("@fight") > 0)) {
            room->notifySkillInvoked(player, objectName());

            LogMessage log;
            log.type = "#ZhanshenWake";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            room->broadcastSkillInvoke(objectName());
            //room->doLightbox("$ZhanshenAnimate");

            if (player->getMark("@fight") > 0)
                room->setPlayerMark(player, "@fight", 0);
            player->setMark("zhanshen_fight", 0);

			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				room->setPlayerMark(player, "zhanshen", 1);
				if (room->changeMaxHpForAwakenSkill(player)) {
					if (player->getWeapon())
						room->throwCard(player->getWeapon(), player);
					if (player->getMark("zhanshen") == 1)
						room->handleAcquireDetachSkills(player, "mashu|shenji");
				}
				room->removePlayerMark(player, objectName()+"engine");
			}
        }
        return false;
    }
};

class ZhenweiDistance: public DistanceSkill {
public:
    ZhenweiDistance(): DistanceSkill("#zhenwei") {
    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        if (ServerInfo.GameMode.startsWith("06_") || ServerInfo.GameMode.startsWith("04_")
            || ServerInfo.GameMode == "08_defense") {
            int dist = 0;
            if (from->getRole().at(0) != to->getRole().at(0)) {
                foreach (const Player *p, to->getAliveSiblings()) {
                    if (p->hasSkill("zhenwei") && p->getRole().at(0) == to->getRole().at(0))
                        ++dist;
                }
            }
            return dist;
        } else if (to->getMark("@defense") > 0 && from->getMark("@defense") == 0  && from->objectName() != to->property("zhenwei_from").toString()) {
            return 1;
        }
        return 0;
    }
};

ZhenweiCard::ZhenweiCard() {
}

bool ZhenweiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    int total = Self->getSiblings().length() + 1;
    return targets.length() < total / 2 - 1 && to_select != Self;
}

void ZhenweiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->setPlayerProperty(effect.to, "zhenwei_from", QVariant::fromValue(effect.from->objectName()));
    room->addPlayerMark(effect.to, "@defense");
}

class ZhenweiViewAsSkill: public ZeroCardViewAsSkill {
public:
    ZhenweiViewAsSkill(): ZeroCardViewAsSkill("zhenwei") {
        response_pattern = "@@zhenwei";
    }

    virtual const Card *viewAs() const{
        return new ZhenweiCard;
    }
};

class Zhenwei: public TriggerSkill {
public:
    Zhenwei(): TriggerSkill("zhenwei") {
        events << EventPhaseChanging << Death << Damaged;
        view_as_skill = new ZhenweiViewAsSkill;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        QString mode = target->getRoom()->getMode();
        return !mode.startsWith("06_") && !mode.startsWith("04_") && mode != "08_defense";
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player || !player->hasSkill(objectName(), true))
                return false;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->property("zhenwei_from").toString() == player->objectName()) {
                    room->setPlayerProperty(p, "zhenwei_from", QVariant());
                    room->setPlayerMark(p, "@defense", 0);
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            if (!TriggerSkill::triggerable(player) || Sanguosha->getPlayerCount(room->getMode()) <= 3)
                return false;
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->property("zhenwei_from").toString() == player->objectName()) {
                    room->setPlayerProperty(p, "zhenwei_from", QVariant());
                    room->setPlayerMark(p, "@defense", 0);
                }
            }
            room->askForUseCard(player, "@@zhenwei", "@zhenwei");
        } else if (triggerEvent == Damaged) {
            if (!TriggerSkill::triggerable(player) || Sanguosha->getPlayerCount(room->getMode()) <= 3)
                return false;
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->property("zhenwei_from").toString() == player->objectName()) {
                    room->setPlayerProperty(p, "zhenwei_from", QVariant());
                    room->setPlayerMark(p, "@defense", 0);
                }
            }
            room->askForUseCard(player, "@@zhenwei", "@zhenwei");
        }
        return false;
    }
};

VSCrossbow::VSCrossbow(Suit suit, int number)
    : Crossbow(suit, number)
{
    setObjectName("vscrossbow");
}

bool VSCrossbow::match(const QString &pattern) const{
    QStringList patterns = pattern.split("+");
    if (patterns.contains("crossbow"))
        return true;
    else
        return Crossbow::match(pattern);
}

New3v3CardPackage::New3v3CardPackage()
    : Package("New3v3Card")
{
    QList<Card *> cards;
    cards << new SupplyShortage(Card::Spade, 1)
          << new SupplyShortage(Card::Club, 12)
          << new Nullification(Card::Heart, 12);

    foreach (Card *card, cards)
        card->setParent(this);

    type = CardPack;
}

ADD_PACKAGE(New3v3Card)

New3v3_2013CardPackage::New3v3_2013CardPackage()
    : Package("New3v3_2013Card")
{
    QList<Card *> cards;
    cards << new VSCrossbow(Card::Club)
          << new VSCrossbow(Card::Diamond);

    foreach (Card *card, cards)
        card->setParent(this);

    type = CardPack;
}

ADD_PACKAGE(New3v3_2013Card)

Special3v3Package::Special3v3Package()
    : Package("Special3v3")
{
    General *vs_nos_xiahoudun = new General(this, "vs_nos_xiahoudun", "wei", 4, true, Config.value("EnableHidden", true).toBool());
    vs_nos_xiahoudun->addSkill(new VsGanglie);

    General *vs_nos_guanyu = new General(this, "vs_nos_guanyu", "shu", 4, true, Config.value("EnableHidden", true).toBool());
    vs_nos_guanyu->addSkill("wusheng");
    vs_nos_guanyu->addSkill(new Zhongyi);
    vs_nos_guanyu->addSkill(new ZhongyiAction);
    related_skills.insertMulti("zhongyi", "#zhongyi-action");

    General *vs_nos_zhaoyun = new General(this, "vs_nos_zhaoyun", "shu", 4, true, Config.value("EnableHidden", true).toBool());
    vs_nos_zhaoyun->addSkill("longdan");
    vs_nos_zhaoyun->addSkill(new Jiuzhu);

    General *vs_nos_lvbu = new General(this, "vs_nos_lvbu", "qun", 4, true, Config.value("EnableHidden", true).toBool());
    vs_nos_lvbu->addSkill("wushuang");
    vs_nos_lvbu->addSkill(new Zhanshen);

    addMetaObject<ZhongyiCard>();
    addMetaObject<JiuzhuCard>();
}

ADD_PACKAGE(Special3v3)

Special3v3ExtPackage::Special3v3ExtPackage()
    : Package("Special3v3Ext")
{
    General *wenpin = new General(this, "wenpin", "wei", 4, true, Config.value("EnableHidden", true).toBool()); // WEI 019
    wenpin->addSkill(new Zhenwei);
    wenpin->addSkill(new ZhenweiDistance);
    related_skills.insertMulti("zhenwei", "#zhenwei");

    addMetaObject<ZhenweiCard>();
}

ADD_PACKAGE(Special3v3Ext)
