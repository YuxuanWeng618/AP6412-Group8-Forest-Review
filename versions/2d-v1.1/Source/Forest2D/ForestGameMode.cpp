#include "ForestGameMode.h"
#include "ForestController.h"
#include "ForestHUD.h"
AForestGameMode::AForestGameMode()
{
    PlayerControllerClass = AForestController::StaticClass();
    HUDClass = AForestHUD::StaticClass();
    DefaultPawnClass = nullptr;
}
