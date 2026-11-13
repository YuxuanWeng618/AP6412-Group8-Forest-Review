#include "BCR/Headers/Core/BCRGameViewportClient.h"
#include "BCR/Headers/Core/BCRWorkshopReplayBridge.h"
#include "BCR/Headers/Interfaces/Interactable.h"
#include "BCR/Headers/Interfaces/IPickable.h"
#include "BCR/Headers/Player/MainPlayer.h"
#include "BCR/Headers/System/DeliveryPoint.h"
#include "BCR/Headers/System/Event/TriggerZone.h"
#include "BCR/Headers/System/FurnitureAssembler.h"
#include "BCR/Headers/System/MiniGame/MiniGameSystem.h"
#include "BCR/Headers/System/QTE/QTE_Subsystem.h"
#include "BCR/Headers/System/UI_Quest/UI_Subsystem.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
void UBCRGameViewportClient::TickWorkshopSmoke()
{
    static const bool bRequested = FParse::Param(FCommandLine::Get(), TEXT("BCRWorkshopSmoke"));
    UWorld* ActiveWorld = GetWorld();
    if (!bRequested || !ActiveWorld || ActiveWorld->GetName() != TEXT("Map_Blocking_V2")) return;
    if (WorkshopReplayWorld.Get() != ActiveWorld)
    {
        WorkshopReplay = nullptr;
        WorkshopReplayWorld = ActiveWorld;
    }
    if (!WorkshopReplay)
    {
        UClass* Class = FindObject<UClass>(nullptr, TEXT("/Script/UnrealSharp.ManagedWorkshopReplay_C"));
        if (!Class || !Class->IsChildOf(UBCRWorkshopReplayBridge::StaticClass())) return;
        WorkshopReplay = NewObject<UBCRWorkshopReplayBridge>(this, Class);
    }
    WorkshopReplay->RunReplayFrame(FPlatformTime::Seconds());
}
#endif

UWorld* UBCRWorkshopReplayBridge::ReplayWorld() const
{
    return GetTypedOuter<UBCRGameViewportClient>()->GetWorld();
}

UQTE_Subsystem* UBCRWorkshopReplayBridge::ReplayQte() const
{
    return ReplayWorld()->GetGameInstance()->GetSubsystem<UQTE_Subsystem>();
}

AActor* UBCRWorkshopReplayBridge::ReplayPlayer(int32 Index) const
{
    return Cast<AMainPlayer>(UGameplayStatics::GetPlayerPawn(ReplayWorld(), Index));
}

AActor* UBCRWorkshopReplayBridge::ReplayHeld(AActor* Player) const
{
    const auto* Character = Cast<AMainPlayer>(Player);
    return Character && IsValid(Character->PickedUpObject) ? Character->PickedUpObject : nullptr;
}

AActor* UBCRWorkshopReplayBridge::ReplayFind(const FString& ClassName, const FString& QuestTag) const
{
    for (TActorIterator<AActor> It(ReplayWorld()); It; ++It)
    {
        if (It->GetClass()->GetName() != ClassName) continue;
        if (QuestTag.IsEmpty()) return *It;
        const auto* Property = FindFProperty<FStrProperty>(It->GetClass(), TEXT("QuestTag"));
        if (Property && Property->GetPropertyValue_InContainer(*It) == QuestTag) return *It;
    }
    return nullptr;
}

USceneComponent* UBCRWorkshopReplayBridge::ReplayPart(AActor* Actor, const FString& Name) const
{
    if (!IsValid(Actor)) return nullptr;
    const FName PartName(*Name);
    for (UActorComponent* Part : Actor->GetComponents())
        if (Part && Part->GetFName() == PartName) return Cast<USceneComponent>(Part);
    return nullptr;
}

FString UBCRWorkshopReplayBridge::ReplayClass(AActor* Actor) const
{
    return IsValid(Actor) ? Actor->GetClass()->GetName() : FString();
}

FVector UBCRWorkshopReplayBridge::ReplayPosition(UObject* Object) const
{
    if (const auto* Actor = Cast<AActor>(Object)) return Actor->GetActorLocation();
    if (const auto* Part = Cast<USceneComponent>(Object)) return Part->GetComponentLocation();
    return FVector::ZeroVector;
}

FVector UBCRWorkshopReplayBridge::ReplayTriggerPosition(AActor* Actor) const
{
    if (auto* Zone = Actor->FindComponentByClass<UTriggerZone>())
        return Zone->GetInnerZoneComponent()->GetComponentLocation();
    if (const auto* Shape = Actor->FindComponentByClass<UBoxComponent>()) return Shape->GetComponentLocation();
    return Actor->GetActorLocation();
}

FVector UBCRWorkshopReplayBridge::ReplayStationPosition(AActor* Actor, int32 Station) const
{
    const auto* Machine = CastChecked<AMiniGameSystem>(Actor);
    return (Station == 0 ? Machine->snapPlayerPoint1 : Machine->snapPlayerPoint2)->GetComponentLocation();
}

FVector UBCRWorkshopReplayBridge::ReplayMachineInput(AActor* Actor) const
{
    return CastChecked<AMiniGameSystem>(Actor)->inputBox->GetComponentLocation();
}

FVector UBCRWorkshopReplayBridge::ReplayDeliveryPosition(AActor* Actor) const
{
    return CastChecked<ADeliveryPoint>(Actor)->ColliderBox->GetComponentLocation();
}

void UBCRWorkshopReplayBridge::ReplayPlace(AActor* Player, FVector Position)
{
    auto* Character = CastChecked<AMainPlayer>(Player);
    Character->GetCharacterMovement()->StopMovementImmediately();
    Character->SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);
    Character->GetCapsuleComponent()->UpdateOverlaps();
}

