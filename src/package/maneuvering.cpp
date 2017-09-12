#include "maneuvering.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "room.h"
#include "standard.h"
#include "standard-equips.h"
#include "ai.h"
#include "settings.h"

NatureSlash::NatureSlash(Suit suit, int number, DamageStruct::Nature nature)
    : Slash(suit, number)
{
    this->nature = nature;
}

bool NatureSlash::match(const QString &pattern) const{
    QStringList patterns = pattern.split("+");
    if (patterns.contains("slash"))
        return true;
    else
        return Slash::match(pattern);
}

ThunderSlash::ThunderSlash(Suit suit, int number)
    : NatureSlash(suit, number, DamageStruct::Thunder)
{
    setObjectName("thunder_slash");
}

FireSlash::FireSlash(Suit suit, int number)
    : NatureSlash(suit, number, DamageStruct::Fire)
{
    setObjectName("fire_slash");
}

Analeptic::Analeptic(Card::Suit suit, int number)
    : BasicCard(suit, number)
{
    setObjectName("analeptic");
    target_fixed = true;
}

QString Analeptic::getSubtype() const
{
    return "buff_card";
}

bool Analeptic::IsAvailable(const Player *player, const Card *analeptic)
{
    Analeptic *newanaleptic = new Analeptic(Card::NoSuit, 0);
    newanaleptic->deleteLater();
#define THIS_ANALEPTIC (analeptic == NULL ? newanaleptic : analeptic)
    if (player->isCardLimited(THIS_ANALEPTIC, Card::MethodUse) || player->isProhibited(player, THIS_ANALEPTIC))
        return false;

    return player->usedTimes("Analeptic") <= Sanguosha->correctCardTarget(TargetModSkill::Residue, player, THIS_ANALEPTIC);
#undef THIS_ANALEPTIC
}

bool Analeptic::isAvailable(const Player *player) const
{
    return IsAvailable(player, this) && BasicCard::isAvailable(player);
}

void Analeptic::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    if (use.to.isEmpty())
        use.to << use.from;
    BasicCard::onUse(room, use);
}

void Analeptic::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    if (targets.isEmpty())
        targets << source;
    BasicCard::use(room, source, targets);
}

void Analeptic::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    room->setEmotion(effect.to, "analeptic");

    if (effect.to->hasFlag("Global_Dying") && Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY)
        room->recover(effect.to, RecoverStruct(effect.from, this));
    else
        room->addPlayerMark(effect.to, "drank");
}

class FanSkill: public OneCardViewAsSkill {
public:
    FanSkill(): OneCardViewAsSkill("fan") {
        filter_pattern = "%slash";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player) && player->getMark("Equips_Nullified_to_Yourself") == 0;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
               && pattern == "slash" && player->getMark("Equips_Nullified_to_Yourself") == 0;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *acard = new FireSlash(originalCard->getSuit(), originalCard->getNumber());
        acard->addSubcard(originalCard->getId());
        acard->setSkillName(objectName());
        return acard;
    }
};

Fan::Fan(Suit suit, int number)
    : Weapon(suit, number, 4)
{
    setObjectName("fan");
}

class GudingBladeSkill: public WeaponSkill {
public:
    GudingBladeSkill(): WeaponSkill("guding_blade") {
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash")
            && damage.to->getMark("Equips_of_Others_Nullified_to_You") == 0
            && damage.to->isKongcheng() && damage.by_user && !damage.chain && !damage.transfer) {
            room->setEmotion(player, "weapon/guding_blade");

            LogMessage log;
            log.type = "#GudingBladeEffect";
            log.from = player;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);

            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

GudingBlade::GudingBlade(Suit suit, int number)
    : Weapon(suit, number, 2)
{
    setObjectName("guding_blade");
}

class VineSkill: public ArmorSkill {
public:
    VineSkill(): ArmorSkill("vine") {
        events << DamageInflicted << SlashEffected << CardEffected;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == SlashEffected) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.nature == DamageStruct::Normal) {
                room->setEmotion(player, "armor/vine");
                LogMessage log;
                log.from = player;
                log.type = "#ArmorNullify";
                log.arg = objectName();
                log.arg2 = effect.slash->objectName();
                room->sendLog(log);

                effect.to->setFlags("Global_NonSkillNullify");
                return true;
            }
        } else if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card->isKindOf("AOE")) {
                room->setEmotion(player, "armor/vine");
                LogMessage log;
                log.from = player;
                log.type = "#ArmorNullify";
                log.arg = objectName();
                log.arg2 = effect.card->objectName();
                room->sendLog(log);

                effect.to->setFlags("Global_NonSkillNullify");
                return true;
            }
        } else if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.nature == DamageStruct::Fire) {
                room->setEmotion(player, "armor/vineburn");
                LogMessage log;
                log.type = "#VineDamage";
                log.from = player;
                log.arg = QString::number(damage.damage);
                log.arg2 = QString::number(++damage.damage);
                room->sendLog(log);

                data = QVariant::fromValue(damage);
            }
        }

        return false;
    }
};

