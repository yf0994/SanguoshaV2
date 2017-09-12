#include "hegemony.h"
#include "skill.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "room.h"
#include "standard-skillcards.h"
#include "settings.h"

DuoshiCard::DuoshiCard() {
    mute = true;
}

bool DuoshiCard::targetFilter(const QList<const Player *> &, const Player *, const Player *) const{
    return true;
}

bool DuoshiCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
	return true;
}

void DuoshiCard::onUse(Room *room, const CardUseStruct &card_use) const{
    CardUseStruct use = card_use;
    if (!use.to.contains(use.from))
        use.to << use.from;
    use.from->getRoom()->broadcastSkillInvoke("duoshi", qMin(2, use.to.length()));
    room->addPlayerMark(use.from, "duoshiengine");
    if (use.from->getMark("duoshiengine") > 0) {
		SkillCard::onUse(room, use);
        room->removePlayerMark(use.from, "duoshiengine");
    }
}

void DuoshiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    effect.to->drawCards(2, "duoshi");
    room->askForDiscard(effect.to, "duoshi", 2, 2, false, true);
}

class Duoshi: public OneCardViewAsSkill {
public:
    Duoshi(): OneCardViewAsSkill("duoshi") {
        filter_pattern = ".|red|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->usedTimes("DuoshiCard") < 4;
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        DuoshiCard *await = new DuoshiCard;
        await->addSubcard(originalcard->getId());
        return await;
    }
};

class Mingshi: public TriggerSkill {
public:
    Mingshi(): TriggerSkill("mingshi") {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from) {
            if (damage.from->getEquips().length() <= qMin(2, player->getEquips().length())) {
                room->notifySkillInvoked(player, objectName());
                room->broadcastSkillInvoke(objectName());

                LogMessage log;
                log.type = "#Mingshi";
                log.from = player;
                log.arg = QString::number(damage.damage);
                log.arg2 = QString::number(--damage.damage);
                room->sendLog(log);

				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					if (damage.damage < 1) {
						room->removePlayerMark(player, objectName()+"engine");
						return true;
					}
					data = QVariant::fromValue(damage);
					room->removePlayerMark(player, objectName()+"engine");
				}
            }
        }
        return false;
    }
};

class Lirang: public TriggerSkill {
public:
    Lirang(): TriggerSkill("lirang") {
        events << BeforeCardsMove;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *kongrong, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from != kongrong)
            return false;
        if (move.to_place == Player::DiscardPile
            && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)) {

            int i = 0;
            QList<int> lirang_card;
            foreach (int card_id, move.card_ids) {
                if (room->getCardOwner(card_id) == move.from
                    && (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip)) {
                        lirang_card << card_id;
                }
                ++i;
            }
            if (lirang_card.isEmpty())
                return false;

            QList<int> original_lirang = lirang_card;
            while (room->askForYiji(kongrong, lirang_card, objectName(), false, true, true, -1,
                                    QList<ServerPlayer *>(), move.reason, "@lirang-distribute", true)) {
				room->addPlayerMark(kongrong, objectName()+"engine");
				if (kongrong->getMark(objectName()+"engine") > 0) {
					if (kongrong->isDead()) break;
					room->removePlayerMark(kongrong, objectName()+"engine");
				}
            }

            QList<int> ids;
            foreach (int card_id, original_lirang) {
                if (!lirang_card.contains(card_id))
                    ids << card_id;
            }
            move.removeCardIds(ids);
            data = QVariant::fromValue(move);
        }
        return false;
    }
};

class Sijian: public TriggerSkill {
public:
    Sijian(): TriggerSkill("sijian") {
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *tianfeng, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == tianfeng && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard) {
            QList<ServerPlayer *> other_players = room->getOtherPlayers(tianfeng);
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, other_players) {
                if (tianfeng->canDiscard(p, "he"))
                    targets << p;
            }
            if (targets.isEmpty()) return false;
            ServerPlayer *to = room->askForPlayerChosen(tianfeng, targets, objectName(), "sijian-invoke", true, true);
            if (to) {
                room->broadcastSkillInvoke(objectName(), to->isLord() ? 2 : 1);
				room->addPlayerMark(tianfeng, objectName()+"engine");
				if (tianfeng->getMark(objectName()+"engine") > 0) {
					int card_id = room->askForCardChosen(tianfeng, to, "he", objectName(), false, Card::MethodDiscard);
					room->throwCard(card_id, to, tianfeng);
					room->removePlayerMark(tianfeng, objectName()+"engine");
				}
            }
        }
        return false;
    }
};

