#include "general.h"
#include "standard.h"
#include "skill.h"
#include "engine.h"
#include "client.h"
#include "serverplayer.h"
#include "room.h"
#include "standard-skillcards.h"
#include "ai.h"
#include "settings.h"

class Jianxiong: public MasochismSkill {
public:
    Jianxiong(): MasochismSkill("jianxiong") {
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const{
        Room *room = player->getRoom();
        QVariant data = QVariant::fromValue(damage);
        QStringList choices;
        choices << "draw" << "cancel";

        const Card *card = damage.card;
        if (card) {
            QList<int> ids;
            if (card->isVirtualCard())
                ids = card->getSubcards();
            else
                ids << card->getEffectiveId();
            if (ids.length() > 0) {
                bool all_place_table = true;
                foreach (int id, ids) {
                    if (room->getCardPlace(id) != Player::PlaceTable) {
                        all_place_table = false;
                        break;
                    }
                }
                if (all_place_table) choices.append("obtain");
            }
        }

        QString choice = room->askForChoice(player, objectName(), choices.join("+"), data);
        if (choice != "cancel") {
            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            room->notifySkillInvoked(player, objectName());
            if (choice == "obtain")
				room->broadcastSkillInvoke(objectName(), 2);
            else
				room->broadcastSkillInvoke(objectName(), 1);
            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                if (choice == "obtain")
                    player->obtainCard(card);
                else
                    player->drawCards(1, objectName());
                
                room->removePlayerMark(player, objectName()+"engine");
            }
        }
    }
};

class Hujia: public TriggerSkill {
public:
    Hujia(): TriggerSkill("hujia$") {
        events << CardAsked;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasLordSkill("hujia");
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QString pattern = data.toStringList().first();
        QString prompt = data.toStringList().at(1);
        if (pattern != "jink" || prompt.startsWith("@hujia-jink"))
            return false;

        QList<ServerPlayer *> lieges = room->getLieges("wei", player);
        if (lieges.isEmpty())
            return false;

        if (!room->askForSkillInvoke(player, objectName(), data))
            return false;

        int index = qrand() % 2 + 1;
        if (Player::isNostalGeneral(player, "caocao"))
            index += 2;
        room->broadcastSkillInvoke(objectName(), index);

        foreach (ServerPlayer *liege, lieges) {
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), liege->objectName());
		}
        room->addPlayerMark(player, objectName()+"engine");
        if (player->getMark(objectName()+"engine") > 0) {
            QVariant tohelp = QVariant::fromValue(player);
            foreach (ServerPlayer *liege, lieges) {
                const Card *jink = room->askForCard(liege, "jink", "@hujia-jink:" + player->objectName(),
                    tohelp, Card::MethodResponse, player, false, QString(), true);
                if (jink && !jink->hasFlag("response_failed")) {
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, liege->objectName(), player->objectName());
                    room->setCardFlag(jink, "-response_failed");
                    room->provide(jink);
                    return true;
                }
            }
            room->removePlayerMark(player, objectName()+"engine");
        }

        return false;
    }
};

class TuxiViewAsSkill: public ZeroCardViewAsSkill {
public:
    TuxiViewAsSkill(): ZeroCardViewAsSkill("tuxi") {
        response_pattern = "@@tuxi";
    }

    virtual const Card *viewAs() const{
        return new TuxiCard;
    }
};

class Tuxi: public DrawCardsSkill {
public:
    Tuxi(): DrawCardsSkill("tuxi") {
        view_as_skill = new TuxiViewAsSkill;
    }

    virtual int getPriority(TriggerEvent) const{
        return 1;
    }

    virtual int getDrawNum(ServerPlayer *zhangliao, int n) const{
        Room *room = zhangliao->getRoom();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(zhangliao))
            if (p->getHandcardNum() >= zhangliao->getHandcardNum())
                targets << p;
        int num = qMin(targets.length(), n);
        foreach (ServerPlayer *p, room->getOtherPlayers(zhangliao))
            p->setFlags("-TuxiTarget");

        if (num > 0) {
            room->setPlayerMark(zhangliao, "tuxi", num);
            int count = 0;
            if (room->askForUseCard(zhangliao, "@@tuxi", "@tuxi-card:::" + QString::number(num))) {
                
                foreach (ServerPlayer *p, room->getOtherPlayers(zhangliao))
                    if (p->hasFlag("TuxiTarget")) ++count;
            } else {
                room->setPlayerMark(zhangliao, "tuxi", 0);
            }
            return n - count;
        } else
            return n;
    }
};

class TuxiAct: public TriggerSkill {
public:
    TuxiAct(): TriggerSkill("#tuxi") {
        events << AfterDrawNCards;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return player != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getMark("tuxi") == 0) return false;
        room->setPlayerMark(player, "tuxi", 0);

        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->hasFlag("TuxiTarget")) {
                p->setFlags("-TuxiTarget");
                targets << p;
            }
        }
        room->addPlayerMark(player, "tuxiengine");
        if (player->getMark("tuxiengine") > 0) {
            foreach (ServerPlayer *p, targets) {
                if (!player->isAlive())
                    break;
                if (p->isAlive() && !p->isKongcheng()) {
                    int card_id = room->askForCardChosen(player, p, "h", "tuxi");

                    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
                    room->obtainCard(player, Sanguosha->getCard(card_id), reason, false);
                }
            }
            room->removePlayerMark(player, "tuxiengine");
        }
        return false;
    }
};

class Tiandu: public TriggerSkill {
public:
    Tiandu(): TriggerSkill("tiandu") {
        frequency = Frequent;
        events << FinishJudge;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        JudgeStruct *judge = data.value<JudgeStruct *>();
        const Card *card = judge->card;

        QVariant data_card = QVariant::fromValue(card);
        if (room->getCardPlace(card->getEffectiveId()) == Player::PlaceJudge && player->askForSkillInvoke(objectName(), data_card)) {
            int effectIndex = qrand() % 2 + 1;
            if (Player::isNostalGeneral(player, "guojia")) {
                effectIndex += 2;
            } else if (player->getGeneralName() == "xizhicai") {
                effectIndex += 4;
			}
            room->broadcastSkillInvoke(objectName(), effectIndex);
            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                player->obtainCard(judge->card);
                room->removePlayerMark(player, objectName()+"engine");
            }
            return false;
        }

        return false;
    }
};

class YijiViewAsSkill: public ZeroCardViewAsSkill {
public:
    YijiViewAsSkill(): ZeroCardViewAsSkill("yiji") {
        response_pattern = "@@yiji";
    }

    virtual const Card *viewAs() const{
        return new YijiCard;
    }
};

class Yiji: public MasochismSkill {
public:
    Yiji(): MasochismSkill("yiji") {
        view_as_skill = new YijiViewAsSkill;
        frequency = Frequent;
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const{
        Room *room = target->getRoom();
        for (int i = 0; i < damage.damage; ++i) {
            if (target->isAlive() && room->askForSkillInvoke(target, objectName(), QVariant::fromValue(damage))) {
                room->broadcastSkillInvoke(objectName(), 2);
                room->addPlayerMark(target, objectName()+"engine");
                if (target->getMark(objectName()+"engine") > 0) {
                    target->drawCards(2, objectName());
                    room->askForUseCard(target, "@@yiji", "@yiji");
                    room->removePlayerMark(target, objectName()+"engine");
                }
            } else {
                break;
            }
        }
    }
};

class YijiObtain: public PhaseChangeSkill {
public:
    YijiObtain(): PhaseChangeSkill("#yiji") {
    }

    virtual int getPriority(TriggerEvent) const{
        return 4;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        if (target->getPhase() == Player::Draw && !target->getPile("ruse").isEmpty()) {
            room->broadcastSkillInvoke("yiji", 1);
            DummyCard *dummy = new DummyCard(target->getPile("ruse"));
            CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, target->objectName(), "yiji", QString());
            room->obtainCard(target, dummy, reason, false);
            delete dummy;
        }
        return false;
    }
};

class Ganglie: public TriggerSkill {
public:
    Ganglie(): TriggerSkill("ganglie") {
        events << Damaged << FinishJudge;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == Damaged && TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *from = damage.from;

            for (int i = 0; i < damage.damage; ++i) {
                if (room->askForSkillInvoke(player, "ganglie", data)) {
                    room->broadcastSkillInvoke(objectName());
                    room->addPlayerMark(player, objectName()+"engine");
                    if (player->getMark(objectName()+"engine") > 0) {

						JudgeStruct judge;
						judge.pattern = ".";
						judge.play_animation = false;
						judge.reason = objectName();
						judge.who = player;

						room->judge(judge);
						room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.from->objectName());
						if (!from || from->isDead()) continue;
						Card::Suit suit = (Card::Suit)(judge.pattern.toInt());
						switch (suit) {
						case Card::Heart:
						case Card::Diamond: {
							room->damage(DamageStruct(objectName(), player, from));
							break;
						}
						case Card::Club:
						case Card::Spade: {
							if (player->canDiscard(from, "he")) {
								int id = room->askForCardChosen(player, from, "he", objectName(), false, Card::MethodDiscard);
								room->throwCard(id, from, player);
							}
							break;
						}
						default:
							break;
						}
                        room->removePlayerMark(player, objectName()+"engine");
                    }
                } else {
                    break;
                }
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason != objectName()) return false;
            judge->pattern = QString::number(int(judge->card->getSuit()));
        }
        return false;
    }
};

class Qingjian: public TriggerSkill {
public:
    Qingjian(): TriggerSkill("qingjian") {
        events << CardsMoveOneTime << AfterGiveCards;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (triggerEvent == CardsMoveOneTime || triggerEvent == AfterGiveCards) {
            if (!room->getTag("FirstRound").toBool()
                && player->getPhase() != Player::Draw
                && move.to == player && move.to_place == Player::PlaceHand
				) {
                QList<int> ids;
                foreach (int id, move.card_ids) {
                    if (room->getCardOwner(id) == player
                        && room->getCardPlace(id) == Player::PlaceHand) {
                        ids << id;
                    }
                }
                if (ids.isEmpty()) {
                    return false;
                }

                player->tag["QingjianCurrentMoveSkill"] = QVariant(move.reason.m_skillName);
                while (room->askForYiji(player, ids, objectName(), false, false, true, -1, QList<ServerPlayer *>(), CardMoveReason(), "@qingjian-distribute", true)) {
					room->addPlayerMark(player, objectName()+"engine");
					if (player->isDead()) return false;
					room->removePlayerMark(player, objectName()+"engine");
                }
            }
        }
        return false;
    }
};

