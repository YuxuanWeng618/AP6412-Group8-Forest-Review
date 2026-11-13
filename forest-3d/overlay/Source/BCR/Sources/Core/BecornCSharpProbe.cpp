#include "BCR/Headers/Core/BecornCSharpProbe.h"

#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "HighResScreenshot.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

void ABecornCSharpProbe::RecordManagedStart(int32 Result, const FString& Runtime, const FString& CoreLibrary, const FString& ManagedAssembly)
{
    bManagedStarted = true;
    StartResult = Result;
    RuntimeVersion = Runtime;
    CoreLibraryPath = CoreLibrary;
    ManagedAssemblyPath = ManagedAssembly;
    UE_LOG(LogTemp, Display, TEXT("[BecornCSharpPhaseA] Managed BeginPlay: result=%d runtime=%s"), Result, *Runtime);
}

ABecornCSharpProbeGameMode::ABecornCSharpProbeGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = nullptr;
    // Hard reference makes the engine mesh available in a cook containing only our test map.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        // Keep this reference on the CDO using a visible component, without any plugin dependency.
        UStaticMeshComponent* Preview = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PhaseAMeshReference"));
        SetRootComponent(Preview);
        Preview->SetStaticMesh(Cube.Object);
        Preview->SetVisibility(false);
    }
}

void ABecornCSharpProbeGameMode::BeginPlay()
{
    Super::BeginPlay();
    UWorld* World = GetWorld();
    const FVector CameraLocation(700, -1000, 600);
    const FRotator CameraRotation = (FVector(0, 0, 125) - CameraLocation).Rotation();
    ACameraActor* Camera = World->SpawnActor<ACameraActor>(CameraLocation, CameraRotation);
    if (APlayerController* PC = World->GetFirstPlayerController()) { PC->SetViewTarget(Camera); }
    ADirectionalLight* Light = World->SpawnActor<ADirectionalLight>(FVector(0, 0, 500), FRotator(-45, -30, 0));
    (void)Light;
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto AddCube = [&](const FVector& Location, const FVector& Scale)
    {
        AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
        Actor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
        Actor->SetActorScale3D(Scale);
    };
    AddCube(FVector(0, 0, -60), FVector(8, 8, .2));
    AddCube(FVector(-140, 0, 0), FVector(1, 1, 1));
    AddCube(FVector(140, 0, 40), FVector(1, 1, 1.8));
    StatusText = World->SpawnActor<ATextRenderActor>(FVector(0, 0, 200), FRotator(0, -90, 0));
    StatusText->GetTextRender()->SetHorizontalAlignment(EHTA_Center);
    StatusText->GetTextRender()->SetWorldSize(30);
    StatusText->SetActorRotation((Camera->GetActorLocation() - StatusText->GetActorLocation()).Rotation());
    ShowStatus(TEXT("forest / UE 5.8.1 + C#\nPhase A: waiting for managed actor"), FColor::White);
}

void ABecornCSharpProbeGameMode::ShowStatus(const FString& Text, const FColor& Color)
{
    if (StatusText)
    {
        StatusText->GetTextRender()->SetText(FText::FromString(Text));
        StatusText->GetTextRender()->SetTextRenderColor(Color);
    }
}

void ABecornCSharpProbeGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Elapsed += DeltaSeconds;
    if (!bFinished && Elapsed > 1.0f) { RunProbe(); }
    if (bExitScheduled && Elapsed > ExitAt)
    {
        FPlatformMisc::RequestExit(false);
        bExitScheduled = false;
    }
}

