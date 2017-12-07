#pragma once

#include <vector>

#include <raidfuse/drive.hpp>

namespace raidfuse {

class raid5
{
	public:
		raid5(size_t stripe = 32 * 1024):
			m_stripe(stripe)
		{

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
			size_t mod_4 = index % m_drives.size();
			size_t div_4 = index / m_drives.size();
			size_t div_mod_4 = div_4 % m_drives.size();

			size_t pos = m_drives.size() - div_mod_4 - 1;

			if (mod_4 == pos)
			{
				std::cout << std::setw(2) << index << " " << mod_4 << " " << div_4 << " " << div_mod_4 << " " << pos << " parity" << std::endl;
			}
			else
			{
				std::cout << std::setw(2) << index << " " << mod_4 << " " << div_4 << " " << div_mod_4 << " " << pos << std::endl;
			}
		}

		void disk(size_t index)
		{
			size_t mod_4 = index % m_drives.size();
			size_t div_4 = index / m_drives.size();
			size_t div_mod_4 = div_4 % m_drives.size();

			size_t mod_3 = index % (m_drives.size() - 1);
			size_t div_3 = index / (m_drives.size() - 1);
		//	size_t div_mod_4 = div_4 % m_drives.size();

			size_t pos = m_drives.size() - div_mod_4 - 1;

			std::cout << std::setw(2) << index << " " << mod_4 << " " << div_4 << " " << div_mod_4 << " " << pos << " " << mod_3 << " " << div_3 << " (" << (index + div_3) << ")" << std::endl;
		}

	protected:
		std::vector<drive *> m_drives;
		size_t m_stripe;
};

}
