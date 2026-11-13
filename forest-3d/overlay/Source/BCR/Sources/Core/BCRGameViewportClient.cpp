#include "BCR/Headers/Core/BCRGameViewportClient.h"
#include "BCR/Headers/Core/BCRKeyboardBridge.h"
#include "BCR/Headers/Core/BCRKeyboardSmokeBridge.h"
#include "BCR/Headers/Core/BCRWorkshopReplayBridge.h"
#include "BCR/Headers/System/UI_Quest/BCRQuestSmokeObserver.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Blueprint/UserWidget.h"
#include "UObject/UObjectIterator.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

UBCRKeyboardBridge* UBCRGameViewportClient::GetKeyboardBridge()
{
    UWorld* ActiveWorld = GetWorld();
    if (!ActiveWorld || !ActiveWorld->IsGameWorld()) return nullptr;
    if (KeyboardWorld.Get() != ActiveWorld)
    {
        KeyboardBridge = nullptr;
        KeyboardWorld = ActiveWorld;
        bLoggedMissingKeyboardBridge = false;
    }
    if (!KeyboardBridge)
    {
        UClass* Class = FindObject<UClass>(nullptr, TEXT("/Script/UnrealSharp.ManagedKeyboardInput_C"));
        if (Class && Class->IsChildOf(UBCRKeyboardBridge::StaticClass()))
        {
            KeyboardBridge = NewObject<UBCRKeyboardBridge>(this, Class);
            UE_LOG(LogTemp, Display, TEXT("BCR_KEYBOARD_POLICY %s world=%s"), *Class->GetPathName(), *ActiveWorld->GetName());
        }
        else if (!bLoggedMissingKeyboardBridge)
        {
            UE_LOG(LogTemp, Warning, TEXT("forest managed keyboard policy is unavailable in %s"), *ActiveWorld->GetName());
            bLoggedMissingKeyboardBridge = true;
        }
    }
    return KeyboardBridge;
}

void UBCRGameViewportClient::SendKey(int32 PlayerIndex, FKey Key, EInputEvent Event, float Amount)
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), PlayerIndex);
    if (!PC || !PC->PlayerInput) return;
    // Feed the chosen controller directly: routing both halves through the default keyboard owner would control P1.
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, Event, Amount));
#else
    if (Event == IE_Axis) PC->InputKey(FInputKeyParams(Key, static_cast<double>(Amount), 1.f / 60.f, 1, true));
    else PC->InputKey(FInputKeyParams(Key, Event, static_cast<double>(Amount), true));
#endif
}

bool UBCRGameViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
    if (UBCRKeyboardBridge* Bridge = GetKeyboardBridge())
    {
        UWorld* ActiveWorld = GetWorld();
        if (Bridge->RouteViewportKey(EventArgs.Key.GetFName().ToString(), EventArgs.Event == IE_Pressed,
            EventArgs.Event == IE_Released, IgnoreInput(), UGameplayStatics::IsGamePaused(ActiveWorld), IsKeyboardHelpOpen(),
            UGameplayStatics::GetPlayerPawn(ActiveWorld, 0) != nullptr, UGameplayStatics::GetPlayerPawn(ActiveWorld, 1) != nullptr))
            return true;
    }
    // Unhandled keys keep editor shortcuts such as Shift+F1 and Escape.
    return Super::InputKey(EventArgs);
}

void UBCRGameViewportClient::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    RefreshWorldPrompts(DeltaTime);
#if WITH_DEV_AUTOMATION_TESTS
    TickKeyboardSmoke(DeltaTime);
    TickQuestSmoke();
    TickWorkshopSmoke();
#endif
    if (UBCRKeyboardBridge* Bridge = GetKeyboardBridge())
        Bridge->TickViewportInput(IgnoreInput(), UGameplayStatics::IsGamePaused(GetWorld()), IsKeyboardHelpOpen());
}

