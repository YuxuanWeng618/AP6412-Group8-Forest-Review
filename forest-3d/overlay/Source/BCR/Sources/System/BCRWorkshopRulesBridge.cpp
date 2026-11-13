#include "BCR/Headers/System/BCRWorkshopRulesBridge.h"

#include <map>
#include <set>
#include <vector>
#include "WorkshopSession.h" // Shared with the 2D game: forest-2d/Source/Forest2D/Domain

namespace
{
// Values of the QTE round state passed in by UQTE_Subsystem.
enum class ERoundState : int32 { Inactive, Waiting, Running, Paused, Completed, Failed };
enum class EDeposit : int32 { Rejected, Accepted, AlreadyPresent };

// 3D only: the saw machine accepts named parts and can swap a log for its cut version.
class FMachineInput
{
    std::vector<FString> Requirements;
    std::set<int32> Present;
    std::map<int32, FString> Fulfilled;
    int32 Replacement = INDEX_NONE;
    bool bHasReplacement = false;

    void Fulfil(int32 Item, int32 Index)
    {
        Fulfilled.emplace(Item, Requirements[Index]);
        Requirements.erase(Requirements.begin() + Index);
    }

public:
    bool Supplied() const { return Requirements.empty(); }
    bool HasItems() const { return !Present.empty(); }
    void Reset()
    {
        Requirements.clear();
        Fulfilled.clear();
        bHasReplacement = false;
    }
    void Add(const FString& Name) { Requirements.push_back(Name); }
    int32 Find(const FString& Name) const
    {
        for (int32 Index = 0; Index < static_cast<int32>(Requirements.size()); ++Index)
            if (Requirements[Index].Equals(Name, ESearchCase::IgnoreCase)) return Index;
        return INDEX_NONE;
    }
    EDeposit Deposit(int32 Item, const FString& Name, bool bInside)
    {
        if (Item < 0) return EDeposit::Rejected;
        if (Present.count(Item)) return EDeposit::AlreadyPresent;
        if (!bInside) return EDeposit::Rejected;
        const int32 Index = Find(Name);
        if (Index < 0 && !Supplied()) return EDeposit::Rejected;
        Present.insert(Item);
        if (Index >= 0) Fulfil(Item, Index);
        return EDeposit::Accepted;
    }
    void Stage(int32 Index)
    {
        Replacement = Index;
        bHasReplacement = true;
    }
    bool Register(int32 Item)
    {
        const bool bIndexed = bHasReplacement;
        const int32 Index = Replacement;
        bHasReplacement = false;
        if (Item < 0 || (bIndexed && (Index < 0 || Index >= static_cast<int32>(Requirements.size()))) ||
            !Present.insert(Item).second) return false;
        if (bIndexed) Fulfil(Item, Index);
        return true;
    }
    void Forget(int32 Item)
    {
        Present.erase(Item);
        const auto Found = Fulfilled.find(Item);
        if (Found == Fulfilled.end()) return;
        Requirements.push_back(Found->second);
        Fulfilled.erase(Found);
    }
};
}

struct FForestWorkshopRulesState
{
    Forest::StationPair Stations;   // shared
    Forest::ItemRecipe Recipe;      // shared
    Forest::RopePair Ropes;         // shared
    Forest::OutputGate Output;      // shared
    FMachineInput Machine;          // 3D adapter
    bool bOwnsRound = false;        // 3D adapter
};

UBCRWorkshopRulesBridge::UBCRWorkshopRulesBridge()
    : State(MakeShared<FForestWorkshopRulesState>())
{
}

UBCRWorkshopRulesBridge* UBCRWorkshopRulesBridge::Create(UObject* Owner)
{
    return NewObject<UBCRWorkshopRulesBridge>(Owner);
}

int32 UBCRWorkshopRulesBridge::InteractWithStation(int32 Player, bool bSupplied, float FirstDistance, float SecondDistance)
{
    const int32 Occupied = State->Stations.Find(Player);
    if (Occupied >= 0 && State->Stations.Leave(Player)) return 2 + Occupied;
    const int32 Preferred = FirstDistance <= SecondDistance ? 0 : 1;
    return bSupplied && State->Stations.Join(Preferred, Player) ? Preferred : INDEX_NONE;
}

bool UBCRWorkshopRulesBridge::AreStationsFilled() { return State->Stations.Full(); }

