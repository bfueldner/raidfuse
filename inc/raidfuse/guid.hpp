#pragma once

#include <cstdint>

namespace raidfuse {

typedef std::uint8_t guid_t[16];

}

std::ostream& operator<<(std::ostream& out, raidfuse::guid_t uuid);
