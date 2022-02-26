/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "Chat.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"

// Add player scripts
class UnbondAnnounce: public PlayerScript
{
public:
    UnbondAnnounce() : PlayerScript("UnbondAnnounce") { }

    void OnLogin(Player* player) override
    {
        if (sConfigMgr->GetOption<bool>("Mod_Unbond.Enable", false))
        {
            ChatHandler(player->GetSession()).SendSysMessage("This server is running the |cff4CFF00Unbond |rmodule.");
        }
    }
};

class Unbond_npc :public CreatureScript
{
public:
    Unbond_npc() : CreatureScript("Unbond_npc") { }

private:
    const uint8 PageSize = 13;

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        std::ostringstream messageAction;
        player->PlayerTalkClass->ClearMenus();

        messageAction << "Select from my bags.";
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Please select item that you want to unbond.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        AddGossipItemFor(player, GOSSIP_ICON_VENDOR, messageAction.str(), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 100);
        SendGossipMenuFor(player, 601644, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 uiAction) override
    {
        if (sender != GOSSIP_SENDER_MAIN)
            return false;
        // Initialize
        player->PlayerTalkClass->ClearMenus();

        //Back
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            OnGossipHello(player, creature);
        }

        if (uiAction >= GOSSIP_ACTION_INFO_DEF + 100 && uiAction < GOSSIP_ACTION_INFO_DEF + 200)
        {
            std::vector<Item*> items = GetWeaponArmorFromPlayerInventory(player);
            if (items.size() == 0)
            {
                auto chatHandler = ChatHandler(player->GetSession());
                chatHandler.PSendSysMessage("You don't have weapons or armors to unbond.");
                OnGossipHello(player, creature);
                return true;
            }

            int maxPage = items.size() / PageSize + (items.size() % PageSize != 0);
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Back..", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            int page = uiAction - (GOSSIP_ACTION_INFO_DEF + 100) + 1;
            if (page > 1)
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Previous..", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 100 + page - 2);
            AddGossipMenuForUnbond(player, items, page);
            if (page < maxPage)
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Next..", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 100 + page);
            player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
            return true;
        }
        else if (uiAction >= GOSSIP_ACTION_INFO_DEF + 200)
        {
            int itemId = uiAction - (GOSSIP_ACTION_INFO_DEF + 200);
            UnbondItem(player, itemId);
            player->PlayerTalkClass->ClearMenus();
            OnGossipHello(player, creature);
            return true;
        }

