#include "standard.h"
#include "skill.h"
#include "wind.h"
#include "client.h"
#include "engine.h"
#include "nostalgia.h"
#include "yjcm.h"
#include "yjcm2013.h"
#include "settings.h"

class MoonSpearSkill: public WeaponSkill {
public:
    MoonSpearSkill(): WeaponSkill("moon_spear") {
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

        player->setFlags("MoonspearUse");
        if (!room->askForUseCard(player, "slash", "@moon-spear-slash", -1, Card::MethodUse, false))
            player->setFlags("-MoonspearUse");

        return false;
    }
};

MoonSpear::MoonSpear(Suit suit, int number)
    : Weapon(suit, number, 3)
{
    setObjectName("moon_spear");
}

NostalgiaPackage::NostalgiaPackage()
    : Package("nostalgia")
{
    type = CardPack;

    Card *moon_spear = new MoonSpear;
    moon_spear->setParent(this);

    skills << new MoonSpearSkill;
}

// old yjcm's generals

class NosWuyan: public TriggerSkill {
public:
    NosWuyan(): TriggerSkill("noswuyan") {
        events << CardEffected;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.to == effect.from)
            return false;
        if (effect.card->isNDTrick()) {
            if (effect.from && effect.from->hasSkill(objectName())) {
				room->addPlayerMark(effect.from, objectName()+"engine");
				if (effect.from->getMark(objectName()+"engine") > 0) {
					LogMessage log;
					log.type = "#WuyanBaD";
					log.from = effect.from;
					log.to << effect.to;
					log.arg = effect.card->objectName();
					log.arg2 = objectName();
					room->sendLog(log);
					room->notifySkillInvoked(effect.from, objectName());
					room->broadcastSkillInvoke("noswuyan", 1);
					room->removePlayerMark(effect.from, objectName()+"engine");
					return true;
				}
            }
            if (effect.to->hasSkill(objectName()) && effect.from) {
				room->addPlayerMark(effect.to, objectName()+"engine");
				if (effect.to->getMark(objectName()+"engine") > 0) {
					LogMessage log;
					log.type = "#WuyanGooD";
					log.from = effect.to;
					log.to << effect.from;
					log.arg = effect.card->objectName();
					log.arg2 = objectName();
					room->sendLog(log);
					room->notifySkillInvoked(effect.to, objectName());
					room->broadcastSkillInvoke("noswuyan", 2);
					room->removePlayerMark(effect.to, objectName()+"engine");
					return true;
				}
            }
        }
        return false;
    }
};

NosJujianCard::NosJujianCard() {
}

void NosJujianCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    int n = subcardsLength();
    room->addPlayerMark(effect.from, "nosjujianengine");
    if (effect.from->getMark("nosjujianengine") > 0) {
		effect.to->drawCards(n, "nosjujian");

		if (effect.from->isAlive() && n == 3) {
			QSet<Card::CardType> types;
			foreach (int card_id, effect.card->getSubcards())
				types << Sanguosha->getCard(card_id)->getTypeId();

			if (types.size() == 1) {
				LogMessage log;
				log.type = "#JujianRecover";
				log.from = effect.from;
				const Card *card = Sanguosha->getCard(subcards.first());
				log.arg = card->getType();
				room->sendLog(log);
				room->recover(effect.from, RecoverStruct(effect.from));
			}
		}
        room->removePlayerMark(effect.from, "nosjujianengine");
    }
}

class NosJujian: public ViewAsSkill {
public:
    NosJujian(): ViewAsSkill("nosjujian") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return selected.length() < 3 && !Self->isJilei(to_select);
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("NosJujianCard");
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
            return NULL;

        NosJujianCard *card = new NosJujianCard;
        card->addSubcards(cards);
        return card;
    }
};

class NosEnyuan: public TriggerSkill {
public:
    NosEnyuan(): TriggerSkill("nosenyuan") {
        events << HpRecover << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == HpRecover) {
            RecoverStruct recover = data.value<RecoverStruct>();
            if (recover.who && recover.who != player) {
                room->broadcastSkillInvoke("nosenyuan", qrand() % 2 + 1);
				for (int i = 0; i < recover.recover; ++i) {
					if (!recover.who->isAlive())
						return false;
					room->sendCompulsoryTriggerLog(player, objectName());
					room->addPlayerMark(player, objectName()+"engine");
					if (player->getMark(objectName()+"engine") > 0) {
						recover.who->drawCards(1, objectName());
						room->removePlayerMark(player, objectName()+"engine");
					}
				}
            }
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *source = damage.from;
            if (source && source != player) {
                room->broadcastSkillInvoke("nosenyuan", qrand() % 2 + 3);
                room->sendCompulsoryTriggerLog(player, objectName());

                const Card *card = room->askForCard(source, ".|heart|.|hand", "@nosenyuan-heart", data, Card::MethodNone);
                if (card)
                    player->obtainCard(card);
                else
                    room->loseHp(source);
            }
        }

        return false;
    }
};

NosXuanhuoCard::NosXuanhuoCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

void NosXuanhuoCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), "nosxuanhuo", QString());
    room->obtainCard(effect.to, this, reason);

    room->addPlayerMark(effect.from, "nosxuanhuoengine");
    if (effect.from->getMark("nosxuanhuoengine") > 0) {
		int card_id = room->askForCardChosen(effect.from, effect.to, "he", "nosxuanhuo");
		CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
		room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);

		QList<ServerPlayer *> targets = room->getOtherPlayers(effect.to);
		targets.removeOne(effect.from);
		if (!targets.isEmpty()) {
			ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "nosxuanhuo", "@nosxuanhuo-give:" + effect.to->objectName(), true);
			if (target != NULL) {
				CardMoveReason reason2(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), target->objectName(), "nosxuanhuo", QString());
				room->obtainCard(target, Sanguosha->getCard(card_id), reason2, false);
			}
		}
        room->removePlayerMark(effect.from, "nosxuanhuoengine");
    }
}

class NosXuanhuo: public OneCardViewAsSkill {
public:
    NosXuanhuo():OneCardViewAsSkill("nosxuanhuo") {
        filter_pattern = ".|heart|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && !player->hasUsed("NosXuanhuoCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        NosXuanhuoCard *xuanhuoCard = new NosXuanhuoCard;
        xuanhuoCard->addSubcard(originalCard);
        return xuanhuoCard;
    }
};

class NosXuanfeng: public TriggerSkill {
public:
    NosXuanfeng(): TriggerSkill("nosxuanfeng") {
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && move.from_places.contains(Player::PlaceEquip)) {
                QStringList choicelist;
                choicelist << "nothing";
                QList<ServerPlayer *> targets1;
                foreach (ServerPlayer *target, room->getAlivePlayers()) {
                    if (player->canSlash(target, NULL, false))
                        targets1 << target;
                }
                Slash *slashx = new Slash(Card::NoSuit, 0);
                if (!targets1.isEmpty() && !player->isCardLimited(slashx, Card::MethodUse))
                    choicelist << "slash";
                slashx->deleteLater();
                QList<ServerPlayer *> targets2;
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (player->distanceTo(p) <= 1)
                        targets2 << p;
                }
                if (!targets2.isEmpty()) choicelist << "damage";

                QString choice = room->askForChoice(player, objectName(), choicelist.join("+"));
				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					if (choice == "slash") {
						ServerPlayer *target = room->askForPlayerChosen(player, targets1, "nosxuanfeng_slash", "@dummy-slash");
						room->broadcastSkillInvoke(objectName(), 1);
						Slash *slash = new Slash(Card::NoSuit, 0);
						slash->setSkillName(objectName());
						room->useCard(CardUseStruct(slash, player, target));
					} else if (choice == "damage") {
						room->broadcastSkillInvoke(objectName(), 2);

						LogMessage log;
						log.type = "#InvokeSkill";
						log.from = player;
						log.arg = objectName();
						room->sendLog(log);
						room->notifySkillInvoked(player, objectName());

						ServerPlayer *target = room->askForPlayerChosen(player, targets2, "nosxuanfeng_damage", "@nosxuanfeng-damage");
						room->damage(DamageStruct("nosxuanfeng", player, target));
					}
					room->removePlayerMark(player, objectName()+"engine");
				}
			}
        }

        return false;
    }
};

class NosShangshi: public Shangshi {
public:
    NosShangshi(): Shangshi() {
        setObjectName("nosshangshi");
    }

    virtual int getMaxLostHp(ServerPlayer *zhangchunhua) const{
        return qMin(zhangchunhua->getLostHp(), zhangchunhua->getMaxHp());
    }
};

class NosFuhun: public TriggerSkill {
public:
    NosFuhun(): TriggerSkill("nosfuhun") {
        events << EventPhaseStart << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Draw && TriggerSkill::triggerable(player)) {
            if (player->askForSkillInvoke(objectName())) {
				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					int card1 = room->drawCard();
					int card2 = room->drawCard();
					QList<int> ids;
					ids << card1 << card2;
					bool diff = (Sanguosha->getCard(card1)->getColor() != Sanguosha->getCard(card2)->getColor());

					CardsMoveStruct move;
					move.card_ids = ids;
					move.reason = CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), "fuhun", QString());
					move.to_place = Player::PlaceTable;
					room->moveCardsAtomic(move, true);
					room->getThread()->delay();

					DummyCard *dummy = new DummyCard(move.card_ids);
					room->obtainCard(player, dummy);
					delete dummy;

					if (diff) {
						room->handleAcquireDetachSkills(player, "wusheng|paoxiao");
						room->broadcastSkillInvoke(objectName(), qrand() % 2 + 1);
						player->setFlags(objectName());
					} else {
						room->broadcastSkillInvoke(objectName(), 3);
					}

					room->removePlayerMark(player, objectName()+"engine");
				}
                return true;
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && player->hasFlag(objectName()))
                room->handleAcquireDetachSkills(player, "-wusheng|-paoxiao", true);
        }

        return false;
    }
};

class NosGongqi: public OneCardViewAsSkill {
public:
    NosGongqi(): OneCardViewAsSkill("nosgongqi") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "slash";
    }

    virtual bool viewFilter(const Card *to_select) const{
        if (to_select->getTypeId() != Card::TypeEquip)
            return false;

        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            Slash *slash = new Slash(Card::SuitToBeDecided, -1);
            slash->addSubcard(to_select->getEffectiveId());
            slash->deleteLater();
            return slash->isAvailable(Self);
        }
        return true;
    }

    const Card *viewAs(const Card *originalCard) const{
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->addSubcard(originalCard);
        slash->setSkillName(objectName());
        return slash;
    }
};

