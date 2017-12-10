/**
 * @see https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout
 *
 * @see https://de.wikipedia.org/wiki/Master_Boot_Record
 * @see https://de.wikipedia.org/wiki/GUID_Partition_Table
 *
 * http://libfuse.github.io/doxygen/
 */

#define FUSE_USE_VERSION 29

#include <stdio.h>
#include <time.h>

#include <iostream>
#include <iomanip>
#include <vector>

#include <cstdint>
#include <cmath>

#include <ext2fs/ext2fs.h>

#include <fuse.h>

#define RAID
#define MBR
#define GPT
#define EXT2
#define PARTITION_

#include <raidfuse/raid.hpp>
#include <raidfuse/mbr.hpp>
#include <raidfuse/gpt.hpp>
#include <raidfuse/partition.hpp>

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

static const char *raid_file = "/raid";
static const char *partition_file = "/partition";

raidfuse::raid5 raid(256 * 1024);
raidfuse::partition *part;
//std::vector<raidfuse::partition *> paritions;

int raid_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0)
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else
	if (strcmp(path, raid_file) == 0)
	{
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = raid.size();
		stbuf->st_atime = 0;
	}
	else
	if (strcmp(path, partition_file) == 0)
	{
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = part->size();
		stbuf->st_atime = 0;
	}
	else
		res = -ENOENT;

	return res;
}

int raid_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	if (strcmp(path, "/") != 0)
	{
		return -ENOENT;
	}

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, raid_file + 1, NULL, 0);
	filler(buf, partition_file + 1, NULL, 0);

/*
	for (std::string &entry: entries)
	{
		filler(buf, entry.c_str(), NULL, 0);
	}
*/

	return 0;
}

int raid_open(const char *path, struct fuse_file_info *fi)
{
	if ((strcmp(path, raid_file) != 0) && (strcmp(path, partition_file) != 0))
	{
		return -ENOENT;
	}

	if ((fi->flags & 3) != O_RDONLY)
	{
		return -EACCES;
	}
	return 0;
}

int raid_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	constexpr size_t sector_size = 512;

	size_t len;
	(void) fi;

	if (strcmp(path, raid_file) == 0)
	{
		std::clog << "read: offset = " << offset << ", size = " << size << std::endl;

		if (offset + size > raid.logical_size())
		{
			std::cerr << " End of disk!" << std::endl;
			return 0;
		}

		size_t test_offset = offset % sector_size;
		size_t test_size = size % sector_size;

		if ((test_offset) || (test_size))
		{
			std::cerr << "Out of bound!" << std::endl;
			return 0;
		}

		size_t lba_offset = offset / sector_size;
		size_t lba_size = size / sector_size;

		while (lba_size)
		{
			std::clog << "      lba = " << lba_offset << std::endl;
			raid.read(lba_offset, (std::uint8_t *)buf);

			buf += sector_size;
			lba_offset++;
			lba_size--;
		}

	}
	else
	if (strcmp(path, partition_file) == 0)
	{
		std::clog << "read: offset = " << offset << ", size = " << size << std::endl;

		if (offset + size > part->size())
		{
			std::cerr << " End of partition!" << std::endl;
			return 0;
		}

		size_t test_offset = offset % sector_size;
		size_t test_size = size % sector_size;

		if ((test_offset) || (test_size))
		{
			std::cerr << "Out of bound!" << std::endl;
			return 0;
		}

		size_t lba_offset = offset / sector_size;
		size_t lba_size = size / sector_size;

		while (lba_size)
		{
			std::clog << "      lba = " << lba_offset << std::endl;
			part->read(lba_offset, (std::uint8_t *)buf);

			buf += sector_size;
			lba_offset++;
			lba_size--;
		}
	}
	else
	{
		return -ENOENT;
	}



/*
	len = strlen(hello_str);
	if (offset < len)
	{
		if (offset + size > len)
		{
			size = len - offset;
		}
		memcpy(buf, hello_str + offset, size);
	}
	else
		size = 0;
*/

	return size;
}

fuse_operations fuse_callback;

int main(int argc, char** argv)
{
#ifdef RAID
	raidfuse::drive hdd0("/srv/benjamin/raid/dump4_hdd0.bin");
	raidfuse::drive hdd1("/srv/benjamin/raid/dump3_hdd1.bin");
	raidfuse::drive hdd2("/srv/benjamin/raid/dump2_hdd2.bin");
	raidfuse::drive hdd3("/srv/benjamin/raid/dump1_hdd3.bin");

	std::cout << "hdd0 size: " << hdd0.size() << std::endl;

//	raidfuse::raid5 raid(32 * 1024);
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

		if ((entry[i].start) && (entry[i].end))
		{
			part = new raidfuse::partition(raid, "partition", entry[i].start, entry[i].end);

		/*
			raidfuse::partition *partition = new raidfuse::partition(raid, "partition", entry[i].start, entry[i].end);
			paritions.push_back(partition);
		*/
		//	paritions.push_back({ raid, "partition", entry[i].start, entry[i].end });
		}
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
	fuse_callback.getattr = raid_getattr;
	fuse_callback.readdir = raid_readdir;
	fuse_callback.open = raid_open;
	fuse_callback.read = raid_read;

/*
	entries.push_back("disk");
	entries.push_back("partition1");
	entries.push_back("partition2");
*/
	return fuse_main(argc, argv, &fuse_callback, NULL);
//	return EXIT_SUCCESS;
}
