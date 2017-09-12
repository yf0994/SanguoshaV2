#include "yjcm2016.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "engine.h"
#include "clientplayer.h"
#include "settings.h"
#include "jsonutils.h"

class dummyVS : public ZeroCardViewAsSkill
{
public:
    dummyVS() : ZeroCardViewAsSkill("dummy")
    {
    }

    virtual const Card *viewAs() const
    {
        return NULL;
    }
};

JiaozhaoCard::JiaozhaoCard()
{
	target_fixed = true;
	will_throw = false;
    handling_method = Card::MethodNone;
}

void JiaozhaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->showCard(source, getEffectiveId());

	ServerPlayer *target = NULL;
	if (source->getMark("@danxin_modify") > 1)
		target = source;
	else {
		QList<ServerPlayer *> players = room->getOtherPlayers(source);
        QList<int> distance_list;
        int nearest = 1000;
        foreach (ServerPlayer *player, players) {
            int distance = source->distanceTo(player);
            distance_list << distance;
            nearest = qMin(nearest, distance);
        }

        QList<ServerPlayer *> nearest_targets;
        for (int i = 0; i < distance_list.length(); i++) {
            if (distance_list[i] == nearest)
                nearest_targets << players[i];
        }

	    if (nearest_targets.isEmpty()) return;

	    target = room->askForPlayerChosen(source, nearest_targets, "jiaozhao", "@jiaozhao-target");
		room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), target->objectName());
	}

	QStringList basics, tricks1, tricks2;
	QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    foreach (const Card *card, cards) {
        if (card->getTypeId() == Card::TypeBasic && !basics.contains(card->objectName())
            && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
            basics << card->objectName();
        } else if (card->isNDTrick() && !tricks1.contains(card->objectName()) && !tricks2.contains(card->objectName())
            && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
			if (card->isKindOf("SingleTargetTrick"))
                tricks1 << card->objectName();
			else
				tricks2 << card->objectName();
        }
    }
    QString card_names;
	bool has_option = false;
	if (!basics.isEmpty()) {
		has_option = true;
		card_names = basics.join("+");
	}
	if (source->getMark("@danxin_modify") > 0) {
		if (!tricks1.isEmpty()) {
		    has_option = true;
		    card_names = card_names + "|" + tricks1.join("+");
	    }
		if (!tricks2.isEmpty()) {
		    has_option = true;
		    card_names = card_names + "|" + tricks2.join("+");
	    }
	}
	if (has_option) {
		QString card_name = room->askForChoice(target, "jiaozhao", card_names);
		LogMessage log;
        log.type = "$JiaozhaoDeclare";
	    log.arg = card_name;
        log.from = target;
        room->sendLog(log);
		room->setPlayerProperty(source, "jiaozhao_record_id", QString::number(getEffectiveId()));
		room->setPlayerProperty(source, "jiaozhao_record_name", card_name);
	}
}

class JiaozhaoViewAsSkill : public OneCardViewAsSkill
{
public:
    JiaozhaoViewAsSkill() : OneCardViewAsSkill("jiaozhao")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
		if (player->hasUsed("JiaozhaoCard")) {
			QString record_id = Self->property("jiaozhao_record_id").toString();
			QString record_name = Self->property("jiaozhao_record_name").toString();
			if (record_id == "" || record_name == "") return false;
            Card *use_card = Sanguosha->cloneCard(record_name);
            if (!use_card) return false;
            use_card->setCanRecast(false);
            use_card->addSubcard(record_id.toInt());
		    use_card->setSkillName("jiaozhao");
			return use_card->isAvailable(player);
		}
        return true;
    }

    virtual bool viewFilter(const Card *card) const
    {
        if (card->isEquipped()) return false;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !Self->hasUsed("JiaozhaoCard"))
			return true;
	    QString record_id = Self->property("jiaozhao_record_id").toString();
		return record_id != "" && record_id.toInt() == card->getEffectiveId();
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
		if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !Self->hasUsed("JiaozhaoCard")) {
		    JiaozhaoCard *jiaozhao = new JiaozhaoCard;
            jiaozhao->addSubcard(originalCard);
            return jiaozhao;
		}
		QString record_name = Self->property("jiaozhao_record_name").toString();
	    Card *use_card = Sanguosha->cloneCard(record_name);
		if (!use_card) return NULL;
        use_card->setCanRecast(false);
        use_card->addSubcard(originalCard);
		use_card->setSkillName("jiaozhao");
        return use_card;
	}

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return false;
		
		if (pattern.startsWith(".") || pattern.startsWith("@"))
			return false;

		QString record_name = player->property("jiaozhao_record_name").toString();
		if (record_name == "") return false;
		QString pattern_names = pattern;
		if (pattern == "slash")
			pattern_names = "slash+fire_slash+thunder_slash";
		else if (pattern == "peach" && player->getMark("Global_PreventPeach") > 0)
			return false;
		else if (pattern == "peach+analeptic")
			return false;
		
		return pattern_names.split("+").contains(record_name);
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return player->property("jiaozhao_record_name").toString() == "nullification";
    }
};

