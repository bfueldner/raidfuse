#pragma once

#include <cstdint>

#include <raidfuse/guid.hpp>

namespace raidfuse { namespace gpt {

typedef std::uint16_t name_t[36];

struct __attribute__((packed)) header_t
{
	std::uint8_t signature[8];
	std::uint32_t revision;
	std::uint32_t size;
	std::uint32_t crc;
	std::uint32_t reserved_0;
	std::uint64_t offset_this;
	std::uint64_t offset_backup;
	std::uint64_t first_lba;
	std::uint64_t last_lba;
	std::uint8_t uuid[16];
	std::uint64_t partition_lba;
	std::uint32_t partition_count;
	std::uint32_t partition_size;
	std::uint32_t partition_crc;
	std::uint8_t space[420];
};
static_assert(sizeof(header_t) == 512, "Size of GPT header mismatch!");

struct __attribute__((packed)) entry_t
{
	guid_t type;
	guid_t partition;
	std::uint64_t start;
	std::uint64_t end;
	std::uint64_t attribute;
	name_t name;
};
static_assert(sizeof(entry_t) == 128, "Size of GPT entry mismatch!");

} }