class Fankui: public MasochismSkill {
public:
    Fankui(): MasochismSkill("fankui") {
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const{
        ServerPlayer *from = damage.from;
        Room *room = player->getRoom();
        for (int i = 0; i < damage.damage; ++i) {
            QVariant data = QVariant::fromValue(from);
            if (from && !from->isNude() && room->askForSkillInvoke(player, "fankui", data)) {
                room->broadcastSkillInvoke(objectName());
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.from->objectName());
                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {
                int card_id = room->askForCardChosen(player, from, "he", "fankui");
                CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
                room->obtainCard(player, Sanguosha->getCard(card_id),
                    reason, room->getCardPlace(card_id) != Player::PlaceHand);
                    room->removePlayerMark(player, objectName()+"engine");
                }
            } else {
                break;
            }
        }
    }
};

class Guicai: public TriggerSkill {
public:
    Guicai(): TriggerSkill("guicai") {
        events << AskForRetrial;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->isNude())
            return false;

        JudgeStruct *judge = data.value<JudgeStruct *>();

        QStringList prompt_list;
        prompt_list << "@guicai-card" << judge->who->objectName()
                    << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");
        bool forced = false;
        if (player->getMark("JilveEvent") == int(AskForRetrial))
            forced = true;
        const Card *card = room->askForCard(player, forced ? "..!" : ".." , prompt, data, Card::MethodResponse, judge->who, true);
       if (forced && card == NULL) {
            QList<const Card *> c = player->getCards("he");
            card = c.at(qrand() % c.length());
        }
        if (card) {
            if (player->hasInnateSkill("guicai") && !player->hasSkill("jilve") && Config.value("simayi_down", true).toBool())
                room->broadcastSkillInvoke(objectName());
            else
                room->broadcastSkillInvoke("jilve", 1);
			room->addPlayerMark(player, objectName()+"engine");
			if (card && player->getMark(objectName()+"engine") > 0) {
				room->retrial(card, player, judge, objectName());
				room->removePlayerMark(player, objectName()+"engine");
			}
        }

        return false;
    }
};

class LuoyiBuff: public TriggerSkill {
public:
    LuoyiBuff(): TriggerSkill("#luoyi") {
        events << DamageCaused;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getMark("@luoyi") > 0 && target->isAlive();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer) return false;
        const Card *reason = damage.card;
        if (reason && (reason->isKindOf("Slash") || reason->isKindOf("Duel"))) {
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());
            room->broadcastSkillInvoke("luoyi", 1);
            LogMessage log;
            log.type = "#LuoyiBuff";
            log.from = player;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);

            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

class Luoyi: public TriggerSkill {
public:
    Luoyi(): TriggerSkill("luoyi") {
        events << EventPhaseStart << EventPhaseChanging;
    }

    virtual int getPriority(TriggerEvent triggerEvent) const{
        if (triggerEvent == EventPhaseStart)
            return 4;
        else
            return TriggerSkill::getPriority(triggerEvent);
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::RoundStart && player->getMark("@luoyi") > 0)
                room->setPlayerMark(player, "@luoyi", 0);
        } else {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (TriggerSkill::triggerable(player) && change.to == Player::Draw && !player->isSkipped(Player::Draw)
                && room->askForSkillInvoke(player, objectName())) {
                room->broadcastSkillInvoke(objectName(), 2);
                player->skip(Player::Draw, true);
                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {

                    QList<int> ids = room->getNCards(3, false);
                    CardsMoveStruct move(ids, player, Player::PlaceTable,
                        CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), "luoyi", QString()));
                    room->moveCardsAtomic(move, true);
                    room->getThread()->delay();
                    room->getThread()->delay();

                    QList<int> card_to_throw;
                    QList<int> card_to_gotback;
                    for (int i = 0; i < 3; i++) {
                        const Card *card = Sanguosha->getCard(ids[i]);
                        if (card->getTypeId() == Card::TypeBasic || card->isKindOf("Weapon") || card->isKindOf("Duel"))
                            card_to_gotback << ids[i];
                        else
                            card_to_throw << ids[i];
                    }
                    if (!card_to_throw.isEmpty()) {
                        DummyCard *dummy = new DummyCard(card_to_throw);
                        CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "luoyi", QString());
                        room->throwCard(dummy, reason, NULL);
                        delete dummy;
                    }
                    if (!card_to_gotback.isEmpty()) {
                        DummyCard *dummy = new DummyCard(card_to_gotback);
                        room->obtainCard(player, dummy);
                        delete dummy;
                    }
                    room->setPlayerMark(player, "@luoyi", 1);
                    room->removePlayerMark(player, objectName()+"engine");
                }
            }
        }
        return false;
    }
};

class Luoshen: public TriggerSkill {
public:
    Luoshen(): TriggerSkill("luoshen") {
        events << EventPhaseStart << FinishJudge;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Start) {
            bool first = true;
            while (player->isAlive() && player->askForSkillInvoke("luoshen")) {
                if (first) {
                    room->broadcastSkillInvoke(objectName());
                    first = false;
                }
                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {
					JudgeStruct judge;
					judge.pattern = ".|black";
					judge.good = true;
					judge.reason = objectName();
					judge.who = player;
					judge.time_consuming = true;

					room->judge(judge);

					if (judge.isBad())
						break;
                    room->removePlayerMark(player, objectName()+"engine");
                }
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName()) {
                if (judge->card->isBlack()) {
                    if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge) {
                        if (Config.value("heg_skill", true).toBool()) {
                            CardMoveReason reason(CardMoveReason::S_REASON_JUDGEDONE, player->objectName(), QString(), judge->reason);
                            room->moveCardTo(judge->card, player, NULL, Player::PlaceTable, reason, true);
                            QVariantList luoshen_list = player->tag[objectName()].toList();
                            luoshen_list << judge->card->getEffectiveId();
                            player->tag[objectName()] = luoshen_list;
                        } else {
                            player->obtainCard(judge->card);
                        }
                    }
                } else {
                    if (Config.value("heg_skill", true).toBool()) {
                        DummyCard *dummy = new DummyCard(VariantList2IntList(player->tag[objectName()].toList()));
                        if (dummy->subcardsLength() > 0)
                            player->obtainCard(dummy);
                        player->tag.remove(objectName());
                        delete dummy;
                    }
                }
            }
        }

        return false;
    }
};

class Qingguo: public OneCardViewAsSkill {
public:
    Qingguo(): OneCardViewAsSkill("qingguo") {
        filter_pattern = ".|black|.|hand";
        response_pattern = "jink";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
        jink->setSkillName(objectName());
        jink->addSubcard(originalCard->getId());
        return jink;
    }
};

class RendeViewAsSkill: public ViewAsSkill {
public:
    RendeViewAsSkill(): ViewAsSkill("rende") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (ServerInfo.GameMode == "04_1v3" && selected.length() + Self->getMark("rende") >= 2)
           return false;
        else {
            if (to_select->isEquipped()) return false;
            if (Sanguosha->getCurrentCardUsePattern() == "@@rende") {
                QList<int> rende_list = StringList2IntList(Self->property("rende").toString().split("+"));
                return rende_list.contains(to_select->getEffectiveId());
            } else
                return true;
        }
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if (ServerInfo.GameMode == "04_1v3" && player->getMark("rende") >= 2)
           return false;
        return !player->hasUsed("RendeCard") && !player->isKongcheng();
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@@rende";
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
            return NULL;

        RendeCard *rende_card = new RendeCard;
        rende_card->addSubcards(cards);
        return rende_card;
    }
};

class Rende: public TriggerSkill {
public:
    Rende(): TriggerSkill("rende") {
        events << EventPhaseChanging;
        view_as_skill = new RendeViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getMark("rende") > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return false;
        room->setPlayerMark(player, "rende", 0);
        room->setPlayerProperty(player, "rende", QString());
        return false;
    }
};

JijiangViewAsSkill::JijiangViewAsSkill(): ZeroCardViewAsSkill("jijiang$") {
}

bool JijiangViewAsSkill::isEnabledAtPlay(const Player *player) const{
    return hasShuGenerals(player) && !player->hasFlag("Global_JijiangFailed") && Slash::IsAvailable(player);
}

bool JijiangViewAsSkill::isEnabledAtResponse(const Player *player, const QString &pattern) const{
    return hasShuGenerals(player)
           && (pattern == "slash" || pattern == "@jijiang")
           && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
           && !player->hasFlag("Global_JijiangFailed");
}

const Card *JijiangViewAsSkill::viewAs() const{
    return new JijiangCard;
}

bool JijiangViewAsSkill::hasShuGenerals(const Player *player) {
    foreach (const Player *p, player->getAliveSiblings())
        if (p->getKingdom() == "shu")
            return true;
    return false;
}

class Jijiang: public TriggerSkill {
public:
    Jijiang(): TriggerSkill("jijiang$") {
        events << CardAsked;
        view_as_skill = new JijiangViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasLordSkill("jijiang");
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QString pattern = data.toStringList().first();
        QString prompt = data.toStringList().at(1);
        if (pattern != "slash" || prompt.startsWith("@jijiang-slash"))
            return false;

        QList<ServerPlayer *> lieges = room->getLieges("shu", player);
        if (lieges.isEmpty())
            return false;

        if (!player->hasFlag("qinwangjijiang") && !room->askForSkillInvoke(player, objectName(), data))
            return false;
        if (!player->hasFlag("qinwangjijiang")) {
			if (!player->isLord() && player->hasSkill("weidi"))
				room->broadcastSkillInvoke("weidi");
			else {
				int r = 1 + qrand() % 2;
				if (!player->hasInnateSkill("jijiang") && (Config.value("heg_skill", true).toBool() || player->getMark("ruoyu") > 0))
					r += 2;
				room->broadcastSkillInvoke("jijiang", r);
			}
		} else {
			room->broadcastSkillInvoke("qinwang");
		}
        foreach (ServerPlayer *liege, lieges) {
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), liege->objectName());
		}
        room->addPlayerMark(player, objectName()+"engine");
        if (player->getMark(objectName()+"engine") > 0) {
            foreach (ServerPlayer *liege, lieges) {
                const Card *slash = room->askForCard(liege, "slash", "@jijiang-slash:" + player->objectName(),
                    QVariant(), Card::MethodResponse, player, false, QString(), true);
                if (slash && !slash->hasFlag("response_failed")){
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, liege->objectName(), player->objectName());
                    room->setCardFlag(slash, "-response_failed");
                    room->provide(slash);
                    return true;
                }
            }

            room->removePlayerMark(player, objectName()+"engine");
        }

        return false;
	}
};

   

