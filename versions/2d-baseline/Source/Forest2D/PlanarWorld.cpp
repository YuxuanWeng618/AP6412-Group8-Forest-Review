#include "PlanarWorld.h"

using namespace Forest;
const FVector2D FPlanarWorld::Elder(180, 430);
const FVector2D FPlanarWorld::Grove(1120, 260);
const FVector2D FPlanarWorld::Saw(1060, 490);
const FVector2D FPlanarWorld::Assembler(550, 465);

FPlanarWorld::FPlanarWorld() { Reset(); }
void FPlanarWorld::Reset()
{
    Session = WorkshopSession();
    Players[0] = {FVector2D(290, 410), FVector2D(-1, 0), -1};
    Players[1] = {FVector2D(290, 490), FVector2D(-1, 0), -1};
    Items.Empty();
    NextItem = 0;
    Spawn(ItemKind::Stone, FVector2D(870, 650));
    Notice.Empty(); NoticeSeconds = 0;
    Started = false; Paused = false; Help = false; PlaySeconds = 0;
}
void FPlanarWorld::Spawn(ItemKind kind, FVector2D position)
{
    Items.Add({NextItem++, kind, position, -1, false});
}
void FPlanarWorld::Announce(const FString& message) { Notice = message; NoticeSeconds = 3.5f; }
TArray<FBox2D> FPlanarWorld::Obstacles()
{
    return {FBox2D(FVector2D(650, 225), FVector2D(805, 330)),
        FBox2D(FVector2D(1070, 190), FVector2D(1170, 270)),
        FBox2D(FVector2D(975, 445), FVector2D(1145, 515)),
        FBox2D(FVector2D(510, 420), FVector2D(590, 460)),
        FBox2D(FVector2D(105, 290), FVector2D(225, 355))};
}
bool FPlanarWorld::CanStand(FVector2D p) const
{
    constexpr double radius = 17;
    if (!FMath::IsFinite(p.X) || !FMath::IsFinite(p.Y) || p.X < 70 || p.X > 1370 || p.Y < 185 || p.Y > 725) return false;
    for (const FBox2D& box : Obstacles())
    {
        const FVector2D closest(FMath::Clamp(p.X, box.Min.X, box.Max.X), FMath::Clamp(p.Y, box.Min.Y, box.Max.Y));
        if (FVector2D::DistSquared(p, closest) < radius * radius) return false;
    }
    return true;
}
const FForestItem* FPlanarWorld::Held(int player) const
{
    if (player < 0 || player > 1) return nullptr;
    return Items.FindByPredicate([&](const FForestItem& item) { return item.Id == Players[player].HeldItem && !item.Consumed; });
}
FVector2D FPlanarWorld::StationPosition(int slot) const
{
    if (Session.GetStage() == Stage::Harvest) return FVector2D(1040 + slot * 160, 325);
    if (Session.GetStage() == Stage::Saw) return FVector2D(970 + slot * 180, 570);
    return FVector2D(440 + slot * 220, 510);
}
void FPlanarWorld::Move(int player, FVector2D axis, float dt)
{
    if (Session.GetStations().Find(player) >= 0 || axis.IsNearlyZero()) return;
    if (axis.SizeSquared() > 1) axis.Normalize();
    auto& actor = Players[player];
    actor.Facing = axis.GetSafeNormal();
    const double speed = actor.HeldItem >= 0 ? 155 : 215;
    const FVector2D displacement = axis * speed * dt;
    // Small substeps prevent tunnelling on low frame rates. Axes slide separately.
    const int steps = FMath::Max(1, FMath::CeilToInt(displacement.Size() / 6));
    for (int i = 0; i < steps; ++i)
    {
        FVector2D next = actor.Position + FVector2D(displacement.X / steps, 0);
        if (CanStand(next)) actor.Position = next;
        next = actor.Position + FVector2D(0, displacement.Y / steps);
        if (CanStand(next)) actor.Position = next;
    }
}
void FPlanarWorld::Interact(int player)
{
    if (Session.Leave(player)) { Announce(TEXT("Station released. Your partner can swap roles.")); return; }
    const FVector2D pos = Players[player].Position;
    if (Session.GetStage() == Stage::Order && FVector2D::Distance(pos, Elder) < 110)
    {
        Session.AcceptOrder(); Announce(TEXT("Grandfather: Could you build me a rocking chair?")); return;
    }
    const auto* item = Held(player);
    if (item && FVector2D::Distance(pos, Saw) < 150)
    {
        if (Session.LoadSaw(item->Kind))
        {
            for (auto& candidate : Items) if (candidate.Id == Players[player].HeldItem) candidate.Consumed = true;
            Players[player].HeldItem = -1;
            Announce(TEXT("Log loaded. Take the wheel and blade stations."));
        }
        else Announce(TEXT("The saw needs the harvested log at the correct order step."));
        return;
    }
    const Stage stage = Session.GetStage();
    if (stage == Stage::Harvest || stage == Stage::Saw || stage == Stage::Ropes)
    {
        if (item) { Announce(TEXT("Put the item down before joining a station.")); return; }
        const int slot = FVector2D::DistSquared(pos, StationPosition(0)) <= FVector2D::DistSquared(pos, StationPosition(1)) ? 0 : 1;
        if (FVector2D::Distance(pos, StationPosition(slot)) < 68)
        {
            if (Session.Join(slot, player))
            {
                Players[player].Position = StationPosition(slot);
                Announce(Session.GetStations().Full() ? TEXT("Both ready. Follow your station's controls!") : TEXT("Waiting for a partner at the other station."));
            }
            else Announce(TEXT("This station is occupied. Use the other circle."));
            return;
        }
    }
    Announce(TEXT("Move closer to the highlighted task or an empty station."));
}
void FPlanarWorld::PickOrDrop(int player)
{
    if (Session.GetStations().Find(player) >= 0) { Announce(TEXT("Leave your station before carrying items.")); return; }
    auto& actor = Players[player];
    if (actor.HeldItem >= 0)
    {
        auto* item = Items.FindByPredicate([&](const auto& candidate) { return candidate.Id == actor.HeldItem; });
        if (!item) { actor.HeldItem = -1; return; }
        FVector2D destination = actor.Position + actor.Facing * 32;
        if (!CanStand(destination)) destination = actor.Position;
        item->HeldBy = -1; item->Position = destination; actor.HeldItem = -1;
        if (FVector2D::Distance(destination, Assembler) < 140)
        {
            if (Session.Supply(item->Id, item->Kind))
            {
                item->Consumed = true;
                Announce(FString::Printf(TEXT("Wood supplied: %d / 3"), Session.GetRecipe().Count()));
            }
            else Announce(TEXT("Only three distinct planks can fill the recipe. Finish sawing first."));
        }
        return;
    }
    FForestItem* nearest = nullptr;
    double distance = 65;
    for (auto& item : Items)
    {
        const double candidate = FVector2D::Distance(actor.Position, item.Position);
        if (!item.Consumed && item.HeldBy < 0 && candidate < distance) { nearest = &item; distance = candidate; }
    }
    if (!nearest) { Announce(TEXT("Stand beside a log, plank or package to pick it up.")); return; }
    nearest->HeldBy = player; actor.HeldItem = nearest->Id;
}
void FPlanarWorld::ProcessStageChange(Stage previous)
{
    const Stage stage = Session.GetStage();
    if (stage == previous) return;
    if (previous == Stage::Harvest && stage == Stage::ReturnLog)
    {
        Spawn(ItemKind::Log, FVector2D(1120, 365)); Announce(TEXT("A fresh log! Carry it south to the saw."));
    }
    if (previous == Stage::Saw && stage == Stage::Supply)
    {
        for (int i = 0; i < 3; ++i) Spawn(ItemKind::Plank, FVector2D(1000 + i * 65, 625));
        Announce(TEXT("Three planks cut. Carry and drop each at the assembly flower."));
    }
    if (previous == Stage::Ropes && stage == Stage::Deliver)
    {
        Spawn(ItemKind::Package, FVector2D(550, 570)); Announce(TEXT("Beautiful teamwork. Take the package to Grandfather!"));
    }
}
void FPlanarWorld::Update(float dt, const FForestInput (&inputs)[2])
{
    if (!Started || Paused || Help || Session.GetStage() == Stage::Complete || !FMath::IsFinite(dt) || dt <= 0) return;
    dt = FMath::Min(dt, 0.1f);
    PlaySeconds += dt;
    NoticeSeconds = FMath::Max(0.f, NoticeSeconds - dt);
    for (int p = 0; p < 2; ++p) Move(p, inputs[p].Move, dt);
    for (int p = 0; p < 2; ++p)
    {
        const FVector2D pos = Players[p].Position;
        if (Session.GetStage() == Stage::Discover && FVector2D::Distance(pos, Grove) < 200)
        { Session.Discover(); Announce(TEXT("Two circles, two helpers. Join with F / K.")); }
        const auto* held = Held(p);
        if (held && held->Kind == ItemKind::Log && FVector2D::Distance(pos, Saw) < 180) Session.ReturnLog();
        if (inputs[p].Interact) Interact(p);
        if (inputs[p].Pick) PickOrDrop(p);
        const Stage before = Session.GetStage();
        Session.Work(p, inputs[p].Direction, inputs[p].ActionPressed);
        ProcessStageChange(before);
    }
    const Stage beforePull = Session.GetStage();
    Session.Pull(inputs[0].ActionHeld, inputs[1].ActionHeld);
    ProcessStageChange(beforePull);
    for (auto& item : Items)
    {
        if (item.Consumed || item.HeldBy < 0) continue;
        item.Position = Players[item.HeldBy].Position + FVector2D(0, -28);
        if (FVector2D::Distance(Players[item.HeldBy].Position, Elder) < 100 && Session.Deliver(item.Kind))
        {
            Players[item.HeldBy].HeldItem = -1;
            item.HeldBy = -1; item.Consumed = true;
            Announce(TEXT("Order delivered!"));
        }
    }
}
FString FPlanarWorld::Objective() const
{
    static const TCHAR* labels[] = {TEXT("Meet Grandfather and accept his order"), TEXT("Follow the path to the mushroom grove"),
        TEXT("Harvest together: join both circles, then alternate left / right"), TEXT("Carry the log back to the sawmill"),
        TEXT("Hold the log near the saw and interact to load it"), TEXT("Work together: turn the wheel and press the blade"),
        TEXT("Carry three planks to the flower and drop them inside"), TEXT("Join opposite ropes, then hold both action keys together"),
        TEXT("Carry the finished package back to Grandfather"), TEXT("A chair made together. Order complete!")};
    return labels[static_cast<int>(Session.GetStage())];
}
FString FPlanarWorld::Hint(int player) const
{
    const int slot = Session.GetStations().Find(player);
    const TCHAR* interact = player == 0 ? TEXT("F") : TEXT("K");
    if (slot >= 0)
    {
        if (!Session.GetStations().Full()) return FString::Printf(TEXT("Waiting for partner  |  %s leave"), interact);
        if (Session.GetStage() == Stage::Harvest) return player == 0 ? TEXT("Alternate A / D   |   F leave") : TEXT("Alternate LEFT / RIGHT   |   K leave");
        if (Session.GetStage() == Stage::Ropes || slot == 1) return player == 0 ? TEXT("SPACE action   |   F leave") : TEXT("RIGHT SHIFT action   |   K leave");
        return player == 0 ? TEXT("Turn: W D S A   |   F leave") : TEXT("Turn: UP RIGHT DOWN LEFT   |   K leave");
    }
    return player == 0 ? TEXT("WASD move   E carry/drop   F interact") : TEXT("ARROWS move   J carry/drop   K interact");
}
