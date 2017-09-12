#include "h-formation.h"
#include "general.h"
#include "standard.h"
#include "standard-equips.h"
#include "maneuvering.h"
#include "skill.h"
#include "engine.h"
#include "client.h"
#include "serverplayer.h"
#include "room.h"
#include "ai.h"
#include "settings.h"
#include "jsonutils.h"

ZiliangCard::ZiliangCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void ZiliangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{	
    room->addPlayerMark(source, "ziliangengine");
    if (source->getMark("ziliangengine") > 0) {
		ServerPlayer *target = source->tag["ZiliangCurrentTarget"].value<ServerPlayer *>();
		if (target == NULL)
			return;

		source->tag.remove("ZiliangCurrentTarget");

		if (target == source) {
			LogMessage log;
			log.type = "$MoveCard";
			log.from = source;
			log.to << source;
			log.card_str = QString::number(subcards.first());
			room->sendLog(log);
		}

		target->obtainCard(Sanguosha->getCard(subcards.first()));
        room->removePlayerMark(source, "ziliangengine");
    }
}

class ZiliangVS : public OneCardViewAsSkill
{
public:
    ZiliangVS() : OneCardViewAsSkill("ziliang")
    {
        expand_pile = "field";
        filter_pattern = ".|.|.|field";
        response_pattern = "@@ziliang";
    }

    virtual const Card *viewAs(const Card *c) const
    {
        ZiliangCard *zl = new ZiliangCard;
        zl->addSubcard(c);
        return zl;
    }
};

class Ziliang : public TriggerSkill
{
public:
    Ziliang() : TriggerSkill("ziliang")
    {
        events << Damaged;
        view_as_skill = new ZiliangVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        foreach (ServerPlayer *dengai, room->getAllPlayers()) {
            if (!TriggerSkill::triggerable(dengai) || !player->isAlive()) continue;
            if (dengai->getPile("field").isEmpty()) continue;

            dengai->tag["ZiliangCurrentTarget"] = QVariant::fromValue(player);
            room->askForUseCard(dengai, "@@ziliang", "@ziliang", -1, Card::MethodNone);
        }

        return false;
    }
};

HuyuanCard::HuyuanCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool HuyuanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    if (!targets.isEmpty())
        return false;

    const Card *card = Sanguosha->getCard(subcards.first());
    const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
    int equip_index = static_cast<int>(equip->location());
    return to_select->getEquip(equip_index) == NULL;
}

void HuyuanCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *caohong = effect.from;
    Room *room = caohong->getRoom();
    room->addPlayerMark(effect.from, "huyuanengine");
    if (effect.from->getMark("huyuanengine") > 0) {
		room->moveCardTo(this, caohong, effect.to, Player::PlaceEquip,
						 CardMoveReason(CardMoveReason::S_REASON_PUT, caohong->objectName(), "huyuan", QString()));

		const Card *card = Sanguosha->getCard(subcards.first());

		LogMessage log;
		log.type = "$ZhijianEquip";
		log.from = effect.to;
		log.card_str = QString::number(card->getEffectiveId());
		room->sendLog(log);

		QList<ServerPlayer *> targets;
		foreach (ServerPlayer *p, room->getAllPlayers()) {
			if (effect.to->distanceTo(p) == 1 && caohong->canDiscard(p, "he"))
				targets << p;
		}
		if (!targets.isEmpty()) {
			ServerPlayer *to_dismantle = room->askForPlayerChosen(caohong, targets, "huyuan", "@huyuan-discard:" + effect.to->objectName(), true, true);
			if (to_dismantle) {
				int card_id = room->askForCardChosen(caohong, to_dismantle, "he", "huyuan", false, Card::MethodDiscard);
				room->throwCard(Sanguosha->getCard(card_id), to_dismantle, caohong);
			}
		}
        room->removePlayerMark(effect.from, "huyuanengine");
    }
}

