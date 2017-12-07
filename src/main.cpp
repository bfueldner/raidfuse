/**
 * @see https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout
 *
 * @see https://de.wikipedia.org/wiki/Master_Boot_Record
 * @see https://de.wikipedia.org/wiki/GUID_Partition_Table
 */

#include <stdio.h>
#include <time.h>

#include <iostream>
#include <iomanip>

#include <cstdint>
#include <cmath>

#include <ext2fs/ext2fs.h>

#include <fuse.h>

#define PARTITION_

static constexpr std::uint8_t safety_mbr = 0xEE;
static constexpr std::uint8_t efi_partition = 0xEF;

typedef std::uint8_t guid_t[16];
typedef std::uint16_t name_t[36];

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
};
static_assert(sizeof(mbr_t) == 512, "Size of MBR mismatch!");

struct __attribute__((packed)) gpt_header_t
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
static_assert(sizeof(gpt_header_t) == 512, "Size of GPT header mismatch!");

struct __attribute__((packed)) gpt_entry_t
{
	guid_t type;
	guid_t partition;
	std::uint64_t start;
	std::uint64_t end;
	std::uint64_t attribute;
	name_t name;
};
static_assert(sizeof(gpt_entry_t) == 128, "Size of GPT entry mismatch!");

std::ostream& operator<<(std::ostream& out, guid_t guid)
{
	out << std::hex;
	for (int index = 0; index < 4; index++)
	{
		out << std::setfill('0') << std::setw(2) << (int)guid[3 - index];
	}
	out << "-";
	for (int index = 0; index < 2; index++)
	{
		out << std::setfill('0') << std::setw(2) << (int)guid[5 - index];
	}
	out << "-";
	for (int index = 0; index < 2; index++)
	{
		out << std::setfill('0') << std::setw(2) << (int)guid[7 - index];
	}
	out << "-";
	for (int index = 0; index < 2; index++)
	{
		out << std::setfill('0') << std::setw(2) << (int)guid[index + 8];
	}
	out << "-";
	for (int index = 0; index < 6; index++)
	{
		out << std::setfill('0') << std::setw(2) << (int)guid[index + 10];
	}
	out << std::dec;
	return out;
}

std::ostream& operator<<(std::ostream& out, name_t name)
{
	for (int index = 0; index < 36; index++)
	{
		if (!name[index])
			return out;

		out << (char)name[index];
	}
	return out;
}


static struct fuse_operations hello_oper =
{
	.init           = hello_init,
	.getattr	= hello_getattr,
	.readdir	= hello_readdir,
	.open		= hello_open,
	.read		= hello_read,
};


