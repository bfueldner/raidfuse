#pragma once

#include <string>
#include <fstream>
#include <stdexcept>

#include <raidfuse/interface.hpp>

namespace raidfuse {

class drive:
	public interface::drive
{
	public:
		drive(std::string filename):
			m_stream(filename, std::ios::binary)
		{
			if (!m_stream.is_open())
			{
				throw std::runtime_error("Error opening file '" + filename + "'");
			}

			m_stream.seekg(0, std::ios::end);
			m_size = m_stream.tellg();
			m_stream.seekg(0);
		}

		virtual size_t size()
		{
			return m_size;
		}

		virtual size_t read(size_t lba, std::uint8_t *data)
		{
			m_stream.seekg(lba * sector_size);
			m_stream.read((char *)data, sector_size);
			return m_stream.gcount();
		}

	protected:
		std::ifstream m_stream;
		size_t m_size;
};

}
