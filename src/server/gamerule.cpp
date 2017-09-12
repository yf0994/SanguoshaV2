#include "gamerule.h"
#include "serverplayer.h"
#include "room.h"
#include "standard.h"
#include "maneuvering.h"
#include "engine.h"
#include "settings.h"
#include "jsonutils.h"

#include "client.h"
#include "skill.h"
#include "general.h"

#include <QTime>

GameRule::GameRule(QObject *)
    : TriggerSkill("game_rule")
    , m_yeKingdomIndex(0)
{
    //@todo: this setParent is illegitimate in QT and is equivalent to calling
    // setParent(NULL). So taking it off at the moment until we figure out
    // a way to do it.
    //setParent(parent);

    events << GameStart << TurnStart
           << EventPhaseProceeding << EventPhaseEnd << EventPhaseChanging
           << PreCardUsed << CardUsed << CardFinished << CardEffected << CardEffected << CardResponded
           << HpChanged
           << EventLoseSkill << EventAcquireSkill
           << AskForPeaches << AskForPeachesDone << BuryVictim << GameOverJudge
           << SlashHit << SlashEffected << SlashProceed
           << ConfirmDamage << DamageDone << DamageComplete
           << StartJudge << FinishRetrial << FinishJudge
           << ChoiceMade;
}

bool GameRule::triggerable(const ServerPlayer *) const{
    return true;
}

int GameRule::getPriority(TriggerEvent) const{
    return 0;
}

void GameRule::onPhaseProceed(ServerPlayer *player) const{
    Room *room = player->getRoom();
    switch(player->getPhase()) {
    case Player::PhaseNone: {
            Q_ASSERT(false);
        }
    case Player::RoundStart:{
            break;
        }
    case Player::Start: {
			if (player->isLord()) {
				foreach (ServerPlayer *p, room->getPlayers()) {
					if (room->getTag(p->objectName()).toString() == "Zhouyu_Buff")
						room->askForGuanxing(player, room->getNCards(3));
				}
			}
            break;
        }
    case Player::Judge: {
            QList<const Card *> tricks = player->getJudgingArea();
            while (!tricks.isEmpty() && player->isAlive()) {
                const Card *trick = tricks.takeLast();
                bool on_effect = room->cardEffect(trick, NULL, player);
                if (!on_effect)
                    trick->onNullified(player);
            }
            break;
        }
    case Player::Draw: {
            int num = Config.value("yummy_food", true).toBool() ? 2 + Config.value("yummy_food_draw_spinbox", 1).toInt() : 2;
            if (player->hasFlag("Global_FirstRound")) {
                room->setPlayerFlag(player, "-Global_FirstRound");
                if (room->getMode() == "02_1v1") --num;
            }

            QVariant data = num;
            room->getThread()->trigger(DrawNCards, room, player, data);
            int n = data.toInt();
            if (n > 0)
                player->drawCards(n, "draw_phase");
            data = QVariant::fromValue(n);
            room->getThread()->trigger(AfterDrawNCards, room, player, data);
            break;
        }
    case Player::Play: {
            while (player->isAlive()) {
                CardUseStruct card_use;
                room->activate(player, card_use);
                if (card_use.card != NULL)
                    room->useCard(card_use, true);
                else
                    break;
            }
            break;
        }
    case Player::Discard: {
            int discard_num = player->getHandcardNum() - player->getMaxCards();
            if (discard_num > 0)
                room->askForDiscard(player, "gamerule", discard_num, discard_num);
            break;
        }
    case Player::Finish: {
            break;
        }
    case Player::NotActive:{
            break;
        }
    }
}

