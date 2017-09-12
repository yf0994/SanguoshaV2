#include "thicket.h"
#include "general.h"
#include "skill.h"
#include "room.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "settings.h"

class Xingshang: public TriggerSkill {
public:
    Xingshang(): TriggerSkill("xingshang") {
        events << Death;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *caopi, QVariant &data) const{
        DeathStruct death = data.value<DeathStruct>();
        ServerPlayer *player = death.who;
        if (player->isNude() || caopi == player)
            return false;
        if (caopi->isAlive() && room->askForSkillInvoke(caopi, objectName(), data)) {
			if (Config.value("music", true).toBool()) {
				room->broadcastSkillInvoke(objectName(), qrand() % 2 + 1);
			} else {
				 if (player->getGeneralName().contains("zhenji")){
					room->broadcastSkillInvoke(objectName(), 3);
					//room->doLightbox("$XingshangAnimate");
					} else if(player->getGeneral()->isMale())
						room->broadcastSkillInvoke(objectName(), 1);
					else
						room->broadcastSkillInvoke(objectName(), 2);
			}
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, caopi->objectName(), player->objectName());
            room->addPlayerMark(caopi, objectName()+"engine");
            if (caopi->getMark(objectName()+"engine") > 0) {
                DummyCard *dummy = new DummyCard(player->handCards());
                QList <const Card *> equips = player->getEquips();
                foreach(const Card *card, equips)
                    dummy->addSubcard(card);

                if (dummy->subcardsLength() > 0) {
                    CardMoveReason reason(CardMoveReason::S_REASON_RECYCLE, caopi->objectName());
                    room->obtainCard(caopi, dummy, reason, false);
                }
                delete dummy;
                room->removePlayerMark(caopi, objectName()+"engine");
            }
        }

        return false;
    }
};

class Fangzhu: public MasochismSkill {
public:
    Fangzhu(): MasochismSkill("fangzhu") {
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &) const{
        Room *room = target->getRoom();
        ServerPlayer *to = room->askForPlayerChosen(target, room->getOtherPlayers(target), objectName(),
                                                    "fangzhu-invoke", target->getMark("JilveEvent") != int(Damaged), true);
        if (to) {
            if (target->hasInnateSkill("fangzhu") || !target->hasSkill("jilve")) {
                int index = to->faceUp() ? 1 : 2;
                if (!Config.value("music", true).toBool() && to->getGeneralName().contains("caozhi") || (to->getGeneral2() && to->getGeneral2Name().contains("caozhi")))
                    index = 3;
                room->broadcastSkillInvoke("fangzhu", index);
            } else {
                room->broadcastSkillInvoke("jilve", 2);
			}
            room->addPlayerMark(target, objectName()+"engine");
            if (target->getMark(objectName()+"engine") > 0) {
                to->drawCards(target->getLostHp(), objectName());
                to->turnOver();
                room->removePlayerMark(target, objectName()+"engine");
            }
        }
    }
};

class Songwei: public TriggerSkill {
public:
    Songwei(): TriggerSkill("songwei$") {
        events << FinishJudge;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getKingdom() == "wei";
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        JudgeStruct *judge = data.value<JudgeStruct *>();
        const Card *card = judge->card;

        if (card->isBlack()) {
            QList<ServerPlayer *> caopis;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasLordSkill(objectName()))
                    caopis << p;
            }

            while (!caopis.isEmpty()) {
                ServerPlayer *caopi = room->askForPlayerChosen(player, caopis, objectName(), "@songwei-to", true);
                if (caopi) {
                    if (!caopi->isLord() && caopi->hasSkill("weidi")){
                        room->broadcastSkillInvoke("weidi");
					} else {
						room->broadcastSkillInvoke(objectName(), Config.value("music", true).toBool() ? NULL : player->isMale() ? 1 : 2);
					}
                    room->notifySkillInvoked(caopi, objectName());
                    LogMessage log;
                    log.type = "#InvokeOthersSkill";
                    log.from = player;
                    log.to << caopi;
                    log.arg = objectName();
                    room->sendLog(log);

                    room->addPlayerMark(player, objectName()+"engine");
                    if (player->getMark(objectName()+"engine") > 0) {
                        caopi->drawCards(1, objectName());
                        room->removePlayerMark(player, objectName()+"engine");
                    }
                    caopis.removeOne(caopi);
                } else
                    break;
            }
        }

        return false;
    }
};