class Wusheng: public OneCardViewAsSkill {
public:
    Wusheng(): OneCardViewAsSkill("wusheng") {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "slash";
    }

    virtual bool viewFilter(const Card *card) const{
        if (!card->isRed())
            return false;

        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            Slash *slash = new Slash(Card::SuitToBeDecided, -1);
            slash->addSubcard(card->getEffectiveId());
            slash->deleteLater();
            return slash->isAvailable(Self);
        }
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->addSubcard(originalCard->getId());
        slash->setSkillName(objectName());
        return slash;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const {
        int index = qrand() % 2 + 1;
        if ((!player->hasInnateSkill(objectName()) && player->hasSkill("fuhun")) || Player::isNostalGeneral(player, "guanyu")) 
            index += 2;
        
		else if (player->getGeneralName() == "jsp_guanyu" || (player->getGeneralName() != "guanyu" && player->getGeneral2Name() == "jsp_guanyu"))
            index += 4;
		else if (player->getGeneralName() == "guansuo")
            index = 7;
        return index;
    }
};

class YijueViewAsSkill: public ZeroCardViewAsSkill {
public:
    YijueViewAsSkill(): ZeroCardViewAsSkill("yijue") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("YijueCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const{
        return new YijueCard;
    }
};

class Yijue: public TriggerSkill {
public:
    Yijue(): TriggerSkill("yijue") {
        events << EventPhaseChanging << Death;
        view_as_skill = new YijueViewAsSkill;
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
            if (player->getMark("yijue") == 0) continue;
            player->removeMark("yijue");
            room->removePlayerMark(player, "@skill_invalidity");

            foreach (ServerPlayer *p, room->getAllPlayers())
                room->filterCards(p, p->getCards("he"), false);
            Json::Value args;
            args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

            room->removePlayerCardLimitation(player, "use,response", ".|.|.|hand$1");
        }
        return false;
    }
};

class NonCompulsoryInvalidity: public InvaliditySkill {
public:
    NonCompulsoryInvalidity(): InvaliditySkill("#non-compulsory-invalidity") {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const{
        return player->getMark("@skill_invalidity") == 0 || skill->getFrequency(player) == Skill::Compulsory || (!Config.value("Yoka_meaning", true).toBool() && skill->getFrequency(player) == Skill::Wake);
    }
};

class Paoxiao: public TargetModSkill {
public:
    Paoxiao(): TargetModSkill("paoxiao") {
		if (Config.value("heg_skill", true).toBool() || Config.value("paoxiao_down", true).toBool()) 
			frequency = Compulsory;
		else
			frequency = NotCompulsory;
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()))
            return 1000;
        else
            return 0;
    }
};

class Tishen: public TriggerSkill {
public:
    Tishen(): TriggerSkill("tishen") {
        events << EventPhaseChanging << EventPhaseStart;
        frequency = Limited;
        limit_mark = "@substitute";
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                room->setPlayerProperty(player, "tishen_hp", QString::number(player->getHp()));
                room->setPlayerMark(player, "@substitute", player->getMark("@substitute")); // For UI coupling
            }
        } else if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player)
            && player->getMark("@substitute") > 0 && player->getPhase() == Player::Start) {
			QString hp_str = player->property("tishen_hp").toString();
			if (hp_str.isEmpty()) return false;
			int hp = hp_str.toInt();
			int x = qMin(hp - player->getHp(), player->getMaxHp() - player->getHp());
			if (x > 0 && room->askForSkillInvoke(player, objectName(), QVariant::fromValue(x))) {
				room->broadcastSkillInvoke(objectName());
				//room->doLightbox("$TishenAnimate");
				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					room->removePlayerMark(player, "@substitute");
					room->recover(player, RecoverStruct(player, NULL, x));
					player->drawCards(x, objectName());
					room->removePlayerMark(player, objectName()+"engine");
				}
			}
        }
        return false;
    }
};

class Longdan: public OneCardViewAsSkill {
public:
    Longdan(): OneCardViewAsSkill("longdan") {
        response_or_use = true;
    }

    virtual bool viewFilter(const Card *to_select) const{
        const Card *card = to_select;

        switch (Sanguosha->getCurrentCardUseReason()) {
        case CardUseStruct::CARD_USE_REASON_PLAY: {
                return card->isKindOf("Jink");
            }
        case CardUseStruct::CARD_USE_REASON_RESPONSE:
        case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: {
                QString pattern = Sanguosha->getCurrentCardUsePattern();
                if (pattern == "slash")
                    return card->isKindOf("Jink");
                else if (pattern == "jink")
                    return card->isKindOf("Slash");
            }
        default:
            return false;
        }
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "jink" || pattern == "slash";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard->isKindOf("Slash")) {
            Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
            jink->addSubcard(originalCard);
            jink->setSkillName(objectName());
            return jink;
        } else if (originalCard->isKindOf("Jink")) {
            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->addSubcard(originalCard);
            slash->setSkillName(objectName());
            return slash;
        } else
            return NULL;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const{
        int index = qrand() % 2 + 1;
        if (Player::isNostalGeneral(player, "zhaoyun"))
            index += 2;
        return index;
    }
};

class Yajiao: public TriggerSkill {
public:
    Yajiao(): TriggerSkill("yajiao") {
        events << CardUsed << CardResponded;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() != Player::NotActive) return false;
        const Card *card_ptr = NULL;
        bool isHandcard = false;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            card_ptr = use.card;
            isHandcard = use.m_isHandcard;
        } else {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            card_ptr = resp.m_card;
            isHandcard = resp.m_isHandcard;
        }
        if (isHandcard && room->askForSkillInvoke(player, objectName(), data)) {
            room->broadcastSkillInvoke(objectName(), 1);
            QList<int> ids = room->getNCards(1, false);
            CardsMoveStruct move(ids, player, Player::PlaceTable,
                CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), "yajiao", QString()));
            room->moveCardsAtomic(move, true);

            int id = ids.first();
            const Card *card = Sanguosha->getCard(id);
            room->fillAG(ids, player);
            bool dealt = false;
            if (card->getTypeId() == card_ptr->getTypeId()) {
                player->setMark("yajiao", id); // For AI
                ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(),
                    QString("@yajiao-give:::%1:%2\\%3").arg(card->objectName())
                    .arg(card->getSuitString() + "_char")
                    .arg(card->getNumberString()),
                    true);
                if (target) {
                    room->clearAG(player);
                    dealt = true;
                    CardMoveReason reason(CardMoveReason::S_REASON_DRAW, target->objectName(), "yajiao", QString());
                    room->getThread()->delay(1500);
                    room->obtainCard(target, card, reason);
					if (target == player)
						room->broadcastSkillInvoke(objectName(), 2);
                }
            } else {
                QVariant carddata = QVariant::fromValue(card);
                if (room->askForChoice(player, objectName(), "throw+cancel", carddata) == "throw") {
                    room->clearAG(player);
                    dealt = true;
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "yajiao", QString());
                    room->throwCard(card, reason, NULL);
                }
            }
            if (!dealt) {
                room->clearAG(player);
                room->returnToTopDrawPile(ids);
            }
        }
        return false;
    }
};

class Tieji: public TriggerSkill {
public:
    Tieji(): TriggerSkill("tieji") {
        events << TargetSpecified << FinishJudge;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetSpecified && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return false;
            QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
            int index = 0;
            QList<ServerPlayer *> tos;
            foreach (ServerPlayer *p, use.to) {
                if (!player->isAlive()) break;
                if (player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                    room->broadcastSkillInvoke(objectName(), 1);
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());
                    room->addPlayerMark(player, objectName()+"engine");
                    if (player->getMark(objectName()+"engine") > 0) {
						if (!tos.contains(p)) {
							p->addMark("tieji");
							room->addPlayerMark(p, "@skill_invalidity");
							tos << p;

							foreach (ServerPlayer *pl, room->getAllPlayers())
								room->filterCards(pl, pl->getCards("he"), true);
							Json::Value args;
							args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
							room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
						}

						JudgeStruct judge;
						judge.pattern = ".";
						judge.good = true;
						judge.reason = objectName();
						judge.who = player;
						judge.play_animation = false;

						room->judge(judge);

						if (p->isAlive() && !p->canDiscard(p, "he") || !room->askForCard(p, ".|" + judge.pattern, "@tieji-discard:::" + judge.pattern, data, Card::MethodDiscard)) {
							room->getThread()->delay(2000);
							room->broadcastSkillInvoke(objectName(), 2);
                            LogMessage log;
                            log.type = "#NoJink";
                            log.from = p;
                            room->sendLog(log);
							jink_list.replace(index, QVariant(0));
						}
                        room->removePlayerMark(player, objectName()+"engine");
                    }
                }
                ++index;
            }
            player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
            return false;
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName()) {
                judge->pattern = judge->card->getSuitString();
            }
        }
        return false;
    }
};

class TiejiClear: public TriggerSkill {
public:
    TiejiClear(): TriggerSkill("#tieji-clear") {
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
            if (player->getMark("tieji") == 0) continue;
            room->removePlayerMark(player, "@skill_invalidity", player->getMark("tieji"));
            player->setMark("tieji", 0);

            foreach (ServerPlayer *p, room->getAllPlayers())
                room->filterCards(p, p->getCards("he"), false);
            Json::Value args;
            args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
        }
        return false;
    }
};

class Guanxing: public PhaseChangeSkill {
public:
    Guanxing(): PhaseChangeSkill("guanxing") {
        frequency = Frequent;
    }

