#include "./validate.hpp"

#include <stdint.h>

extern "C" int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
   return validate_one_input (data, size);
}
