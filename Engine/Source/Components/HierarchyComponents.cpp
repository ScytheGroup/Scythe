module;
#include "Serialization/SerializationMacros.h"
module Components;

import Serialization;
import :Hierarchy;


DEFINE_SERIALIZATION_AND_DESERIALIZATION(ParentComponent, parentId);
// Alex where do you put the common stuff?
DEFINE_SERIALIZATION_AND_DESERIALIZATION(Handle, id);