Vine::Vine(Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("vine");
}

class SilverLionSkill: public ArmorSkill {
public:
    SilverLionSkill(): ArmorSkill("silver_lion") {
        events << DamageInflicted << CardsMoveOneTime;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == DamageInflicted && ArmorSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.damage > 1) {
                room->setEmotion(player, "armor/silver_lion");
                LogMessage log;
                log.type = "#SilverLion";
                log.from = player;
                log.arg = QString::number(damage.damage);
                log.arg2 = objectName();
                room->sendLog(log);

                damage.damage = 1;
                data = QVariant::fromValue(damage);
            }
        } else if (player->hasFlag("SilverLionRecover")) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != player || !move.from_places.contains(Player::PlaceEquip))
                return false;
            for (int i = 0; i < move.card_ids.size(); ++i) {
                if (move.from_places[i] != Player::PlaceEquip) continue;
                const Card *card = Sanguosha->getEngineCard(move.card_ids[i]);
                if (card->objectName() == objectName()) {
                    player->setFlags("-SilverLionRecover");
                    if (player->isWounded()) {
                        room->setEmotion(player, "armor/silver_lion");
                        room->recover(player, RecoverStruct(NULL, card));
                    }
                    return false;
                }
            }
        }
        return false;
    }
};

SilverLion::SilverLion(Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("silver_lion");
}

void SilverLion::onUninstall(ServerPlayer *player) const{
    if (player->isAlive() && player->hasArmorEffect(objectName()))
        player->setFlags("SilverLionRecover");
}

FireAttack::FireAttack(Card::Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("fire_attack");
}

bool FireAttack::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    return targets.length() < total_num && !to_select->isKongcheng() && (to_select != Self || !Self->isLastHandCard(this, true));
}

void FireAttack::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    if (effect.to->isKongcheng())
        return;

    const Card *card = room->askForCardShow(effect.to, effect.from, objectName());
    room->showCard(effect.to, card->getEffectiveId());

    QString suit_str = card->getSuitString();
    QString pattern = QString(".%1").arg(suit_str.at(0).toUpper());
    QString prompt = QString("@fire-attack:%1::%2").arg(effect.to->objectName()).arg(suit_str);
    if (effect.from->isAlive()) {
        const Card *card_to_throw = room->askForCard(effect.from, pattern, prompt);
        if (card_to_throw)
            room->damage(DamageStruct(this, effect.from, effect.to, 1, DamageStruct::Fire));
        else
            effect.from->setFlags("FireAttackFailed_" + effect.to->objectName()); // For AI
    }

    if (card->isVirtualCard())
        delete card;
}

IronChain::IronChain(Card::Suit suit, int number)
    : TrickCard(suit, number)
{
    setObjectName("iron_chain");
    can_recast = true;
}

QString IronChain::getSubtype() const{
    return "damage_spread";
}

bool IronChain::targetFilter(const QList<const Player *> &targets, const Player *, const Player *Self) const{
    int total_num = 2 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    return targets.length() < total_num && !Self->isCardLimited(this, Card::MethodUse);
}

bool IronChain::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    bool rec = (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) && can_recast;
    QList<int> sub;
    if (isVirtualCard())
        sub = subcards;
    else
        sub << getEffectiveId();
    foreach (int id, sub) {
        if (Self->getHandPile().contains(id)) {
            rec = false;
            break;
        }
    }

    if (rec && Self->isCardLimited(this, Card::MethodUse))
        return targets.length() == 0;
    int total_num = 2 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (!rec)
        return targets.length() > 0 && targets.length() <= total_num;
    else
        return targets.length() <= total_num;
}