class Suishi: public TriggerSkill {
public:
    Suishi(): TriggerSkill("suishi") {
        events << Dying << Death;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *target = NULL;
        if (triggerEvent == Dying) {
            DyingStruct dying = data.value<DyingStruct>();
            if (dying.damage && dying.damage->from)
                target = dying.damage->from;
            if (dying.who != player && target
                && room->askForSkillInvoke(target, objectName(), QString("draw:%1").arg(player->objectName()))) {
                room->broadcastSkillInvoke(objectName(), 1);
                if (target != player) {
                    room->notifySkillInvoked(player, objectName());
                    LogMessage log;
                    log.type = "#InvokeOthersSkill";
                    log.from = target;
                    log.to << player;
                    log.arg = objectName();
                    room->sendLog(log);
                }
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, target->objectName(), player->objectName());
				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					player->drawCards(1, objectName());
					room->removePlayerMark(player, objectName()+"engine");
				}
            }
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.damage && death.damage->from)
                target = death.damage->from;
            if (target && room->askForSkillInvoke(target, objectName(), QString("losehp:%1").arg(player->objectName()))) {
                room->broadcastSkillInvoke(objectName(), 2);
                if (target != player) {
                    room->notifySkillInvoked(player, objectName());
                    LogMessage log;
                    log.type = "#InvokeOthersSkill";
                    log.from = target;
                    log.to << player;
                    log.arg = objectName();
                    room->sendLog(log);
                }

				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, target->objectName(), player->objectName());
				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					room->loseHp(player);
					room->removePlayerMark(player, objectName()+"engine");
				}
            }
        }
        return false;
    }
};

ShuangrenCard::ShuangrenCard() {
}

bool ShuangrenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void ShuangrenCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->addPlayerMark(effect.from, "shuangrenengine");
    if (effect.from->getMark("shuangrenengine") > 0) {
		bool success = effect.from->pindian(effect.to, "shuangren", NULL);
		if (success) {
			QList<ServerPlayer *> targets;
			foreach (ServerPlayer *target, room->getAlivePlayers()) {
				if (effect.from->canSlash(target, NULL, false))
					targets << target;
			}
			if (targets.isEmpty())
				return;

			ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "shuangren", "@dummy-slash");

			Slash *slash = new Slash(Card::NoSuit, 0);
			slash->setSkillName("_shuangren");
			room->useCard(CardUseStruct(slash, effect.from, target));
		} else {
			room->broadcastSkillInvoke("shuangren", 2);
			room->setPlayerFlag(effect.from, "ShuangrenSkipPlay");
		}
        room->removePlayerMark(effect.from, "shuangrenengine");
    }
}

class ShuangrenViewAsSkill: public ZeroCardViewAsSkill {
public:
    ShuangrenViewAsSkill(): ZeroCardViewAsSkill("shuangren") {
        response_pattern = "@@shuangren";
    }

    virtual const Card *viewAs() const{
        return new ShuangrenCard;
    }
};

class Shuangren: public PhaseChangeSkill {
public:
    Shuangren(): PhaseChangeSkill("shuangren") {
        view_as_skill = new ShuangrenViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *jiling) const{
        if (jiling->getPhase() == Player::Play && !jiling->isKongcheng()) {
            Room *room = jiling->getRoom();
            bool can_invoke = false;
            QList<ServerPlayer *> other_players = room->getOtherPlayers(jiling);
            foreach (ServerPlayer *player, other_players) {
                if (!player->isKongcheng()) {
                    can_invoke = true;
                    break;
                }
            }

            if (can_invoke)
                room->askForUseCard(jiling, "@@shuangren", "@shuangren-card");
            if (jiling->hasFlag("ShuangrenSkipPlay"))
                return true;
        }

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        if (!card->isKindOf("Slash"))
            return 1;
        return NULL;
    }
};

XiongyiCard::XiongyiCard() {
    mute = true;
}

bool XiongyiCard::targetFilter(const QList<const Player *> &, const Player *, const Player *) const{
    return true;
}

bool XiongyiCard::targetsFeasible(const QList<const Player *> &, const Player *) const{
	return true;
}

