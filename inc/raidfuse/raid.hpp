#pragma once

#include <vector>

#include <raidfuse/drive.hpp>

namespace raidfuse {

class raid5
{
	public:
		raid5(const size_t stripe = 32 * 1024):
			m_stripe(stripe),
			m_count(0),
			m_physical_sequence(0),
			m_logical_sequence(0)
		{

		}

		size_t count() const
		{
			return m_count;
		}

		size_t size()
		{
			if (m_drives.size())
			{
				return m_drives.front()->size() * (m_drives.size() - 1);
			}
			return 0;
		}

		void add(drive &drv)
		{
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

		bool check()
		{
			constexpr size_t count = 128;

			std::uint8_t buffer[m_drives.size()][count];

			size_t index = 0;
			for (drive *drv: m_drives)
			{
				drv->read(buffer[index++], count);
			}

			bool result = true;
			for (size_t index = 0; index < count; index++)
			{
				std::uint8_t parity = 0;
				std::cout << std::hex << std::setfill('0');
				for (size_t drv_index = 0; drv_index < m_drives.size(); drv_index++)
				{
					std::cout << "0x" << std::setw(2) << (int)buffer[drv_index][index] << " ";
					parity ^= buffer[drv_index][index];
				}
				std::cout << "- 0x" << std::setw(2) << (int)parity << std::dec << std::endl;

				if (parity)
				{
					result = false;
				}
			}
			return result;
		}

		void lba(size_t index)
		{

		}

		void disk(size_t index)
		{

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

	protected:
		const size_t m_stripe;
		std::vector<drive *> m_drives;
		std::vector<size_t> m_offset;

		size_t m_count;
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