void IronChain::onUse(Room *room, const CardUseStruct &card_use) const{
    if (card_use.to.isEmpty()) {
        CardMoveReason reason(CardMoveReason::S_REASON_RECAST, card_use.from->objectName());
        reason.m_skillName = this->getSkillName();
        QList<int> ids;
        if (isVirtualCard())
            ids = subcards;
        else
            ids << getId();
        QList<CardsMoveStruct> moves;
        foreach (int id, ids) {
            CardsMoveStruct move(id, NULL, Player::DiscardPile, reason);
            moves << move;
        }
        room->moveCardsAtomic(moves, true);
        card_use.from->broadcastSkillInvoke("@recast");

        LogMessage log;
        log.type = "#UseCard_Recast";
        log.from = card_use.from;
        log.card_str = card_use.card->toString();
        room->sendLog(log);

        card_use.from->drawCards(1, "recast");
    } else
        TrickCard::onUse(room, card_use);
}

void IronChain::onEffect(const CardEffectStruct &effect) const{
    effect.to->setChained(!effect.to->isChained());

    Room *room = effect.to->getRoom();

    room->broadcastProperty(effect.to, "chained");
    room->setEmotion(effect.to, "chain");
    room->getThread()->trigger(ChainStateChanged, room, effect.to);
}

SupplyShortage::SupplyShortage(Card::Suit suit, int number)
    : DelayedTrick(suit, number)
{
    setObjectName("supply_shortage");

    judge.pattern = ".|club";
    judge.good = true;
    judge.reason = objectName();
}

bool SupplyShortage::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty() || to_select->containsTrick(objectName()) || to_select == Self)
        return false;

    int distance_limit = 1 + Sanguosha->correctCardTarget(TargetModSkill::DistanceLimit, Self, this, to_select);
    int rangefix = 0;
    if (Self->getOffensiveHorse() && subcards.contains(Self->getOffensiveHorse()->getId()))
        rangefix += 1;

	

    if (Self->distanceTo(to_select, rangefix) > distance_limit)
        return false;

    return true;
}

void SupplyShortage::takeEffect(ServerPlayer *target) const{
    target->skip(Player::Draw);
	target->broadcastSkillInvoke("@supply_shortage");
}

class GodBladeSkill: public WeaponSkill {
public:
    GodBladeSkill(): WeaponSkill("god_blade") {
        events << TargetSpecified;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash") || !use.card->isRed())
            return false;
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *p, use.to) {
            LogMessage log;
            log.type = "#NoJink";
            log.from = p;
            room->sendLog(log);
            jink_list.replace(index, QVariant(0));
            ++index;
        }
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
		return false;
    }
};

GodBlade::GodBlade(Suit suit, int number)
    : Weapon(suit, number, 3)
{
    setObjectName("god_blade");
}

class GodDiagramSkill: public ArmorSkill {
public:
    GodDiagramSkill(): ArmorSkill("god_diagram") {
        events << SlashEffected;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        //防止随机出现的“装备了青釭剑，使用黑杀已造成伤害，
        //但仍触发仁王盾效果，屏幕上打印出了相关日志”的问题
        if (!effect.from->hasWeapon("qinggang_sword")) {
            LogMessage log;
            log.type = "#ArmorNullify";
            log.from = player;
            log.arg = objectName();
            log.arg2 = effect.slash->objectName();
            player->getRoom()->sendLog(log);

            room->setEmotion(player, "armor/god_diagram");
            effect.to->setFlags("Global_NonSkillNullify");
            return true;
        } else
            return false;
    }
};

GodDiagram::GodDiagram(Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("god_diagram");
}

GodPao::GodPao(Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("god_pao");
}

class GodQinSkill: public WeaponSkill {
public:
    GodQinSkill(): WeaponSkill("god_qin") {
        events << ConfirmDamage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature != DamageStruct::Fire) {
            room->setEmotion(player, "armor/god_qin");
			damage.nature = DamageStruct::Fire;
            data = QVariant::fromValue(damage);
		}
		return false;
    }
};

GodQin::GodQin(Suit suit, int number)
    : Weapon(suit, number, 4)
{
    setObjectName("god_qin");
}

class GodHalberdsSkill: public TargetModSkill {
public:
    GodHalberdsSkill(): TargetModSkill("#god_halberd") {
    }

    virtual int getExtraTargetNum(const Player *from, const Card *card) const{
        if (from->hasWeapon("god_halberd"))
            return 1000;
        else
            return 0;
    }
};

GodHalberd::GodHalberd(Suit suit, int number)
    : Weapon(suit, number, 4)
{
    setObjectName("god_halberd");
}

class GodHalberdSkill: public WeaponSkill {
public:
    GodHalberdSkill(): WeaponSkill("god_halberd") {
        events << ConfirmDamage << Damage;
    }

