#include "StrategyEquipmentVisualComponent.h"

UStrategyEquipmentVisualComponent::UStrategyEquipmentVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

FName UStrategyEquipmentVisualComponent::GetSocketForSlot(
    EStrategyEquipmentSlot Slot) const
{
    switch (Slot)
    {
        case EStrategyEquipmentSlot::PrimaryWeapon:
            return SocketMap.PrimaryWeaponSocket;

        case EStrategyEquipmentSlot::Bayonet:
            return SocketMap.BayonetSocket;

        case EStrategyEquipmentSlot::Sabre:
            return SocketMap.SabreSocket;

        case EStrategyEquipmentSlot::Tool:
            return SocketMap.ToolSocket;

        case EStrategyEquipmentSlot::Backpack:
            return SocketMap.BackpackSocket;

        case EStrategyEquipmentSlot::CartridgeBox:
            return SocketMap.CartridgeBoxSocket;

        case EStrategyEquipmentSlot::Sidearm:
        case EStrategyEquipmentSlot::Headgear:
        default:
            return NAME_None;
    }
}
