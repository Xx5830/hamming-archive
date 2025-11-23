#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#ifndef INCLUDE_MANAGER_FILES
#define INCLUDE_MANAGER_FILES
namespace managefile {

// file - need as a rule

/*order operations:
1) create
2) append
3) concatenate
4) list
5) delete
6) extract
*/

/*
format:

4 byte = buffer_size

2 byte name
4 byte = count blocks
..
..
4 byte hafman bits

2 byte name
4 byte count blocks;
..
..
4 byte hafman bits
*/

class File {
  protected:
    static const size_t def_buffer_size = 0;
    std::string name;
    std::string suffix_name;
    std::fstream stream;
    bool is_tmp;
    std::string fully_name;

  public:
    File(std::string name, std::string suffix_name, bool is_tmp = 0) {
        this->name = name;
        this->suffix_name = suffix_name;
        this->fully_name = this->name + "." + this->suffix_name;
        this->is_tmp = is_tmp;

        stream = std::fstream{fully_name, std::ios::in | std::ios::out | std::ios::binary};
    };
    File(std::string fully_name, bool is_tmp = 0){
        int32_t index_point = 0;
        for (uint32_t index = 0; index < fully_name.size(); index++){
            if (fully_name[index] == '.'){
                index_point = index;
            }
        }

        std::string name, suffix_name;
        for (uint32_t index = 0; index < index_point; index++){
            name += fully_name[index];
        }
        for (uint32_t index = index_point + 1; index < fully_name.size(); index++){
            suffix_name += fully_name[index];
        }

        File(name, suffix_name, is_tmp);
    }
    File(File &&other) { *this = std::move(other); }
    File &operator=(File &&other) {
        if (IsOpen()) {
            stream.close();
        }
        if (other.IsOpen()) {
            other.stream.close();
        }

        this->name = other.name;
        this->suffix_name = other.suffix_name;
        this->fully_name = other.fully_name;
        stream.open(fully_name);
        this->is_tmp = other.is_tmp;
        other.is_tmp = 0;

        return *this;
    }

    bool IsOpen() const { return stream.is_open(); }
    bool IsEOF() const { return stream.eof(); }
    size_t GCount() const { return stream.gcount(); }
    std::string GetName() const { return name; }
    std::string GetShortName() const {
        std::string name = GetName();
        std::string short_name;
        for (int index = name.size() - 1; index >= 0 && name[index] != '/'; index--) {
            short_name += name[index];
        }
        std::reverse(short_name.begin(), short_name.end());

        return short_name;
    }
    std::string GetFormat() const { return suffix_name; }
    std::string GetData(size_t size_get, size_t pos) {
        if (stream.eof()) {
            stream.clear();
        }

        stream.seekg(pos, std::ios::beg);
        std::string current(size_get, '0');
        stream.read(&current[0], size_get);
        return current;
    };
    std::string GetCurrentData(size_t size_get) {
        std::string current(size_get, '0');
        stream.read(&current[0], size_get);
        return current;
    }
    std::streampos GetPos() { return stream.tellg(); }
    void SetPos(size_t index) {
        if (stream.eof()) {
            stream.clear();
        }
        stream.seekg(index, std::ios::beg);
    }
    void NextPos(size_t next_pos) { SetPos(GetPos() + next_pos); }
    void Replace(size_t pos, const std::string &str, size_t size_replace) {
        if (stream.eof()) {
            stream.clear();
        }

        stream.seekg(pos, std::ios::beg);
        stream.write(&str[0], size_replace);
    }
    void PushBack(const std::string &str, size_t size_add) {
        if (stream.eof()) {
            stream.clear();
        }

        stream.seekg(0, std::ios::end);
        stream.write(&str[0], size_add);
    }
    void PushBack(File &other, size_t size_buff = def_buffer_size) {
        std::string buff(size_buff, '0');
        other.SetPos(0);

        while (!other.IsEOF()) {
            buff = other.GetCurrentData(size_buff);
            PushBack(buff, other.GCount());
        }
    }
    void ReplaceCurrentPos(const std::string &str, size_t size_replace) {
        if (stream.eof()) {
            stream.clear();
        }

        stream.write(&str[0], size_replace);
    }
    void Insert(size_t pos, const std::string &str, size_t size_insert, size_t size_buff = def_buffer_size) {
        if (stream.eof()) {
            stream.clear();
        }

        File current_file = GetUniqueFile();
        stream.seekg(pos, std::ios::beg);
        std::string buff(size_buff, '0');
        while (!stream.eof()) {
            stream.read(&buff[0], size_buff);
            size_t count_symbol = stream.gcount();
            current_file.PushBack(buff, count_symbol);
        }

        Replace(pos, str, size_insert);
        current_file.SetPos(0);

        while (!current_file.IsEOF()) {
            buff = current_file.GetCurrentData(size_buff);
            ReplaceCurrentPos(buff, current_file.GCount());
        }

        current_file.DeleteFile();
    }
    void DeletePos(size_t pos, size_t size_delete, size_t size_buff = def_buffer_size) {
        if (stream.eof()) {
            stream.clear();
        }

        File current_file = GetUniqueFile();
        std::string buff(size_buff, '0');
        while (!stream.eof()) {
            stream.read(&buff[0], size_buff);
            size_t count_symbol = stream.gcount();
            current_file.PushBack(buff, count_symbol);
        }

        current_file.SetPos(0);
        stream.close();
        stream.open(name + "." + suffix_name, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);

        PushBack(current_file, size_buff);

        current_file.DeleteFile();
    }
    void DeleteFile() { std::remove((name + "." + suffix_name).c_str()); }
    size_t GetSize() const { return std::filesystem::file_size(fully_name); }

