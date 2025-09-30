#include "ObjectSystem.h"

namespace sakura {

	void ReferenceBase::IncrementReference() {
		if (manager != nullptr && reference != nullptr) {
			manager->IncrementReference(reference);
		}
	}

	void ReferenceBase::DecrementReference() {
		if (manager != nullptr && reference != nullptr) {
			manager->DecrementReference(reference);
		}
	}
}