    virtual int getPriority(TriggerEvent) const{
        return 1;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        if (player->getPhase() == Player::Start && room->askForSkillInvoke(player, objectName())) {
            QList<int> guanxing = room->getNCards(getGuanxingNum(room));
            int index = qrand() % 2 + 1;
            if (objectName() == "guanxing" && !player->hasInnateSkill(objectName()) && (Config.value("heg_skill", true).toBool() || player->getMark("zhiji") > 0)) {
				if (Config.value("heg_skill", true).toBool())
					guanxing = room->getNCards(5);
                index += 2;
			}
            room->broadcastSkillInvoke(objectName(), index);

            LogMessage log;
            log.type = "$ViewDrawPile";
            log.from = player;
            log.card_str = IntList2StringList(guanxing).join("+");
            room->sendLog(log, player);

            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
				room->askForGuanxing(player, guanxing);
                room->removePlayerMark(player, objectName()+"engine");
            }
        }

        return false;
    }

    virtual int getGuanxingNum(Room *room) const{
        return qMin(5, room->alivePlayerCount());
    }
};

class Kongcheng: public ProhibitSkill {
public:
    Kongcheng(): ProhibitSkill("kongcheng") {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const{
        return to->hasSkill(objectName()) && (card->isKindOf("Slash") || card->isKindOf("Duel")) && to->isKongcheng();
    }
};

class KongchengEffect: public TriggerSkill {
public:
    KongchengEffect() :TriggerSkill("#kongcheng-effect") {
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->isKongcheng()) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && move.from_places.contains(Player::PlaceHand)) {
                room->broadcastSkillInvoke("kongcheng");
                room->notifySkillInvoked(player, "kongcheng");
            }
        }

        return false;
    }
};

class Jizhi: public TriggerSkill {
public:
    Jizhi(): TriggerSkill("jizhi") {
        frequency = Frequent;
        events << CardUsed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();

        if (use.card->getTypeId() == Card::TypeTrick
            && ((player->getMark("JilveEvent") > 0 && !Config.value("simayi_duang", true).toBool()) || room->askForSkillInvoke(player, objectName()))) {
            if (player->getMark("JilveEvent") > 0 && !Config.value("simayi_duang", true).toBool())
                room->broadcastSkillInvoke("jilve", 5);
            else
                room->broadcastSkillInvoke(objectName());

            QList<int> ids = room->getNCards(1, false);
            CardsMoveStruct move(ids, player, Player::PlaceTable,
                                 CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), "jizhi", QString()));
            room->moveCardsAtomic(move, true);

            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
				int id = ids.first();
				const Card *card = Sanguosha->getCard(id);
				if (!card->isKindOf("BasicCard")) {
					CardMoveReason reason(CardMoveReason::S_REASON_DRAW, player->objectName(), "jizhi", QString());
					room->obtainCard(player, card, reason);
				} else {
					const Card *card_ex = NULL;
					if (!player->isKongcheng())
						card_ex = room->askForCard(player, ".", "@jizhi-exchange:::" + card->objectName(),
												   QVariant::fromValue(card), Card::MethodNone);
					if (card_ex) {
						CardMoveReason reason1(CardMoveReason::S_REASON_PUT, player->objectName(), "jizhi", QString());
						CardMoveReason reason2(CardMoveReason::S_REASON_DRAW, player->objectName(), "jizhi", QString());
						CardsMoveStruct move1(card_ex->getEffectiveId(), player, NULL, Player::PlaceUnknown, Player::DrawPile, reason1);
						CardsMoveStruct move2(ids, player, player, Player::PlaceUnknown, Player::PlaceHand, reason2);

						QList<CardsMoveStruct> moves;
						moves.append(move1);
						moves.append(move2);
						room->moveCardsAtomic(moves, false);
					} else {
						CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "jizhi", QString());
						room->throwCard(card, reason, NULL);
					}
				}
                room->removePlayerMark(player, objectName()+"engine");
            }
        }

        return false;
    }
};

class Qicai: public TargetModSkill {
public:
    Qicai(): TargetModSkill("qicai") {
        pattern = "TrickCard";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()))
            return 1000;
        else
            return 0;
    }
};

class Zhuhai: public TriggerSkill {
public:
    Zhuhai(): TriggerSkill("zhuhai") {
        events << EventPhaseStart << ChoiceMade;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish) {
            ServerPlayer *xushu = room->findPlayerBySkillName(objectName());
            if (xushu && xushu != player && xushu->canSlash(player, false) && player->getMark("damage_point_round") > 0) {
                xushu->setFlags("ZhuhaiSlash");
                QString prompt = QString("@zhuhai-slash:%1:%2").arg(xushu->objectName()).arg(player->objectName());
                if (!room->askForUseSlashTo(xushu, player, prompt, false))
                    xushu->setFlags("-ZhuhaiSlash");
            }
        } else if (triggerEvent == ChoiceMade && player->hasFlag("ZhuhaiSlash") && data.canConvert<CardUseStruct>()) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());

            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            room->addPlayerMark(player, objectName()+"engine");
            player->setFlags("-ZhuhaiSlash");
            if (player->getMark(objectName()+"engine") > 0) {
                room->removePlayerMark(player, objectName()+"engine");
                return false;
            }
        }
        return false;
    }
};



class Qianxin: public TriggerSkill {
public:
    Qianxin(): TriggerSkill("qianxin") {
        events << Damage;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && TriggerSkill::triggerable(target)
            && target->getMark("qianxin") == 0
            && target->isWounded();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());
        //room->doLightbox("$QianxinAnimate");

        LogMessage log;
        log.type = "#QianxinWake";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);

        room->addPlayerMark(player, objectName()+"engine");
        if (player->getMark(objectName()+"engine") > 0) {
            room->setPlayerMark(player, "qianxin", 1);
            if (room->changeMaxHpForAwakenSkill(player) && player->getMark("qianxin") == 1)
                room->acquireSkill(player, "jianyan");

            room->removePlayerMark(player, objectName()+"engine");
        }
        return false;
    }
};

class Jianyan: public ZeroCardViewAsSkill {
public:
    Jianyan(): ZeroCardViewAsSkill("jianyan") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("JianyanCard");
    }

    virtual const Card *viewAs() const{
        return new JianyanCard;
    }
};

class Zhiheng: public ViewAsSkill {
public:
    Zhiheng(): ViewAsSkill("zhiheng") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (ServerInfo.GameMode == "02_1v1" && ServerInfo.GameRuleMode != "Classical" && selected.length() >= 2) return false;
        if ((Config.value("heg_skill", true).toBool()) && selected.length() >= Self->getMaxHp()) return false;
        return !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
            return NULL;

        ZhihengCard *zhiheng_card = new ZhihengCard;
        zhiheng_card->addSubcards(cards);
        zhiheng_card->setSkillName(objectName());
        return zhiheng_card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("ZhihengCard");
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@zhiheng";
    }
};

class Jiuyuan: public TriggerSkill {
public:
    Jiuyuan(): TriggerSkill("jiuyuan$") {
        events << TargetConfirmed << PreHpRecover;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasLordSkill("jiuyuan");
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Peach") && use.from && use.from->getKingdom() == "wu"
                && player != use.from && player->hasFlag("Global_Dying")) {
                room->setCardFlag(use.card, "jiuyuan");
            }
        } else if (triggerEvent == PreHpRecover) {
            RecoverStruct rec = data.value<RecoverStruct>();
            if (rec.card && rec.card->hasFlag("jiuyuan")) {
                room->notifySkillInvoked(player, "jiuyuan");
                room->broadcastSkillInvoke("jiuyuan", rec.who->isMale() ? 1 : 2);

                LogMessage log;
                log.type = "#JiuyuanExtraRecover";
                log.from = player;
                log.to << rec.who;
                log.arg = objectName();
                room->sendLog(log);

                ++rec.recover;
                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {
                    data = QVariant::fromValue(rec);
                    room->removePlayerMark(player, objectName()+"engine");
                }
            }
        }

        return false;
    }
};

class Yingzi: public DrawCardsSkill {
public:
    Yingzi(): DrawCardsSkill("yingzi") {
        frequency = Compulsory;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();

        int index = qrand() % 2 + 1;
        if (!player->hasInnateSkill(objectName())) {
            if (!Config.value("music", true).toBool() && !player->hasInnateSkill(objectName()) && player->hasSkill("hunzi") && player->getMark("hunzi") > 0)
                index += 4;
            else if (!player->hasInnateSkill(objectName()) && player->hasSkill("mouduan") && player->getMark("@wen") > 0)
                index += 2;
            else if (!player->hasInnateSkill(objectName()) && player->hasSkill("jiazhou"))
                index = 7;
        }
        room->broadcastSkillInvoke(objectName(), index);
        room->sendCompulsoryTriggerLog(player, objectName());
        room->addPlayerMark(player, objectName()+"engine");
        if (player->getMark(objectName()+"engine") > 0) {
            room->removePlayerMark(player, objectName()+"engine");
            return n + 1;
        }

        return n;
    }
};

class YingziMaxCards: public MaxCardsSkill {
public:
    YingziMaxCards(): MaxCardsSkill("#yingzi") {
    }

    virtual int getFixed(const Player *target) const{
        if (target->hasSkill("yingzi"))
            return target->getMaxHp();
        else
            return -1;
    }
};

