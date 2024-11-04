#include "PlayerCharacter.h"

#include "CustomCharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

APlayerCharacter::APlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCustomCharacterMovementComponent>(
		ACharacter::CharacterMovementComponentName))
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

	MaxWalkSpeedBase = CustomCharacterMovementComponent->MaxWalkSpeed;
	GroundFrictionBase = CustomCharacterMovementComponent->GroundFriction;
	BrakingDecelerationBase = CustomCharacterMovementComponent->BrakingDecelerationWalking;
	MaxMovementInputSpeed = MaxWalkSpeedBase;
	
	CameraSpringArmHeightBase = SpringArmComponent->GetComponentLocation().Z - GetActorLocation().Z;
	CapsuleHalfHeightBase = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
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
	Input->BindAction(DilateTimeAction, ETriggerEvent::Triggered, this, &APlayerCharacter::DilateTime);

	// Add Input Mapping Context
	bool success = false;
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (!InputMapping.IsNull())
			{
				InputSystem->AddMappingContext(InputMapping.LoadSynchronous(), 0);
				success = true;
			}
		}
	}

	// if (!success) UE_LOG(LogTemp, Warning, TEXT("Successfully set up Player Input Component"));
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsSprinting)
		IsSliding = false;

	// Slide down slopes
	if (IsSliding)
	{
		FHitResult FloorHit;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		FVector Start = GetActorLocation();
		FVector End = GetActorLocation() + FVector::DownVector * (GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 1.5f);
		GetWorld()->LineTraceSingleByChannel(FloorHit, Start, End, ECC_WorldStatic, QueryParams);
		
		if (FloorHit.bBlockingHit && IsValid(FloorHit.GetActor()))
		{
			FVector SlopeDirection = FloorHit.ImpactNormal.GetSafeNormal2D();
			SlideVector += SlopeDirection * SlideSpeedSlopeModifier;
			
			// DrawDebugLine(GetWorld(), Start, Start + (SlopeDirection * 100), FColor::Blue, false, 1.0f);
			// DrawDebugLine(GetWorld(), Start, Start + SlideVector, FColor::Cyan, false, 1.0f);
		}
	}

	// Set Camera Height
	// FVector CameraLocation = SpringArmComponent->GetComponentLocation();
	// float CameraHeight = CameraLocation.Z;
	// float TargetCameraHeight = GetActorLocation().Z + (IsSliding ? CameraHeightSliding : CameraSpringArmHeightBase);
	// float NewCameraHeight = FMath::FInterpConstantTo(CameraHeight, TargetCameraHeight, GetWorld()->GetDeltaSeconds(),
	//                                                  CameraHeightChangeSpeed);
	// SpringArmComponent->SetWorldLocation(FVector(CameraLocation.X, CameraLocation.Y, NewCameraHeight));

	// Set Capsule Height
	GetCapsuleComponent()->SetCapsuleHalfHeight(FMath::FInterpConstantTo(GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),
	                                                                (IsSliding
		                                                                 ? CapsuleHalfHeightSliding
		                                                                 : CapsuleHalfHeightBase),
	                                                                GetWorld()->GetDeltaSeconds(),
	                                                                CameraHeightChangeSpeed));
	
	// Apply Slide Velocity
	const float SlideVectorMagnitude = SlideVector.Length() / MaxWalkSpeedSliding;
	const float SlideVectorScaleValue = SlideVectorMagnitude * SlideMovementInputScale;
	AddMovementInput(SlideVector.GetSafeNormal(), SlideVectorScaleValue); 

	// Apply Slide Deceleration
	SlideVector = SlideVector.GetSafeNormal() * FMath::Max(0, SlideVector.Length() - (IsSliding ? SlideDeceleration : SlideExitDeceleration));

	// Lerp Between Slidy + Standard Physics
	CustomCharacterMovementComponent->BrakingDecelerationWalking	= FMath::Lerp(BrakingDecelerationBase,BrakingDecelerationSliding,SlideVectorMagnitude);
	CustomCharacterMovementComponent->GroundFriction				= FMath::Lerp(GroundFrictionBase,		GroundFrictionSliding,	SlideVectorMagnitude);
	CustomCharacterMovementComponent->MaxWalkSpeed					= FMath::Lerp(MaxMovementInputSpeed,	MaxWalkSpeedSliding,		SlideVectorMagnitude);

	// Apply Movement Input
	if (Controller != nullptr && MoveInput != FVector2d(0.0f, 0.0f))
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector XDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector YDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		
		float MoveScaleX = 1.0f;
		float MoveScaleY = 1.0f;
		
		// Strafe while sliding. Input is scaled by slide vector if perpendicular, zeroed if parallel. 
		if (IsSliding || CustomCharacterMovementComponent->Velocity.Length() > MaxMovementInputSpeed)
		{
			const float PercentXParallelToSlide = FMath::Abs( 
			 FMath::Acos(FVector::DotProduct(XDirection * MoveInput.X, SlideVector.GetSafeNormal2D())) // angle between slide vector and movement input vector
			 - (PI / 2)) / ( PI / 2); // transform into % parallel 
			const float PercentYParallelToSlide = FMath::Abs( 
			 FMath::Acos(FVector::DotProduct(YDirection * MoveInput.Y, SlideVector.GetSafeNormal2D())) 
			 - (PI / 2)) / ( PI / 2); 
			
			const float SlideVectorScaleValueClamped = FMath::Min(SlideVector.Length(), MaxMovementInputSpeed) / MaxWalkSpeedSliding * SlideMovementInputScale;
			MoveScaleX = FMath::Lerp(SlideVectorScaleValueClamped, 0, PercentXParallelToSlide);
			MoveScaleY = FMath::Lerp(SlideVectorScaleValueClamped, 0, PercentYParallelToSlide);
		}

		// Apply Movement Input
		AddMovementInput(XDirection, MoveInput.X * MoveScaleX);
		AddMovementInput(YDirection, MoveInput.Y * MoveScaleY);
	}

	// Ledge Grab
	// TODO: stick player move directions relative to grabbed ledge normal so you can't slide off ledge by holding W
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector CameraFlatForward = CameraComponent->GetForwardVector().GetSafeNormal2D();
	const FVector HeadPosition = GetActorLocation() + (FVector::UpVector * HalfHeight);

	if (!CustomCharacterMovementComponent->IsMovingOnGround())
	{
		const FVector ForwardHeadPosition = HeadPosition + (CameraFlatForward * LedgeGrabForwardReach);
		const FVector GrabPosition = ForwardHeadPosition + (FVector::UpVector * LedgeGrabOverheadReach);

		FHitResult LedgeHit;
		const FVector LedgeTraceStart = GrabPosition + (FVector::UpVector * 10.0f);
		// Trace begins above grab position so we can check if there exists open space above (a ledge, not a wall). 
		const FVector LedgeTraceEnd = ForwardHeadPosition + (FVector::DownVector * (HalfHeight * 2.0 -
			CustomCharacterMovementComponent->MaxStepHeight));
		// Trace extends down to height at which character can step up onto ledge. Velocity override persists until no collision exists.

		FHitResult CeilingHit;
		const FVector CeilingTraceStart = GetActorLocation() + FVector::UpVector * HalfHeight;
		const FVector CeilingTraceEnd = CeilingTraceStart + FVector::UpVector * LedgeGrabOverheadReach;

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		GetWorld()->LineTraceSingleByChannel(LedgeHit, LedgeTraceStart, LedgeTraceEnd, ECC_WorldStatic,
		                                     QueryParams);
		GetWorld()->LineTraceSingleByChannel(CeilingHit, CeilingTraceStart, CeilingTraceEnd, ECC_WorldStatic,
		                                     QueryParams);
		// DrawDebugLine(GetWorld(), HeadPosition, HeadPosition + (CameraFlatForward * 10000), FColor::Red);

		if (!CeilingHit.bBlockingHit && LedgeHit.bBlockingHit && IsValid(LedgeHit.GetActor()) && LedgeHit.
			ImpactPoint.Z < GrabPosition.Z)
		{
			CustomCharacterMovementComponent->Velocity.Z = LedgeGrabVelocity;
		}

		// DrawDebugLine(GetWorld(), TraceStart, TraceEnd, success ? FColor::Blue : FColor::Red);
		// DrawDebugLine(GetWorld(), TraceStart + (FVector::UpVector * 10000), TraceStart, success ? FColor::Green : FColor::Purple);

		// Wall Run
		CanWallJump = false;
		IsWallRunning = false;

		if (!CustomCharacterMovementComponent->IsMovingOnGround())
		{
			FHitResult LeftHit;
			FHitResult RightHit;
			FVector TraceStart = GetActorLocation();
			FVector LeftTraceEnd = TraceStart - (CameraComponent->GetRightVector().GetSafeNormal2D() *
				WallRunReach); // currently straight left / right, may need to be a 45
			FVector RightTraceEnd = TraceStart + (CameraComponent->GetRightVector().GetSafeNormal2D() *
				WallRunReach);

			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);

			GetWorld()->LineTraceSingleByChannel(LeftHit, TraceStart, LeftTraceEnd, ECC_WorldStatic, Params);
			GetWorld()->LineTraceSingleByChannel(RightHit, TraceStart, RightTraceEnd, ECC_WorldStatic, Params);
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
				CustomCharacterMovementComponent->Velocity.Z = std::max(
					CustomCharacterMovementComponent->Velocity.Z, 0.0);
			}
		}
	}

	// UE_LOG(LogTemp, Log, TEXT("Slide %f\tVelocity %f"), SlideVector.Length(), CustomCharacterMovementComponent->Velocity.Length());
}