class HuyuanViewAsSkill: public OneCardViewAsSkill {
public:
    HuyuanViewAsSkill(): OneCardViewAsSkill("huyuan") {
        filter_pattern = "EquipCard";
        response_pattern = "@@huyuan";
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        HuyuanCard *first = new HuyuanCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class Huyuan: public PhaseChangeSkill {
public:
    Huyuan(): PhaseChangeSkill("huyuan") {
        view_as_skill = new HuyuanViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        if (target->getPhase() == Player::Finish && !target->isNude())
            room->askForUseCard(target, "@@huyuan", "@huyuan-equip", -1, Card::MethodNone);
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        const Card *rcard = Sanguosha->getCard(card->getEffectiveId());
        if (rcard->isKindOf("Weapon")) return 1;
        else if (rcard->isKindOf("Armor")) return 2;
        else return 3;
    }
};

HeyiCard::HeyiCard() {
}

bool HeyiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const{
    return targets.length() < 2;
}

bool HeyiCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    return targets.length() == 2;
}

void HeyiCard::onUse(Room *room, const CardUseStruct &card_use) const{
    CardUseStruct use = card_use;

    LogMessage log;
    log.from = use.from;
    log.to << card_use.to;
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);

    QVariant data = QVariant::fromValue(use);
    RoomThread *thread = room->getThread();

    thread->trigger(PreCardUsed, room, use.from, data);
    use = data.value<CardUseStruct>();
    thread->trigger(CardUsed, room, use.from, data);
    use = data.value<CardUseStruct>();
    thread->trigger(CardFinished, room, use.from, data);
}

void HeyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->addPlayerMark(source, "engine");
    if (source->getMark("engine") > 0) {
		room->setTag("HeyiSource", QVariant::fromValue(source));
		QList<ServerPlayer *> players = room->getAllPlayers();
		int index1 = players.indexOf(targets.first()), index2 = players.indexOf(targets.last());
		int index_self = players.indexOf(source);
		QList<ServerPlayer *> cont_targets;
		if (index1 == index_self || index2 == index_self) {
			forever {
				cont_targets.append(players.at(index1));
				if (index1 == index2) break;
				++index1;
				if (index1 >= players.length())
					index1 -= players.length();
			}
		} else {
			if (index1 > index2)
				qSwap(index1, index2);
			if (index_self > index1 && index_self < index2) {
				for (int i = index1; i <= index2; ++i)
					cont_targets.append(players.at(i));
			} else {
				forever {
					cont_targets.append(players.at(index2));
					if (index1 == index2) break;
					++index2;
					if (index2 >= players.length())
						index2 -= players.length();
				}
			}
		}
		cont_targets.removeOne(source);
		QStringList list;
		foreach(ServerPlayer *p, cont_targets) {
			if (!p->isAlive()) continue;
			list.append(p->objectName());
			source->tag["heyi"] = QVariant::fromValue(list);
			room->acquireSkill(p, "feiying");
		}
        room->removePlayerMark(source, "engine");
    }
}

class HeyiViewAsSkill: public ZeroCardViewAsSkill {
public:
    HeyiViewAsSkill(): ZeroCardViewAsSkill("heyi") {
        response_pattern = "@@heyi";
    }

    virtual const Card *viewAs() const{
        return new HeyiCard;
    }
};

class Heyi: public TriggerSkill {
public:
    Heyi(): TriggerSkill("heyi") {
        events << EventPhaseChanging << Death;
        view_as_skill = new HeyiViewAsSkill;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return false;
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        }
        if (room->getTag("HeyiSource").value<ServerPlayer *>() == player) {
            room->removeTag("HeyiSource");
            QStringList list = player->tag[objectName()].toStringList();
            player->tag.remove(objectName());
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (list.contains(p->objectName()))
                    room->detachSkillFromPlayer(p, "feiying", false, true);
            }
        }
        if (TriggerSkill::triggerable(player) && triggerEvent == EventPhaseChanging)
            room->askForUseCard(player, "@@heyi", "@heyi");
        return false;
    }
};

class Tianfu: public TriggerSkill {
public:
    Tianfu(): TriggerSkill("tianfu") {
        events << EventPhaseStart << EventPhaseChanging;
    }

    virtual int getPriority(TriggerEvent) const{
        return 4;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart) {
            QList<ServerPlayer *> jiangweis = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *jiangwei, jiangweis) {
                if (jiangwei->isAlive() && (player == jiangwei || player->isAdjacentTo(jiangwei))
                    && (player == jiangwei || room->askForSkillInvoke(player, objectName(), QVariant::fromValue(jiangwei)))) {
                    if (player != jiangwei) {
                        room->notifySkillInvoked(jiangwei, objectName());
                        LogMessage log;
                        log.type = "#InvokeOthersSkill";
                        log.from = player;
                        log.to << jiangwei;
                        log.arg = objectName();
                        room->sendLog(log);
                    }
                    jiangwei->addMark(objectName());
					room->addPlayerMark(player, objectName()+"engine");
					if (player->getMark(objectName()+"engine") > 0) {
						room->acquireSkill(jiangwei, "kanpo");
						room->removePlayerMark(player, objectName()+"engine");
					}
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive) return false;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->getMark(objectName()) > 0) {
                    p->setMark(objectName(), 0);
                    room->detachSkillFromPlayer(p, "kanpo", false, true);
                }
            }
        }
        return false;
    }
};

