/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "CommandScript.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "Pet.h"
#include "Player.h"
#include "PlayerCommand.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

using namespace Acore::ChatCommands;

class player_commandscript : public CommandScript
{
public:
    player_commandscript() : CommandScript("player_commandscript") {}

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable playerCommandTable =
            {
                {"learn", HandlePlayerLearnCommand, SEC_GAMEMASTER, Console::Yes},
                {"maxprof", HandleLearnAllRecipesCommand2, SEC_GAMEMASTER, Console::Yes},
                {"unlearn", HandlePlayerUnLearnCommand, SEC_GAMEMASTER, Console::Yes}};

        static ChatCommandTable commandTable =
            {
                {"player", playerCommandTable}};
        return commandTable;
    }

    static bool HandlePlayerLearnCommand(ChatHandler *handler, Optional<PlayerIdentifier> player, SpellInfo const *spell, Optional<EXACT_SEQUENCE("all")> allRanks)
    {
        if (!player)
            player = PlayerIdentifier::FromTargetOrSelf(handler);
        if (!player || !player->IsConnected())
            return false;

        Player *targetPlayer = player->GetConnectedPlayer();
        return Acore::PlayerCommand::HandleLearnSpellCommand(handler, targetPlayer, spell, allRanks);
    }

    static bool HandlePlayerUnLearnCommand(ChatHandler *handler, Optional<PlayerIdentifier> player, SpellInfo const *spell, Optional<EXACT_SEQUENCE("all")> allRanks)
    {
        if (!player)
            player = PlayerIdentifier::FromTargetOrSelf(handler);
        if (!player || !player->IsConnected())
            return false;

        Player *targetPlayer = player->GetConnectedPlayer();
        return Acore::PlayerCommand::HandleUnlearnSpellCommand(handler, targetPlayer, spell, allRanks);
    }

    static bool HandleLearnAllRecipesCommand2(ChatHandler *handler, Optional<PlayerIdentifier> player, WTail namePart)
    {
        //  Learns all recipes of specified profession and sets skill to max
        //  Example: .learn all_recipes enchanting
        if (!player || !player->IsConnected())
            return false;
        Player *target = player->GetConnectedPlayer();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            return false;
        }

        if (namePart.empty())
            return false;

        // converting string that we try to find to lower case
        wstrToLower(namePart);

        SkillLineEntry const *targetSkillInfo = nullptr;
        char const *name = nullptr;
        for (uint32 i = 1; i < sSkillLineStore.GetNumRows(); ++i)
        {
            SkillLineEntry const *skillInfo = sSkillLineStore.LookupEntry(i);
            if (!skillInfo)
                continue;

            if ((skillInfo->categoryId != SKILL_CATEGORY_PROFESSION &&
                 skillInfo->categoryId != SKILL_CATEGORY_SECONDARY) ||
                !skillInfo->canLink) // only prof with recipes have set
                continue;

            uint8 locale = 0;
            for (; locale < TOTAL_LOCALES; ++locale)
            {
                name = skillInfo->name[locale];
                if (!name || !*name)
                    continue;

                if (Utf8FitTo(name, namePart))
                    break;
            }

            if (locale < TOTAL_LOCALES)
            {
                targetSkillInfo = skillInfo;
                break;
            }
        }

        if (!(name && targetSkillInfo))
            return false;

        HandleLearnSkillRecipesHelper2(target, targetSkillInfo->id);

        uint16 maxLevel = target->GetPureMaxSkillValue(targetSkillInfo->id);
        target->SetSkill(targetSkillInfo->id, target->GetSkillStep(targetSkillInfo->id), maxLevel, maxLevel);
        handler->PSendSysMessage(LANG_COMMAND_LEARN_ALL_RECIPES, name);
        return true;
    }

    static void HandleLearnSkillRecipesHelper2(Player *player, uint32 skillId)
    {
        uint32 classmask = player->getClassMask();

        for (uint32 j = 0; j < sSkillLineAbilityStore.GetNumRows(); ++j)
        {
            SkillLineAbilityEntry const *skillLine = sSkillLineAbilityStore.LookupEntry(j);
            if (!skillLine)
                continue;

            // wrong skill
            if (skillLine->SkillLine != skillId)
                continue;

            // not high rank
            if (skillLine->SupercededBySpell)
                continue;

            // skip racial skills
            if (skillLine->RaceMask != 0)
                continue;

            // skip wrong class skills
            if (skillLine->ClassMask && (skillLine->ClassMask & classmask) == 0)
                continue;

            SpellInfo const *spellInfo = sSpellMgr->GetSpellInfo(skillLine->Spell);
            if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo))
                continue;

            player->learnSpell(skillLine->Spell);
        }
    }
};

void AddSC_player_commandscript()
{
    new player_commandscript();
}
