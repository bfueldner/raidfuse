/**
 * @see https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout
 *
 * @see https://de.wikipedia.org/wiki/Master_Boot_Record
 * @see https://de.wikipedia.org/wiki/GUID_Partition_Table
 *
 * http://libfuse.github.io/doxygen/
 */

#include <stdio.h>
#include <time.h>

#include <iostream>
#include <iomanip>
#include <vector>

#include <cstdint>
#include <cmath>

#include <ext2fs/ext2fs.h>

#define TEST
#define RAID
#define PARTITION_

#include <raidfuse/raid.hpp>
#include <raidfuse/gpt.hpp>

std::ostream& operator<<(std::ostream& out, raidfuse::gpt::name_t name)
{
	for (int index = 0; index < 36; index++)
	{
		if (!name[index])
			return out;

		out << (char)name[index];
	}
	return out;
}

int main(int, char**)
{
#ifdef RAID
	raidfuse::drive hdd0("raidfuse");
	raidfuse::drive hdd1("raidfuse");
	raidfuse::drive hdd2("raidfuse");
	raidfuse::drive hdd3("raidfuse");

	std::cout << "hdd0 size: " << hdd0.size() << std::endl;

	raidfuse::raid5 raid;
	raid.add(hdd0);
	raid.add(hdd1);
	raid.add(hdd2);
	raid.add(hdd3);

	std::cout << "raid size: " << raid.size() << std::endl;
	std::cout << "raid member count: " << raid.count() << std::endl;

	for (size_t physical_stripe = 0; physical_stripe < 48; physical_stripe++)
	{
		size_t logical, drive, stripe;
		raid.map(physical_stripe, logical, drive, stripe);
		std::cout << std::setw(2) << physical_stripe << " " << std::setw(2) << logical << " " << std::setw(2) << drive << " " << std::setw(2) << stripe << std::endl;
	}

#ifdef XORCHECK
	if (raid.check())
	{
		std::cout << "Check passed" << std::endl;
	}
	else
	{
		std::cerr << "Check failed" << std::endl;
	}
	std::cout << std::endl;
#endif

#endif

#ifdef PARTITION
	FILE *handle;
//	handle = fopen("/dev/sda", "rb");
	handle = fopen("/srv/benjamin/raid/dump4_hdd0.bin", "rb");
	if (!handle)
	{
		printf("Error: Can not open file\n");
		return EXIT_FAILURE;
	}

	mbr_t mbr;
	fread(&mbr, sizeof(mbr_t), 1, handle);



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