void UBCRGameViewportClient::LostFocus(FViewport* InViewport)
{
    if (UBCRKeyboardBridge* Bridge = GetKeyboardBridge())
        Bridge->OnViewportFocusLost(IgnoreInput(), UGameplayStatics::IsGamePaused(GetWorld()),
            UGameplayStatics::GetPlayerPawn(GetWorld(), 0) != nullptr);
    Super::LostFocus(InViewport);
}

void UBCRGameViewportClient::PostRender(UCanvas* Canvas)
{
    Super::PostRender(Canvas);
    DrawKeyboardPrompts(Canvas);
}

FString UBCRGameViewportClient::KeyboardActionLabel(int32 PlayerIndex, FKey Key)
{
    if (UBCRKeyboardBridge* Bridge = GetKeyboardBridge())
    {
        FString Label = Bridge->GetActionLabel(PlayerIndex, Key.GetFName().ToString());
        if (!Label.IsEmpty()) return Label;
    }
    return Key.GetDisplayName().ToString();
}

FString UBCRGameViewportClient::KeyboardHorizontalLabel(int32 PlayerIndex)
{
    if (UBCRKeyboardBridge* Bridge = GetKeyboardBridge()) return Bridge->GetHorizontalLabel(PlayerIndex);
    return FString();
}

bool UBCRGameViewportClient::IsKeyboardHelpOpen() const
{
    return IsValid(KeyboardHelp) && KeyboardHelp->IsInViewport();
}

void UBCRGameViewportClient::ToggleKeyboardHelp()
{
    if (!GetWorld() || !GetWorld()->IsGameWorld()) return;
    if (UBCRKeyboardBridge* Bridge = GetKeyboardBridge())
        Bridge->RequestKeyboardHelpToggle(UGameplayStatics::IsGamePaused(GetWorld()), IsKeyboardHelpOpen());
}

// The C# policy decides when help opens and which pause state to restore; UE owns the widget.
bool UBCRGameViewportClient::SetKeyboardHelpVisible(bool bVisible)
{
    if (!GetWorld() || !GetWorld()->IsGameWorld()) return false;
    if (!bVisible)
    {
        if (!IsKeyboardHelpOpen()) return false;
        KeyboardHelp->RemoveFromParent();
        KeyboardHelp = nullptr;
        return true;
    }
    if (IsKeyboardHelpOpen()) return false;
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    UClass* Class = LoadClass<UUserWidget>(nullptr, TEXT("/Game/Becorn/Blueprints/Widgets/WBP_Input.WBP_Input_C"));
    if (!PC || !Class) return false;
    KeyboardHelp = CreateWidget<UUserWidget>(PC, Class);
    if (!KeyboardHelp) return false;
    KeyboardHelp->AddToViewport(900);
    return true;
}

void UBCRGameViewportClient::RefreshWorldPrompts(float DeltaTime)
{
    PromptRefreshTime -= DeltaTime;
    if (PromptRefreshTime > 0.f || !GetWorld() || !GetWorld()->IsGameWorld()) return;
    PromptRefreshTime = .5f;
    WorldPrompts.Reset();
    for (TObjectIterator<UNiagaraComponent> It; It; ++It)
    {
        if (It->GetWorld() != GetWorld() || !It->GetAsset()) continue;
        const FString Name = It->GetAsset()->GetName();
        if (Name == TEXT("NS_UI_X") || Name == TEXT("NS_UI_A") || Name == TEXT("NS_UI_A_Pressed") ||
            Name == TEXT("NS_UI_LT") || Name == TEXT("NS_UI_RT") ||
            Name == TEXT("NS_UI_LeftStick_Alternate") || Name == TEXT("NS_UI_LeftStick_Rotating"))
        {
            // Preserve Blueprint activation, visibility and gameplay callbacks. Only replace the renderer.
            It->SetRenderingEnabled(false);
            WorldPrompts.Add(*It);
        }
    }
}
