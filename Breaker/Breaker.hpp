#include <stdbool.h>
#include <string>
#include <vector>

namespace Breaker {

	void Centered_Text(const char* text);
	void Horizontal_Centered_Text(const char* text);

	unsigned int Read_Big_Endian_32(FILE* file);
	auto Write_Big_Endian_32(FILE* f, uint32_t v);

	void Reset();

	struct Property {

		uint32_t hash;
		std::string name;
		uint32_t size;
		std::vector<unsigned char> value;
	};

	struct ToolProperty {
		
		std::string name;
		std::vector<unsigned char> value;
	};

	enum class gGames { GLM, GMS };
	unsigned int gGame = static_cast<unsigned int>(gGames::GLM);

	static bool gIs_About_Window = false;
	static const unsigned int gWindow_Sizes[3][2] = {
		{ 800, 600 },
		{ 1024, 768 },
		{ 1280, 960 },
	};

	static unsigned int gWindow_Size = 0;

	static char gParameter_File_Path[512] = "";
	std::string gParameter_File_Type = ".prm";
	std::vector<Breaker::ToolProperty> gParameter_Properties;

	// Editable hex state per property index
	static std::unordered_map<int, std::string> gEditableHex;
	static std::unordered_map<int, std::string> gEditError;
	static std::unordered_map<int, std::string> gLastValidHex;

	static size_t gSelected_Property = -1;
}