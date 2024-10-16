// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerCharacter.h"

#include "CustomCharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Math/UnitConversion.h"

APlayerCharacter::APlayerCharacter(const FObjectInitializer& ObjectInitializer)
 : Super(ObjectInitializer.SetDefaultSubobjectClass<UCustomCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	CustomCharacterMovementComponent = Cast<UCustomCharacterMovementComponent>(GetCharacterMovement());
	
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Instantiate Class Components
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));

	// Set position and rotation of Character Mesh Transform
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -90.0f), FQuat(FRotator(0.0f, -90.0f, 0.0f)));

	// Attach class components to the default character's Skeletal Mesh Component
	SpringArmComponent->SetupAttachment(GetMesh());
	CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);

	SpringArmComponent->bUsePawnControlRotation = true;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->bIgnoreBaseRotation = true;
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	MaxWalkSpeedBase = GetCharacterMovement()->MaxWalkSpeed;
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Bind Actions
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	Input->BindAction(MoveInputAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
	Input->BindAction(MoveInputAction, ETriggerEvent::Started, this, &APlayerCharacter::MoveStart);
	Input->BindAction(MoveInputAction, ETriggerEvent::Completed, this, &APlayerCharacter::MoveEnd);
	Input->BindAction(LookInputAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
	Input->BindAction(JumpInputAction, ETriggerEvent::Started, this, &APlayerCharacter::Jump);
	Input->BindAction(JumpInputAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopJumping);
	Input->BindAction(SprintInputAction, ETriggerEvent::Started, this, &APlayerCharacter::SprintStart);
	Input->BindAction(SprintInputAction, ETriggerEvent::Completed, this, &APlayerCharacter::SprintEnd);
	Input->BindAction(SlideInputAction, ETriggerEvent::Started, this, &APlayerCharacter::SlideStart);
	Input->BindAction(SlideInputAction, ETriggerEvent::Completed, this, &APlayerCharacter::SlideEnd);
	Input->BindAction(DashInputAction, ETriggerEvent::Completed, this, &APlayerCharacter::Dash);

	// Add Input Mapping Context
	bool success = false;
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (!InputMapping.IsNull())
			{
				InputSystem->AddMappingContext(InputMapping.LoadSynchronous(), 0);
				success = true;
			}
		}
	}
	
	if (!success) UE_LOG(LogTemp, Warning, TEXT("Successfully set up Player Input Component"));
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Ledge Grab
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector CameraFlatForward = CameraComponent->GetForwardVector().GetSafeNormal2D();
	const FVector HeadPosition = GetActorLocation() + (FVector::UpVector * HalfHeight);
	
	if (!CustomCharacterMovementComponent->IsMovingOnGround())
	{
		const FVector ForwardHeadPosition = HeadPosition + (CameraFlatForward * LedgeGrabForwardReach);
		const FVector GrabPosition = ForwardHeadPosition + (FVector::UpVector * LedgeGrabOverheadReach);
	
		FHitResult LedgeHit;
		const FVector LedgeTraceStart = GrabPosition + (FVector::UpVector * 10.0f); // Trace begins above grab position so we can check if there exists open space above (a ledge, not a wall). 
		const FVector LedgeTraceEnd = ForwardHeadPosition + (FVector::DownVector * (HalfHeight * 2.0 - CustomCharacterMovementComponent->MaxStepHeight)); // Trace extends down to height at which character can step up onto ledge. Velocity override persists until no collision exists.
		
		FHitResult CeilingHit;
		const FVector CeilingTraceStart = GetActorLocation() + FVector::UpVector * HalfHeight;
		const FVector CeilingTraceEnd = CeilingTraceStart + FVector::UpVector * LedgeGrabOverheadReach;
		
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		GetWorld()->LineTraceSingleByChannel(LedgeHit, LedgeTraceStart, LedgeTraceEnd, ECC_WorldStatic, QueryParams);
		GetWorld()->LineTraceSingleByChannel(CeilingHit, CeilingTraceStart, CeilingTraceEnd, ECC_WorldStatic, QueryParams);
		// DrawDebugLine(GetWorld(), HeadPosition, HeadPosition + (CameraFlatForward * 10000), FColor::Red);

		if (!CeilingHit.bBlockingHit && LedgeHit.bBlockingHit && IsValid(LedgeHit.GetActor()) && LedgeHit.ImpactPoint.Z < GrabPosition.Z)
		{
			CustomCharacterMovementComponent->Velocity.Z = LedgeGrabVelocity;
		} 
		
		// DrawDebugLine(GetWorld(), TraceStart, TraceEnd, success ? FColor::Blue : FColor::Red);
		// DrawDebugLine(GetWorld(), TraceStart + (FVector::UpVector * 10000), TraceStart, success ? FColor::Green : FColor::Purple);
	}

	// Wall Run
	CanWallJump = false;
	IsWallRunning = false;
	
	if (!CustomCharacterMovementComponent->IsMovingOnGround())
	{
		FHitResult LeftHit;
		FHitResult RightHit;
		FVector TraceStart = GetActorLocation();
		FVector LeftTraceEnd = TraceStart - (CameraComponent->GetRightVector().GetSafeNormal2D() * WallRunReach); // currently straight left / right, may need to be a 45
		FVector RightTraceEnd = TraceStart + (CameraComponent->GetRightVector().GetSafeNormal2D() * WallRunReach);
		
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		GetWorld()->LineTraceSingleByChannel(LeftHit, TraceStart, LeftTraceEnd, ECC_WorldStatic, QueryParams);
		GetWorld()->LineTraceSingleByChannel(RightHit, TraceStart, RightTraceEnd, ECC_WorldStatic, QueryParams);
		// DrawDebugLine(GetWorld(), TraceStart, LeftTraceEnd, FColor::Red);
		// DrawDebugLine(GetWorld(), TraceStart, RightTraceEnd + (CameraFlatForward * 10000), FColor::Red);

		if (LeftHit.bBlockingHit && IsValid(LeftHit.GetActor()))
		{
			CanWallJump = true;
			ToWallRun = (LeftHit.ImpactPoint - GetActorLocation()).GetSafeNormal2D();
		}
		else if (RightHit.bBlockingHit && IsValid(RightHit.GetActor()))
		{
			CanWallJump = true;
			ToWallRun = (RightHit.ImpactPoint - GetActorLocation()).GetSafeNormal2D();
		}

		if (CanWallJump && IsSprinting && IsMoving)
		{
			IsWallRunning = true;
			CustomCharacterMovementComponent->Velocity.Z = std::max(CustomCharacterMovementComponent->Velocity.Z, 0.0);
		}
	}
}

