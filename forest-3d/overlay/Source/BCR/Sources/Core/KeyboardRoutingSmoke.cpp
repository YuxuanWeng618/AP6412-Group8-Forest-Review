#include "BCR/Headers/Core/BCRKeyboardSmokeBridge.h"
#include "BCR/Headers/Core/BCRGameViewportClient.h"
#include "BCR/Headers/Core/BCRKeyboardBridge.h"
#include "BCR/Headers/Player/BCRPlayerRulesBridge.h"
#include "BCR/Headers/Player/MainPlayer.h"
#include "BCR/Headers/System/QTE/QTE_Subsystem.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "HAL/FileManager.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "UObject/UObjectIterator.h"

#if WITH_DEV_AUTOMATION_TESTS
void UBCRGameViewportClient::TickKeyboardSmoke(float DeltaTime)
{
    static const int32 Mode = FParse::Param(FCommandLine::Get(), TEXT("BCRPlayerRulesSmoke")) ? 3
        : FParse::Param(FCommandLine::Get(), TEXT("BCRViewportSmoke")) ? 2
        : FParse::Param(FCommandLine::Get(), TEXT("BCRKeyboardSmoke")) ? 1 : 0;
    if (!Mode || !GetWorld() || !GetWorld()->IsGameWorld()) return;

    if (!KeyboardSmoke)
    {
        UClass* Class = FindObject<UClass>(nullptr, TEXT("/Script/UnrealSharp.ManagedKeyboardReplay_C"));
        if (!Class || !Class->IsChildOf(UBCRKeyboardSmokeBridge::StaticClass())) return;
        KeyboardSmoke = NewObject<UBCRKeyboardSmokeBridge>(this, Class);
    }

    KeyboardSmoke->RunKeyboardFrame(DeltaTime, Mode);
}
#endif

UBCRGameViewportClient* UBCRKeyboardSmokeBridge::Client() const { return GetTypedOuter<UBCRGameViewportClient>(); }

UQTE_Subsystem* UBCRKeyboardSmokeBridge::Qte() const
{
    return Client()->GetWorld()->GetGameInstance()->GetSubsystem<UQTE_Subsystem>();
}

APlayerController* UBCRKeyboardSmokeBridge::TestController(int32 Player) const
{
    return UGameplayStatics::GetPlayerController(Client()->GetWorld(), Player);
}

APawn* UBCRKeyboardSmokeBridge::TestPawn(int32 Player) const
{
    APlayerController* Controller = TestController(Player);
    return Controller ? Controller->GetPawn() : nullptr;
}

UBCRPlayerRulesBridge* UBCRKeyboardSmokeBridge::TestPlayerRules() const
{
    UClass* Class = FindObject<UClass>(nullptr, TEXT("/Script/UnrealSharp.ManagedPlayerRules_C"));
    APawn* Pawn = TestPawn(0);
    return Pawn && Class && Class->IsChildOf(UBCRPlayerRulesBridge::StaticClass())
        ? NewObject<UBCRPlayerRulesBridge>(Pawn, Class) : nullptr;
}

UBCRKeyboardBridge* UBCRKeyboardSmokeBridge::TestKeyboardPolicy() const
{
    const auto* Property = FindFProperty<FObjectPropertyBase>(Client()->GetClass(), TEXT("KeyboardBridge"));
    return Property ? Cast<UBCRKeyboardBridge>(Property->GetObjectPropertyValue_InContainer(Client())) : nullptr;
}

bool UBCRKeyboardSmokeBridge::TestKey(const FString& Key, int32 Event)
{
    const EInputEvent Kind = Event == 0 ? IE_Pressed : Event == 1 ? IE_Released : IE_Repeat;
    return Client()->InputKey(FInputKeyEventArgs::CreateSimulated(FKey(FName(Key)), Kind, Event == 1 ? 0.f : 1.f,
        -1, FInputDeviceId::CreateFromInternalId(0), false, Client()->Viewport));
}

bool UBCRKeyboardSmokeBridge::TestDown(int32 Player, const FString& Key) const
{
    const APlayerController* Controller = TestController(Player);
    return Controller && Controller->IsInputKeyDown(FKey(FName(Key)));
}

FVector2D UBCRKeyboardSmokeBridge::TestStick(int32 Player) const
{
    float X = 0.f, Y = 0.f;
    if (const APlayerController* Controller = TestController(Player))
        Controller->GetInputAnalogStickState(EControllerAnalogStick::CAS_LeftStick, X, Y);
    return FVector2D(X, Y);
}

float UBCRKeyboardSmokeBridge::TestRawAxis(int32 Player, const FString& Key) const
{
    const APlayerController* Controller = TestController(Player);
    return Controller && Controller->PlayerInput ? Controller->PlayerInput->GetRawKeyValue(FKey(FName(Key))) : 0.f;
}

void UBCRKeyboardSmokeBridge::TestIgnoreInput(bool bIgnore) { Client()->SetIgnoreInput(bIgnore); }

void UBCRKeyboardSmokeBridge::TestGameInputMode(int32 Player)
{
    if (APlayerController* Controller = TestController(Player))
        Controller->SetInputMode(FInputModeGameOnly());
}