void ABecornCSharpProbeGameMode::RunProbe()
{
    UClass* ManagedClass = FindObject<UClass>(nullptr, TEXT("/Script/UnrealSharp.ManagedPhaseAActor_C"));
    if (ManagedClass && !ManagedClass->IsChildOf(ABecornCSharpProbe::StaticClass())) { ManagedClass = nullptr; }
    if (!ManagedClass && Elapsed < 25.0f) { return; }
    bFinished = true;
    auto Check = [&](const TCHAR* Name, bool Passed, const FString& Actual)
    {
        TSharedPtr<FJsonObject> Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("name"), Name);
        Row->SetBoolField(TEXT("passed"), Passed);
        Row->SetStringField(TEXT("actual"), Actual);
        Checks.Add(MakeShared<FJsonValueObject>(Row));
        UE_LOG(LogTemp, Display, TEXT("[BecornCSharpPhaseA] %s %s: %s"), Passed ? TEXT("PASS") : TEXT("FAIL"), Name, *Actual);
    };
    Check(TEXT("managed_class_registered"), ManagedClass != nullptr, GetPathNameSafe(ManagedClass));
    if (ManagedClass)
    {
        ManagedClassPath = ManagedClass->GetPathName();
        ABecornCSharpProbe* Probe = GetWorld()->SpawnActor<ABecornCSharpProbe>(ManagedClass);
        Check(TEXT("managed_actor_spawn"), Probe != nullptr, GetNameSafe(Probe));
        if (Probe)
        {
            RuntimeVersion = Probe->RuntimeVersion;
            CoreLibraryPath = Probe->CoreLibraryPath;
            ManagedAssemblyPath = Probe->ManagedAssemblyPath;
            Check(TEXT("managed_beginplay"), Probe->bManagedStarted, FString::FromInt(Probe->StartResult));
            Check(TEXT("csharp_to_cpp_method"), Probe->StartResult == 42, FString::FromInt(Probe->StartResult));
            Check(TEXT("native_property_roundtrip"), Probe->NativeValue == 21, FString::FromInt(Probe->NativeValue));
            FIntProperty* Property = FindFProperty<FIntProperty>(ManagedClass, TEXT("ManagedValue"));
            const int32 ManagedInitial = Property ? Property->GetPropertyValue_InContainer(Probe) : -1;
            Check(TEXT("managed_property_from_cpp"), ManagedInitial == 7, FString::FromInt(ManagedInitial));
            if (Property) { Property->SetPropertyValue_InContainer(Probe, 9); }
            UFunction* Method = Probe->FindFunction(TEXT("ManagedMultiply"));
            struct FCall { int32 Value = 6; int32 ReturnValue = -1; } Call;
            if (Method) { Probe->ProcessEvent(Method, &Call); }
            Check(TEXT("cpp_to_csharp_return_after_property_write"), Method && Call.ReturnValue == 54, FString::FromInt(Call.ReturnValue));
            Probe->OnNativePulse.Broadcast(5);
            Check(TEXT("native_delegate_to_csharp"), Probe->EventCount == 1 && Probe->EventValue == 14,
                FString::Printf(TEXT("count=%d value=%d"), Probe->EventCount, Probe->EventValue));
            Probe->Destroy();
            Check(TEXT("managed_endplay_unsubscribes"), Probe->bManagedEnded && !Probe->OnNativePulse.IsBound(),
                FString::Printf(TEXT("ended=%d bound=%d"), Probe->bManagedEnded, Probe->OnNativePulse.IsBound()));
            ABecornCSharpProbe* Second = GetWorld()->SpawnActor<ABecornCSharpProbe>(ManagedClass);
            Check(TEXT("new_actor_starts_clean"), Second && Second->bManagedStarted && Second->EventCount == 0 && Second->NativeValue == 21,
                Second ? FString::Printf(TEXT("events=%d value=%d"), Second->EventCount, Second->NativeValue) : TEXT("null"));
            if (Second) { Second->Destroy(); }
        }
    }
    Finish();
}

void ABecornCSharpProbeGameMode::Finish()
{
    bool bPassed = Checks.Num() == 10;
    for (const TSharedPtr<FJsonValue>& Value : Checks) { bPassed &= Value->AsObject()->GetBoolField(TEXT("passed")); }
    TSharedPtr<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetBoolField(TEXT("passed"), bPassed);
    Report->SetStringField(TEXT("utc"), FDateTime::UtcNow().ToIso8601());
    Report->SetNumberField(TEXT("process_id"), FPlatformProcess::GetCurrentProcessId());
    Report->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString());
    Report->SetBoolField(TEXT("requires_cooked_data"), FPlatformProperties::RequiresCookedData());
    Report->SetStringField(TEXT("runtime"), RuntimeVersion);
    Report->SetStringField(TEXT("core_library"), CoreLibraryPath);
    Report->SetStringField(TEXT("managed_assembly"), ManagedAssemblyPath);
    Report->SetStringField(TEXT("managed_class"), ManagedClassPath);
    Report->SetArrayField(TEXT("checks"), Checks);
    FString Json;
    FJsonSerializer::Serialize(Report.ToSharedRef(), TJsonWriterFactory<>::Create(&Json));
    FString ReportPath;
    if (!FParse::Value(FCommandLine::Get(), TEXT("PhaseAReport="), ReportPath))
        ReportPath = FPaths::ProjectSavedDir() / TEXT("PhaseA/result.json");
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(ReportPath), true);
    FFileHelper::SaveStringToFile(Json, *ReportPath);
    ShowStatus(bPassed ? TEXT("forest / UE 5.8.1 + C#\nPHASE A: ALL 10 CHECKS PASSED\nBeginPlay | properties | methods | event | lifecycle")
                       : TEXT("forest / C#\nPHASE A FAILED - inspect JSON report"), bPassed ? FColor::Green : FColor::Red);
    if (FParse::Param(FCommandLine::Get(), TEXT("BecornCSharpSmoke")))
    {
        FString Screenshot;
        if (FParse::Value(FCommandLine::Get(), TEXT("PhaseAScreenshot="), Screenshot))
        {
            FTimerHandle Handle;
            GetWorldTimerManager().SetTimer(Handle, [Screenshot]() { FScreenshotRequest::RequestScreenshot(Screenshot, false, false); }, 1.f, false);
        }
        bExitScheduled = true;
        ExitAt = Elapsed + 4.0f;
    }
}