class NosGongqiTargetMod: public TargetModSkill {
public:
    NosGongqiTargetMod(): TargetModSkill("#nosgongqi-target") {
        frequency = NotFrequent;
    }

    virtual int getDistanceLimit(const Player *, const Card *card) const{
        if (card->getSkillName() == "nosgongqi")
            return 1000;
        else
            return 0;
    }
};

NosJiefanCard::NosJiefanCard() {
    target_fixed = true;
    mute = true;
}

void NosJiefanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->addPlayerMark(source, "nosjiefanengine");
    if (source->getMark("nosjiefanengine") > 0) {
		ServerPlayer *current = room->getCurrent();
		if (!current || current->isDead() || current->getPhase() == Player::NotActive) return;
		ServerPlayer *who = room->getCurrentDyingPlayer();
		if (!who) return;

		source->setFlags("NosJiefanUsed");
		room->setTag("NosJiefanTarget", QVariant::fromValue(who));
		bool use_slash = room->askForUseSlashTo(source, current, "nosjiefan-slash:" + current->objectName(), false);
		if (!use_slash) {
			source->setFlags("-NosJiefanUsed");
			room->removeTag("NosJiefanTarget");
			room->setPlayerFlag(source, "Global_NosJiefanFailed");
		}
        room->removePlayerMark(source, "nosjiefanengine");
    }
}

class NosJiefanViewAsSkill: public ZeroCardViewAsSkill {
public:
    NosJiefanViewAsSkill(): ZeroCardViewAsSkill("nosjiefan") {
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (!pattern.contains("peach")) return false;
        if (player->hasFlag("Global_NosJiefanFailed")) return false;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->getPhase() != Player::NotActive)
                return true;
        }
        return false;
    }

    virtual const Card *viewAs() const{
        return new NosJiefanCard;
    }
};

class NosJiefan: public TriggerSkill {
public:
    NosJiefan(): TriggerSkill("nosjiefan") {
        events << DamageCaused << CardFinished << PreCardUsed;
        view_as_skill = new NosJiefanViewAsSkill;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *handang, QVariant &data) const{
        if (triggerEvent == PreCardUsed) {
            if (!handang->hasFlag("NosJiefanUsed"))
                return false;

            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash")) {
                handang->setFlags("-NosJiefanUsed");
                room->setCardFlag(use.card, "nosjiefan-slash");
            }
        } else if (triggerEvent == DamageCaused) {
            ServerPlayer *current = room->getCurrent();
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->hasFlag("nosjiefan-slash")) {
                LogMessage log2;
                log2.type = "#NosJiefanPrevent";
                log2.from = handang;
                log2.to << damage.to;
                room->sendLog(log2);

                ServerPlayer *target = room->getTag("NosJiefanTarget").value<ServerPlayer *>();
                if (target && target->getHp() > 0) {
                    LogMessage log;
                    log.type = "#NosJiefanNull1";
                    log.from = target;
                    room->sendLog(log);
                } else if (target && target->isDead()) {
                    LogMessage log;
                    log.type = "#NosJiefanNull2";
                    log.from = target;
                    log.to << handang;
                    room->sendLog(log);
                } else if (handang->hasFlag("Global_PreventPeach")) {
                    LogMessage log;
                    log.type = "#NosJiefanNull3";
                    log.from = current;
                    room->sendLog(log);
                } else {
                    Peach *peach = new Peach(Card::NoSuit, 0);
                    peach->setSkillName("_nosjiefan");

                    room->setCardFlag(damage.card, "nosjiefan_success");
                    if ((target->getGeneralName().contains("sunquan")
                         || target->getGeneralName().contains("sunce")
                         || target->getGeneralName().contains("sunjian"))
                        && target->isLord())
                        handang->setFlags("NosJiefanToLord");
                    room->useCard(CardUseStruct(peach, handang, target));
                    handang->setFlags("-NosJiefanToLord");
                }
                return true;
            }
            return false;
        } else if (triggerEvent == CardFinished && !room->getTag("NosJiefanTarget").isNull()) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.card->hasFlag("nosjiefan-slash")) {
                if (!use.card->hasFlag("nosjiefan_success"))
                    room->setPlayerFlag(handang, "Global_NosJiefanFailed");
                room->removeTag("NosJiefanTarget");
            }
        }

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const{
        if (player->hasFlag("NosJiefanToLord"))
            return 2;
        else
            return 1;
    }
};

class NosQianxi: public TriggerSkill {
public:
    NosQianxi(): TriggerSkill("nosqianxi") {
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        if (player->distanceTo(damage.to) == 1 && damage.card && damage.card->isKindOf("Slash")
            && damage.by_user && !damage.chain && !damage.transfer && player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName(), 1);
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				JudgeStruct judge;
				judge.pattern = ".|heart";
				judge.good = false;
				judge.who = player;
				judge.reason = objectName();

				room->judge(judge);
				if (judge.isGood()) {
					room->broadcastSkillInvoke(objectName(), 2);
					room->loseMaxHp(damage.to);
					return true;
				} else
					room->broadcastSkillInvoke(objectName(), 3);
				room->removePlayerMark(player, objectName()+"engine");
			}
        }
        return false;
    }
};

class NosZhenlie: public TriggerSkill {
public:
    NosZhenlie(): TriggerSkill("noszhenlie") {
        events << AskForRetrial;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if (judge->who != player)
            return false;

        if (player->askForSkillInvoke(objectName(), data)) {
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				int card_id = room->drawCard();
				room->broadcastSkillInvoke(objectName(), room->getCurrent() == player ? 2 : 1);
				room->getThread()->delay();
				const Card *card = Sanguosha->getCard(card_id);

				room->retrial(card, player, judge, objectName());
				room->removePlayerMark(player, objectName()+"engine");
			}
        }
        return false;
    }
};

class NosMiji: public PhaseChangeSkill {
public:
    NosMiji(): PhaseChangeSkill("nosmiji") {
        frequency = Frequent;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        if (!player->isWounded())
            return false;
        if (player->getPhase() == Player::Start || player->getPhase() == Player::Finish) {
            if (!player->askForSkillInvoke(objectName()))
                return false;
            Room *room = player->getRoom();
            room->broadcastSkillInvoke(objectName(), 1);
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				JudgeStruct judge;
				judge.pattern = ".|black";
				judge.good = true;
				judge.reason = objectName();
				judge.who = player;

				room->judge(judge);

				if (judge.isGood() && player->isAlive()) {
					QList<int> pile_ids = room->getNCards(player->getLostHp(), false);
					room->fillAG(pile_ids, player);
					ServerPlayer *target = room->askForPlayerChosen(player, room->getAllPlayers(), objectName());
					room->clearAG(player);
					if (target == player)
						room->broadcastSkillInvoke(objectName(), 2);
					else if (target->getGeneralName().contains("machao"))
						room->broadcastSkillInvoke(objectName(), 4);
					else
						room->broadcastSkillInvoke(objectName(), 3);

					DummyCard *dummy = new DummyCard(pile_ids);
					player->setFlags("Global_GongxinOperator");
					target->obtainCard(dummy, false);
					player->setFlags("-Global_GongxinOperator");
					delete dummy;
				}
				room->removePlayerMark(player, objectName()+"engine");
			}
        }
        return false;
    }
};

class NosChengxiang: public Chengxiang {
public:
    NosChengxiang(): Chengxiang() {
        setObjectName("noschengxiang");
        total_point = 12;
    }
};

NosRenxinCard::NosRenxinCard() {
    target_fixed = true;
    mute = true;
}

void NosRenxinCard::use(Room *room, ServerPlayer *player, QList<ServerPlayer *> &) const{
    if (player->isKongcheng()) return;
    ServerPlayer *who = room->getCurrentDyingPlayer();
    if (!who) return;

    room->broadcastSkillInvoke("renxin");
	CardMoveReason reason(CardMoveReason::S_REASON_GIVE, player->objectName(), who->objectName(), "nosrenxin", QString());
	DummyCard *handcards = player->wholeHandCards();
	room->obtainCard(who, handcards, reason, false);
	delete handcards;
    room->addPlayerMark(player, "nosrenxinengine");
    if (player->getMark("nosrenxinengine") > 0) {
		player->turnOver();
		room->recover(who, RecoverStruct(player));
        room->removePlayerMark(player, "nosrenxinengine");
    }
}

class NosRenxin: public ZeroCardViewAsSkill {
public:
    NosRenxin(): ZeroCardViewAsSkill("nosrenxin") {
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "peach" && !player->isKongcheng();
    }

    virtual const Card *viewAs() const{
        return new NosRenxinCard;
    }
};

class NosDanshou: public TriggerSkill {
public:
    NosDanshou(): TriggerSkill("nosdanshou") {
        events << Damage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (room->askForSkillInvoke(player, objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				player->drawCards(1, objectName());
				ServerPlayer *current = room->getCurrent();
				if (current && current->isAlive() && current->getPhase() != Player::NotActive) {
					LogMessage log;
					log.type = "#SkipAllPhase";
					log.from = current;
					room->sendLog(log);
				}
				room->removePlayerMark(player, objectName()+"engine");
				throw TurnBroken;
			}
        }
        return false;
    }
};

class NosJuece: public TriggerSkill {
public:
    NosJuece(): TriggerSkill("nosjuece") {
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (player->getPhase() != Player::NotActive && move.from && move.from_places.contains(Player::PlaceHand)
            && move.is_last_handcard) {
            ServerPlayer *from = (ServerPlayer *)move.from;
            if (from->getHp() > 0 && room->askForSkillInvoke(player, objectName(), data)) {
                room->broadcastSkillInvoke(objectName());
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), from->objectName());
				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					room->damage(DamageStruct(objectName(), player, from));
					room->removePlayerMark(player, objectName()+"engine");
				}
            }
        }
        return false;
    }
};