class Duanliang: public OneCardViewAsSkill {
public:
    Duanliang(): OneCardViewAsSkill("duanliang") {
        filter_pattern = "BasicCard,EquipCard|black";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        SupplyShortage *shortage = new SupplyShortage(originalCard->getSuit(), originalCard->getNumber());
        shortage->setSkillName(objectName());
        shortage->addSubcard(originalCard);

        return shortage;
    }
};

class DuanliangTargetMod: public TargetModSkill {
public:
    DuanliangTargetMod(): TargetModSkill("#duanliang-target") {
        frequency = NotFrequent;
        pattern = "SupplyShortage";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const{
        if (from->hasSkill("duanliang"))
            return 1;
        else
            return 0;
    }
};

class SavageAssaultAvoid: public TriggerSkill {
public:
    SavageAssaultAvoid(const QString &avoid_skill)
        : TriggerSkill("#sa_avoid_" + avoid_skill), avoid_skill(avoid_skill)
    {
        events << CardEffected;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.card->isKindOf("SavageAssault") && player->hasSkill(avoid_skill)) {
			room->broadcastSkillInvoke(avoid_skill, 1);

            LogMessage log;
            log.type = "#SkillNullify";
            log.from = player;
            log.arg = avoid_skill;
            log.arg2 = "savage_assault";
            room->sendLog(log);

            room->addPlayerMark(player, avoid_skill+"engine");
            if (player->getMark(avoid_skill+"engine") > 0) {
                room->removePlayerMark(player, avoid_skill+"engine");
                return true;
            }
        }
        return false;
    }

private:
    QString avoid_skill;
};

class Huoshou: public TriggerSkill {
public:
    Huoshou(): TriggerSkill("huoshou") {
        events << TargetSpecified << ConfirmDamage;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("SavageAssault")) {
                ServerPlayer *menghuo = room->findPlayerBySkillName(objectName());
                if (menghuo && menghuo != use.from) {
					room->broadcastSkillInvoke(objectName(), 2);
                    room->sendCompulsoryTriggerLog(player, objectName());

                    room->addPlayerMark(menghuo, objectName()+"engine");
                    if (menghuo->getMark(objectName()+"engine") > 0) {
                        use.card->setFlags("HuoshouDamage_" + menghuo->objectName());
                        room->removePlayerMark(menghuo, objectName()+"engine");
                    }
                }
            }
        } else if (triggerEvent == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.card || !damage.card->isKindOf("SavageAssault"))
                return false;

            ServerPlayer *menghuo = NULL;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (damage.card->hasFlag("HuoshouDamage_" + p->objectName())) {
                    menghuo = p;
                    break;
                }
            }
            if (!menghuo) return false;
            damage.from = menghuo->isAlive() ? menghuo : NULL;
            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

