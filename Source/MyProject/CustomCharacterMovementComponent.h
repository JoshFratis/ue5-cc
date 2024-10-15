#pragma once
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomCharacterMovementComponent.generated.h"

UCLASS()
class MYPROJECT_API UCustomCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

protected:

	// Dash
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Dashing")
	float DashImpulse;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Dashing")
	float DashCooldownDuration;
	
	float DashStartTime;

	
public:
	bool CanDash();
	void PerformDash();
};
