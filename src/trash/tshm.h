
#pragma once
#include <TGlobal>


namespace Tf {

T_CORE_EXPORT void *shmcreate(const char *name, size_t size, bool *created = nullptr);

T_CORE_EXPORT void shmdelete(const char *name);

}