void UBCRWorkshopRulesBridge::ResetMachineRequirements() { State->Machine.Reset(); }
void UBCRWorkshopRulesBridge::AddMachineRequirement(const FString& ItemName) { State->Machine.Add(ItemName); }
int32 UBCRWorkshopRulesBridge::FindMachineRequirement(const FString& ItemName) { return State->Machine.Find(ItemName); }
bool UBCRWorkshopRulesBridge::IsMachineSupplied() { return State->Machine.Supplied(); }
bool UBCRWorkshopRulesBridge::HasMachineItems() { return State->Machine.HasItems(); }
int32 UBCRWorkshopRulesBridge::DepositMachineItem(int32 Item, const FString& ItemName, bool bInside)
{
    return static_cast<int32>(State->Machine.Deposit(Item, ItemName, bInside));
}
void UBCRWorkshopRulesBridge::StageMachineReplacement(int32 Index) { State->Machine.Stage(Index); }
bool UBCRWorkshopRulesBridge::RegisterMachineReplacement(int32 Item) { return State->Machine.Register(Item); }
void UBCRWorkshopRulesBridge::ForgetMachineItem(int32 Item) { State->Machine.Forget(Item); }

bool UBCRWorkshopRulesBridge::CanStartMachineRound(int32 RoundState)
{
    const ERoundState Round = static_cast<ERoundState>(RoundState);
    return State->Machine.Supplied() && State->Stations.Full() &&
        (Round == ERoundState::Inactive || Round == ERoundState::Completed || Round == ERoundState::Failed);
}

bool UBCRWorkshopRulesBridge::ConfirmMachineRound(int32 RoundState)
{
    if (static_cast<ERoundState>(RoundState) != ERoundState::Waiting) return false;
    State->bOwnsRound = true;
    return true;
}

bool UBCRWorkshopRulesBridge::OwnsMachineRound() { return State->bOwnsRound; }
void UBCRWorkshopRulesBridge::ReleaseMachineRound() { State->bOwnsRound = false; }
bool UBCRWorkshopRulesBridge::ShouldFinishSawRound(bool bCut) { return State->bOwnsRound && bCut; }
bool UBCRWorkshopRulesBridge::ShouldFinishTreeRound(int32 CutsRemaining) { return State->bOwnsRound && CutsRemaining == 0; }
bool UBCRWorkshopRulesBridge::ShouldUnlockReplacementCarry(bool bHeldIsValid, bool bHeldWasReplacedLog)
{
    return !bHeldIsValid || bHeldWasReplacedLog;
}

void UBCRWorkshopRulesBridge::ResetRecipe()
{
    State->Recipe = Forest::ItemRecipe();
    State->Output = Forest::OutputGate();
    State->Ropes = Forest::RopePair();
}

void UBCRWorkshopRulesBridge::AddIngredient(int32 Type, int32 Count) { State->Recipe.Require(Type, Count); }
int32 UBCRWorkshopRulesBridge::RemainingIngredient(int32 Type) { return State->Recipe.RemainingFor(Type); }
int32 UBCRWorkshopRulesBridge::RemainingTotal() { return State->Recipe.RemainingTotal(); }
bool UBCRWorkshopRulesBridge::ReserveIngredient(int32 Item, int32 Type) { return State->Recipe.Reserve(Item, Type); }
bool UBCRWorkshopRulesBridge::FinishIngredient(int32 Item, bool bConsumed) { return State->Recipe.Finish(Item, bConsumed); }

// A dropped part is queued once released inside the zone, and consumed once detached there.
bool UBCRWorkshopRulesBridge::ShouldQueueIngredient(bool bStillHeld, bool bInZone) { return !bStillHeld && bInZone; }
bool UBCRWorkshopRulesBridge::CanConsumeIngredient(bool bAttached, bool bInZone) { return !bAttached && bInZone; }

void UBCRWorkshopRulesBridge::ObserveRopes(int32 LeftPlayer, int32 RightPlayer, bool bLeftPulled, bool bRightPulled)
{
    if (State->Recipe.Ready()) State->Ropes.Observe(LeftPlayer, RightPlayer, bLeftPulled, bRightPulled);
}

bool UBCRWorkshopRulesBridge::ReserveCraft(int32 LeftPlayer, int32 RightPlayer, bool bLeftPulled, bool bRightPulled)
{
    ObserveRopes(LeftPlayer, RightPlayer, bLeftPulled, bRightPulled);
    return State->Output.Reserve(State->Recipe.Ready() && State->Ropes.CanActivate(LeftPlayer, RightPlayer));
}

bool UBCRWorkshopRulesBridge::ReserveDelivery(int32 ActualType, int32 RequiredType)
{
    return State->Output.Reserve(RequiredType >= 0 && ActualType == RequiredType);
}

bool UBCRWorkshopRulesBridge::FinishOutput(bool bSpawned) { return State->Output.Finish(bSpawned); }
