#include "BCR/Headers/System/UI_Quest/BCRQuestSmokeObserver.h"
#include "BCR/Headers/Core/BCRGameViewportClient.h"
#include "BCR/Headers/System/UI_Quest/UI_Subsystem.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/UObjectIterator.h"

namespace
{
    constexpr const TCHAR* QuestMap = TEXT("/Game/Becorn/Maps/Map_Blocking_V2");

    UTextBlock* FindHud(UWorld* World)
    {
        for (TObjectIterator<UUserWidget> It; It; ++It)
            if (It->GetWorld() == World && It->IsInViewport() && It->GetClass()->GetName() == TEXT("WBP_Quest_C"))
                return Cast<UTextBlock>(It->GetWidgetFromName(TEXT("QUest_Desc")));
        return nullptr;
    }
}

#if WITH_DEV_AUTOMATION_TESTS
void UBCRGameViewportClient::TickQuestSmoke()
{
    static const bool bRequested = FParse::Param(FCommandLine::Get(), TEXT("BCRQuestSmoke"));
    UWorld* ActiveWorld = GetWorld();
    if (!bRequested || !ActiveWorld || ActiveWorld->GetName() != TEXT("Map_Blocking_V2")) return;
    if (!QuestSmoke)
    {
        UClass* Class = FindObject<UClass>(nullptr, TEXT("/Script/UnrealSharp.ManagedQuestReplay_C"));
        if (!Class || !Class->IsChildOf(UBCRQuestSmokeObserver::StaticClass())) return;
        QuestSmoke = NewObject<UBCRQuestSmokeObserver>(this, Class);
    }
    QuestSmoke->RunQuestFrame(FPlatformTime::Seconds());
}
#endif

UWorld* UBCRQuestSmokeObserver::ActiveWorld() const
{
    return GetTypedOuter<UBCRGameViewportClient>()->GetWorld();
}

UQuest_Subsystem* UBCRQuestSmokeObserver::ReplayQuest() const
{
    const UWorld* World = ActiveWorld();
    UGameInstance* Instance = World ? World->GetGameInstance() : nullptr;
    return Instance ? Instance->GetSubsystem<UQuest_Subsystem>() : nullptr;
}

UObject* UBCRQuestSmokeObserver::ReplayQuestBridge() const
{
    UQuest_Subsystem* Quest = ReplayQuest();
    const auto* Field = FindFProperty<FObjectPropertyBase>(UQuest_Subsystem::StaticClass(), TEXT("QuestBridge"));
    return Quest && Field ? Field->GetObjectPropertyValue_InContainer(Quest) : nullptr;
}

bool UBCRQuestSmokeObserver::ReplayHasManagedQuest() const
{
    const UObject* Bridge = ReplayQuestBridge();
    return Bridge && Bridge->GetClass()->GetPathName() == TEXT("/Script/UnrealSharp.ManagedQuestProgress_C");
}

UQuestData* UBCRQuestSmokeObserver::ReplayQuestData() const
{
    return LoadObject<UQuestData>(nullptr, TEXT("/Game/Becorn/Data/Quest_Data/DA_Quest_Becorn.DA_Quest_Becorn"));
}

int32 UBCRQuestSmokeObserver::ReplayAssetCount(UQuestData* Data) const
{
    return Data->QuestLine.QuestLineData.Num();
}

FString UBCRQuestSmokeObserver::ReplayAssetLine(UQuestData* Data) const
{
    return Data->QuestLine.Tag;
}

FString UBCRQuestSmokeObserver::ReplayAssetDescription(UQuestData* Data, const FString& Tag) const
{
    const auto* Row = Data->QuestLine.QuestLineData.FindByPredicate([&Tag](const FQuestStruct& Item) { return Item.Tag == Tag; });
    return Row ? Row->Desc : FString();
}

bool UBCRQuestSmokeObserver::ReplayHasHud() const { return FindHud(ActiveWorld()) != nullptr; }

FString UBCRQuestSmokeObserver::ReplayHudText() const
{
    const UTextBlock* Hud = FindHud(ActiveWorld());
    return Hud ? Hud->GetText().ToString() : FString();
}

bool UBCRQuestSmokeObserver::ReplaySameWorld() const { return ActiveWorld() == BoundWorld.Get(); }

bool UBCRQuestSmokeObserver::ReplaySameInstance() const { return ActiveWorld()->GetGameInstance() == BoundInstance.Get(); }

void UBCRQuestSmokeObserver::ReplayListen()
{
    BoundWorld = ActiveWorld();
    BoundInstance = ActiveWorld()->GetGameInstance();
    UQuest_Subsystem* Quest = ReplayQuest();
    Quest->OnQuestCompleted.AddUniqueDynamic(this, &UBCRQuestSmokeObserver::OnStep);
    Quest->OnQuestLineCompleted.AddUniqueDynamic(this, &UBCRQuestSmokeObserver::OnLine);
}

bool UBCRQuestSmokeObserver::ReplayIsListening() const
{
    const UQuest_Subsystem* Quest = ReplayQuest();
    return Quest && (Quest->OnQuestCompleted.GetAllObjects().Contains(this)
        || Quest->OnQuestLineCompleted.GetAllObjects().Contains(this));
}

void UBCRQuestSmokeObserver::OnStep(FString Tag)
{
    if (UQuest_Subsystem* Quest = ReplayQuest())
        ObserveQuestEvent(false, Tag, Quest->GetCurrentQuestTag(), Quest->GetCurrentQuestProgress());
}

void UBCRQuestSmokeObserver::OnLine(FString Tag)
{
    if (UQuest_Subsystem* Quest = ReplayQuest())
        ObserveQuestEvent(true, Tag, Quest->GetCurrentQuestTag(), Quest->GetCurrentQuestProgress());
}

void UBCRQuestSmokeObserver::ReplayNotifyOutput() { ReplayQuest()->NotifyAssemblerOutput(); }

void UBCRQuestSmokeObserver::ReplayStart(bool bDown)
{
    auto* Client = GetTypedOuter<UBCRGameViewportClient>();
    Client->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::SpaceBar, bDown ? IE_Pressed : IE_Released,
        bDown ? 1.f : 0.f, -1, FInputDeviceId::CreateFromInternalId(0), false, Client->Viewport));
}

void UBCRQuestSmokeObserver::ReplayReload() { UGameplayStatics::OpenLevel(ActiveWorld(), FName(QuestMap)); }

FString UBCRQuestSmokeObserver::ReplayQuestReportPath() const
{
    return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("PhaseC/quest-runtime.json"));
}

void UBCRQuestSmokeObserver::ReplayQuestLog(const FString& Message)
{
    UE_LOG(LogTemp, Display, TEXT("BCR_QUEST_SMOKE %s"), *Message);
}

void UBCRQuestSmokeObserver::ReplayQuestExit(int32 ExitCode)
{
    FPlatformMisc::RequestExitWithStatus(false, ExitCode);
}
