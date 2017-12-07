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

		size_t read(std::uint8_t *data, size_t size)
		{
			m_stream.read((char *)data, size);
			return m_stream.gcount();
		}

	protected:
		std::ifstream m_stream;
		size_t m_size;
};

}