    static File GetUniqueFile() {
        std::string name = "current";
        std::string result_name = name + ".txt";

        size_t number = 1;
        while (std::filesystem::exists(result_name)) {
            result_name = name + std::to_string(number) + ".txt";
        }

        File result(name + std::to_string(number), "txt");

        return result;
    }

    ~File() {
        if (is_tmp) {
            DeleteFile();
        }

        stream.close();
    }
};

class Archive : public File {
  public:
    Archive(std::string name, std::string suff_name) : File(name, suff_name) {}
    virtual bool Add(File &file) const = 0;
    virtual bool Delete(const std::string &name) const = 0;
    ~Archive() = default;
};

class TextFile : public File {};

class Hamarc : public Archive {
    static const size_t countability_size = 1;
    static const size_t buff_size = (1 << (countability_size * 8)) - 1;
    static const size_t name_size = buff_size;
    static const size_t cnt_block_size = buff_size;
    static constexpr size_t chunk_size = buff_size + countability_size;

    void NormalBlock(std::string &str) const {
        while (str.size() < buff_size) {
            str += '\0';
        }
    }
    
    std::vector<int> GetCountabilityInfo(std::string data, size_t size) const {
        std::vector <int> table(8, 0);
        size_t p = 0;
        for (size_t index = 0; index < size && index < buff_size; index++) {
            for (size_t bit = 0; bit < 8; bit++) {
                size_t pos = index * 8 + bit + p;
                while (pos == (1 << p)) {
                    ++p;
                    ++pos;
                }

                if (data[index] & (1 << bit)) {
                    for (size_t bit2 = 0; bit2 < 8; bit2++) {
                        if (pos & (1 << bit2)) {
                            table[bit2] = 1 - table[bit2];
                        }
                    }
                }
            }
        }

        return table;
    }
    
    std::string DecodeBuff(const std::string &buff, bool is_metainfo = 1) {
        std::string current = buff;
        for (uint32_t index = 0; is_metainfo && index < name.size(); index++) {
            if (name[index] == '\0') {
                current = buff.substr(0, index);
                break;
            }
        }

        

        return current;
    }