class Lieren: public TriggerSkill {
public:
    Lieren(): TriggerSkill("lieren") {
        events << Damage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        if (damage.card && damage.card->isKindOf("Slash") && !player->isKongcheng() && !target->isKongcheng() && !target->hasFlag("Global_DebutFlag") && !damage.chain && !damage.transfer && target != player && room->askForSkillInvoke(player, objectName(), data)) {
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
            if (player->hasSkill("jiwu")){
                room->broadcastSkillInvoke(objectName(), 4);
            } else {
                room->broadcastSkillInvoke(objectName(), 1);
            }

            if (!target->isNude()) {
                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {
                    bool success = player->pindian(target, "lieren", NULL);
                    if (!success) {
						if (Config.value("music", true).toBool())
							room->broadcastSkillInvoke(objectName(), 3);
                        room->removePlayerMark(player, objectName()+"engine");
                        return false;
                    }
                    if (player->hasSkill("jiwu")) {
                        room->broadcastSkillInvoke(objectName(), 5);
                    } else {
                        room->broadcastSkillInvoke(objectName(), 2);
                    }
					if (!target->isNude()) {
						int card_id = room->askForCardChosen(player, target, "he", objectName(), false, Card::MethodNone, QList<int>());
						CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
						room->obtainCard(player, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);
						room->removePlayerMark(player, objectName()+"engine");
					}
                }
            }
        }

        return false;
    }
};

class Zaiqi: public PhaseChangeSkill {
public:
    Zaiqi(): PhaseChangeSkill("zaiqi") {
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        if (player->getPhase() == Player::Draw && player->isWounded()) {
            Room *room = player->getRoom();
            if (room->askForSkillInvoke(player, objectName())) {
                room->broadcastSkillInvoke(objectName(), 1);

                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {
                    bool has_heart = false;
                    int x = player->getLostHp();
                    QList<int> ids = room->getNCards(x, false);
                    CardsMoveStruct move(ids, player, Player::PlaceTable,
                        CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), "zaiqi", QString()));
                    room->moveCardsAtomic(move, true);

                    room->getThread()->delay();
                    room->getThread()->delay();

                    QList<int> card_to_throw;
                    QList<int> card_to_gotback;
                    for (int i = 0; i < x; i++) {
                        if (Sanguosha->getCard(ids[i])->getSuit() == Card::Heart)
                            card_to_throw << ids[i];
                        else
                            card_to_gotback << ids[i];
                    }
                    if (!card_to_throw.isEmpty()) {
                        room->recover(player, RecoverStruct(player, NULL, card_to_throw.length()));

                        DummyCard *dummy = new DummyCard(card_to_throw);
                        CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "zaiqi", QString());
                        room->throwCard(dummy, reason, NULL);
                        delete dummy;
                        has_heart = true;
                    }
                    if (!card_to_gotback.isEmpty()) {
                        DummyCard *dummy2 = new DummyCard(card_to_gotback);
                        room->obtainCard(player, dummy2);
                        delete dummy2;
                    }

                    if (has_heart)
                        room->broadcastSkillInvoke(objectName(), 2);
                    else
						if (!Config.value("music", true).toBool())
							room->broadcastSkillInvoke(objectName(), 3);

                    room->removePlayerMark(player, objectName()+"engine");
                }
                return true;
            }
        }

        return false;
    }
};

class Juxiang: public TriggerSkill {
public:
    Juxiang(): TriggerSkill("juxiang") {
        events << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.card_ids.length() == 1 && move.from_places.contains(Player::PlaceTable) && move.to_place == Player::DiscardPile
            && move.reason.m_reason == CardMoveReason::S_REASON_USE) {
                const Card *card = move.reason.m_extraData.value<const Card *>();
                if (!card || !card->isKindOf("SavageAssault"))
                    return false;
                if (!Config.OL && card->isVirtualCard()) {
                    if (card->getSkillName() != "guhuo" && card->getSkillName() != "nosguhuo")
                        return false;
                }
                if (player != move.from) {
					room->broadcastSkillInvoke(objectName(), 2);
                    room->sendCompulsoryTriggerLog(player, objectName());

					room->addPlayerMark(player, objectName()+"engine");
					if (player->getMark(objectName()+"engine") > 0) {
						player->obtainCard(card);
						move.removeCardIds(move.card_ids);
						room->removePlayerMark(player, objectName()+"engine");
					}
                    data = QVariant::fromValue(move);
                }
        }
        return false;
    }
};

