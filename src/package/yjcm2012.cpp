#include "yjcm2012.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "maneuvering.h"
#include "settings.h"

class Zhenlie: public TriggerSkill {
public:
    Zhenlie(): TriggerSkill("zhenlie") {
        events << TargetConfirmed;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.to.contains(player) && use.from != player) {
                if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
                    if (room->askForSkillInvoke(player, objectName(), data)) {
                        room->broadcastSkillInvoke(objectName());
                        player->setFlags("-ZhenlieTarget");
                        player->setFlags("ZhenlieTarget");
                        room->loseHp(player);
                        if (player->isAlive() && player->hasFlag("ZhenlieTarget")) {
                            player->setFlags("-ZhenlieTarget");
							room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), use.from->objectName());
                            room->addPlayerMark(player, objectName()+"engine");
                            if (player->getMark(objectName()+"engine") > 0) {
                                use.nullified_list << player->objectName();
                                data = QVariant::fromValue(use);
                                if (player->canDiscard(use.from, "he")) {
                                    int id = room->askForCardChosen(player, use.from, "he", objectName(), false, Card::MethodDiscard);
                                    room->throwCard(id, use.from, player);
                                }
                                room->removePlayerMark(player, objectName()+"engine");
                            }
                        }
                    }
                }
            }
        }
        return false;
    }
};

class Miji: public TriggerSkill {
public:
    Miji(): TriggerSkill("miji") {
        events << EventPhaseStart << ChoiceMade;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (TriggerSkill::triggerable(player) && triggerEvent == EventPhaseStart
            && player->getPhase() == Player::Finish && player->isWounded() && player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                QStringList draw_num;
                for (int i = 1; i <= player->getLostHp(); draw_num << QString::number(i++)) {

                }
                int num = room->askForChoice(player, "miji_draw", draw_num.join("+")).toInt();
                player->drawCards(num, objectName());
                player->setMark(objectName(), 0);
                if (!player->isKongcheng()) {
                    forever {
                        int n = player->getMark(objectName());
                        if (n < num && !player->isKongcheng()) {
                            QList<int> handcards = player->handCards();
                            if (!room->askForYiji(player, handcards, objectName(), false, false, false, num - n))
                                break;
                        } else {
                            break;
                        }
                    }
                    // give the rest cards randomly
                    if (player->getMark(objectName()) < num && !player->isKongcheng()) {
                        int rest_num = num - player->getMark(objectName());
                        forever {
                            QList<int> handcard_list = player->handCards();
                            qShuffle(handcard_list);
                            int give = qrand() % rest_num + 1;
                            rest_num -= give;
                            QList<int> to_give = handcard_list.length() < give ? handcard_list : handcard_list.mid(0, give);
                            ServerPlayer *receiver = room->getOtherPlayers(player).at(qrand() % (player->aliveCount() - 1));
                            DummyCard *dummy = new DummyCard(to_give);
                            room->obtainCard(receiver, dummy, false);
                            delete dummy;
                            if (rest_num == 0 || player->isKongcheng())
                                break;
                        }
                    }
                }
                room->removePlayerMark(player, objectName()+"engine");
            }
        } else if (triggerEvent == ChoiceMade) {
            QString str = data.toString();
            if (str.startsWith("Yiji:" + objectName()))
                player->addMark(objectName(), str.split(":").last().split("+").length());
        }
        return false;
    }
};

QiceCard::QiceCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool QiceCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    const Card *card = Self->tag.value("qice").value<const Card *>();
    Card *mutable_card = Sanguosha->cloneCard(card);
    if (mutable_card) {
        mutable_card->addSubcards(this->subcards);
		mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
   }
    return mutable_card && mutable_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, mutable_card, targets);
}

