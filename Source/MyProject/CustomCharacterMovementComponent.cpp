#include "CustomCharacterMovementComponent.h"

void UCustomCharacterMovementComponent::BeginPlay()
{
	GroundFrictionBase = GroundFriction;
}

void UCustomCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	switch (CustomMovementMode)
	{
	case CMOVE_Sliding:
		PhysSlide(deltaTime, Iterations);
		break;
	default:
		UE_LOG(LogTemp, Fatal, TEXT("Invalid Movement Mode"));
	}
}

void UCustomCharacterMovementComponent::PhysSlide(float deltaTime, int32 Iteration)
{
	if (deltaTime < MIN_TICK_TIME)
		return;

	const FVector SlopeNormal = CurrentFloor.HitResult.ImpactNormal;
	const FVector SlideDirection = FVector::VectorPlaneProject(Velocity, SlopeNormal).GetSafeNormal();
	
	// UE_LOG(LogTemp, Log, TEXT("Slope Normal %ls"), *SlopeNormal.ToCompactString());
	// UE_LOG(LogTemp, Log, TEXT("Slide Direction %ls"), *SlideDirection.ToCompactString());
	DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() - SlopeNormal * 100, FColor::Red);
	DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + SlideDirection * 100, FColor::Blue);

	// Apply Forces 
	Velocity += GetGravityZ() * FVector::UpVector * deltaTime;
	Velocity += SlideDirection * SlideSpeed * deltaTime;

	// Update Velocity 
	const FVector DeltaVelocity = Velocity * deltaTime;
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(DeltaVelocity, UpdatedComponent->GetComponentQuat(), true, Hit);

	// Handle Collisions Smoothly 
	if (Hit.IsValidBlockingHit())
	{
		SlideAlongSurface(DeltaVelocity, 1.f - Hit.Time, Hit.Normal, Hit, true);
	}
}

bool UCustomCharacterMovementComponent::CanDash() const
{
	return DashStartTime + DashCooldownDuration < GetWorld()->GetTimeSeconds();
}

void UCustomCharacterMovementComponent::PerformDash()
{
	DashStartTime = GetWorld()->GetTimeSeconds();

	const FVector DashDirection = (Acceleration.IsNearlyZero() ? UpdatedComponent->GetForwardVector() : Acceleration).GetSafeNormal2D();
	const FVector NewVelocity = DashImpulse * (DashDirection + FVector::UpVector * 0.1f);
	
	UE_LOG(LogTemp, Warning, TEXT("Velocity (%3f, %f, %3f) -> (%3f, %f, %3f)"), Velocity.X, Velocity.Y, Velocity.Z, NewVelocity.X, NewVelocity.Y, NewVelocity.Z);
	// UE_LOG(LogTemp, Warning, TEXT("Direction: %f, %f, %f"), DashDirection.X, DashDirection.Y, DashDirection.Z);
	// UE_LOG(LogTemp, Warning, TEXT("Dash Impulse: %f"), DashImpulse);
	
	Velocity = NewVelocity;
	// UE_LOG(LogTemp, Warning, TEXT("Velocity: %f, %f, %f"), NewVelocity.X, NewVelocity.Y, NewVelocity.Z);

	// Update rotation to face the direction of the dash
	const FQuat NewRotation = FRotationMatrix::MakeFromXZ(DashDirection, FVector::UpVector).ToQuat();
	FHitResult Hit;
	SafeMoveUpdatedComponent(FVector::ZeroVector, NewRotation, false, Hit);

	// Consider the character moving through the air
	SetMovementMode(MOVE_Falling);
	
	// UE_LOG(LogTemp, Warning, TEXT("Velocity: %f, %f, %f"), NewVelocity.X, NewVelocity.Y, NewVelocity.Z);
}

bool UCustomCharacterMovementComponent::CanSlide() const
{
	return IsMovingOnGround();
}

void UCustomCharacterMovementComponent::EnterSlide()
{
	BrakingFriction = BrakingDecelerationSliding;
	BrakingFriction = GroundFrictionSliding;
	
	// SetMovementMode(MOVE_Custom, CMOVE_Sliding);
}

void UCustomCharacterMovementComponent::ExitSlide()
{
	BrakingFriction = BrakingDecelerationWalking;
	GroundFriction = GroundFrictionBase;
	
	// SetMovementMode(MOVE_Walking);
}