void APlayerCharacter::MoveStart()
{
	IsMoving = true;
}

void APlayerCharacter::MoveEnd()
{
	IsMoving = false;
	MoveInput = FVector2D::ZeroVector;
	// UE_LOG(LogTemp, Warning, TEXT("Move Input %f, %f"), MoveInput.X, MoveInput.Y);
}

void APlayerCharacter::Move(const FInputActionInstance& Instance)
{
	MoveInput = Instance.GetValue().Get<FVector2D>();
	// UE_LOG(LogTemp, Warning, TEXT("Move Input %f, %f"), MoveInput.X, MoveInput.Y);
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
		// UE_LOG(LogTemp, Warning, TEXT("Wall Jump"));
		const FVector WallJumpImpulse = (-ToWallRun * WallJumpImpulseAway) + (FVector::UpVector * WallJumpImpulseUp);
		CustomCharacterMovementComponent->AddImpulse(WallJumpImpulse, true);
		// DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + WallJumpImpulse, FColor::Red, false, 10.0f);
		// UE_LOG(LogTemp, Log, TEXT("Wall Jump Vector: %ls"), *WallJumpImpulse.ToCompactString());
	}
	else
	{
		// UE_LOG(LogTemp, Warning, TEXT("Jump"));
		Super::Jump();
	}
}

