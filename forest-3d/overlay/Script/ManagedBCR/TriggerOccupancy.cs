namespace ManagedBCR;

internal enum ZoneTransition { None, OneEntered, TwoEntered, Emptied }

internal sealed class TriggerOccupancy
{
    private readonly Dictionary<int, HashSet<(int Component, int Body)>> inner = new();
    private readonly Dictionary<int, HashSet<(int Component, int Body)>> outer = new();

    internal ZoneTransition Change(bool isInner, int player, int component, int body, bool entered)
    {
        var zone = isInner ? inner : outer;
        if (entered)
        {
            if (zone.TryGetValue(player, out var existing))
            {
                existing.Add((component, body));
                return ZoneTransition.None;
            }

            zone.Add(player, [(component, body)]);
            return zone.Count switch
            {
                1 => ZoneTransition.OneEntered,
                2 => ZoneTransition.TwoEntered,
                _ => ZoneTransition.None
            };
        }

        if (!zone.TryGetValue(player, out var overlaps) ||
            !overlaps.Remove((component, body)) || overlaps.Count != 0)
            return ZoneTransition.None;
        zone.Remove(player);
        return zone.Count == 0 ? ZoneTransition.Emptied : ZoneTransition.None;
    }
}