class Jiaozhao : public TriggerSkill
{
public:
    Jiaozhao() : TriggerSkill("jiaozhao")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
		view_as_skill = new JiaozhaoViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
			if (player != move.from) return false;
			QString record_id = player->property("jiaozhao_record_id").toString();
		    if (record_id == "") return false;
			int id = record_id.toInt();
			if (move.card_ids.contains(id) && move.from_places[move.card_ids.indexOf(id)] == Player::PlaceHand) {
                room->setPlayerProperty(player, "jiaozhao_record_id", QString());
			    room->setPlayerProperty(player, "jiaozhao_record_name", QString());
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
            room->setPlayerProperty(player, "jiaozhao_record_id", QString());
			room->setPlayerProperty(player, "jiaozhao_record_name", QString());
        }
        return false;
    }
};

class JiaozhaoProhibit : public ProhibitSkill
{
public:
    JiaozhaoProhibit() : ProhibitSkill("#jiaozhao")
    {
    }

    bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> & /* = QList<const Player *>() */) const
    {
        return (!card->isKindOf("SkillCard") && card->getSkillName() == "jiaozhao" && from == to);
    }
};

class Danxin : public MasochismSkill
{
public:
    Danxin() : MasochismSkill("danxin")
    {
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &) const
    {
        Room *room = target->getRoom();
        if (target->askForSkillInvoke(objectName())){
			room->broadcastSkillInvoke(objectName());
            if (target->getMark("@danxin_modify") > 1 || room->askForChoice(target, objectName(), "modify+draw") == "draw")
				target->drawCards(1, objectName());
			else {
				room->addPlayerMark(target, "@danxin_modify");
				QString translate = Sanguosha->translate(":jiaozhao" + QString::number(target->getMark("@danxin_modify")));
				Sanguosha->addTranslationEntry(":jiaozhao", translate.toStdString().c_str());

				Json::Value args;
                        args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

			}
		}
    }
};

ZhigeCard::ZhigeCard()
{
}

bool ZhigeCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->inMyAttackRange(Self);
}

void ZhigeCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
	bool use_slash = room->askForUseSlashTo(effect.to, room->getAlivePlayers(), "@zhige-slash:" + effect.from->objectName());
    if (!use_slash && effect.to->hasEquip()) {
		QList<int> equips;
        foreach (const Card *card, effect.to->getEquips())
            equips << card->getEffectiveId();
		room->fillAG(equips, effect.to);
        int to_give = room->askForAG(effect.to, equips, false, "zhige");
        room->clearAG(effect.to);
		CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.to->objectName(), effect.from->objectName(), "zhige", QString());
		reason.m_playerId = effect.from->objectName();
        room->moveCardTo(Sanguosha->getCard(to_give), effect.to, effect.from, Player::PlaceHand, reason);
	}
}

class Zhige : public ZeroCardViewAsSkill
{
public:
    Zhige() : ZeroCardViewAsSkill("zhige")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ZhigeCard") && player->getHandcardNum() > player->getHp();
    }

    virtual const Card *viewAs() const
    {
        return new ZhigeCard;
    }
};

class Zongzuo : public TriggerSkill
{
public:
    Zongzuo() : TriggerSkill("zongzuo")
    {
        events << GameStart << BuryVictim;
	}

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    int getPriority(TriggerEvent triggerEvent) const
    {
		if (triggerEvent == BuryVictim)
            return -2;
		return TriggerSkill::getPriority(triggerEvent);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == GameStart && TriggerSkill::triggerable(player)) {
			QSet<QString> kingdom_set;
            foreach(ServerPlayer *p, room->getAlivePlayers())
                kingdom_set << p->getKingdom();

            int n = kingdom_set.size();
			if (n > 0) {
                room->sendCompulsoryTriggerLog(player, objectName());
				room->broadcastSkillInvoke(objectName());

                LogMessage log2;
                log2.type = "#GainMaxHp";
                log2.from = player;
                log2.arg = QString::number(n);
                room->sendLog(log2);

                room->setPlayerProperty(player, "maxhp", player->getMaxHp() + n);
				room->recover(player, RecoverStruct(player, NULL, n));
			}
		} else if (triggerEvent == BuryVictim) {
			foreach (ServerPlayer *liuyu, room->getAlivePlayers()) {
                if (!TriggerSkill::triggerable(liuyu)) continue;
				foreach(ServerPlayer *p, room->getAlivePlayers()) {
					if (p->getKingdom() == player->getKingdom())
						return false;
				}
				room->sendCompulsoryTriggerLog(liuyu, objectName());
				room->broadcastSkillInvoke(objectName());
				room->loseMaxHp(liuyu);
		    }
		}
        return false;
    }
};

