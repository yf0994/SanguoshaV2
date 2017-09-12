#include "yjcm.h"
#include "skill.h"
#include "standard.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "engine.h"
#include "settings.h"
#include "ai.h"
#include "general.h"

class Yizhong: public TriggerSkill {
public:
    Yizhong(): TriggerSkill("yizhong") {
        events << SlashEffected;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && TriggerSkill::triggerable(target) && target->getArmor() == NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.slash->isBlack()) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());

            LogMessage log;
            log.type = "#SkillNullify";
            log.from = player;
            log.arg = objectName();
            log.arg2 = effect.slash->objectName();
            room->sendLog(log);

            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
				if (Config.value("yujin_down", true).toBool())
					room->setEmotion(player, "armor/renwang_shield");
                room->removePlayerMark(player, objectName()+"engine");
                return true;
            }
        }
        return false;
    }
};

class Luoying: public TriggerSkill {
public:
    Luoying(): TriggerSkill("luoying") {
        events << BeforeCardsMove;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player || move.from == NULL)
            return false;
        if (move.to_place == Player::DiscardPile
            && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD
                ||move.reason.m_reason == CardMoveReason::S_REASON_JUDGEDONE)) {
            QList<int> card_ids;
            int i = 0;
            foreach (int card_id, move.card_ids) {
                if (Sanguosha->getCard(card_id)->getSuit() == Card::Club
                    && ((move.reason.m_reason == CardMoveReason::S_REASON_JUDGEDONE
                         && move.from_places[i] == Player::PlaceJudge
                         && move.to_place == Player::DiscardPile)
                        || (move.reason.m_reason != CardMoveReason::S_REASON_JUDGEDONE
                            && room->getCardOwner(card_id) == move.from
                            && (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip))))
                    card_ids << card_id;
                ++i;
            }
            if (card_ids.isEmpty())
                return false;
            else if (player->askForSkillInvoke(objectName(), data)) {
                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {
					int ai_delay = Config.AIDelay;
					Config.AIDelay = 0;
					while (card_ids.length() > 1 && Config.value("caozhi_down", true).toBool()) {
						room->fillAG(card_ids, player);
						int id = room->askForAG(player, card_ids, true, objectName());
						if (id == -1) {
							room->clearAG(player);
							break;
						}
						card_ids.removeOne(id);
						room->clearAG(player);
					}
					Config.AIDelay = ai_delay;

					if (!card_ids.isEmpty()) {
						move.removeCardIds(card_ids );
						room->broadcastSkillInvoke("luoying");
						data = QVariant::fromValue(move);
						DummyCard *dummy = new DummyCard(card_ids);
						room->moveCardTo(dummy, player, Player::PlaceHand, move.reason, true);
						delete dummy;
					}
					room->removePlayerMark(player, objectName()+"engine");
				}
			}
        }
        return false;
    }
};

class Jiushi: public ZeroCardViewAsSkill {
public:
    Jiushi(): ZeroCardViewAsSkill("jiushi") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Analeptic::IsAvailable(player) && player->faceUp();
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern.contains("analeptic") && player->faceUp();
    }

    virtual const Card *viewAs() const{
        Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
        analeptic->setSkillName(objectName());
        return analeptic;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
		int index = Config.OL ? 1 : qrand() % 2 + 1;
		if (index == 2) index = 3;
        return index;
    }
};

class JiushiFlip: public TriggerSkill {
public:
    JiushiFlip(): TriggerSkill("#jiushi-flip") {
        events << PreCardUsed << PreDamageDone << DamageComplete;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == "jiushi")
                player->turnOver();
        } else if (triggerEvent == PreDamageDone) {
            //��ȷ�����ܵ��˺�������һ����־���Ա�DamageCompleteʱ�ж��Ƿ��ܷ�����ʫ�ٷ�����
            //�����ֹ���˺��������ִ�����̲�����뵽�������ɲμ�Room::damage��
            //if (thread->trigger(DamageCaused, this, damage_data.from, qdata)) break;
            //��ֹ���˺���DamageCaused���������᷵��true���ͻ�ֱ��break�Ǹ�do-whileѭ����
            player->tag["jiushi_injuried"] = true;

            player->tag["PredamagedFace"] = !player->faceUp();
        } else if (triggerEvent == DamageComplete) {
            //����˺��ѱ���ֹ������ִ�о�ʫ�ĵڶ���Ч��
            if (player->tag.contains("jiushi_injuried")) {
                player->tag.remove("jiushi_injuried");
                bool facedown = player->tag.value("PredamagedFace").toBool();
                player->tag.remove("PredamagedFace");
                if (facedown && !player->faceUp() && player->askForSkillInvoke("jiushi", data)) {
                    room->broadcastSkillInvoke("jiushi", 2);
					room->addPlayerMark(player, "jiushiengine");
					if (player->getMark("jiushiengine") > 0) {
						player->turnOver();
						room->removePlayerMark(player, "jiushiengine");
					}
                }
            }
        }
        return false;
    }
};