class Shengxi : public TriggerSkill
{
public:
    Shengxi() : TriggerSkill("shengxi")
    {
        events << EventPhaseEnd << EventPhaseStart;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &) const
    {
        if ((!Config.OL && event == EventPhaseEnd && player->getPhase() == Player::Play && player->getMark("damage_point_play_phase") == 0 && player->askForSkillInvoke(objectName())) || (Config.value("jiangwanfeiy_down", true).toBool() && event == EventPhaseStart && player->getPhase() == Player::Play && player->getMark("damage_point_round") == 0 && player->askForSkillInvoke(objectName()))) {
            room->broadcastSkillInvoke(objectName());
            player->drawCards(2, objectName());
		}
        return false;
    }
};

class Shoucheng: public TriggerSkill {
public:
    Shoucheng(): TriggerSkill("shoucheng") {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from && move.from->isAlive() && move.from->getPhase() == Player::NotActive
            && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard) {
            if (room->askForSkillInvoke(player, objectName(), data)) {
                if (move.from == player || room->askForChoice(player, objectName(), "accept+reject") == "accept") {
                    room->broadcastSkillInvoke(objectName());
                    ServerPlayer *from = (ServerPlayer *)move.from;
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), from->objectName());
					room->addPlayerMark(from, objectName()+"engine");
					if (from->getMark(objectName()+"engine") > 0) {
						from->drawCards(1, objectName());
						room->removePlayerMark(from, objectName()+"engine");
					}
                } else {
                    LogMessage log;
                    log.type = "#ZhibaReject";
                    log.from = (ServerPlayer *)move.from;
                    log.to << player;
                    log.arg = objectName();
                    room->sendLog(log);
                }
            }
        }
        return false;
    }
};

ShangyiCard::ShangyiCard() {
}

bool ShangyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self;
}

void ShangyiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    ServerPlayer *player = effect.to;
    if (!effect.from->isKongcheng())
        room->showAllCards(effect.from, player);
	
    room->addPlayerMark(effect.from, "shangyiengine");
    if (effect.from->getMark("shangyiengine") > 0) {
		QStringList choicelist;
		if (!effect.to->isKongcheng())
			choicelist.append("handcards");
		if (room->getMode() == "04_1v3" || room->getMode() == "04_boss"
			|| room->getMode() == "06_3v3" || room->getMode() == "08_defense") {
			;
		} else if (room->getMode() == "06_XMode") {
			QStringList backup = player->tag["XModeBackup"].toStringList();
			if (backup.length() > 0)
				choicelist.append("remainedgenerals");
		} else if (room->getMode() == "02_1v1") {
			QStringList list = player->tag["1v1Arrange"].toStringList();
			if (list.length() > 0)
				choicelist.append("remainedgenerals");
		} else if (Config.EnableBasara) {
			QStringList hidden_generals = player->getBasaraGeneralNames();
			if (!hidden_generals.isEmpty())
				choicelist.append("generals");
		} else if (!player->isLord()) {
			choicelist.append("role");
		}
		if (choicelist.isEmpty()) return;
		QString choice = room->askForChoice(effect.from, "shangyi", choicelist.join("+"), QVariant::fromValue(player));

		LogMessage log;
		log.type = "$ShangyiView";
		log.from = effect.from;
		log.to << effect.to;
		log.arg = "shangyi:" + choice;
		room->sendLog(log, room->getOtherPlayers(effect.from));

		if (choice == "handcards") {
			QList<int> ids;
			foreach (const Card *card, player->getHandcards()) {
				if (card->isBlack())
					ids << card->getEffectiveId();
			}

			int card_id = room->doGongxin(effect.from, player, ids, "shangyi");
			if (card_id == -1) return;
			effect.from->tag.remove("shangyi");
			CardMoveReason reason(CardMoveReason::S_REASON_DISMANTLE, effect.from->objectName(), QString(), "shangyi", QString());
			room->throwCard(Sanguosha->getCard(card_id), reason, effect.to, effect.from);
		} else if (choice == "remainedgenerals") {
			QStringList list;
			if (room->getMode() == "02_1v1")
				list = player->tag["1v1Arrange"].toStringList();
			else if (room->getMode() == "06_XMode")
				list = player->tag["XModeBackup"].toStringList();
			foreach (QString name, list) {
				LogMessage log;
				log.type = "$ShangyiViewRemained";
				log.from = effect.from;
				log.to << player;
				log.arg = name;
				room->sendLog(log, effect.from);
			}
			Json::Value arr(Json::arrayValue);
			arr[0] = QSanProtocol::Utils::toJsonString("shangyi");
			arr[1] = QSanProtocol::Utils::toJsonArray(list);
			room->doNotify(effect.from, QSanProtocol::S_COMMAND_VIEW_GENERALS, arr);
		} else if (choice == "generals") {
			QStringList list = player->getBasaraGeneralNames();
			foreach (const QString &name, list) {
				LogMessage log;
				log.type = "$ShangyiViewUnknown";
				log.from = effect.from;
				log.to << player;
				log.arg = name;
				room->sendLog(log, effect.from);
			}
			Json::Value arg(Json::arrayValue);
			arg[0] = QSanProtocol::Utils::toJsonString("shangyi");
			arg[1] = QSanProtocol::Utils::toJsonArray(list);
			room->doNotify(effect.from, QSanProtocol::S_COMMAND_VIEW_GENERALS, arg);
		} else if (choice == "role") {
			Json::Value arg(Json::arrayValue);
			arg[0] = QSanProtocol::Utils::toJsonString(player->objectName());
			arg[1] = QSanProtocol::Utils::toJsonString(player->getRole());
			room->doNotify(effect.from, QSanProtocol::S_COMMAND_SET_EMOTION, arg);

			LogMessage log;
			log.type = "$ViewRole";
			log.from = effect.from;
			log.to << player;
			log.arg = player->getRole();
			room->sendLog(log, effect.from);
		}
        room->removePlayerMark(effect.from, "shangyiengine");
    }
}

