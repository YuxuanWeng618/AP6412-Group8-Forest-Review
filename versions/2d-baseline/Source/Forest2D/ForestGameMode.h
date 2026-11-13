#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PlanarWorld.h"
#include "ForestGameMode.generated.h"

UCLASS()
class AForestGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AForestGameMode();
    FPlanarWorld Board;
};