void APlayerCharacter::StopJumping()
{
	Super::StopJumping();
	// UE_LOG(LogTemp, Warning, TEXT("Stop Jumping"));
}

void APlayerCharacter::SprintStart()
{
	// UE_LOG(LogTemp, Warning, TEXT("Sprint Start"));
	IsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeedSprinting;
	MaxMovementInputSpeed = MaxWalkSpeedSprinting;
}

void APlayerCharacter::SprintEnd()
{
	// UE_LOG(LogTemp, Warning, TEXT("Sprint End"));
	IsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeedBase;
	MaxMovementInputSpeed = MaxWalkSpeedBase;
}

void APlayerCharacter::Dash()
{
	if (CustomCharacterMovementComponent->CanDash())
		CustomCharacterMovementComponent->PerformDash();
}

void APlayerCharacter::DilateTime(const FInputActionInstance& Instance)
{
	const float Value = Instance.GetValue().Get<float>();
	// UE_LOG(LogTemp, Warning, TEXT("Got %f from scroll wheel"), Value);

	if (Controller != nullptr && Value != 0.0f)
	{
		TimeDilation = FMath::Clamp(TimeDilation + (Value * TimeDilationScale), TimeDilationMin, 1.0f);
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), TimeDilation);
		// UE_LOG(LogTemp, Warning, TEXT("Changing game speed by %f. Time dilation %f"), Value * TimeDilationScale, TimeDilation);
	}
}

void APlayerCharacter::SlideStart()
{
	// if (!CustomCharacterMovementComponent->IsMovingOnGround())
	// 	return;

	// Maintain velocity with small impulse
	if (CustomCharacterMovementComponent->Velocity.Length() < MaxWalkSpeedBase + SlideImpulse)
	{
		SlideVector = CustomCharacterMovementComponent->Velocity.GetClampedToMaxSize(MaxWalkSpeedBase) + (
			SlideImpulse * CustomCharacterMovementComponent->GetCurrentAcceleration().GetSafeNormal2D());
	}
	else
	{
		SlideVector = CustomCharacterMovementComponent->Velocity;
	}

	// Set slidy physics 
	CustomCharacterMovementComponent->BrakingDecelerationWalking = BrakingDecelerationSliding;
	CustomCharacterMovementComponent->BrakingFriction = GroundFrictionSliding;
	GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeedSliding;

	IsSliding = true;
}

void APlayerCharacter::SlideEnd()
{
	IsSliding = false;
}