class Wuyan: public TriggerSkill {
public:
    Wuyan(): TriggerSkill("wuyan") {
        events << DamageCaused << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->getTypeId() == Card::TypeTrick) {
            if (triggerEvent == DamageInflicted && TriggerSkill::triggerable(player)) {
                LogMessage log;
                log.type = "#WuyanGood";
                log.from = player;
                log.arg = damage.card->objectName();
                log.arg2 = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(player, objectName());
                room->broadcastSkillInvoke(objectName(), 2);

                if (damage.from) 
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.from->objectName());
                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {
                    room->removePlayerMark(player, objectName()+"engine");
                    return true;
                }
            } else if (triggerEvent == DamageCaused && damage.from && TriggerSkill::triggerable(damage.from)) {
                LogMessage log;
                log.type = "#WuyanBad";
                log.from = player;
                log.arg = damage.card->objectName();
                log.arg2 = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(player, objectName());
                room->broadcastSkillInvoke(objectName(), 1);

				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, damage.from->objectName(), player->objectName());
                room->addPlayerMark(damage.from, objectName()+"engine");
                if (damage.from->getMark(objectName()+"engine") > 0) {
                    room->removePlayerMark(damage.from, objectName()+"engine");
                    return true;
                }
            }
        }

        return false;
    }
};

JujianCard::JujianCard() {
    mute = true;
}

bool JujianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self;
}

void JujianCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    if (effect.to->getGeneralName().contains("zhugeliang") || effect.to->getGeneralName() == "wolong")
        room->broadcastSkillInvoke("jujian", 2);
    else
        room->broadcastSkillInvoke("jujian", 1);

    room->addPlayerMark(effect.from, "jujianengine");
    if (effect.from->getMark("jujianengine") > 0) {
        QStringList choicelist;
        choicelist << "draw";
        if (effect.to->isWounded())
            choicelist << "recover";
        if (!effect.to->faceUp() || effect.to->isChained())
            choicelist << "reset";
        QString choice = room->askForChoice(effect.to, "jujian", choicelist.join("+"));

        if (choice == "draw")
            effect.to->drawCards(2, "jujian");
        else if (choice == "recover")
            room->recover(effect.to, RecoverStruct(effect.from));
        else if (choice == "reset") {
            if (effect.to->isChained())
                room->setPlayerProperty(effect.to, "chained", false);
            if (!effect.to->faceUp())
                effect.to->turnOver();
        }
        room->removePlayerMark(effect.from, "jujianengine");
    }
}

class JujianViewAsSkill: public OneCardViewAsSkill {
public:
    JujianViewAsSkill(): OneCardViewAsSkill("jujian") {
        filter_pattern = "^BasicCard!";
        response_pattern = "@@jujian";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        JujianCard *jujianCard = new JujianCard;
        jujianCard->addSubcard(originalCard);
        return jujianCard;
    }
};

class Jujian: public PhaseChangeSkill {
public:
    Jujian(): PhaseChangeSkill("jujian") {
        view_as_skill = new JujianViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *xushu) const{
        Room *room = xushu->getRoom();
        if (xushu->getPhase() == Player::Finish && xushu->canDiscard(xushu, "he"))
            room->askForUseCard(xushu, "@@jujian", "@jujian-card", -1, Card::MethodDiscard);
        return false;
    }
};

