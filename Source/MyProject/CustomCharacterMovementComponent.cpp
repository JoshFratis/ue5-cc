#include "CustomCharacterMovementComponent.h"

bool UCustomCharacterMovementComponent::CanDash()
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