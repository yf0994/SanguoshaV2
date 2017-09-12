#include "assassins.h"
#include "skill.h"
#include "standard.h"
#include "clientplayer.h"
#include "engine.h"

MixinCard::MixinCard()
{
    will_throw = false;
    mute = true;
    handling_method = Card::MethodNone;
}

bool MixinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self;
}

void MixinCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = source->getRoom();
    room->broadcastSkillInvoke("mixin", 1);
    room->addPlayerMark(effect.from, "mixinengine");
    if (effect.from->getMark("mixinengine") > 0) {
		target->obtainCard(this, false);
		QList<ServerPlayer *> others;
		foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
			if (target->canSlash(p, NULL, false))
				others << p;
		}
		if (others.isEmpty())
			return;

		ServerPlayer *target2 = room->askForPlayerChosen(source, others, "mixin");
		LogMessage log;
		log.type = "#CollateralSlash";
		log.from = source;
		log.to << target2;
		room->sendLog(log);
		room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, target->objectName(), target2->objectName());
		if (room->askForUseSlashTo(target, target2, "#mixin", false)) {
			room->broadcastSkillInvoke("mixin", 2);
		} else {
			room->broadcastSkillInvoke("mixin", 3);
			QList<int> card_ids = target->handCards();
			room->fillAG(card_ids, target2);
			int cdid = room->askForAG(target2, card_ids, false, objectName());
			room->obtainCard(target2, cdid, false);
			room->clearAG(target2);
		}
        room->removePlayerMark(effect.from, "mixinengine");
    }
    return;
}

class Mixin :public OneCardViewAsSkill
{
public:
    Mixin() :OneCardViewAsSkill("mixin")
    {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("MixinCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        MixinCard *card = new MixinCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Cangni : public TriggerSkill
{
public:
    Cangni() :TriggerSkill("cangni")
    {
        events << EventPhaseStart << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Discard && player->askForSkillInvoke(objectName())) {
            QStringList choices;
            choices << "draw";
            if (player->isWounded())
                choices << "recover";

            QString choice;
            if (choices.size() == 1)
                choice = choices.first();
            else
                choice = room->askForChoice(player, objectName(), choices.join("+"));

			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				if (choice == "recover") {
					RecoverStruct recover;
					recover.who = player;
					room->recover(player, recover);
				} else
					player->drawCards(2, "cangni");

				room->broadcastSkillInvoke("cangni", 1);
				player->turnOver();
				room->removePlayerMark(player, objectName()+"engine");
			}
            return false;
        } else if (triggerEvent == CardsMoveOneTime && !player->faceUp()) {
            if (player->getPhase() != Player::NotActive)
                return false;

            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *target = room->getCurrent();
            if (target->isDead())
                return false;

            if (move.from == player && move.to != player) {
                bool invoke = false;
                for (int i = 0; i < move.card_ids.size(); i++) {
                    if (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip) {
                        invoke = true;
                        break;
                    }
                }
                room->setPlayerFlag(player, "cangnilose");    //for AI

                if (invoke && !target->isNude() && player->askForSkillInvoke(objectName())) {
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
                    room->broadcastSkillInvoke("cangni", 3);
					room->addPlayerMark(player, objectName()+"engine");
					if (player->getMark(objectName()+"engine") > 0) {
						room->askForDiscard(target, objectName(), 1, 1, false, true);
						room->removePlayerMark(player, objectName()+"engine");
					}
                }

                room->setPlayerFlag(player, "-cangnilose");    //for AI

                return false;
            }

            if (move.to == player && move.from != player) {
                if (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip) {
                    room->setPlayerFlag(player, "cangniget");    //for AI

                    if (!target->hasFlag("cangni_used") && player->askForSkillInvoke(objectName())) {
                        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
                        room->broadcastSkillInvoke("cangni", 2);
						room->addPlayerMark(player, objectName()+"engine");
						if (player->getMark(objectName()+"engine") > 0) {
							room->setPlayerFlag(target, "cangni_used");
							target->drawCards(1, "cangni");
							room->removePlayerMark(player, objectName()+"engine");
						}
                    }

                    room->setPlayerFlag(player, "-cangniget");    //for AI
                }
            }
        }

        return false;
    }
};

DuyiCard::DuyiCard()
{
    target_fixed = true;
    mute = true;
}

void DuyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->addPlayerMark(source, "dyiengine");
    if (source->getMark("dyiengine") > 0) {
		QList<int> card_ids = room->getNCards(1);
		int id = card_ids.first();
		room->fillAG(card_ids, NULL);
		room->getThread()->delay();
		ServerPlayer *target = room->askForPlayerChosen(source, room->getAlivePlayers(), "duyi");
		room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), target->objectName());
		const Card *card = Sanguosha->getCard(id);
		target->obtainCard(card);
		if (card->isBlack()) {
			room->setPlayerCardLimitation(target, "use,response", ".|.|.|hand", false);
			room->setPlayerMark(target, "duyi_target", 1);
			LogMessage log;
			log.type = "#duyi_eff";
			log.from = source;
			log.to << target;
			log.arg = "duyi";
			room->sendLog(log);
			room->broadcastSkillInvoke("duyi", 1);
		} else
			room->broadcastSkillInvoke("duyi", 2);

		room->getThread()->delay();
		room->clearAG();
        room->removePlayerMark(source, "dyiengine");
    }
}

