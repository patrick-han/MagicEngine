#include "UUID.h"

#if MAGIC_USE_SIMPLE_UUID
std::size_t Magic::UUID::nextUUID = 0;
#endif // MAGIC_USE_SIMPLE_UUID