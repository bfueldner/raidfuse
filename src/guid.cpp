#include <iostream>
#include <iomanip>

#include <raidfuse/guid.hpp>

std::ostream& operator<<(std::ostream& out, raidfuse::guid_t guid)
{
	out << std::hex;
	for (int index = 0; index < 4; index++)
	{
		out << std::setfill('0') << std::setw(2) << (int)guid[3 - index];
	}
	out << "-";
	for (int index = 0; index < 2; index++)
	{
		out << std::setfill('0') << std::setw(2) << (int)guid[5 - index];
	}
	out << "-";
	for (int index = 0; index < 2; index++)
	{
		out << std::setfill('0') << std::setw(2) << (int)guid[7 - index];
	}
	out << "-";
	for (int index = 0; index < 2; index++)
	{
		out << std::setfill('0') << std::setw(2) << (int)guid[index + 8];
	}
	out << "-";
	for (int index = 0; index < 6; index++)
	{
		out << std::setfill('0') << std::setw(2) << (int)guid[index + 10];
	}
	out << std::dec;
	return out;
}
