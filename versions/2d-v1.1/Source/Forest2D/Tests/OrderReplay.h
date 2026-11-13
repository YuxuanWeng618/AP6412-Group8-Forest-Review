#pragma once
#include "PlanarWorld.h"

// Deterministic input-level replay: no teleportation, rule writes, inventory
// injection or shortened QTE targets. Used by tests and -ForestReplay.
class FOrderReplay
{
    enum class EAction { Walk, Interact, Pick, Work, Pull, Check };
    struct FCommand { EAction Action; int Player=0; FVector2D Destination; int Value=0; };
    TArray<FCommand> Commands;
    int Cursor=0;
    float Age=0;
    float Delay=0;
public:
    bool Done=false;
    FString Error;
    int Steps() const { return Cursor; }
    FOrderReplay()
    {
        auto walk=[&](int p,double x,double y) { Commands.Add({EAction::Walk,p,FVector2D(x,y),0}); };
        auto act=[&](EAction a,int p=0,int value=0) { Commands.Add({a,p,FVector2D::ZeroVector,value}); };
        walk(0,250,430); act(EAction::Interact); act(EAction::Check,0,1);
        walk(0,900,395); walk(0,1040,325); act(EAction::Interact);
        walk(1,900,395); walk(1,1230,395); walk(1,1200,325); act(EAction::Interact,1);
        for(int i=0;i<8;++i) act(EAction::Work,0,i);
        act(EAction::Check,0,3);
        walk(0,1120,365); act(EAction::Pick);
        walk(0,1230,365); walk(0,1230,570); walk(0,1170,570); act(EAction::Interact);
        act(EAction::Check,0,5);
        walk(0,970,570); act(EAction::Interact);
        walk(1,1230,365); walk(1,1230,570); walk(1,1150,570); act(EAction::Interact,1);
        for(int i=0;i<8;++i) act(EAction::Work,0,i);
        act(EAction::Check,0,6);
        for(int i=0;i<3;++i)
        {
            walk(0,850,625); walk(0,1000+i*65,625); act(EAction::Pick);
            walk(0,850,625); walk(0,550,600); walk(0,550,550); act(EAction::Pick);
        }
        act(EAction::Check,0,7);
        walk(0,440,510); act(EAction::Interact);
        walk(1,850,570); walk(1,660,510); act(EAction::Interact,1);
        act(EAction::Pull); act(EAction::Check,0,8);
        walk(0,550,570); act(EAction::Pick);
        walk(0,300,570); walk(0,250,430); act(EAction::Check,0,9);
    }
    void Tick(FPlanarWorld& world,float dt)
    {
        if(Done || !Error.IsEmpty()) return;
        world.Started=true;
        if(Cursor>=Commands.Num()) { Done=true; return; }
        Age+=dt; Delay+=dt;
        if(Age>20) { Error=FString::Printf(TEXT("Replay stuck at input step %d, order stage %d"),Cursor,static_cast<int>(world.Session.GetStage())); return; }
        const auto& command=Commands[Cursor];
        FForestInput inputs[2];
        bool advance=false;
        if(command.Action==EAction::Walk)
        {
            const FVector2D delta=command.Destination-world.Players[command.Player].Position;
            // Delivery ends gameplay at the trigger radius, before the actor
            // reaches the waypoint's center. That is success, not a stuck walk.
            advance=delta.Size()<4 || world.Session.GetStage()==Forest::Stage::Complete;
            if(!advance) inputs[command.Player].Move=delta.GetSafeNormal()*FMath::Min(1.0,delta.Size()/4.0);
        }
        else if(Delay>=0.14f)
        {
            advance=true;
            switch(command.Action)
            {
            case EAction::Interact: inputs[command.Player].Interact=true; break;
            case EAction::Pick: inputs[command.Player].Pick=true; break;
            case EAction::Work:
                inputs[0].Direction=world.Session.GetStage()==Forest::Stage::Harvest?(command.Value%2?1:3):command.Value%4;
                inputs[1].Direction=command.Value%2?1:3;
                inputs[1].ActionPressed=true; break;
            case EAction::Pull: inputs[0].ActionHeld=true; inputs[1].ActionHeld=true; break;
            case EAction::Check:
                if(static_cast<int>(world.Session.GetStage())!=command.Value)
                    Error=FString::Printf(TEXT("Expected order stage %d, got %d at replay step %d"),command.Value,static_cast<int>(world.Session.GetStage()),Cursor);
                break;
            default: break;
            }
        }
        world.Update(dt,inputs);
        if(advance) { ++Cursor; Age=0; Delay=0; }
    }
};