class DuyiViewAsSkill :public ZeroCardViewAsSkill
{
public:
    DuyiViewAsSkill() :ZeroCardViewAsSkill("duyi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("DuyiCard");
    }

    virtual const Card *viewAs() const
    {
        return new DuyiCard;
    }
};

class Duyi :public TriggerSkill
{
public:
    Duyi() :TriggerSkill("duyi")
    {
        view_as_skill = new DuyiViewAsSkill;
        events << EventPhaseChanging << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasInnateSkill(objectName());
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return false;
        } else {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        }

        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->getMark("duyi_target") > 0) {
                room->removePlayerCardLimitation(p, "use,response", ".|.|.|hand$0");
                room->setPlayerMark(p, "duyi_target", 0);
                LogMessage log;
                log.type = "#duyi_clear";
                log.from = p;
                log.arg = objectName();
                room->sendLog(log);
            }
        }

        return false;
    }
};

class Duanzhi : public TriggerSkill
{
public:
    Duanzhi() : TriggerSkill("duanzhi")
    {
        events << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeSkill || use.from == player || !use.to.contains(player))
            return false;

        if (player->askForSkillInvoke(objectName(), data)) {
			room->broadcastSkillInvoke(objectName());
			room->addPlayerMark(player, objectName()+"engine");
			if (player->getMark(objectName()+"engine") > 0) {
				room->setPlayerFlag(player, "duanzhi_InTempMoving");
				ServerPlayer *target = use.from;
				DummyCard *dummy = new DummyCard;
				QList<int> card_ids;
				QList<Player::Place> original_places;
				for (int i = 0; i < 2; i++) {
					if (!player->canDiscard(target, "he"))
						break;
					if (room->askForChoice(player, objectName(), "discard+cancel") == "cancel")
						break;
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
					card_ids << room->askForCardChosen(player, target, "he", objectName());
					original_places << room->getCardPlace(card_ids[i]);
					dummy->addSubcard(card_ids[i]);
					target->addToPile("#duanzhi", card_ids[i], false);
				}

				if (dummy->subcardsLength() > 0)
					for (int i = 0; i < dummy->subcardsLength(); i++)
						room->moveCardTo(Sanguosha->getCard(card_ids[i]), target, original_places[i], false);

				room->setPlayerFlag(player, "-duanzhi_InTempMoving");

				if (dummy->subcardsLength() > 0)
					room->throwCard(dummy, target, player);
				delete dummy;
				room->loseHp(player);
				room->removePlayerMark(player, objectName()+"engine");
			}
        }
        return false;
    }
};

class Fengyin :public TriggerSkill
{
public:
    Fengyin() :TriggerSkill("fengyin")
    {
        events << EventPhaseChanging << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
        if (!splayer || splayer == player)
            return false;

        if (triggerEvent == EventPhaseChanging && data.value<PhaseChangeStruct>().to == Player::Start
            && player->getHp() >= splayer->getHp()) {
            const Card *card = room->askForCard(splayer, "Slash|.|.|hand", "@fengyin", QVariant(), Card::MethodNone);
            if (card) {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, splayer->objectName(), player->objectName());
                player->obtainCard(card);
                room->broadcastSkillInvoke("fengyin");
				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					room->setPlayerFlag(player, "fengyin_target");
					room->removePlayerMark(player, objectName()+"engine");
				}
            }
        }

        if (triggerEvent == EventPhaseStart && player->hasFlag("fengyin_target")) {
            player->skip(Player::Play);
            player->skip(Player::Discard);
        }

        return false;
    }
};

class ChizhongKeep : public MaxCardsSkill
{
public:
    ChizhongKeep() :MaxCardsSkill("chizhong")
    {
    }

    virtual int getFixed(const Player *target) const
    {
        if (target->hasSkill(objectName()))
            return target->getMaxHp();
        else
            return -1;
    }
};

class Chizhong : public TriggerSkill
{
public:
    Chizhong() :TriggerSkill("#chizhong")
    {
        events << Death;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who == player)
            return false;

        room->broadcastSkillInvoke("chizhong", 2);
		room->addPlayerMark(player, objectName()+"engine");
		if (player->getMark(objectName()+"engine") > 0) {
			room->setPlayerProperty(player, "maxhp", player->getMaxHp() + 1);
			// @todo_P: log message here
			LogMessage log;
			log.type = "#TriggerSkill";
			log.from = player;
			log.arg = "chizhong";
			room->sendLog(log);
			room->removePlayerMark(player, objectName()+"engine");
		}

        return false;
    }
};

AssassinsPackage::AssassinsPackage(): Package("assassins") {
    General *fuhuanghou = new General(this, "as_fuhuanghou", "qun", 3, false);
    fuhuanghou->addSkill(new Mixin);
    fuhuanghou->addSkill(new Cangni);

    General *jiben = new General(this, "as_jiben", "qun", 3);
    jiben->addSkill(new Duyi);
    jiben->addSkill(new Duanzhi);
    jiben->addSkill(new FakeMoveSkill("duanzhi"));
    related_skills.insertMulti("duanzhi", "#duanzhi-fake-move");

    General *mushun = new General(this, "as_mushun", "qun", 3);
    mushun->addSkill(new Fengyin);
    mushun->addSkill(new ChizhongKeep);
    mushun->addSkill(new Chizhong);
    related_skills.insertMulti("chizhong", "#chizhong");
	
    addMetaObject<MixinCard>();
    addMetaObject<DuyiCard>();
}

ADD_PACKAGE(Assassins)
