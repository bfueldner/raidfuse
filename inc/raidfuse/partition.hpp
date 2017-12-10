#pragma once

#include <string>

#include <raidfuse/interface.hpp>

namespace raidfuse {

class partition:
	public interface::drive
{
	public:
		partition(interface::drive &drive, std::string name, size_t start, size_t end):
			m_drive(drive),
			m_name(name),
			m_start(start),
			m_end(end)
		{

		}

		virtual size_t size()
		{
			return ((m_end - m_start) + 1) * sector_size;
		}

		virtual size_t read(size_t lba, uint8_t *data)
		{
			return m_drive.read(lba + m_start, data);
		}

		std::string name() const
		{
			return m_name;
		}

	protected:
		interface::drive &m_drive;
		std::string m_name;
		size_t m_start;
		size_t m_end;
};

}