class Shangyi: public ZeroCardViewAsSkill {
public:
    Shangyi(): ZeroCardViewAsSkill("shangyi") {
    }

    virtual const Card *viewAs() const{
        return new ShangyiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ShangyiCard");
    }
};

class Niaoxiang: public TriggerSkill {
public:
    Niaoxiang(): TriggerSkill("niaoxiang") {
        events << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && use.from->isAlive()) {
            QVariantList jink_list = use.from->tag["Jink_" + use.card->toString()].toList();
            for (int i = 0; i < use.to.length(); ++i) {
                ServerPlayer *to = use.to.at(i);
                if (to->isAlive() && to->isAdjacentTo(player) && to->isAdjacentTo(use.from)
                    && room->askForSkillInvoke(player, objectName(), QVariant::fromValue(to))) {
                    room->broadcastSkillInvoke(objectName());
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), to->objectName());
					room->addPlayerMark(player, objectName()+"engine");
					if (player->getMark(objectName()+"engine") > 0) {
						if (jink_list.at(i).toInt() == 1)
							jink_list.replace(i, QVariant(2));
						room->removePlayerMark(player, objectName()+"engine");
					}
                }
            }
            use.from->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        }

        return false;
    }
};

class Yicheng: public TriggerSkill {
public:
    Yicheng(): TriggerSkill("yicheng") {
        events << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash")) return false;
        foreach (ServerPlayer *p, use.to) {
            if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(p))) {
                room->broadcastSkillInvoke(objectName());
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());
				room->addPlayerMark(p, objectName()+"engine");
				if (p->getMark(objectName()+"engine") > 0) {
					p->drawCards(1, objectName());
					if (p->isAlive() && p->canDiscard(p, "he"))
						room->askForDiscard(p, objectName(), 1, 1, false, true);
					room->removePlayerMark(p, objectName()+"engine");
				}
            }
            if (!player->isAlive())
                break;
        }
        return false;
    }
};

