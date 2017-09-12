#include "fire.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "engine.h"
#include "maneuvering.h"

QuhuCard::QuhuCard() {
    mute = true;
}

bool QuhuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->getHp() > Self->getHp() && !to_select->isKongcheng();
}

void QuhuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *tiger = targets.first();

    room->broadcastSkillInvoke("quhu", 1);

    room->addPlayerMark(source, "quhuengine");
    if (source->getMark("quhuengine") > 0) {
        bool success = source->pindian(tiger, "quhu", NULL);
        if (success) {
            room->broadcastSkillInvoke("quhu", 2);

            QList<ServerPlayer *> players = room->getOtherPlayers(tiger), wolves;
            foreach (ServerPlayer *source, players) {
                if (tiger->inMyAttackRange(source))
                    wolves << source;
            }

            if (wolves.isEmpty()) {
                LogMessage log;
                log.type = "#QuhuNoWolf";
                log.from = source;
                log.to << tiger;
                room->sendLog(log);

                return;
            }

            ServerPlayer *wolf = room->askForPlayerChosen(source, wolves, "quhu", QString("@quhu-damage:%1").arg(tiger->objectName()));
            room->damage(DamageStruct("quhu", tiger, wolf));
        } else {
            room->damage(DamageStruct("quhu", tiger, source));
        }
        room->removePlayerMark(source, "quhuengine");
    }
}

class Jieming: public MasochismSkill {
public:
    Jieming(): MasochismSkill("jieming") {
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const{
        Room *room = player->getRoom();
        for (int i = 0; i < damage.damage; i++) {
            ServerPlayer *to = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "jieming-invoke", true, true);
            if (!to) break;

            int upper = qMin(5, to->getMaxHp());
            int x = upper - to->getHandcardNum();
            if (x <= 0) continue;
			int index = 1;
			if (to == player)
				index = 2;
			room->broadcastSkillInvoke(objectName(), index);
            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                to->drawCards(x, objectName());
                if (!player->isAlive())
                    break;
                room->removePlayerMark(player, objectName()+"engine");
            }
        }
    }
};

class Quhu: public ZeroCardViewAsSkill {
public:
    Quhu(): ZeroCardViewAsSkill("quhu") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("QuhuCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const{
        return new QuhuCard;
    }
};

QiangxiCard::QiangxiCard() {
}

bool QiangxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    int rangefix = 0;
    if (!subcards.isEmpty() && Self->getWeapon() && Self->getWeapon()->getId() == subcards.first()) {
        const Weapon *card = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        rangefix += card->getRange() - Self->getAttackRange(false);
    }

    return Self->inMyAttackRange(to_select, rangefix);
}

void QiangxiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    if (subcards.isEmpty())
        room->loseHp(effect.from);

    room->addPlayerMark(effect.from, "qiangxiengine");
    if (effect.from->getMark("qiangxiengine") > 0) {
        room->damage(DamageStruct("qiangxi", effect.from, effect.to));
        room->removePlayerMark(effect.from, "qiangxiengine");
    }
}

class Qiangxi: public ViewAsSkill {
public:
    Qiangxi(): ViewAsSkill("qiangxi") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("QiangxiCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return selected.isEmpty() && to_select->isKindOf("Weapon") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
            return new QiangxiCard;
        else if (cards.length() == 1) {
            QiangxiCard *card = new QiangxiCard;
            card->addSubcards(cards);

            return card;
        } else
            return NULL;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const {
        int index = qrand() % 2 + 1;
        if (!player->hasInnateSkill(objectName()) && player->hasSkill("jiwu"))
            index += 2;
        return index;
    }
};

class Luanji: public ViewAsSkill {
public:
    Luanji(): ViewAsSkill("luanji") {
        response_or_use = true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (selected.isEmpty())
            return !to_select->isEquipped();
        else if (selected.length() == 1) {
            const Card *card = selected.first();
            return !to_select->isEquipped() && to_select->getSuit() == card->getSuit();
        } else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() == 2) {
            ArcheryAttack *aa = new ArcheryAttack(Card::SuitToBeDecided, 0);
            aa->addSubcards(cards);
            aa->setSkillName(objectName());
            return aa;
        } else
            return NULL;
    }
};

class Xueyi: public MaxCardsSkill {
public:
    Xueyi(): MaxCardsSkill("xueyi$") {
    }

    virtual int getExtra(const Player *target) const{
        //血裔发动时增加显示技能名与播放技能配音
        int extra = 0;
        if (target->hasLordSkill(objectName())) {
            QList<const Player *> players = target->getAliveSiblings();
            foreach (const Player *player, players) {
                if (player->getKingdom() == "qun")
                    extra += 2;
            }
        }
        if (extra > 0) {
            const ServerPlayer *player = qobject_cast<const ServerPlayer *>(target);
            if (NULL != player && Player::Discard == player->getPhase()
                && player->getHandcardNum() > player->getHp()) {
                Room *room = player->getRoom();
                room->broadcastSkillInvoke("xueyi");
                room->notifySkillInvoked(player, "xueyi");
            }
        }
        return extra;
    }
};