class Yinghun: public PhaseChangeSkill {
public:
    Yinghun(): PhaseChangeSkill("yinghun") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Start
               && target->isWounded();
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        ServerPlayer *to = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "yinghun-invoke", true, true);
        if (to) {
            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                int x = player->getLostHp();

                int index = 1;
                if (!player->hasInnateSkill("yinghun") && player->hasSkill("hunzi"))
                    index += 2;

                if (x == 1) {
                    room->broadcastSkillInvoke(objectName(), index);

                    to->drawCards(1, objectName());
                    room->askForDiscard(to, objectName(), 1, 1, false, true);
                } else {
                    to->setFlags("YinghunTarget");
                    QString choice = room->askForChoice(player, objectName(), "d1tx+dxt1");
                    to->setFlags("-YinghunTarget");
                    if (choice == "d1tx") {
                        room->broadcastSkillInvoke(objectName(), index + 1);

                        to->drawCards(1, objectName());
                        room->askForDiscard(to, objectName(), x, x, false, true);
                    } else {
                        room->broadcastSkillInvoke(objectName(), index);

                        to->drawCards(x, objectName());
                        room->askForDiscard(to, objectName(), 1, 1, false, true);
                    }
                }
                room->removePlayerMark(player, objectName()+"engine");
            }
        }
        return false;
    }
};

HaoshiCard::HaoshiCard() {
    will_throw = false;
    mute = true;
    handling_method = Card::MethodNone;
    m_skillName = "_haoshi";
}

bool HaoshiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    return to_select->getHandcardNum() == Self->getMark("haoshi");
}

void HaoshiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(),
                          targets.first()->objectName(), "haoshi", QString());
    room->moveCardTo(this, targets.first(), Player::PlaceHand, reason);
}

class HaoshiViewAsSkill: public ViewAsSkill {
public:
    HaoshiViewAsSkill(): ViewAsSkill("haoshi") {
        response_pattern = "@@haoshi!";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (to_select->isEquipped())
            return false;

        int length = Self->getHandcardNum() / 2;
        return selected.length() < length;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() != Self->getHandcardNum() / 2)
            return NULL;

        HaoshiCard *card = new HaoshiCard;
        card->addSubcards(cards);
        return card;
    }
};

class HaoshiGive: public TriggerSkill {
public:
    HaoshiGive(): TriggerSkill("#haoshi-give") {
        events << AfterDrawNCards;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *lusu, QVariant &) const{
        if (lusu->hasFlag("haoshi")) {
            lusu->setFlags("-haoshi");

            if (lusu->getHandcardNum() <= 5)
                return false;

            QList<ServerPlayer *> other_players = room->getOtherPlayers(lusu);
            int least = 1000;
            foreach (ServerPlayer *player, other_players)
                least = qMin(player->getHandcardNum(), least);
            room->setPlayerMark(lusu, "haoshi", least);
            bool used = room->askForUseCard(lusu, "@@haoshi!", "@haoshi", -1, Card::MethodNone);

            if (!used) {
                // force lusu to give his half cards
                ServerPlayer *beggar = NULL;
                foreach (ServerPlayer *player, other_players) {
                    if (player->getHandcardNum() == least) {
                        beggar = player;
                        break;
                    }
                }

                int n = lusu->getHandcardNum() / 2;
                QList<int> to_give = lusu->handCards().mid(0, n);
                HaoshiCard *haoshi_card = new HaoshiCard;
                haoshi_card->addSubcards(to_give);
                QList<ServerPlayer *> targets;
                targets << beggar;
                haoshi_card->use(room, lusu, targets);
                delete haoshi_card;
            }
        }

        return false;
    }
};

class Haoshi: public DrawCardsSkill {
public:
    Haoshi(): DrawCardsSkill("#haoshi") {
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        if (room->askForSkillInvoke(player, "haoshi")) {
            room->broadcastSkillInvoke("haoshi");
            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                player->setFlags("haoshi");
                room->removePlayerMark(player, objectName()+"engine");
                return n + 2;
            }
        }
        return n;
    }
};