bool QiceCard::targetFixed() const{
    const Card *card = Self->tag.value("qice").value<const Card *>();
    Card *mutable_card = Sanguosha->cloneCard(card);
    if (mutable_card) {
        mutable_card->addSubcards(this->subcards);
		mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetFixed();
}

bool QiceCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    const Card *card = Self->tag.value("qice").value<const Card *>();
    Card *mutable_card = Sanguosha->cloneCard(card);
    if (mutable_card) {
        mutable_card->addSubcards(this->subcards);
		mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetsFeasible(targets, Self);
}

const Card *QiceCard::validate(CardUseStruct &card_use) const{
    Card *use_card = Sanguosha->cloneCard(user_string);
    use_card->setSkillName("qice");
    use_card->addSubcards(this->subcards);
    bool available = true;
    foreach (ServerPlayer *to, card_use.to)
        if (card_use.from->isProhibited(to, use_card)) {
            available = false;
            break;
        }
    available = available && use_card->isAvailable(card_use.from);
    use_card->deleteLater();
    if (!available) return NULL;
    return use_card;
}

class Qice: public ViewAsSkill {
public:
    Qice(): ViewAsSkill("qice") {
    }

    virtual QDialog *getDialog() const{
        return GuhuoDialog::getInstance("qice", false);
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() < Self->getHandcardNum())
            return NULL;

        const Card *c = Self->tag.value("qice").value<const Card *>();
        if (c) {
            QiceCard *card = new QiceCard;
            card->setUserString(c->objectName());
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
       return !player->hasUsed("QiceCard") && !player->isKongcheng();
    }
};

class Zhiyu: public MasochismSkill {
public:
    Zhiyu(): MasochismSkill("zhiyu") {
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const{
        Room *room = target->getRoom();
        if (target->askForSkillInvoke(objectName(), QVariant::fromValue(damage))) {
            room->addPlayerMark(target, objectName()+"engine");
            if (target->getMark(objectName()+"engine") > 0) {
                target->drawCards(1, objectName());

                room->broadcastSkillInvoke(objectName(), 2);

                if (target->isKongcheng())
                    return;
                room->showAllCards(target);

                QList<const Card *> cards = target->getHandcards();
                Card::Color color = cards.first()->getColor();
                bool same_color = true;
                foreach (const Card *card, cards) {
                    if (card->getColor() != color) {
                        same_color = false;
                        break;
                    }
                }

                if (same_color && damage.from && damage.from->canDiscard(damage.from, "h")){
                    room->getThread()->delay();
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, target->objectName(), damage.from->objectName());
					room->broadcastSkillInvoke(objectName(), 1);
                    room->askForDiscard(damage.from, objectName(), 1, 1);
				}
                room->removePlayerMark(target, objectName()+"engine");
            }
        }
    }
};

class Jiangchi: public DrawCardsSkill {
public:
    Jiangchi(): DrawCardsSkill("jiangchi") {
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        QString choices = "jiang+cancel";
		if (n > 0)
			choices = "jiang+chi+cancel";
        QString choice = room->askForChoice(player, objectName(), choices);
        if (choice == "cancel")
            return n;

        room->notifySkillInvoked(player, objectName());
        room->addPlayerMark(player, objectName()+"engine");
        if (player->getMark(objectName()+"engine") > 0) {
            LogMessage log;
            log.from = player;
            log.arg = objectName();
            if (choice == "jiang") {
                log.type = "#Jiangchi1";
                room->broadcastSkillInvoke(objectName(), 1);
                room->setPlayerCardLimitation(player, "use,response", "Slash", true);
                n = n + 1;
            } else {
                log.type = "#Jiangchi2";
                room->broadcastSkillInvoke(objectName(), 2);
                room->setPlayerFlag(player, "JiangchiInvoke");
                n = n - 1;
            }
            room->sendLog(log);
            room->removePlayerMark(player, objectName()+"engine");
		}
		return n;
    }
};

class JiangchiTargetMod: public TargetModSkill {
public:
    JiangchiTargetMod(): TargetModSkill("#jiangchi-target") {
        frequency = NotFrequent;
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasFlag("JiangchiInvoke"))
            return 1;
        else
            return 0;
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const{
        if (from->hasFlag("JiangchiInvoke"))
            return 1000;
        else
            return 0;
    }
};

class Qianxi: public TriggerSkill {
public:
    Qianxi(): TriggerSkill("qianxi") {
        events << EventPhaseStart << FinishJudge;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Start) {
            if (room->askForSkillInvoke(player, objectName())) {
                room->addPlayerMark(player, objectName()+"engine");
                if (player->getMark(objectName()+"engine") > 0) {
                    JudgeStruct judge;
                    judge.reason = objectName();
                    judge.play_animation = false;
                    judge.who = player;

                    room->judge(judge);
                    if (!player->isAlive()) return false;
                    QString color = judge.pattern;
                    QList<ServerPlayer *> to_choose;
                    foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                        if (player->distanceTo(p) == 1)
                            to_choose << p;
                    }
                    if (to_choose.isEmpty()) {
						room->removePlayerMark(player, objectName()+"engine");
                        return false;
					}

                    ServerPlayer *victim = room->askForPlayerChosen(player, to_choose, objectName());
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), victim->objectName());
                    QString pattern = QString(".|%1|.|hand$0").arg(color);

                    room->broadcastSkillInvoke(objectName(), judge.card->isRed() ? 1 : 2);
                    room->setPlayerFlag(victim, "QianxiTarget");
                    room->addPlayerMark(victim, QString("@qianxi_%1").arg(color));
                    room->setPlayerCardLimitation(victim, "use,response", pattern, false);

                    LogMessage log;
                    log.type = "#Qianxi";
                    log.from = victim;
                    log.arg = QString("no_suit_%1").arg(color);
                    room->sendLog(log);
                    room->removePlayerMark(player, objectName()+"engine");
                }
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason != objectName() || !player->isAlive()) return false;

            QString color = judge->card->isRed() ? "red" : "black";
            player->tag[objectName()] = QVariant::fromValue(color);
            judge->pattern = color;
        }
        return false;
    }
};

