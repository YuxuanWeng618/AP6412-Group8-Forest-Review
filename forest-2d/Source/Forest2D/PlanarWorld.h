#pragma once
#include "CoreMinimal.h"
#include "Domain/WorkshopSession.h"

struct FForestInput
{
    FVector2D Move = FVector2D::ZeroVector;
    int Direction = -1;
    bool Interact = false;
    bool Pick = false;
    bool ActionPressed = false;
    bool ActionHeld = false;
};
struct FForestPlayer
{
    FVector2D Position;
    FVector2D Facing = FVector2D(0, 1);
    int HeldItem = -1;
};
struct FForestItem
{
    int Id;
    Forest::ItemKind Kind;
    FVector2D Position;
    int HeldBy = -1;
    bool Consumed = false;
};

// Spatial adapter: all positions and collision live in a 1440 x 900 plane.
// It owns world entities; the session owns order/QTE/recipe decisions.
class FPlanarWorld
{
public:
    Forest::WorkshopSession Session;
    FForestPlayer Players[2];
    TArray<FForestItem> Items;
    FString Notice;
    float NoticeSeconds = 0;
    bool Started = false;
    bool Paused = false;
    bool Help = false;
    float PlaySeconds = 0;
    static const FVector2D Elder;
    static const FVector2D Grove;
    static const FVector2D Saw;
    static const FVector2D Assembler;
    FPlanarWorld();
    void Reset();
    void Update(float dt, const FForestInput (&inputs)[2]);
    FVector2D StationPosition(int slot) const;
    const FForestItem* Held(int player) const;
    bool CanStand(FVector2D position) const;
    static TArray<FBox2D> Obstacles();
    FString Objective() const;
    FString Hint(int player) const;
private:
    int NextItem = 0;
    void Move(int player, FVector2D axis, float dt);
    void Interact(int player);
    void PickOrDrop(int player);
    void Spawn(Forest::ItemKind kind, FVector2D position);
    void Announce(const FString& message);
    void ProcessStageChange(Forest::Stage previous);
};