class Fanjian: public OneCardViewAsSkill {
public:
    Fanjian(): OneCardViewAsSkill("fanjian") {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && !player->hasUsed("FanjianCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        FanjianCard *card = new FanjianCard;
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

class Keji: public TriggerSkill {
public:
    Keji(): TriggerSkill("keji") {
        events << PreCardUsed << CardResponded << EventPhaseChanging;
        frequency = Frequent;
        global = true;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            bool can_trigger = true;
            if (player->hasFlag("KejiSlashInPlayPhase")) {
                can_trigger = false;
                player->setFlags("-KejiSlashInPlayPhase");
            }
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard && player->isAlive() && player->hasSkill(objectName()) ) {
                if (can_trigger && player->askForSkillInvoke(objectName())) {
                    if (player->getHandcardNum() > player->getMaxCards()) {
                        int index = qrand() % 2 + 1;
                        if (!player->hasInnateSkill(objectName()) && player->hasSkill("mouduan"))
                            index += 4;
                        else if (Player::isNostalGeneral(player, "lvmeng"))
                            index += 2;
                        room->broadcastSkillInvoke(objectName(), index);
                    }
                    room->addPlayerMark(player, objectName()+"engine");
                    if (player->getMark(objectName()+"engine") > 0) {
                        player->skip(Player::Discard);
                        room->removePlayerMark(player, objectName()+"engine");
                    }
                }
            }
        } else if (player->getPhase() == Player::Play) {
            const Card *card = NULL;
            if (triggerEvent == PreCardUsed)
                card = data.value<CardUseStruct>().card;
            else
                card = data.value<CardResponseStruct>().m_card;
            if (card->isKindOf("Slash"))
                player->setFlags("KejiSlashInPlayPhase");
        }

        return false;
    }
};

class Qinxue: public PhaseChangeSkill {
public:
    Qinxue(): PhaseChangeSkill("qinxue") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Start
            && target->getMark("qinxue") == 0;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        int n = player->getHandcardNum() - player->getHp();
        int wake_lim = (Sanguosha->getPlayerCount(room->getMode()) >= 7) ? 2 : 3;
        if (n < wake_lim) return false;

        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());
        //room->doLightbox("$QinxueAnimate");

        LogMessage log;
        log.type = "#QinxueWake";
        log.from = player;
        log.arg = QString::number(n);
        log.arg2 = "qinxue";
        room->sendLog(log);

        room->addPlayerMark(player, objectName()+"engine");
        if (player->getMark(objectName()+"engine") > 0) {
            room->setPlayerMark(player, "qinxue", 1);
            if (room->changeMaxHpForAwakenSkill(player) && player->getMark("qinxue") == 1)
                room->acquireSkill(player, "gongxin");

            room->removePlayerMark(player, objectName()+"engine");
        }

        return false;
    }
};

class Qixi: public OneCardViewAsSkill {
public:
    Qixi(): OneCardViewAsSkill("qixi") {
        filter_pattern = ".|black";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Dismantlement *dismantlement = new Dismantlement(originalCard->getSuit(), originalCard->getNumber());
        dismantlement->addSubcard(originalCard->getId());
        dismantlement->setSkillName(objectName());
        return dismantlement;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const{
        int index = qrand() % 2 + 1;
        if (Player::isNostalGeneral(player, "ganning"))
            index += 2;
        return index;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@jiewei";
    }
};

class FenweiViewAsSkill: public ZeroCardViewAsSkill {
public:
    FenweiViewAsSkill():ZeroCardViewAsSkill("fenwei") {
        response_pattern = "@@fenwei";
    }

    virtual const Card *viewAs() const{
        return new FenweiCard;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const{
        int index = qrand() % 2 + 1;
        if (!player->hasInnateSkill(objectName()) && player->hasSkill("jiazhou"))
            index = 3;
        return index;
    }
};

class Fenwei: public TriggerSkill {
public:
    Fenwei(): TriggerSkill("fenwei") {
        events << TargetSpecifying;
        view_as_skill = new FenweiViewAsSkill;
        frequency = Limited;
        limit_mark = "@fenwei";
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *ganning = room->findPlayerBySkillName(objectName());
        if (!ganning || ganning->getMark("@fenwei") == 0) return false;

        CardUseStruct use = data.value<CardUseStruct>();
        if (use.to.length() <= 1 || !use.card->isNDTrick())
            return false;

        if (use.from == NULL)
            return false;

        QStringList target_list;
        foreach (ServerPlayer *p, use.to)
            target_list << p->objectName();
        room->setPlayerProperty(ganning, "fenwei_targets", target_list.join("+"));
        ganning->tag["fenwei"] = data;
        room->askForUseCard(ganning, "@@fenwei", "@fenwei-card");
        room->addPlayerMark(ganning, objectName()+"engine");
        if (ganning->getMark(objectName()+"engine") > 0) {
            data = ganning->tag["fenwei"];
            room->removePlayerMark(ganning, objectName()+"engine");
        }

        return false;
    }
};

class Kurou: public OneCardViewAsSkill {
public:
    Kurou(): OneCardViewAsSkill("kurou") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("KurouCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        KurouCard *card = new KurouCard;
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

class Zhaxiang: public TriggerSkill {
public:
    Zhaxiang(): TriggerSkill("zhaxiang") {
        events << HpLost << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual int getPriority(TriggerEvent triggerEvent) const{
        if (triggerEvent == EventPhaseChanging) {
            return 8;
        }
        return TriggerSkill::getPriority(triggerEvent);
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == HpLost && TriggerSkill::triggerable(player)) {
            int lose = data.toInt();

            room->sendCompulsoryTriggerLog(player, objectName());
			room->getThread()->delay();
            room->broadcastSkillInvoke(objectName());

            for (int i = 0; i < lose; i++) {
                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {
                    player->drawCards(3, objectName());
                    if (player->getPhase() == Player::Play)
                        room->addPlayerMark(player, objectName());
                    
                    room->removePlayerMark(player, objectName()+"engine");
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive || change.to == Player::RoundStart) {
                room->setPlayerMark(player, objectName(), 0);
            }
        }
        return false;
    }
};

class ZhaxiangRedSlash: public TriggerSkill {
public:
    ZhaxiangRedSlash(): TriggerSkill("#zhaxiang") {
        events << TargetSpecified;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target && target->isAlive() && target->getMark("zhaxiang") > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash") || !use.card->isRed())
            return false;
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *p, use.to) {
            LogMessage log;
            log.type = "#NoJink";
            log.from = p;
            room->sendLog(log);
            jink_list.replace(index, QVariant(0));
            ++index;
        }
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

class ZhaxiangTargetMod: public TargetModSkill {
public:
    ZhaxiangTargetMod(): TargetModSkill("#zhaxiang-target") {
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        return from->getMark("zhaxiang");
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const{
        if (card->isRed() && from->getMark("zhaxiang") > 0)
            return 1000;
        else
            return 0;
    }
};

class GuoseViewAsSkill: public OneCardViewAsSkill {
public:
    GuoseViewAsSkill(): OneCardViewAsSkill("guose") {
        filter_pattern = ".|diamond";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
         return !player->hasUsed("GuoseCard") && !(player->isNude() && player->getHandPile().isEmpty());
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        GuoseCard *card = new GuoseCard;
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

class Guose: public TriggerSkill {
public:
    Guose(): TriggerSkill("guose") {
        events << CardFinished;
        view_as_skill = new GuoseViewAsSkill;
    }

    virtual int getPriority(TriggerEvent) const{
        return 1;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Indulgence") && use.card->getSkillName() == objectName())
            player->drawCards(1, objectName());
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        return (card->isKindOf("Indulgence") ? 1 : 2);
    }
};

class LiuliViewAsSkill: public OneCardViewAsSkill {
public:
    LiuliViewAsSkill(): OneCardViewAsSkill("liuli") {
        filter_pattern = ".!";
        response_pattern = "@@liuli";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        LiuliCard *liuli_card = new LiuliCard;
        liuli_card->addSubcard(originalCard);
        return liuli_card;
    }
};

class Liuli: public TriggerSkill {
public:
    Liuli(): TriggerSkill("liuli") {
        events << TargetConfirming;
        view_as_skill = new LiuliViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *daqiao, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();

        if (use.card->isKindOf("Slash") && use.to.contains(daqiao) && daqiao->canDiscard(daqiao, "he")) {
            QList<ServerPlayer *> players = room->getOtherPlayers(daqiao);
            players.removeOne(use.from);

            bool can_invoke = false;
            foreach (ServerPlayer *p, players) {
                if (use.from->canSlash(p, use.card, false) && daqiao->inMyAttackRange(p)) {
                    can_invoke = true;
                    break;
                }
            }

            if (can_invoke) {
                QString prompt = "@liuli:" + use.from->objectName();
                room->setPlayerFlag(use.from, "LiuliSlashSource");
                // a temp nasty trick
                daqiao->tag["liuli-card"] = QVariant::fromValue(use.card); // for the server (AI)
                room->setPlayerProperty(daqiao, "liuli", use.card->toString()); // for the client (UI)
                if (room->askForUseCard(daqiao, "@@liuli", prompt, -1, Card::MethodDiscard)) {
                    daqiao->tag.remove("liuli-card");
                    room->setPlayerProperty(daqiao, "liuli", QString());
                    room->setPlayerFlag(use.from, "-LiuliSlashSource");
                    foreach (ServerPlayer *p, players) {
                        if (p->hasFlag("LiuliTarget")) {
                            p->setFlags("-LiuliTarget");
                            if (!use.from->canSlash(p, false))
                                return false;
                            use.to.removeOne(daqiao);
                            use.to.append(p);
                            room->sortByActionOrder(use.to);
                            data = QVariant::fromValue(use);
                            room->getThread()->trigger(TargetConfirming, room, p, data);
                            return false;
                        }
                    }
                } else {
                    daqiao->tag.remove("liuli-card");
                    room->setPlayerProperty(daqiao, "liuli", QString());
                    room->setPlayerFlag(use.from, "-LiuliSlashSource");
                }
            }
        }

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const {
        int index = qrand() % 2 + 1;
        if (Player::isNostalGeneral(player, "daqiao")) {
            index += 2;
        }
        else if (!player->hasInnateSkill(objectName()) && player->hasSkill("xingwu")) {
            index += 4;
        }
        return index;
    }
};

class Qianxun: public TriggerSkill {
public:
    Qianxun(): TriggerSkill("qianxun") {
        events << TrickEffect << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TrickEffect && TriggerSkill::triggerable(player)) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (!effect.multiple && effect.card->getTypeId() == Card::TypeTrick
                && (effect.card->isKindOf("DelayedTrick") || effect.from != player)
                && room->askForSkillInvoke(player, objectName(), data)) {
                    room->broadcastSkillInvoke(objectName(), 2);
                    player->tag["QianxunEffectData"] = data;

                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {
                    CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, player->objectName(), objectName(), QString());
                    QList<int> handcards = player->handCards();
                    QList<ServerPlayer *> open;
                    open << player;
                    player->addToPile("integrity", handcards, false, open, reason);
                    room->removePlayerMark(player, objectName()+"engine");
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getPile("integrity").length() > 0) {
						room->broadcastSkillInvoke(objectName(), 1);
                        DummyCard *dummy = new DummyCard(p->getPile("integrity"));
                        CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, p->objectName(), "integrity", QString());
                        room->obtainCard(p, dummy, reason, false);
                        delete dummy;
                    }
                }
            }
        }
        return false;
    }
};

class LianyingViewAsSkill: public ZeroCardViewAsSkill {
public:
    LianyingViewAsSkill(): ZeroCardViewAsSkill("lianying") {
        response_pattern = "@@lianying";
    }

    virtual const Card *viewAs() const{
        return new LianyingCard;
    }
};

class Lianying: public TriggerSkill {
public:
    Lianying(): TriggerSkill("lianying") {
        events << CardsMoveOneTime;
        view_as_skill = new LianyingViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *luxun, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == luxun && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard) {
            luxun->tag["LianyingMoveData"] = data;
            int count = 0;
            for (int i = 0; i < move.from_places.length(); ++i) {
                if (move.from_places[i] == Player::PlaceHand) ++count;
            }
            room->setPlayerMark(luxun, "lianying", count);
            room->askForUseCard(luxun, "@@lianying", "@lianying-card:::" + QString::number(count));
        }
        return false;
    }
};

class Jieyin: public ViewAsSkill {
public:
    Jieyin(): ViewAsSkill("jieyin") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getHandcardNum() >= 2 && !player->hasUsed("JieyinCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (selected.length() > 1 || Self->isJilei(to_select))
            return false;

        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() != 2)
            return NULL;

        JieyinCard *jieyin_card = new JieyinCard();
        jieyin_card->addSubcards(cards);
        return jieyin_card;
    }
};

class Xiaoji: public TriggerSkill {
public:
    Xiaoji(): TriggerSkill("xiaoji") {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && move.from_places.contains(Player::PlaceEquip)) {
            for (int i = 0; i < move.card_ids.size(); ++i) {
                if (!player->isAlive())
                    return false;
                if (move.from_places[i] == Player::PlaceEquip) {
                    if (room->askForSkillInvoke(player, objectName())) {
                         int index = qrand() % 2 + 1;
                        if (!player->hasInnateSkill(objectName()) && player->getMark("fanxiang") > 0)
                            index += 2;
                        room->broadcastSkillInvoke(objectName(), index);
                        room->addPlayerMark(player, objectName()+"engine");
						if (player->getMark(objectName()+"engine") > 0) {
							player->drawCards(2, objectName());
							room->removePlayerMark(player, objectName()+"engine");
						}
						if (Config.value("heg_skill", true).toBool())
							break;
                    } else {
                        break;
                    }
                }
            }
        }
        return false;
    }
};

class Wushuang: public TriggerSkill {
public:
    Wushuang(): TriggerSkill("wushuang") {
        events << TargetSpecified << CardFinished;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetSpecified) {
            int index = qrand() % 2 + 1;
            if (Player::isNostalGeneral(player, "lvbu")) index += 2;
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && TriggerSkill::triggerable(player)) {
                room->broadcastSkillInvoke(objectName(), index);
                room->sendCompulsoryTriggerLog(player, objectName());

                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {
                    QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
                    for (int i = 0; i < use.to.length(); i++) {
						room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), use.to.at(i)->objectName());
                        if (jink_list.at(i).toInt() == 1)
                            jink_list.replace(i, QVariant(2));
                    }
                    player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
                    room->removePlayerMark(player, objectName()+"engine");
                }
            } else if (use.card->isKindOf("Duel")) {
                if (TriggerSkill::triggerable(player)) {
                    room->broadcastSkillInvoke(objectName(), index);
                    room->sendCompulsoryTriggerLog(player, objectName());

                    room->addPlayerMark(player, objectName()+"engine");
                    if (player->getMark(objectName()+"engine") > 0) {
                        QStringList wushuang_tag;
                        foreach(ServerPlayer *to, use.to) {
							room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), to->objectName());
                            wushuang_tag << to->objectName();
						}
                        player->tag["Wushuang_" + use.card->toString()] = wushuang_tag;
                        room->removePlayerMark(player, objectName()+"engine");
                    }
                }
                foreach (ServerPlayer *p, use.to.toSet()) {
                    if (TriggerSkill::triggerable(p) && use.from != NULL) {
                        room->broadcastSkillInvoke(objectName(), index);
                        room->sendCompulsoryTriggerLog(p, objectName());
						room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), use.from->objectName());

                        room->addPlayerMark(player, objectName()+"engine");
                        if (player->getMark(objectName()+"engine") > 0) {
                            p->tag["Wushuang_" + use.card->toString()] = QStringList(player->objectName());
                            room->removePlayerMark(player, objectName()+"engine");
                        }
                    }
                }
            }
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Duel")) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    p->tag.remove("Wushuang_" + use.card->toString());
            }
        }

        return false;
    }
};