    void EncodeBlock(std::string &str) const {
        NormalBlock(str);

        std::vector <int> table = GetCountabilityInfo(str, str.size());

        uint32_t cu = 0;
        for (uint32_t index = 0; index < 8; index++){
            cu <<= 1;
            cu += table[index];
        }
        str += std::to_string(cu);
    }

    File EncodeFile(const File &file) {
        File tmp_file(file.GetName() + ".tmp", file.GetFormat(), true);
        std::string short_name = file.GetShortName();
        short_name += "." + file.GetFormat();

        EncodeBlock(short_name);
        tmp_file.PushBack(short_name, buff_size);

        size_t file_size = File::GetSize();
        size_t cnt_block = file_size / cnt_block_size + (file_size % cnt_block_size != 0 ? 1 : 0);

        std::string cnt_block_str = std::to_string(cnt_block);
        EncodeBlock(cnt_block_str);
        tmp_file.PushBack(cnt_block_str, cnt_block_str.size());

        SetPos(0);
        for (uint32_t index = 0; index < cnt_block; index++) {
            std::string current_str;
            current_str = GetCurrentData(buff_size);

            EncodeBlock(current_str);
            tmp_file.PushBack(current_str, current_str.size());
        }

        return tmp_file;
    }

  public:
    Hamarc(std::string name) : Archive(name, "haf") {}

    long long GetPosFile(std::string name) {
        SetPos(0);
        while (!IsEOF()) {
            std::string current = GetCurrentData(name_size);
            current = DecodeBuff(current);

            if (current == name) {
                return GetPos() - name_size;
            }

            std::string cnt_block_str = GetCurrentData(name_size);
            cnt_block_str = DecodeBuff(cnt_block_str);
            size_t cnt_block = atoll(cnt_block_str.c_str());
            size_t cnt_symbol = cnt_block * chunk_size;

            NextPos(cnt_symbol);
        }

        return -1;
    }

    void Add(File &file) {
        File tmp_file = EncodeFile(file);
        PushBack(tmp_file);
    }

    bool Delete(const std::string &name) {
        long long pos = GetPosFile(name);

        if (pos == -1) {
            return false;
        }
        std::string count_block_str = GetCurrentData(cnt_block_size);
        count_block_str = DecodeBuff(count_block_str);
        long long count_block = atoll(count_block_str.c_str());
        long long size_delete_field = count_block * chunk_size;
        DeletePos(pos, size_delete_field);

        return true;
    }

    std::vector<std::pair<std::string, long long>> GetInfo() {
        SetPos(0);

        std::vector<std::pair<std::string, long long>> result;
        while (!IsEOF()){
            std::string current = GetCurrentData(name_size);
            current = DecodeBuff(current);
            std::string count_block_str = GetCurrentData(cnt_block_size);
            count_block_str = DecodeBuff(count_block_str);
            long long count_block = atoll(count_block_str.c_str());

            result.push_back({current, count_block});

            NextPos(count_block * chunk_size);
        }

        return result;
    }

    bool Extract(const std::string &name){
        long long pos = GetPosFile(name);

        if (pos == -1){
            return false;
        }

        std::string file_name = GetCurrentData(name_size);
        file_name = DecodeBuff(file_name);

        std::string count_block_str = GetCurrentData(cnt_block_size);
        count_block_str = DecodeBuff(count_block_str);
        long long count_block = atoll(count_block_str.c_str());

        File now_file(file_name);

        for (uint64_t index_block = 0; index_block < count_block; index_block++){
            std::string data = GetCurrentData(chunk_size);
            data = DecodeBuff(data, 0);
            
        }

        return true;
    }
};

struct ManagerHaf {
    std::fstream archive;

    ManagerHaf(std::string &path);
    void AddFile(std::ifstream &in);
    void AddArchive(std::ifstream &in);
    std::vector<std::string> GetNameFiles();
    void Erase(std::string &name);
    void Extract(std::string &name);
};

} // namespace managefile

#endif