DuliangCard::DuliangCard()
{
}

bool DuliangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void DuliangCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
	if (effect.to->isKongcheng()) return;
	int card_id = room->askForCardChosen(effect.from, effect.to, "h", "duliang");
    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
    room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);
	if (room->askForChoice(effect.from, "duliang", "send+delay") == "delay")
		room->addPlayerMark(effect.to, "duliang_delay");
	else {
		QList<int> ids = room->getNCards(2, false);
		LogMessage log;
        log.type = "$ViewDrawPile";
        log.from = effect.to;
        log.card_str = IntList2StringList(ids).join("+");
        room->sendLog(log, effect.to);
		room->fillAG(ids, effect.to);
        room->getThread()->delay();
        room->clearAG(effect.to);
		room->returnToTopDrawPile(ids);
		QList<int> to_obtain;
        foreach (int card_id, ids)
			if (Sanguosha->getCard(card_id)->getTypeId() == Card::TypeBasic)
				to_obtain << card_id;
		if (!to_obtain.isEmpty()) {
			DummyCard *dummy = new DummyCard(to_obtain);
            effect.to->obtainCard(dummy, false);
			delete dummy;
		}
	}
}

class DuliangViewAsSkill : public ZeroCardViewAsSkill
{
public:
    DuliangViewAsSkill() : ZeroCardViewAsSkill("duliang")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("DuliangCard");
    }

    virtual const Card *viewAs() const
    {
        return new DuliangCard;
    }
};

class Duliang : public DrawCardsSkill
{
public:
    Duliang() : DrawCardsSkill("duliang")
    {
        view_as_skill = new DuliangViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target && target->getMark("duliang_delay") > 0;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        int x = player->getMark("duliang_delay");
		player->setMark("duliang_delay", 0);
        return n + x;
    }
};

class FulinDiscard : public ViewAsSkill
{
public:
    FulinDiscard() : ViewAsSkill("fulin_discard")
    {
        response_pattern = "@@fulin_discard!";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
		QStringList fulin = Self->property("fulin").toString().split("+");
        foreach (QString id, fulin) {
            bool ok;
            if (id.toInt(&ok) == to_select->getEffectiveId() && ok)
                return false;
        }
        return !Self->isJilei(to_select) && !to_select->isEquipped() && selected.length() < Self->getMark("fulin_discard");
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == Self->getMark("fulin_discard")) {
            DummyCard *xt = new DummyCard;
            xt->addSubcards(cards);
            return xt;
        }

        return NULL;
    }
};