class QianxiClear: public TriggerSkill {
public:
    QianxiClear(): TriggerSkill("#qianxi-clear") {
        events << EventPhaseChanging << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return !target->tag["qianxi"].toString().isNull();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return false;
        }

        QString color = player->tag["qianxi"].toString();
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->hasFlag("QianxiTarget")) {
                room->removePlayerCardLimitation(p, "use,response", QString(".|%1|.|hand$0").arg(color));
                room->setPlayerMark(p, QString("@qianxi_%1").arg(color), 0);
            }
        }
        return false;
    }
};

class Dangxian: public TriggerSkill {
public:
    Dangxian(): TriggerSkill("dangxian") {
        frequency = Compulsory;
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() == Player::RoundStart) {
            int n = qrand() % 2 + 1;
            if (player->hasSkill("zhengnan") && !player->hasInnateSkill(objectName())) {
                n = 3;
            }
            room->broadcastSkillInvoke(objectName(), n);
            room->sendCompulsoryTriggerLog(player, objectName());

            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                player->setPhase(Player::Play);
                room->broadcastProperty(player, "phase");
                RoomThread *thread = room->getThread();
                if (!thread->trigger(EventPhaseStart, room, player))
                    thread->trigger(EventPhaseProceeding, room, player);
                thread->trigger(EventPhaseEnd, room, player);

                player->setPhase(Player::RoundStart);
                room->broadcastProperty(player, "phase");
                room->removePlayerMark(player, objectName()+"engine");
			}
            
        }
        return false;
    }
};

class Fuli: public TriggerSkill {
public:
    Fuli(): TriggerSkill("fuli") {
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@laoji";
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getMark("@laoji") > 0;
    }

    int getKingdoms(Room *room) const{
        QSet<QString> kingdom_set;
        foreach (ServerPlayer *p, room->getAlivePlayers())
            kingdom_set << p->getKingdom();
        return kingdom_set.size();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DyingStruct dying_data = data.value<DyingStruct>();
        if (dying_data.who != player)
            return false;
        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            //room->doLightbox("$FuliAnimate", 3000);
            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                room->removePlayerMark(player, "@laoji");
                room->recover(player, RecoverStruct(player, NULL, getKingdoms(room) - player->getHp()));

                player->turnOver();
                
                room->removePlayerMark(player, objectName()+"engine");
            }
            if (player->getHp() > 0)
                return true;
        }
        return false;
    }
};

