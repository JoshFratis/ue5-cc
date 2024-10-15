// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
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

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void APlayerCharacter::Move(const FInputActionInstance& Instance)
{
	const FVector2D Value = Instance.GetValue().Get<FVector2D>();
	UE_LOG(LogTemp, Warning, TEXT("Move Input %f, %f"), Value.X, Value.Y);

	if (Controller != nullptr && Value != FVector2d(0.0f, 0.0f))
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector XDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(XDirection, Value.X);
		
		const FVector YDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(YDirection, Value.Y);

		UE_LOG(LogTemp, Warning, TEXT("Move Direction %f, %f, %f, %f"), XDirection.X, XDirection.Y, YDirection.X, YDirection.Y);
	}
	
}

void APlayerCharacter::Look(const FInputActionInstance& Instance)
{
	const FVector2D Value = Instance.GetValue().Get<FVector2D>();
	UE_LOG(LogTemp, Warning, TEXT("Look Input %f, %f"), Value.X, Value.Y);

	if (Controller != nullptr && Value != FVector2d(0.0f, 0.0f))
	{
		AddControllerYawInput(Value.X);
		AddControllerPitchInput(Value.Y);
	}
}

void APlayerCharacter::Jump()
{
	Super::Jump();
	UE_LOG(LogTemp, Warning, TEXT("Jump"));
}

void APlayerCharacter::StopJumping()
{
	Super::StopJumping();
	UE_LOG(LogTemp, Warning, TEXT("Stop Jumping"));
}

void APlayerCharacter::SprintStart()
{
	UE_LOG(LogTemp, Warning, TEXT("Sprint Start"));
	
}

void APlayerCharacter::SprintEnd()
{
	UE_LOG(LogTemp, Warning, TEXT("Sprint End"));
	
}

void APlayerCharacter::SlideStart()
{
	UE_LOG(LogTemp, Warning, TEXT("Slide Start"));
}

void APlayerCharacter::SlideEnd()
{
	UE_LOG(LogTemp, Warning, TEXT("Slide End"));
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Bind Actions
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	Input->BindAction(MoveInputAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
	Input->BindAction(LookInputAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
	Input->BindAction(JumpInputAction, ETriggerEvent::Started, this, &APlayerCharacter::Jump);
	Input->BindAction(JumpInputAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopJumping);
	Input->BindAction(SprintInputAction, ETriggerEvent::Started, this, &APlayerCharacter::SprintStart);
	Input->BindAction(SprintInputAction, ETriggerEvent::Completed, this, &APlayerCharacter::SprintEnd);
	Input->BindAction(SlideInputAction, ETriggerEvent::Started, this, &APlayerCharacter::SlideStart);
	Input->BindAction(SlideInputAction, ETriggerEvent::Completed, this, &APlayerCharacter::SlideEnd);

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

