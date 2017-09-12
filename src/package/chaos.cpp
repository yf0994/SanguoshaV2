#include "chaos.h"
#include "settings.h"

Fentian::Fentian(): PhaseChangeSkill("fentian"){
    frequency = Compulsory;
}

bool Fentian::onPhaseChange(ServerPlayer *hanba) const{
    if (hanba->getPhase() != Player::Finish)
        return false;

    if (hanba->getHandcardNum() >= hanba->getHp())
        return false;

    QList<ServerPlayer*> targets;
    Room* room = hanba->getRoom();
    foreach(ServerPlayer* p, room->getOtherPlayers(hanba)){
        if (hanba->inMyAttackRange(p) && !p->isNude())
            targets << p;
    }

    if (targets.isEmpty())
        return false;

    room->broadcastSkillInvoke(objectName());
    room->addPlayerMark(hanba, objectName()+"engine");
    if (hanba->getMark(objectName()+"engine") > 0) {
		ServerPlayer *target = room->askForPlayerChosen(hanba, targets, objectName());
		int id = room->askForCardChosen(hanba, target, "he", objectName());
		hanba->addToPile("burn", id);
        room->removePlayerMark(hanba, objectName()+"engine");
    }
    return false;
}

class FentianRange: public AttackRangeSkill{
public:
    FentianRange(): AttackRangeSkill("#fentian"){
    }

    virtual int getExtra(const Player *target, bool ) const{
        if (target->hasSkill(objectName()))
            return target->getPile("burn").length();

        return 0;
    }
};

Zhiri::Zhiri(): PhaseChangeSkill("zhiri") {
    frequency = Wake;
}

bool Zhiri::onPhaseChange(ServerPlayer *player) const {
    if (player->getMark(objectName()) > 0 || player->getPhase() != Player::Start)
        return false;

    if (player->getPile("burn").length() < 3)
        return false;

    Room *room = player->getRoom();
    room->broadcastSkillInvoke(objectName());
    //room->doLightbox("$ZhiriAnimate", 4000);

	room->addPlayerMark(player, objectName()+"engine");
	if (player->getMark(objectName()+"engine") > 0) {
		if (room->changeMaxHpForAwakenSkill(player)) {
			room->acquireSkill(player, "xintan");
			room->addPlayerMark(player, objectName());
		}
        room->removePlayerMark(player, objectName()+"engine");
    }

    return false;
};

XintanCard::XintanCard() {
    will_throw = true;
    handling_method = Card::MethodNone;
}

bool XintanCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const {
    return targets.isEmpty();
}

void XintanCard::onEffect(const CardEffectStruct &effect) const {
    ServerPlayer *hanba = effect.from;
    QList<int> burn = hanba->getPile("burn");
    if (burn.length() < 2)
        return;

    Room *room = hanba->getRoom();
    QList<int> subs;

    if (burn.length() == 2)
        subs = burn;
    else {
        int aidelay = Config.AIDelay;
        Config.AIDelay = 0;
        while (subs.length() < 2) {
            room->fillAG(burn, hanba);
            int id = room->askForAG(hanba, burn, false, objectName());
            subs << id;
            burn.removeOne(id);
            room->clearAG(hanba);
        }
        Config.AIDelay = aidelay;
    };
    CardsMoveStruct move;
    move.from = hanba;
    move.to_place = Player::DiscardPile;
    move.reason = CardMoveReason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, hanba->objectName(), objectName(), QString());
    move.card_ids = subs;
    room->moveCardsAtomic(move, true);

    room->addPlayerMark(effect.from, "engine");
    if (effect.from->getMark("engine") > 0) {
		room->loseHp(effect.to);
        room->removePlayerMark(effect.from, "engine");
    }
}

class Xintan: public ZeroCardViewAsSkill {
public:
    Xintan(): ZeroCardViewAsSkill("xintan") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getPile("burn").length() >= 2 && !player->hasUsed("XintanCard");
    }

    virtual const Card *viewAs() const{
        return new XintanCard;
    }
};

ChaosPackage::ChaosPackage()
    : Package("Chaos")
{
    General *hanba = new General(this, "hanba", "qun", 4, false);
    hanba->addSkill(new Fentian);
    hanba->addSkill(new FentianRange);
    hanba->addSkill(new Zhiri);
    hanba->addRelateSkill("xintan");
    related_skills.insertMulti("fentian", "#fentian");

    addMetaObject<XintanCard>();

    skills << new Xintan;
}

ADD_PACKAGE(Chaos)