class Fulin : public TriggerSkill
{
public:
    Fulin() : TriggerSkill("fulin")
    {
        events << EventPhaseProceeding << CardsMoveOneTime << EventPhaseChanging;
		frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseProceeding && player->getPhase() == Player::Discard && TriggerSkill::triggerable(player)){
			if (player->property("fulin").toString() == "") return false;
			QStringList fulin_list = player->property("fulin").toString().split("+");
			int num = 0;
			foreach (const Card *card, player->getHandcards())
                if (!fulin_list.contains(QString::number(card->getEffectiveId())))
					num++;
            int discard_num = num - player->getMaxCards();
			if (player->getHandcardNum() > player->getMaxCards() && player->getHandcardNum() > num){
				room->sendCompulsoryTriggerLog(player, objectName());
		        room->broadcastSkillInvoke(objectName());
			}
			if (discard_num > 0) {
				QList<int> default_ids;
			    bool will_ask = false;
				foreach (const Card *card, player->getHandcards()) {
                    if (!fulin_list.contains(QString::number(card->getEffectiveId())) && !player->isJilei(card)){
					    if (default_ids.length() < discard_num)
						    default_ids << card->getEffectiveId();
						else {
							will_ask = true;
							break;
						}
				    }
                }
				const Card *card = NULL;
				if (will_ask) {
					room->setPlayerMark(player, "fulin_discard", discard_num);
				    card = room->askForCard(player, "@@fulin_discard!", "@fulin-discard:::" + QString::number(discard_num), data, Card::MethodNone);
				    room->setPlayerMark(player, "fulin_discard", 0);
				}
				if (card == NULL || card->subcardsLength() != discard_num)
					card = new DummyCard(default_ids);
                CardMoveReason mreason(CardMoveReason::S_REASON_THROW, player->objectName(), QString(), "gamerule", QString());
                room->throwCard(card, mreason, player);
			}
			room->setTag("SkipGameRule", true);
		} else if (triggerEvent == CardsMoveOneTime && player->getPhase() != Player::NotActive) {
			CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
			if (!room->getTag("FirstRound").toBool() && move.to == player && move.to_place == Player::PlaceHand) {
                QStringList fulin_list;
				if (player->property("fulin").toString() != "")
					fulin_list = player->property("fulin").toString().split("+");
                fulin_list << IntList2StringList(move.card_ids);
                room->setPlayerProperty(player, "fulin", fulin_list.join("+"));
			} else if (move.from == player && move.from_places.contains(Player::PlaceHand) && player->property("fulin").toString() != "") {
				QStringList fulin_list = player->property("fulin").toString().split("+");
				QStringList copy_list = fulin_list;
				foreach (QString id_str, fulin_list) {
					int id = id_str.toInt();
					if (move.card_ids.contains(id) && move.from_places[move.card_ids.indexOf(id)] == Player::PlaceHand)
						copy_list.removeOne(id_str);
				}
				room->setPlayerProperty(player, "fulin", copy_list.join("+"));
			}
		} else if (triggerEvent == EventPhaseChanging) {
			PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
			room->setPlayerProperty(player, "fulin", QString());
		}
        return false;
    }
};

KuangbiCard::KuangbiCard()
{
}

bool KuangbiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->isNude() && to_select != Self;
}

void KuangbiCard::onEffect(const CardEffectStruct &effect) const
{
    if (effect.to->isNude()) return;
	const Card *card = effect.from->getRoom()->askForExchange(effect.to, "kuangbi", 3, 1, true, "@kuangbi-put:" + effect.from->objectName(), false);
    effect.from->addToPile("kuangbi", card->getSubcards(), false);
	effect.from->tag["KuangbiTarget"] = QVariant::fromValue(effect.to);
}

class KuangbiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    KuangbiViewAsSkill() : ZeroCardViewAsSkill("kuangbi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("KuangbiCard");
    }

    virtual const Card *viewAs() const
    {
        return new KuangbiCard;
    }
};

class Kuangbi : public PhaseChangeSkill
{
public:
    Kuangbi() : PhaseChangeSkill("kuangbi")
    {
        view_as_skill = new KuangbiViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target && !target->getPile("kuangbi").isEmpty() && target->getPhase() == Player::RoundStart;
    }

    virtual bool onPhaseChange(ServerPlayer *sundeng) const
    {
        Room *room = sundeng->getRoom();

        int n = sundeng->getPile("kuangbi").length();
		DummyCard *dummy = new DummyCard(sundeng->getPile("kuangbi"));
        CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, sundeng->objectName(), "kuangbi", QString());
        room->obtainCard(sundeng, dummy, reason, false);
        delete dummy;
		ServerPlayer *target = sundeng->tag["KuangbiTarget"].value<ServerPlayer *>();
		sundeng->tag.remove("KuangbiTarget");
		if (target)
			target->drawCards(n);

        return false;
    }
};

JisheCard::JisheCard()
{
    target_fixed = true;
}

void JisheCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->drawCards(1, "jishe");
	room->addPlayerMark(source, "JisheTimes");
}

JisheChainCard::JisheChainCard()
{
    handling_method = Card::MethodNone;
    m_skillName = "jishe";
}

bool JisheChainCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() < Self->getHp() && !to_select->isChained();
}

void JisheChainCard::onEffect(const CardEffectStruct &effect) const
{
    if (!effect.to->isChained())
        effect.to->getRoom()->setPlayerProperty(effect.to, "chained", true);
}

class JisheViewAsSkill : public ZeroCardViewAsSkill
{
public:
    JisheViewAsSkill() : ZeroCardViewAsSkill("jishe")
    {
    }

    virtual const Card *viewAs() const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern() == "@@jishe")
            return new JisheChainCard;
        else
            return new JisheCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMaxCards() > 0;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "@@jishe";
    }
};

