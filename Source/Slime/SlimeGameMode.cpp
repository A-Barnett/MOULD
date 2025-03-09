#include "SlimeGameMode.h"
#include "UObject/ConstructorHelpers.h"
#include "SlimePlayer.h"

ASlimeGameMode::ASlimeGameMode()
{
    // Locate the blueprint for the player
    static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/SlimePlay")); // Update the path to your blueprint
    if (PlayerPawnBPClass.Class != nullptr)
    {
        DefaultPawnClass = PlayerPawnBPClass.Class;
    }
}