class Zishou: public DrawCardsSkill {
public:
    Zishou(): DrawCardsSkill("zishou") {
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        if (player->isWounded() && room->askForSkillInvoke(player, objectName())) {
            int losthp = player->getLostHp();
            room->broadcastSkillInvoke(objectName(), Config.value("music", true).toBool() ? NULL : qMin(3, losthp));
            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                player->clearHistory();
                player->skip(Player::Play);
                room->removePlayerMark(player, objectName()+"engine");
                return n + losthp;
            }
        }
        return n;
    }
};

class Zongshi: public MaxCardsSkill {
public:
    Zongshi(): MaxCardsSkill("zongshi") {
    }

    virtual int getExtra(const Player *target) const{
        int extra = 0;
        QSet<QString> kingdom_set;
        if (target->parent()) {
            foreach (const Player *player, target->parent()->findChildren<const Player *>()) {
                if (player->isAlive())
                    kingdom_set << player->getKingdom();
            }
        }
        extra = kingdom_set.size();
        if (target->hasSkill(objectName()))
            return extra;
        else
            return 0;
    }
};

class Shiyong: public TriggerSkill {
public:
    Shiyong(): TriggerSkill("shiyong") {
        events << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash")
            && (damage.card->isRed() || damage.card->hasFlag("drank"))) {
            int index = 1;
            if (damage.from->getGeneralName().contains("guanyu") && !Config.value("music", true).toBool())
                index = 3;
            else if (damage.card->hasFlag("drank"))
                index = 2;
            room->broadcastSkillInvoke(objectName(), index);
            room->sendCompulsoryTriggerLog(player, objectName());

            room->addPlayerMark(player, objectName()+"engine");
            if (player->getMark(objectName()+"engine") > 0) {
                room->loseMaxHp(player);
                room->removePlayerMark(player, objectName()+"engine");
            }
        }
        return false;
    }
};

class FuhunViewAsSkill: public ViewAsSkill {
public:
    FuhunViewAsSkill(): ViewAsSkill("fuhun") {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "slash";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return selected.length() < 2 && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() != 2)
            return NULL;

        Slash *slash = new Slash(Card::SuitToBeDecided, 0);
        slash->setSkillName(objectName());
        slash->addSubcards(cards);

        return slash;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return 1;
    }
};

class Fuhun: public TriggerSkill {
public:
    Fuhun(): TriggerSkill("fuhun") {
        events << Damage << EventPhaseChanging;
        view_as_skill = new FuhunViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == Damage && TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->getSkillName() == objectName()
                && player->getPhase() == Player::Play) {
                room->broadcastSkillInvoke(objectName(), 2);
				room->addPlayerMark(player, objectName()+"engine");
				if (player->getMark(objectName()+"engine") > 0) {
					room->handleAcquireDetachSkills(player, "wusheng|paoxiao");
					player->setFlags(objectName());
					room->removePlayerMark(player, objectName()+"engine");
				}
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && player->hasFlag(objectName()))
                room->handleAcquireDetachSkills(player, "-wusheng|-paoxiao", true);
        }

        return false;
    }
};

GongqiCard::GongqiCard() {
    mute = true;
    target_fixed = true;
}

void GongqiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->addPlayerMark(source, "gongqiiengine");
    if (source->getMark("gongqiiengine") > 0) {
        room->setPlayerFlag(source, "InfinityAttackRange");
        const Card *cd = Sanguosha->getCard(subcards.first());
        if (cd->isKindOf("EquipCard")) {
            room->broadcastSkillInvoke("gongqi", 2);
            QList<ServerPlayer *> targets;
            foreach(ServerPlayer *p, room->getOtherPlayers(source))
                if (source->canDiscard(p, "he")) targets << p;
            if (!targets.isEmpty()) {
                ServerPlayer *to_discard = room->askForPlayerChosen(source, targets, "gongqi", "@gongqi-discard", true);
                if (to_discard)
                    room->throwCard(room->askForCardChosen(source, to_discard, "he", "gongqi", false, Card::MethodDiscard), to_discard, source);
            }
        } else {
            room->broadcastSkillInvoke("gongqi", 1);
        }
        room->removePlayerMark(source, "gongqiiengine");
    }
}

