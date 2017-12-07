#pragma once

#include <string>
#include <fstream>
#include <stdexcept>

namespace raidfuse {

class drive
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

		size_t size() const
		{
			return m_size;
		}

	/*
		size_t read(std::uint8_t *data, size_t size)
		{
			m_stream.read((char *)data, size);
			return m_stream.gcount();
		}
	*/
		size_t read(size_t lba, std::uint8_t *data)
		{
			m_stream.seekg(lba * sector_size);
			m_stream.read((char *)data, sector_size);
			return m_stream.gcount();
		}

	protected:
		static constexpr size_t sector_size = 512;

		std::ifstream m_stream;
		size_t m_size;
};

}
