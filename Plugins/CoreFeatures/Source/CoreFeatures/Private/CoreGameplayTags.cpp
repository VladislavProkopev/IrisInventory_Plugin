#include "CoreGameplayTags.h"

namespace CoreGameplayTags
{
	namespace ADVMInputTags
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Action_Move, "ADVM.Input.Action.Walk", "Trigger walk movement ability.");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Action_Look, "ADVM.Input.Action.Look", "Trigger look movement ability.");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Action_Jump, "ADVM.Input.Action.Jump", "Trigger jump movement ability.");
	}
	namespace InitStateTags
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_Spawned, "InitState.Spawned", "Actor or Component spawned in world");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_DataAvaliable,"InitState.DataAvaliable", "Actor or Component data available");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_DataInitialized,"InitState.DataInitialized", "Actor or Component data initialized");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_GameplayReady,"InitState.GameplayReady", "Actor or Component game play ready");
	}
	
	
	
	namespace InventoryTags
	{
		UE_DEFINE_GAMEPLAY_TAG(Item_Stat_MaxStackSize, "Item.Stat.MaxStackSize");
		UE_DEFINE_GAMEPLAY_TAG(Item_Stat_Weight, "Item.Stat.Weight");
		UE_DEFINE_GAMEPLAY_TAG(Item_Stat_BaseValue, "Item.Stat.BaseValue");
		
		UE_DEFINE_GAMEPLAY_TAG(Item_State_StackCount, "Item.State.StackCount");
		UE_DEFINE_GAMEPLAY_TAG(Item_State_Durability, "Item.State.Durability");
		UE_DEFINE_GAMEPLAY_TAG(Item_State_Level, "Item.State.Level");
		
		UE_DEFINE_GAMEPLAY_TAG(Item_Type_Weapon, "Item.Type.Weapon");
		UE_DEFINE_GAMEPLAY_TAG(Item_Type_Consumable, "Item.Type.Consumable");
		UE_DEFINE_GAMEPLAY_TAG(Item_Type_Material, "Item.Type.Material");
		
		UE_DEFINE_GAMEPLAY_TAG(Item_Slot_PrimaryWeapon, "Item.Slot.PrimaryWeapon");
		UE_DEFINE_GAMEPLAY_TAG(Item_Slot_SecondaryWeapon, "Item.Slot.SecondaryWeapon");
		UE_DEFINE_GAMEPLAY_TAG(Item_Slot_Backpack, "Item.Slot.Backpack");
		UE_DEFINE_GAMEPLAY_TAG(Item_Slot_Head, "Item.Slot.Head");
		UE_DEFINE_GAMEPLAY_TAG(Item_Slot_Chest, "Item.Slot.Chest");
		
		UE_DEFINE_GAMEPLAY_TAG(Event_Inventory_ItemAdded, "Item.Inventory.ItemAdded");
		UE_DEFINE_GAMEPLAY_TAG(Event_Inventory_ItemRemoved, "Item.Inventory.ItemRemoved");
	}

	namespace GMR
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Inventory_Message_Updated, "Inventory.Message.Updated","Send Gameplay Message Router when Inventory Updated");
	}
}

