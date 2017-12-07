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

#define RAID
#define MBR
#define GPT
#define EXT2
#define PARTITION_

#include <raidfuse/raid.hpp>
#include <raidfuse/mbr.hpp>
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
	raidfuse::drive hdd0("/srv/benjamin/raid/dump4_hdd0.bin");
	raidfuse::drive hdd1("/srv/benjamin/raid/dump3_hdd1.bin");
	raidfuse::drive hdd2("/srv/benjamin/raid/dump2_hdd2.bin");
	raidfuse::drive hdd3("/srv/benjamin/raid/dump1_hdd3.bin");

	std::cout << "hdd0 size: " << hdd0.size() << std::endl;

	raidfuse::raid5 raid(32 * 1024);
	raid.add(hdd0);
	raid.add(hdd1);
	raid.add(hdd2);
	raid.add(hdd3);

	std::cout << "raid member count: " << raid.count() << std::endl;
	std::cout << "raid physical size: " << raid.physical_size() << " Bytes" << std::endl;
	std::cout << "raid logical size: " << raid.logical_size() << " Bytes" << std::endl;
	std::cout << "raid physical LBA: " << raid.physical_lba() << " LBAs" << std::endl;
	std::cout << "raid logical LBA: " << raid.logical_lba() << " LBAs" << std::endl;

	for (size_t physical_stripe = 0; physical_stripe < 48; physical_stripe++)
	{
		size_t logical, drive, stripe;
		raid.map2lba(physical_stripe, logical, drive, stripe);
	//	std::cout << std::setw(2) << physical_stripe << " " << std::setw(2) << logical << " " << std::setw(2) << drive << " " << std::setw(2) << stripe << std::endl;
	}

#ifdef MBR
	std::clog << "Checking MBR sector... " << std::flush;

	raidfuse::mbr::mbr_t mbr;
	if (!raid.read(0, (std::uint8_t *)&mbr))
	{
		throw std::runtime_error("MBR read error");
	}

	if (!mbr.valid())
	{
		throw std::runtime_error("MBR invalid");
	}

	std::clog << "OK" << std::endl;

	int index = 0;
	for (raidfuse::mbr::partition_t &partition: mbr.partition)
	{
		std::cout << "Partition " << (index++) << ": " << partition.sector_start << " - " << partition.sector_count << " (0x" << std::hex << (int)partition.type << std::dec << ")" << std::endl;
	}
#endif

#ifdef GPT
	std::clog << "Checking GPT sector... " << std::flush;

	raidfuse::gpt::header_t header;
	if (!raid.read(1, (std::uint8_t *)&header))
	{
		throw std::runtime_error("GPT header read error");
	}

	if (!header.valid())
	{
		throw std::runtime_error("GPT header invalid");
	}

	std::clog << "OK" << std::endl;

	std::cout << "[GPT header]" << std::endl;
	std::cout << "Header size: " << header.size << std::endl;
	std::cout << "Header checksum: 0x" << std::hex << header.crc << std::dec << std::endl;
	std::cout << std::endl;
	std::cout << "First header: " << header.offset_this << std::endl;
	std::cout << "Backup header: " << header.offset_backup << std::endl;
	std::cout << "First LBA: " << header.first_lba << std::endl;
	std::cout << "Last LBA: " << header.first_lba << std::endl;

	std::cout << "UUID:" << std::hex;
	for (std::uint8_t &item: header.uuid)
	{
		std::cout << " 0x" << (int)item;
	}
	std::cout << std::dec << std::endl;

	std::cout << "Partition start LBA: " << header.partition_lba << std::endl;
	std::cout << "Partition count: " << header.partition_count << std::endl;
	std::cout << "Partition size: " << header.partition_size << std::endl;
	std::cout << "Partition checksum: 0x" << std::hex << header.partition_crc << std::dec << std::endl;
	std::cout << std::endl;

	raidfuse::gpt::entry_t entry[4];
	if (!raid.read(2, (std::uint8_t *)entry))
	{
		throw std::runtime_error("GPT entry read error");
	}

	for (int i = 0; i < 4; i++)
	{
		std::cout << "[GPT entry #" << i << "]" << std::endl;

		std::cout << "Type: " << entry[i].type << std::endl;
		std::cout << "Partition: " << entry[i].partition << std::endl;
		std::cout << "Start: " << entry[i].start << std::endl;
		std::cout << "End: " << entry[i].end << std::endl;
		std::cout << "Attribute: " << entry[i].attribute << std::endl;
		std::cout << "Name: " << entry[i].name << std::endl;
		std::cout << std::endl;
	}
#endif

#ifdef PARITY_CHECK
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

#ifdef EXT2
	ext2_super_block block;
	if (!raid.read(36, (std::uint8_t *)&block))
	{
		throw std::runtime_error("EXT2 superblock read error");
	}

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
#endif

#endif
	return EXIT_SUCCESS;
}
