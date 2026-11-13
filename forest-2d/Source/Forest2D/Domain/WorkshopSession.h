#pragma once
#include <array>
#include <map>
#include <set>

// No UE, positions, input keys or rendering dependencies. The 2D game and the
// 3D UE workshop (forest-ue-assignment) both compile this file.
namespace Forest
{
enum class Stage { Order, Discover, Harvest, ReturnLog, LoadSaw, Saw, Supply, Ropes, Deliver, Complete };
enum class ItemKind { Log, Plank, Package, Stone };

class StationPair
{
    std::array<int, 2> Owners{{-1, -1}};
public:
    int Owner(int slot) const { return slot >= 0 && slot < 2 ? Owners[slot] : -1; }
    int Find(int player) const
    {
        if (player < 0 || player > 1) return -1;
        return Owners[0] == player ? 0 : Owners[1] == player ? 1 : -1;
    }
    bool Full() const { return Owners[0] >= 0 && Owners[1] >= 0; }
    bool Join(int slot, int player)
    {
        if (slot < 0 || slot > 1 || player < 0 || player > 1 || Owners[slot] >= 0 || Find(player) >= 0) return false;
        Owners[slot] = player;
        return true;
    }
    bool Leave(int player)
    {
        const int slot = Find(player);
        if (slot < 0) return false;
        Owners[slot] = -1;
        return true;
    }
    void Reset() { Owners = {{-1, -1}}; }
};

// Distinct items counted against per-type requirements. Reserve then Finish lets
// an adapter count an item only after the world actually consumed it.
class ItemRecipe
{
    std::map<int, int> Remaining;
    std::map<int, int> Pending;
    std::set<int> Used;
    int Total = 0;
public:
    void Require(int type, int count)
    {
        if (count > 0 && Remaining.emplace(type, count).second) Total += count;
    }
    int RemainingFor(int type) const
    {
        const auto Found = Remaining.find(type);
        return Found == Remaining.end() ? 0 : Found->second;
    }
    int RemainingTotal() const { return Total; }
    bool Ready() const { return !Remaining.empty() && Total == 0; }
    bool Reserve(int id, int type)
    {
        if (id < 0 || Pending.count(id) || Used.count(id) || RemainingFor(type) <= 0) return false;
        int reserved = 0;
        for (const auto& Entry : Pending) reserved += Entry.second == type ? 1 : 0;
        return reserved < RemainingFor(type) && Pending.emplace(id, type).second;
    }
    bool Finish(int id, bool consumed)
    {
        const auto Found = Pending.find(id);
        if (Found == Pending.end()) return false;
        const int type = Found->second;
        Pending.erase(Found);
        if (!consumed) return false;
        Used.insert(id);
        --Remaining[type];
        --Total;
        return true;
    }
};

class PlankRecipe
{
    ItemRecipe Planks;
public:
    static constexpr int Required = 3;
    PlankRecipe() { Planks.Require(static_cast<int>(ItemKind::Plank), Required); }
    int Count() const { return Required - Planks.RemainingTotal(); }
    bool Ready() const { return Planks.Ready(); }
    bool Consume(int id, ItemKind kind)
    {
        return Planks.Reserve(id, static_cast<int>(kind)) && Planks.Finish(id, true);
    }
};

// Both ropes must be pulled together by two different players.
class RopePair
{
    int ApprovedLeft = -1;
    int ApprovedRight = -1;
public:
    void Observe(int left, int right, bool leftPulled, bool rightPulled)
    {
        if (left < 0 || right < 0 || left == right || left != ApprovedLeft || right != ApprovedRight)
            ApprovedLeft = ApprovedRight = -1;
        if (left >= 0 && right >= 0 && left != right && leftPulled && rightPulled)
        {
            ApprovedLeft = left;
            ApprovedRight = right;
        }
    }
    bool CanActivate(int left, int right) const
    {
        return left >= 0 && right >= 0 && left == ApprovedLeft && right == ApprovedRight;
    }
};

// A one-off result (package, chair) is settled only after it was really created.
class OutputGate
{
    bool Waiting = false;
    bool Done = false;
public:
    bool Completed() const { return Done; }
    bool Reserve(bool eligible)
    {
        if (!eligible || Waiting || Done) return false;
        Waiting = true;
        return true;
    }
    bool Finish(bool created)
    {
        if (!Waiting) return false;
        Waiting = false;
        Done = created;
        return created;
    }
};

// Input adapters produce distinct direction edges, not held/repeated keys.
// The 2D QTE uses explicit, visible targets rather than missing 3D asset values.
class CooperativeQte
{
    std::array<int, 2> Counts{{0, 0}};
    std::array<int, 2> LastDirection{{-1, -1}};
public:
    static constexpr int Target = 8;
    int Progress(int slot) const { return slot >= 0 && slot < 2 ? Counts[slot] : 0; }
    bool Done() const { return Counts[0] >= Target && Counts[1] >= Target; }
    bool Input(int slot, int direction, bool blade, bool bothPresent)
    {
        if (!bothPresent || slot < 0 || slot > 1 || Counts[slot] >= Target) return false;
        if (!blade)
        {
            if (direction < 0 || direction > 3 || LastDirection[slot] == direction) return false;
            LastDirection[slot] = direction;
        }
        ++Counts[slot];
        return true;
    }
};

class WorkshopSession
{
    Stage Current = Stage::Order;
    StationPair Stations;
    PlankRecipe Recipe;
    CooperativeQte Qte;
    RopePair Ropes;
public:
    Stage GetStage() const { return Current; }
    const StationPair& GetStations() const { return Stations; }
    const PlankRecipe& GetRecipe() const { return Recipe; }
    const CooperativeQte& GetQte() const { return Qte; }
    bool AcceptOrder() { return Advance(Stage::Order); }
    bool Discover() { return Advance(Stage::Discover); }
    bool ReturnLog() { return Advance(Stage::ReturnLog); }
    bool LoadSaw(ItemKind kind) { return kind == ItemKind::Log && Advance(Stage::LoadSaw); }
    bool Join(int slot, int player)
    {
        if (Current != Stage::Harvest && Current != Stage::Saw && Current != Stage::Ropes) return false;
        return Stations.Join(slot, player);
    }
    bool Leave(int player) { return Stations.Leave(player); }
    bool Work(int player, int direction, bool action)
    {
        const int slot = Stations.Find(player);
        if (slot < 0 || (Current != Stage::Harvest && Current != Stage::Saw)) return false;
        const bool blade = Current == Stage::Saw && slot == 1;
        if (blade && !action) return false;
        if (!blade && (direction < 0 || (Current == Stage::Harvest && direction != 1 && direction != 3))) return false;
        if (!Qte.Input(slot, direction, blade, Stations.Full())) return false;
        if (Qte.Done())
        {
            Current = Current == Stage::Harvest ? Stage::ReturnLog : Stage::Supply;
            Stations.Reset();
            Qte = CooperativeQte();
        }
        return true;
    }
    bool Supply(int id, ItemKind kind)
    {
        if (Current != Stage::Supply || !Recipe.Consume(id, kind)) return false;
        if (Recipe.Ready()) Current = Stage::Ropes;
        return true;
    }
    bool Pull(bool p1Held, bool p2Held)
    {
        if (Current != Stage::Ropes || !Recipe.Ready() || !Stations.Full()) return false;
        const int left = Stations.Owner(0);
        const int right = Stations.Owner(1);
        Ropes.Observe(left, right, left == 0 ? p1Held : p2Held, right == 0 ? p1Held : p2Held);
        if (!Ropes.CanActivate(left, right)) return false;
        Current = Stage::Deliver;
        Stations.Reset();
        return true;
    }
    bool Deliver(ItemKind kind) { return kind == ItemKind::Package && Advance(Stage::Deliver); }
private:
    bool Advance(Stage expected)
    {
        if (Current != expected) return false;
        Current = static_cast<Stage>(static_cast<int>(Current) + 1);
        return true;
    }
};
}
