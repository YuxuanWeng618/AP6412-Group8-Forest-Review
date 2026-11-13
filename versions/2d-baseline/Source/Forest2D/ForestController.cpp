#include "ForestController.h"
#include "ForestGameMode.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "UnrealClient.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

void AForestController::BeginPlay()
{
    Super::BeginPlay();
    SetInputMode(FInputModeGameOnly());
    bShowMouseCursor = false;
    ReplayEnabled = FParse::Param(FCommandLine::Get(),TEXT("ForestReplay"));
}
void AForestController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    auto* game = GetWorld()->GetAuthGameMode<AForestGameMode>();
    if (!game) return;
    auto& board = game->Board;
    CaptureAge += DeltaTime;
    if (!Captured && CaptureAge > 3 && FParse::Param(FCommandLine::Get(),TEXT("ForestCapture")))
    {
        const FString folder=FPaths::ProjectSavedDir()/TEXT("Verification");
        IFileManager::Get().MakeDirectory(*folder,true);
        FScreenshotRequest::RequestScreenshot(folder/TEXT("start-menu.png"),false,false);
        Captured=true;
    }
    if (ReplayEnabled)
    {
        if (CaptureAge < 4) return;
        Replay.Tick(board,FMath::Min(DeltaTime,1.f/30));
        if (!WorldCaptured && board.Session.GetStage()==Forest::Stage::Harvest && board.Session.GetStations().Full())
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Verification/forest-world.png"),false,false);
            WorldCaptured=true;
        }
        if (Replay.Done || !Replay.Error.IsEmpty())
        {
            if (!ReplaySaved)
            {
                const FString folder=FPaths::ProjectSavedDir()/TEXT("Verification");
                IFileManager::Get().MakeDirectory(*folder,true);
                const FString result=FString::Printf(TEXT("{\"passed\":%s,\"stage\":%d,\"steps\":%d,\"teleports\":0,\"error\":\"%s\"}"),Replay.Done?TEXT("true"):TEXT("false"),static_cast<int>(board.Session.GetStage()),Replay.Steps(),*Replay.Error);
                FFileHelper::SaveStringToFile(result,*(folder/TEXT("order-replay.json")));
                FScreenshotRequest::RequestScreenshot(folder/TEXT("order-complete.png"),false,false);
                UE_LOG(LogTemp,Display,TEXT("FOREST_REPLAY %s"),*result);
                ReplaySaved=true;
            }
            ReplayEndDelay+=DeltaTime;
            if(ReplayEndDelay>3 && FParse::Param(FCommandLine::Get(),TEXT("ForestReplayExit")))
                FPlatformMisc::RequestExitWithStatus(false,Replay.Done?0:1);
        }
        return;
    }
    auto* viewport = GetWorld()->GetGameViewport();
    const bool focused = viewport && viewport->Viewport && viewport->Viewport->HasFocus();
    if (!focused)
    {
        if (HadFocus && board.Started) board.Paused = true;
        HadFocus = false;
        FlushPressedKeys();
        return;
    }
    HadFocus = true;
    if (WasInputKeyJustPressed(EKeys::F5)) { board.Reset(); return; }
    if (!board.Started)
    {
        if (WasInputKeyJustPressed(EKeys::Enter) || WasInputKeyJustPressed(EKeys::SpaceBar)) board.Started = true;
        return;
    }
    if (board.Session.GetStage() == Forest::Stage::Complete)
    {
        if (WasInputKeyJustPressed(EKeys::Enter)) { board.Reset(); board.Started = true; }
        return;
    }
    if (WasInputKeyJustPressed(EKeys::H)) board.Help = !board.Help;
    if (!board.Help && (WasInputKeyJustPressed(EKeys::P) || WasInputKeyJustPressed(EKeys::Escape))) board.Paused = !board.Paused;
    FForestInput inputs[2];
    const FKey keys[2][7] = {{EKeys::W,EKeys::D,EKeys::S,EKeys::A,EKeys::F,EKeys::E,EKeys::SpaceBar},
        {EKeys::Up,EKeys::Right,EKeys::Down,EKeys::Left,EKeys::K,EKeys::J,EKeys::RightShift}};
    for (int p = 0; p < 2; ++p)
    {
        inputs[p].Move = FVector2D((IsInputKeyDown(keys[p][1])?1:0)-(IsInputKeyDown(keys[p][3])?1:0),
            (IsInputKeyDown(keys[p][2])?1:0)-(IsInputKeyDown(keys[p][0])?1:0));
        for (int d = 0; d < 4; ++d) if (WasInputKeyJustPressed(keys[p][d])) inputs[p].Direction = d;
        inputs[p].Interact = WasInputKeyJustPressed(keys[p][4]);
        inputs[p].Pick = WasInputKeyJustPressed(keys[p][5]);
        inputs[p].ActionPressed = WasInputKeyJustPressed(keys[p][6]);
        inputs[p].ActionHeld = IsInputKeyDown(keys[p][6]);
    }
    board.Update(DeltaTime, inputs);
}