bool GameRule::trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
    if (room->getTag("SkipGameRule").toBool()) {
        room->removeTag("SkipGameRule");
        return false;
    }

    // Handle global events
    if (player == NULL) {
        if (triggerEvent == GameStart) {
			if (Config.value("sp_face", true).toBool()) {
				QList<ServerPlayer *> loyalists;
				QList<ServerPlayer *> rebels;
				int zhouyu = Config.value("sp_face_zhouyu_spinbox", 1).toInt();
				int zhaoyun = Config.value("sp_face_zhaoyun_spinbox", 1).toInt();
				int zhangyi = Config.value("sp_face_zhangyi_spinbox", 1).toInt();
				foreach (ServerPlayer *p, room->getPlayers()) {
					if (p->getRole() == "loyalist")
						loyalists << p;
					else if (p->getRole() == "rebel")
						rebels << p;
				}
				while (!loyalists.isEmpty() && zhouyu + zhaoyun > 0 ) {
					QList<ServerPlayer *> logsp;
					ServerPlayer * p = loyalists.at(qrand() % loyalists.length());
					loyalists.removeOne(p);
					logsp << p;
					if (zhouyu > 0 && (zhaoyun == 0 || qrand() % 2 == 1)) {
						zhouyu = zhouyu - 1;
						room->setTag(p->objectName(), "Zhouyu_Buff");

						LogMessage log;
						log.type = "#sp_face_zhouyu";
						room->sendLog(log, logsp);
					} else if (zhaoyun > 0 ) {
						zhaoyun = zhaoyun - 1;
						room->setTag(p->objectName(), "Zhaoyun_Buff");

						LogMessage log;
						log.type = "#sp_face_zhaoyun";
						room->sendLog(log, logsp);
					}
				}
				while (!rebels.isEmpty() && zhangyi > 0) {
					QList<ServerPlayer *> logsp;
					zhangyi = zhangyi - 1;
					ServerPlayer * p = rebels.at(qrand() % rebels.length());
					rebels.removeOne(p);
					logsp << p;
					room->setTag(p->objectName(), "Zhangyi_Buff");

					LogMessage log;
					log.type = "#sp_face_zhangyi";
					room->sendLog(log, logsp);
				}
			}
			if (Config.value("EnableSUPERConvert", true).toBool()) {
				foreach (ServerPlayer *p, room->getPlayers()) {
					QString to_cv;
					QStringList choicelist;
					QString name = p->getGeneralName();
					name.remove(0, name.indexOf("_") + 1);
					foreach (const General *gen, Sanguosha->getAllGenerals()) {
						const QString names = gen->objectName();
						if (gen && !Config.value("Banlist/Roles", "").toStringList().contains(names) && !Sanguosha->getBanPackages().contains(gen->getPackage()) && names.endsWith(name) && (!Config.value("My_god", true).toBool() || !names.startsWith("shen")) && (!Config.EnableHegemony || gen->getKingdom() == Sanguosha->getGeneral(name)->getKingdom()) && (!Config.EnableHegemony || !Config.value("Banlist/Pairs", "").toStringList().contains(names)))
							choicelist << names;
					}
					if (choicelist.length() >= 1) {
						AI *ai = p->getAI();
						if (ai)
							to_cv = room->askForChoice(p, objectName(), choicelist.join("+"));
						else
							to_cv = choicelist.length() == 1 ? choicelist.first() : room->askForGeneral(p, choicelist.join("+"));
						
						if (to_cv != p->getGeneralName())
							room->changeHero(p, to_cv, true, false, false);

						const General *general = Sanguosha->getGeneral(to_cv);
						const QString kingdom = general->getKingdom();
						if (kingdom != "god" && kingdom != p->getKingdom())
							room->setPlayerProperty(p, "kingdom", kingdom);
					}
					if (p->getGeneral2Name() != NULL) {
						QStringList choicelis;
						QString nam = p->getGeneral2Name();
						nam.remove(0, nam.indexOf("_") + 1);
						foreach (const General *gen, Sanguosha->getAllGenerals()) {
							const QString n = gen->objectName();
							if (gen && !Config.value("Banlist/Roles", "").toStringList().contains(n) && !Sanguosha->getBanPackages().contains(gen->getPackage()) && n.endsWith(nam) && (!Config.value("My_god", true).toBool() || !n.startsWith("shen")) && (!Config.EnableHegemony || gen->getKingdom() == Sanguosha->getGeneral(nam)->getKingdom()) && !Config.value("Banlist/Pairs", "").toStringList().contains(to_cv+"+"+n) && !Config.value("Banlist/Pairs", "").toStringList().contains(n+"+"+to_cv) && !Config.value("Banlist/Pairs", "").toStringList().contains(n))
								choicelis << n;
						}
						if (choicelis.length() >= 1) {
							QString to_c;
							AI *ai = p->getAI();
							if (ai)
								to_c = room->askForChoice(p, objectName(), choicelis.join("+"));
							else
								to_c = choicelis.length() == 1 ? choicelis.first() : room->askForGeneral(p, choicelis.join("+"));

							if (to_c != p->getGeneral2Name())
								room->changeHero(p, to_c, true, false, true);
						}
					}
				}
            }
            //room->doLightbox("$gamestart", 3500);
            if (room->getMode() == "04_boss") {
                  int difficulty = Config.value("BossModeDifficulty", 0).toInt();
                  if ((difficulty & (1 << GameRule::BMDIncMaxHp)) > 0) {
                      foreach (ServerPlayer *p, room->getPlayers()) {
                          if (p->isLord()) continue;
                          int m = p->getMaxHp() + 2;
                          p->setProperty("maxhp", m);
                          p->setProperty("hp", m);
                          room->broadcastProperty(p, "maxhp");
                          room->broadcastProperty(p, "hp");
                      }
                   }
            }
            if (Config.value("yummy_food", true).toBool()) {
                foreach (ServerPlayer *p, room->getPlayers()) {
                    room->setPlayerFlag(p, "stop_trigger_maxhp");
                    int now = p->getMaxHp() + Config.value("yummy_food_maxhp_spinbox", 1).toInt();
                    p->setProperty("maxhp", now);
                    p->setProperty("hp", now);
                    room->broadcastProperty(p, "maxhp");
                    room->broadcastProperty(p, "hp");
                    room->setPlayerFlag(p, "-stop_trigger_maxhp");
                }
            }
            foreach (ServerPlayer *player, room->getPlayers()) {
                if (player->getGeneral()->getKingdom() == "god" && player->getGeneralName() != "anjiang"
                    && !player->getGeneralName().startsWith("boss_") && room->getMode() != "06_boss")
                    room->setPlayerProperty(player, "kingdom", room->askForKingdom(player));
                foreach (const Skill *skill, player->getVisibleSkillList()) {
                    if (skill->getFrequency() == Skill::Limited && !skill->getLimitMark().isEmpty()
                        && (!skill->isLordSkill() || player->hasLordSkill(skill->objectName())))
                        room->setPlayerMark(player, skill->getLimitMark(), 1);
                }
            }
			if (room->getMode() == "06_boss") {
                room->setPlayerProperty(room->getLord(), "maxhp", room->getLord()->getMaxHp() - 1);
				foreach (ServerPlayer *p, room->getPlayers()) {
					room->setPlayerProperty(p, "role", p->getRole());
					foreach (int id, room->getDrawPile()) {
						if ((Sanguosha->getCard(id)->objectName() == "god_horse" && p->getGeneralName() == "shencaocao") || (Sanguosha->getCard(id)->objectName() == "god_sword" && p->getGeneralName() == "shenzhaoyun") || (Sanguosha->getCard(id)->objectName() == "god_hat" && p->getGeneralName() == "shensimayi") || (Sanguosha->getCard(id)->objectName() == "god_diagram" && p->getGeneralName() == "shenzhugeliang") || (Sanguosha->getCard(id)->objectName() == "god_qin" && p->getGeneralName() == "shenzhouyu") || (Sanguosha->getCard(id)->objectName() == "god_pao" && p->getGeneralName() == "shenlvmeng") || (Sanguosha->getCard(id)->objectName() == "god_halberd" && p->getGeneralName() == "shenlvbu") || (Sanguosha->getCard(id)->objectName() == "god_blade" && p->getGeneralName() == "shenguanyu")) {
							room->moveCardTo(Sanguosha->getCard(id), NULL, p, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_PUT, p->objectName(), "gamerule", QString()));
						}
					}
				}
				foreach (ServerPlayer *p, room->getPlayers()) {
					if (p->getRole() == "rebel")
						break;
					room->addPlayerMark(p, "stop");
				}
			}
            room->setTag("FirstRound", true);
            bool kof_mode = room->getMode() == "02_1v1" && Config.value("1v1/Rule", "2013").toString() != "Classical";
            QList<int> n_list;
            foreach (ServerPlayer *p, room->getPlayers()) {
                int n = kof_mode ? p->getMaxHp() : 4;
				if (room->getMode() == "06_boss")
					n = p->isLord() ? 8 : 4;
                QVariant data = n;
                room->getThread()->trigger(DrawInitialCards, room, p, data);
                n_list << data.toInt();
            }
            room->drawCards(room->getPlayers(), n_list, QString());
            if (Config.LuckCardLimitation > 0)
                room->askForLuckCard();

            int i = 0;
            foreach (ServerPlayer *p, room->getPlayers()) {
                QVariant data = QVariant::fromValue(n_list.at(i));
                room->getThread()->trigger(AfterDrawInitialCards, room, p, data);
                ++i;
            }
        }
        return false;
    }

    switch (triggerEvent) {
    case TurnStart: {
			if (room->getCurrent()->getMark("stop") > 0) {
				room->removePlayerMark(room->getCurrent(), "stop");
				return true;
			}
            player = room->getCurrent();
            if (room->getTag("FirstRound").toBool()) {
                room->setTag("FirstRound", false);
                room->setPlayerFlag(player, "Global_FirstRound");
            }

            LogMessage log;
            log.type = "$AppendSeparator";
            room->sendLog(log);
            room->addPlayerMark(player, "Global_TurnCount");
			room->setPlayerMark(player, "damage_point_round", 0);
            if (room->getMode() == "04_boss" && player->isLord()) {
                int turn = player->getMark("Global_TurnCount");
                if (turn == 1)
                    room->doLightbox("BossLevelA\\ 1 \\BossLevelB", 2000, 100);

                LogMessage log2;
                log2.type = "#BossTurnCount";
                log2.from = player;
                log2.arg = QString::number(turn);
                room->sendLog(log2);

                int limit = Config.value("BossModeTurnLimit", 70).toInt();
                int level = room->getTag("BossModeLevel").toInt();
                if (limit >= 0 && level < Config.BossLevel && player->getMark("Global_TurnCount") > limit)
                    room->gameOver("lord");
            }
            if (!player->faceUp()) {
                room->setPlayerFlag(player, "-Global_FirstRound");
                player->turnOver("gamerule");
            } else if (player->isAlive())
                player->play();

            break;
        }
    case EventPhaseProceeding: {
            onPhaseProceed(player);
            break;
        }
    case EventPhaseEnd: {
            if (player->getPhase() == Player::Play)
                room->addPlayerHistory(player, ".");
			if (player->getPhase() == Player::NotActive) {
            room->addPlayerHistory(player, "Analeptic", 0);     //clear Analeptic
        }
            break;
        }
    case EventPhaseChanging: {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getMark("drank") > 0) {
                        LogMessage log;
                        log.type = "#UnsetDrankEndOfTurn";
                        log.from = player;
                        log.to << p;
                        room->sendLog(log);

                        room->setPlayerMark(p, "drank", 0);
                    }
                }
                room->setPlayerFlag(player, ".");
                room->clearPlayerCardLimitation(player, true);
            } else if (change.to == Player::Play) {
				room->setPlayerMark(player, "damage_point_play_phase", 0);
                room->addPlayerHistory(player, ".");
            }
            break;
        }
    case PreCardUsed: {
            if (data.canConvert<CardUseStruct>()) {
                CardUseStruct card_use = data.value<CardUseStruct>();
                if (card_use.from->hasFlag("Global_ForbidSurrender")) {
                    card_use.from->setFlags("-Global_ForbidSurrender");
                    room->doNotify(card_use.from, QSanProtocol::S_COMMAND_ENABLE_SURRENDER, Json::Value(true));
                }

                card_use.from->broadcastSkillInvoke(card_use.card);
                if (!card_use.card->getSkillName().isNull() && card_use.card->getSkillName(true) == card_use.card->getSkillName(false)
                    && card_use.m_isOwnerUse && card_use.from->hasSkill(card_use.card->getSkillName()))
                    room->notifySkillInvoked(card_use.from, card_use.card->getSkillName());
            }
            break;
        }
    case CardUsed: {
            if (data.canConvert<CardUseStruct>()) {
                CardUseStruct card_use = data.value<CardUseStruct>();
                RoomThread *thread = room->getThread();

                if (Config.value("lucky_pile", true).toBool() && card_use.card->getNumber() >= Config.value("lucky_pile_min_spinbox", 6).toInt() && card_use.card->getNumber() <= Config.value("lucky_pile_max_spinbox", 6).toInt()) {
					int lucky = qrand() % (Config.value("lucky_pile_recover_spinbox", 2).toInt() + Config.value("lucky_pile_draw_spinbox", 2).toInt() + Config.value("lucky_pile_zhiheng_spinbox", 1).toInt() + Config.value("lucky_pile_jushou_spinbox", 1).toInt());
					room->setEmotion(player, lucky < Config.value("lucky_pile_recover_spinbox", 2).toInt() ? "lucky1" : lucky < Config.value("lucky_pile_recover_spinbox", 2).toInt() + Config.value("lucky_pile_draw_spinbox", 2).toInt() ? "lucky2" : lucky < Config.value("lucky_pile_recover_spinbox", 2).toInt() + Config.value("lucky_pile_draw_spinbox", 2).toInt() + Config.value("lucky_pile_zhiheng_spinbox", 1).toInt() ? "lucky3" : "lucky4");
					if (lucky < Config.value("lucky_pile_recover_spinbox", 2).toInt())
						player->drawCards(2, "lucky");
					else if (lucky < Config.value("lucky_pile_recover_spinbox", 2).toInt() + Config.value("lucky_pile_draw_spinbox", 2).toInt() && player->getLostHp() > 0)
						room->recover(player, RecoverStruct(NULL));
					else if (lucky < Config.value("lucky_pile_recover_spinbox", 2).toInt() + Config.value("lucky_pile_draw_spinbox", 2).toInt() + Config.value("lucky_pile_zhiheng_spinbox", 1).toInt() && !player->isNude()) {
						const Card *card = room->askForExchange(player, "@lucky", player->getCards("he").length(), 0, true, "@lucky", true, card_use.card->isKindOf("EquipCard") ? "^"+card_use.card->toString() : ".");
						if (card) {
							room->throwCard(card, player);
							player->drawCards(card->getSubcards().length(), "lucky");
						}
					} else if (lucky < Config.value("lucky_pile_recover_spinbox", 2).toInt() + Config.value("lucky_pile_draw_spinbox", 2).toInt() + Config.value("lucky_pile_zhiheng_spinbox", 1).toInt() + Config.value("lucky_pile_jushou_spinbox", 1).toInt() && player->faceUp())
						player->turnOver();
				}
                if (card_use.card->hasPreAction())
                    card_use.card->doPreAction(room, card_use);

                if (card_use.from && !card_use.to.isEmpty()) {
                    thread->trigger(TargetSpecifying, room, card_use.from, data);
                    CardUseStruct card_use = data.value<CardUseStruct>();
                    QList<ServerPlayer *> targets = card_use.to;
                    foreach (ServerPlayer *to, card_use.to) {
                        if (targets.contains(to)) {
                            thread->trigger(TargetConfirming, room, to, data);
                            CardUseStruct new_use = data.value<CardUseStruct>();
                            targets = new_use.to;
                        }
                    }
                }
                card_use = data.value<CardUseStruct>();

                try {
                    QVariantList jink_list_backup;
                    if (card_use.card->isKindOf("Slash")) {
                        jink_list_backup = card_use.from->tag["Jink_" + card_use.card->toString()].toList();
                        QVariantList jink_list;
                        for (int i = 0; i < card_use.to.length(); ++i)
                            jink_list.append(QVariant(1));
                        card_use.from->tag["Jink_" + card_use.card->toString()] = QVariant::fromValue(jink_list);
                    }
                    if (card_use.from && !card_use.to.isEmpty()) {
                        thread->trigger(TargetSpecified, room, card_use.from, data);
                        foreach (ServerPlayer *p, room->getAllPlayers())
                            thread->trigger(TargetConfirmed, room, p, data);
                    }
                    card_use = data.value<CardUseStruct>();
                    room->setTag("CardUseNullifiedList", QVariant::fromValue(card_use.nullified_list));
                    card_use.card->use(room, card_use.from, card_use.to);
                    if (!jink_list_backup.isEmpty())
                        card_use.from->tag["Jink_" + card_use.card->toString()] = QVariant::fromValue(jink_list_backup);
                }
                catch (TriggerEvent triggerEvent) {
                    if (triggerEvent == TurnBroken || triggerEvent == StageChange)
                        card_use.from->tag.remove("Jink_" + card_use.card->toString());
                    throw triggerEvent;
                }
            }

            break;
        }
    case CardFinished: {
            CardUseStruct use = data.value<CardUseStruct>();
            room->clearCardFlag(use.card);

            if (use.card->isKindOf("AOE") || use.card->isKindOf("GlobalEffect")) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    room->doNotify(p, QSanProtocol::S_COMMAND_NULLIFICATION_ASKED, QSanProtocol::Utils::toJsonString("."));
            }
            if (use.card->isKindOf("Slash"))
                use.from->tag.remove("Jink_" + use.card->toString());

            break;
        }
    case CardResponded: {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (Config.value("lucky_pile", true).toBool() && response.m_card->getNumber() >= Config.value("lucky_pile_min_spinbox", 6).toInt() && response.m_card->getNumber() <= Config.value("lucky_pile_max_spinbox", 6).toInt()) {
				int lucky = qrand() % (Config.value("lucky_pile_recover_spinbox", 2).toInt() + Config.value("lucky_pile_draw_spinbox", 2).toInt() + Config.value("lucky_pile_zhiheng_spinbox", 1).toInt() + Config.value("lucky_pile_jushou_spinbox", 1).toInt());
				room->setEmotion(player, lucky < Config.value("lucky_pile_recover_spinbox", 2).toInt() ? "lucky1" : lucky < Config.value("lucky_pile_recover_spinbox", 2).toInt() + Config.value("lucky_pile_draw_spinbox", 2).toInt() ? "lucky2" : lucky < Config.value("lucky_pile_recover_spinbox", 2).toInt() + Config.value("lucky_pile_draw_spinbox", 2).toInt() + Config.value("lucky_pile_zhiheng_spinbox", 1).toInt() ? "lucky3" : "lucky4");
				if (lucky < Config.value("lucky_pile_recover_spinbox", 2).toInt())
					player->drawCards(2, "lucky");
				else if (lucky < Config.value("lucky_pile_recover_spinbox", 2).toInt() + Config.value("lucky_pile_draw_spinbox", 2).toInt() && player->getLostHp() > 0)
					room->recover(player, RecoverStruct(NULL));
				else if (lucky < Config.value("lucky_pile_recover_spinbox", 2).toInt() + Config.value("lucky_pile_draw_spinbox", 2).toInt() + Config.value("lucky_pile_zhiheng_spinbox", 1).toInt() && !player->isNude()) {
					const Card *card = room->askForExchange(player, "@lucky", player->getCards("he").length(), 0, true, "@lucky", true, response.m_card->isKindOf("EquipCard") ? "^"+response.m_card->toString()+"$0" : ".");
					room->throwCard(card, player);
					player->drawCards(card->getSubcards().length(), "lucky");
				} else if (!player->faceUp())
					player->turnOver();
			}

            break;
		}
    case EventAcquireSkill:
    case EventLoseSkill: {
            QString skill_name = data.toString();
            const Skill *skill = Sanguosha->getSkill(skill_name);
            bool refilter = skill->inherits("FilterSkill");

            if (refilter)
                room->filterCards(player, player->getCards("he"), triggerEvent == EventLoseSkill);

            break;
        }
    case HpChanged: {
            if (room->getMode() == "06_boss" && player->isLord() && player->getHp() <= player->getMaxHp()/2) {
				room->acquireSkill(player, player->getGeneralName() == "hundun" ? "yinzei" : player->getGeneralName() == "qiongqi" ? "yandu" : player->getGeneralName() == "taowu" ? "luanchang" : player->getGeneralName() == "taotie" ? "jicai" : "juejing");
			}
            if (player->getHp() > 0)
                break;
            if (data.isNull() || data.canConvert<RecoverStruct>())
                break;
            if (data.canConvert<DamageStruct>()) {
                DamageStruct damage = data.value<DamageStruct>();
                room->enterDying(player, &damage);
            } else {
                room->enterDying(player, NULL);
            }

            break;
        }
    case AskForPeaches: {
            DyingStruct dying = data.value<DyingStruct>();
            const Card *peach = NULL;

            while (dying.who->getHp() <= 0) {
                peach = NULL;

                // coupling Wansha here to deal with complicated rule problems
                ServerPlayer *current = room->getCurrent();
                if (current && current->isAlive() && current->getPhase() != Player::NotActive && current->hasSkill("wansha")) {
                    if (player != current && player != dying.who) {
                        player->setFlags("wansha");
                        room->addPlayerMark(player, "Global_PreventPeach");
                    }
                }

                if (dying.who->isAlive())
                    peach = room->askForSinglePeach(player, dying.who);

                if (player->hasFlag("wansha") && player->getMark("Global_PreventPeach") > 0) {
                    player->setFlags("-wansha");
                    room->removePlayerMark(player, "Global_PreventPeach");
                }

                if (peach == NULL)
                    break;
                room->useCard(CardUseStruct(peach, player, dying.who));
            }
            break;
        }
    case AskForPeachesDone: {
            if (player->getHp() <= 0 && player->isAlive()) {
                DyingStruct dying = data.value<DyingStruct>();
                room->killPlayer(player, dying.damage);
            }

            break;
        }
    case ConfirmDamage: {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.to->getMark("SlashIsDrank") > 0) {
                LogMessage log;
                log.type = "#AnalepticBuff";
                log.from = damage.from;
                log.to << damage.to;
                log.arg = QString::number(damage.damage);

                damage.damage += damage.to->getMark("SlashIsDrank");
                damage.to->setMark("SlashIsDrank", 0);

                log.arg2 = QString::number(damage.damage);

                room->sendLog(log);

                data = QVariant::fromValue(damage);
            }

            break;
        }
    case DamageDone: {
            //������ʱһ�£��Ǳ��⵱�ж���ͬʱ�ܵ��˺�ʱ(�����ͷ���AOE��)��
            //��ѪЧ��̫�쵼�¿������������
            if (player->getAI()) {
                room->getThread()->delay(500);
            }

            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && !damage.from->isAlive())
                damage.from = NULL;
            data = QVariant::fromValue(damage);

            LogMessage log;

            if (damage.from) {
                log.type = "#Damage";
                log.from = damage.from;
            } else {
                log.type = "#DamageNoSource";
            }

            log.to << damage.to;
            log.arg = QString::number(damage.damage);

            switch (damage.nature) {
            case DamageStruct::Normal: log.arg2 = "normal_nature"; break;
            case DamageStruct::Fire: log.arg2 = "fire_nature"; break;
            case DamageStruct::Thunder: log.arg2 = "thunder_nature"; break;
            }

            room->sendLog(log);

            int new_hp = damage.to->getHp() - damage.damage;

            QString change_str = QString("%1:%2").arg(damage.to->objectName()).arg(-damage.damage);
            switch (damage.nature) {
            case DamageStruct::Fire: change_str.append("F"); break;
            case DamageStruct::Thunder: change_str.append("T"); break;
            default: break;
            }

            Json::Value arg(Json::arrayValue);
            arg[0] = QSanProtocol::Utils::toJsonString(damage.to->objectName());
            arg[1] = -damage.damage;
            arg[2] = int(damage.nature);
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_CHANGE_HP, arg);

            room->setTag("HpChangedData", data);
           

            if (damage.nature != DamageStruct::Normal && player->isChained() && !damage.chain) {
                int n = room->getTag("is_chained").toInt();
                ++n;
                room->setTag("is_chained", n);
            }

			room->setPlayerProperty(damage.to, "hp", new_hp);

            break;
        }
    case DamageComplete: {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.prevented)
                break;
            if (damage.nature != DamageStruct::Normal && player->isChained()) {
                room->setPlayerProperty(player, "chained", false);
				 room->setEmotion(player, "chain");
			}
            if (room->getTag("is_chained").toInt() > 0) {
                if (damage.nature != DamageStruct::Normal && !damage.chain) {
                    // iron chain effect
                    int n = room->getTag("is_chained").toInt();
                    --n;
                    room->setTag("is_chained", n);
                    QList<ServerPlayer *> chained_players;
                    if (room->getCurrent()->isDead())
                        chained_players = room->getOtherPlayers(room->getCurrent());
                    else
                        chained_players = room->getAllPlayers();
                    foreach (ServerPlayer *chained_player, chained_players) {
                        if (chained_player->isChained()) {
                            room->getThread()->delay();
                            LogMessage log;
                            log.type = "#IronChainDamage";
                            log.from = chained_player;
                            room->sendLog(log);

                            DamageStruct chain_damage = damage;
                            chain_damage.to = chained_player;
                            chain_damage.chain = true;
							chain_damage.transfer = false;
                            chain_damage.transfer_reason = QString();

                            room->damage(chain_damage);
                        }
                    }
                }
            }
            if (room->getMode() == "02_1v1" || room->getMode() == "06_XMode") {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("Global_DebutFlag")) {
                        p->setFlags("-Global_DebutFlag");
                        if (room->getMode() == "02_1v1")
                            room->getThread()->trigger(Debut, room, p);
                    }
                }
            }
            break;
        }
    case CardEffected: {
            if (data.canConvert<CardEffectStruct>()) {
                CardEffectStruct effect = data.value<CardEffectStruct>();
                 if (!effect.card->isKindOf("Slash") && effect.nullified) {
                    LogMessage log;
                    log.type = "#CardNullified";
                    log.from = effect.to;
                    log.arg = effect.card->objectName();
                    room->sendLog(log);

                        return true;
                } else if (effect.card->getTypeId() == Card::TypeTrick) {
                    if (room->isCanceled(effect)) {
                        effect.to->setFlags("Global_NonSkillNullify");
                        return true;
                    } else {
                        room->getThread()->trigger(TrickEffect, room, effect.to, data);
                    }
                }
                if (effect.to->isAlive() || effect.card->isKindOf("Slash"))
                    effect.card->onEffect(effect);
            }

            break;
        }
    case SlashEffected: {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.nullified) {
                LogMessage log;
                log.type = "#CardNullified";
                log.from = effect.to;
                log.arg = effect.slash->objectName();
                room->sendLog(log);

                return true;
            }
            if (effect.jink_num > 0)
                room->getThread()->trigger(SlashProceed, room, effect.from, data);
            else
                room->slashResult(effect, NULL);
            break;
        }
    case SlashProceed: {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            QString slasher = effect.from->objectName();
            if (!effect.to->isAlive())
                break;
            if (effect.jink_num == 1) {
                const Card *jink = room->askForCard(effect.to, "jink", "slash-jink:" + slasher, data, Card::MethodUse, effect.from);
                room->slashResult(effect, room->isJinkEffected(effect.to, jink) ? jink : NULL);
            } else {
                DummyCard *jink = new DummyCard;
                const Card *asked_jink = NULL;
                for (int i = effect.jink_num; i > 0; --i) {
                    QString prompt = QString("@multi-jink%1:%2::%3").arg(i == effect.jink_num ? "-start" : QString())
                                                                    .arg(slasher).arg(i);
                    asked_jink = room->askForCard(effect.to, "jink", prompt, data, Card::MethodUse, effect.from);
                    if (!room->isJinkEffected(effect.to, asked_jink)) {
                        delete jink;
                        room->slashResult(effect, NULL);
                        return false;
                    } else {
                        jink->addSubcard(asked_jink->getEffectiveId());

                        //�޸����������ΰ����ж�����һ���ж��ɹ������ڶ���Ҳ�ж��ɹ���
                        //���ڲ��Ű��Զ������ּ���������������������
                        jink->setSkillName(asked_jink->getSkillName());
                    }
                }
                room->slashResult(effect, jink);
            }

            break;
        }
    case SlashHit: {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();

            if (effect.drank > 0) effect.to->setMark("SlashIsDrank", effect.drank);
            room->damage(DamageStruct(effect.slash, effect.from, effect.to, 1, effect.nature));

            break;
        }
    case GameOverJudge: {
        if (room->getMode() == "04_boss" && player->isLord()
            && (Config.value("BossModeEndless", false).toBool() || room->getTag("BossModeLevel").toInt() < Config.BossLevel - 1))
                break;
            if (room->getMode() == "02_1v1") {
                QStringList list = player->tag["1v1Arrange"].toStringList();
                QString rule = Config.value("1v1/Rule", "2013").toString();
                if (list.length() > ((rule == "2013") ? 3 : 0)) break;
            }

            QString winner = getWinner(player);
            if (!winner.isEmpty()) {
                room->gameOver(winner);
            }

            break;
        }
    case BuryVictim: {
            DeathStruct death = data.value<DeathStruct>();
			if (room->getMode() == "06_boss" && room->getTag("fuhuo"+player->objectName()).toInt() < 3 && player->getRole() == "rebel" && room->askForSkillInvoke(player, "fuhuo")) {
                room->setTag("fuhuo"+player->objectName(), room->getTag("fuhuo"+player->objectName()).toInt());
				room->revivePlayer(player);
				room->setPlayerProperty(player, "maxhp", player->getGeneralMaxHp());
				room->setPlayerProperty(player, "hp", qMin(3, player->getMaxHp()));
                player->drawCards(3, "gamerule");
			} else
				player->bury();

            if (room->getTag("SkipNormalDeathProcess").toBool())
                return false;

            ServerPlayer *killer = death.damage ? death.damage->from : NULL;
            if (killer && room->getMode() != "06_boss")
                rewardAndPunish(killer, player);

			if (room->getMode() == "06_boss" && player->getGeneralName() == "zhuyin" && killer && killer->getRole() == "rebel"){
				killer->drawCards(3, "gamerule");
				room->recover(killer, RecoverStruct(killer));
			}

            foreach (ServerPlayer *p, room->getOtherPlayers(room->getLord())) {
                if (room->getMode() == "06_boss" && player->getRole() == "rebel" && player->isDead() && p->isAlive() && p->getRole() == "rebel") {
                    p->drawCards(player->getKingdom() == "god" ? 3 : 1, "gamerule");
					room->recover(p, RecoverStruct(p));
				}
            }
			
            if (room->getMode() == "02_1v1") {
                QStringList list = player->tag["1v1Arrange"].toStringList();
                QString rule = Config.value("1v1/Rule", "2013").toString();
                if (list.length() <= ((rule == "2013") ? 3 : 0)) break;

                if (rule == "Classical") {
                    player->tag["1v1ChangeGeneral"] = list.takeFirst();
                    player->tag["1v1Arrange"] = list;
                } else {
                    player->tag["1v1ChangeGeneral"] = list.first();
                }

                changeGeneral1v1(player);
                if (death.damage == NULL)
                    room->getThread()->trigger(Debut, room, player);
                else
                    player->setFlags("Global_DebutFlag");
                return false;
            } else if (room->getMode() == "06_XMode") {
                changeGeneralXMode(player);
                if (death.damage != NULL)
                    player->setFlags("Global_DebutFlag");
                return false;
            } else if (room->getMode() == "04_boss" && player->isLord()) {
                int level = room->getTag("BossModeLevel").toInt();
                ++level;
                room->setTag("BossModeLevel", level);
                doBossModeDifficultySettings(player);
                changeGeneralBossMode(player);
                if (death.damage != NULL)
                    player->setFlags("Global_DebutFlag");
                room->doLightbox(QString("BossLevelA\\ %1 \\BossLevelB").arg(level + 1), 2000, 100);
                return false;
            }

            break;
        }
    case StartJudge: {
            int card_id = room->drawCard();

            JudgeStruct *judge = data.value<JudgeStruct *>();
            judge->card = Sanguosha->getCard(card_id);

            LogMessage log;
            log.type = "$InitialJudge";
            log.from = player;
            log.card_str = QString::number(judge->card->getEffectiveId());
            room->sendLog(log);

            room->moveCardTo(judge->card, NULL, judge->who, Player::PlaceJudge,
                             CardMoveReason(CardMoveReason::S_REASON_JUDGE,
                             judge->who->objectName(),
                             QString(), QString(), judge->reason), true);
            judge->updateResult();
            break;
        }
    case FinishRetrial: {
            JudgeStruct *judge = data.value<JudgeStruct *>();

            LogMessage log;
            log.type = "$JudgeResult";
            log.from = player;
            log.card_str = QString::number(judge->card->getEffectiveId());
            room->sendLog(log);

            int delay = Config.AIDelay;
            if (judge->time_consuming) {
                delay /= 1.5;
            }
            else {
                delay /= 2.5;
            }

            room->getThread()->delay(delay);
            if (judge->play_animation) {
                room->sendJudgeResult(judge);
                room->getThread()->delay(Config.S_JUDGE_LONG_DELAY);
            }

            break;
        }
    case FinishJudge: {
            JudgeStruct *judge = data.value<JudgeStruct *>();

            if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge) {
                CardMoveReason reason(CardMoveReason::S_REASON_JUDGEDONE, judge->who->objectName(),
                    judge->reason, QString());
                if (judge->retrial_by_response)
                    reason.m_extraData = QVariant::fromValue(judge->retrial_by_response);
                room->moveCardTo(judge->card, judge->who, NULL, Player::DiscardPile, reason, true);
            }

            break;
        }
    case ChoiceMade: {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                foreach (QString flag, p->getFlagList()) {
                    if (flag.startsWith("Global_") && flag.endsWith("Failed"))
                        room->setPlayerFlag(p, "-" + flag);
                }
            }
            break;
        }
    default:
            break;
    }

    return false;
}