class NosMieji: public TargetModSkill {
public:
    NosMieji(): TargetModSkill("#nosmieji") {
		frequency = NotCompulsory;
        pattern = "SingleTargetTrick|black"; // deal with Ex Nihilo and Collateral later
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const{
        if (from->hasSkill("nosmieji"))
            return 1;
        else
            return 0;
    }
};

class NosMiejiViewAsSkill: public ZeroCardViewAsSkill {
public:
    NosMiejiViewAsSkill(): ZeroCardViewAsSkill("nosmieji") {
        response_pattern = "@@nosmieji";
    }

    virtual const Card *viewAs() const{
        return new ExtraCollateralCard;
    }
};

class NosMiejiForExNihiloAndCollateral: public TriggerSkill {
public:
    NosMiejiForExNihiloAndCollateral(): TriggerSkill("nosmieji") {
        events << PreCardUsed;
		frequency = NotCompulsory;
        view_as_skill = new NosMiejiViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isBlack() && (use.card->isKindOf("ExNihilo") || use.card->isKindOf("Collateral"))) {
            QList<ServerPlayer *> targets;
            ServerPlayer *extra = NULL;
            if (use.card->isKindOf("ExNihilo")) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (!use.to.contains(p) && !room->isProhibited(player, p, use.card))
                        targets << p;
                }
                if (targets.isEmpty()) return false;
                extra = room->askForPlayerChosen(player, targets, objectName(), "@qiaoshui-add:::" + use.card->objectName(), true);
            } else if (use.card->isKindOf("Collateral")) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (use.to.contains(p) || room->isProhibited(player, p, use.card)) continue;
                    if (use.card->targetFilter(QList<const Player *>(), p, player))
                        targets << p;
                }
                if (targets.isEmpty()) return false;

                QStringList tos;
                foreach (ServerPlayer *t, use.to)
                    tos.append(t->objectName());
                room->setPlayerProperty(player, "extra_collateral", use.card->toString());
                room->setPlayerProperty(player, "extra_collateral_current_targets", tos.join("+"));
                bool used = room->askForUseCard(player, "@@nosmieji", "@qiaoshui-add:::collateral");
                room->setPlayerProperty(player, "extra_collateral", QString());
                room->setPlayerProperty(player, "extra_collateral_current_targets", QString());
                if (!used) return false;
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->hasFlag("ExtraCollateralTarget")) {
                        p->setFlags("-ExtraCollateralTarget");
                        extra = p;
                        break;
                    }
                }
            }
            if (!extra) return false;
            room->broadcastSkillInvoke(objectName());
            use.to.append(extra);
            room->sortByActionOrder(use.to);
            data = QVariant::fromValue(use);

            LogMessage log;
            log.type = "#QiaoshuiAdd";
            log.from = player;
            log.to << extra;
            log.arg = use.card->objectName();
            log.arg2 = objectName();
            room->sendLog(log);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), extra->objectName());

            if (use.card->isKindOf("Collateral")) {
                ServerPlayer *victim = extra->tag["collateralVictim"].value<ServerPlayer *>();
                if (victim) {
                    LogMessage log;
                    log.type = "#CollateralSlash";
                    log.from = player;
                    log.to << victim;
                    room->sendLog(log);
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, extra->objectName(), victim->objectName());
                }
            }
        }
        return false;
    }
};

class NosMiejiEffect: public TriggerSkill {
public:
    NosMiejiEffect(): TriggerSkill("#nosmieji-effect") {
        events << PreCardUsed;
		frequency = NotCompulsory;
    }

    virtual int getPriority(TriggerEvent) const{
        return 6;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("SingleTargetTrick") && !use.card->targetFixed() && use.to.length() > 1
            && use.card->isBlack() && use.from->hasSkill("nosmieji"))
            room->broadcastSkillInvoke("nosmieji");
			room->addPlayerMark(use.from, objectName()+"engine");
			if (use.from->getMark(objectName()+"engine") > 0) {
				room->removePlayerMark(use.from, objectName()+"engine");
			}
        return false;
    }
};

class NosFencheng: public ZeroCardViewAsSkill {
public:
    NosFencheng(): ZeroCardViewAsSkill("nosfencheng") {
        frequency = Limited;
        limit_mark = "@nosburn";
    }

    virtual const Card *viewAs() const{
        return new NosFenchengCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@nosburn") >= 1;
    }
};

NosFenchengCard::NosFenchengCard() {
    mute = true;
    target_fixed = true;
}

void NosFenchengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->removePlayerMark(source, "@nosburn");
    room->broadcastSkillInvoke("fencheng");
    //room->doLightbox("$NosFenchengAnimate", 3000);

    QList<ServerPlayer *> players = room->getOtherPlayers(source);
    foreach (ServerPlayer *p, players) {
		room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), p->objectName());
    }
    room->addPlayerMark(source, "nosfenchengengine");
    if (source->getMark("nosfenchengengine") > 0) {
    source->setFlags("NosFenchengUsing");
		try {
			foreach (ServerPlayer *player, players) {
				if (player->isAlive()) {
					room->cardEffect(this, source, player);
					room->getThread()->delay();
				}
			}
			source->setFlags("-NosFenchengUsing");
			room->removePlayerMark(source, "nosfenchengengine");
		}
		catch (TriggerEvent triggerEvent) {
			if (triggerEvent == TurnBroken || triggerEvent == StageChange)
				source->setFlags("-NosFenchengUsing");
			room->removePlayerMark(source, "nosfenchengengine");
			throw triggerEvent;
		}
    }
}

void NosFenchengCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    int length = qMax(1, effect.to->getEquips().length());
    if (!effect.to->canDiscard(effect.to, "he") || !room->askForDiscard(effect.to, "nosfencheng", length, length, true, true))
        room->damage(DamageStruct("nosfencheng", effect.from, effect.to, 1, DamageStruct::Fire));
}

class NosZhuikong: public TriggerSkill {
public:
    NosZhuikong(): TriggerSkill("noszhuikong") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() != Player::RoundStart || player->isKongcheng())
            return false;

        bool skip = false;
        foreach (ServerPlayer *fuhuanghou, room->getAllPlayers()) {
            if (TriggerSkill::triggerable(fuhuanghou)
                && player != fuhuanghou && fuhuanghou->isWounded() && !fuhuanghou->isKongcheng()
                && room->askForSkillInvoke(fuhuanghou, objectName())) {
                room->broadcastSkillInvoke(objectName());
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, fuhuanghou->objectName(), player->objectName());
				room->addPlayerMark(fuhuanghou, objectName()+"engine");
				if (fuhuanghou->getMark(objectName()+"engine") > 0) {
					if (fuhuanghou->pindian(player, objectName(), NULL)) {
						if (!skip) {
							player->skip(Player::Play);
							skip = true;
						}
					} else {
						room->setFixedDistance(player, fuhuanghou, 1);
						QVariantList zhuikonglist = player->tag[objectName()].toList();
						zhuikonglist.append(QVariant::fromValue(fuhuanghou));
						player->tag[objectName()] = QVariant::fromValue(zhuikonglist);
					}
					room->removePlayerMark(fuhuanghou, objectName()+"engine");
				}
            }
        }
        return false;
    }
};

class NosZhuikongClear: public TriggerSkill {
public:
    NosZhuikongClear(): TriggerSkill("#noszhuikong-clear") {
        events << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return false;

        QVariantList zhuikonglist = player->tag["noszhuikong"].toList();
        if (zhuikonglist.isEmpty()) return false;
        foreach (QVariant p, zhuikonglist) {
            ServerPlayer *fuhuanghou = p.value<ServerPlayer *>();
            room->removeFixedDistance(player, fuhuanghou, 1);
        }
        player->tag.remove("noszhuikong");
        return false;
    }
};

class NosQiuyuan: public TriggerSkill {
public:
    NosQiuyuan(): TriggerSkill("nosqiuyuan") {
        events << TargetConfirming;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (!p->isKongcheng() && p != use.from)
                    targets << p;
            }
            if (targets.isEmpty()) return false;
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "nosqiuyuan-invoke", true, true);
            if (target) {
                if (target->getGeneralName().contains("fuwan") || target->getGeneral2Name().contains("fuwan"))
                    room->broadcastSkillInvoke("qiuyuan", 2);
                else
                    room->broadcastSkillInvoke("qiuyuan", 1);
                const Card *card = NULL;
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
                if (target->getHandcardNum() > 1) {
                    card = room->askForCard(target, ".!", "@nosqiuyuan-give:" + player->objectName(), data, Card::MethodNone);
                    if (!card)
                        card = target->getHandcards().at(qrand() % target->getHandcardNum());
                } else {
                    Q_ASSERT(target->getHandcardNum() == 1);
                    card = target->getHandcards().first();
                }
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), player->objectName(), "nosqiuyuan", QString());
                room->obtainCard(player, card, reason);
                room->showCard(player, card->getEffectiveId());
                if (!card->isKindOf("Jink")) {
                    if (use.from->canSlash(target, use.card, false)) {
                        LogMessage log;
                        log.type = "#BecomeTarget";
                        log.from = target;
                        log.card_str = use.card->toString();
                        room->sendLog(log);

                        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());

                        use.to.append(target);
                        room->sortByActionOrder(use.to);
                        data = QVariant::fromValue(use);
                        room->getThread()->trigger(TargetConfirming, room, target, data);
                    }
                }
				room->removePlayerMark(player, objectName()+"engine");
			}
            }
        }
        return false;
    }
};

class NosZhenggong: public MasochismSkill {
public:
    NosZhenggong(): MasochismSkill("noszhenggong") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getMark("nosbaijiang") == 0;
    }

    virtual void onDamaged(ServerPlayer *zhonghui, const DamageStruct &damage) const{
        if (damage.from && damage.from->hasEquip()) {
            QVariant data = QVariant::fromValue(damage.from);
            if (!zhonghui->askForSkillInvoke(objectName(), data))
                return;

            Room *room = zhonghui->getRoom();
            room->broadcastSkillInvoke(objectName());
            int equip = room->askForCardChosen(zhonghui, damage.from, "e", objectName());
            const Card *card = Sanguosha->getCard(equip);

            int equip_index = -1;
            const EquipCard *equipcard = qobject_cast<const EquipCard *>(card->getRealCard());
            equip_index = static_cast<int>(equipcard->location());

            QList<CardsMoveStruct> exchangeMove;
            CardsMoveStruct move1(equip, zhonghui, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_ROB, zhonghui->objectName()));
            exchangeMove.push_back(move1);
            if (zhonghui->getEquip(equip_index) != NULL) {
                CardsMoveStruct move2(zhonghui->getEquip(equip_index)->getId(), NULL, Player::DiscardPile,
                                      CardMoveReason(CardMoveReason::S_REASON_CHANGE_EQUIP, zhonghui->objectName()));
                exchangeMove.push_back(move2);
            }
            room->moveCardsAtomic(exchangeMove, true);
        }
    }
};

