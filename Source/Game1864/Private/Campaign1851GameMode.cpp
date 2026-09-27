#include "Campaign1851GameMode.h"

#include "Campaign1851Camera.h"
#include "Campaign1851PlayerController.h"

ACampaign1851GameMode::ACampaign1851GameMode()
{
	DefaultPawnClass = ACampaign1851Camera::StaticClass();
	PlayerControllerClass = ACampaign1851PlayerController::StaticClass();
}