void UBCRWorkshopReplayBridge::ReplayPlaceForGrab(AActor* Player, AActor* Item, double Side)
{
    FVector Center, Extent;
    Item->GetActorBounds(true, Center, Extent);
    const FVector Direction = Item->GetActorRightVector() * Side;
    ReplayPlace(Player, Center + Direction * 145 + FVector(0, 0, 42));
    Player->SetActorRotation((-Direction).Rotation());
}

void UBCRWorkshopReplayBridge::ReplayInteract(AActor* Target, AActor* Player, AActor* Item)
{
    auto* Character = CastChecked<AMainPlayer>(Player);
    if (Item) IInteractable::Execute_InteractWithObject(Target, Character, Item);
    else IInteractable::Execute_Interact(Target, Character);
}

void UBCRWorkshopReplayBridge::ReplayKey(const FString& Input, bool bDown)
{
    auto* Viewport = GetTypedOuter<UBCRGameViewportClient>();
    const FKey Key{FName(*Input)};
    Viewport->InputKey(FInputKeyEventArgs::CreateSimulated(Key, bDown ? IE_Pressed : IE_Released,
        bDown ? 1.f : 0.f, -1, FInputDeviceId::CreateFromInternalId(0), false, Viewport->Viewport));
}

AActor* UBCRWorkshopReplayBridge::ReplayTraceGrab(AActor* Player) const
{
    const FVector Center = Player->GetActorLocation() + Player->GetActorForwardVector() * 150;
    TArray<FHitResult> Hits;
    UKismetSystemLibrary::BoxTraceMultiForObjects(Player, Center, Center, FVector(100),
        Player->GetActorRotation(), {EObjectTypeQuery::ObjectTypeQuery4}, false, {},
        EDrawDebugTrace::None, Hits, true);
    return Hits.IsEmpty() ? nullptr : Hits[0].GetActor();
}

bool UBCRWorkshopReplayBridge::ReplayFlag(AActor* Actor, const FString& Name) const
{
    const auto* Flag = IsValid(Actor) ? FindFProperty<FBoolProperty>(Actor->GetClass(), FName(*Name)) : nullptr;
    return Flag && Flag->GetPropertyValue_InContainer(Actor);
}

double UBCRWorkshopReplayBridge::ReplayNumber(AActor* Actor, const FString& Name) const
{
    const auto* Property = IsValid(Actor) ? FindFProperty<FDoubleProperty>(Actor->GetClass(), FName(*Name)) : nullptr;
    return Property ? Property->GetPropertyValue_InContainer(Actor) : std::numeric_limits<double>::quiet_NaN();
}

void UBCRWorkshopReplayBridge::ReplaySetHeld(AActor* Player, AActor* Item)
{
    auto* Character = CastChecked<AMainPlayer>(Player);
    if (IsValid(Item)) IIPickable::Execute_PickedUp(Item, Character, Item);
    else if (AActor* Held = ReplayHeld(Player)) IIPickable::Execute_Drop(Held, Character, Held);
    Character->PickedUpObject = Item;
}

bool UBCRWorkshopReplayBridge::ReplayHasMachineItem(AActor* Actor)
{
    auto* Machine = Cast<AMiniGameSystem>(Actor);
    return Machine && Machine->checkItemPresent();
}

int32 UBCRWorkshopReplayBridge::ReplayRecipeRemaining(AActor* Actor) const
{
    auto* Assembler = Cast<AFurnitureAssembler>(Actor);
    return Assembler && Assembler->GetActualRecipiesInfo().Out ? Assembler->requieredMaterials : -1;
}

bool UBCRWorkshopReplayBridge::ReplayAlignLooseItem(AActor* Assembler, AActor* Item, FVector Position)
{
    if (!IsValid(Item) || Item->GetAttachParentActor() || Assembler->IsOverlappingActor(Item)) return false;
    auto* Mesh = Cast<UPrimitiveComponent>(ReplayPart(Item, TEXT("StaticMesh")));
    if (!Mesh) return false;
    Mesh->SetWorldLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);
    Mesh->UpdateOverlaps();
    return true;
}

FString UBCRWorkshopReplayBridge::ReplayQuestTag() const
{
    return ReplayWorld()->GetGameInstance()->GetSubsystem<UQuest_Subsystem>()->GetCurrentQuestTag();
}

int32 UBCRWorkshopReplayBridge::ReplayRoundState() const
{
    return static_cast<int32>(ReplayQte()->GetCurrentState());
}

void UBCRWorkshopReplayBridge::ReplayStartTimer(float Timeout)
{
    FQTEConfiguration Probe;
    Probe.TotalTime = Timeout;
    Probe.SnapPoints.AddDefaulted();
    ReplayQte()->StartQTE(Probe);
}

void UBCRWorkshopReplayBridge::ReplayPauseTimer(bool bPaused)
{
    ReplayQte()->SetQTEPaused(bPaused);
}

void UBCRWorkshopReplayBridge::ReplayStopTimer()
{
    ReplayQte()->StopQTE();
}

FString UBCRWorkshopReplayBridge::ReplayReportPath() const
{
    return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("WorkshopSmoke/workshop-runtime.json"));
}

void UBCRWorkshopReplayBridge::ReplayExit(int32 ExitCode)
{
    FPlatformMisc::RequestExitWithStatus(false, static_cast<uint8>(ExitCode));
}

void UBCRWorkshopReplayBridge::ReplayLog(const FString& Message)
{
    UE_LOG(LogTemp, Display, TEXT("BCR_WORKSHOP_SMOKE %s"), *Message);
}
