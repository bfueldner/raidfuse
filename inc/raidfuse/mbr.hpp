#pragma once

#include <cstdint>

namespace raidfuse { namespace mbr {

static constexpr std::uint8_t safety_mbr = 0xEE;
static constexpr std::uint8_t efi_partition = 0xEF;

struct __attribute__((packed)) partition_t
{
	std::uint8_t boot;
	std::uint8_t chs_start[3];
	std::uint8_t type;
	std::uint8_t chs_end[3];
	std::uint32_t sector_start;
	std::uint32_t sector_count;
};
static_assert(sizeof(partition_t) == 16, "Size of partition mismatch!");

struct __attribute__((packed)) mbr_t
{
	std::uint8_t bootloader[440];
	std::uint32_t drive_signature;
	std::uint16_t null;
	partition_t partition[4];
	std::uint16_t sector_signature;

	/**
	 * @brief Simple data validation
	 * @return true, if data is valid
	 */
	bool valid()
	{
		return (null == 0) && (sector_signature == 0xAA55);
	}
};
static_assert(sizeof(mbr_t) == 512, "Size of MBR mismatch!");

} }