void UBCRKeyboardSmokeBridge::TestSetPaused(bool bPause) { UGameplayStatics::SetGamePaused(Client()->GetWorld(), bPause); }
bool UBCRKeyboardSmokeBridge::TestPaused() const { return UGameplayStatics::IsGamePaused(Client()->GetWorld()); }
bool UBCRKeyboardSmokeBridge::TestHelpOpen() const { return Client()->IsKeyboardHelpOpen(); }
void UBCRKeyboardSmokeBridge::TestToggleHelp() { Client()->ToggleKeyboardHelp(); }
void UBCRKeyboardSmokeBridge::TestLostFocus() { Client()->LostFocus(Client()->Viewport); }

void UBCRKeyboardSmokeBridge::TestPossess(APawn* Pawn)
{
    if (APlayerController* Controller = TestController(0))
    {
        if (Pawn) Controller->Possess(Pawn);
        else Controller->UnPossess();
    }
}

bool UBCRKeyboardSmokeBridge::TestStartRound(const FString& AssetName, int32 Repeat)
{
    const FString Path = FString::Printf(TEXT("/Game/Becorn/Data/QTE_Config/%s.%s"), *AssetName, *AssetName);
    const auto* Asset = LoadObject<UQTEConfigurationAsset>(nullptr, *Path);
    if (!Asset || !Qte()) return false;
    FQTEConfiguration Config = Asset->ToRuntimeConfig();
    for (FSnapPointConfig& Slot : Config.SnapPoints) Slot.RepeatCount = Repeat;
    Qte()->StartQTE(Config);
    return true;
}

void UBCRKeyboardSmokeBridge::TestStopRound() { if (Qte()) Qte()->StopQTE(); }

void UBCRKeyboardSmokeBridge::TestJoinRound(int32 Player, int32 Slot)
{
    if (Qte()) Qte()->OnPlayerEnterSnapPoint(Cast<AMainPlayer>(TestPawn(Player)), static_cast<ESnapPointType>(Slot));
}

int32 UBCRKeyboardSmokeBridge::TestRoundState() const
{
    return Qte() ? static_cast<int32>(Qte()->GetCurrentState()) : INDEX_NONE;
}

FString UBCRKeyboardSmokeBridge::TestRoundLabel(int32 Player) const
{
    FSnapPointConfig Config;
    int32 Count = 0;
    if (!Qte() || !Qte()->GetPlayerActionInfo(Cast<AMainPlayer>(TestPawn(Player)), Config, Count)) return {};
    return Client()->KeyboardActionLabel(Player, Config.RequiredInput);
}

UUserWidget* UBCRKeyboardSmokeBridge::FindWidget(const FString& ClassName) const
{
    for (TObjectIterator<UUserWidget> It; It; ++It)
        if (It->GetWorld() == Client()->GetWorld() && It->IsInViewport() && It->GetClass()->GetName() == ClassName)
            return *It;
    return nullptr;
}

int32 UBCRKeyboardSmokeBridge::TestVisibility(const FString& Widget) const
{
    const UUserWidget* Found = FindWidget(Widget);
    return Found ? static_cast<int32>(Found->GetVisibility()) : INDEX_NONE;
}

FString UBCRKeyboardSmokeBridge::TestWidgetText(const FString& Widget, const FString& Element) const
{
    const UUserWidget* Found = FindWidget(Widget);
    const UTextBlock* Text = Found ? Cast<UTextBlock>(Found->GetWidgetFromName(FName(Element))) : nullptr;
    return Text ? Text->GetText().ToString() : FString();
}

TArray<FString> UBCRKeyboardSmokeBridge::TestRopeKeys() const
{
    TArray<FString> Result;
    for (TActorIterator<AActor> It(Client()->GetWorld()); It; ++It)
    {
        if (It->GetClass()->GetName() != TEXT("BP_NewActivationRopes_C")) continue;
        const auto* Keys = FindFProperty<FArrayProperty>(It->GetClass(), TEXT("AcceptableKey"));
        const auto* Inner = Keys ? CastField<FStructProperty>(Keys->Inner) : nullptr;
        if (!Inner || Inner->Struct != TBaseStructure<FKey>::Get()) continue;
        FScriptArrayHelper Values(Keys, Keys->ContainerPtrToValuePtr<void>(*It));
        for (int32 Index = 0; Index < Values.Num(); ++Index)
            Result.Add(reinterpret_cast<const FKey*>(Values.GetRawPtr(Index))->GetFName().ToString());
    }
    return Result;
}

FString UBCRKeyboardSmokeBridge::TestReportPath(bool bViewport) const
{
    return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() /
        (bViewport ? TEXT("PhaseC2/viewport-runtime.json") : TEXT("PhaseB/live-routing.json")));
}

void UBCRKeyboardSmokeBridge::TestScreenshot(const FString& Name, bool bViewport)
{
    const FString Folder = FPaths::GetPath(TestReportPath(bViewport));
    IFileManager::Get().MakeDirectory(*Folder, true);
    FScreenshotRequest::RequestScreenshot(Folder / Name, true, false);
}

void UBCRKeyboardSmokeBridge::TestKeyboardLog(const FString& Message)
{
    UE_LOG(LogTemp, Display, TEXT("BCR_KEYBOARD_SMOKE %s"), *Message);
}

void UBCRKeyboardSmokeBridge::TestKeyboardExit(int32 Code) { FPlatformMisc::RequestExitWithStatus(false, Code); }