class Jishe : public TriggerSkill
{
public:
    Jishe() : TriggerSkill("jishe")
    {
        events << EventPhaseChanging << EventPhaseStart;
        view_as_skill = new JisheViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
		if (triggerEvent == EventPhaseChanging) {
			if (data.value<PhaseChangeStruct>().to != Player::NotActive) return false;
            room->setPlayerMark(player, "JisheTimes", 0);
		} else if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish && player->isKongcheng()) {
			QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!p->isChained())
                    targets << p;
            }
			int x = qMin(targets.length(), player->getHp());
			if (x > 0)
                room->askForUseCard(player, "@@jishe", "@jishe-chain:::" + QString::number(x), -1, Card::MethodNone);
		}
        return false;
    }
};

class JisheMaxcards : public MaxCardsSkill
{
public:
    JisheMaxcards() : MaxCardsSkill("#jishe")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        return - target->getMark("JisheTimes");
    }
};

class Lianhuo : public TriggerSkill
{
public:
    Lianhuo() : TriggerSkill("lianhuo")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
		DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature == DamageStruct::Fire && player->isChained() && !damage.chain) {
            room->sendCompulsoryTriggerLog(player, objectName());
            room->broadcastSkillInvoke(objectName());

            ++damage.damage;
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};

class Qinqing : public PhaseChangeSkill
{
public:
    Qinqing() : PhaseChangeSkill("qinqing")
    {
        view_as_skill = new dummyVS;
    }

    virtual bool onPhaseChange(ServerPlayer *huanghao) const
    {
        Room *room = huanghao->getRoom();
		if (!isNormalGameMode(room->getMode())) return false;
		if (huanghao->getPhase() != Player::Finish) return false;
		ServerPlayer *lord = room->getLord();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(huanghao)) {
            if (p->inMyAttackRange(lord) && huanghao->canDiscard(p, "he"))
                targets << p;
        }
		if (targets.isEmpty()) return false;
		ServerPlayer *target = room->askForPlayerChosen(huanghao, targets, objectName(), "@qinqing", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
			room->throwCard(room->askForCardChosen(huanghao, target, "he", objectName(), false, Card::MethodDiscard), target, huanghao);
            target->drawCards(1, objectName());
			if (target->getHandcardNum() > lord->getHandcardNum())
				huanghao->drawCards(1, objectName());
        }
        return false;
    }
};

class HuishengViewAsSkill : public ViewAsSkill
{
public:
    HuishengViewAsSkill() : ViewAsSkill("huisheng")
    {
        response_pattern = "@@huisheng";
    }

    bool viewFilter(const QList<const Card *> &, const Card *) const
    {
        return true;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() > 0) {
            DummyCard *xt = new DummyCard;
            xt->addSubcards(cards);
            return xt;
        }

        return NULL;
    }
};

class HuishengObtain : public OneCardViewAsSkill
{
public:
    HuishengObtain() : OneCardViewAsSkill("huisheng_obtain")
    {
        expand_pile = "#huisheng";
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.startsWith("@@huisheng_obtain");
    }

    bool viewFilter(const Card *to_select) const
    {
        return Self->getPile("#huisheng").contains(to_select->getEffectiveId());
    }

    const Card *viewAs(const Card *originalCard) const
    {
        return originalCard;
    }
};

class Huisheng : public TriggerSkill
{
public:
    Huisheng() : TriggerSkill("huisheng")
    {
        events << DamageInflicted;
        view_as_skill = new HuishengViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *huanghao, QVariant &data) const
    {
		ServerPlayer *target = data.value<DamageStruct>().from;
		if (huanghao->isNude() || !target || target->isDead() || target == huanghao || target->getMark("huisheng" + huanghao->objectName()) > 0) return false;
	const Card *card = room->askForCard(huanghao, "@@huisheng", "@huisheng-show::" + target->objectName(), data, Card::MethodNone);
		if (card){
			LogMessage log;
            log.type = "#InvokeSkill";
            log.arg = objectName();
            log.from = huanghao;
            room->sendLog(log);
            room->notifySkillInvoked(huanghao, objectName());
            room->broadcastSkillInvoke(objectName());
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, huanghao->objectName(), target->objectName());
			room->addPlayerMark(target, "huisheng" + huanghao->objectName());
			QList<int> to_show = card->getSubcards();
			int n = card->subcardsLength();
			room->notifyMoveToPile(target, to_show, "huisheng", Player::PlaceTable, true, true);
			QString prompt = "@huisheng-obtain1:" + huanghao->objectName() + "::" + QString::number(n);
			QString pattern = "@@huisheng_obtain";
			bool optional = true;
			if (target->forceToDiscard(n, true).length() < n) {
				optional = false;
                pattern = "@@huisheng_obtain!";
				prompt = "@huisheng-obtain2:" + huanghao->objectName();
			}
			const Card *to_obtain = room->askForCard(target, pattern, prompt, data, Card::MethodNone);
			room->notifyMoveToPile(target, to_show, "huisheng", Player::PlaceTable, false, false);
			if (to_obtain) {
				target->obtainCard(to_obtain);
                return true;
			} else if (optional)
				room->askForDiscard(target, "huisheng", n, n, false, true);
			else {
				target->obtainCard(Sanguosha->getCard(to_show.first()));
                return true;
			}
		}
        return false;
    }
};