void APlayerCharacter::MoveStart()
{
	IsMoving = true;
}

void APlayerCharacter::MoveEnd()
{
	IsMoving = false;
}

void APlayerCharacter::Move(const FInputActionInstance& Instance)
{
	const FVector2D Value = Instance.GetValue().Get<FVector2D>();
	// UE_LOG(LogTemp, Warning, TEXT("Move Input %f, %f"), Value.X, Value.Y);

	if (Controller != nullptr && Value != FVector2d(0.0f, 0.0f))
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector XDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(XDirection, Value.X);
		
		const FVector YDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(YDirection, Value.Y);

		// UE_LOG(LogTemp, Warning, TEXT("Move Direction %f, %f, %f, %f"), XDirection.X, XDirection.Y, YDirection.X, YDirection.Y);
	}
}

void APlayerCharacter::Look(const FInputActionInstance& Instance)
{
	const FVector2D Value = Instance.GetValue().Get<FVector2D>();
	// UE_LOG(LogTemp, Warning, TEXT("Look Input %f, %f"), Value.X, Value.Y);

	if (Controller != nullptr && Value != FVector2d(0.0f, 0.0f))
	{
		AddControllerYawInput(Value.X);
		AddControllerPitchInput(Value.Y);
	}
}

void APlayerCharacter::Jump()
{
	if (CanWallJump)
	{
		UE_LOG(LogTemp, Warning, TEXT("Wall Jump"));
		const FVector WallJumpImpulse = (-ToWallRun * WallJumpImpulseAway) + (FVector::UpVector * WallJumpImpulseUp);
		CustomCharacterMovementComponent->AddImpulse(WallJumpImpulse, true);
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + WallJumpImpulse, FColor::Red, false, 10.0f);
		UE_LOG(LogTemp, Log, TEXT("Wall Jump Vector: %ls"), *WallJumpImpulse.ToCompactString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Jump"));
		Super::Jump();
	}
}

void APlayerCharacter::StopJumping()
{
	Super::StopJumping();
	UE_LOG(LogTemp, Warning, TEXT("Stop Jumping"));
}

void APlayerCharacter::SprintStart() 
{
	UE_LOG(LogTemp, Warning, TEXT("Sprint Start"));
	IsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeedSprinting;
}

void APlayerCharacter::SprintEnd() 
{
	UE_LOG(LogTemp, Warning, TEXT("Sprint End"));
	IsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeedBase;
}

void APlayerCharacter::Dash() 
{
	if (CustomCharacterMovementComponent->CanDash())
		CustomCharacterMovementComponent->PerformDash();
}

void APlayerCharacter::SlideStart()
{
	UE_LOG(LogTemp, Warning, TEXT("Slide Start"));
}

void APlayerCharacter::SlideEnd()
{
	UE_LOG(LogTemp, Warning, TEXT("Slide End"));
}