DimengCard::DimengCard() {
}

bool DimengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (to_select == Self)
        return false;

    if (targets.isEmpty())
        return true;

    if (targets.length() == 1) {
        return qAbs(to_select->getHandcardNum() - targets.first()->getHandcardNum()) == subcardsLength();
    }

    return false;
}

bool DimengCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    return targets.length() == 2;
}

#include "jsonutils.h"
void DimengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *a = targets.at(0);
    ServerPlayer *b = targets.at(1);
    a->setFlags("DimengTarget");
    b->setFlags("DimengTarget");

    int n1 = a->getHandcardNum();
    int n2 = b->getHandcardNum();

    try {
        room->addPlayerMark(source, "dimengengine");
        if (source->getMark("dimengengine") > 0) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p != a && p != b) {
                room->doNotify(p, QSanProtocol::S_COMMAND_EXCHANGE_KNOWN_CARDS, QSanProtocol::Utils::toJsonArray(a->objectName(), b->objectName()));
                }
            }
            QList<CardsMoveStruct> exchangeMove;
            QList<int> a_ids = a->handCards();
            QList<int> b_ids = b->handCards();
            if ((a->hasSkill("wanwei") || a->getMark("wanwei") != 0) && room->askForSkillInvoke(a, "wanwei")) {
                room->broadcastSkillInvoke("wanwei");
                const Card *exchange_card = room->askForExchange(a, "dimeng", a->getHandcardNum(), a->getHandcardNum(), true, "@wanwei!");
                a_ids.clear();
                foreach(int i, exchange_card->getSubcards())
                    a_ids << i;
            } else if ((b->hasSkill("wanwei") || b->getMark("wanwei") != 0) && room->askForSkillInvoke(b, "wanwei")) {
                room->broadcastSkillInvoke("wanwei");
                const Card *exchange_card = room->askForExchange(b, "dimeng", b->getHandcardNum(), b->getHandcardNum(), true, "@wanwei!");
                b_ids.clear();
                foreach(int i, exchange_card->getSubcards())
                    b_ids << i;
            }
            CardsMoveStruct move1(a_ids, b, Player::PlaceHand,
                CardMoveReason(CardMoveReason::S_REASON_SWAP, a->objectName(), b->objectName(), "dimeng", QString()));
            CardsMoveStruct move2(b_ids, a, Player::PlaceHand,
                CardMoveReason(CardMoveReason::S_REASON_SWAP, b->objectName(), a->objectName(), "dimeng", QString()));
            exchangeMove.push_back(move1);
            exchangeMove.push_back(move2);
            room->moveCardsAtomic(exchangeMove, false);

            LogMessage log;
            log.type = "#Dimeng";
            log.from = a;
            log.to << b;
            log.arg = QString::number(n1);
            log.arg2 = QString::number(n2);
            room->sendLog(log);
            room->getThread()->delay();

            a->setFlags("-DimengTarget");
            b->setFlags("-DimengTarget");
            room->removePlayerMark(source, "dimengengine");
        }
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange) {
            a->setFlags("-DimengTarget");
            b->setFlags("-DimengTarget");
        }
        throw triggerEvent;
    }
}

class Dimeng: public ViewAsSkill {
public:
    Dimeng(): ViewAsSkill("dimeng") {
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        return !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        DimengCard *card = new DimengCard;
        foreach (const Card *c, cards)
            card->addSubcard(c);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("DimengCard");
    }
};

