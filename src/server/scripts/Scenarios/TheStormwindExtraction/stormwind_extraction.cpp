/*
 * Copyright (C) 2017-2018 AshamaneProject <https://github.com/AshamaneProject>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "CombatAI.h"
#include "GameObjectAI.h"
#include "Scenario.h"
#include "ScriptMgr.h"
#include "stormwind_extraction.h"

// 281484 - Sewer access portal
struct go_se_sewer_access_portal : public GameObjectAI
{
    go_se_sewer_access_portal(GameObject* go) : GameObjectAI(go) { }

    void Reset() override
    {
        go->GetScheduler().CancelAll();
        go->GetScheduler().Schedule(1s, [this](TaskContext context)
        {
            if (Player* player = go->SelectNearestPlayer(1.f))
                if (Scenario* scenario = player->GetScenario())
                    if (scenario->CheckCompletedCriteriaTree(CRITERIA_TREE_OPEN_SEWERS, player))
                    {
                        player->CastSpell(player, SPELL_TELEPORT_STOCKADE, true);
                        scenario->SendScenarioEvent(player, SCENARIO_EVENT_ENTER_STOCKADE);

                        if (InstanceScript* instanceScript = go->GetInstanceScript())
                            instanceScript->SetData(SCENARIO_EVENT_ENTER_STOCKADE, 1);
                        return;
                    }

            context.Repeat();
        });
    }
};

// 134037
struct npc_se_thalyssra : public CombatAI
{
    npc_se_thalyssra(Creature * creature) : CombatAI(creature) { }

    void DoAction(int32 actionId) override
    {
        m_currentCombatMovement = actionId;

        if (actionId == 1)
            MoveCombat(Position(-8690.216797f, 902.533203f, 53.732910f));
        else if (actionId == 2)
            MoveCombat(Position(-8741.128906f, 885.286560f, 52.815895f));
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        CombatAI::MovementInform(type, id);

        if (type == POINT_MOTION_TYPE && id == POINT_ID_COMBAT_MOVEMENT)
        {
            if (m_currentCombatMovement == 1)
                instance->SetData(SCENARIO_EVENT_FIND_ROKHAN, 1);
            else if (m_currentCombatMovement == 2)
                instance->SetData(EVENT_FIND_PRISONNERS, 1);
            else if (m_currentCombatMovement == 3)
                instance->SetData(EVENT_END_OF_PRISON_REACHED, 1);
        }
    }

private:
    int32 m_currentCombatMovement = 0;
};

// 134120
struct npc_se_saurfang : public ScriptedAI
{
    npc_se_saurfang(Creature * creature) : ScriptedAI(creature) { }

    void sGossipSelect(Player* player, uint32 /*menuId*/, uint32 /*gossipListId*/) override
    {
        me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        me->RemoveAurasDueToSpell(SPELL_CHAT_BUBBLE);
        player->PlayConversation(CONVERSATION_SAURFANG);

        player->GetScheduler().Schedule(42s, [](TaskContext context)
        {
            if (InstanceScript* instanceScript = GetContextPlayer()->GetInstanceScript())
                instanceScript->DoSendScenarioEvent(SCENARIO_EVENT_FREE_SAURFANG);

        }).Schedule(47s, [](TaskContext context)
        {
            if (InstanceScript* instanceScript = GetContextPlayer()->GetInstanceScript())
                instanceScript->SetData(SCENARIO_EVENT_FREE_SAURFANG, 1);
        });
    }
};

//287550
struct go_se_talanji_zul_cell_door : public GameObjectAI
{
    go_se_talanji_zul_cell_door(GameObject* go) : GameObjectAI(go) { }

    void OnStateChanged(uint32 state, Unit* /*unit*/) override
    {
        if (state == GO_ACTIVATED)
        {
            if (InstanceScript* instanceScript = go->GetInstanceScript())
            {
                instanceScript->DoSendScenarioEvent(SCENARIO_EVENT_FREE_PRISONNERS);
                instanceScript->SetData(SCENARIO_EVENT_FREE_PRISONNERS, 1);
            }
        }
    }
};

// 2119
class scene_se_jaina_and_zul : public SceneScript
{
public:
    scene_se_jaina_and_zul() : SceneScript("scene_se_jaina_and_zul") { }

    void OnSceneEnd(Player* player, uint32 /*sceneInstanceID*/, SceneTemplate const* /*sceneTemplate*/) override
    {
        player->GetScheduler().Schedule(1s, [player](TaskContext /*context*/)
        {
            player->CastSpell(player, SPELL_SCENARIO_COMPLETE, true);

            player->AddMovieDelayedAction(857, [player]
            {
                player->CastSpell(player, SPELL_SCENARIO_COMPLETE_TELEPORT, true);
            });
        });
    }
};

void AddSC_stormwind_extraction()
{
    RegisterGameObjectAI(go_se_sewer_access_portal);
    RegisterCreatureAI(npc_se_thalyssra);
    RegisterCreatureAI(npc_se_saurfang);
    RegisterGameObjectAI(go_se_talanji_zul_cell_door);
    RegisterSceneScript(scene_se_jaina_and_zul);
}