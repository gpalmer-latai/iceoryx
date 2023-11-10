#include <array>

struct LargeData
{
  std::array<uint8_t, 1024UL * 1024 * 128> data;
};