void GameRule::changeGeneral1v1(ServerPlayer *player) const{
    Config.AIDelay = Config.OriginAIDelay;

    Room *room = player->getRoom();
    bool classical = (Config.value("1v1/Rule", "2013").toString() == "Classical");
    QString new_general;
	ServerPlayer *target = Config.value("BehindEnemyLines", true).toBool() ? room->getOtherPlayers(player).first() : player;
    if (classical) {
        new_general = target->tag["1v1ChangeGeneral"].toString();
        target->tag.remove("1v1ChangeGeneral");
    } else {
        QStringList list = target->tag["1v1Arrange"].toStringList();
        if (target->getAI())
            new_general = list.first();
        else
            new_general = room->askForGeneral(target, list);
        list.removeOne(new_general);
        target->tag["1v1Arrange"] = QVariant::fromValue(list);
    }

    if (player->getPhase() != Player::NotActive)
        player->changePhase(player->getPhase(), Player::NotActive);

    room->revivePlayer(player);
    room->changeHero(player, new_general, true, true);
    if (player->getGeneral()->getKingdom() == "god")
        room->setPlayerProperty(player, "kingdom", room->askForKingdom(player));
    room->addPlayerHistory(player, ".");

    if (player->getKingdom() != player->getGeneral()->getKingdom())
        room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());

    QList<ServerPlayer *> notified = classical ? room->getOtherPlayers(player, true) : room->getPlayers();
    room->doBroadcastNotify(notified, QSanProtocol::S_COMMAND_REVEAL_GENERAL,
                            QSanProtocol::Utils::toJsonArray(player->objectName(), new_general));

    if (!player->faceUp())
        player->turnOver();

    if (player->isChained())
        room->setPlayerProperty(player, "chained", false);

    room->setTag("FirstRound", true); //For Manjuan
    int draw_num = classical ? 4 : player->getMaxHp();
    QVariant data = draw_num;
    room->getThread()->trigger(DrawInitialCards, room, player, data);
    draw_num = data.toInt();
    try {
        player->drawCards(draw_num);
        room->setTag("FirstRound", false);
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange)
            room->setTag("FirstRound", false);
        throw triggerEvent;
    }
    data = QVariant::fromValue(draw_num);
    room->getThread()->trigger(AfterDrawInitialCards, room, player, data);
}

