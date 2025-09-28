#include "ObjectSystem.h"

namespace sakura {

	void CollectableBase::IncrementReference() {
		manager->IncrementReference(this);
	}

	void CollectableBase::DecrementReference(){
		manager->DecrementReference(this);
	}

}