class Guizao : public TriggerSkill
{
public:
    Guizao() : TriggerSkill("guizao")
    {
        events << CardsMoveOneTime << EventPhaseEnd << EventPhaseChanging;
		view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime && player->getPhase() == Player::Discard) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            QVariantList guizaoRecord = player->tag["GuizaoRecord"].toList();
            if (move.from == player && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                foreach (int card_id, move.card_ids) {
                    guizaoRecord << card_id;
                }
            }
            player->tag["GuizaoRecord"] = guizaoRecord;
        } else if (triggerEvent == EventPhaseEnd && player->getPhase() == Player::Discard && TriggerSkill::triggerable(player)) {
            QVariantList guizaoRecord = player->tag["GuizaoRecord"].toList();
			if (guizaoRecord.length() < 2) return false;
			QStringList suitlist;
			foreach (QVariant card_data, guizaoRecord) {
                int card_id = card_data.toInt();
                const Card *card = Sanguosha->getCard(card_id);
                QString suit = card->getSuitString();
                if (!suitlist.contains(suit))
                    suitlist << suit;
                else{
					return false;
				}
            }
			if (room->askForSkillInvoke(player, "skill_ask", "prompt:::" + objectName())){
			    QStringList choices;
			    if (player->isWounded())
			    	choices << "recover";
			    choices << "draw" << "cancel";
		        QString choice = room->askForChoice(player, objectName(), choices.join("+"), data);
                if (choice != "cancel") {
                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = player;
                     log.arg = objectName();
                    room->sendLog(log);

                    room->notifySkillInvoked(player, objectName());
                    room->broadcastSkillInvoke(objectName());
                    if (choice == "recover")
                        room->recover(player, RecoverStruct(player));
                    else
                        player->drawCards(1, objectName());
                }
		    }
        } else if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::Discard) {
                player->tag.remove("GuizaoRecord");
            }
        }

        return false;
    }
};

JiyuCard::JiyuCard()
{
    
}

bool JiyuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !Self->hasFlag("jiyu" + to_select->objectName()) && !to_select->isKongcheng();
}

void JiyuCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
	ServerPlayer *target = effect.to;
	Room *room = source->getRoom();
	room->setPlayerFlag(source, "jiyu" + target->objectName());
	if (target->canDiscard(target, "h")) {
		const Card *c = room->askForCard(target, ".!", "@jiyu-discard");
        if (c == NULL) {
            c = target->getCards("h").at(0);
            room->throwCard(c, target);
        }
		room->setPlayerCardLimitation(source, "use", QString(".|%1|.|.").arg(c->getSuitString()), true);
		if (c->getSuit() == Card::Spade) {
			source->turnOver();
        	room->loseHp(target);
		}
	}
}

class Jiyu : public ZeroCardViewAsSkill
{
public:
    Jiyu() : ZeroCardViewAsSkill("jiyu")
    {
    }

    virtual const Card *viewAs() const
    {
        return new JiyuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
		foreach (const Card *card, player->getHandcards()) {
            if (card->isAvailable(player))
                return true;
        }
        return false;
    }
};

TaoluanDialog::TaoluanDialog() : GuhuoDialog("taoluan", true, true)
{

}

TaoluanDialog *TaoluanDialog::getInstance()
{
    static TaoluanDialog *instance;
    if (instance == NULL || instance->objectName() != "taoluan")
        instance = new TaoluanDialog;

    return instance;
}

bool TaoluanDialog::isButtonEnabled(const QString &button_name) const
{
    const Card *c = map[button_name];
    QString classname = c->getClassName();
    if (c->isKindOf("Slash"))
        classname = "Slash";

    bool r = Self->getMark("Taoluan_" + classname) == 0;
    if (!r)
        return false;

    return GuhuoDialog::isButtonEnabled(button_name);
}

