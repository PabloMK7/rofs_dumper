#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <format>
#include <array>
#include <cstring>
#include <filesystem>

typedef uint8_t u8;   ///<  8-bit unsigned integer
typedef uint16_t u16; ///< 16-bit unsigned integer
typedef uint32_t u32; ///< 32-bit unsigned integer
typedef uint64_t u64; ///< 64-bit unsigned integer

typedef int8_t s8;   ///<  8-bit signed integer
typedef int16_t s16; ///< 16-bit signed integer
typedef int32_t s32; ///< 32-bit signed integer
typedef int64_t s64; ///< 64-bit signed integer

struct Header {
    std::array<u8, 4> magic;
    u32 unknown;
    u32 header_size;
    u32 fdi_offset;
    u32 fdi_offset_full;
    u32 fdi_size_bytes;
    u32 padding[2];
};

struct FSEntry {
    u32 entry_data_offset;
    u16 starting_fdi_index;
    u8 parent_or_count;
    u8 unk;
};

struct FileDataInfo {
    u32 start_offset;
    u32 end_offset;
};

void DumpFile(std::string out_folder, std::string filename, u8* data, u32 size) {
    if (!out_folder.ends_with('/') && !out_folder.ends_with('\\')) {
        out_folder.append("/");
    }

    std::filesystem::path file = out_folder + filename;
    if (!std::filesystem::exists(file.parent_path())) {
        std::filesystem::create_directories(file.parent_path());
    }

    std::ofstream fs(file, std::ios::out | std::ios::binary);
    fs.write(reinterpret_cast<char*>(data), size);
    fs.close();
}

void parseFS(FSEntry* fse_all, FileDataInfo* fdi_all, u8* file_data, u8* body, std::string& out_folder) {

    auto process_dir = [fse_all, fdi_all, file_data, body, out_folder](std::string path, FSEntry* fse_curr, FileDataInfo* fdi_curr, auto& process_dir) -> void {
        const u8* entry_data = body + fse_curr->entry_data_offset;
        while (true)
        {
            char name[256] = {0};
            u8 control_byte = *entry_data++;
            if (control_byte == 0) {
                break;
            }
            bool isDir = (control_byte & 0x80) != 0;
            u8 size = control_byte & 0x7F;
            
            strncpy(name, reinterpret_cast<const char*>(entry_data), size);
            if (isDir) {
                u16 entry_id = *reinterpret_cast<const u16*>(entry_data + size) & 0xFFF;
                FSEntry* next = fse_all + entry_id;
                process_dir(path + name + "/", next, fdi_all + next->starting_fdi_index, process_dir);
                entry_data += size + sizeof(u16);
            } else {
                std::string filename = path + name;
                entry_data += size;

                FileDataInfo* curr = fdi_curr++;
                u8* offset = file_data + curr->start_offset;
                u32 size = curr->end_offset - curr->start_offset;

                std::cout << filename << std::endl;
                DumpFile(out_folder, filename, offset, size);
            }
        }
    };

    // Start from root
    process_dir("", fse_all, fdi_all, process_dir);
};

int main(int argc, char* argv[])
{
    std::cout << "3DS ROFS Dumper v1.0" << std::endl;
    
    if (argc != 3) {
        std::cout << "Usage:" << std::endl;
        std::cout << std::format("\t{} (infile) (outfolder)", argv[0]) << std::endl;
        return 1;
    }
    
    // Just read the entire thing to memory to make things easy
    std::ifstream input( argv[1], std::ios::binary );
    std::vector<u8> file_buf(std::istreambuf_iterator<char>(input), {});
    input.close();

    if (file_buf.size() < sizeof(Header)) {
        std::cout << "File is too small" << std::endl;
        return 1;
    }

    Header* hdr = reinterpret_cast<Header*>(file_buf.data());
    u8* body = file_buf.data() + hdr->header_size;

    if (hdr->magic != std::array<u8, 4>({0x52, 0x4F, 0x46, 0x53})) {
        std::cout << "File is not ROFS" << std::endl;
        return 1;
    }

    // Assume everything is valid from now on
    FileDataInfo* fdi_all = reinterpret_cast<FileDataInfo*>(body + hdr->fdi_offset);
    FSEntry* fse_all = reinterpret_cast<FSEntry*>(body);

    std::string out_folder = argv[2];
    parseFS(fse_all, fdi_all, file_buf.data(), body, out_folder);

    return 0;
}