class Liyu: public TriggerSkill {
public:
    Liyu(): TriggerSkill("liyu") {
        events << Damage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->isAlive() && player != damage.to && !damage.to->hasFlag("Global_DebutFlag") && !damage.to->isNude()
            && damage.card && damage.card->isKindOf("Slash") && player->getMark("bf_huashenlvbu") == 0) {
                Duel *duel = new Duel(Card::NoSuit, 0);
                duel->setSkillName("_liyu");

                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p != damage.to && !player->isProhibited(p, duel))
                        targets << p;
                }
                if (targets.isEmpty()) {
                    delete duel;
                } else {
                    ServerPlayer *target = room->askForPlayerChosen(damage.to, targets, objectName(), "@liyu:" + player->objectName(), true, true);
                    if (target) {
                        room->broadcastSkillInvoke(objectName());
                        room->notifySkillInvoked(player, objectName());

                        LogMessage log;
                        log.type = "#InvokeOthersSkill";
                        log.from = damage.to;
                        log.to << player;
                        log.arg = objectName();
                        room->sendLog(log);

                        int id = room->askForCardChosen(player, damage.to, "he", objectName());
                        room->obtainCard(player, id);
						room->addPlayerMark(damage.to, objectName()+"engine");
						if (damage.to->getMark(objectName()+"engine") > 0) {
							if (player->isAlive() && target->isAlive() && !player->isLocked(duel))
								room->useCard(CardUseStruct(duel, player, target));
							else
								delete duel;
							
							room->removePlayerMark(damage.to, objectName()+"engine");
						}
                    }
                }
        }
        return false;
    }
};

class Lijian: public OneCardViewAsSkill {
public:
    Lijian(): OneCardViewAsSkill("lijian") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getAliveSiblings().length() > 1
               && player->canDiscard(player, "he") && !player->hasUsed("LijianCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        LijianCard *lijian_card = new LijianCard;
        lijian_card->addSubcard(originalCard->getId());
        return lijian_card;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        return card->isKindOf("Duel") ? 0 : -1;
    }
};

class Biyue: public PhaseChangeSkill {
public:
    Biyue(): PhaseChangeSkill("biyue") {
        frequency = Frequent;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        if (player->getPhase() == Player::Finish) {
            Room *room = player->getRoom();
            if (room->askForSkillInvoke(player, objectName())) {
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

class Chuli: public OneCardViewAsSkill {
public:
    Chuli(): OneCardViewAsSkill("chuli") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("ChuliCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ChuliCard *chuli_card = new ChuliCard;
        chuli_card->addSubcard(originalCard->getId());
        return chuli_card;
    }
};

class Jijiu: public OneCardViewAsSkill {
public:
    Jijiu(): OneCardViewAsSkill("jijiu") {
        filter_pattern = ".|red";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern.contains("peach") && !player->hasFlag("Global_PreventPeach")
                && player->getPhase() == Player::NotActive && player->canDiscard(player, "he");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Peach *peach = new Peach(originalCard->getSuit(), originalCard->getNumber());
        peach->addSubcard(originalCard->getId());
        peach->setSkillName(objectName());
        return peach;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const{
        int index = qrand() % 2 + 1;
        if (Player::isNostalGeneral(player, "huatuo"))
            index += 2;
        return index;
    }
};

class Mashu: public DistanceSkill {
public:
    Mashu(): DistanceSkill("mashu") {
    }

    virtual int getCorrect(const Player *from, const Player *) const{
        if (from->hasSkill(objectName()) || from->hasSkill("xiongshou"))
            return -1;
        else
            return 0;
    }
};

class Xunxun: public PhaseChangeSkill {
public:
    Xunxun(): PhaseChangeSkill("xunxun") {
        frequency = Frequent;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        if (player->getPhase() == Player::Draw) {
            Room *room = player->getRoom();
            if (room->askForSkillInvoke(player, objectName())) {
                room->broadcastSkillInvoke(objectName());

                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {
                    QList<int> card_ids = room->getNCards(4);
                    QList<ServerPlayer *> p_list;
                    p_list << player;
					QList<int> ids;
					room->fillAG(card_ids, player);
					for (int i = 1; i <= 2; ++i) {
						int id = room->askForAG(player, card_ids, false, objectName());
						card_ids.removeOne(id);
						ids << id;
						room->takeAG(player, id, false, p_list);
					}
					room->clearAG(player);
					if (Config.value("lidian_down", true).toBool()) {
						room->returnToTopDrawPile(ids);
					} else {
						DummyCard *dummy = new DummyCard(ids);
						player->obtainCard(dummy, false);
						delete dummy;
					}
                    room->askForGuanxing(player, card_ids, Room::GuanxingDownOnly);
                    room->removePlayerMark(player, objectName()+"engine");
                }
                return !Config.value("lidian_down", true).toBool();
            }
        }

        return false;
    }
};

class Wangxi: public TriggerSkill {
public:
    Wangxi(): TriggerSkill("wangxi") {
        events << Damage << Damaged;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = NULL;
        if (triggerEvent == Damage && !damage.to->hasFlag("Global_DebutFlag"))
            target = damage.to;
        else if (triggerEvent == Damaged)
            target = damage.from;
        if (!target || target == player) return false;
        QList<ServerPlayer *> players;
        players << player << target;
        room->sortByActionOrder(players);

        for (int i = 1; i <= damage.damage; ++i) {
            if (!target->isAlive() || !player->isAlive())
                return false;
            if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(target))) {
                room->broadcastSkillInvoke(objectName(), (triggerEvent == Damaged) ? 1 : 2);
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
                room->drawCards(players, 1, objectName());
            } else {
                break;
            }
        }
        return false;
    }
};

class Wangzun: public PhaseChangeSkill {
public:
    Wangzun(): PhaseChangeSkill("wangzun") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        if (!isNormalGameMode(room->getMode()))
            return false;
        if (target->isLord() && target->getPhase() == Player::Start) {
            ServerPlayer *yuanshu = room->findPlayerBySkillName(objectName());
            if (yuanshu && room->askForSkillInvoke(yuanshu, objectName())) {
                room->broadcastSkillInvoke(objectName());
                room->addPlayerMark(yuanshu, objectName()+"engine");
                if (yuanshu->getMark(objectName()+"engine") > 0) {
                    yuanshu->drawCards(1, objectName());
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, yuanshu->objectName(), target->objectName());
                    room->setPlayerFlag(target, "WangzunDecMaxCards");
                    room->removePlayerMark(yuanshu, objectName()+"engine");
                }
            }
        }
        return false;
    }
};

class WangzunMaxCards: public MaxCardsSkill {
public:
    WangzunMaxCards(): MaxCardsSkill("#wangzun-maxcard") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasFlag("WangzunDecMaxCards"))
            return -1;
        else
            return 0;
    }
};