class Enyuan: public TriggerSkill {
public:
    Enyuan(): TriggerSkill("enyuan") {
        events << CardsMoveOneTime << AfterGiveCards << Damaged;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == CardsMoveOneTime || triggerEvent == AfterGiveCards) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to == player && move.from && move.from->isAlive() && move.from != move.to
                && move.to_pile_name != "wooden_ox"
                && move.reason.m_skillName != "rende"
                && move.card_ids.size() >= 2
                && move.reason.m_reason != CardMoveReason::S_REASON_PREVIEWGIVE) {
                move.from->setFlags("EnyuanDrawTarget");
                bool invoke = room->askForSkillInvoke(player, objectName(), data);
                move.from->setFlags("-EnyuanDrawTarget");
                if (invoke) {
                    room->broadcastSkillInvoke(objectName(), 1);
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), move.from->objectName());
                    room->addPlayerMark(player, objectName()+"engine");
                    if (player->getMark(objectName()+"engine") > 0) {
                        room->drawCards((ServerPlayer *)move.from, 1, objectName());
                        room->removePlayerMark(player, objectName()+"engine");
                    }
                }
            }
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *source = damage.from;
            if (!source || source == player) return false;
            int x = damage.damage;
            for (int i = 0; i < x; ++i) {
                if (source->isAlive() && player->isAlive() && room->askForSkillInvoke(player, objectName(), data)) {
                    room->broadcastSkillInvoke(objectName(), 2);
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), source->objectName());
                    room->addPlayerMark(player, objectName()+"engine");
                    if (player->getMark(objectName()+"engine") > 0) {
                        const Card *card = NULL;
                        if (!source->isKongcheng())
                            card = room->askForExchange(source, objectName(), 1, 1, false, "EnyuanGive::" + player->objectName(), true);
                        if (card) {
                            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(),
                                player->objectName(), objectName(), QString());
                            reason.m_playerId = player->objectName();
                            room->moveCardTo(card, source, player, Player::PlaceHand, reason);
                            delete card;
                        } else {
                            room->loseHp(source);
                        }
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

class Xuanhuo: public PhaseChangeSkill {
public:
    Xuanhuo(): PhaseChangeSkill("xuanhuo") {
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        if (player->getPhase() == Player::Draw) {
            ServerPlayer *to = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "xuanhuo-invoke", true, true);
            if (to) {
                room->broadcastSkillInvoke(objectName(), 1);
                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {
                    room->drawCards(to, 2, objectName());
                    if (!player->isAlive() || !to->isAlive()) {
						room->removePlayerMark(player, objectName()+"engine");
                        return true;
					}
                    QList<ServerPlayer *> targets;
                    foreach (ServerPlayer *vic, room->getOtherPlayers(to)) {
                        if (to->canSlash(vic))
                            targets << vic;
                    }
                    ServerPlayer *victim = NULL;
                    if (!targets.isEmpty()) {
                        victim = room->askForPlayerChosen(player, targets, "xuanhuo_slash", "@dummy-slash2:" + to->objectName());

                        LogMessage log;
                        log.type = "#CollateralSlash";
                        log.from = player;
                        log.to << victim;
                        room->sendLog(log);
                    }

                    if (victim == NULL || !room->askForUseSlashTo(to, victim, QString("xuanhuo-slash:%1:%2").arg(player->objectName()).arg(victim->objectName()))) {
                        if (to->isNude()) {
							room->removePlayerMark(player, objectName()+"engine");
							return true;
						}
                        room->setPlayerFlag(to, "xuanhuo_InTempMoving");
                        DummyCard *dummy = new DummyCard;
                            int first_id = room->askForCardChosen(player, to, "he", "xuanhuo");
                            Player::Place original_place = room->getCardPlace(first_id);
                            dummy->addSubcard(first_id);
                            to->addToPile("#xuanhuo", dummy, false);
                            if (!to->isNude()) {
                                int second_id = room->askForCardChosen(player, to, "he", "xuanhuo");
                                dummy->addSubcard(second_id);
                            }

                            room->moveCardTo(Sanguosha->getCard(first_id), to, original_place, false);
                            //move the first card back temporarily
                        room->setPlayerFlag(to, "-xuanhuo_InTempMoving");
                        room->getThread()->delay();
						room->broadcastSkillInvoke(objectName(), 2);
                        room->moveCardTo(dummy, player, Player::PlaceHand, false);
                        delete dummy;
                    }
                    room->removePlayerMark(player, objectName()+"engine");
                }

                return true;
            }
        }

        return false;
    }
};

class Huilei: public TriggerSkill {
public:
    Huilei():TriggerSkill("huilei") {
        events << Death;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player)
            return false;
        ServerPlayer *killer = death.damage ? death.damage->from : NULL;
        if (killer && killer != player) {
            LogMessage log;
            log.type = "#HuileiThrow";
            log.from = player;
            log.to << killer;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(player, objectName());

            QString killer_name = killer->getGeneralName();
            if (killer_name.contains("zhugeliang") || killer_name == "wolong")
                room->broadcastSkillInvoke(objectName(), 1);
            else
                room->broadcastSkillInvoke(objectName(), 2);

			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), killer->objectName());
            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                killer->throwAllHandCardsAndEquips();
                room->removePlayerMark(player, objectName()+"engine");
            }
        }

        return false;
    }
};