class NosQuanji: public TriggerSkill {
public:
    NosQuanji(): TriggerSkill("nosquanji") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() != Player::RoundStart || player->isKongcheng())
            return false;

        bool skip = false;
        foreach (ServerPlayer *zhonghui, room->getAllPlayers()) {
            if (!TriggerSkill::triggerable(zhonghui) || zhonghui == player || zhonghui->isKongcheng()
                || zhonghui->getMark("nosbaijiang") > 0 || player->isKongcheng())
                continue;

            if (room->askForSkillInvoke(zhonghui, "nosquanji")) {
                room->broadcastSkillInvoke(objectName(), 1);
                if (zhonghui->pindian(player, objectName(), NULL)) {
                    if (!skip) {
                        room->broadcastSkillInvoke(objectName(), 2);
                        player->skip(Player::Start);
                        player->skip(Player::Judge);
                        skip = true;
                    } else {
                        room->broadcastSkillInvoke(objectName(), 3);
                    }
                }
            }
        }
        return skip;
    }
};

class NosBaijiang: public PhaseChangeSkill {
public:
    NosBaijiang(): PhaseChangeSkill("nosbaijiang") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getMark("nosbaijiang") == 0
               && target->getPhase() == Player::Start
               && target->getEquips().length() >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const{
        Room *room = zhonghui->getRoom();
        room->notifySkillInvoked(zhonghui, objectName());

        LogMessage log;
        log.type = "#NosBaijiangWake";
        log.from = zhonghui;
        log.arg = QString::number(zhonghui->getEquips().length());
        log.arg2 = objectName();
        room->sendLog(log);
        room->broadcastSkillInvoke(objectName());
        room->doLightbox("$NosBaijiangAnimate", 5000);

        room->setPlayerMark(zhonghui, "nosbaijiang", 1);
        if (room->changeMaxHpForAwakenSkill(zhonghui, 1)) {
            room->recover(zhonghui, RecoverStruct(zhonghui));
            if (zhonghui->getMark("nosbaijiang") == 1)
                room->handleAcquireDetachSkills(zhonghui, "-noszhenggong|-nosquanji|nosyexin");
        }

        return false;
    }
};

NosYexinCard::NosYexinCard() {
    target_fixed = true;
}

void NosYexinCard::onUse(Room *, const CardUseStruct &card_use) const{
    ServerPlayer *zhonghui = card_use.from;

    QList<int> powers = zhonghui->getPile("nospower");
    if (powers.isEmpty())
        return;
    zhonghui->exchangeFreelyFromPrivatePile("nosyexin", "nospower");
}

class NosYexinViewAsSkill: public ZeroCardViewAsSkill {
public:
    NosYexinViewAsSkill(): ZeroCardViewAsSkill("nosyexin") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->getPile("nospower").isEmpty() && !player->hasUsed("NosYexinCard");
    }

    virtual const Card *viewAs() const{
        return new NosYexinCard;
    }
};

class NosYexin: public TriggerSkill {
public:
    NosYexin(): TriggerSkill("nosyexin") {
        events << Damage << Damaged;
        view_as_skill = new NosYexinViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *zhonghui, QVariant &) const{
        if (!zhonghui->askForSkillInvoke(objectName()))
            return false;
        room->broadcastSkillInvoke(objectName(), 1);
        zhonghui->addToPile("nospower", room->drawCard());

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return 2;
    }
};

class NosPaiyi: public PhaseChangeSkill {
public:
    NosPaiyi(): PhaseChangeSkill("nospaiyi") {
        _m_place["Judging"] = Player::PlaceDelayedTrick;
        _m_place["Equip"] = Player::PlaceEquip;
        _m_place["Hand"] = Player::PlaceHand;
    }

    QString getPlace(Room *room, ServerPlayer *player, QStringList places) const{
        if (places.length() > 0) {
            QString place = room->askForChoice(player, "nospaiyi", places.join("+"));
            return place;
        }
        return QString();
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const{
        if (zhonghui->getPhase() != Player::Finish || zhonghui->getPile("nospower").isEmpty())
            return false;

        Room *room = zhonghui->getRoom();
        QList<int> powers = zhonghui->getPile("nospower");
        if (powers.isEmpty() || !room->askForSkillInvoke(zhonghui, objectName()))
            return false;
        QStringList places;
        places << "Hand";

        room->fillAG(powers, zhonghui);
        int power = room->askForAG(zhonghui, powers, false, "nospaiyi");
        room->clearAG(zhonghui);

        if (power == -1)
            power = powers.first();

        const Card *card = Sanguosha->getCard(power);

        ServerPlayer *target = room->askForPlayerChosen(zhonghui, room->getAlivePlayers(), "nospaiyi", "@nospaiyi-to:::" + card->objectName());
        CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, zhonghui->objectName(), "nospaiyi", QString());

        if (card->isKindOf("DelayedTrick")) {
            if (!zhonghui->isProhibited(target, card) && !target->containsTrick(card->objectName()))
                places << "Judging";
            room->moveCardTo(card, zhonghui, target, _m_place[getPlace(room, zhonghui, places)], reason, true);
        } else if (card->isKindOf("EquipCard")) {
            const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
            if (!target->getEquip(equip->location()))
                places << "Equip";
            room->moveCardTo(card, zhonghui, target, _m_place[getPlace(room, zhonghui, places)], reason, true);
        } else
            room->moveCardTo(card, zhonghui, target, _m_place[getPlace(room, zhonghui, places)], reason, true);

        int index = 1;
        if (target != zhonghui) {
            ++index;
            room->drawCards(zhonghui, 1, objectName());
        }
        room->broadcastSkillInvoke(objectName(), index);

        return false;
    }

private:
    QMap<QString, Player::Place> _m_place;
};

class NosZili: public PhaseChangeSkill {
public:
    NosZili(): PhaseChangeSkill("noszili") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getMark("noszili") == 0
               && target->getPhase() == Player::Start
               && target->getPile("nospower").length() >= 4;
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const{
        Room *room = zhonghui->getRoom();
        room->notifySkillInvoked(zhonghui, objectName());

        LogMessage log;
        log.type = "#NosZiliWake";
        log.from = zhonghui;
        log.arg = QString::number(zhonghui->getPile("nospower").length());
        log.arg2 = objectName();
        room->sendLog(log);
        room->broadcastSkillInvoke(objectName());
        room->doLightbox("$NosZiliAnimate", 5000);

        room->setPlayerMark(zhonghui, "noszili", 1);
        if (room->changeMaxHpForAwakenSkill(zhonghui) && zhonghui->getMark("noszili") == 1)
            room->acquireSkill(zhonghui, "nospaiyi");

        return false;
    }
};

class NosGuixin: public PhaseChangeSkill {
public:
    NosGuixin(): PhaseChangeSkill("nosguixin") {
    }

    virtual bool onPhaseChange(ServerPlayer *weiwudi) const{
        if (weiwudi->getPhase() != Player::Finish)
            return false;

        Room *room = weiwudi->getRoom();
        if (!room->askForSkillInvoke(weiwudi, objectName()))
            return false;

        QString choice = room->askForChoice(weiwudi, objectName(), "modify+obtain");

        int index = qrand() % 2 + 1;

        if (choice == "modify") {
            ServerPlayer *to_modify = room->askForPlayerChosen(weiwudi, room->getOtherPlayers(weiwudi), objectName());
            QStringList kingdomList = Sanguosha->getKingdoms();
            kingdomList.removeOne("god");
            QString old_kingdom = to_modify->getKingdom();
            kingdomList.removeOne(old_kingdom);
            if (kingdomList.isEmpty()) return false;
            QString kingdom = room->askForChoice(weiwudi, "nosguixin_kingdom", kingdomList.join("+"), QVariant::fromValue(to_modify));
            room->setPlayerProperty(to_modify, "kingdom", kingdom);

            room->broadcastSkillInvoke(objectName(), index);

            LogMessage log;
            log.type = "#ChangeKingdom";
            log.from = weiwudi;
            log.to << to_modify;
            log.arg = old_kingdom;
            log.arg2 = kingdom;
            room->sendLog(log);
        } else if (choice == "obtain") {
            room->broadcastSkillInvoke(objectName(), index + 2);
            QStringList lords = Sanguosha->getLords();
            foreach (ServerPlayer *player, room->getAlivePlayers()) {
                QString name = player->getGeneralName();
                if (Sanguosha->isGeneralHidden(name)) {
                    QString fname = Sanguosha->findConvertFrom(name);
                    if (!fname.isEmpty()) name = fname;
                }
                lords.removeOne(name);

                if (!player->getGeneral2()) continue;

                name = player->getGeneral2Name();
                if (Sanguosha->isGeneralHidden(name)) {
                    QString fname = Sanguosha->findConvertFrom(name);
                    if (!fname.isEmpty()) name = fname;
                }
                lords.removeOne(name);
            }

            QStringList lord_skills;
            foreach (QString lord, lords) {
                const General *general = Sanguosha->getGeneral(lord);
                QList<const Skill *> skills = general->findChildren<const Skill *>();
                foreach (const Skill *skill, skills) {
                    if (skill->isLordSkill() && !weiwudi->hasSkill(skill->objectName()))
                        lord_skills << skill->objectName();
                }
            }

            if (!lord_skills.isEmpty()) {
                QString skill_name = room->askForChoice(weiwudi, "nosguixin_lordskills", lord_skills.join("+"));

                const Skill *skill = Sanguosha->getSkill(skill_name);
                room->acquireSkill(weiwudi, skill);
            }
        }
        return false;
    }
};

// old stantard generals