void GameRule::changeGeneralXMode(ServerPlayer *player) const{
    Config.AIDelay = Config.OriginAIDelay;

    Room *room = player->getRoom();
    ServerPlayer *leader = player->tag["XModeLeader"].value<ServerPlayer *>();
    Q_ASSERT(leader);
    QStringList backup = leader->tag["XModeBackup"].toStringList();
    QString general = room->askForGeneral(leader, backup);
    if (backup.contains(general))
        backup.removeOne(general);
    else
        backup.takeFirst();
    leader->tag["XModeBackup"] = QVariant::fromValue(backup);

    if (player->getPhase() != Player::NotActive)
        player->changePhase(player->getPhase(), Player::NotActive);

    room->revivePlayer(player);
    room->changeHero(player, general, true, true);
    if (player->getGeneral()->getKingdom() == "god")
        room->setPlayerProperty(player, "kingdom", room->askForKingdom(player));
    room->addPlayerHistory(player, ".");

    if (player->getKingdom() != player->getGeneral()->getKingdom())
        room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());

    if (!player->faceUp())
        player->turnOver();

    if (player->isChained())
        room->setPlayerProperty(player, "chained", false);

    room->setTag("FirstRound", true); //For Manjuan
    QVariant data(4);
    room->getThread()->trigger(DrawInitialCards, room, player, data);
    int num = data.toInt();
    try {
        player->drawCards(num);
        room->setTag("FirstRound", false);
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange)
            room->setTag("FirstRound", false);
        throw triggerEvent;
    }
    data = QVariant::fromValue(num);
    room->getThread()->trigger(AfterDrawInitialCards, room, player, data);
}