class Xuanfeng: public TriggerSkill {
public:
    Xuanfeng(): TriggerSkill("xuanfeng") {
        events << CardsMoveOneTime << EventPhaseEnd << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    void perform(Room *room, ServerPlayer *lingtong) const{
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *target, room->getOtherPlayers(lingtong)) {
            if (lingtong->canDiscard(target, "he"))
                targets << target;
        }
        if (targets.isEmpty())
            return;

        if (lingtong->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());

            ServerPlayer *first = room->askForPlayerChosen(lingtong, targets, "xuanfeng");
            ServerPlayer *second = NULL;
            int first_id = -1;
            int second_id = -1;
            if (first != NULL) {
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, lingtong->objectName(), first->objectName());
                first_id = room->askForCardChosen(lingtong, first, "he", "xuanfeng", false, Card::MethodDiscard);
                room->throwCard(first_id, first, lingtong);
            }
            if (!lingtong->isAlive())
                return;
            targets.clear();
            foreach (ServerPlayer *target, room->getOtherPlayers(lingtong)) {
                if (lingtong->canDiscard(target, "he"))
                    targets << target;
            }
            if (!targets.isEmpty())
                second = room->askForPlayerChosen(lingtong, targets, "xuanfeng", NULL, Config.value("xuanfeng_down", true).toBool());
            if (second != NULL) {
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, lingtong->objectName(), second->objectName());
                second_id = room->askForCardChosen(lingtong, second, "he", "xuanfeng", false, Card::MethodDiscard);
                room->throwCard(second_id, second, lingtong);
            }
        }
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *lingtong, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            lingtong->setMark("xuanfeng", 0);
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != lingtong)
                return false;

            if (lingtong->getPhase() == Player::Discard
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
                lingtong->addMark("xuanfeng", move.card_ids.length());

            if (move.from_places.contains(Player::PlaceEquip) && TriggerSkill::triggerable(lingtong))
                perform(room, lingtong);
        } else if (triggerEvent == EventPhaseEnd && TriggerSkill::triggerable(lingtong)
                   && lingtong->getPhase() == Player::Discard && lingtong->getMark("xuanfeng") >= 2) {
            perform(room, lingtong);
        }

        return false;
    }
};

class Pojun: public TriggerSkill {
public:
    Pojun(): TriggerSkill("pojun") {
        events << Damage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash") && !damage.chain && !damage.transfer
            && damage.to->isAlive() && !damage.to->hasFlag("Global_DebutFlag")
            && room->askForSkillInvoke(player, objectName(), data)) {
            int x = qMin(5, damage.to->getHp());
            room->broadcastSkillInvoke(objectName(), (x >= 3 || !damage.to->faceUp()) ? 2 : 1);
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());
            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                damage.to->drawCards(x, objectName());
                damage.to->turnOver();
                room->removePlayerMark(player, objectName()+"engine");
            }
        }
        return false;
    }
};

XianzhenCard::XianzhenCard() {
}

bool XianzhenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self && !to_select->isKongcheng();
}

void XianzhenCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->addPlayerMark(effect.from, "xianzhenengine");
    if (effect.from->getMark("xianzhenengine") > 0) {
        if (effect.from->pindian(effect.to, "xianzhen", NULL)) {
            ServerPlayer *target = effect.to;
            effect.from->tag["XianzhenTarget"] = QVariant::fromValue(target);
            room->setPlayerFlag(effect.from, "XianzhenSuccess");

            QStringList assignee_list = effect.from->property("extra_slash_specific_assignee").toString().split("+");
            assignee_list << target->objectName();
            room->setPlayerProperty(effect.from, "extra_slash_specific_assignee", assignee_list.join("+"));

            room->setFixedDistance(effect.from, effect.to, 1);
            room->addPlayerMark(effect.to, "Armor_Nullified");
        } else {
            room->setPlayerCardLimitation(effect.from, "use", "Slash", true);
        }
        room->removePlayerMark(effect.from, "xianzhenengine");
    }
}

class XianzhenViewAsSkill: public ZeroCardViewAsSkill {
public:
    XianzhenViewAsSkill(): ZeroCardViewAsSkill("xianzhen") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("XianzhenCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const{
        return new XianzhenCard;
    }
};

