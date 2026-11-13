#include "BCR/Headers/Core/BCRGameViewportClient.h"
#include "BCR/Headers/Core/BCRKeyboardBridge.h"
#include "BCR/Headers/Player/MainPlayer.h"
#include "BCR/Headers/System/MiniGame/MiniGameSystem.h"
#include "BCR/Headers/System/QTE/QTE_Subsystem.h"
#include "Components/BillboardComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "CanvasItem.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "TextureResource.h"
#include "UObject/UnrealType.h"

namespace
{
int32 LocalPlayerIndex(UWorld* World, const AActor* Actor)
{
    for (int32 Index = 0; Index < 2; ++Index)
        if (Actor && UGameplayStatics::GetPlayerPawn(World, Index) == Actor) return Index;
    return INDEX_NONE;
}

AMainPlayer* RopePlayer(AActor* Actor, const TCHAR* PropertyName)
{
    const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Actor->GetClass(), PropertyName);
    return Property ? Cast<AMainPlayer>(Property->GetObjectPropertyValue_InContainer(Actor)) : nullptr;
}

FString WorldLabel(UBCRKeyboardBridge& Bridge, UNiagaraComponent* Component, UWorld* World)
{
    AActor* Owner = Component->GetOwner();
    if (!Owner) return {};

    const FString Asset = Component->GetAsset()->GetName();
    const bool bIsRope = Owner->GetClass()->GetName().Contains(TEXT("Ropes"));
    int32 RopeIndex = INDEX_NONE;
    if (bIsRope && (Asset == TEXT("NS_UI_A") || Asset == TEXT("NS_UI_A_Pressed")))
        RopeIndex = LocalPlayerIndex(World, RopePlayer(Owner,
            Component->GetName() == TEXT("UI_A1") ? TEXT("PlayerOnRopeLeft") : TEXT("PlayerOnRopeRight")));

    int32 FirstIndex = INDEX_NONE, SecondIndex = INDEX_NONE;
    double FirstDistance = 0, SecondDistance = 0;
    const auto* Machine = Cast<AMiniGameSystem>(Owner);
    if (Machine && Machine->snapPlayerPoint1 && Machine->snapPlayerPoint2)
    {
        FirstIndex = LocalPlayerIndex(World, Machine->snapPointMap.FindRef(Machine->snapPlayerPoint1));
        SecondIndex = LocalPlayerIndex(World, Machine->snapPointMap.FindRef(Machine->snapPlayerPoint2));
        const FVector Point = Component->GetComponentLocation();
        FirstDistance = FVector::DistSquared(Point, Machine->snapPlayerPoint1->GetComponentLocation());
        SecondDistance = FVector::DistSquared(Point, Machine->snapPlayerPoint2->GetComponentLocation());
    }

    return Bridge.WorldKeyboardPrompt(Asset, bIsRope, RopeIndex, Machine != nullptr,
        FirstIndex, SecondIndex, FirstDistance, SecondDistance);
}

void DrawPanel(UCanvas* Canvas, UFont* Font, const FString& Text, float X, float Y, float Scale, FLinearColor Color,
    UBCRKeyboardBridge* StatusLayout = nullptr, float OverviewTop = 0.f)
{
    // Canvas uses the UFont's legacy size (12px), independent of the UMG point size.
    const float ViewportScale = Scale;
    Scale *= 1.75f;
    float Width = 0, Height = 0;
    Canvas->StrLen(Font, Text, Width, Height);
    Width = FMath::Min(Width * Scale + 20.f, Canvas->ClipX - 20.f);
    Height = Height * Scale + 12.f;
    if (StatusLayout) Y = StatusLayout->PositionKeyboardStatus(Y, Height, OverviewTop, ViewportScale);
    X = FMath::Clamp(X, 10.f, FMath::Max(10.f, Canvas->ClipX - Width - 10.f));
    Y = FMath::Clamp(Y, 10.f, FMath::Max(10.f, Canvas->ClipY - Height - 10.f));
    FCanvasTileItem Panel(FVector2D(X, Y), FVector2D(Width, Height), FLinearColor(.015f,.025f,.035f,.9f));
    Panel.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Panel);
    FCanvasTextItem Item(FVector2D(X + 10.f, Y + 6.f), FText::FromString(Text), Font, Color);
    Item.Scale = FVector2D(Scale, Scale);
    Item.EnableShadow(FLinearColor::Black);
    Canvas->DrawItem(Item);
}

