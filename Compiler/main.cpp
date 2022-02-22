#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <map>

#define ADDRESS_RANGE 15

size_t eepromSize = std::pow((int)2, ADDRESS_RANGE);
int8_t* eeprom = nullptr;

#define INSTR(d, c, b, a) (uint8_t)(d << 3 | c << 2 | b << 1 | a)
#define FLAG1 (uint8_t)(1 << 7)
#define FLAG2 (uint8_t)(1 << 6)
#define FLAG3 (uint8_t)(1 << 5)
#define FLAG4 (uint8_t)(1 << 4)

enum Instructions : uint8_t
{
	NOP = INSTR(0, 0, 0, 0),
	HLT = INSTR(0, 0, 0, 1),
	LDR = INSTR(0, 0, 1, 0),
	STR = INSTR(0, 0, 1, 1),
	CPY = INSTR(0, 0, 1, 1) | FLAG4,
	SET = INSTR(0, 1, 0, 0),
	ADD = INSTR(0, 1, 0, 1),
	SUB = INSTR(0, 1, 0, 1) | FLAG2,
	JMP = INSTR(0, 1, 1, 0),
	JPC = INSTR(0, 1, 1, 0) | FLAG3,
	JPN = INSTR(0, 1, 1, 0) | FLAG4,
	JPZ = INSTR(0, 1, 1, 0) | FLAG3 | FLAG4,
	LABEL,
};

std::map<std::string, Instructions> instructions = {
	{ "NOP", NOP },
	{ "HLT", HLT },
	{ "LDR", LDR },
	{ "STR", STR },
	{ "CPY", CPY },
	{ "SET", SET },
	{ "ADD", ADD },
	{ "SUB", SUB },
	{ "JMP", JMP },
	{ "JPC", JPC },
	{ "JPN", JPN },
	{ "JPZ", JPZ },
	{ "LBS", LABEL },
};

#undef INSTR

std::map<std::string, size_t> labelList;

std::string extractNextToken(std::string& line)
{
	size_t pos1 = line.find_first_not_of(' ');
	size_t pos2 = line.find_first_of(' ', pos1);
	if (pos1 == std::string::npos)
		return "";

	std::string token = line.substr(pos1, pos2 - pos1);

	line.erase(0, pos2);

	return token;
}

uint8_t decodeHex(std::string token)
{
	return std::stoul(token, nullptr, 16);
}