class Xianzhen: public TriggerSkill {
public:
    Xianzhen(): TriggerSkill("xianzhen") {
        events << EventPhaseChanging << Death;
        view_as_skill = new XianzhenViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->tag["XianzhenTarget"].value<ServerPlayer *>() != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *gaoshun, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        }
        ServerPlayer *target = gaoshun->tag["XianzhenTarget"].value<ServerPlayer *>();
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != gaoshun) {
                if (death.who == target) {
                    room->removeFixedDistance(gaoshun, target, 1);
                    gaoshun->tag.remove("XianzhenTarget");
                    room->setPlayerFlag(gaoshun, "-XianzhenSuccess");
                }
                return false;
            }
        }
        if (target) {
            QStringList assignee_list = gaoshun->property("extra_slash_specific_assignee").toString().split("+");
            assignee_list.removeOne(target->objectName());
            room->setPlayerProperty(gaoshun, "extra_slash_specific_assignee", assignee_list.join("+"));

            room->removeFixedDistance(gaoshun, target, 1);
            gaoshun->tag.remove("XianzhenTarget");
            room->removePlayerMark(target, "Armor_Nullified");
        }
        return false;
    }
};

class Jinjiu: public FilterSkill {
public:
    Jinjiu(): FilterSkill("jinjiu") {
    }

    virtual bool viewFilter(const Card *to_select) const{
        return to_select->objectName() == "analeptic";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

MingceCard::MingceCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

void MingceCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    QList<ServerPlayer *> targets;
    if (Slash::IsAvailable(effect.to)) {
        foreach (ServerPlayer *p, room->getOtherPlayers(effect.to)) {
            if (effect.to->canSlash(p))
                targets << p;
        }
    }

    ServerPlayer *target = NULL;
    QStringList choicelist;
    choicelist << "draw";
    if (!targets.isEmpty() && effect.from->isAlive()) {
        target = room->askForPlayerChosen(effect.from, targets, "mingce", "@dummy-slash2:" + effect.to->objectName());
        target->setFlags("MingceTarget"); // For AI

        LogMessage log;
        log.type = "#CollateralSlash";
        log.from = effect.from;
        log.to << target;
        room->sendLog(log);

        choicelist << "use";
    }

    try {
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), "mingce", QString());
        room->obtainCard(effect.to, this, reason);
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange)
            if (target && target->hasFlag("MingceTarget")) target->setFlags("-MingceTarget");
        throw triggerEvent;
    }

    room->addPlayerMark(effect.from, "mingceengine");
    if (effect.from->getMark("mingceengine") > 0) {
        QString choice = room->askForChoice(effect.to, "mingce", choicelist.join("+"));
        if (target && target->hasFlag("MingceTarget")) target->setFlags("-MingceTarget");

        if (choice == "use") {
            if (effect.to->canSlash(target, NULL, false)) {
                Slash *slash = new Slash(Card::NoSuit, 0);
                slash->setSkillName("_mingce");
                room->useCard(CardUseStruct(slash, effect.to, target));
            }
        } else if (choice == "draw") {
            effect.to->drawCards(1, "mingce");
        }
        room->removePlayerMark(effect.from, "mingceengine");
    }
}

class Mingce: public OneCardViewAsSkill {
public:
    Mingce(): OneCardViewAsSkill("mingce") {
        filter_pattern = "EquipCard,Slash";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("MingceCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        MingceCard *mingceCard = new MingceCard;
        mingceCard->addSubcard(originalCard);

        return mingceCard;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        if (card->isKindOf("Slash"))
            return -2;
        else
            return -1;
    }
};

class Zhichi: public TriggerSkill {
public:
    Zhichi(): TriggerSkill("zhichi") {
        events << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() != Player::NotActive)
            return false;

        ServerPlayer *current = room->getCurrent();
        if (current && current->isAlive() && current->getPhase() != Player::NotActive) {
            room->broadcastSkillInvoke(objectName(), 1);
            room->notifySkillInvoked(player, objectName());
            LogMessage log;
            log.type = "#ZhichiDamaged";
            log.from = player;
            room->sendLog(log);
            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
				if (player->getMark("@late") == 0)
					room->addPlayerMark(player, "@late");

                room->removePlayerMark(player, objectName()+"engine");
            }
        }
        return false;
    }
};

class ZhichiProtect: public TriggerSkill {
public:
    ZhichiProtect(): TriggerSkill("#zhichi-protect") {
        events << CardEffected;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if ((effect.card->isKindOf("Slash") || effect.card->isNDTrick()) && effect.to->getMark("@late") > 0) {
            room->broadcastSkillInvoke("zhichi", 2);
            room->notifySkillInvoked(effect.to, "zhichi");
            LogMessage log;
            log.type = "#ZhichiAvoid";
            log.from = effect.to;
            log.arg = "zhichi";
            room->sendLog(log);

            return true;
        }
        return false;
    }
};

class ZhichiClear: public TriggerSkill {
public:
    ZhichiClear(): TriggerSkill("#zhichi-clear") {
        events << EventPhaseChanging << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        } else {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player || player != room->getCurrent())
                return false;
        }

        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("@late") > 0)
                room->setPlayerMark(p, "@late", 0);
        }

        return false;
    }
};