float DrawControlsPanel(UCanvas* Canvas, UFont* Font, const FTexture* Paper, const TArray<FString>& Rows, float Scale)
{
    float TextWidth = 0.f;
    float LineHeight = 0.f;
    for (const FString& Row : Rows)
    {
        float Width, Height;
        Canvas->StrLen(Font, Row, Width, Height);
        TextWidth = FMath::Max(TextWidth, Width);
        LineHeight = FMath::Max(LineHeight, Height);
    }

    const float PaddingX = 27.f * Scale;
    const float PaddingY = 9.f * Scale;
    const float Margin = 10.f;
    const float AvailableWidth = Canvas->ClipX - 2.f * (Margin + PaddingX);
    if (TextWidth <= 0.f || LineHeight <= 0.f || AvailableWidth <= 0.f) return Canvas->ClipY;

    const float TextScale = FMath::Min(1.55f * Scale, AvailableWidth / TextWidth);
    const float RowHeight = LineHeight * TextScale + 3.f * Scale;
    const FVector2D Size(TextWidth * TextScale + PaddingX * 2.f, RowHeight * Rows.Num() + PaddingY * 2.f);
    const FVector2D Origin(Margin, FMath::Max(Margin, Canvas->ClipY - Size.Y - 34.f * Scale));
    const auto DrawPaperSlice = [&](FVector2D Position, float Width, float StartU, float EndU, FLinearColor Tint)
    {
        FCanvasTileItem Tile(Position, Paper, FVector2D(Width, Size.Y), FVector2D(StartU, 0.f), FVector2D(EndU, 1.f), Tint);
        Tile.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(Tile);
    };
    const auto DrawPaper = [&](FVector2D Position, FLinearColor Tint)
    {
        if (!Paper)
        {
            DrawPaperSlice(Position, Size.X, 0.f, 1.f, Tint);
            return;
        }

        // Stretch the center so the irregular ends keep their proportions.
        const float EdgeWidth = FMath::Min(Size.X * .25f, Size.Y * .33f);
        DrawPaperSlice(Position, EdgeWidth, 0.f, .25f, Tint);
        DrawPaperSlice(Position + FVector2D(EdgeWidth, 0.f), Size.X - 2.f * EdgeWidth, .25f, .75f, Tint);
        DrawPaperSlice(Position + FVector2D(Size.X - EdgeWidth, 0.f), EdgeWidth, .75f, 1.f, Tint);
    };
    DrawPaper(Origin + FVector2D(2.f, 3.f) * Scale, FLinearColor(0.f, 0.f, 0.f, .25f));
    DrawPaper(Origin, Paper ? FLinearColor::White : FLinearColor::FromSRGBColor(FColor(204, 173, 124)));

    FVector2D Position = Origin + FVector2D(PaddingX, PaddingY);
    for (const FString& Row : Rows)
    {
        FCanvasTextItem Text(Position, FText::FromString(Row), Font, FLinearColor::FromSRGBColor(FColor(42, 25, 15)));
        Text.Scale = FVector2D(TextScale, TextScale);
        Canvas->DrawItem(Text);
        Position.Y += RowHeight;
    }
    return Origin.Y;
}
}

void UBCRGameViewportClient::DrawKeyboardPrompts(UCanvas* Canvas)
{
    if (!Canvas || !GEngine || !GetWorld() || !UGameplayStatics::GetPlayerPawn(GetWorld(), 0) || IsKeyboardHelpOpen()) return;
    UBCRKeyboardBridge* Bridge = GetKeyboardBridge();
    if (!Bridge) return;
    if (!KeyboardFont) KeyboardFont = LoadObject<UFont>(nullptr, TEXT("/BecornBilingual/Fonts/DroidSansFallback_Font.DroidSansFallback_Font"));
    UFont* Font = KeyboardFont ? KeyboardFont.Get() : GEngine->GetSmallFont();
    const float Scale = FMath::Clamp(Canvas->ClipX / 1600.f, .65f, 1.f);
    if (!KeyboardPaper) KeyboardPaper = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Becorn/ART/Asset/UI/T_CreditsFondCompe4.T_CreditsFondCompe4"));
    const TArray<FString> Overview = Bridge->OverviewKeyboardPrompts(UGameplayStatics::IsGamePaused(GetWorld()));
    const float OverviewTop = DrawControlsPanel(Canvas, Font, KeyboardPaper ? KeyboardPaper->GetResource() : nullptr, Overview, Scale);
    if (UGameInstance* GI = GetWorld()->GetGameInstance())
    {
        if (UQTE_Subsystem* QTE = GI->GetSubsystem<UQTE_Subsystem>())
        {
            FString Status;
            for (int32 Index = 0; Index < 2; ++Index)
            {
                FSnapPointConfig Config; int32 Count = 0;
                if (!QTE->GetPlayerActionInfo(Cast<AMainPlayer>(UGameplayStatics::GetPlayerPawn(GetWorld(), Index)), Config, Count)) continue;
                if (!Status.IsEmpty()) Status += TEXT("\n");
                Status += Bridge->QteKeyboardPrompt(Index, KeyboardActionLabel(Index, Config.RequiredInput),
                    QTE->GetCurrentState() == EQTEState::WaitingForPlayers);
            }
            if (!Status.IsEmpty())
                DrawPanel(Canvas, Font, Status, 10.f, Canvas->ClipY - 178.f * Scale, Scale,
                    FLinearColor(1.f,.85f,.45f,1.f), Bridge, OverviewTop);
        }
    }
    TArray<FVector2D> Placed;
    for (const TWeakObjectPtr<UNiagaraComponent>& Weak : WorldPrompts)
    {
        UNiagaraComponent* Component = Weak.Get();
        if (!Component || !Component->IsActive() || !Component->IsVisible() || Component->bHiddenInGame || !Component->GetAsset()) continue;
        if (!Component->GetOwner() || Component->GetOwner()->IsHidden()) continue;
        const FVector Screen = Canvas->Project(Component->GetComponentLocation());
        const FVector2D Position = Bridge->PlaceKeyboardPrompt(Screen, Canvas->ClipX, Canvas->ClipY, Scale, Placed);
        if (!FMath::IsFinite(Position.X) || !FMath::IsFinite(Position.Y)) continue;
        Placed.Add(Position);
        DrawPanel(Canvas, Font, WorldLabel(*Bridge, Component, GetWorld()), Position.X, Position.Y, Scale * .8f, FLinearColor::White);
    }
}
