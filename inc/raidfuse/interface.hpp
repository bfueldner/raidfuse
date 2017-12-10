#pragma once

#include <cstdint>
#include <cstddef>

namespace raidfuse { namespace interface {

class drive
{
	public:
		static constexpr size_t sector_size = 512;

		virtual size_t size() = 0;
		virtual size_t read(size_t lba, std::uint8_t *data) = 0;
};

} }
