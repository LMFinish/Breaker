#include <filesystem>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <iomanip>

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_opengl3.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include "Breaker.hpp"
#include "L2DFileDialog.h"

static const ImWchar latinRanges[] = {
    0x0020, 0x00FF, // Basic Latin + Latin-1 Supplement
    0,
};

void Breaker::Centered_Text(const char* text)
{
    float window_width = ImGui::GetWindowSize().x;
    float text_width = ImGui::CalcTextSize(text).x;
    float window_height = ImGui::GetWindowSize().y;
    float text_height = ImGui::CalcTextSize(text).y;

    ImGui::SetCursorPosX((window_width - text_width) * 0.5f);
    ImGui::SetCursorPosY((window_height - text_height) * 0.5f);
    ImGui::TextUnformatted(text);
}

void Breaker::Horizontal_Centered_Text(const char* text)
{
    float window_width = ImGui::GetWindowSize().x;
    float text_width = ImGui::CalcTextSize(text).x;

    ImGui::SetCursorPosX((window_width - text_width) * 0.5f);
    ImGui::TextUnformatted(text);
}

unsigned int Breaker::Read_Big_Endian_32(FILE* file) {

    unsigned char b[4];
    fread(b, 1, 4, file);
    return (uint32_t(b[0]) << 24) |
        (uint32_t(b[1]) << 16) |
        (uint32_t(b[2]) << 8) |
        (uint32_t(b[3]));
}

auto Breaker::Write_Big_Endian_32(FILE* f, uint32_t v) {

    unsigned char b[4] = { 0 };
    b[0] = static_cast<unsigned char>((v >> 24) & 0xFF);
    b[1] = static_cast<unsigned char>((v >> 16) & 0xFF);
    b[2] = static_cast<unsigned char>((v >> 8) & 0xFF);
    b[3] = static_cast<unsigned char>((v) & 0xFF);
    fwrite(b, 1, 4, f);
}

void Breaker::Reset() {

    /* Clear all parameter binary data and UI information upon new load */
    Breaker::gParameter_Properties.clear();
    Breaker::gEditableHex.clear();
    Breaker::gLastValidHex.clear();
    Breaker::gEditError.clear();
    Breaker::gSelected_Property = -1;
}

static std::string FormatBytesToHexString(const std::vector<uint8_t>& bytes)
{
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0');
    bool first = true;
    for (uint8_t b : bytes) {
        if (!first) oss << ' ';
        oss << std::setw(2) << static_cast<int>(b);
        first = false;
    }
    return oss.str();
}

static std::string NormalizeHexString(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (std::isxdigit(static_cast<unsigned char>(c))) out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return out;
}

static bool HexStringToBytes(const std::string& hexNormalized, std::vector<uint8_t>& outBytes)
{
    outBytes.clear();
    if ((hexNormalized.size() % 2) != 0) return false;
    size_t pairs = hexNormalized.size() / 2;
    outBytes.reserve(pairs);
    for (size_t i = 0; i < pairs; ++i) {
        std::string pair = hexNormalized.substr(i * 2, 2);
        char* endptr = nullptr;
        long val = strtol(pair.c_str(), &endptr, 16);
        if (endptr == pair.c_str() || *endptr != '\0' || val < 0 || val > 0xFF) return false;
        outBytes.push_back(static_cast<uint8_t>(val));
    }
    return true;
}

