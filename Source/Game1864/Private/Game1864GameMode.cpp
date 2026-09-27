#include "Game1864GameMode.h"

#include "TacticalCameraPawn.h"
#include "TacticalPlayerController.h"

AGame1864GameMode::AGame1864GameMode()
{
	DefaultPawnClass = ATacticalCameraPawn::StaticClass();
	PlayerControllerClass = ATacticalPlayerController::StaticClass();
}