GanluCard::GanluCard() {
}

void GanluCard::swapEquip(ServerPlayer *first, ServerPlayer *second) const{
    Room *room = first->getRoom();

    QList<int> equips1, equips2;
    foreach (const Card *equip, first->getEquips())
        equips1.append(equip->getId());
    foreach (const Card *equip, second->getEquips())
        equips2.append(equip->getId());

    QList<CardsMoveStruct> exchangeMove;
    CardsMoveStruct move1(equips1, second, Player::PlaceEquip,
                          CardMoveReason(CardMoveReason::S_REASON_SWAP, first->objectName(), second->objectName(), "ganlu", QString()));
    CardsMoveStruct move2(equips2, first, Player::PlaceEquip,
                          CardMoveReason(CardMoveReason::S_REASON_SWAP, second->objectName(), first->objectName(), "ganlu", QString()));
    exchangeMove.push_back(move2);
    exchangeMove.push_back(move1);
    room->moveCardsAtomic(exchangeMove, false);
}

bool GanluCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    return targets.length() == 2;
}

bool GanluCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    switch (targets.length()) {
    case 0: return true;
    case 1: {
            int n1 = targets.first()->getEquips().length();
            int n2 = to_select->getEquips().length();
            return qAbs(n1 - n2) <= Self->getLostHp();
        }
    default:
        return false;
    }
}

void GanluCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    LogMessage log;
    log.type = "#GanluSwap";
    log.from = source;
    log.to = targets;
    room->sendLog(log);
    room->addPlayerMark(source, "ganluengine");
    if (source->getMark("ganluengine") > 0) {
        swapEquip(targets.first(), targets[1]);
        room->removePlayerMark(source, "ganluengine");
    }
}

class Ganlu: public ZeroCardViewAsSkill {
public:
    Ganlu(): ZeroCardViewAsSkill("ganlu") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("GanluCard");
    }

    virtual const Card *viewAs() const{
        return new GanluCard;
    }
};

class Buyi: public TriggerSkill {
public:
    Buyi(): TriggerSkill("buyi") {
        events << Dying;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *wuguotai, QVariant &data) const{
        DyingStruct dying = data.value<DyingStruct>();
        ServerPlayer *player = dying.who;
        if (player->isKongcheng()) return false;
        if (player->getHp() < 1 && wuguotai->askForSkillInvoke(objectName(), data)) {
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, wuguotai->objectName(), player->objectName());
            room->addPlayerMark(wuguotai, objectName()+"engine");
			if (wuguotai->getMark(objectName()+"engine") > 0) {
				const Card *card = NULL;
				if (player == wuguotai)
					card = room->askForCardShow(player, wuguotai, objectName());
				else {
					int card_id = room->askForCardChosen(wuguotai, player, "h", "buyi");
					card = Sanguosha->getCard(card_id);
				}

                room->broadcastSkillInvoke(objectName(), 1);
				room->showCard(player, card->getEffectiveId());

				if (card->getTypeId() != Card::TypeBasic) {
					if (!player->isJilei(card))
						room->throwCard(card, player);

                    room->getThread()->delay();
					room->broadcastSkillInvoke(objectName(), 2);
					room->recover(player, RecoverStruct(wuguotai));
				}
                room->removePlayerMark(wuguotai, objectName()+"engine");
            }
			if (player->getHp() > 0) 
				return true;
        }
        return false;
    }
};

XinzhanCard::XinzhanCard() {
    target_fixed = true;
}

void XinzhanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->addPlayerMark(source, "xinzhanengine");
    if (source->getMark("xinzhanengine") > 0) {
		QList<int> cards = room->getNCards(3), left;

		LogMessage log;
		log.type = "$ViewDrawPile";
		log.from = source;
		log.card_str = IntList2StringList(cards).join("+");
		room->sendLog(log, source);

		left = cards;

		QList<int> hearts, non_hearts;
		foreach (int card_id, cards) {
			const Card *card = Sanguosha->getCard(card_id);
			if (card->getSuit() == Card::Heart)
				hearts << card_id;
			else
				non_hearts << card_id;
		}

		if (!hearts.isEmpty()) {
			DummyCard *dummy = new DummyCard;
			do {
				room->fillAG(left, source, non_hearts);
				int card_id = room->askForAG(source, hearts, true, "xinzhan");
				if (card_id == -1) {
					room->clearAG(source);
					break;
				}

				hearts.removeOne(card_id);
				left.removeOne(card_id);

				dummy->addSubcard(card_id);
				room->clearAG(source);
			} while (!hearts.isEmpty());

			if (dummy->subcardsLength() > 0) {
				room->doBroadcastNotify(QSanProtocol::S_COMMAND_UPDATE_PILE, Json::Value(room->getDrawPile().length() + dummy->subcardsLength()));
				source->obtainCard(dummy);
				foreach (int id, dummy->getSubcards())
					room->showCard(source, id);
			}
			delete dummy;
		}

		if (!left.isEmpty())
			room->askForGuanxing(source, left, Room::GuanxingUpOnly);
        room->removePlayerMark(source, "xinzhanengine");
    }
 }