class NosJianxiong: public MasochismSkill {
public:
    NosJianxiong(): MasochismSkill("nosjianxiong") {
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const{
        Room *room = player->getRoom();
        const Card *card = damage.card;
        if (!card) return;

        QList<int> ids;
        if (card->isVirtualCard())
            ids = card->getSubcards();
        else
            ids << card->getEffectiveId();

        if (ids.isEmpty()) return;
        foreach (int id, ids) {
            if (room->getCardPlace(id) != Player::PlaceTable) return;
        }
        QVariant data = QVariant::fromValue(damage);
        if (room->askForSkillInvoke(player, objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				player->obtainCard(card);
				room->removePlayerMark(player, objectName()+"engine");
			}
        }
    }
};

class NosFankui: public MasochismSkill {
public:
    NosFankui(): MasochismSkill("nosfankui") {
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const{
        ServerPlayer *from = damage.from;
        Room *room = player->getRoom();
        QVariant data = QVariant::fromValue(from);
        if (from && !from->isNude() && room->askForSkillInvoke(player, "nosfankui", data)) {
            room->broadcastSkillInvoke(objectName());
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), from->objectName());
			if (from == player && player->getCards("ej").isEmpty()) return;
            int card_id = room->askForCardChosen(player, from, "he", "nosfankui");
            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
            room->obtainCard(player, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);
        }
    }
};

class NosGuicai: public TriggerSkill {
public:
    NosGuicai(): TriggerSkill("nosguicai") {
        events << AskForRetrial;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->isKongcheng())
            return false;

        JudgeStruct *judge = data.value<JudgeStruct *>();

        QStringList prompt_list;
        prompt_list << "@nosguicai-card" << judge->who->objectName()
            << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");
        bool forced = false;
        if (player->getMark("JilveEvent") == int(AskForRetrial))
            forced = true;
        const Card *card = room->askForCard(player, forced ? ".!" : "." , prompt, data, Card::MethodResponse, judge->who, true);
        if (card) {
            if (player->hasInnateSkill("guicai") && !player->hasSkill("jilve") && !Config.value("simayi_down", true).toBool())
                room->broadcastSkillInvoke(objectName());
            else
                room->broadcastSkillInvoke("jilve", 1);
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				room->retrial(card, player, judge, objectName());
				room->removePlayerMark(player, objectName()+"engine");
			}
        }

        return false;
    }
};

class NosGanglie: public MasochismSkill {
public:
    NosGanglie(): MasochismSkill("nosganglie") {
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const{
        ServerPlayer *from = damage.from;
        Room *room = player->getRoom();
        QVariant data = QVariant::fromValue(damage);

        if (room->askForSkillInvoke(player, "nosganglie", data)) {
            room->broadcastSkillInvoke("nosganglie");
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), from->objectName());

			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				JudgeStruct judge;
				judge.pattern = ".|heart";
				judge.good = false;
				judge.reason = objectName();
				judge.who = player;

				room->judge(judge);
				if (!from || from->isDead()) return;
				if (judge.isGood()) {
					if (from->getHandcardNum() < 2 || !room->askForDiscard(from, objectName(), 2, 2, true))
						room->damage(DamageStruct(objectName(), player, from));
				}
				room->removePlayerMark(player, objectName()+"engine");
			}
        }
    }
};

NosTuxiCard::NosTuxiCard() {
}

bool NosTuxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (targets.length() >= 2 || to_select == Self)
        return false;

    return !to_select->isKongcheng();
}

void NosTuxiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->addPlayerMark(effect.from, "engine");
    if (effect.from->isAlive() && !effect.to->isKongcheng()) {
		if (effect.from->getMark("engine") > 0) {
			int card_id = room->askForCardChosen(effect.from, effect.to, "h", "tuxi");
			CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
			room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, false);
			room->removePlayerMark(effect.from, "engine");
		}
    }
}

class NosTuxiViewAsSkill: public ZeroCardViewAsSkill {
public:
    NosTuxiViewAsSkill(): ZeroCardViewAsSkill("nostuxi") {
        response_pattern = "@@nostuxi";
    }

    virtual const Card *viewAs() const{
        return new NosTuxiCard;
    }
};

class NosTuxi: public PhaseChangeSkill {
public:
    NosTuxi(): PhaseChangeSkill("nostuxi") {
        view_as_skill = new NosTuxiViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *zhangliao) const{
        if (zhangliao->getPhase() == Player::Draw) {
            Room *room = zhangliao->getRoom();
            bool can_invoke = false;
            QList<ServerPlayer *> other_players = room->getOtherPlayers(zhangliao);
            foreach (ServerPlayer *player, other_players) {
                if (!player->isKongcheng()) {
                    can_invoke = true;
                    break;
                }
            }
            zhangliao->setFlags("-NosTuxiAudioBroadcast");

            if (can_invoke && room->askForUseCard(zhangliao, "@@nostuxi", "@nostuxi-card"))
                return true;
        }

        return false;
    }
};

class NosLuoyiBuff: public TriggerSkill {
public:
    NosLuoyiBuff(): TriggerSkill("#nosluoyi") {
        events << DamageCaused;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasFlag("nosluoyi") && target->isAlive();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *xuchu, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user) return false;
        const Card *reason = damage.card;
        if (reason && (reason->isKindOf("Slash") || reason->isKindOf("Duel"))) {
            LogMessage log;
            log.type = "#LuoyiBuff";
            log.from = xuchu;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);

            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

class NosLuoyi: public DrawCardsSkill {
public:
    NosLuoyi(): DrawCardsSkill("nosluoyi") {
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        if (n > 0 && room->askForSkillInvoke(player, objectName())) {
            room->broadcastSkillInvoke(objectName());
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				player->setFlags(objectName());
				n = n - 1;
				room->removePlayerMark(player, objectName()+"engine");
			}
        }
        return n;
    }
};

NosYiji::NosYiji(): MasochismSkill("nosyiji") {
    frequency = Frequent;
    n = 2;
}

void NosYiji::onDamaged(ServerPlayer *player, const DamageStruct &damage) const{
    Room *room = player->getRoom();
    int x = damage.damage;
    for (int i = 0; i < x; ++i) {
        if (!player->isAlive() || !room->askForSkillInvoke(player, objectName()))
            return;
        room->broadcastSkillInvoke(objectName());

		room->addPlayerMark(player, objectName()+"engine");
		if (player->getMark(objectName()+"engine") > 0) {
			QList<ServerPlayer *> _player;
			_player.append(player);
			QList<int> yiji_cards = room->getNCards(n, false);

			CardsMoveStruct move(yiji_cards, NULL, player, Player::PlaceTable, Player::PlaceHand,
				CardMoveReason(CardMoveReason::S_REASON_PREVIEW, player->objectName(), objectName(), QString()));
			QList<CardsMoveStruct> moves;
			moves.append(move);
			room->notifyMoveCards(true, moves, false, _player);
			room->notifyMoveCards(false, moves, false, _player);

			QList<int> origin_yiji = yiji_cards;
			while (room->askForYiji(player, yiji_cards, objectName(), true, false, true, -1, room->getAlivePlayers())) {
				CardsMoveStruct move(QList<int>(), player, NULL, Player::PlaceHand, Player::PlaceTable,
					CardMoveReason(CardMoveReason::S_REASON_PREVIEW, player->objectName(), objectName(), QString()));
				foreach (int id, origin_yiji) {
					if (room->getCardPlace(id) != Player::DrawPile) {
						move.card_ids << id;
						yiji_cards.removeOne(id);
					}
				}
				origin_yiji = yiji_cards;
				QList<CardsMoveStruct> moves;
				moves.append(move);
				room->notifyMoveCards(true, moves, false, _player);
				room->notifyMoveCards(false, moves, false, _player);
				if (!player->isAlive())
					return;
			}

			if (!yiji_cards.isEmpty()) {
				CardsMoveStruct move(yiji_cards, player, NULL, Player::PlaceHand, Player::PlaceTable,
					CardMoveReason(CardMoveReason::S_REASON_PREVIEW, player->objectName(), objectName(), QString()));
				QList<CardsMoveStruct> moves;
				moves.append(move);
				room->notifyMoveCards(true, moves, false, _player);
				room->notifyMoveCards(false, moves, false, _player);

				DummyCard *dummy = new DummyCard(yiji_cards);
				player->obtainCard(dummy, false);
				delete dummy;
			}
			room->removePlayerMark(player, objectName()+"engine");
		}
    }
}

NosRendeCard::NosRendeCard() {
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void NosRendeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();

    QDateTime dtbefore = source->tag.value("nosrende", QDateTime(QDate::currentDate(), QTime(0, 0, 0))).toDateTime();
    QDateTime dtafter = QDateTime::currentDateTime();

    if (dtbefore.secsTo(dtafter) > 3 * Config.AIDelay / 1000)
        room->broadcastSkillInvoke("rende");

    room->addPlayerMark(source, "nosrendeengine");
    if (source->getMark("nosrendeengine") > 0) {
		source->tag["nosrende"] = QDateTime::currentDateTime();
		CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "nosrende", QString());
		room->obtainCard(target, this, reason, false);

		int old_value = source->getMark("nosrende");
		int new_value = old_value + subcards.length();
		room->setPlayerMark(source, "nosrende", new_value);

		if (old_value < 2 && new_value >= 2)
			room->recover(source, RecoverStruct(source));
        room->removePlayerMark(source, "nosrendeengine");
    }
}

class NosRendeViewAsSkill: public ViewAsSkill {
public:
    NosRendeViewAsSkill(): ViewAsSkill("nosrende") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (ServerInfo.GameMode == "04_1v3" && selected.length() + Self->getMark("nosrende") >= 2)
            return false;
        else
            return !to_select->isEquipped();
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if (ServerInfo.GameMode == "04_1v3" && player->getMark("nosrende") >= 2)
           return false;
        return !player->isKongcheng();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
            return NULL;

        NosRendeCard *rende_card = new NosRendeCard;
        rende_card->addSubcards(cards);
        return rende_card;
    }
};

class NosRende: public TriggerSkill {
public:
    NosRende(): TriggerSkill("nosrende") {
        events << EventPhaseChanging;
        view_as_skill = new NosRendeViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getMark("nosrende") > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return false;
        room->setPlayerMark(player, "nosrende", 0);
        return false;
    }
};