int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) return -1;

    SDL_Window* window = SDL_CreateWindow(
        "The Breaker Room",
        Breaker::gWindow_Sizes[Breaker::gWindow_Size][0], Breaker::gWindow_Sizes[Breaker::gWindow_Size][1],
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );

    if (!window) return -1;

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);

	// Try adaptive VSync first, then fall back to standard VSync
	if (!SDL_GL_SetSwapInterval(-1)) SDL_GL_SetSwapInterval(1);

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    // Initialize SDL3 backend
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    bool running = true;
    SDL_Event event;

    /* We are hasty for time being */
    ImGui::GetIO().IniFilename = NULL;
    io.Fonts->AddFontFromFileTTF("Assets/SegoeUI.ttf", 20.5f, NULL, latinRanges);
    io.Fonts->Build();

    while (running)
    {
        while (SDL_PollEvent(&event)) {

            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) running = false;

            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
            running = false;
        }

        // New ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        FILE* Parameter_File = NULL;
        FILE* Hash_Table = NULL;
		FILE* Name_Table = NULL;
        FILE* Property_Description = NULL;

        static char* File_Dialog_Path = NULL;

        ImFontConfig cfg;
        cfg.OversampleH = 10;
        cfg.OversampleV = 10;
        ImGui::SetFontRasterizerDensity(100.0f);

        ////////// Menu Bar //////////
        if (ImGui::BeginMainMenuBar()) 
        {
            if (ImGui::BeginMenu("File")) {

                if (ImGui::MenuItem("Open")) {

                    File_Dialog_Path = Breaker::gParameter_File_Path;
                    FileDialog::file_dialog_open = true;
                    FileDialog::file_dialog_open_type = FileDialog::FileDialogType::OpenFile;
                }

                if (ImGui::MenuItem("Save")) {

                    if (Breaker::gParameter_Properties.size() <= 0) std::cout << "There is nothing to save!" << std::endl;
                    
                    else {

                        // Read the external hash/name tables so we can write corresponding hashes for each property name
                        fopen_s(&Hash_Table, "Assets/GLM/PropertyHashTable.bin", "rb");
                        fopen_s(&Name_Table, "Assets/GLM/PropertyNameTable.txt", "r");

                        if (!Hash_Table || !Name_Table) {
                            std::cout << "Unable to open hash/name tables required for saving." << std::endl;
                            if (Parameter_File) fclose(Parameter_File);
                            if (Hash_Table) fclose(Hash_Table);
                            if (Name_Table) fclose(Name_Table);
                        }
                        else {
                            // Load hashList
                            std::vector<uint32_t> hashList;
                            fseek(Hash_Table, 0, SEEK_END);
                            long hashSize = ftell(Hash_Table);
                            fseek(Hash_Table, 0, SEEK_SET);
                            int hashCount = (hashSize > 0) ? (int)(hashSize / 4) : 0;
                            hashList.resize(hashCount);
                            for (int i = 0; i < hashCount; ++i) {
                                hashList[i] = Breaker::Read_Big_Endian_32(Hash_Table);
                            }

                            // Load nameList (whitespace separated)
                            std::vector<std::string> nameList;
                            {
                                fseek(Name_Table, 0, SEEK_END);
                                long len = ftell(Name_Table);
                                rewind(Name_Table);

                                if (len > 0) {

                                    std::string buffer;
                                    buffer.resize(len);
                                    fread(&buffer[0], 1, len, Name_Table);

                                    std::stringstream ss(buffer);
                                    std::string word;

                                    while (ss >> word) nameList.push_back(word);
                                }
                            }

                            fclose(Hash_Table);
                            fclose(Name_Table);

                            // Create temp file and write new parameter file contents
                            std::string originalPath = Breaker::gParameter_File_Path ? Breaker::gParameter_File_Path : "";
                            std::string tmpPath = originalPath + ".tmp";

                            FILE* out = nullptr;
                            fopen_s(&out, tmpPath.c_str(), "wb");
                            if (!out) {
                                std::cout << "Unable to create temp file for saving." << std::endl;
                            }
                            else {
                               
                                uint32_t entryCount = static_cast<uint32_t>(Breaker::gParameter_Properties.size());
                                Breaker::Write_Big_Endian_32(out, entryCount);

                                for (uint32_t i = 0; i < entryCount; ++i) {
                                    const auto& p = Breaker::gParameter_Properties[i];

                                    // Find hash for this property name in the loaded nameList
                                    int foundIndex = -1;
                                    for (size_t j = 0; j < nameList.size(); ++j) {
                                        if (nameList[j] == p.name) { foundIndex = static_cast<int>(j); break; }
                                    }

                                    uint32_t hashToWrite = 0;
                                    if (foundIndex >= 0 && foundIndex < (int)hashList.size()) hashToWrite = hashList[foundIndex];

                                    // Write hash
                                    Breaker::Write_Big_Endian_32(out, hashToWrite);

                                    // Write property name raw bytes (no null terminator)
                                    if (!p.name.empty()) fwrite(p.name.c_str(), 1, p.name.size(), out);

                                    // Write size and value
                                    uint32_t valSize = static_cast<uint32_t>(p.value.size());
                                    Breaker::Write_Big_Endian_32(out, valSize);
                                    if (valSize > 0) fwrite(p.value.data(), 1, valSize, out);
                                }

                                fclose(out);

                                // Replace original file with temp file atomically if possible
                                std::error_code ec;
                                std::filesystem::rename(tmpPath, originalPath, ec);

                                if (ec) {

                                    // If rename failed (maybe target exists), try remove + rename
                                    std::filesystem::remove(originalPath, ec);
                                    std::filesystem::rename(tmpPath, originalPath, ec);

                                    if (ec) std::cout << "Failed to write original file: " << ec.message() << std::endl;
                                    else std::cout << "Saved parameter file to: " << originalPath << std::endl;
                                }
                                else {
                                    std::cout << "Saved parameter file to: " << originalPath << std::endl;
                                }
                            }
                        }
                    }
                }

                if (ImGui::MenuItem("Close")) {
					if (Breaker::gParameter_Properties.size() > 0) {

                        Breaker::Reset();
                        std::cout << "Closed parameter file information.\n" << std::endl;
					}
					
                    else std::cout << "There is nothing to close!\n" << std::endl;
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Exit")) running = false;
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {

                if (ImGui::MenuItem("About")) {
                    Breaker::gIs_About_Window = true;
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

		////////// File Dialog //////////
        if (FileDialog::file_dialog_open) {
            FileDialog::ShowFileDialog_s(&FileDialog::file_dialog_open, File_Dialog_Path, Breaker::gParameter_File_Type);
        }

        ////////// File Stuff //////////
        if (FileDialog::file_open == true) {
            
            /* Clear all parameter binary data and UI information upon new load */
            Breaker::Reset();

            fopen_s(&Parameter_File, Breaker::gParameter_File_Path, "rb");
            if (Parameter_File) std::cout << "Opened file: " << Breaker::gParameter_File_Path << std::endl;
            else {
                
                std::cout << "Failed to open file: " << Breaker::gParameter_File_Path << std::endl;
                return -1;
			}

			fopen_s(&Hash_Table, "Assets/GLM/PropertyHashTable.bin", "rb");
			fopen_s(&Name_Table, "Assets/GLM/PropertyNameTable.txt", "r");

            if (Hash_Table) std::cout << "Temporary restriction: Assets/GLM/PropertyHashTable.bin" << std::endl;
			else {
                std::cout << "Failed to open file: Assets/GLM/PropertyHashTable.bin" << std::endl;
				fclose(Parameter_File);
                return -1;
			}

			if (Name_Table) std::cout << "Temporary restriction: Assets/GLM/PropertyNameTable.txt" << std::endl;
            else {

                std::cout << "Failed to open file: Assets/GLM/PropertyNameTable.txt" << std::endl;
                fclose(Parameter_File);
                fclose(Hash_Table);
                return -1;
            }

            ///// Read Hash Table /////
            std::vector<unsigned int> hashList;

            fseek(Hash_Table, 0, SEEK_END);
            long hashSize = ftell(Hash_Table);
            fseek(Hash_Table, 0, SEEK_SET);

            int hashCount = hashSize / 4;
            hashList.resize(hashCount);

            for (int i = 0; i < hashCount; i++)
                hashList[i] = Breaker::Read_Big_Endian_32(Hash_Table);

			///// Read Name Table /////
            std::vector<std::string> nameList;
            {
                fseek(Name_Table, 0, SEEK_END);
                long len = ftell(Name_Table);
                rewind(Name_Table);

                std::string buffer;
                buffer.resize(len);
                fread(&buffer[0], 1, len, Name_Table);

                std::stringstream ss(buffer);
                std::string word;
                while (ss >> word) nameList.push_back(word);
            }

            fclose(Name_Table);
			fclose(Hash_Table);

			std::cout << "Name List Count: " << nameList.size() << ", Hash List Count: " << hashList.size() << ". This is a work-in-progress." << std::endl;

            // This should never happen
            if (nameList.size() != hashList.size()) {

                std::cout << "Hash table and name list count mismatch!\n" << std::endl;
				fclose(Parameter_File);
				fclose(Name_Table);
				fclose(Hash_Table);
				return -1;
            }

            // Get parameter file entry count, then move to entry data
                uint32_t entryCount = Breaker::Read_Big_Endian_32(Parameter_File); 
                fseek(Parameter_File, 4, SEEK_SET);    

                Breaker::gParameter_Properties.reserve(entryCount);
                
                for (uint32_t i = 0; i < entryCount; i++)
                {
                    uint32_t hashBE = Breaker::Read_Big_Endian_32(Parameter_File);

                    // Lookup hash in hash table
                    int id = -1;
                    for (int j = 0; j < (int)hashList.size(); j++)
                    {
                        if (hashList[j] == hashBE) {
                            id = j;
                            break;
                        }
                    }

                    std::string propName = (id >= 0 && id < (int)nameList.size()) ? nameList[id] : "<UNKNOWN>";
                    int nameLen = (int)propName.size();

                    fseek(Parameter_File, nameLen, SEEK_CUR);
                     
                    // Get value size and then read value itself into vector (both 04 and 02 exist)
                    uint32_t size = Breaker::Read_Big_Endian_32(Parameter_File);
                    uint32_t value = Breaker::Read_Big_Endian_32(Parameter_File);
                    fseek(Parameter_File, -4, SEEK_CUR);

                    std::vector<uint8_t> val(size);
                    fread(val.data(), 1, size, Parameter_File);

                    Breaker::gParameter_Properties.push_back({ propName, val });
                }

                fclose(Parameter_File);
				FileDialog::file_open = false;
            }
        
		////////// Main Windows //////////
        ImGui::SetNextWindowPos(ImVec2(-1, 25), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(402, 372), ImGuiCond_Always);
        ImGui::Begin("Breaker Panel", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

        if (Breaker::gParameter_Properties.size() <= 0) ImGui::TextWrapped("Once a parameter file has been opened, its property list will appear here.");

        else {

            ImGui::BeginChild("left pane", ImVec2(ImGui::GetWindowSize().x - 16.0f, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);

            for (size_t i = 0; i < Breaker::gParameter_Properties.size(); i++) {
            
                const auto& p = Breaker::gParameter_Properties[i];

                char PropertyName[128];
                sprintf_s(PropertyName, "%s", p.name.c_str());

                ImGui::PushID(i);

                if (ImGui::Selectable(PropertyName, Breaker::gSelected_Property == i, ImGuiSelectableFlags_SelectOnNav)) Breaker::gSelected_Property = i;

                ImGui::PopID();
            }

            ImGui::EndChild();
        }

        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(-1, 396), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(802, 205), ImGuiCond_Always);
        ImGui::Begin("Breaker Switch Description", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);

        if (Breaker::gSelected_Property == -1) Breaker::Centered_Text("Once an opened parameter file's property is chosen, its description will appear here.");

        else {
            
            const auto& p = Breaker::gParameter_Properties[Breaker::gSelected_Property];
            char PropertyName[128];

            // Temporary restriction
            sprintf_s(PropertyName, "Assets/GLM/Descriptions/%s.txt", p.name.c_str());

            fopen_s(&Property_Description, PropertyName, "r");
            if (Property_Description == NULL) Breaker::Centered_Text("No description available for this property.");

            else {

                // Read entire description file into a string and render it centered.
                fseek(Property_Description, 0, SEEK_END);
                long len = ftell(Property_Description);

                if (len <= 0) {

                    // Empty file or error determining size
                    Breaker::Centered_Text("No description available for this property.");
                }

                else {
                    rewind(Property_Description);

                    std::string desc;
                    desc.resize(static_cast<size_t>(len));

                    size_t read = fread(&desc[0], 1, static_cast<size_t>(len), Property_Description);
                    if (read == 0) {
                        Breaker::Centered_Text("No description available for this property.");
                    }

                    else {
                        // Ensure the string is the actual read length (in case of partial read)
                        if (read < desc.size()) desc.resize(read);

                        // Render the full file text centered in the window
                        Breaker::Centered_Text(desc.c_str());
                    }
                }

                fclose(Property_Description);
            }
        }

        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(399, 25), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(402, 372), ImGuiCond_Always);
        ImGui::Begin("Breaker Switch Control", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

        if (Breaker::gSelected_Property == -1) ImGui::TextWrapped("Once an opened parameter file's property is chosen, you can edit it here.");

        else {

            auto& p = Breaker::gParameter_Properties[Breaker::gSelected_Property];

            // Ensure editable hex initialized for this property
            int idx = Breaker::gSelected_Property;
            if (Breaker::gEditableHex.find(idx) == Breaker::gEditableHex.end()) {

                std::string initHex = FormatBytesToHexString(p.value);
                Breaker::gEditableHex[idx] = initHex;
                Breaker::gLastValidHex[idx] = initHex;
                Breaker::gEditError[idx].clear();
            }

            // Prepare local input buffer for ImGui (sized to a reasonable maximum)
            const size_t BUF_SIZE = 8192;
            char buf[BUF_SIZE];
            memset(buf, 0, BUF_SIZE);
            std::string& editable = Breaker::gEditableHex[idx];

            // copy up to BUF_SIZE-1 characters
            size_t copyLen = std::min(editable.size(), BUF_SIZE - 1);
            memcpy(buf, editable.c_str(), copyLen);

            // Show raw value as hex in editable text box
            // Label uses a hidden unique id to avoid duplicates
            std::string label = "##value" + std::to_string(idx);
            bool changed = ImGui::InputText(label.c_str(), buf, BUF_SIZE);

            if (changed) {
                // Update editable string from buffer
                editable.assign(buf);

                // Normalize input: remove non-hex characters
                std::string normalized = NormalizeHexString(editable);
                std::vector<uint8_t> parsed;
                bool ok = HexStringToBytes(normalized, parsed);

                if (!ok) {
                    // Invalid hex (odd length or invalid chars)
                    Breaker::gEditError[idx] = "Invalid input";
                    // Revert to last valid visually and keep last valid in editable
                    editable = Breaker::gLastValidHex[idx];
                }
                else {
                    // Validate not below property's size (per requirement)
                    if (parsed.size() < p.value.size()) {

                        std::ostringstream oss;
                        oss << "Input must be at least " << p.value.size() << " bytes";
                        Breaker::gEditError[idx] = oss.str();

                        // Revert to last valid
                        editable = Breaker::gLastValidHex[idx];
                    }
                    else {

                        // Accept change: apply parsed bytes to property
                        p.value = parsed;

                        // Reformat editable to canonical spaced uppercase hex and record as last valid
                        std::string canonical = FormatBytesToHexString(p.value);
                        Breaker::gEditableHex[idx] = canonical;
                        Breaker::gLastValidHex[idx] = canonical;
                        Breaker::gEditError[idx].clear();
                    }
                }
            }

            // Display any error message for this property
            const std::string& err = Breaker::gEditError[idx];
            if (!err.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", err.c_str());
            }
        }

        ImGui::End();

        // ImGui::ShowDemoWindow();

        if (Breaker::gIs_About_Window) {

            ImGui::Begin("About", &Breaker::gIs_About_Window, ImGuiWindowFlags_AlwaysAutoResize);

            Breaker::Horizontal_Centered_Text("Breaker 0.1 Alpha");
            Breaker::Horizontal_Centered_Text("2025 Kevin Andrade @ LMFinish");
            ImGui::Spacing();
            ImGui::Separator();

            ImGui::Spacing();
            ImGui::TextUnformatted("A tool to edit a parameter file format available in multiple");
            Breaker::Horizontal_Centered_Text("Nintendo GameCube JSystem-based games.");

            ImGui::Spacing();
            ImGui::End();
        }

        ///// Rendering /////
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

	///// Cleanup /////
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
