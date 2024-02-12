// MIT

#include "Components/Character/ALSXTIdleAnimationComponent.h"
#include "Net/UnrealNetwork.h"
#include "Interfaces/ALSXTCharacterInterface.h"
#include "Interfaces/ALSXTIdleAnimationComponentInterface.h"

// Sets default values for this component's properties
UALSXTIdleAnimationComponent::UALSXTIdleAnimationComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UALSXTIdleAnimationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, CurrentIdleMontage, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, IdleCounterCurrent, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, IdleCounterTarget, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, CurrentTimeBetweenAnimations, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, TargetTimeBetweenAnimations, Parameters)
}


// Called when the game starts
void UALSXTIdleAnimationComponent::BeginPlay()
{
	Super::BeginPlay();

	// if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_SimulatedProxy && GetOwner()->GetRemoteRole() == ROLE_Authority)
	if (GetOwner())
	{
		StartIdleCounterTimer();
	}
}


// Called every frame
void UALSXTIdleAnimationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	StatusState = IALSXTCharacterInterface::Execute_GetStatusState(GetOwner());
	if (IsValid(CurrentIdleMontage) && !IsPlayerInputIdle())
	{
		StopIdle();
	}
}

bool UALSXTIdleAnimationComponent::IsPlayerIdle()
{
	return bIsIdle;
}

void UALSXTIdleAnimationComponent::SetPlayerIdle(bool NewIdle)
{
	bIsIdle = NewIdle;
}

bool UALSXTIdleAnimationComponent::IsPlayerInputIdle()
{
	bool bIsInputIdle = (GetOwner()->GetVelocity().Size() == 0.0) && (IALSXTCharacterInterface::Execute_GetCharacterControlRotation(GetOwner()) == PreviousControlRotation);
	PreviousControlRotation = IALSXTCharacterInterface::Execute_GetCharacterControlRotation(GetOwner());
	return bIsInputIdle;
}

TArray<FIdleAnimation> UALSXTIdleAnimationComponent::SelectIdleAnimations(const FGameplayTag& Sex, const FGameplayTag& Stance, const FGameplayTag& Overlay, const FGameplayTag& Injury, const FGameplayTag& CombatStance)
{
	UALSXTIdleAnimationSettings* Settings = IALSXTIdleAnimationComponentInterface::Execute_SelectIdleSettings(GetOwner());
	TArray<FIdleAnimation> Animations = Settings->IdleAnimations;
	FGameplayTagContainer TagsContainer;
	TArray<FIdleAnimation> SelectedAnimations;
	TagsContainer.AddTag(Sex);
	TagsContainer.AddTag(Stance);
	TagsContainer.AddTag(Overlay);
	TagsContainer.AddTag(Injury);
	TagsContainer.AddTag(CombatStance);

	// Return if there are no sounds
	if (Animations.Num() < 1)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "No Animations");
		return SelectedAnimations;
	}

	// Filter sounds based on Tag parameters
	for (FIdleAnimation Animation : Animations)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(Animation.Sex);
		CurrentTagsContainer.AppendTags(Animation.Stance);
		CurrentTagsContainer.AppendTags(Animation.Overlay);
		CurrentTagsContainer.AppendTags(Animation.Injury);
		CurrentTagsContainer.AppendTags(Animation.CombatStance);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			SelectedAnimations.Add(Animation);
		}
	}

	// Return if Return is there are no filtered sounds
	if (SelectedAnimations.Num() < 1)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "No Animations");
		return SelectedAnimations;
	}
	return SelectedAnimations;
}

UAnimMontage* UALSXTIdleAnimationComponent::GetNewIdleAnimation(TArray<FIdleAnimation> IdleAnimations)
{
	TArray<FIdleAnimation> FilteredAnimations;
	FIdleAnimation NewAnimationResult;
	UAnimMontage* NewAssetResult{ nullptr };

	// 	// Return if Return is there are no filtered sounds
	if (IdleAnimations.Num() < 1)
	{
		return NewAssetResult;
	}

	// If more than one result, avoid duplicates
	if (IdleAnimations.Num() > 1)
	{

		for (FIdleAnimation IdleAnimation : IdleAnimations)
		{
			if (!PreviousMontages.Contains(IdleAnimation.Montage))
			{
				FilteredAnimations.Add(IdleAnimation);
			}
		}

		//Shuffle Array
		for (int m = FilteredAnimations.Num() - 1; m >= 0; --m)
		{
			int n = FMath::Rand() % (m + 1);
			if (m != n) FilteredAnimations.Swap(m, n);
		}

		// Select Random Array Entry
		int RandIndex = FMath::RandRange(0, (FilteredAnimations.Num() - 1));
		NewAnimationResult = FilteredAnimations[RandIndex];
		NewAssetResult = NewAnimationResult.Montage;
		return NewAssetResult;
	}
	else
	{
		NewAnimationResult = FilteredAnimations[0];
		NewAssetResult = NewAnimationResult.Montage;
		return NewAssetResult;
	}
}

void UALSXTIdleAnimationComponent::SetNewAnimation(UAnimMontage* Animation, int NoRepeats)
{
	if (PreviousMontages.Num() >= abs(NoRepeats))
	{
		CurrentIdleMontage = Animation;
		PreviousMontages.Add(Animation);
		PreviousMontages.RemoveAt(0, 1, true);
	}
	else
	{
		CurrentIdleMontage = Animation;
		PreviousMontages.Add(Animation);
	}
}

