#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#define ADDRESS_RANGE 15

size_t eepromSize = std::pow((int)2, ADDRESS_RANGE);
int8_t* eeprom = nullptr;

void setValue(size_t address, int8_t value, std::vector<size_t> dontCareList)
{
	// Check for dont cares
	if (dontCareList.empty())
	{
		eeprom[address] = value;
	}
	else
	{
		// Take first dont care
		size_t bitPosition = dontCareList.back();
		dontCareList.pop_back();

		// Write to address with a 0 and 1 at defined bit position
		// With zero
		address &= ~(1 << bitPosition);
		setValue(address, value, dontCareList);

		// With one
		address |= 1 << bitPosition;
		setValue(address, value, dontCareList);
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

	while (!inputFile.eof())
	{
		// Get line
		lineCount++;
		std::getline(inputFile, line);

		if (line.empty())
			continue;

		if (line[0] == '%')
			continue;

		std::string leftSide, rightSide;

		size_t pos = line.find_first_of("=>");
		if (pos != std::string::npos)
		{
			// Seperate left from right side
			leftSide = line.substr(0, pos);
			rightSide = line.substr(pos + 2);

			size_t address = 0;
			uint8_t value = 0;

			// Decode address
			std::vector<size_t> dontCareList;

			// Parse string
			{
				size_t bitPosition = 0;
				for (size_t i = leftSide.size(); i > 0; i--)
				{
					char character = leftSide[i - 1];

					if (bitPosition >= ADDRESS_RANGE && (character == '0' || character == '1' || character == 'x'))
					{
						printf("Warning : Line %i has too many address bits\n", lineCount);
						break;
					}

					switch (character)
					{
					case '0':
						bitPosition++;
						break;
					case '1':
						address |= 1 << bitPosition;
						bitPosition++;
						break;
					case 'x':
						dontCareList.push_back(bitPosition);
						bitPosition++;
						break;
					case ' ':
					case '.':
						break;
					default:
						printf("Warning : Unknown symbol '%c' at line %i\n", character, lineCount);
						break;
					}
				}
			}

			// Decode value

			// Parse string
			{
				size_t bitPosition = 0;
				for (size_t i = rightSide.size(); i > 0; i--)
				{
					char character = rightSide[i - 1];

					if (bitPosition >= 8 && (character == '0' || character == '1'))
					{
						printf("Warning : Line %i has too many value bits\n", lineCount);
						break;
					}

					switch (character)
					{
					case '0':
						bitPosition++;
						break;
					case '1':
						value |= 1 << bitPosition;
						bitPosition++;
						break;
					case ' ':
					case '.':
						break;
					default:
						printf("Warning : Unknown symbol '%c' at line %i\n", character, lineCount);
						break;
					}
				}
			}

			// Set value
			setValue(address, value, dontCareList);
		}
	}

	// Write to file
	for (size_t i = 0; i < eepromSize; i++)
		outputFile.put(eeprom[i]);

	// Clean up
	inputFile.close();
	outputFile.close();
	if (eeprom != nullptr)
		delete[] eeprom;

	printf("\nFinished!\n");
	getchar();
}