class Tongji: public ProhibitSkill {
public:
    Tongji(): ProhibitSkill("tongji") {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const{
        if (card->isKindOf("Slash")) {
            // get rangefix
            int rangefix = 0;
            if (card->isVirtualCard()) {
                QList<int> subcards = card->getSubcards();
                if (from->getWeapon() && subcards.contains(from->getWeapon()->getId())) {
                    const Weapon *weapon = qobject_cast<const Weapon *>(from->getWeapon()->getRealCard());
                    rangefix += weapon->getRange() - from->getAttackRange(false);
                }

                if (from->getOffensiveHorse() && subcards.contains(from->getOffensiveHorse()->getId()))
                    rangefix += 1;
            }
            // find yuanshu
            foreach (const Player *p, from->getAliveSiblings()) {
                if (p->hasSkill(objectName()) && p != to && p->getHandcardNum() > p->getHp()
                    && from->inMyAttackRange(p, rangefix)) {
                    return true;
                }
            }
        }
        return false;
    }
};

class Yaowu: public TriggerSkill {
public:
    Yaowu(): TriggerSkill("yaowu") {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash") && damage.card->isRed()
            && damage.from && damage.from->isAlive()) {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(damage.to, objectName());
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.from->objectName());

            room->addPlayerMark(damage.from, objectName()+"engine");
            if (damage.from->getMark(objectName()+"engine") > 0) {
                if (damage.from->isWounded() && room->askForChoice(damage.from, objectName(), "recover+draw", data) == "recover")
                    room->recover(damage.from, RecoverStruct(damage.to));
                else
                    damage.from->drawCards(1, objectName());
                room->removePlayerMark(damage.from, objectName()+"engine");
            }
        }
        return false;
    }
};

class Qiaomeng: public TriggerSkill {
public:
    Qiaomeng(): TriggerSkill("qiaomeng") {
        events << Damage << CardsMoveOneTime;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == Damage && TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to->isAlive() && !damage.to->hasFlag("Global_DebutFlag")
                && damage.card && damage.card->isKindOf("Slash") && damage.card->isBlack()
                && player->canDiscard(damage.to, "e") && room->askForSkillInvoke(player, objectName(), data)) {
                room->broadcastSkillInvoke(objectName(), 1);
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());
                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {
                    int id = room->askForCardChosen(player, damage.to, "e", objectName(), false, Card::MethodDiscard);
                    CardMoveReason reason(CardMoveReason::S_REASON_DISMANTLE, player->objectName(), damage.to->objectName(),
                        objectName(), QString());
                    room->throwCard(Sanguosha->getCard(id), reason, damage.to, player);
                    room->removePlayerMark(player, objectName()+"engine");
                }
            }
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.reason.m_skillName == objectName() && move.reason.m_playerId == player->objectName()
                && move.card_ids.length() > 0) {
                const Card *card = Sanguosha->getCard(move.card_ids.first());
                if (card->isKindOf("Horse")) {
                    move.card_ids.clear();
                    data = QVariant::fromValue(move);
					room->broadcastSkillInvoke(objectName(), 2);
                    room->obtainCard(player, card);
                }
            }
        }
        return false;
    }
};

class Xiaoxi: public TriggerSkill {
public:
    Xiaoxi(): TriggerSkill("xiaoxi") {
        events << Debut;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        ServerPlayer *opponent = player->getNext();
        if (!opponent->isAlive())
            return false;
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_xiaoxi");
        if (player->isLocked(slash) || !player->canSlash(opponent, slash, false)) {
            delete slash;
            return false;
        } 
        if (room->askForSkillInvoke(player, objectName()))
            room->useCard(CardUseStruct(slash, player, opponent));
        return false;
    }
};

class DuoChuPai: public TargetModSkill {
public:
    DuoChuPai(): TargetModSkill("duochupai") {
		frequency = Compulsory;
        pattern = "Slash,Analeptic";
    }

    virtual int getResidueNum(const Player *from, const Card *card) const{
        if (Config.value("yummy_food", true).toBool())
            return card->isKindOf("Slash") ? Config.value("yummy_food_slash_spinbox", 1).toInt() : Config.value("yummy_food_analeptic_spinbox", 1).toInt();
        else
            return 0;
    }
};

class FACEMAX: public MaxCardsSkill {
public:
    FACEMAX(): MaxCardsSkill("facemax") {
    }

    virtual int getExtra(const Player *target) const{
        const ServerPlayer *p = qobject_cast<const ServerPlayer *>(target);
        Room *room = p->getRoom();
		int n = 0;
		if (target->isLord()) {
			foreach (const Player *player, target->getAliveSiblings()) {
				if (room->getTag(player->objectName()).toString() == "Zhaoyun_Buff")
					n += 1;
				else if (room->getTag(player->objectName()).toString() == "Zhangyi_Buff")
					n = n - 1;
			}
		}
		return n;
    }
};
void StandardPackage::addGenerals() {
    // Wei
    General *caocao = new General(this, "caocao$", "wei"); // WEI 001
    caocao->addSkill(new Jianxiong);
    caocao->addSkill(new Hujia);

    General *simayi = new General(this, "simayi", "wei", 3); // WEI 002
    simayi->addSkill(new Fankui);
    simayi->addSkill(new Guicai);

    General *xiahoudun = new General(this, "xiahoudun", "wei"); // WEI 003
    xiahoudun->addSkill(new Ganglie);
    xiahoudun->addSkill(new Qingjian);

    General *zhangliao = new General(this, "zhangliao", "wei"); // WEI 004
    zhangliao->addSkill(new Tuxi);
    zhangliao->addSkill(new TuxiAct);
    related_skills.insertMulti("tuxi", "#tuxi");

    General *xuchu = new General(this, "xuchu", "wei"); // WEI 005
    xuchu->addSkill(new Luoyi);
    xuchu->addSkill(new LuoyiBuff);
    related_skills.insertMulti("luoyi", "#luoyi");

    General *guojia = new General(this, "guojia", "wei", 3); // WEI 006
    guojia->addSkill(new Tiandu);
    guojia->addSkill(new Yiji);
    guojia->addSkill(new YijiObtain);
    related_skills.insertMulti("yiji", "#yiji");

    General *zhenji = new General(this, "zhenji", "wei", 3, false); // WEI 007
    zhenji->addSkill(new Qingguo);
    zhenji->addSkill(new Luoshen);

    General *lidian = new General(this, "lidian", "wei", 3); // WEI 017
    lidian->addSkill(new Xunxun);
    lidian->addSkill(new Wangxi);

    // Shu
    General *liubei = new General(this, "liubei$", "shu"); // SHU 001
    liubei->addSkill(new Rende);
    liubei->addSkill(new Jijiang);

    General *guanyu = new General(this, "guanyu", "shu"); // SHU 002
    guanyu->addSkill(new Wusheng);
    guanyu->addSkill(new Yijue);

    General *zhangfei = new General(this, "zhangfei", "shu"); // SHU 003
    zhangfei->addSkill(new Paoxiao);
    zhangfei->addSkill(new Tishen);

    General *zhugeliang = new General(this, "zhugeliang", "shu", 3); // SHU 004
    zhugeliang->addSkill(new Guanxing);
    zhugeliang->addSkill(new Kongcheng);
    zhugeliang->addSkill(new KongchengEffect);
    related_skills.insertMulti("kongcheng", "#kongcheng-effect");

    General *zhaoyun = new General(this, "zhaoyun", "shu"); // SHU 005
    zhaoyun->addSkill(new Longdan);
    zhaoyun->addSkill(new Yajiao);

    General *machao = new General(this, "machao", "shu"); // SHU 006
    machao->addSkill(new Mashu);
    machao->addSkill(new Tieji);
    machao->addSkill(new TiejiClear);
    related_skills.insertMulti("tieji", "#tieji-clear");

    General *huangyueying = new General(this, "huangyueying", "shu", 3, false); // SHU 007
    huangyueying->addSkill(new Jizhi);
    huangyueying->addSkill(new Qicai);

    General *st_xushu = new General(this, "st_xushu", "shu"); // SHU 017
    st_xushu->addSkill(new Zhuhai);
    
	if (Config.value("heg_skill", true).toBool())
		st_xushu->addSkill("jianyan");
	else {
		st_xushu->addSkill(new Qianxin);
		st_xushu->addRelateSkill("jianyan");
    }

    // Wu
    General *sunquan = new General(this, "sunquan$", "wu"); // WU 001
    sunquan->addSkill(new Zhiheng);
    sunquan->addSkill(new Jiuyuan);

    General *ganning = new General(this, "ganning", "wu"); // WU 002
    ganning->addSkill(new Qixi);
    ganning->addSkill(new Fenwei);

    General *lvmeng = new General(this, "lvmeng", "wu"); // WU 003
    lvmeng->addSkill(new Keji);
	if (Config.value("heg_skill", true).toBool())
		lvmeng->addSkill("gongxin");
	else
		lvmeng->addSkill(new Qinxue);
    General *huanggai = new General(this, "huanggai", "wu"); // WU 004
    huanggai->addSkill(new Kurou);
    huanggai->addSkill(new Zhaxiang);
    huanggai->addSkill(new ZhaxiangRedSlash);
    huanggai->addSkill(new ZhaxiangTargetMod);
    related_skills.insertMulti("zhaxiang", "#zhaxiang");
    related_skills.insertMulti("zhaxiang", "#zhaxiang-target");

    General *zhouyu = new General(this, "zhouyu", "wu", 3); // WU 005
    zhouyu->addSkill(new Yingzi);
    zhouyu->addSkill(new YingziMaxCards);
    zhouyu->addSkill(new Fanjian);
    related_skills.insertMulti("yingzi", "#yingzi");

    General *daqiao = new General(this, "daqiao", "wu", 3, false); // WU 006
    daqiao->addSkill(new Guose);
    daqiao->addSkill(new Liuli);

    General *luxun = new General(this, "luxun", "wu", 3); // WU 007
    luxun->addSkill(new Qianxun);
    luxun->addSkill(new Lianying);

    General *sunshangxiang = new General(this, "sunshangxiang", "wu", 3, false); // WU 008
    sunshangxiang->addSkill(new Jieyin);
    sunshangxiang->addSkill(new Xiaoji);

    // Qun
    General *huatuo = new General(this, "huatuo", "qun", 3); // QUN 001
    huatuo->addSkill(new Chuli);
    huatuo->addSkill(new Jijiu);

    General *lvbu = new General(this, "lvbu", "qun", 5); // QUN 002
    lvbu->addSkill(new Wushuang);
    lvbu->addSkill(new Liyu);

    General *diaochan = new General(this, "diaochan", "qun", 3, false); // QUN 003
    diaochan->addSkill(new Lijian);
    diaochan->addSkill(new Biyue);

    General *st_huaxiong = new General(this, "st_huaxiong", "qun", 6); // QUN 019
    st_huaxiong->addSkill(new Yaowu);

    General *st_yuanshu = new General(this, "st_yuanshu", "qun"); // QUN 021
    st_yuanshu->addSkill(new Wangzun);
    st_yuanshu->addSkill(new WangzunMaxCards);
    st_yuanshu->addSkill(new Tongji);
    related_skills.insertMulti("wangzun", "#wangzun-maxcard");

    General *st_gongsunzan = new General(this, "st_gongsunzan", "qun"); // QUN 026
    st_gongsunzan->addSkill(new Qiaomeng);
    st_gongsunzan->addSkill("yicong");

    // for skill cards
    addMetaObject<ZhihengCard>();
    addMetaObject<RendeCard>();
    addMetaObject<YijueCard>();
    addMetaObject<TuxiCard>();
    addMetaObject<JieyinCard>();
    addMetaObject<KurouCard>();
    addMetaObject<LijianCard>();
    addMetaObject<FanjianCard>();
    addMetaObject<ChuliCard>();
    addMetaObject<LiuliCard>();
    addMetaObject<LianyingCard>();
    addMetaObject<JijiangCard>();
    addMetaObject<YijiCard>();
    addMetaObject<FenweiCard>();
    addMetaObject<JianyanCard>();
    addMetaObject<GuoseCard>();

    skills << new Xiaoxi << new NonCompulsoryInvalidity << new Jianyan << new DuoChuPai << new FACEMAX ;
}