class ShuangxiongBViewAsSkill: public OneCardViewAsSkill {
public:
    ShuangxiongBViewAsSkill():OneCardViewAsSkill("shuangxiong_buff") {
        attached_lord_skill = true;
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("shuangxiong") != 0 && !player->isKongcheng();
    }

    virtual bool viewFilter(const Card *card) const{
        if (card->isEquipped())
            return false;

        int value = Self->getMark("shuangxiong");
        if (value == 1)
            return card->isBlack();
        else if (value == 2)
            return card->isRed();

        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Duel *duel = new Duel(originalCard->getSuit(), originalCard->getNumber());
        duel->addSubcard(originalCard);
        duel->setSkillName("shuangxiong");
        return duel;
    }
};

class Shuangxiong: public TriggerSkill {
public:
    Shuangxiong(): TriggerSkill("shuangxiong") {
        events << EventPhaseStart << FinishJudge << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Start) {
                room->setPlayerMark(player, "shuangxiong", 0);
            } else if (player->getPhase() == Player::Draw && TriggerSkill::triggerable(player)) {
                if (room->askForSkillInvoke(player, objectName(), data)) {
                    room->broadcastSkillInvoke("shuangxiong", 1);
                    room->addPlayerMark(player, objectName()+"engine");
                    if (player->getMark(objectName()+"engine") > 0) {
                        room->setPlayerFlag(player, "shuangxiong");

                        JudgeStruct judge;
                        judge.good = true;
                        judge.play_animation = false;
                        judge.reason = objectName();
                        judge.pattern = ".";
                        judge.who = player;

                        room->judge(judge);
                        room->setPlayerMark(player, "shuangxiong", judge.pattern == "red" ? 1 : 2);
                        room->setPlayerMark(player, "ViewAsSkill_shuangxiongEffect", 1);
                        room->attachSkillToPlayer(player, "shuangxiong_buff");

                        room->removePlayerMark(player, objectName()+"engine");
                    }
                    return true;
                }
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == "shuangxiong") {
                judge->pattern = (judge->card->isRed() ? "red" : "black");
                if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge)
                    player->obtainCard(judge->card);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && player->getMark("ViewAsSkill_shuangxiongEffect") > 0) {
                room->setPlayerMark(player, "ViewAsSkill_shuangxiongEffect", 0);
                room->detachSkillFromPlayer(player, "shuangxiong_buff", true);
			}
        }

        return false;
    }
    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        if (card->isKindOf("Duel"))
            return 2;
		return NULL;
    }
};

class Mengjin: public TriggerSkill {
public:
    Mengjin():TriggerSkill("mengjin") {
        events << SlashMissed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.to->isAlive() && player->canDiscard(effect.to, "he")) {
            if (room->askForSkillInvoke(player, objectName(), data)) {
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), effect.to->objectName());
                room->broadcastSkillInvoke(objectName());
                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {
                    int to_throw = room->askForCardChosen(player, effect.to, "he", objectName(), false, Card::MethodDiscard);
                    room->throwCard(Sanguosha->getCard(to_throw), effect.to, player);
                    room->removePlayerMark(player, objectName()+"engine");
                }
            }
        }
        return false;
    }
};

class Lianhuan: public OneCardViewAsSkill {
public:
    Lianhuan(): OneCardViewAsSkill("lianhuan") {
        filter_pattern = ".|club|.|hand";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IronChain *chain = new IronChain(originalCard->getSuit(), originalCard->getNumber());
        chain->addSubcard(originalCard);
        chain->setSkillName(objectName());
        return chain;
    }
};

class Niepan: public TriggerSkill {
public:
    Niepan(): TriggerSkill("niepan") {
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@nirvana";
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getMark("@nirvana") > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DyingStruct dying_data = data.value<DyingStruct>();
        if (dying_data.who != player)
            return false;

        if (room->askForSkillInvoke(player, objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            //room->doLightbox("$NiepanAnimate");
            //room->doSuperLightbox("pangtong", "niepan");


            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                room->removePlayerMark(player, "@nirvana");
                player->throwAllHandCardsAndEquips();
                QList<const Card *> tricks = player->getJudgingArea();
                foreach (const Card *trick, tricks) {
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName());
                    room->throwCard(trick, reason, NULL);
                }

                room->recover(player, RecoverStruct(player, NULL, 3 - player->getHp()));
                player->drawCards(3, objectName());

                if (player->isChained())
                    room->setPlayerProperty(player, "chained", false);

                if (!player->faceUp())
                    player->turnOver();
                
                room->removePlayerMark(player, objectName()+"engine");
            }
            if (player->getHp() > 0)
                return true;
        }
		return false;
    }
};