class NosTieji: public TriggerSkill {
public:
    NosTieji(): TriggerSkill("nostieji") {
        events << TargetSpecified;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash"))
            return false;
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *p, use.to) {
            if (!player->isAlive()) break;
            if (player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                room->broadcastSkillInvoke(objectName());
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());

				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					p->setFlags("NosTiejiTarget"); // For AI

					JudgeStruct judge;
					judge.pattern = ".|red";
					judge.good = true;
					judge.reason = objectName();
					judge.who = player;

					try {
						room->judge(judge);
					}
					catch (TriggerEvent triggerEvent) {
						if (triggerEvent == TurnBroken || triggerEvent == StageChange)
							p->setFlags("-NosTiejiTarget");
						throw triggerEvent;
					}

					if (judge.isGood()) {
						LogMessage log;
						log.type = "#NoJink";
						log.from = p;
						room->sendLog(log);
						jink_list.replace(index, QVariant(0));
					}

					p->setFlags("-NosTiejiTarget");
					room->removePlayerMark(player, objectName()+"engine");
				}
            }
            ++index;
        }
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

class NosJizhi: public TriggerSkill {
public:
    NosJizhi(): TriggerSkill("nosjizhi") {
        frequency = Frequent;
        events << CardUsed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();

        if (use.card->isNDTrick() && ((player->getMark("JilveEvent") > 0 && Config.value("simayi_duang", true).toBool()) || room->askForSkillInvoke(player, objectName()))) {
            room->broadcastSkillInvoke(objectName());
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				player->drawCards(1, objectName());
				room->removePlayerMark(player, objectName()+"engine");
			}
        }

        return false;
    }
};

class NosQicai: public TargetModSkill {
public:
    NosQicai(): TargetModSkill("nosqicai") {
        pattern = "TrickCard";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()))
            return 1000;
        else
            return 0;
    }
};

NosKurouCard::NosKurouCard() {
    target_fixed = true;
}

void NosKurouCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->loseHp(source);
    if (source->isAlive()) {
		room->addPlayerMark(source, "noskurouengine");
		if (source->getMark("noskurouengine") > 0) {
			room->drawCards(source, 2, "noskurou");
			room->removePlayerMark(source, "noskurouengine");
		}
	}
}

class NosKurou: public ZeroCardViewAsSkill {
public:
    NosKurou(): ZeroCardViewAsSkill("noskurou") {
    }

    virtual const Card *viewAs() const{
        return new NosKurouCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getHp() > 0;
    }
};

class NosYingzi: public DrawCardsSkill {
public:
    NosYingzi(): DrawCardsSkill("nosyingzi") {
        frequency = Frequent;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        if (room->askForSkillInvoke(player, objectName())) {
            room->broadcastSkillInvoke("nosyingzi");
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				n = n + 1;
				room->removePlayerMark(player, objectName()+"engine");
			}
        }
        return n;
    }
};

NosFanjianCard::NosFanjianCard() {
}

void NosFanjianCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *zhouyu = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = zhouyu->getRoom();

    room->addPlayerMark(zhouyu, "nosfanjianengine");
    if (zhouyu->getMark("nosfanjianengine") > 0) {
		int card_id = zhouyu->getRandomHandCardId();
		const Card *card = Sanguosha->getCard(card_id);
		Card::Suit suit = room->askForSuit(target, "nosfanjian");

		LogMessage log;
		log.type = "#ChooseSuit";
		log.from = target;
		log.arg = Card::Suit2String(suit);
		room->sendLog(log);

		room->getThread()->delay();
		target->obtainCard(card);
		room->showCard(target, card_id);

		if (card->getSuit() != suit)
			room->damage(DamageStruct("nosfanjian", zhouyu, target));
        room->removePlayerMark(zhouyu, "nosfanjianengine");
    }
}

class NosFanjian: public ZeroCardViewAsSkill {
public:
    NosFanjian(): ZeroCardViewAsSkill("nosfanjian") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && !player->hasUsed("NosFanjianCard");
    }

    virtual const Card *viewAs() const{
        return new NosFanjianCard;
    }
};

class NosGuose: public OneCardViewAsSkill {
public:
    NosGuose(): OneCardViewAsSkill("nosguose") {
        filter_pattern = ".|diamond";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Indulgence *indulgence = new Indulgence(originalCard->getSuit(), originalCard->getNumber());
        indulgence->addSubcard(originalCard->getId());
        indulgence->setSkillName(objectName());
        return indulgence;
    }
};

class NosQianxun: public ProhibitSkill {
public:
    NosQianxun(): ProhibitSkill("nosqianxun") {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const{
        return to->hasSkill(objectName()) && (card->isKindOf("Snatch") || card->isKindOf("Indulgence"));
    }
};

class NosLianying: public TriggerSkill {
public:
    NosLianying(): TriggerSkill("noslianying") {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard) {
            if (room->askForSkillInvoke(player, objectName(), data)) {
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

QingnangCard::QingnangCard() {
}

bool QingnangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    return targets.isEmpty() && to_select->isWounded();
}

bool QingnangCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.value(0, Self)->isWounded();
}

void QingnangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.value(0, source);
    room->cardEffect(this, source, target);
}

void QingnangCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->addPlayerMark(effect.from, "qingnangengine");
    if (effect.from->getMark("qingnangengine") > 0) {
		room->recover(effect.to, RecoverStruct(effect.from));
        room->removePlayerMark(effect.from, "qingnangengine");
    }
}

class Qingnang: public OneCardViewAsSkill {
public:
    Qingnang(): OneCardViewAsSkill("qingnang") {
        filter_pattern = ".|.|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "h") && !player->hasUsed("QingnangCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        QingnangCard *qingnang_card = new QingnangCard;
        qingnang_card->addSubcard(originalCard->getId());
        return qingnang_card;
    }
};

NosLijianCard::NosLijianCard(): LijianCard(false) {
}

class NosLijian: public OneCardViewAsSkill {
public:
    NosLijian(): OneCardViewAsSkill("noslijian") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getAliveSiblings().length() > 1
               && player->canDiscard(player, "he") && !player->hasUsed("NosLijianCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        NosLijianCard *lijian_card = new NosLijianCard;
        lijian_card->addSubcard(originalCard->getId());
        return lijian_card;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        return card->isKindOf("Duel") ? 0 : -1;
    }
};

// old wind generals

class NosLeiji: public TriggerSkill {
public:
    NosLeiji(): TriggerSkill("nosleiji") {
        events << CardResponded;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        const Card *card = data.value<CardResponseStruct>().m_card;
        if (card && card->isKindOf("Jink")) {
            ServerPlayer *target = room->askForPlayerChosen(player, Config.value("nos_leiji_down", true).toBool() ? room->getOtherPlayers(player) : room->getAlivePlayers(), objectName(), "leiji-invoke", true, true);
            if (target) {
                room->broadcastSkillInvoke("nosleiji");

				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					JudgeStruct judge;
					judge.pattern = ".|spade";
					judge.good = false;
					judge.negative = true;
					judge.reason = objectName();
					judge.who = target;

					room->judge(judge);

					if (judge.isBad())
						room->damage(DamageStruct(objectName(), player, target, 2, DamageStruct::Thunder));
					room->removePlayerMark(player, objectName()+"engine");
				}
            }
        }
        return false;
    }
};

#include "wind.h"
class NosJushou: public Jushou {
public:
    NosJushou(): Jushou() {
        setObjectName("nosjushou");
    }

    virtual int getJushouDrawNum(ServerPlayer *) const{
        return 3;
    }
};

class NosBuquRemove: public TriggerSkill {
public:
    NosBuquRemove(): TriggerSkill("#nosbuqu-remove") {
        events << HpRecover;
    }

    static void Remove(ServerPlayer *player) {
        Room *room = player->getRoom();
        QList<int> nosbuqu(player->getPile("nostrauma"));

        CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "nosbuqu", QString());
        int need = 1 - player->getHp();
        if (need <= 0) {
            // clear all the buqu cards
            foreach (int card_id, nosbuqu) {
                LogMessage log;
                log.type = "$NosBuquRemove";
                log.from = player;
                log.card_str = Sanguosha->getCard(card_id)->toString();
                room->sendLog(log);

				room->addPlayerMark(player, "nosbuquengine");
				if (player->getMark("nosbuquengine") > 0) {
					room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
					room->removePlayerMark(player, "nosbuquengine");
				}
            }
        } else {
            int to_remove = nosbuqu.length() - need;
            for (int i = 0; i < to_remove; ++i) {
                room->fillAG(nosbuqu);
                int card_id = room->askForAG(player, nosbuqu, false, "nosbuqu");

                LogMessage log;
                log.type = "$NosBuquRemove";
                log.from = player;
                log.card_str = Sanguosha->getCard(card_id)->toString();
                room->sendLog(log);

                nosbuqu.removeOne(card_id);
				room->addPlayerMark(player, "nosbuquengine");
				if (player->getMark("nosbuquengine") > 0) {
					room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
					room->removePlayerMark(player, "nosbuquengine");
				}
                room->clearAG();
            }
        }
    }

    virtual bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &) const{
        if (player->getPile("nostrauma").length() > 0)
            Remove(player);

        return false;
    }
};