void GameRule::changeGeneralBossMode(ServerPlayer *player) const{
    Config.AIDelay = Config.OriginAIDelay;

    Room *room = player->getRoom();
    int level = room->getTag("BossModeLevel").toInt();
    room->doBroadcastNotify(QSanProtocol::S_COMMAND_UPDATE_BOSS_LEVEL, Json::Value(level));
    QString general;
    if (level <= Config.BossLevel - 1) {
        QStringList boss_generals = Config.BossGenerals.at(level).split("+");
        if (boss_generals.length() == 1)
            general = boss_generals.first();
        else {
            if (Config.value("OptionalBoss", false).toBool())
                general = room->askForGeneral(player, boss_generals);
            else
                general = boss_generals.at(qrand() % boss_generals.length());
        }
    } else {
        general = (qrand() % 2 == 0) ? "sujiang" : "sujiangf";
    }

    if (player->getPhase() != Player::NotActive)
        player->changePhase(player->getPhase(), Player::NotActive);

    room->revivePlayer(player);
    room->changeHero(player, general, true, true);
    room->setPlayerMark(player, "BossMode_Boss", 1);
    int actualmaxhp = player->getMaxHp();
    if (level >= Config.BossLevel)
        actualmaxhp = level * 5 + 5;
    int difficulty = Config.value("BossModeDifficulty", 0).toInt();
    if ((difficulty & (1 << BMDDecMaxHp)) > 0) {
        if (level == 0) ;
        else if (level == 1) actualmaxhp -= 2;
        else if (level == 2) actualmaxhp -= 4;
        else actualmaxhp -= 5;
    }
    if (actualmaxhp != player->getMaxHp()) {
        player->setProperty("maxhp", actualmaxhp);
        player->setProperty("hp", actualmaxhp);
        room->broadcastProperty(player, "maxhp");
        room->broadcastProperty(player, "hp");
    }
    if (level >= Config.BossLevel)
        acquireBossSkills(player, level);
    room->addPlayerHistory(player, ".");

    if (player->getKingdom() != player->getGeneral()->getKingdom())
        room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());

    if (!player->faceUp())
        player->turnOver();

    if (player->isChained())
        room->setPlayerProperty(player, "chained", false);

    room->setTag("FirstRound", true); //For Manjuan
    QVariant data(4);
    room->getThread()->trigger(DrawInitialCards, room, player, data);
    int num = data.toInt();
    try {
        player->drawCards(num);
        room->setTag("FirstRound", false);
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange)
            room->setTag("FirstRound", false);
        throw triggerEvent;
    }

    data = QVariant::fromValue(num);
    room->getThread()->trigger(AfterDrawInitialCards, room, player, data);
}

void GameRule::acquireBossSkills(ServerPlayer *player, int level) const{
    QStringList skills = Config.BossEndlessSkills;
    int num = qBound(qMin(5, skills.length()), 5 + level - Config.BossLevel, qMin(10, skills.length()));
    for (int i = 0; i < num; ++i) {
        QString skill = skills.at(qrand() % skills.length());
        skills.removeOne(skill);
        if (skill.contains("+")) {
            QStringList subskills = skill.split("+");
            skill = subskills.at(qrand() % subskills.length());
        }
        player->getRoom()->acquireSkill(player, skill);
    }
}