    virtual bool trigger(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
		if (damage.card && damage.card->isKindOf("Slash")) {
			if (event == ConfirmDamage) {
				room->setEmotion(player, "armor/god_halberd");
				++damage.damage;
				data = QVariant::fromValue(damage);
			} else {
				room->recover(damage.to, RecoverStruct(damage.from));
			}
		}
		return false;
    }
};

class GodHatSkill: public TreasureSkill {
public:
    GodHatSkill(): TreasureSkill("god_hat") {
        events << DrawNCards;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        room->setEmotion(player, "armor/god_hat");
        data = QVariant::fromValue(data.toInt() + 2);
        return false;
    }
};

GodHat::GodHat(Suit suit, int number)
    : Treasure(suit, number)
{
    setObjectName("god_hat");
}

class GodHatBuff: public MaxCardsSkill {
public:
    GodHatBuff(): MaxCardsSkill("#god_hat") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasTreasure("god_hat"))
            return -1;
        else
            return 0;
    }
};

class GodSwordSkill: public WeaponSkill {
public:
    GodSwordSkill(): WeaponSkill("god_sword") {
        events << TargetSpecified << CardFinished;
    }

    virtual bool trigger(TriggerEvent event, Room *room, ServerPlayer *, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
		if (event == TargetSpecified) {
			if (use.card->isKindOf("Slash")) {
				bool do_anim = false;
				foreach (ServerPlayer *p, use.to.toSet()) {
					if (p->getMark("Equips_of_Others_Nullified_to_You") == 0) {
						room->setPlayerCardLimitation(p, "use,response", ".|.|.|hand", true);
						do_anim = (p->getArmor() && p->hasArmorEffect(p->getArmor()->objectName())) || p->hasSkills("bazhen|linglong|bossmanjia");
						p->addQinggangTag(use.card);
					}
				}
				if (do_anim)
					room->setEmotion(use.from, "weapon/god_sword");
			}
		} else {
			foreach (ServerPlayer *p, use.to.toSet()) {
				room->removePlayerCardLimitation(p, "use,response", ".|.|.|hand$1");
			}
		}
        return false;
    }
};

GodSword::GodSword(Suit suit, int number)
    : Weapon(suit, number, 2)
{
    setObjectName("god_sword");
}

GodNihilo::GodNihilo(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("god_nihilo");
}

bool GodNihilo::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
	if ((to_select->getKingdom() == "god") || (to_select->getKingdom() != "god" && to_select->getHandcardNum() < qMin(to_select->getMaxHp(), 5))) {
		if (total_num == 1) return to_select == Self;
		return targets.length() < total_num && to_select != Self;
	}
	return false;
}

bool GodNihilo::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return Self->getKingdom() == "god" || (Self->getKingdom() != "god" && Self->getHandcardNum() < qMin(Self->getMaxHp(), 5)) || targets.length() > 0;
}

void GodNihilo::onUse(Room *room, const CardUseStruct &card_use) const{
    CardUseStruct use = card_use;
    if (use.to.isEmpty())
        use.to << use.from;
    SingleTargetTrick::onUse(room, use);
}

bool GodNihilo::isAvailable(const Player *player) const{
    return !player->isProhibited(player, this) && TrickCard::isAvailable(player);
}

void GodNihilo::onEffect(const CardEffectStruct &effect) const{
    effect.to->drawCards(effect.to->getKingdom() == "god" ? qMin(effect.to->getMaxHp(), 5) : qMin(effect.to->getMaxHp(), 5) - effect.to->getHandcardNum(), "god_nihilo");
}

GodFlower::GodFlower(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("god_flower");
}

bool GodFlower::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    return targets.length() < total_num && to_select != Self && !to_select->isNude();
}

void GodFlower::onEffect(const CardEffectStruct &effect) const{
	QList<ServerPlayer *> targets;
    Room *room = effect.to->getRoom();
    foreach (ServerPlayer *p, room->getAlivePlayers()) {
		if (effect.to->canSlash(p, NULL, false))
			targets << p;
    }
	if (!room->askForUseSlashTo(effect.to, targets, "@god_flower") && !effect.to->getCards("he").isEmpty()) {
		const Card *card = effect.to->getCards("he").length() == 1 ? effect.to->getCards("he").first() : room->askForExchange(effect.to, "god_flower", 2, 2, true, "@god_flower");
		room->obtainCard(effect.from, card, false);
	}
}