TaoluanCard::TaoluanCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool TaoluanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    }

    const Card *_card = Self->tag.value("taoluan").value<const Card *>();
    if (_card == NULL)
        return false;

    Card *card = Sanguosha->cloneCard(_card->objectName(), Card::NoSuit, 0);
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool TaoluanCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFixed();
    }

    const Card *_card = Self->tag.value("taoluan").value<const Card *>();
    if (_card == NULL)
        return false;

    Card *card = Sanguosha->cloneCard(_card->objectName(), Card::NoSuit, 0);
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetFixed();
}

bool TaoluanCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetsFeasible(targets, Self);
    }

    const Card *_card = Self->tag.value("taoluan").value<const Card *>();
    if (_card == NULL)
        return false;

    Card *card = Sanguosha->cloneCard(_card->objectName(), Card::NoSuit, 0);
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetsFeasible(targets, Self);
}

const Card *TaoluanCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *zhangrang = card_use.from;
    Room *room = zhangrang->getRoom();

    QString to_guhuo = user_string;
    if (user_string == "slash" && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        QStringList guhuo_list;
        guhuo_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            guhuo_list = QStringList() << "thunder_slash" << "fire_slash";
        to_guhuo = room->askForChoice(zhangrang, "taoluan_slash", guhuo_list.join("+"));
    }

    Card *c = Sanguosha->cloneCard(to_guhuo, Card::NoSuit, 0);

    QString classname;
    if (c->isKindOf("Slash"))
        classname = "Slash";
    else
        classname = c->getClassName();

    room->setPlayerMark(zhangrang, "Taoluan_" + classname, 1);

    QStringList taoluanList = zhangrang->tag.value("taoluanClassName").toStringList();
    taoluanList << classname;
    zhangrang->tag["taoluanClassName"] = taoluanList;

    c->setSkillName("taoluan");
    c->deleteLater();
    return c;
}

const Card *TaoluanCard::validateInResponse(ServerPlayer *zhangrang) const
{
    Room *room = zhangrang->getRoom();

    QString to_guhuo = user_string;
    if (user_string == "peach+analeptic") {
        bool can_use_peach = zhangrang->getMark("Taoluan_Peach") == 0;
        bool can_use_analeptic = zhangrang->getMark("Taoluan_Analeptic") == 0;
        QStringList guhuo_list;
        if (can_use_peach)
            guhuo_list << "peach";
        if (can_use_analeptic && !Config.BanPackages.contains("maneuvering"))
            guhuo_list << "analeptic";
        to_guhuo = room->askForChoice(zhangrang, "taoluan_saveself", guhuo_list.join("+"));
    } else if (user_string == "slash") {
        QStringList guhuo_list;
        guhuo_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            guhuo_list = QStringList() << "thunder_slash" << "fire_slash";
        to_guhuo = room->askForChoice(zhangrang, "taoluan_slash", guhuo_list.join("+"));
    } else
        to_guhuo = user_string;

    Card *c = Sanguosha->cloneCard(to_guhuo, Card::NoSuit, 0);

    QString classname;
    if (c->isKindOf("Slash"))
        classname = "Slash";
    else
        classname = c->getClassName();

    room->setPlayerMark(zhangrang, "Taoluan_" + classname, 1);

    QStringList taoluanList = zhangrang->tag.value("taoluanClassName").toStringList();
    taoluanList << classname;
    zhangrang->tag["taoluanClassName"] = taoluanList;

    c->setSkillName("taoluan");
    c->deleteLater();
    return c;

}

class TaoluanVS : public ZeroCardViewAsSkill
{
public:
    TaoluanVS() : ZeroCardViewAsSkill("taoluan")
    {
        
    }

    const Card *viewAs() const
    {
        QString pattern;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            const Card *c = Self->tag["taoluan"].value<const Card *>();
            if (c == NULL || Self->getMark("Taoluan_" + (c->isKindOf("Slash") ? "Slash" : c->getClassName())) > 0)
                return NULL;

            pattern = c->objectName();
        } else {
            pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "peach+analeptic" && Self->getMark("Global_PreventPeach") > 0)
                pattern = "analeptic";

            // check if it can use
            bool can_use = false;
            QStringList p = pattern.split("+");
            foreach (const QString &x, p) {
                const Card *c = Sanguosha->cloneCard(x);
                QString us = c->getClassName();
                if (c->isKindOf("Slash"))
                    us = "Slash";

                if (Self->getMark("Taoluan_" + us) == 0)
                    can_use = true;

                delete c;
                if (can_use)
                    break;
            }