void GameRule::doBossModeDifficultySettings(ServerPlayer *lord) const{
    Room *room = lord->getRoom();
    QList<ServerPlayer *> unions = room->getOtherPlayers(lord, true);
    int difficulty = Config.value("BossModeDifficulty", 0).toInt();
    if ((difficulty & (1 << BMDRevive)) > 0) {
        foreach (ServerPlayer *p, unions) {
            if (p->isDead() && p->getMaxHp() > 0) {
                room->revivePlayer(p, true);
                room->addPlayerHistory(p, ".");
                if (!p->faceUp())
                    p->turnOver();
                if (p->isChained())
                    room->setPlayerProperty(p, "chained", false);
                p->setProperty("hp", qMin(p->getMaxHp(), 4));
                room->broadcastProperty(p, "hp");
                QStringList acquired = p->tag["BossModeAcquiredSkills"].toStringList();
                foreach (QString skillname, acquired) {
                    if (p->hasSkill(skillname, true))
                        acquired.removeOne(skillname);
                }
                p->tag["BossModeAcquiredSkills"] = QVariant::fromValue(acquired);
                if (!acquired.isEmpty())
                    room->handleAcquireDetachSkills(p, acquired, true);
                foreach (const Skill *skill, p->getSkillList()) {
                    if (skill->getFrequency() == Skill::Limited && !skill->getLimitMark().isEmpty())
                        room->setPlayerMark(p, skill->getLimitMark(), 1);
                }
            }
        }
    }
    if ((difficulty & (1 << BMDRecover)) > 0) {
        foreach (ServerPlayer *p, unions) {
            if (p->isAlive() && p->isWounded()) {
                p->setProperty("hp", p->getMaxHp());
                room->broadcastProperty(p, "hp");
            }
        }
    }
    if ((difficulty & (1 << BMDDraw)) > 0) {
        foreach (ServerPlayer *p, unions) {
            if (p->isAlive() && p->getHandcardNum() < 4) {
                room->setTag("FirstRound", true); //For Manjuan
                try {
                    p->drawCards(4 - p->getHandcardNum());
                    room->setTag("FirstRound", false);
                }
                catch (TriggerEvent triggerEvent) {
                    if (triggerEvent == TurnBroken || triggerEvent == StageChange)
                        room->setTag("FirstRound", false);
                    throw triggerEvent;
                }
            }
        }
    }
    if ((difficulty & (1 << BMDReward)) > 0) {
        foreach (ServerPlayer *p, unions) {
            if (p->isAlive()) {
                room->setTag("FirstRound", true); //For Manjuan
                try {
                    p->drawCards(2);
                    room->setTag("FirstRound", false);
                }
                catch (TriggerEvent triggerEvent) {
                    if (triggerEvent == TurnBroken || triggerEvent == StageChange)
                        room->setTag("FirstRound", false);
                    throw triggerEvent;
                }
            }
        }
    }
    if (Config.value("BossModeExp", false).toBool()) {
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->isLord() || p->isDead()) continue;

            QMap<QString, int> exp_map;
            while (true) {
                QStringList choices;
                QStringList allchoices;
                int exp = p->getMark("@bossExp");
                int level = room->getTag("BossModeLevel").toInt();
                exp_map["drawcard"] = 20 * level;
                exp_map["recover"] = 30 * level;
                exp_map["maxhp"] = p->getMaxHp() * 10 * level;
                exp_map["recovermaxhp"] = (20 + p->getMaxHp() * 10) * level;
                foreach (QString c, exp_map.keys()) {
                    allchoices << QString("[%1]|%2").arg(exp_map[c]).arg(c);
                    if (exp >= exp_map[c] && (c != "recover" || p->isWounded()))
                        choices << QString("[%1]|%2").arg(exp_map[c]).arg(c);
                }

                QStringList acquired = p->tag["BossModeAcquiredSkills"].toStringList();
                foreach (QString a, acquired) {
                    if (!p->getAcquiredSkills().contains(a))
                        acquired.removeOne(a);
                }
                int len = qMin(4, acquired.length() + 1);
                foreach (QString skillname, Config.BossExpSkills.keys()) {
                    int cost = Config.BossExpSkills[skillname] * len;
                    allchoices << QString("[%1]||%2").arg(cost).arg(skillname);
                    if (p->hasSkill(skillname, true)) continue;
                    if (exp >= cost)
                        choices << QString("[%1]||%2").arg(cost).arg(skillname);
                }
                if (choices.isEmpty()) break;
                allchoices << "cancel";
                choices << "cancel";
                ServerPlayer *choiceplayer = p;
                if (!p->isOnline()) {
                    foreach (ServerPlayer *cp, room->getPlayers()) {
                        if (!cp->isLord() && cp->isOnline()) {
                            choiceplayer = cp;
                            break;
                        }
                    }
                }
                room->setPlayerProperty(choiceplayer, "bossmodeexp", p->objectName());
                room->setPlayerProperty(choiceplayer, "bossmodeacquiredskills", acquired.join("+"));
                room->setPlayerProperty(choiceplayer, "bossmodeexpallchoices", allchoices.join("+"));
                QString choice = room->askForChoice(choiceplayer, "BossModeExpStore", choices.join("+"));
                if (choice == "cancel") {
                    break;
                } else if (choice.contains("||")) { // skill
                    QStringList skilllist;
                    QString skillattach = choice.split("|").last();
                    if (acquired.length() == 4) {
                        QString skilldetach = room->askForChoice(choiceplayer, "BossModeExpStoreSkillDetach", acquired.join("+"));
                        skilllist << "-" + skilldetach;
                        acquired.removeOne(skilldetach);
                    }
                    skilllist.append(skillattach);
                    acquired.append(skillattach);
                    p->tag["BossModeAcquiredSkills"] = QVariant::fromValue(acquired);
                    int cost = choice.split("]").first().mid(1).toInt();

                    LogMessage log;
                    log.type = "#UseExpPoint";
                    log.from = p;
                    log.arg = QString::number(cost);
                    log.arg2 = "BossModeExpStore:acquireskill";
                    room->sendLog(log);

                    room->removePlayerMark(p, "@bossExp", cost);
                    room->handleAcquireDetachSkills(p, skilllist, true);
                } else {
                    QString type = choice.split("|").last();
                    int cost = choice.split("]").first().mid(1).toInt();
                    room->removePlayerMark(p, "@bossExp", cost);

                    LogMessage log;
                    log.type = "#UseExpPoint";
                    log.from = p;
                    log.arg = QString::number(cost);
                    log.arg2 = "BossModeExpStore:" + type;
                    room->sendLog(log);

                    if (type == "drawcard") {
                        room->setTag("FirstRound", true); //For Manjuan
                        try {
                            p->drawCards(1);
                            room->setTag("FirstRound", false);
                        }
                        catch (TriggerEvent triggerEvent) {
                            if (triggerEvent == TurnBroken || triggerEvent == StageChange)
                                room->setTag("FirstRound", false);
                            throw triggerEvent;
                        }
                    } else {
                        int maxhp = p->getMaxHp();
                        int hp = p->getHp();
                        if (type.contains("maxhp")) {
                            p->setProperty("maxhp", maxhp + 1);
                            room->broadcastProperty(p, "maxhp");
                        }
                        if (type.contains("recover")) {
                            p->setProperty("hp", hp + 1);
                            room->broadcastProperty(p, "hp");
                        }

                        LogMessage log2;
                        log2.type = "#GetHp";
                        log2.from = p;
                        log2.arg = QString::number(p->getHp());
                        log2.arg2 = QString::number(p->getMaxHp());
                        room->sendLog(log2);
                    }
                }
            }
        }
    }
}

void GameRule::rewardAndPunish(ServerPlayer *killer, ServerPlayer *victim) const{
    if (killer->isDead() || killer->getRoom()->getMode() == "06_XMode"
        || killer->getRoom()->getMode() == "04_boss"
        || killer->getRoom()->getMode() == "08_defense")
        return;

    if (killer->getRoom()->getMode() == "06_3v3") {
        if (Config.value("3v3/OfficialRule", "2013").toString().startsWith("201"))
            killer->drawCards(2, "kill");
        else
            killer->drawCards(3, "kill");
    } else {
        if (victim->getRole() == "rebel" && killer != victim)
            killer->drawCards(3, "kill");
        else if (victim->getRole() == "loyalist" && killer->getRole() == "lord")
            killer->throwAllHandCardsAndEquips();
    }
}

