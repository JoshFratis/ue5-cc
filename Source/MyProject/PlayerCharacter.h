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
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement")
	class UCustomCharacterMovementComponent* CustomCharacterMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintreadWrite)
	class USpringArmComponent* SpringArmComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UCameraComponent* CameraComponent;

	UPROPERTY(EditAnywhere, Category="Input")
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Sprinting")
	float MaxWalkSpeedSprinting;
	
	float MaxWalkSpeedBase;
	
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