int main(int, char**)
{
#ifdef PARTITION
	FILE *handle;
	handle = fopen("/dev/sda", "rb");
	if (!handle)
	{
		printf("Error: Can not open file\n");
		return EXIT_FAILURE;
	}

	mbr_t mbr;
	fread(&mbr, sizeof(mbr_t), 1, handle);

	if ((mbr.null != 0) && (mbr.sector_signature != 0xAA55))
	{
		std::cerr << "MBR signature mismatch" << std::endl;
	}

	int index = 0;
	for (partition_t &partition: mbr.partition)
	{
		std::cout << "Partition " << (index++) << ": " << partition.sector_start << " - " << partition.sector_count << " (0x" << std::hex << (int)partition.type << std::dec << ")" << std::endl;
	}

	gpt_header_t gpt_header;
	fread(&gpt_header, sizeof(gpt_header_t), 1, handle);

	if ((memcmp(gpt_header.signature, "EFI PART", sizeof(gpt_header.signature))) || (gpt_header.revision != 0x00010000) || (gpt_header.reserved_0))
	{
		std::cerr << "GPT header not found!" << std::endl;
	}

	std::cout << "[GPT header]" << std::endl;
	std::cout << "Header size: " << gpt_header.size << std::endl;
	std::cout << "Header checksum: 0x" << std::hex << gpt_header.crc << std::dec << std::endl;
	std::cout << std::endl;
	std::cout << "First header: " << gpt_header.offset_this << std::endl;
	std::cout << "Backup header: " << gpt_header.offset_backup << std::endl;
	std::cout << "First LBA: " << gpt_header.first_lba << std::endl;
	std::cout << "Last LBA: " << gpt_header.first_lba << std::endl;

	std::cout << "UUID:" << std::hex;
	for (std::uint8_t &item: gpt_header.uuid)
	{
		std::cout << " 0x" << (int)item;
	}
	std::cout << std::dec << std::endl;

	std::cout << "Partition start LBA: " << gpt_header.partition_lba << std::endl;
	std::cout << "Partition count: " << gpt_header.partition_count << std::endl;
	std::cout << "Partition size: " << gpt_header.partition_size << std::endl;
	std::cout << "Partition checksum: 0x" << std::hex << gpt_header.partition_crc << std::dec << std::endl;
	std::cout << std::endl;

	gpt_entry_t gpt_entry;

	for (int i = 0; i < 8; i++)
	{
		std::cout << "[GPT entry #" << i << "]" << std::endl;
		fread(&gpt_entry, sizeof(gpt_entry_t), 1, handle);
		std::cout << "Type: " << gpt_entry.type << std::endl;
		std::cout << "Partition: " << gpt_entry.partition << std::endl;
		std::cout << "Start: " << gpt_entry.start << std::endl;
		std::cout << "End: " << gpt_entry.end << std::endl;
		std::cout << "Attribute: " << gpt_entry.attribute << std::endl;
		std::cout << "Name: " << gpt_entry.name << std::endl;
		std::cout << std::endl;
	}

#endif


#ifdef EXT2
	FILE *handle;
	handle = fopen("/dev/sda5", "rb");
	if (!handle)
	{
		printf("Error: Can not open file\n");
		return EXIT_FAILURE;
	}
//	printf("Handle = %i\n", (long unsigned)handle);
	fseek(handle, 0x400, SEEK_SET);

	char buffer[255];
//	tm time = ctime()

	ext2_super_block block;
	printf("Read = %lu\n", fread(&block, sizeof(ext2_super_block), 1, handle));

	printf("Total inode count: %d\n", block.s_inodes_count);
	printf("Total block count: %d\n", block.s_blocks_count);
	printf("This number of blocks can only be allocated by the super-user: %d\n", block.s_r_blocks_count);
	printf("Free block count: %d\n", block.s_free_blocks_count);
	printf("Free inode count: %d\n", block.s_free_inodes_count);

	printf("First data block: %d\n", block.s_first_data_block); // This must be at least 1 for 1k-block filesystems and is typically 0 for all other block sizes.
	printf("Block size: %d\n", (long)std::pow(2u, block.s_log_block_size + 10u));	// 2 ^ (10 + s_log_block_size).
	printf("Cluster size: %d\n", (long)std::pow(2u, block.s_log_cluster_size));	// (2 ^ s_log_cluster_size) blocks if bigalloc is enabled, zero otherwise.
	printf("Blocks per group: %d\n", block.s_blocks_per_group);
	printf("Clusters per group: %d\n", block.s_clusters_per_group);	// if bigalloc is enabled.
	printf("Inodes per group: %d\n", block.s_inodes_per_group);
	time_t raw_time = block.s_mtime;
	printf("Mount time: %s", ctime(&raw_time));	// in seconds since the epoch.
	raw_time = block.s_wtime;
	printf("Write time: %s", ctime(&raw_time));	// in seconds since the epoch.
	printf("Number of mounts since the last fsck: %d\n", block.s_mnt_count);
	printf("Number of mounts beyond which a fsck is needed: %d\n", block.s_max_mnt_count);
	printf("Magic = 0x%.4X\n", block.s_magic);

	fclose(handle);
#endif
	return EXIT_SUCCESS;
}
