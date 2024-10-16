// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MyProject.h"
#include "GameFramework/Character.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "PlayerCharacter.generated.h"

UCLASS()
class MYPROJECT_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;

	// Components Config
	UPROPERTY(VisibleAnywhere, BlueprintreadWrite)
	class USpringArmComponent* SpringArmComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UCameraComponent* CameraComponent;

	// Input Config
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TSoftObjectPtr<UInputMappingContext> InputMapping;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	class UInputAction* MoveInputAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	class UInputAction* LookInputAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	class UInputAction* JumpInputAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	class UInputAction* SprintInputAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	class UInputAction* SlideInputAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	class UInputAction* DashInputAction;
	
	// Custom Movement Config
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement")
	class UCustomCharacterMovementComponent* CustomCharacterMovementComponent;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement")
	float LedgeGrabForwardReach;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement")
	float LedgeGrabOverheadReach;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement")
	float LedgeGrabVelocity;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement")
	float WallRunReach;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement")
	float WallJumpImpulseUp;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement")
	float WallJumpImpulseAway;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Sprinting")
	float MaxWalkSpeedSprinting;

	// Stored Constants
	float MaxWalkSpeedBase;

	// State Data
	bool IsSprinting;
	bool IsWallRunning;
	
	FVector ToWallRun;
	
	void Move(const FInputActionInstance& Instance);
	void Look(const FInputActionInstance& Instance);
	virtual void Jump() override;
	virtual void StopJumping() override;
	void SprintStart();
	void SprintEnd();
	void SlideStart();
	void SlideEnd();
	void Dash();

public:	
	// Sets default values for this character's properties
	APlayerCharacter(const FObjectInitializer& ObjectInitializer);
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
