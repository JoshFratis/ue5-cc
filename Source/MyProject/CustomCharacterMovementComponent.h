#pragma once
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomCharacterMovementComponent.generated.h"

class APlayerCharacter;

UENUM(BlueprintType)
enum ECustomMovementMove 
{
	CMOVE_None		UMETA(Hidden),
	CMOVE_Sliding	UMETA(DisplayName = "Sliding"),
};

UCLASS()
class MYPROJECT_API UCustomCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	// Base
	float GroundFrictionBase;

	// Slide
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement: Sliding")
	float SlideImpulse;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement: Sliding")
	float SlideSpeed;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement: Sliding")
	float GroundFrictionSliding;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement: Sliding")
	float BrakingDecelerationSliding;

	// Dash
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Dashing")
	float DashImpulse;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Dashing")
	float DashCooldownDuration;
	
	float DashStartTime;

private:
	void PhysSlide(float deltaTime, int32 Iteration);
	
public:
	bool CanDash() const;
	void PerformDash();
	
	bool CanSlide() const;
	void EnterSlide();
	void ExitSlide();

};
