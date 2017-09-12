#ifndef _GOD_H
#define _GOD_H

#include "package.h"
#include "card.h"
#include "skill.h"
#include "standard.h"

class GodPackage: public Package {
    Q_OBJECT

public:
    GodPackage();
};

class GongxinCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE GongxinCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class YeyanCard: public SkillCard {
    Q_OBJECT

public:
    void damage(ServerPlayer *shenzhouyu, ServerPlayer *target, int point) const;
};

class GreatYeyanCard: public YeyanCard {
    Q_OBJECT

public:
    Q_INVOKABLE GreatYeyanCard();

    virtual bool targetFilter(const QList<const Player *> &targets,
                              const Player *to_select, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select,
                              const Player *Self, int &maxVotes) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class SmallYeyanCard: public YeyanCard {
    Q_OBJECT

public:
    Q_INVOKABLE SmallYeyanCard();
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ShenfenCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ShenfenCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class WuqianCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE WuqianCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class KuangfengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE KuangfengCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class DawuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE DawuCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class JilveCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE JilveCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class Longhun: public ViewAsSkill {
    Q_OBJECT

public:
    Longhun();
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const;
    virtual bool isEnabledAtPlay(const Player *player) const;
    virtual bool viewFilter(const QList<const Card *> &selected, const Card *card) const;
    virtual const Card *viewAs(const QList<const Card *> &cards) const;
    virtual int getEffectIndex(const ServerPlayer *player, const Card *card) const;
    virtual bool isEnabledAtNullification(const ServerPlayer *player) const;

protected:
    virtual int getEffHp(const Player *zhaoyun) const;
};
class QixingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QixingCard();
    void onUse(Room *room, const CardUseStruct &card_use) const;
};


#endif
