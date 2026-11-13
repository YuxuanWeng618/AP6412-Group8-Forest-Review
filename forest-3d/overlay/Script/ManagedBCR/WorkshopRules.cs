using System;

namespace ManagedBCR;

// Station occupancy for the C# QTE round. The workshop stations, recipe, ropes and
// outputs moved to the shared C++ rules (forest-2d Domain) used by UBCRWorkshopRulesBridge.
internal sealed class WorkstationSlots
{
    private readonly int[] players = [-1, -1];

    public int Count => (players[0] >= 0 ? 1 : 0) + (players[1] >= 0 ? 1 : 0);

    public int Find(int player) => player < 0 ? -1 : Array.IndexOf(players, player);

    public bool Join(int slot, int player)
    {
        if (slot is < 0 or > 1 || player < 0 || players[slot] >= 0 || Find(player) >= 0)
            return false;
        players[slot] = player;
        return true;
    }

    public void Reset() => Array.Fill(players, -1);
}