class Xinzhan: public ZeroCardViewAsSkill {
public:
    Xinzhan(): ZeroCardViewAsSkill("xinzhan") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("XinzhanCard") && player->getHandcardNum() > player->getMaxHp();
    }

    virtual const Card *viewAs() const{
        return new XinzhanCard;
    }
};

class Quanji: public MasochismSkill {
public:
    Quanji(): MasochismSkill("#quanji") {
        frequency = Frequent;
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const{
        Room *room = player->getRoom();

        int x = damage.damage;
        for (int i = 0; i < x; ++i) {
            if (player->hasSkill("quanji") && player->askForSkillInvoke("quanji")) {
                room->broadcastSkillInvoke("quanji");
                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {
                    room->drawCards(player, 1, objectName());
                    if (!player->isKongcheng()) {
                        int card_id;
                        if (player->getHandcardNum() == 1) {
                            room->getThread()->delay();
                            card_id = player->handCards().first();
                        } else {
                            const Card *card = room->askForExchange(player, "quanji", 1, 1, false, "QuanjiPush");
                            card_id = card->getEffectiveId();
                            delete card;
                        }
                        player->addToPile("power", card_id);
                    }
                    room->removePlayerMark(player, objectName()+"engine");
                }
            }
        }

    }
};

class QuanjiKeep: public MaxCardsSkill {
public:
    QuanjiKeep(): MaxCardsSkill("quanji") {
        frequency = Frequent;
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill(objectName()))
            return target->getPile("power").length();
        else
            return 0;
    }
};

class Zili: public PhaseChangeSkill {
public:
    Zili(): PhaseChangeSkill("zili") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Start
               && target->getMark("zili") == 0
               && target->getPile("power").length() >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        room->notifySkillInvoked(player, objectName());

        LogMessage log;
        log.type = "#ZiliWake";
        log.from = player;
        log.arg = QString::number(player->getPile("power").length());
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());
        //room->doLightbox("$ZiliAnimate", 4000);

        room->addPlayerMark(player, objectName()+"engine");
        if (player->getMark(objectName()+"engine") > 0) {
            room->setPlayerMark(player, "zili", 1);
            if (room->changeMaxHpForAwakenSkill(player)) {
                if (player->isWounded() && room->askForChoice(player, objectName(), "recover+draw") == "recover")
                    room->recover(player, RecoverStruct(player));
                else
                    room->drawCards(player, 2, objectName());
                if (player->getMark("zili") == 1)
                    room->acquireSkill(player, "paiyi");
            }
            room->removePlayerMark(player, objectName()+"engine");
        }

        return false;
    }
};


PaiyiCard::PaiyiCard()
{
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool PaiyiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void PaiyiCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *zhonghui = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = zhonghui->getRoom();
    QList<int> powers = zhonghui->getPile("power");
    if (powers.isEmpty()) return;

    room->broadcastSkillInvoke("paiyi", target == zhonghui ? 1 : 2);

    int card_id = subcards.first();

    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), target->objectName(), "paiyi", QString());
    room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
    room->addPlayerMark(effect.from, "paiyiengine");
    if (effect.from->getMark("paiyiengine") > 0) {
        room->drawCards(target, 2, "paiyi");
        if (target->getHandcardNum() > zhonghui->getHandcardNum())
            room->damage(DamageStruct("paiyi", zhonghui, target));
        room->removePlayerMark(effect.from, "paiyiengine");
    }
}

class Paiyi : public OneCardViewAsSkill
{
public:
    Paiyi() : OneCardViewAsSkill("paiyi")
    {
        expand_pile = "power";
        filter_pattern = ".|.|.|power";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->getPile("power").isEmpty() && !player->hasUsed("PaiyiCard");
    }

    const Card *viewAs(const Card *c) const
    {
        PaiyiCard *py = new PaiyiCard;
        py->addSubcard(c);
        return py;
    }
};

