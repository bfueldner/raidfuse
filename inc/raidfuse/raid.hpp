#pragma once

#include <vector>

#include <raidfuse/drive.hpp>

namespace raidfuse {

class raid5:
	public interface::drive
{
	public:
		raid5(const size_t stripe = 32 * 1024):
			m_stripe_size(stripe),
			m_stripe_lba(stripe / sector_size),
			m_count(0),
			m_physical_size(0),
			m_logical_size(0),
			m_physical_lba(0),
			m_logical_lba(0),
			m_physical_sequence(0),
			m_logical_sequence(0)
		{

		}

		size_t count() const { return m_count; }
		size_t physical_size() const { return m_physical_size; }
		size_t logical_size() const { return m_logical_size; }
		size_t physical_lba() const { return m_physical_lba; }
		size_t logical_lba() const { return m_logical_lba; }

		void add(raidfuse::drive &drv)
		{
			/* Check sector boundary */
			if (drv.size() % sector_size)
			{
				throw std::runtime_error("Drive size out of sector boundary!");
			}

			/* Check stripe boundary */
			if (drv.size() % m_stripe_size)
			{
			//	throw std::runtime_error("Drive size out of stripe boundary!");
			}

			if (m_drives.size())
			{
				size_t size = m_drives.front()->size();
				if (size != drv.size())
				{
					throw std::runtime_error("Can not add drive with different size!");
				}
			}

			m_drives.push_back(&drv);
			calculate();
		}

		size_t physical(size_t lba, std::uint8_t *data)
		{
			size_t stripe_index = lba / m_stripe_lba;
			size_t stripe_lba = lba % m_stripe_lba;
			size_t stripe_block = stripe_index / m_count;
			size_t drive = stripe_index % m_count;
			size_t physical_lba = (stripe_lba + stripe_block * m_stripe_lba);

			std::cout << std::setw(4) << lba << std::setw(4) << physical_lba << std::setw(4) << stripe_index << std::setw(4) << drive<< std::endl;

			return m_drives[drive]->read(physical_lba, data);
		}

		virtual size_t size()
		{
			return m_logical_size;
		}

		virtual size_t read(size_t lba, std::uint8_t *data)
		{
			size_t stripe_lba = lba / m_stripe_lba;
			size_t stripe_index = lba % m_stripe_lba;

			size_t logical_lba, drive, stripe;
			map(stripe_lba, logical_lba, drive, stripe);

			size_t drive_lba = stripe * m_stripe_lba + stripe_index;

		//	std::cout << std::setw(4) << lba << std::setw(4) << stripe_lba << std::setw(4) << logical_lba << std::setw(4) << drive << std::setw(4) << drive_lba << std::endl;

			return m_drives[drive]->read(drive_lba, data);
		}

		/**
		 * @brief Check parity of each stripe/sector/byte
		 * @return false on error, true on success
		 */
		bool check()
		{
			std::vector< std::vector<std::uint8_t> > buffer;
			buffer.resize(m_count);
			for (size_t index = 0; index < m_count; index++)
			{
				buffer[index].resize(sector_size);
			}

			bool result = true;
			for (size_t logical_lba = 0; logical_lba < m_physical_lba && result; logical_lba++)
			{
				size_t drive = logical_lba % m_count;
				size_t physical_lba = logical_lba / m_count;

				m_drives[drive]->read(physical_lba, &buffer[drive][0]);

				/* Last drive of raid -> check */
				if (drive == m_count - 1)
				{
					for (size_t index = 0; index < sector_size; index++)
					{
						std::uint8_t parity = 0;
						for (size_t drive = 0; drive < m_count; drive++)
						{
							parity ^= buffer[drive][index];
						}

						if (parity)
						{
							result = false;
							break;
						}
					}
				}
			}
			return result;
		}

		/**
		 * @brief Map physical stripes to logical stripes
		 * @param physical
		 * @param logical
		 * @param drive
		 * @param lba
		 */
		void map(size_t physical, size_t &logical, size_t &drive, size_t &stripe)
		{
			size_t physical_stripe = physical / m_logical_sequence;
			size_t physical_drive = physical % m_logical_sequence;

			logical = physical + physical_stripe * m_count + m_offset[physical_drive];
			stripe = logical / m_count;
			drive = logical % m_count;
		}

		void map2lba(size_t physical, size_t &logical, size_t &drive, size_t &stripe)
		{
			size_t physical_stripe = physical / m_logical_sequence;
			size_t physical_drive = physical % m_logical_sequence;

			logical = physical + physical_stripe * m_count + m_offset[physical_drive];
			stripe = logical / m_count;
			drive = logical % m_count;
		}

	protected:
		const size_t m_stripe_size;
		const size_t m_stripe_lba;

		std::vector<raidfuse::drive *> m_drives;
		std::vector<size_t> m_offset;

		size_t m_count;
		size_t m_physical_size;
		size_t m_logical_size;
		size_t m_physical_lba;
		size_t m_logical_lba;
		size_t m_physical_sequence;
		size_t m_logical_sequence;

		/**
		 * @brief Check, if selected stripe is a parity stripe
		 * @param stripe
		 * @return
		 */
		bool is_parity_stripe(size_t stripe) const
		{
			size_t drive_number = stripe % m_count;
			size_t drive_lba = stripe / m_count;
			size_t parity_drive = m_count - (drive_lba % m_count) - 1;

			return (drive_number == parity_drive);
		}

		/**
		 * @brief Calculate stripe offset table
		 */
		void calculate()
		{
			m_offset.clear();

			m_count = m_drives.size();
			if (m_count)
			{
				m_physical_size = m_count * m_drives.front()->size();
				m_logical_size = m_physical_size - m_drives.front()->size();
			}
			else
			{
				m_physical_size = 0;
				m_logical_size = 0;
			}
			m_physical_lba = m_physical_size / sector_size;
			m_logical_lba = m_logical_size / sector_size;
			m_physical_sequence = m_count * m_count;
			m_logical_sequence = m_physical_sequence - m_count;

			int value = 0;
			for (size_t stripe = 0; stripe < m_physical_sequence; stripe++)
			{
				if (is_parity_stripe(stripe))
				{
					value++;
				}
				else
				{
					m_offset.push_back(value);
				}
			}
		}
};

}