void XiongyiCard::onUse(Room *room, const CardUseStruct &card_use) const{
    CardUseStruct use = card_use;
    if (!use.to.contains(use.from))
        use.to << use.from;
    room->removePlayerMark(use.from, "@arise");
    room->broadcastSkillInvoke("xiongyi");
    //room->doLightbox("$XiongyiAnimate", 4500);
    room->addPlayerMark(use.from, "xiongyiengine");
    if (use.from->getMark("xiongyiengine") > 0) {
		SkillCard::onUse(room, use);
        room->removePlayerMark(use.from, "xiongyiengine");
    }
}

void XiongyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    foreach (ServerPlayer *p, targets)
        p->drawCards(3, "xiongyi");
    if (targets.length() <= room->getAlivePlayers().length() / 2 && source->isWounded())
        room->recover(source, RecoverStruct(source));
}

class Xiongyi: public ZeroCardViewAsSkill {
public:
    Xiongyi(): ZeroCardViewAsSkill("xiongyi") {
        frequency = Limited;
        limit_mark = "@arise";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@arise") >= 1;
    }

    virtual const Card *viewAs() const{
        return new XiongyiCard;
    }
};

class Huoshui: public TriggerSkill {
public:
    Huoshui(): TriggerSkill("huoshui") {
        events << EventPhaseStart << Death
               << EventLoseSkill << EventAcquireSkill
               << HpChanged << MaxHpChanged;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual int getPriority(TriggerEvent) const{
        return 5;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart) {
            if (!TriggerSkill::triggerable(player)
                || (player->getPhase() != Player::RoundStart && player->getPhase() != Player::NotActive)) return false;
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player || !player->hasSkill(objectName())) return false;
        } else if (triggerEvent == EventLoseSkill) {
            if (data.toString() != objectName() || player->getPhase() == Player::NotActive) return false;
        } else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() != objectName() || !player->hasSkill(objectName()) || player->getPhase() == Player::NotActive)
                return false;
        } else if (triggerEvent == MaxHpChanged || triggerEvent == HpChanged) {
            if (!room->getCurrent() || !room->getCurrent()->hasSkill(objectName())) return false;
        }

        foreach (ServerPlayer *p, room->getAllPlayers())
            room->filterCards(p, p->getCards("he"), true);
        Json::Value args;
        args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

        return false;
    }
};

class HuoshuiInvalidity: public InvaliditySkill {
public:
    HuoshuiInvalidity(): InvaliditySkill("#huoshui-inv") {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const{
        if (player->getPhase() == Player::NotActive) {
            const Player *current = NULL;
            foreach (const Player *p, player->getAliveSiblings()) {
                if (p->getPhase() != Player::NotActive) {
                    current = p;
                    break;
                }
            }
            if (current && current->hasSkill("huoshui")
                && player->getHp() >= (player->getMaxHp() + 1) / 2 && !skill->isAttachedLordSkill())
                return false;
        }
        return true;
    }
};

QingchengCard::QingchengCard() {
    handling_method = Card::MethodDiscard;
}

void QingchengCard::onUse(Room *room, const CardUseStruct &use) const{
    room->addPlayerMark(use.from, "qingchengengine");
    if (use.from->getMark("qingchengengine") > 0) {
		CardUseStruct card_use = use;
		ServerPlayer *player = card_use.from, *to = card_use.to.first();

		LogMessage log;
		log.from = player;
		log.to = card_use.to;
		log.type = "#UseCard";
		log.card_str = card_use.card->toString();
		room->sendLog(log);

		QStringList skill_list;
		foreach (const Skill *skill, to->getVisibleSkillList()) {
			if (!skill_list.contains(skill->objectName()) && !skill->isAttachedLordSkill()) {
				skill_list << skill->objectName();
			}
		}
		QString skill_qc;
		if (!skill_list.isEmpty()) {
			QVariant data_for_ai = QVariant::fromValue(to);
			skill_qc = room->askForChoice(player, "qingcheng", skill_list.join("+"), data_for_ai);
		}

		if (!skill_qc.isEmpty()) {
			LogMessage log;
			log.type = "$QingchengNullify";
			log.from = player;
			log.to << to;
			log.arg = skill_qc;
			room->sendLog(log);

			QStringList Qingchenglist = to->tag["Qingcheng"].toStringList();
			Qingchenglist << skill_qc;
			to->tag["Qingcheng"] = QVariant::fromValue(Qingchenglist);
			room->addPlayerMark(to, "Qingcheng" + skill_qc);

			foreach (ServerPlayer *p, room->getAllPlayers())
				room->filterCards(p, p->getCards("he"), true);

			Json::Value args;
			args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
			room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
		}

		QVariant data = QVariant::fromValue(card_use);
		RoomThread *thread = room->getThread();
		thread->trigger(PreCardUsed, room, player, data);
		card_use = data.value<CardUseStruct>();

		CardMoveReason reason(CardMoveReason::S_REASON_THROW, player->objectName(), QString(), card_use.card->getSkillName(), QString());
		room->moveCardTo(this, player, NULL, Player::DiscardPile, reason, true);

		thread->trigger(CardUsed, room, card_use.from, data);
		card_use = data.value<CardUseStruct>();
		thread->trigger(CardFinished, room, card_use.from, data);
        room->removePlayerMark(use.from, "qingchengengine");
    }
}

bool QingchengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self;
}