class Jueqing: public TriggerSkill {
public:
    Jueqing(): TriggerSkill("jueqing") {
        frequency = Compulsory;
        events << Predamage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from == player) {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                room->loseHp(damage.to, damage.damage);
                room->removePlayerMark(player, objectName()+"engine");
				return true;
            }
        }
        return false;
    }
};

Shangshi::Shangshi(): TriggerSkill("shangshi") {
    events << HpChanged << MaxHpChanged << CardsMoveOneTime;
    frequency = Frequent;
}

int Shangshi::getMaxLostHp(ServerPlayer *player) const{
    int losthp = player->getLostHp();
    if (losthp > 2)
        losthp = 2;
    return qMin(losthp, player->getMaxHp());
}

bool Shangshi::trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
    int losthp = getMaxLostHp(player);
    if (triggerEvent == CardsMoveOneTime) {
        bool can_invoke = false;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if ((move.from == player && move.from_places.contains(Player::PlaceHand)) || (move.to == player && move.to_place == Player::PlaceHand))
            can_invoke = true;
        if (!can_invoke)
            return false;
    }

    if (player->getHandcardNum() < losthp && player->askForSkillInvoke(objectName())) {
        room->broadcastSkillInvoke("shangshi");
        room->addPlayerMark(player, objectName()+"engine");
        if (player->getMark(objectName()+"engine") > 0) {
            player->drawCards(losthp - player->getHandcardNum(), objectName());
            room->removePlayerMark(player, objectName()+"engine");
        }
    }

    return false;
}

YJCMPackage::YJCMPackage()
    : Package("YJCM")
{
    General *caozhi = new General(this, "caozhi", "wei", 3); // YJ 001
    caozhi->addSkill(new Luoying);
    caozhi->addSkill(new Jiushi);
    caozhi->addSkill(new JiushiFlip);
    related_skills.insertMulti("jiushi", "#jiushi-flip");

    General *chengong = new General(this, "chengong", "qun", 3); // YJ 002
    chengong->addSkill(new Zhichi);
    chengong->addSkill(new ZhichiProtect);
    chengong->addSkill(new ZhichiClear);
    chengong->addSkill(new Mingce);
    related_skills.insertMulti("zhichi", "#zhichi-protect");
    related_skills.insertMulti("zhichi", "#zhichi-clear");

    General *fazheng = new General(this, "fazheng", "shu", 3); // YJ 003
    fazheng->addSkill(new Enyuan);
    fazheng->addSkill(new Xuanhuo);
    fazheng->addSkill(new FakeMoveSkill("xuanhuo"));
    related_skills.insertMulti("xuanhuo", "#xuanhuo-fake-move");

    General *gaoshun = new General(this, "gaoshun", "qun"); // YJ 004
    gaoshun->addSkill(new Xianzhen);
    gaoshun->addSkill(new Jinjiu);

    General *lingtong = new General(this, "lingtong", "wu"); // YJ 005
    lingtong->addSkill(new Xuanfeng);

    General *masu = new General(this, "masu", "shu", 3); // YJ 006
    masu->addSkill(new Xinzhan);
    masu->addSkill(new Huilei);

    General *wuguotai = new General(this, "wuguotai", "wu", 3, false); // YJ 007
    wuguotai->addSkill(new Ganlu);
    wuguotai->addSkill(new Buyi);

    General *xusheng = new General(this, "xusheng", "wu"); // YJ 008
    xusheng->addSkill(new Pojun);

    General *xushu = new General(this, "xushu", "shu", 3); // YJ 009
    xushu->addSkill(new Wuyan);
    xushu->addSkill(new Jujian);

    General *yujin = new General(this, "yujin", "wei"); // YJ 010
    yujin->addSkill(new Yizhong);

    General *zhangchunhua = new General(this, "zhangchunhua", "wei", 3, false); // YJ 011
    zhangchunhua->addSkill(new Jueqing);
    zhangchunhua->addSkill(new Shangshi);

    General *zhonghui = new General(this, "zhonghui", "wei"); // YJ 012
    zhonghui->addSkill(new QuanjiKeep);
    zhonghui->addSkill(new Quanji);
    zhonghui->addSkill(new Zili);
    zhonghui->addRelateSkill("paiyi");
    related_skills.insertMulti("quanji", "#quanji");

    addMetaObject<MingceCard>();
    addMetaObject<GanluCard>();
    addMetaObject<XianzhenCard>();
    addMetaObject<XinzhanCard>();
    addMetaObject<JujianCard>();
    addMetaObject<PaiyiCard>();

    skills << new Paiyi;
}

ADD_PACKAGE(YJCM)