class Gongqi: public OneCardViewAsSkill {
public:
    Gongqi(): OneCardViewAsSkill("gongqi") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("GongqiCard");
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        GongqiCard *card = new GongqiCard;
        card->addSubcard(originalcard->getId());
        card->setSkillName(objectName());
        return card;
    }
};

JiefanCard::JiefanCard() {
    mute = true;
}

bool JiefanCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const{
    return targets.isEmpty();
}

void JiefanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->removePlayerMark(source, "@rescue");
    ServerPlayer *target = targets.first();
    source->tag["JiefanTarget"] = QVariant::fromValue(target);
    room->broadcastSkillInvoke("jiefan");
    //room->doLightbox("$JiefanAnimate", 2500);

    room->addPlayerMark(source, "jiefaniengine");
    if (source->getMark("jiefaniengine") > 0) {
        room->removePlayerMark(source, "@rescue");
        foreach (ServerPlayer *player, room->getAllPlayers()) {
            if (player->isAlive() && player->inMyAttackRange(target))
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), player->objectName());
        }
        foreach (ServerPlayer *player, room->getAllPlayers()) {
            if (player->isAlive() && player->inMyAttackRange(target))
                room->cardEffect(this, source, player);
        }
        source->tag.remove("JiefanTarget");
        room->removePlayerMark(source, "jiefaniengine");
    }
}

void JiefanCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    ServerPlayer *target = effect.from->tag["JiefanTarget"].value<ServerPlayer *>();
    QVariant data = effect.from->tag["JiefanTarget"];
    if (target && !room->askForCard(effect.to, ".Weapon", "@jiefan-discard::" + target->objectName(), data))
        target->drawCards(1, "jiefan");
}

class Jiefan: public ZeroCardViewAsSkill {
public:
    Jiefan(): ZeroCardViewAsSkill("jiefan") {
        frequency = Limited;
        limit_mark = "@rescue";
    }

    virtual const Card *viewAs() const{
        return new JiefanCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@rescue") >= 1;
    }
};

AnxuCard::AnxuCard() {
    mute = true;
}

bool AnxuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (to_select == Self)
        return false;
    if (targets.isEmpty())
        return true;
    else if (targets.length() == 1)
        return to_select->getHandcardNum() != targets.first()->getHandcardNum();
    else
        return false;
}

bool AnxuCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    return targets.length() == 2;
}

void AnxuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->addPlayerMark(source, "anxuiengine");
    if (source->getMark("anxuiengine") > 0) {
        QList<ServerPlayer *> selecteds = targets;
        ServerPlayer *from = selecteds.first()->getHandcardNum() < selecteds.last()->getHandcardNum() ? selecteds.takeFirst() : selecteds.takeLast();
        ServerPlayer *to = selecteds.takeFirst();
        room->broadcastSkillInvoke("anxu", 1);
        int id = room->askForCardChosen(from, to, "h", "anxu");
        const Card *cd = Sanguosha->getCard(id);
        from->obtainCard(cd);
        room->showCard(from, id);
        if (cd->getSuit() != Card::Spade) {
            room->getThread()->delay(1500);
			room->broadcastSkillInvoke("anxu", 2);
            source->drawCards(1, "anxu");
		}
        room->removePlayerMark(source, "anxuiengine");
    }
}

class Anxu: public ZeroCardViewAsSkill {
public:
    Anxu(): ZeroCardViewAsSkill("anxu") {
    }

    virtual const Card *viewAs() const{
        return new AnxuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("AnxuCard");
    }
};

class Zhuiyi: public TriggerSkill {
public:
    Zhuiyi(): TriggerSkill("zhuiyi") {
        events << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player)
            return false;
        QList<ServerPlayer *> targets = (death.damage && death.damage->from) ? room->getOtherPlayers(death.damage->from) :
                                                                               room->getAlivePlayers();

        if (targets.isEmpty())
            return false;