class Wansha: public TriggerSkill {
public:
    Wansha(): TriggerSkill("wansha") {
        // just to broadcast audio effects and to send log messages
        // main part in the AskForPeaches trigger of Game Rule
        events << AskForPeaches;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual int getPriority(TriggerEvent) const{
        return 7;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player == room->getAllPlayers().first()) {
            DyingStruct dying = data.value<DyingStruct>();
            ServerPlayer *jiaxu = room->getCurrent();
            if (!jiaxu || !TriggerSkill::triggerable(jiaxu) || jiaxu->getPhase() == Player::NotActive)
                return false;
			int index = qrand() % 2 + 1;
            if (jiaxu->hasInnateSkill("wansha") || !jiaxu->hasSkill("jilve") || !jiaxu->hasSkill("jiwu")){
                room->broadcastSkillInvoke(objectName(), index);
            } else if (jiaxu->hasSkill("jiwu")){
                room->broadcastSkillInvoke(objectName(), index + 2);
            } else {
                room->broadcastSkillInvoke("jilve", 3);
            }
            
            room->notifySkillInvoked(jiaxu, objectName());

            LogMessage log;
            log.from = jiaxu;
            log.arg = objectName();
            if (jiaxu != dying.who) {
                log.type = "#WanshaTwo";
                log.to << dying.who;
            } else {
                log.type = "#WanshaOne";
            }
            room->sendLog(log);
        }
        return false;
    }
};

class Luanwu: public ZeroCardViewAsSkill {
public:
    Luanwu(): ZeroCardViewAsSkill("luanwu") {
        frequency = Limited;
        limit_mark = "@chaos";
    }

    virtual const Card *viewAs() const{
        return new LuanwuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@chaos") >= 1;
    }
};

LuanwuCard::LuanwuCard() {
    mute = true;
    target_fixed = true;
}

void LuanwuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    foreach (ServerPlayer *player, room->getOtherPlayers(source)) {
		room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), player->objectName());
    }
    room->broadcastSkillInvoke("luanwu");
    //QString lightbox = "$LuanwuAnimate";
    //if (source->getGeneralName() != "jiaxu" && (source->getGeneralName() == "sp_jiaxu" || source->getGeneral2Name() == "sp_jiaxu"))
        //lightbox = lightbox + "SP";
    //room->doLightbox(lightbox, 3000);


    room->addPlayerMark(source, "luanwuengine");
    if (source->getMark("luanwuengine") > 0) {
        room->removePlayerMark(source, "@chaos");
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach (ServerPlayer *player, players) {
            if (player->isAlive())
                room->cardEffect(this, source, player);
        }
        room->removePlayerMark(source, "luanwuengine");
    }
}

void LuanwuCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    QList<ServerPlayer *> players = room->getOtherPlayers(effect.to);
    QList<int> distance_list;
    int nearest = 1000;
    foreach (ServerPlayer *player, players) {
        int distance = effect.to->distanceTo(player);
        distance_list << distance;
        nearest = qMin(nearest, distance);
    }

    QList<ServerPlayer *> luanwu_targets;
    for (int i = 0; i < distance_list.length(); ++i) {
        if (distance_list[i] == nearest && effect.to->canSlash(players[i], NULL, false))
            luanwu_targets << players[i];
    }

    if (luanwu_targets.isEmpty() || !room->askForUseSlashTo(effect.to, luanwu_targets, "@luanwu-slash"))
        room->loseHp(effect.to);
}

