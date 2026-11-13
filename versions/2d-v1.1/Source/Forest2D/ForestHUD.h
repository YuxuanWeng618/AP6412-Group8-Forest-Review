#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PlanarWorld.h"
#include "ForestHUD.generated.h"
UCLASS()
class AForestHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    virtual void NotifyHitBoxClick(FName BoxName) override;
private:
    float Zoom = 1;
    FVector2D Offset;
    void Box(float x, float y, float w, float h, FLinearColor color);
    void Disc(float x, float y, float rx, float ry, FLinearColor color);
    void Line(float x, float y, float x2, float y2, FLinearColor color, float width = 2);
    void Text(const FString& value, float x, float y, float size, FLinearColor color);
    void Leaf(float x, float y, float size, FLinearColor color);
    void Elf(FVector2D pos, FLinearColor color, int player);
    void DrawItem(const FForestItem& item);
    void DrawWorld(const FPlanarWorld& board);
    void DrawOverlay(const FPlanarWorld& board);
    void Button(FName action,const FString& label,float x,float y,float width,bool primary=false);
};