class QingchengViewAsSkill: public OneCardViewAsSkill {
public:
    QingchengViewAsSkill(): OneCardViewAsSkill("qingcheng") {
        filter_pattern = "EquipCard!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he");
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        QingchengCard *first = new QingchengCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class Qingcheng: public TriggerSkill {
public:
    Qingcheng(): TriggerSkill("qingcheng") {
        events << EventPhaseStart;
        view_as_skill = new QingchengViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual int getPriority(TriggerEvent) const{
        return 6;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() == Player::RoundStart) {
            QStringList Qingchenglist = player->tag["Qingcheng"].toStringList();
            if (Qingchenglist.isEmpty()) return false;
            foreach (QString skill_name, Qingchenglist) {
                room->setPlayerMark(player, "Qingcheng" + skill_name, 0);
                if (player->hasSkill(skill_name)) {
                    LogMessage log;
                    log.type = "$QingchengReset";
                    log.from = player;
                    log.arg = skill_name;
                    room->sendLog(log);
                }
            }
            player->tag.remove("Qingcheng");
            foreach (ServerPlayer *p, room->getAllPlayers())
                room->filterCards(p, p->getCards("he"), true);

            Json::Value args;
            args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
        }
        return false;
    }
};

class QingchengInvalidity: public InvaliditySkill {
public:
    QingchengInvalidity(): InvaliditySkill("#qingcheng-inv") {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const{
        return player->getMark("Qingcheng" + skill->objectName()) == 0;
    }
};

HegemonyPackage::HegemonyPackage()
    : Package("hegemony")
{
    General *heg_luxun = new General(this, "heg_luxun", "wu", 3); // WU 007 G
    heg_luxun->addSkill("nosqianxun");
    heg_luxun->addSkill(new Duoshi);

    General *mateng = new General(this, "mateng", "qun"); // QUN 013
    mateng->addSkill("mashu");
    mateng->addSkill(new Xiongyi);

    General *kongrong = new General(this, "kongrong", "qun", 3); // QUN 014
    kongrong->addSkill(new Mingshi);
    kongrong->addSkill(new Lirang);

    General *jiling = new General(this, "jiling", "qun", 4); // QUN 015
    jiling->addSkill(new Shuangren);
    jiling->addSkill(new SlashNoDistanceLimitSkill("shuangren"));
    related_skills.insertMulti("shuangren", "#shuangren-slash-ndl");

    General *tianfeng = new General(this, "tianfeng", "qun", 3); // QUN 016
    tianfeng->addSkill(new Sijian);
    tianfeng->addSkill(new Suishi);

    General *zoushi = new General(this, "zoushi", "qun", 3, false); // QUN 018
    zoushi->addSkill(new Huoshui);
    zoushi->addSkill(new HuoshuiInvalidity);
    zoushi->addSkill(new Qingcheng);
    zoushi->addSkill(new QingchengInvalidity);
    related_skills.insertMulti("huoshui", "#huoshui-inv");
    related_skills.insertMulti("qingcheng", "#qingcheng-inv");
	
    addMetaObject<DuoshiCard>();
    addMetaObject<ShuangrenCard>();
    addMetaObject<XiongyiCard>();
    addMetaObject<QingchengCard>();
}

ADD_PACKAGE(Hegemony)