QString GameRule::getWinner(ServerPlayer *victim) const{
    Room *room = victim->getRoom();
    QString winner;

    if (room->getMode() == "06_3v3") {
        switch (victim->getRoleEnum()) {
        case Player::Lord: winner = "renegade+rebel"; break;
        case Player::Renegade: winner = "lord+loyalist"; break;
        default:
            break;
        }
    } else if (room->getMode() == "06_XMode") {
        QString role = victim->getRole();
        ServerPlayer *leader = victim->tag["XModeLeader"].value<ServerPlayer *>();
        if (leader->tag["XModeBackup"].toStringList().isEmpty()) {
            if (role.startsWith('r'))
                winner = "lord+loyalist";
            else
                winner = "renegade+rebel";
        }
    } else if (room->getMode() == "08_defense") {
        QStringList alive_roles = room->aliveRoles(victim);
        if (!alive_roles.contains("loyalist"))
            winner = "rebel";
        else if (!alive_roles.contains("rebel"))
            winner = "loyalist";
    } else if (Config.EnableHegemony) {
        bool has_diff_kingdoms = false;
        QString init_kingdom;

        //�޸���������Ҿ�ȫ��������������δ����������ϵͳһֱѯ�ʱ����Ƿ�������
        //ֱ�����������������б���Ӯ����������ѯ����ȥ��������
        int yeKingdomIndex = 0;
        int godKingdomIndex = 0;
        QList<ServerPlayer *> alivePlayers = room->getAlivePlayers();
        foreach (ServerPlayer *p, alivePlayers) {
            QString kingdom = p->getKingdom();
            QStringList names = p->getBasaraGeneralNames();
            if (!names.isEmpty() && (!Config.Enable2ndGeneral || names.count() > 1)) {
                const General *general = Sanguosha->getGeneral(names.first());
                if (NULL != general) {
                    kingdom = general->getKingdom();
                }

                if (kingdom == "god") {
                    kingdom = QString("god%1").arg(godKingdomIndex++);
                }
                else if (Config.Enable2ndGeneral) {
                    QString kingdom2;
                    const General *general2 = Sanguosha->getGeneral(names.last());
                    if (NULL != general2) {
                        kingdom2 = general2->getKingdom();
                    }
                    if (kingdom2 == "god" || kingdom != kingdom2) {
                        kingdom = QString("god%1").arg(godKingdomIndex++);
                    }
                }
            }

            if (!kingdom.contains("god") && !kingdom.contains("ye")) {
                if (judgeCareeristKingdom(p, kingdom)) {
                    kingdom = QString("ye%1").arg(yeKingdomIndex++);
                }
            }

            if (init_kingdom.isEmpty()) {
                init_kingdom = kingdom;
            }
            else if (init_kingdom != kingdom) {
                has_diff_kingdoms = true;
                break;
            }
        }

        if (!has_diff_kingdoms) {
            QStringList winners;
            foreach (ServerPlayer *p, room->getPlayers()) {
                if (p->isAlive()) {
                    winners << p->objectName();
                }
                else if (p->getKingdom() == init_kingdom) {
                    //someone showed his kingdom before death,
                    //he should be considered victorious as well if his kingdom survives
                    winners << p->objectName();
                }
            }
            winner = winners.join("+");
        }

        if (!winner.isEmpty()) {
            foreach (ServerPlayer *player, room->getAllPlayers()) {
                forcePlayerShowGenerals(player);
            }
        }
    } else {
        QStringList alive_roles = room->aliveRoles(victim);
        switch (victim->getRoleEnum()) {
        case Player::Lord: {
                if (alive_roles.length() == 1 && alive_roles.first() == "renegade")
                    winner = room->getAlivePlayers().first()->objectName();
                else
                    winner = "rebel";
                break;
            }
        case Player::Rebel:
        case Player::Renegade: {
                if (!alive_roles.contains("rebel") && !alive_roles.contains("renegade"))
                    winner = "lord+loyalist";
                break;
            }
        default:
                break;
        }
    }

    return winner;
}

//��սģʽʱ���ж�����Ƿ�Ӧ�ó�ΪҰ�ļ�����
//��һ����ɫ�����佫��ȷ������ʱ���������Ľ�ɫ����������Ϸ������һ�룬������ΪҰ�ļ�
//�Ƿ��ΪҰ�ļҰ���Ϸ���������㣬��������ɫ�Ƿ������޹�
bool GameRule::judgeCareeristKingdom(ServerPlayer *player, const QString &newKingdom)
{
    if (!Config.EnableHegemony || NULL == player) {
        return false;
    }

    if (!player->isCareerist()) {
        Room *room = player->getRoom();
        QMap<QString, int> kingdomCounter;
        QList<ServerPlayer *> otherPlayers = room->getPlayers();
        otherPlayers.removeOne(player);

        int godKingdomIndex = 0;
        foreach (ServerPlayer *p, otherPlayers) {
            QString kingdom = p->getKingdom();
            if (kingdom == "god") {
                kingdom = QString("god%1").arg(godKingdomIndex++);
            }
            ++kingdomCounter[kingdom];
        }

        if (kingdomCounter[newKingdom] >= room->getPlayerCount() / 2) {
            room->setPlayerProperty(player, "careerist", true);
        }
    }

    return player->isCareerist();
}

void GameRule::forcePlayerShowGenerals(ServerPlayer *player) const
{
    Room *room = player->getRoom();

    if (player->getGeneralName() == "anjiang") {
        QStringList generals = player->getBasaraGeneralNames();
        if (generals.isEmpty()) {
            return;
        }
        room->changePlayerGeneral(player, generals.at(0));

        QString kingdom = player->getGeneral()->getKingdom();
        if (kingdom == "god") {
            kingdom = "wei";
        }
        //����Ұ�ļ�������Ұ�ļ���Ұ�ļ�֮��Ҳ�ǲ�ͬ�������������ǵĹ����ֱ���ye0,ye1,ye2...�ȣ�
        if (judgeCareeristKingdom(player, kingdom)) {
            kingdom = QString("ye%1").arg(m_yeKingdomIndex++);
        }
        room->setPlayerProperty(player, "kingdom", kingdom);

        if (Config.EnableHegemony) {
            room->setPlayerProperty(player, "role", BasaraMode::getMappedRole(kingdom));
        }

        generals.takeFirst();
        player->setProperty("basara_generals", generals.join("+"));
        room->notifyProperty(player, player, "basara_generals");

        room->setAnjiangNames(player, generals);
    }

    if (Config.Enable2ndGeneral && player->getGeneral2Name() == "anjiang") {
        QStringList generals = player->getBasaraGeneralNames();
        if (generals.isEmpty()) {
            return;
        }
        room->changePlayerGeneral2(player, generals.at(0));
        player->setProperty("basara_generals", QString());
        room->notifyProperty(player, player, "basara_generals");

        room->removeAnjiangNames(player);
    }
}

HulaoPassMode::HulaoPassMode(QObject *parent)
    : GameRule(parent)
{
    setObjectName("hulaopass_mode");
    events << HpChanged << StageChange;
}

bool HulaoPassMode::trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
    switch (triggerEvent) {
    case StageChange: {
            ServerPlayer *lord = room->getLord();
            room->setPlayerMark(lord, "secondMode", 1);
            room->changeHero(lord, "shenlvbu2", true, true, false, false);

            LogMessage log;
            log.type = "$AppendSeparator";
            room->sendLog(log);

            log.type = "#HulaoTransfigure";
            log.arg = "#shenlvbu1";
            log.arg2 = "#shenlvbu2";
            room->sendLog(log);

            Sanguosha->playHulaoPassBGM();

            room->doLightbox("$StageChange", 5000);

            QList<const Card *> tricks = lord->getJudgingArea();
            if (!tricks.isEmpty()) {
                DummyCard *dummy = new DummyCard;
                foreach (const Card *trick, tricks)
                    dummy->addSubcard(trick);
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString());
                room->throwCard(dummy, reason, NULL);
                delete dummy;
            }
            if (!lord->faceUp())
                lord->turnOver();
            if (lord->isChained())
                room->setPlayerProperty(lord, "chained", false);
            break;
        }
    case GameStart: {
            // Handle global events
            if (player == NULL) {
                ServerPlayer *lord = room->getLord();
                lord->drawCards(8);
                foreach (ServerPlayer *player, room->getPlayers()) {
                    if (!player->isLord())
                        player->drawCards(player->getSeat() + 1);
                }

                if (Config.LuckCardLimitation > 0) {
                    room->askForLuckCard();
                }

                return false;
            }
            break;
        }
    case HpChanged: {
            if (player->isLord() && player->getHp() <= 4 && player->getMark("secondMode") == 0)
                throw StageChange;
            break;
        }
    case GameOverJudge: {
            if (player->isLord())
                room->gameOver("rebel");
            else
                if (room->aliveRoles(player).length() == 1)
                    room->gameOver("lord");

            return false;
        }
    case BuryVictim: {
            if (player->hasFlag("actioned")) room->setPlayerFlag(player, "-actioned");

            LogMessage log;
            log.type = "#Reforming";
            log.from = player;
            room->sendLog(log);

            player->bury();
            room->setPlayerProperty(player, "hp", 0);

            foreach (ServerPlayer *p, room->getOtherPlayers(room->getLord())) {
                if (p->isAlive() && p->askForSkillInvoke("draw_1v3"))
                    p->drawCards(1, "draw_1v3");
            }

            return false;
        }
    case TurnStart: {
            if (player->isDead()) {
                Json::Value arg(Json::arrayValue);
                arg[0] = (int)QSanProtocol::S_GAME_EVENT_PLAYER_REFORM;
                arg[1] = QSanProtocol::Utils::toJsonString(player->objectName());
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

                QString choice = player->isWounded() ? "recover" : "draw";
                if (player->isWounded() && player->getHp() > 0)
                    choice = room->askForChoice(player, "Hulaopass", "recover+draw");

                if (choice == "draw") {
                    LogMessage log;
                    log.type = "#ReformingDraw";
                    log.from = player;
                    log.arg = "1";
                    room->sendLog(log);
                    player->drawCards(1, "reform");
                } else {
                    LogMessage log;
                    log.type = "#ReformingRecover";
                    log.from = player;
                    log.arg = "1";
                    room->sendLog(log);
                    room->setPlayerProperty(player, "hp", player->getHp() + 1);
                }

                if (player->getHp() + player->getHandcardNum() == 6) {
                    LogMessage log;
                    log.type = "#ReformingRevive";
                    log.from = player;
                    room->sendLog(log);

                    room->revivePlayer(player);
                }
            } else {
                LogMessage log;
                log.type = "$AppendSeparator";
                room->sendLog(log);
                room->addPlayerMark(player, "Global_TurnCount");

                if (!player->faceUp())
                    player->turnOver("gamerule");
                else
                    player->play();
            }

            return false;
        }
    default:
            break;
    }

    return GameRule::trigger(triggerEvent, room, player, data);
}

