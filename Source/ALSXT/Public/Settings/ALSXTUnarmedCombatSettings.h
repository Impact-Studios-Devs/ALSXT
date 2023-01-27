#pragma once

#include "Utility/ALSXTStructs.h"
#include "ALSXTUnarmedCombatSettings.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FALSXTUnarmedCombatSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAllowUnarmedCombat{ true };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRotateToInputOnStart{ true };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0))
	float RotationInterpolationSpeed{ 10.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	TArray<FUnarmedAttackType> UnarmedAttackTypes;

};