bool decodeInstruction(std::string line, uint8_t& partA, uint8_t& partB, size_t address)
{
	partA = 0x00;
	partB = 0x00;

	std::string token = extractNextToken(line);

	if (token.empty())
		return false;

	std::map<std::string, Instructions>::iterator it = instructions.find(token);
	if (it == instructions.end())
		throw std::exception("Invalid instr.");

	printf("-> <addr>%u %s ", address, token.c_str());

	Instructions instr = it->second;

	partA = (uint8_t)instr;

	// First parameter
	token = extractNextToken(line);

	switch (instr)
	{
	case LABEL:
		labelList[token] = address;
		return false;
	case NOP:
	case HLT:
		return true;
	case LDR:
	{
		if (token == "RAM")
			printf("from RAM ");
		else if (token == "ROM")
		{
			partA |= FLAG3;
			printf("from ROM ");
		}
		else
			throw std::exception("LDR Invalid source (must be RAM or ROM)");
	} break;
	case STR:
	case CPY:
	{
		if (token == "rA")
			printf("from reg. A ");
		else if (token == "rB")
		{
			partA |= FLAG3;
			printf("from reg. B ");
		}
		else
			throw std::exception("STR, CPY Invalid p1 (must be rA or rB)");
	} break;
	case SET:
	{
		if (token == "rA")
			printf("reg. A");
		else if (token == "rB")
		{
			partA |= FLAG3;
			printf("reg. B");
		}
		else
			throw std::exception("ADD or SUB Invalid p1 (must be rA or rB)");
	} break;
	case ADD:
	case SUB:
	{
		if (token == "rA")
			printf(" rA and rB to reg. A");
		else if (token == "rB")
		{
			partA |= FLAG3;
			printf(" rA and rB to reg. B");
		}
		else
			throw std::exception("ADD or SUB Invalid p1 (must be rA or rB)");
	} break;
	case JMP:
	case JPC:
	case JPN:
	case JPZ:
	{
		if ((token.size() != 3 && token.data()[0] == '$') || token.data()[0] != '@')
			throw std::exception("JMP, JPC, JPN, JPZ Invalid address or token (must be $XX or @LABEL)");

		if (token[0] == '$')
		{
			token.erase(0, 1);

			try
			{
				partB = decodeHex(token);
			}
			catch (std::exception e)
			{
				throw std::exception("JMP, JPC, JPN, JPZ Invalid address (must be $XX)");
			}

			printf(" to address %i", partB);
		}
		else
		{
			token.erase(0, 1);

			std::map<std::string, size_t>::iterator it = labelList.find(token);
			if (it == labelList.end())
				throw std::exception("JMP, JPC, JPN, JPZ Invalid label (must be @LABEL)");

			partB = it->second;

			printf(" to address %i", partB);
		}

		// No more parameters needed
		return true;
	}
	}

	// Second parameter
	token = extractNextToken(line);

	switch (instr)
	{
	case LDR:
	{
		if (token == "rA")
			printf(" to reg. A");
		else if (token == "rB")
		{
			partA |= FLAG4;
			printf(" to reg. B");
		}
		else
			throw std::exception("LDR Invalid p2 (must be rA or rB)");
	} break;
	case STR:
	{
		if (token.size() != 3 || token.data()[0] != '$')
			throw std::exception("STR Invalid p2 (must be $XX)");

		token.erase(0, 1);

		try
		{
			partB = decodeHex(token);
		}
		catch (std::exception e)
		{
			throw std::exception("STR Invalid address (must be $XX)");
		}

		printf(" at address %s", token.c_str());

		// No more parameters needed
		return true;
	}
	case CPY:
	{
		if (token == "rA")
			printf(" to reg. A");
		else if (token == "rB")
			printf(" to reg. B");
		else
			throw std::exception("CPY Invalid p2 (must be rA or rB)");

		return true;
	}
	case SET:
	{
		if (token.size() != 4 || token.data()[0] != '0' || token.data()[1] != 'x')
			throw std::exception("SET Invalid p2 (must be 0xYY)");

		token.erase(0, 2);

		try
		{
			partB = decodeHex(token);
		}
		catch (std::exception e)
		{
			throw std::exception("STR Invalid address (must be 0xYY)");
		}

		printf(" to value %s", token.c_str());

		// No more parameters needed
		return true;
	}
	case ADD:
	case SUB:
		return true;
	}

	// Third parameter
	token = extractNextToken(line);

	switch (instr)
	{
	case LDR:
	{
		if (token.size() != 3 || token.data()[0] != '$')
			throw std::exception("LDR Invalid p3 (must be $XX)");

		token.erase(0, 1);

		try
		{
			partB = decodeHex(token);
		}
		catch (std::exception e)
		{
			throw std::exception("LDR Invalid address (must be $XX)");
		}

		printf(" at address %s", token.c_str());

		// No more parameters needed
		return true;
	}
	}
}

int main()
{
	eeprom = new int8_t[eepromSize];
	memset(eeprom, 0, eepromSize);

	// Read input and output filename
	std::string filename, filename2;
	printf("What is the input filename?\n");
	std::getline(std::cin, filename);

	printf("What is the output filename?\n");
	std::getline(std::cin, filename2);

	// Open input file
	std::ifstream inputFile = std::ifstream(filename.c_str());
	if (!inputFile.is_open())
	{
		printf("Error : Open input file\n");
		getchar();
		return -1;
	}

	// Open output file
	std::ofstream outputFile = std::ofstream(filename2.c_str(), std::ios::binary | std::ios::trunc);
	if (!outputFile.is_open())
	{
		printf("Error : Open output file\n");
		getchar();
		return -1;
	}

	// Decoding
	int lineCount = 0;
	std::string line;

	uint8_t partA, partB;

	size_t address = 0;

	while (!inputFile.eof())
	{
		// Get line
		lineCount++;
		std::getline(inputFile, line);

		if (line.empty())
			continue;

		if (line[0] == '%')
			continue;

		try
		{
			if (decodeInstruction(line, partA, partB, address))
			{
				eeprom[address++] = partA;
				eeprom[address++] = partB;
			}
			printf("\n");
		}
		catch (std::exception e)
		{
			printf("\nError at line %i : %s\n", lineCount, e.what());
		}
	}

	// Write to file
	// Address can be used to determine file size
	for (size_t i = 0; i < std::min(eepromSize, address); i++)
		outputFile.put(eeprom[i]);

	// Clean up
	inputFile.close();
	outputFile.close();
	if (eeprom != nullptr)
		delete[] eeprom;

	printf("\nFinished!\n");
	getchar();
}