            if (!can_use)
                return NULL;
        }

        TaoluanCard *hm = new TaoluanCard;
        hm->setUserString(pattern);
        return hm;
        
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return true; // for DIY!!!!!!!
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return false;

#define TAOLUAN_CAN_USE(x) (player->getMark("Taoluan_" #x) == 0)

        if (pattern == "slash")
            return TAOLUAN_CAN_USE(Slash);
        else if (pattern == "peach")
            return TAOLUAN_CAN_USE(Peach) && player->getMark("Global_PreventPeach") == 0;
        else if (pattern.contains("analeptic"))
            return TAOLUAN_CAN_USE(Peach) || TAOLUAN_CAN_USE(Analeptic);
        else if (pattern == "jink")
            return TAOLUAN_CAN_USE(Jink);
		else if (pattern == "nullification")
            return TAOLUAN_CAN_USE(Nullification);

        return false;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return TAOLUAN_CAN_USE(Nullification);

#undef TAOLUAN_CAN_USE

    }
};

class Taoluan : public TriggerSkill
{
public:
    Taoluan() : TriggerSkill("taoluan")
    {
        view_as_skill = new TaoluanVS;
        events << CardFinished;
    }

    QDialog *getDialog() const
    {
        return TaoluanDialog::getInstance();
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *zhangrang, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
		if (use.card->getSkillName() != objectName()) return false;
        ServerPlayer *target = room->askForPlayerChosen(zhangrang, room->getOtherPlayers(zhangrang), objectName(), "@taoluan-choose");
		QString type_name[4] = { QString(), "BasicCard", "TrickCard", "EquipCard" };
		QStringList types;
        types << "BasicCard" << "TrickCard" << "EquipCard";
        types.removeOne(type_name[use.card->getTypeId()]);
		const Card *card = room->askForCard(target, types.join(",") + "|.|.|.", 
		        "@taoluan-give:" + zhangrang->objectName() + "::" + use.card->getType(), data, Card::MethodNone);
		if (card) {
			CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), zhangrang->objectName(), objectName(), QString());
            reason.m_playerId = zhangrang->objectName();
            room->moveCardTo(card, target, zhangrang, Player::PlaceHand, reason);
            delete card;
		} else
			room->loseHp(zhangrang);

        return false;
    }

};


YJCM2016Package::YJCM2016Package()
: Package("YJCM2016")
{
    General *guohuanghou = new General(this, "guohuanghou", "wei", 3, false);
    guohuanghou->addSkill(new Jiaozhao);
	guohuanghou->addSkill(new JiaozhaoProhibit);
	related_skills.insertMulti("jiaozhao", "#jiaozhao");
    guohuanghou->addSkill(new Danxin);

	General *liuyu = new General(this, "liuyu", "qun", 2);
	liuyu->addSkill(new Zhige);
	liuyu->addSkill(new Zongzuo);

	General *liyan = new General(this, "liyan", "shu", 3);
	liyan->addSkill(new Duliang);
	liyan->addSkill(new Fulin);

	General *sundeng = new General(this, "sundeng", "wu");
	sundeng->addSkill(new Kuangbi);

	General *cenhun = new General(this, "cenhun", "wu", 3);
	cenhun->addSkill(new Jishe);
	cenhun->addSkill(new JisheMaxcards);
	related_skills.insertMulti("jishe", "#jishe");
	cenhun->addSkill(new Lianhuo);

	General *sunziliufang = new General(this, "sunziliufang", "wei", 3);
	sunziliufang->addSkill(new Guizao);
	sunziliufang->addSkill(new Jiyu);

	General *huanghao = new General(this, "huanghao", "shu", 3);
	huanghao->addSkill(new Qinqing);
	huanghao->addSkill(new Huisheng);

	General *zhangrang = new General(this, "zhangrang", "qun", 3);
	zhangrang->addSkill(new Taoluan);

	addMetaObject<JiaozhaoCard>();
	addMetaObject<ZhigeCard>();
	addMetaObject<DuliangCard>();
	addMetaObject<KuangbiCard>();
	addMetaObject<JisheCard>();
	addMetaObject<JisheChainCard>();
	addMetaObject<JiyuCard>();
	addMetaObject<TaoluanCard>();

	skills << new FulinDiscard << new HuishengObtain;
}


ADD_PACKAGE(YJCM2016)
