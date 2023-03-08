#pragma once

#include "ALSXTCombatInterface.generated.h"

UINTERFACE(Blueprintable)
class UALSXTCombatInterface : public UInterface {
	GENERATED_BODY()
};

class ALSXT_API IALSXTCombatInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat")
  bool IsTrue();

};