ManeuveringPackage::ManeuveringPackage()
    : Package("maneuvering", Package::CardPack)
{
    QList<Card *> cards;

    // spade
    cards << new GudingBlade(Card::Spade, 1)
          << new Vine(Card::Spade, 2)
          << new Analeptic(Card::Spade, 3)
          << new ThunderSlash(Card::Spade, 4)
          << new ThunderSlash(Card::Spade, 5)
          << new ThunderSlash(Card::Spade, 6)
          << new ThunderSlash(Card::Spade, 7)
          << new ThunderSlash(Card::Spade, 8)
          << new Analeptic(Card::Spade, 9)
          << new SupplyShortage(Card::Spade, 10)
          << new IronChain(Card::Spade, 11)
          << new IronChain(Card::Spade, 12)
          << new Nullification(Card::Spade, 13);
   // club
    cards << new SilverLion(Card::Club, 1)
          << new Vine(Card::Club, 2)
          << new Analeptic(Card::Club, 3)
          << new SupplyShortage(Card::Club, 4)
          << new ThunderSlash(Card::Club, 5)
          << new ThunderSlash(Card::Club, 6)
          << new ThunderSlash(Card::Club, 7)
          << new ThunderSlash(Card::Club, 8)
          << new Analeptic(Card::Club, 9)
          << new IronChain(Card::Club, 10)
          << new IronChain(Card::Club, 11)
          << new IronChain(Card::Club, 12)
          << new IronChain(Card::Club, 13);

     // heart
    cards << new Nullification(Card::Heart, 1)
          << new FireAttack(Card::Heart, 2)
          << new FireAttack(Card::Heart, 3)
          << new FireSlash(Card::Heart, 4)
          << new Peach(Card::Heart, 5)
          << new Peach(Card::Heart, 6)
          << new FireSlash(Card::Heart, 7)
          << new Jink(Card::Heart, 8)
          << new Jink(Card::Heart, 9)
          << new FireSlash(Card::Heart, 10)
          << new Jink(Card::Heart, 11)
          << new Jink(Card::Heart, 12)
          << new Nullification(Card::Heart, 13);

    // diamond
    cards << new Fan(Card::Diamond, 1)
          << new Peach(Card::Diamond, 2)
          << new Peach(Card::Diamond, 3)
          << new FireSlash(Card::Diamond, 4)
          << new FireSlash(Card::Diamond, 5)
          << new Jink(Card::Diamond, 6)
          << new Jink(Card::Diamond, 7)
          << new Jink(Card::Diamond, 8)
          << new Analeptic(Card::Diamond, 9)
          << new Jink(Card::Diamond, 10)
          << new Jink(Card::Diamond, 11)
          << new FireAttack(Card::Diamond, 12);

    DefensiveHorse *hualiu = new DefensiveHorse(Card::Diamond, 13);
    hualiu->setObjectName("hualiu");

    cards << hualiu;

    foreach (Card *card, cards)
        card->setParent(this);

    skills << new GudingBladeSkill << new FanSkill
           << new VineSkill << new SilverLionSkill;
}

GodlailailaiPackage::GodlailailaiPackage()
    : Package("godlailailai", Package::CardPack)
{
    QList<Card *> cards;
    QList<Card *> horses;
    horses << new DefensiveHorse(Card::Spade, 5);
    horses.at(0)->setObjectName("god_horse");
    cards << new GodBlade(Card::Spade, 5)
	      << new GodDiagram(Card::Spade, 2)
	      << new GodPao(Card::Spade, 9)
	      << new GodQin(Card::Diamond, 1)
	      << new GodHalberd(Card::Diamond, 12)
	      << new GodHat(Card::Club, 1)
	      << new GodSword(Card::Spade, 6)
          << new GodNihilo(Card::Heart, 7)
          << new GodNihilo(Card::Heart, 8)
          << new GodNihilo(Card::Heart, 9)
          << new GodNihilo(Card::Heart, 11)
          << new GodFlower(Card::Club, 12)
          << new GodFlower(Card::Club, 13)
		  << horses;

    foreach (Card *card, cards)
        card->setParent(this);

    skills << new GodDiagramSkill << new GodBladeSkill << new GodHalberdSkill << new GodHalberdsSkill << new GodQinSkill << new GodHatSkill << new GodHatBuff << new GodSwordSkill;
		  
}
ADD_PACKAGE(Maneuvering)
ADD_PACKAGE(Godlailailai)
