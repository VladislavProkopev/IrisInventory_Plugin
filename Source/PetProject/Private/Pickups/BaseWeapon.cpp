


#include "Pickups/BaseWeapon.h"


ABaseWeapon::ABaseWeapon()
{
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bCanEverTick = false;
}

void ABaseWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	
}