void UALSXTIdleAnimationComponent::StartIdleCounterTimer()
{
	IdleCounterCurrent = 0.0f;
	IdleCounterTarget = FMath::RandRange(IdleAnimationSettings.TimeDelayBeforeIdle.X, IdleAnimationSettings.TimeDelayBeforeIdle.Y);
	GetWorld()->GetTimerManager().SetTimer(IdleCounterTimerHandle, this, &UALSXTIdleAnimationComponent::IdleCounterTimer, 0.01f, true);
}

void UALSXTIdleAnimationComponent::IdleCounterTimer()
{
	if (GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		return;
	}

	if (IsPlayerInputIdle())
	{
		IdleCounterCurrent = IdleCounterCurrent + 0.01;
		
		if (IdleCounterCurrent >= IdleCounterTarget && IdleAnimationSettings.EligibleStaminaLevels.HasTag(StatusState.CurrentStaminaTag) && IALSXTIdleAnimationComponentInterface::Execute_ShouldIdle(GetOwner()))
		{
			GetWorld()->GetTimerManager().ClearTimer(DelayBetweenAnimationsTimerHandle);
			ResetIdleCounterTimer();
			SetPlayerIdle(true);
			StartIdle();
		}
	}
	else
	{
		//StopIdle();
		SetPlayerIdle(false);
		StartIdleCounterTimer();
	}	
}

void UALSXTIdleAnimationComponent::ResetIdleCounterTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(IdleCounterTimerHandle);
	IdleCounterCurrent = 0.0f;
	IdleCounterTarget = 0.0f;
}

void UALSXTIdleAnimationComponent::StartDelayBetweenAnimationsTimer(float InitialDelay)
{
	CurrentTimeBetweenAnimations = 0.0f;
	TargetTimeBetweenAnimations = FMath::RandRange(IdleAnimationSettings.TimeDelayBetweenAnimations.X, IdleAnimationSettings.TimeDelayBetweenAnimations.Y);
	GetWorld()->GetTimerManager().SetTimer(DelayBetweenAnimationsTimerHandle, this, &UALSXTIdleAnimationComponent::DelayBetweenAnimationsTimer, 0.01f, true, InitialDelay);
}

void UALSXTIdleAnimationComponent::DelayBetweenAnimationsTimer()
{
	if (GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		return;
	}

	if (IsPlayerInputIdle())
	{
		CurrentTimeBetweenAnimations = CurrentTimeBetweenAnimations + 0.01;
		if (CurrentTimeBetweenAnimations >= TargetTimeBetweenAnimations && IALSXTIdleAnimationComponentInterface::Execute_ShouldIdle(GetOwner()))
		{
			ResetDelayBetweenAnimationsTimer();
			StartIdle();
		}
	}
	else
	{
		//StopIdle();
		SetPlayerIdle(false);
		StartIdleCounterTimer();
	}	
}

void UALSXTIdleAnimationComponent::ResetDelayBetweenAnimationsTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(DelayBetweenAnimationsTimerHandle);
	CurrentTimeBetweenAnimations = 0.0f;
	TargetTimeBetweenAnimations = 0.0f;
}

void UALSXTIdleAnimationComponent::StartCameraRotationTimer()
{
	GetWorld()->GetTimerManager().SetTimer(CameraRotationTimerHandle, CameraRotationTimerDelegate, 0.01f, true);
}

void UALSXTIdleAnimationComponent::CameraRotationTimer()
{
	// ...
}

void UALSXTIdleAnimationComponent::ResetCameraRotationTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(CameraRotationTimerHandle);
	CameraOffset = FVector::ZeroVector;
}

void UALSXTIdleAnimationComponent::StartIdle()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, "StartIdle");
	TArray<FIdleAnimation> SelectedAnimations = SelectIdleAnimations(IALSXTCharacterInterface::Execute_GetCharacterSex(GetOwner()), IALSXTCharacterInterface::Execute_GetCharacterStance(GetOwner()), IALSXTCharacterInterface::Execute_GetCharacterOverlayMode(GetOwner()), IALSXTCharacterInterface::Execute_GetCharacterInjury(GetOwner()), IALSXTCharacterInterface::Execute_GetCharacterCombatStance(GetOwner()));
	SetNewAnimation(GetNewIdleAnimation(SelectedAnimations), 1);
	float MontageLength {0.0f};
	if (IsValid(CurrentIdleMontage))
	{
		IALSXTCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetAnimInstance()->Montage_Play(CurrentIdleMontage, 1.0f);
		MontageLength = CurrentIdleMontage->GetPlayLength();
	}
	GetWorld()->GetTimerManager().ClearTimer(IdleCounterTimerHandle);
	ResetDelayBetweenAnimationsTimer();
	StartDelayBetweenAnimationsTimer(MontageLength);
}

void UALSXTIdleAnimationComponent::StopIdle()
{
	if (IsValid(CurrentIdleMontage))
	{
		IALSXTCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetAnimInstance()->Montage_Stop(0.5f, CurrentIdleMontage);
		SetPlayerIdle(false);
	}
}