class NosBuqu: public TriggerSkill {
public:
    NosBuqu(): TriggerSkill("nosbuqu") {
        events << HpChanged << AskForPeachesDone;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == HpChanged && !data.isNull() && !data.canConvert<RecoverStruct>() && player->getHp() < 1) {
            if (room->askForSkillInvoke(player, objectName(), data)) {
                room->setTag("NosBuqu", player->objectName());
                room->broadcastSkillInvoke("nosbuqu");
				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					const QList<int> &nosbuqu = player->getPile("nostrauma");

					int need = 1 - player->getHp(); // the buqu cards that should be turned over
					int n = need - nosbuqu.length();
					if (n > 0) {
						QList<int> card_ids = room->getNCards(n, false);
						player->addToPile("nostrauma", card_ids);
					}
					const QList<int> &nosbuqunew = player->getPile("nostrauma");
					QList<int> duplicate_numbers;

					QSet<int> numbers;
					foreach (int card_id, nosbuqunew) {
						const Card *card = Sanguosha->getCard(card_id);
						int number = card->getNumber();

						if (numbers.contains(number))
							duplicate_numbers << number;
						else
							numbers << number;
					}

					if (duplicate_numbers.isEmpty()) {
						room->setTag("NosBuqu", QVariant());
						return true;
					}
					room->removePlayerMark(player, objectName()+"engine");
				}
            }
        } else if (triggerEvent == AskForPeachesDone) {
            const QList<int> &nosbuqu = player->getPile("nostrauma");

            if (player->getHp() > 0)
                return false;
            if (room->getTag("NosBuqu").toString() != player->objectName())
                return false;
            room->setTag("NosBuqu", QVariant());

            QList<int> duplicate_numbers;
            QSet<int> numbers;
            foreach (int card_id, nosbuqu) {
                const Card *card = Sanguosha->getCard(card_id);
                int number = card->getNumber();

                if (numbers.contains(number) && !duplicate_numbers.contains(number))
                    duplicate_numbers << number;
                else
                    numbers << number;
            }

            if (duplicate_numbers.isEmpty()) {
                room->broadcastSkillInvoke("nosbuqu");
                room->setPlayerFlag(player, "-Global_Dying");
                return true;
            } else {
                LogMessage log;
                log.type = "#NosBuquDuplicate";
                log.from = player;
                log.arg = QString::number(duplicate_numbers.length());
                room->sendLog(log);

                for (int i = 0; i < duplicate_numbers.length(); ++i) {
                    int number = duplicate_numbers.at(i);

                    LogMessage log;
                    log.type = "#NosBuquDuplicateGroup";
                    log.from = player;
                    log.arg = QString::number(i + 1);
                    if (number == 10)
                        log.arg2 = "10";
                    else {
                        const char *number_string = "-A23456789-JQK";
                        log.arg2 = QString(number_string[number]);
                    }
                    room->sendLog(log);

                    foreach (int card_id, nosbuqu) {
                        const Card *card = Sanguosha->getCard(card_id);
                        if (card->getNumber() == number) {
                            LogMessage log;
                            log.type = "$NosBuquDuplicateItem";
                            log.from = player;
                            log.card_str = QString::number(card_id);
                            room->sendLog(log);
                        }
                    }
                }
            }
        }
        return false;
    }
};

class NosBuquClear: public DetachEffectSkill {
public:
    NosBuquClear(): DetachEffectSkill("nosbuqu") {
    }

    virtual void onSkillDetached(Room *room, ServerPlayer *player) const{
        if (player->getHp() <= 0)
            room->enterDying(player, NULL);
    }
};

NosGuhuoCard::NosGuhuoCard() {
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool NosGuhuoCard::nosguhuo(ServerPlayer *yuji) const{
    Room *room = yuji->getRoom();
    QList<ServerPlayer *> players = room->getOtherPlayers(yuji);
    QSet<ServerPlayer *> questioned;

    QList<int> used_cards;
    QList<CardsMoveStruct> moves;
    foreach (int card_id, getSubcards())
        used_cards << card_id;
    room->setTag("NosGuhuoType", user_string);

    foreach (ServerPlayer *player, players) {
        if (player->getHp() <= 0) {
            LogMessage log;
            log.type = "#GuhuoCannotQuestion";
            log.from = player;
            log.arg = QString::number(player->getHp());
            room->sendLog(log);

            room->setEmotion(player, "no-question");
            continue;
        }

        QString choice = room->askForChoice(player, "nosguhuo", "noquestion+question");
        if (choice == "question") {
            room->setEmotion(player, "question");
            questioned << player;
        } else
            room->setEmotion(player, "no-question");

        LogMessage log;
        log.type = "#GuhuoQuery";
        log.from = player;
        log.arg = choice;

        room->sendLog(log);
    }

    LogMessage log;
    log.type = "$GuhuoResult";
    log.from = yuji;
    log.card_str = QString::number(subcards.first());
    room->sendLog(log);

    bool success = false;
    if (questioned.isEmpty()) {
        success = true;
        foreach (ServerPlayer *player, players)
            room->setEmotion(player, ".");

        CardMoveReason reason(CardMoveReason::S_REASON_USE, yuji->objectName(), QString(), "nosguhuo");
        CardsMoveStruct move(used_cards, yuji, NULL, Player::PlaceUnknown, Player::PlaceTable, reason);
        moves.append(move);
        room->moveCardsAtomic(moves, true);
    } else {
        const Card *card = Sanguosha->getCard(subcards.first());
        bool real;
        if (user_string == "peach+analeptic")
            real = card->objectName() == yuji->tag["NosGuhuoSaveSelf"].toString();
        else if (user_string == "slash")
            real = card->objectName().contains("slash");
        else if (user_string == "normal_slash")
            real = card->objectName() == "slash";
        else
            real = card->match(user_string);

        success = real && card->getSuit() == Card::Heart;
        if (success) {
            CardMoveReason reason(CardMoveReason::S_REASON_USE, yuji->objectName(), QString(), "nosguhuo");
            CardsMoveStruct move(used_cards, yuji, NULL, Player::PlaceUnknown, Player::PlaceTable, reason);
            moves.append(move);
            room->moveCardsAtomic(moves, true);
        } else {
            room->moveCardTo(this, yuji, NULL, Player::DiscardPile,
                             CardMoveReason(CardMoveReason::S_REASON_PUT, yuji->objectName(), QString(), "nosguhuo"), true);
        }
        foreach (ServerPlayer *player, players) {
            room->setEmotion(player, ".");
            if (questioned.contains(player)) {
                if (real)
                    room->loseHp(player);
                else
                    player->drawCards(1, "nosguhuo");
            }
        }
    }
    yuji->tag.remove("NosGuhuoSaveSelf");
    yuji->tag.remove("NosGuhuoSlash");
    return success;
}

bool NosGuhuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    const Card *_card = Self->tag.value("nosguhuo").value<const Card *>();
    if (_card == NULL)
        return false;

    Card *card = Sanguosha->cloneCard(_card);
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool NosGuhuoCard::targetFixed() const{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFixed();
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *_card = Self->tag.value("nosguhuo").value<const Card *>();
    if (_card == NULL)
        return false;

    Card *card = Sanguosha->cloneCard(_card);
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetFixed();
}

bool NosGuhuoCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *_card = Self->tag.value("nosguhuo").value<const Card *>();
    if (_card == NULL)
        return false;

    Card *card = Sanguosha->cloneCard(_card);
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetsFeasible(targets, Self);
}

const Card *NosGuhuoCard::validate(CardUseStruct &card_use) const{
    ServerPlayer *yuji = card_use.from;
    Room *room = yuji->getRoom();

    QString to_nosguhuo = user_string;
    if (user_string == "slash"
        && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        QStringList nosguhuo_list;
        nosguhuo_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            nosguhuo_list << "normal_slash" << "thunder_slash" << "fire_slash";
        to_nosguhuo = room->askForChoice(yuji, "nosguhuo_slash", nosguhuo_list.join("+"));
        yuji->tag["NosGuhuoSlash"] = QVariant(to_nosguhuo);
    }
    room->broadcastSkillInvoke("nosguhuo");

    LogMessage log;
    log.type = card_use.to.isEmpty() ? "#GuhuoNoTarget" : "#Guhuo";
    log.from = yuji;
    log.to = card_use.to;
    log.arg = to_nosguhuo;
    log.arg2 = "nosguhuo";

    room->sendLog(log);

    if (nosguhuo(card_use.from)) {
        const Card *card = Sanguosha->getCard(subcards.first());
        QString user_str;
        if (to_nosguhuo == "slash") {
            if (card->isKindOf("Slash"))
                user_str = card->objectName();
            else
                user_str = "slash";
        } else if (to_nosguhuo == "normal_slash")
            user_str = "slash";
        else
            user_str = to_nosguhuo;
        Card *use_card = Sanguosha->cloneCard(user_str, card->getSuit(), card->getNumber());
        use_card->setSkillName("nosguhuo");
        use_card->addSubcard(subcards.first());
        use_card->deleteLater();
        return use_card;
    } else
        return NULL;
}

const Card *NosGuhuoCard::validateInResponse(ServerPlayer *yuji) const{
    Room *room = yuji->getRoom();
    room->broadcastSkillInvoke("nosguhuo");

    QString to_nosguhuo;
    if (user_string == "peach+analeptic") {
        QStringList nosguhuo_list;
        nosguhuo_list << "peach";
        if (!Config.BanPackages.contains("maneuvering"))
            nosguhuo_list << "analeptic";
        to_nosguhuo = room->askForChoice(yuji, "nosguhuo_saveself", nosguhuo_list.join("+"));
        yuji->tag["NosGuhuoSaveSelf"] = QVariant(to_nosguhuo);
    } else if (user_string == "slash") {
        QStringList nosguhuo_list;
        nosguhuo_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            nosguhuo_list << "normal_slash" << "thunder_slash" << "fire_slash";
        to_nosguhuo = room->askForChoice(yuji, "nosguhuo_slash", nosguhuo_list.join("+"));
        yuji->tag["NosGuhuoSlash"] = QVariant(to_nosguhuo);
    }
    else
        to_nosguhuo = user_string;

    LogMessage log;
    log.type = "#GuhuoNoTarget";
    log.from = yuji;
    log.arg = to_nosguhuo;
    log.arg2 = "nosguhuo";
    room->sendLog(log);

    if (nosguhuo(yuji)) {
        const Card *card = Sanguosha->getCard(subcards.first());
        QString user_str;
        if (to_nosguhuo == "slash") {
            if (card->isKindOf("Slash"))
                user_str = card->objectName();
            else
                user_str = "slash";
        } else if (to_nosguhuo == "normal_slash")
            user_str = "slash";
        else
            user_str = to_nosguhuo;
        Card *use_card = Sanguosha->cloneCard(user_str, card->getSuit(), card->getNumber());
        use_card->setSkillName("nosguhuo");
        use_card->addSubcard(subcards.first());
        use_card->deleteLater();
        return use_card;
    } else
        return NULL;
}

class NosGuhuo: public OneCardViewAsSkill {
public:
    NosGuhuo(): OneCardViewAsSkill("nosguhuo") {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (player->isKongcheng() || pattern.startsWith(".") || pattern.startsWith("@")) return false;
        if (pattern == "peach" && player->hasFlag("Global_PreventPeach")) return false;
        for (int i = 0; i < pattern.length(); ++i) {
            QChar ch = pattern[i];
            if (ch.isUpper() || ch.isDigit()) return false; // This is an extremely dirty hack!! For we need to prevent patterns like 'BasicCard'
        }
        return true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE
            || Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            NosGuhuoCard *card = new NosGuhuoCard;
            card->setUserString(Sanguosha->getCurrentCardUsePattern());
            card->addSubcard(originalCard);
            return card;
        }