class Huoji: public OneCardViewAsSkill {
public:
    Huoji(): OneCardViewAsSkill("huoji") {
        filter_pattern = ".|red|.|hand";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        FireAttack *fire_attack = new FireAttack(originalCard->getSuit(), originalCard->getNumber());
        fire_attack->addSubcard(originalCard->getId());
        fire_attack->setSkillName(objectName());
        return fire_attack;
    }
};

class Bazhen: public TriggerSkill {
public:
    Bazhen(): TriggerSkill("bazhen") {
        frequency = Compulsory;
        events << CardAsked;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && !target->getArmor() && target->hasArmorEffect("eight_diagram");
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QString pattern = data.toStringList().first();

        if (pattern != "jink")
            return false;

        if (room->askForSkillInvoke(player, "eight_diagram", data)) {
            room->addPlayerMark(player, "bazhenengine", 2);
            if (player->getMark("bazhenengine") > 0) {
				room->setEmotion(player, "armor/eight_diagram");

				JudgeStruct judge;
				judge.pattern = ".|red";
				judge.good = true;
				judge.reason = objectName();
				judge.who = player;

				room->judge(judge);

				if (judge.isGood()) {
					Jink *jink = new Jink(Card::NoSuit, 0);
					jink->setSkillName(objectName());
					room->provide(jink);
					return true;
				}

				room->setTag("ArmorJudge", "eight_diagram");
            }
        }

        return false;
    }
};

class Kanpo : public OneCardViewAsSkill
{
public:
    Kanpo() : OneCardViewAsSkill("kanpo")
    {
        filter_pattern = ".|black|.|hand";
        response_pattern = "nullification";
        response_or_use = true;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Card *ncard = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        ncard->addSubcard(originalCard);
        ncard->setSkillName(objectName());
        return ncard;
    }

    bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return !player->isKongcheng() || !player->getHandPile().isEmpty();
    }
};


TianyiCard::TianyiCard() {
}

bool TianyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void TianyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->addPlayerMark(source, "tianyiengine");
    if (source->getMark("tianyiengine") > 0) {
        bool success = source->pindian(targets.first(), "tianyi", NULL);
        if (success)
            room->setPlayerFlag(source, "TianyiSuccess");
        else
            room->setPlayerCardLimitation(source, "use", "Slash", true);
        room->removePlayerMark(source, "tianyiengine");
    }
}

class Tianyi: public ZeroCardViewAsSkill {
public:
    Tianyi(): ZeroCardViewAsSkill("tianyi") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("TianyiCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const{
        return new TianyiCard;
    }
};

class TianyiTargetMod: public TargetModSkill {
public:
    TianyiTargetMod(): TargetModSkill("#tianyi-target") {
        frequency = NotFrequent;
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasFlag("TianyiSuccess"))
            return 1;
        else
            return 0;
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const{
        if (from->hasFlag("TianyiSuccess"))
            return 1000;
        else
            return 0;
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const{
        if (from->hasFlag("TianyiSuccess"))
            return 1;
        else
            return 0;
    }
};

FirePackage::FirePackage()
    : Package("fire")
{
    General *dianwei = new General(this, "dianwei", "wei"); // WEI 012
    dianwei->addSkill(new Qiangxi);

    General *xunyu = new General(this, "xunyu", "wei", 3); // WEI 013
    xunyu->addSkill(new Quhu);
    xunyu->addSkill(new Jieming);

    General *pangtong = new General(this, "pangtong", "shu", 3); // SHU 010
    pangtong->addSkill(new Lianhuan);
    pangtong->addSkill(new Niepan);

    General *wolong = new General(this, "wolong", "shu", 3); // SHU 011
    wolong->addSkill(new Huoji);
    wolong->addSkill(new Kanpo);
    wolong->addSkill(new Bazhen);

    General *taishici = new General(this, "taishici", "wu"); // WU 012
    taishici->addSkill(new Tianyi);
    taishici->addSkill(new TianyiTargetMod);
    related_skills.insertMulti("tianyi", "#tianyi-target");

    General *yuanshao = new General(this, "yuanshao$", "qun"); // QUN 004
    yuanshao->addSkill(new Luanji);
    yuanshao->addSkill(new Xueyi);

    General *yanliangwenchou = new General(this, "yanliangwenchou", "qun"); // QUN 005
    yanliangwenchou->addSkill(new Shuangxiong);

    General *pangde = new General(this, "pangde", "qun"); // QUN 008
    pangde->addSkill("mashu");
    pangde->addSkill(new Mengjin);

    addMetaObject<QuhuCard>();
    addMetaObject<QiangxiCard>();
    addMetaObject<TianyiCard>();
	skills << new ShuangxiongBViewAsSkill;
}

ADD_PACKAGE(Fire)