        QString prompt = "zhuiyi-invoke";
        if (death.damage && death.damage->from && death.damage->from != player)
            prompt = QString("%1x:%2").arg(prompt).arg(death.damage->from->objectName());
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), prompt, true, true);
        if (!target) return false;

        room->broadcastSkillInvoke("anxu", Config.value("music", true).toBool() ? NULL : target->getGeneralName().contains("sunquan") ? 2 : 1);
        room->addPlayerMark(player, objectName()+"engine");
        if (player->getMark(objectName()+"engine") > 0) {
            target->drawCards(3, objectName());
            room->recover(target, RecoverStruct(player), true);
            room->removePlayerMark(player, objectName()+"engine");
        }
        return false;
    }
};

class LihuoViewAsSkill: public OneCardViewAsSkill {
public:
    LihuoViewAsSkill(): OneCardViewAsSkill("lihuo") {
        filter_pattern = "%slash";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
               && pattern == "slash";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *acard = new FireSlash(originalCard->getSuit(), originalCard->getNumber());
        acard->addSubcard(originalCard->getId());
        acard->setSkillName(objectName());
        return acard;
    }
};

class Lihuo: public TriggerSkill {
public:
    Lihuo(): TriggerSkill("lihuo") {
        events << PreDamageDone << CardFinished;
        view_as_skill = new LihuoViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->getSkillName() == objectName()) {
                QVariantList slash_list = damage.from->tag["InvokeLihuo"].toList();
                slash_list << QVariant::fromValue(damage.card);
                damage.from->tag["InvokeLihuo"] = QVariant::fromValue(slash_list);
            }
        } else if (TriggerSkill::triggerable(player) && !player->hasFlag("Global_ProcessBroken")) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return false;

            bool can_invoke = false;
            QVariantList slash_list = use.from->tag["InvokeLihuo"].toList();
            foreach (QVariant card, slash_list) {
                if (card.value<const Card *>() == (const Card *)use.card) {
                    can_invoke = true;
                    slash_list.removeOne(card);
                    use.from->tag["InvokeLihuo"] = QVariant::fromValue(slash_list);
                    break;
                }
            }
            if (!can_invoke) return false;

            room->broadcastSkillInvoke("lihuo", 2);
            room->sendCompulsoryTriggerLog(player, objectName());
            room->loseHp(player, 1);
        }
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return 1;
    }
};

class LihuoTargetMod: public TargetModSkill {
public:
    LihuoTargetMod(): TargetModSkill("#lihuo-target") {
        frequency = NotFrequent;
    }

    virtual int getExtraTargetNum(const Player *from, const Card *card) const{
        if (from->hasSkill("lihuo") && card->isKindOf("FireSlash"))
            return 1;
        else
            return 0;
    }
};
ChunlaoCard::ChunlaoCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void ChunlaoCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    Room *room = source->getRoom();
    room->addPlayerMark(source, "chunlaoiengine");
    if (source->getMark("chunlaoiengine") > 0) {
        source->addToPile("wine", this);
        room->removePlayerMark(source, "chunlaoiengine");
    }
}

ChunlaoWineCard::ChunlaoWineCard()
{
    m_skillName = "chunlao";
    mute = true;
    target_fixed = true;
    will_throw = false;
}

void ChunlaoWineCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    ServerPlayer *who = room->getCurrentDyingPlayer();
    if (!who) return;

    if (subcards.length() != 0) {
        CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "chunlao", QString());
        DummyCard *dummy = new DummyCard(subcards);
        room->throwCard(dummy, reason, NULL);
        delete dummy;
        room->addPlayerMark(source, "chunlaoiengine");
        if (source->getMark("chunlaoiengine") > 0) {
            Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
            analeptic->setSkillName("_chunlao");
            room->useCard(CardUseStruct(analeptic, who, who, false));
            room->removePlayerMark(source, "chunlaoiengine");
        }
    }
}