class Weimu: public ProhibitSkill {
public:
    Weimu(): ProhibitSkill("weimu") {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const{
        return to->hasSkill(objectName()) && (card->isKindOf("TrickCard") || card->isKindOf("QiceCard"))
               && card->isBlack() && card->getSkillName() != "nosguhuo"; // Be care!!!!!!
    }
};

class Jiuchi: public OneCardViewAsSkill {
public:
    Jiuchi(): OneCardViewAsSkill("jiuchi") {
        filter_pattern = ".|spade|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Analeptic::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return  pattern.contains("analeptic");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Analeptic *analeptic = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        analeptic->setSkillName(objectName());
        analeptic->addSubcard(originalCard->getId());
        return analeptic;
    }
};

class Roulin: public TriggerSkill {
public:
    Roulin(): TriggerSkill("roulin") {
        events << TargetConfirmed << TargetSpecified;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            QVariantList jink_list = use.from->tag["Jink_" + use.card->toString()].toList();
            int index = 0;
            if (triggerEvent == TargetSpecified) {
                bool play_effect = false;
                foreach (ServerPlayer *p, use.to) {
                    if (p->isFemale()) {
                        if (jink_list.at(index).toInt() == 1) {
							room->addPlayerMark(player, objectName()+"engine");
                            if (player->getMark(objectName()+"engine") > 0) {
								jink_list.replace(index, QVariant(2));
								play_effect = true;
                                room->removePlayerMark(player, objectName()+"engine");
								room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());
							}
						}
                    }
                    ++index;
                }
                use.from->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);

                if (play_effect) {
                    room->broadcastSkillInvoke(objectName(), 1);
                    room->sendCompulsoryTriggerLog(use.from, objectName());
                }
            } else if (triggerEvent == TargetConfirmed && use.from->isFemale()) {
                bool play_effect = false;
                foreach (ServerPlayer *p, use.to) {
                    if (p == player) {
                        if (jink_list.at(index).toInt() == 1) {
							room->addPlayerMark(player, objectName()+"engine");
                            if (player->getMark(objectName()+"engine") > 0) {
								jink_list.replace(index, QVariant(2));
								play_effect = true;
                                room->removePlayerMark(player, objectName()+"engine");
							}
						}
                    }
                    ++index;
                }
                use.from->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);

                if (play_effect) {
                    room->broadcastSkillInvoke(objectName(), 2);
                    room->sendCompulsoryTriggerLog(player, objectName());
                }
            }
        }

        return false;
    }
};

class Benghuai: public PhaseChangeSkill {
public:
    Benghuai(): PhaseChangeSkill("benghuai") {
        frequency = Compulsory;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        bool trigger_this = false;
        Room *room = target->getRoom();

        if (target->getPhase() == Player::Finish) {
            QList<ServerPlayer *> players = room->getOtherPlayers(target);
            foreach (ServerPlayer *player, players) {
                if (target->getHp() > player->getHp()) {
                    trigger_this = true;
                    break;
                }
            }
        }

        if (trigger_this) {
            room->sendCompulsoryTriggerLog(target, objectName());

            QString result = room->askForChoice(target, "benghuai", "hp+maxhp");
            int index = result == "hp" ? 1 : 2;
			if (!Config.value("music", true).toBool())
				index = (target->isFemale()) ? 2 : 1;
			if (!target->hasInnateSkill(objectName()) && target->getMark("juyi") > 0)
                index = 3;

            if (!target->hasInnateSkill(objectName()) && target->getMark("baoling") > 0)
                index = result == "hp" ? 4 : 5;

            room->broadcastSkillInvoke(objectName(), index);
            room->addPlayerMark(target, objectName()+"engine");
            if (target->getMark(objectName()+"engine") > 0) {
                if (result == "hp")
                    room->loseHp(target);
                else
                    room->loseMaxHp(target);
                
                room->removePlayerMark(target, objectName()+"engine");
            }
        }

        return false;
    }
};