BasaraMode::BasaraMode(QObject *parent)
    : GameRule(parent)
{
    setObjectName("basara_mode");
    events << EventPhaseStart << DamageInflicted << BeforeGameOverJudge;
}

const QString &BasaraMode::getMappedRole(const QString &kingdom) {
    static QMap<QString, QString> roles;
    if (roles.isEmpty()) {
        roles["wei"] = "lord";
        roles["shu"] = "loyalist";
        roles["wu"] = "rebel";
        roles["qun"] = "renegade";

        //����Ұ�ļ�����
        roles["ye"] = "careerist";
    }

    if (kingdom.contains("ye")) {
        return roles["ye"];
    }
    else {
        return roles[kingdom];
    }
}

int BasaraMode::getPriority(TriggerEvent) const{
    return 15;
}

void BasaraMode::playerShowed(ServerPlayer *player) const{
    QStringList names = player->getBasaraGeneralNames();
    if (names.isEmpty()) {
        return;
    }

    Room *room = player->getRoom();
    QString answer = room->askForChoice(player, "RevealGeneral", "yes+no");
    if (answer == "yes") {
        QString general_name = room->askForGeneral(player, names);

        generalShowed(player, general_name);
        if (Config.EnableHegemony) room->getThread()->trigger(GameOverJudge, room, player);
        playerShowed(player);
    }
}

void BasaraMode::generalShowed(ServerPlayer *player, const QString &general_name) const{
    QStringList names = player->getBasaraGeneralNames();
    if (names.isEmpty()) {
        return;
    }

    Room *room = player->getRoom();
    if (player->getGeneralName() == "anjiang") {
        room->changeHero(player, general_name, false, true, false, false);
		
        QString new_kingdom = player->getGeneral()->getKingdom();
        if (new_kingdom == "god") {
            new_kingdom = room->askForKingdom(player);
        }

        //����Ұ�ļ�������Ұ�ļ���Ұ�ļ�֮��Ҳ�ǲ�ͬ�������������ǵĹ����ֱ���ye0,ye1,ye2...�ȣ�
        QString kingdom = judgeCareeristKingdom(player, new_kingdom)
            ? QString("ye%1").arg(m_yeKingdomIndex++) : new_kingdom;
        room->setPlayerProperty(player, "kingdom", kingdom);

        if (Config.EnableHegemony)
            room->setPlayerProperty(player, "role", getMappedRole(player->getKingdom()));
    } else {
        room->changeHero(player, general_name, false, false, true, false);
    }

    if (player->tag.contains("god_huangtianv")) {
        player->tag.remove("god_huangtianv");

        if (player->getKingdom() == "qun") {
            room->attachSkillToPlayer(player, "huangtianv");
        }
    }
    if (player->tag.contains("god_zhiba_pindian")) {
        player->tag.remove("god_zhiba_pindian");

        if (player->getKingdom() == "wu") {
            room->attachSkillToPlayer(player, "zhiba_pindian");
        }
    }

    names.removeOne(general_name);
    player->setProperty("basara_generals", names.join("+"));
    room->notifyProperty(player, player, "basara_generals");

    room->setAnjiangNames(player, names);

    LogMessage log;
    log.type = "#BasaraReveal";
    log.from = player;
    log.arg  = player->getGeneralName();
    if (player->getGeneral2()) {
        log.type = "#BasaraRevealDual";
        log.arg2 = player->getGeneral2Name();
    }
    room->sendLog(log);

    //��������赺�ϵͳ
    if (Config.EnableHegemony && Config.Enable2ndGeneral
        && player->getGeneral2Name() != "anjiang" && player->getMark("CompanionEffect") > 0) {
            room->doLightbox("$CompanionEffectAnimate", 4000);
            QStringList choices;
            if (player->isWounded()) {
                choices << "recover";
            }
            choices << "draw" << "cancel";
            QString choice = room->askForChoice(player, "CompanionEffect", choices.join("+"));
            if (choice == "recover") {
                LogMessage log;
                log.type = "#CompanionEffectRecover";
                log.from = player;
                room->sendLog(log);

                RecoverStruct recover;
                recover.who = player;
                recover.recover = 1;
                room->recover(player, recover);
            }
            else if (choice == "draw") {
                LogMessage log;
                log.type = "#CompanionEffectDraw";
                log.from = player;
                room->sendLog(log);

                player->drawCards(2);
            }
            room->removePlayerMark(player, "CompanionEffect");
    }
}

bool BasaraMode::trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
    // Handle global events
    if (player == NULL) {
        if (triggerEvent == GameStart) {
            if (Config.EnableHegemony)
                room->setTag("SkipNormalDeathProcess", true);
            foreach (ServerPlayer *sp, room->getAlivePlayers()) {
                room->setPlayerProperty(sp, "general", "anjiang");
                sp->setGender(General::Sexless);
                room->setPlayerProperty(sp, "kingdom", "god");

                LogMessage log;
                log.type = "#BasaraGeneralChosen";

                QStringList argList = sp->getBasaraGeneralNames();
                if (!argList.isEmpty()) {
                    log.arg = argList.first();
                }

                if (Config.Enable2ndGeneral) {
                    room->setPlayerProperty(sp, "general2", "anjiang");
                    log.type = "#BasaraGeneralChosenDual";

                    if (!argList.isEmpty()) {
                        log.arg2 = argList.last();
                    }
                }

                room->sendLog(log, sp);
            }
        }
        return false;
    }

    player->tag["triggerEvent"] = triggerEvent;
    player->tag["triggerEventData"] = data; // For AI

    switch (triggerEvent) {
    case CardEffected: {
            if (player->getPhase() == Player::NotActive) {
                CardEffectStruct ces = data.value<CardEffectStruct>();
                if (ces.card)
                    if (ces.card->isKindOf("TrickCard") || ces.card->isKindOf("Slash"))
                        playerShowed(player);

                const ProhibitSkill *prohibit = room->isProhibited(ces.from, ces.to, ces.card);
                if (prohibit && ces.to->hasSkill(prohibit->objectName())) {
                    if (prohibit->isVisible()) {
                        LogMessage log;
                        log.type = "#SkillAvoid";
                        log.from = ces.to;
                        log.arg  = prohibit->objectName();
                        log.arg2 = ces.card->objectName();
                        room->sendLog(log);

                        room->broadcastSkillInvoke(prohibit->objectName());
                        room->notifySkillInvoked(ces.to, prohibit->objectName());
                    }

                    return true;
                }
            }
            break;
        }
    case EventPhaseStart: {
            if (player->getPhase() == Player::RoundStart)
                playerShowed(player);

            break;
        }
    case DamageInflicted: {
            playerShowed(player);
            break;
        }
    case BeforeGameOverJudge: {
            forcePlayerShowGenerals(player);
            break;
        }
    case BuryVictim: {
            DeathStruct death = data.value<DeathStruct>();
            player->bury();

            //��ս���ͷ�ʽ��1���Ѿ�ȷ�������Ľ�ɫɱ����ͬ�����Ľ�ɫ�������������ƺ�װ�������ƣ�
            //2���Ѿ�ȷ�������Ľ�ɫɱ����ͬ�����Ľ�ɫ�����ƶ���ȡ��ͬ�ڸ����������������ո�ɱ���Ľ�ɫ�����ơ�
            //ע������ɱ���Ľ�ɫ��û�������佫�ƣ���ûȷ����������������������������
            //û�������Ľ�ɫ�����佫��û�����õĽ�ɫ��ɱ��������ɫû�н��͡�
            if (Config.EnableHegemony) {
                ServerPlayer *killer = death.damage ? death.damage->from : NULL;
                //killer->getKingdom() != "god"����ʾkiller�Ѿ��������佫�ƣ�ȷ��������
                if (killer && killer->getKingdom() != "god") {
                    if (killer->getKingdom() == player->getKingdom()) {
                        killer->throwAllHandCardsAndEquips();
                    }
                    else if (killer->isAlive()) {
                        int sameKingdomPlayerCount = getSameKingdomPlayerCount(player);
                        killer->drawCards(sameKingdomPlayerCount, "kill");
                    }
                }
                return true;
            }
            break;
        }
    default:
        break;
    }
    return false;
}

int BasaraMode::getSameKingdomPlayerCount(ServerPlayer *player)
{
    QString kingdom = player->getKingdom();
    if (kingdom == "god" || kingdom.contains("ye")) {
        return 1;
    }

    Room *room = player->getRoom();
    QList<ServerPlayer *> otherPlayers = room->getPlayers();
    otherPlayers.removeOne(player);

    int count = 1;
    foreach (ServerPlayer *p, otherPlayers) {
        if (p->getKingdom() == kingdom) {
            ++count;
        }
    }

    return count;
}