        const Card *c = Self->tag.value("nosguhuo").value<const Card *>();
        if (c) {
            NosGuhuoCard *card = new NosGuhuoCard;
            if (!c->objectName().contains("slash"))
                card->setUserString(c->objectName());
            else
                card->setUserString(Self->tag["NosGuhuoSlash"].toString());
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }

    virtual QDialog *getDialog() const{
        return GuhuoDialog::getInstance("nosguhuo");
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        if (!card->isKindOf("NosGuhuoCard"))
            return -2;
        else
            return -1;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        return !player->isKongcheng();
    }
};

NostalGeneralPackage::NostalGeneralPackage()
    : Package("nostal_general")
{
    General *nos_zhonghui = new General(this, "nos_zhonghui", "wei", 3);
    nos_zhonghui->addSkill(new NosZhenggong);
    nos_zhonghui->addSkill(new NosQuanji);
    nos_zhonghui->addSkill(new NosBaijiang);
    nos_zhonghui->addSkill(new NosZili);
    nos_zhonghui->addRelateSkill("nosyexin");
    nos_zhonghui->addRelateSkill("#nosyexin-fake-move");
    related_skills.insertMulti("nosyexin", "#nosyexin-fake-move");
    nos_zhonghui->addRelateSkill("nospaiyi");

    General *nos_shencaocao = new General(this, "nos_shencaocao", "god", 3);
    nos_shencaocao->addSkill(new NosGuixin);
    nos_shencaocao->addSkill("feiying");

    addMetaObject<NosYexinCard>();

    skills << new NosYexin << new FakeMoveSkill("nosyexin") << new NosPaiyi;
}

NostalStandardPackage::NostalStandardPackage()
    : Package("nostal_standard")
{
    General *nos_caocao = new General(this, "nos_caocao$", "wei");
    nos_caocao->addSkill(new NosJianxiong);
    nos_caocao->addSkill("hujia");

    General *nos_simayi = new General(this, "nos_simayi", "wei", 3);
    nos_simayi->addSkill(new NosFankui);
    nos_simayi->addSkill(new NosGuicai);

    General *nos_xiahoudun = new General(this, "nos_xiahoudun", "wei");
    nos_xiahoudun->addSkill(new NosGanglie);

    General *nos_zhangliao = new General(this, "nos_zhangliao", "wei");
    nos_zhangliao->addSkill(new NosTuxi);

    General *nos_xuchu = new General(this, "nos_xuchu", "wei");
    nos_xuchu->addSkill(new NosLuoyi);
    nos_xuchu->addSkill(new NosLuoyiBuff);
    related_skills.insertMulti("nosluoyi", "#nosluoyi");

    General *nos_guojia = new General(this, "nos_guojia", "wei", 3);
    nos_guojia->addSkill("tiandu");
    nos_guojia->addSkill(new NosYiji);

    General *nos_liubei = new General(this, "nos_liubei$", "shu");
    nos_liubei->addSkill(new NosRende);
    nos_liubei->addSkill("jijiang");

    General *nos_guanyu = new General(this, "nos_guanyu", "shu");
    nos_guanyu->addSkill("wusheng");

    General *nos_zhangfei = new General(this, "nos_zhangfei", "shu");
    nos_zhangfei->addSkill("paoxiao");

    General *nos_zhaoyun = new General(this, "nos_zhaoyun", "shu");
    nos_zhaoyun->addSkill("longdan");

    General *nos_machao = new General(this, "nos_machao", "shu");
    nos_machao->addSkill("mashu");
    nos_machao->addSkill(new NosTieji);

    General *nos_huangyueying = new General(this, "nos_huangyueying", "shu", 3, false);
    nos_huangyueying->addSkill(new NosJizhi);
    nos_huangyueying->addSkill(new NosQicai);

    General *nos_ganning = new General(this, "nos_ganning", "wu");
    nos_ganning->addSkill("qixi");

    General *nos_lvmeng = new General(this, "nos_lvmeng", "wu");
    nos_lvmeng->addSkill("keji");

    General *nos_huanggai = new General(this, "nos_huanggai", "wu");
    nos_huanggai->addSkill(new NosKurou);

    General *nos_zhouyu = new General(this, "nos_zhouyu", "wu", 3);
    nos_zhouyu->addSkill(new NosYingzi);
    nos_zhouyu->addSkill(new NosFanjian);

    General *nos_daqiao = new General(this, "nos_daqiao", "wu", 3, false);
    nos_daqiao->addSkill(new NosGuose);
    nos_daqiao->addSkill("liuli");

    General *nos_luxun = new General(this, "nos_luxun", "wu", 3);
    nos_luxun->addSkill(new NosQianxun);
    nos_luxun->addSkill(new NosLianying);

    General *nos_huatuo = new General(this, "nos_huatuo", "qun", 3);
    nos_huatuo->addSkill(new Qingnang);
    nos_huatuo->addSkill("jijiu");

    General *nos_lvbu = new General(this, "nos_lvbu", "qun");
    nos_lvbu->addSkill("wushuang");

    General *nos_diaochan = new General(this, "nos_diaochan", "qun", 3, false);
    nos_diaochan->addSkill(new NosLijian);
    nos_diaochan->addSkill("biyue");

    addMetaObject<NosTuxiCard>();
    addMetaObject<NosRendeCard>();
    addMetaObject<NosKurouCard>();
    addMetaObject<NosFanjianCard>();
    addMetaObject<NosLijianCard>();
    addMetaObject<QingnangCard>();
}

NostalWindPackage::NostalWindPackage()
    : Package("nostal_wind")
{
    General *nos_caoren = new General(this, "nos_caoren", "wei");
    nos_caoren->addSkill(new NosJushou);

    General *nos_zhoutai = new General(this, "nos_zhoutai", "wu");
    nos_zhoutai->addSkill(new NosBuqu);
    nos_zhoutai->addSkill(new NosBuquRemove);
    nos_zhoutai->addSkill(new NosBuquClear);
    related_skills.insertMulti("nosbuqu", "#nosbuqu-remove");
    related_skills.insertMulti("nosbuqu", "#nosbuqu-clear");

    General *nos_zhangjiao = new General(this, "nos_zhangjiao$", "qun", 3);
    nos_zhangjiao->addSkill(new NosLeiji);
    nos_zhangjiao->addSkill("guidao");
    nos_zhangjiao->addSkill("huangtian");

    General *nos_yuji = new General(this, "nos_yuji", "qun", 3);
    nos_yuji->addSkill(new NosGuhuo);

    addMetaObject<NosGuhuoCard>();
}

NostalYJCMPackage::NostalYJCMPackage()
    : Package("nostal_yjcm")
{
    General *nos_fazheng = new General(this, "nos_fazheng", "shu", 3);
    nos_fazheng->addSkill(new NosEnyuan);
    nos_fazheng->addSkill(new NosXuanhuo);

    General *nos_lingtong = new General(this, "nos_lingtong", "wu");
    nos_lingtong->addSkill(new NosXuanfeng);
    nos_lingtong->addSkill(new SlashNoDistanceLimitSkill("nosxuanfeng"));
    related_skills.insertMulti("nosxuanfeng", "#nosxuanfeng-slash-ndl");

    General *nos_xushu = new General(this, "nos_xushu", "shu", 3);
    nos_xushu->addSkill(new NosWuyan);
    nos_xushu->addSkill(new NosJujian);

    General *nos_zhangchunhua = new General(this, "nos_zhangchunhua", "wei", 3, false);
    nos_zhangchunhua->addSkill("jueqing");
    nos_zhangchunhua->addSkill(new NosShangshi);

    addMetaObject<NosXuanhuoCard>();
    addMetaObject<NosJujianCard>();
}

NostalYJCM2012Package::NostalYJCM2012Package()
    : Package("nostal_yjcm2012")
{
    General *nos_guanxingzhangbao = new General(this, "nos_guanxingzhangbao", "shu");
    nos_guanxingzhangbao->addSkill(new NosFuhun);

    General *nos_handang = new General(this, "nos_handang", "wu");
    nos_handang->addSkill(new NosGongqi);
    nos_handang->addSkill(new NosGongqiTargetMod);
    nos_handang->addSkill(new NosJiefan);
    related_skills.insertMulti("nosgongqi", "#nosgongqi-target");

    General *nos_madai = new General(this, "nos_madai", "shu");
    nos_madai->addSkill("mashu");
    nos_madai->addSkill(new NosQianxi);

    General *nos_wangyi = new General(this, "nos_wangyi", "wei", 3, false);
    nos_wangyi->addSkill(new NosZhenlie);
    nos_wangyi->addSkill(new NosMiji);

    addMetaObject<NosJiefanCard>();
}

NostalYJCM2013Package::NostalYJCM2013Package()
    : Package("nostal_yjcm2013")
{
    General *nos_caochong = new General(this, "nos_caochong", "wei", 3);
    nos_caochong->addSkill(new NosChengxiang);
    nos_caochong->addSkill(new NosRenxin);

    General *nos_fuhuanghou = new General(this, "nos_fuhuanghou", "qun", 3, false);
    nos_fuhuanghou->addSkill(new NosZhuikong);
    nos_fuhuanghou->addSkill(new NosZhuikongClear);
    nos_fuhuanghou->addSkill(new NosQiuyuan);
    related_skills.insertMulti("noszhuikong", "#noszhuikong-clear");

    General *nos_liru = new General(this, "nos_liru", "qun", 3);
    nos_liru->addSkill(new NosJuece);
    nos_liru->addSkill(new NosMieji);
    nos_liru->addSkill(new NosMiejiForExNihiloAndCollateral);
    nos_liru->addSkill(new NosMiejiEffect);
    nos_liru->addSkill(new NosFencheng);
    related_skills.insertMulti("nosmieji", "#nosmieji");
    related_skills.insertMulti("nosmieji", "#nosmieji-effect");

    General *nos_zhuran = new General(this, "nos_zhuran", "wu");
    nos_zhuran->addSkill(new NosDanshou);

    addMetaObject<NosRenxinCard>();
    addMetaObject<NosFenchengCard>();
}

ADD_PACKAGE(Nostalgia)
ADD_PACKAGE(NostalGeneral)
ADD_PACKAGE(NostalWind)
ADD_PACKAGE(NostalStandard)
ADD_PACKAGE(NostalYJCM)
ADD_PACKAGE(NostalYJCM2012)
ADD_PACKAGE(NostalYJCM2013)