class Baonue: public TriggerSkill {
public:
    Baonue(): TriggerSkill("baonue$") {
        events << Damage << PreDamageDone;
        global = true;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == PreDamageDone && damage.from)
            damage.from->tag["InvokeBaonue"] = damage.from->getKingdom() == "qun";
        else if (triggerEvent == Damage && player->tag.value("InvokeBaonue", false).toBool() && player->isAlive()) {
            QList<ServerPlayer *> dongzhuos;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasLordSkill(objectName()))
                    dongzhuos << p;
            }

            while (!dongzhuos.isEmpty()) {
                ServerPlayer *dongzhuo = room->askForPlayerChosen(player, dongzhuos, objectName(), "@baonue-to", true, true);
                if (dongzhuo) {
                    dongzhuos.removeOne(dongzhuo);

                    LogMessage log;
                    log.type = "#InvokeOthersSkill";
                    log.from = player;
                    log.to << dongzhuo;
                    log.arg = objectName();
                    room->sendLog(log);
                    room->notifySkillInvoked(dongzhuo, objectName());

                    room->addPlayerMark(player, objectName()+"engine");
                    if (player->getMark(objectName()+"engine") > 0) {
                        JudgeStruct judge;
                        judge.pattern = ".|spade";
                        judge.good = true;
                        judge.reason = objectName();
                        judge.who = player;

                        room->judge(judge);
						if (Config.value("music", true).toBool())
							room->broadcastSkillInvoke(objectName(), 1);
                        if (judge.isGood()) {
                            if (!dongzhuo->isLord() && dongzhuo->hasSkill("weidi"))
                                room->broadcastSkillInvoke("weidi");
                            else
								if (Config.value("music", true).toBool())
									room->broadcastSkillInvoke(objectName(), 2);
								else
									room->broadcastSkillInvoke(objectName(), 1);

                            room->recover(dongzhuo, RecoverStruct(player));
                        }
                        room->removePlayerMark(player, objectName()+"engine");
                    }
                } else
                    break;
            }
        }
        return false;
    }
};

ThicketPackage::ThicketPackage()
    : Package("thicket")
{
    General *xuhuang = new General(this, "xuhuang", "wei"); // WEI 010
    xuhuang->addSkill(new Duanliang);
    xuhuang->addSkill(new DuanliangTargetMod);
    related_skills.insertMulti("duanliang", "#duanliang-target");

    General *caopi = new General(this, "caopi$", "wei", 3); // WEI 014
    caopi->addSkill(new Xingshang);
    caopi->addSkill(new Fangzhu);
    caopi->addSkill(new Songwei);

    General *menghuo = new General(this, "menghuo", "shu"); // SHU 014
    menghuo->addSkill(new SavageAssaultAvoid("huoshou"));
    menghuo->addSkill(new Huoshou);
    menghuo->addSkill(new Zaiqi);
    related_skills.insertMulti("huoshou", "#sa_avoid_huoshou");

    General *zhurong = new General(this, "zhurong", "shu", 4, false); // SHU 015
    zhurong->addSkill(new SavageAssaultAvoid("juxiang"));
    zhurong->addSkill(new Juxiang);
    zhurong->addSkill(new Lieren);
    related_skills.insertMulti("juxiang", "#sa_avoid_juxiang");

    General *sunjian = new General(this, "sunjian", "wu"); // WU 009
    sunjian->addSkill(new Yinghun);

    General *lusu = new General(this, "lusu", "wu", 3); // WU 014
    lusu->addSkill(new Haoshi);
    lusu->addSkill(new HaoshiViewAsSkill);
    lusu->addSkill(new HaoshiGive);
    lusu->addSkill(new Dimeng);
    related_skills.insertMulti("haoshi", "#haoshi");
    related_skills.insertMulti("haoshi", "#haoshi-give");

    General *dongzhuo = new General(this, "dongzhuo$", "qun", 8); // QUN 006
    dongzhuo->addSkill(new Jiuchi);
    dongzhuo->addSkill(new Roulin);
    dongzhuo->addSkill(new Benghuai);
    dongzhuo->addSkill(new Baonue);

    General *jiaxu = new General(this, "jiaxu", "qun", 3); // QUN 007
    jiaxu->addSkill(new Wansha);
    jiaxu->addSkill(new Luanwu);
    jiaxu->addSkill(new Weimu);

    addMetaObject<DimengCard>();
    addMetaObject<LuanwuCard>();
    addMetaObject<HaoshiCard>();
}

ADD_PACKAGE(Thicket)