class SuperZhiheng: public Zhiheng {
public:
    SuperZhiheng():Zhiheng() {
        setObjectName("super_zhiheng");
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && player->usedTimes("ZhihengCard") < (player->getLostHp() + 1);
    }
};

class SuperGuanxing: public Guanxing {
public:
    SuperGuanxing(): Guanxing() {
        setObjectName("super_guanxing");
    }

    virtual int getGuanxingNum(Room *) const{
        return 5;
    }
};

class SuperMaxCards: public MaxCardsSkill {
public:
    SuperMaxCards(): MaxCardsSkill("super_max_cards") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill(objectName()))
            return target->getMark("@max_cards_test");
        return 0;
    }
};

class SuperOffensiveDistance: public DistanceSkill {
public:
    SuperOffensiveDistance(): DistanceSkill("super_offensive_distance") {
    }

    virtual int getCorrect(const Player *from, const Player *) const{
        if (from->hasSkill(objectName()))
            return -from->getMark("@offensive_distance_test");
        else
            return 0;
    }
};

class SuperDefensiveDistance: public DistanceSkill {
public:
    SuperDefensiveDistance(): DistanceSkill("super_defensive_distance") {
    }

    virtual int getCorrect(const Player *, const Player *to) const{
        if (to->hasSkill(objectName()))
            return to->getMark("@defensive_distance_test");
        else
            return 0;
    }
};

#include "sp.h"
class SuperYongsi: public Yongsi {
public:
    SuperYongsi(): Yongsi() {
        setObjectName("super_yongsi");
    }

    virtual int getKingdoms(ServerPlayer *yuanshu) const{
        return yuanshu->getMark("@yongsi_test");
    }
};

#include "wind.h"
class SuperJushou: public Jushou {
public:
    SuperJushou(): Jushou() {
        setObjectName("super_jushou");
    }

    virtual int getJushouDrawNum(ServerPlayer *caoren) const{
        return caoren->getMark("@jushou_test");
    }
};

#include "god.h"
#include "maneuvering.h"
class GdJuejing: public TriggerSkill {
public:
    GdJuejing(): TriggerSkill("gdjuejing") {
        events << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *gaodayihao, QVariant &data) const{
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != gaodayihao && move.to != gaodayihao)
                return false;
            if (move.to_place != Player::PlaceHand && !move.from_places.contains(Player::PlaceHand))
                return false;
        }
        if (gaodayihao->getHandcardNum() == 4)
            return false;
        int diff = qAbs(gaodayihao->getHandcardNum() - 4);
        if (gaodayihao->getHandcardNum() < 4) {
            room->sendCompulsoryTriggerLog(gaodayihao, objectName());
            gaodayihao->drawCards(diff, objectName());
        } else if (gaodayihao->getHandcardNum() > 4) {
            room->sendCompulsoryTriggerLog(gaodayihao, objectName());
            room->askForDiscard(gaodayihao, objectName(), diff, diff);
        }

        return false;
    }
};

class GdJuejingSkipDraw: public DrawCardsSkill {
public:
    GdJuejingSkipDraw(): DrawCardsSkill("#gdjuejing") {
    }

    virtual int getPriority(TriggerEvent) const{
        return 1;
    }

    virtual int getDrawNum(ServerPlayer *gaodayihao, int) const{
        LogMessage log;
        log.type = "#GdJuejing";
        log.from = gaodayihao;
        log.arg = "gdjuejing";
        gaodayihao->getRoom()->sendLog(log);

        return 0;
    }
};

class GdLonghun: public Longhun {
public:
    GdLonghun(): Longhun() {
        setObjectName("gdlonghun");
    }

    virtual int getEffHp(const Player *) const{
        return 1;
    }
};

class GdLonghunDuojian: public TriggerSkill {
public:
    GdLonghunDuojian(): TriggerSkill("#gdlonghun-duojian") {
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *gaodayihao, QVariant &) const{
        if (gaodayihao->getPhase() == Player::Start) {
            foreach (ServerPlayer *p, room->getOtherPlayers(gaodayihao)) {
               if (p->getWeapon() && p->getWeapon()->isKindOf("QinggangSword")) {
                   if (room->askForSkillInvoke(gaodayihao, "gdlonghun")) {
                       room->broadcastSkillInvoke("gdlonghun", 5);
                       gaodayihao->obtainCard(p->getWeapon());
                    }
                    break;
                }
            }
        }

        return false;
    }
};

TestPackage::TestPackage()
    : Package("test")
{
    // for test only
    General *zhiba_sunquan = new General(this, "zhiba_sunquan$", "wu", 4, true, true);
    zhiba_sunquan->addSkill(new SuperZhiheng);
    zhiba_sunquan->addSkill("jiuyuan");

    General *wuxing_zhuge = new General(this, "wuxing_zhugeliang", "shu", 3, true, true);
    wuxing_zhuge->addSkill(new SuperGuanxing);
    wuxing_zhuge->addSkill("kongcheng");

    General *gaodayihao = new General(this, "gaodayihao", "god", 1, true, true);
    gaodayihao->addSkill(new GdJuejing);
    gaodayihao->addSkill(new GdJuejingSkipDraw);
    gaodayihao->addSkill(new GdLonghun);
    gaodayihao->addSkill(new GdLonghunDuojian);
    related_skills.insertMulti("gdjuejing", "#gdjuejing");
    related_skills.insertMulti("gdlonghun", "#gdlonghun-duojian");

    General *super_yuanshu = new General(this, "super_yuanshu", "qun", 4, true, true);
    super_yuanshu->addSkill(new SuperYongsi);
    super_yuanshu->addSkill(new MarkAssignSkill("@yongsi_test", 4));
    related_skills.insertMulti("super_yongsi", "#@yongsi_test-4");
    super_yuanshu->addSkill("weidi");

    General *super_caoren = new General(this, "super_caoren", "wei", 4, true, true);
    super_caoren->addSkill(new SuperJushou);
    super_caoren->addSkill(new MarkAssignSkill("@jushou_test", 5));
    related_skills.insertMulti("super_jushou", "#@jushou_test-5");

    General *nobenghuai_dongzhuo = new General(this, "nobenghuai_dongzhuo$", "qun", 4, true, true);
    nobenghuai_dongzhuo->addSkill("jiuchi");
    nobenghuai_dongzhuo->addSkill("roulin");
    nobenghuai_dongzhuo->addSkill("baonue");

    new General(this, "sujiang", "god", 5, true, true);
    new General(this, "sujiangf", "god", 5, false, true);

    new General(this, "anjiang", "god", 4, true, true, true);

    skills << new SuperMaxCards << new SuperOffensiveDistance << new SuperDefensiveDistance;
}

ADD_PACKAGE(Test)