class ChunlaoViewAsSkill : public ViewAsSkill
{
public:
    ChunlaoViewAsSkill() : ViewAsSkill("chunlao")
    {
        expand_pile = "wine";
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "@@chunlao"
            || (pattern.contains("peach") && !player->getPile("wine").isEmpty());
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "@@chunlao")
            return to_select->isKindOf("Slash");
        else {
            ExpPattern pattern(".|.|.|wine");
            if (!pattern.match(Self, to_select)) return false;
            return selected.length() == 0;
        }
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "@@chunlao") {
            if (cards.length() == 0) return NULL;

            Card *acard = new ChunlaoCard;
            acard->addSubcards(cards);
            acard->setSkillName(objectName());
            return acard;
        } else {
            if (cards.length() != 1) return NULL;
            Card *wine = new ChunlaoWineCard;
            wine->addSubcards(cards);
            wine->setSkillName(objectName());
            return wine;
        }
    }
};

class Chunlao : public TriggerSkill
{
public:
    Chunlao() : TriggerSkill("chunlao")
    {
        events << EventPhaseStart;
        view_as_skill = new ChunlaoViewAsSkill;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *chengpu, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart && chengpu->getPhase() == Player::Finish
            && !chengpu->isKongcheng() && chengpu->getPile("wine").isEmpty()) {
            room->askForUseCard(chengpu, "@@chunlao", "@chunlao", -1, Card::MethodNone);
        }
        return false;
    }

    int getEffectIndex(const ServerPlayer *player, const Card *card) const
    {
        if (card->isKindOf("Analeptic")) {
            if (player->getGeneralName().contains("zhouyu") && !Config.value("music", true).toBool())
                return 3;
            else
                return 2;
        } else
            return 1;
    }
};

YJCM2012Package::YJCM2012Package()
    : Package("YJCM2012")
{
    General *bulianshi = new General(this, "bulianshi", "wu", 3, false); // YJ 101
    bulianshi->addSkill(new Anxu);
    bulianshi->addSkill(new Zhuiyi);

    General *caozhang = new General(this, "caozhang", "wei"); // YJ 102
    caozhang->addSkill(new Jiangchi);
    caozhang->addSkill(new JiangchiTargetMod);
    related_skills.insertMulti("jiangchi", "#jiangchi-target");

    General *chengpu = new General(this, "chengpu", "wu"); // YJ 103
    chengpu->addSkill(new Lihuo);
    chengpu->addSkill(new LihuoTargetMod);
    chengpu->addSkill(new Chunlao);
    related_skills.insertMulti("lihuo", "#lihuo-target");

    General *guanxingzhangbao = new General(this, "guanxingzhangbao", "shu"); // YJ 104
    guanxingzhangbao->addSkill(new Fuhun);

    General *handang = new General(this, "handang", "wu"); // YJ 105
    handang->addSkill(new Gongqi);
    handang->addSkill(new Jiefan);

    General *huaxiong = new General(this, "huaxiong", "qun", 6); // YJ 106
    huaxiong->addSkill(new Shiyong);

    General *liaohua = new General(this, "liaohua", "shu"); // YJ 107
    liaohua->addSkill(new Dangxian);
    liaohua->addSkill(new Fuli);

    General *liubiao = new General(this, "liubiao", "qun", 4); // YJ 108
    liubiao->addSkill(new Zishou);
    liubiao->addSkill(new Zongshi);

    General *madai = new General(this, "madai", "shu"); // YJ 109
    madai->addSkill("mashu");
    madai->addSkill(new Qianxi);
    madai->addSkill(new QianxiClear);
    related_skills.insertMulti("qianxi", "#qianxi-clear");

    General *wangyi = new General(this, "wangyi", "wei", 3, false); // YJ 110
    wangyi->addSkill(new Zhenlie);
    wangyi->addSkill(new Miji);

    General *xunyou = new General(this, "xunyou", "wei", 3); // YJ 111
    xunyou->addSkill(new Qice);
    xunyou->addSkill(new Zhiyu);

    addMetaObject<QiceCard>();
    addMetaObject<ChunlaoCard>();
    addMetaObject<ChunlaoWineCard>();
    addMetaObject<GongqiCard>();
    addMetaObject<JiefanCard>();
    addMetaObject<AnxuCard>();
}

ADD_PACKAGE(YJCM2012)
