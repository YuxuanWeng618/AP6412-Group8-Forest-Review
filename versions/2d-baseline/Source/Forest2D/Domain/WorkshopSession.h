#pragma once
#include <array>
#include <set>

// No UE, positions, input keys or rendering dependencies. A future 3D adapter
// can use these exact rules. Existing ManagedBCR is a separate implementation.
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

class PlankRecipe
{
    std::set<int> Consumed;
public:
    static constexpr int Required = 3;
    int Count() const { return static_cast<int>(Consumed.size()); }
    bool Ready() const { return Count() == Required; }
    bool Consume(int id, ItemKind kind)
    {
        return id >= 0 && kind == ItemKind::Plank && !Ready() && Consumed.insert(id).second;
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
        if (Current != Stage::Ropes || !Recipe.Ready() || !Stations.Full() || !p1Held || !p2Held) return false;
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