class Qianhuan: public TriggerSkill {
public:
    Qianhuan(): TriggerSkill("qianhuan") {
        events << Damaged << TargetConfirming;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == Damaged && player->isAlive()) {
            ServerPlayer *yuji = room->findPlayerBySkillName(objectName());
            if (yuji && room->askForSkillInvoke(player, objectName(), "choice:" + yuji->objectName())) {
                room->broadcastSkillInvoke(objectName());
                if (yuji != player) {
                    room->notifySkillInvoked(yuji, objectName());
                    LogMessage log;
                    log.type = "#InvokeOthersSkill";
                    log.from = player;
                    log.to << yuji;
                    log.arg = objectName();
                    room->sendLog(log);
                }

				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					int id = room->drawCard();
					Card::Suit suit = Sanguosha->getCard(id)->getSuit();
					bool duplicate = false;
					foreach (int card_id, yuji->getPile("sorcery")) {
						if (Sanguosha->getCard(card_id)->getSuit() == suit) {
							duplicate = true;
							break;
						}
					}
					yuji->addToPile("sorcery", id);
					if (duplicate) {
						CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
						room->throwCard(Sanguosha->getCard(id), reason, NULL);
					}
					room->removePlayerMark(player, objectName()+"engine");
				}
            }
        } else if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() == Card::TypeEquip || use.card->getTypeId() == Card::TypeSkill)
                return false;
            if (use.to.length() != 1) return false;
            ServerPlayer *yuji = room->findPlayerBySkillName(objectName());
            if (!yuji || yuji->getPile("sorcery").isEmpty()) return false;
            if (room->askForSkillInvoke(yuji, objectName(), data)) {
                room->broadcastSkillInvoke(objectName());
                room->notifySkillInvoked(yuji, objectName());
                if (yuji == player || room->askForChoice(player, objectName(), "accept+reject", data) == "accept") {
                    QList<int> ids = yuji->getPile("sorcery");
                    int id = -1;
                    if (ids.length() > 1) {
                        room->fillAG(ids, yuji);
                        id = room->askForAG(yuji, ids, false, objectName());
                        room->clearAG(yuji);
                    } else {
                        id = ids.first();
                    }
                    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
                    room->throwCard(Sanguosha->getCard(id), reason, NULL);

                    LogMessage log;
                    if (use.from) {
                        log.type = "$CancelTarget";
                        log.from = use.from;
                    } else {
                        log.type = "$CancelTargetNoUser";
                    }
                    log.to = use.to;
                    log.arg = use.card->objectName();
                    room->sendLog(log);

                    use.to.clear();
					room->addPlayerMark(player, objectName()+"engine");
					if (player->getMark(objectName()+"engine") > 0) {
						data = QVariant::fromValue(use);
						room->removePlayerMark(player, objectName()+"engine");
					}
                } else {
                    LogMessage log;
                    log.type = "#ZhibaReject";
                    log.from = player;
                    log.to << yuji;
                    log.arg = objectName();
                    room->sendLog(log);
                }
            }
        }
        return false;
    }
};

HFormationPackage::HFormationPackage()
    : Package("h_formation")
{
    General *heg_dengai = new General(this, "heg_dengai", "wei", 4, true, Config.value("EnableHidden", true).toBool()); // WEI 015 G
    heg_dengai->addSkill("tuntian");
    heg_dengai->addSkill(new Ziliang);

    General *heg_caohong = new General(this, "heg_caohong", "wei", 4, true, Config.value("EnableHidden", true).toBool()); // WEI 018
    heg_caohong->addSkill(new Huyuan);
    heg_caohong->addSkill(new Heyi);

    General *heg_jiangwei = new General(this, "heg_jiangwei", "shu", 4, true, Config.value("EnableHidden", true).toBool()); // SHU 012 G
    heg_jiangwei->addSkill("tiaoxin");
    heg_jiangwei->addSkill(new Tianfu);

    General *jiangwanfeiyi = new General(this, "jiangwanfeiyi", "shu", 3); // SHU 018
    jiangwanfeiyi->addSkill(new Shengxi);
    jiangwanfeiyi->addSkill(new Shoucheng);

    General *jiangqin = new General(this, "jiangqin", "wu"); // WU 017
    jiangqin->addSkill(new Shangyi);
    jiangqin->addSkill(new Niaoxiang);

    General *heg_xusheng = new General(this, "heg_xusheng", "wu", 4, true, Config.value("EnableHidden", true).toBool()); // WU 020
    heg_xusheng->addSkill(new Yicheng);

    General *heg_yuji = new General(this, "heg_yuji", "qun", 3, true, Config.value("EnableHidden", true).toBool()); // QUN 011 G
    heg_yuji->addSkill(new Qianhuan);

    addMetaObject<HuyuanCard>();
    addMetaObject<HeyiCard>();
    addMetaObject<ShangyiCard>();
    addMetaObject<ZiliangCard>();
}

ADD_PACKAGE(HFormation)