        return true;
    }

    std::vector<Item*> GetWeaponArmorFromPlayerInventory(Player* player)
    {
        std::vector<Item*> items;
        ////Iterate through equiped items and get weapons and armors
        //for (uint8 i = EQUIPMENT_SLOT_START;i < EQUIPMENT_SLOT_END; i++)
        //{
        //    if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        //    {
        //        ItemTemplate const* itemTemplate = item->GetTemplate();
        //        //Ignore if quality is too low or out of range
        //        if (itemTemplate->Quality < 1 || itemTemplate->Quality > 5)
        //            continue;
        //        //Get weapons
        //        if (item && itemTemplate->Class == 2)
        //        {
        //            items.push_back(item);
        //        }
        //        else if (item && itemTemplate->Class == 4)
        //        {
        //            items.push_back(item);
        //        }
        //    }
        //}

        //Iterate through backpacks and get weapons and armors
        for (uint8 i = INVENTORY_SLOT_ITEM_START;i < INVENTORY_SLOT_ITEM_END; i++)
        {
            Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
            if (item)
            {
                ItemTemplate const* itemTemplate = item->GetTemplate();
                //Ignore if quality is too low or out of range
                if (itemTemplate->Quality < 1 || itemTemplate->Quality > 5)
                    continue;
                //Get weapons
                if (item && itemTemplate->Class == 2)
                {
                    items.push_back(item);
                }
                else if (item && itemTemplate->Class == 4)
                {
                    items.push_back(item);
                }
            }
        }

        //Iterate through bags and get weapons and armors
        for (uint8 i = INVENTORY_SLOT_BAG_START;i < INVENTORY_SLOT_BAG_END; i++)
        {
            if (Bag* bag = player->GetBagByPos(i))
            {
                for (int j = 0;j < bag->GetBagSize();j++)
                {
                    Item* item = bag->GetItemByPos(j);
                    if (item)
                    {
                        ItemTemplate const* itemTemplate = item->GetTemplate();
                        //Ignore if quality is too low or out of range
                        if (itemTemplate->Quality < 1 || itemTemplate->Quality > 5)
                            continue;
                        //Get weapons
                        if (item && itemTemplate->Class == 2)
                        {
                            items.push_back(item);
                        }
                        else if (item && itemTemplate->Class == 4)
                        {
                            items.push_back(item);
                        }
                    }
                }
            }
        }

        //Ignore items that doesn't have sell price
        std::vector<Item*> result;
        for (auto item : items)
        {
            ItemTemplate const* itemTemplate = item->GetTemplate();
            auto money = itemTemplate->SellPrice;
            //Ignore ones that has no price
            if (money == 0)
                continue;
            //Ignore ones that are not bonded
            if (!item->IsSoulBound())
                continue;
            //Only ones that is equiped binding
            if (itemTemplate->Bonding == BIND_WHEN_EQUIPED)
                result.push_back(item);
        }
        return result;
    }

    void AddGossipMenuForUnbond(Player* player, std::vector<Item*> items, int page)
    {
        LocaleConstant locale = LOCALE_enUS;
        if (player && player->GetSession())
        {
            locale = player->GetSession()->GetSessionDbLocaleIndex();
        }

        int index = 0;
        for (auto item : items)
        {
            if (index >= (page - 1) * PageSize && index < page * PageSize)
            {
                ItemTemplate const* itemTemplate = item->GetTemplate();
                auto money = itemTemplate->SellPrice;

                std::string name = itemTemplate->Name1;
                if (locale > LOCALE_enUS)
                {
                    if (ItemLocale const* itemLocale = sObjectMgr->GetItemLocale(itemTemplate->ItemId))
                    {
                        ObjectMgr::GetLocaleString(itemLocale->Name, locale, name);
                    }
                }
                auto randPropertySuffix = GetRandomPropertySuffix(player, item);
                name = randPropertySuffix + name + " |" + FormatMoneyShort(money);
                auto formatedMoney = FormatMoney(money);
                AddGossipItemFor(player, GOSSIP_ICON_VENDOR, name, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 200 + item->GetEntry(),
                    "Do you want to unbond this item with " + formatedMoney + "?", money, false);
            }
            index++;
            if (index >= page * PageSize)
                break;
        }
    }

    void UnbondItem(Player* player, int itemId)
    {
        ChatHandler chathandler = ChatHandler(player->GetSession());
        std::ostringstream messageAction;
        Item* item = player->GetItemByEntry(itemId);
        if (!item)
        {
            messageAction << "Error, item not found!";
            chathandler.PSendSysMessage(messageAction.str().c_str());
            return;
        }

        ItemTemplate const* itemTemplate = item->GetTemplate();
        int32 money = itemTemplate->SellPrice;
        if (money < 0)
            money *= -1;
        if (!player->HasEnoughMoney(money))
        {
            messageAction << "Sorry, you don't have enough money!";
            chathandler.PSendSysMessage(messageAction.str().c_str());
            return;
        }

        player->ModifyMoney(-money);
        item->SetBinding(false);
        item->SetState(ITEM_CHANGED, player);
        player->CastSpell(player, 47292);
        chathandler.PSendSysMessage("Successfully removed soul bound.");        
    }

    std::string GetRandomPropertySuffix(Player* player, Item* item)
    {
        int locdbc_idx = player->GetSession()->GetSessionDbcLocale();
        int32 propRefID = item->GetItemRandomPropertyId();
        std::string name;
        if (propRefID)
        {
            // Append the suffix to the name (ie: of the Monkey) if one exists
            // These are found in ItemRandomSuffix.dbc and ItemRandomProperties.dbc
            // even though the DBC name seems misleading
            std::array<char const*, 16> const* suffix = nullptr;

            if (propRefID < 0)
            {
                ItemRandomSuffixEntry const* itemRandEntry = sItemRandomSuffixStore.LookupEntry(-item->GetItemRandomPropertyId());
                if (itemRandEntry)
                    suffix = &itemRandEntry->Name;
            }
            else
            {
                ItemRandomPropertiesEntry const* itemRandEntry = sItemRandomPropertiesStore.LookupEntry(item->GetItemRandomPropertyId());
                if (itemRandEntry)
                    suffix = &itemRandEntry->Name;
            }

            // dbc local name
            if (suffix)
            {
                name = (*suffix)[locdbc_idx >= 0 ? locdbc_idx : LOCALE_enUS];
            }
        }
        return name;
    }

    std::string FormatMoney(uint32 money)
    {
        uint32 copper = money % 100;
        uint32 silver = (money / 100) % 100;
        uint32 gold = (money / 10000) % 100;
        std::ostringstream str;
        if (gold > 0)
            str << " " << gold << " gold";

        if (silver > 0)
            str << " " << silver << " silver";

        if (copper > 0)
            str << " " << copper << " copper";
        return str.str();
    }

    std::string FormatMoneyShort(uint32 money)
    {
        uint32 copper = money % 100;
        uint32 silver = (money / 100) % 100;
        uint32 gold = (money / 10000) % 100;
        std::ostringstream str;
        if (gold > 0)
            str << " " << gold << "g";

        if (silver > 0)
            str << " " << silver << "s";

        if (copper > 0)
            str << " " << copper << "c";
        return str.str();
    }
};

// Add all scripts in one
void AddUnbondScripts()
{
    new UnbondAnnounce();
    new Unbond_npc();
}
