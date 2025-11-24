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
4) extract
5) delete
6) list
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
    static const size_t def_buffer_size = 256;
    std::string name;
    std::string suffix_name;
    std::fstream stream;
    bool is_tmp;
    std::string fully_name;

    void Open(std::string name);
    void OpenWithApp(std::string name);
    void TryOpen(std::string name);
    public:
    File(std::string name, std::string suffix_name, bool is_tmp = 0);
    File(std::string fully_name, bool is_tmp = 0);
    File(File &&other);
    File &operator=(File &&other);

    void Close();
    bool IsOpen() const;
    bool IsEOF() const;
    size_t GCount() const;
    std::string GetName() const;
    std::string GetFullyName() const;
    std::string GetLongName() const;
    std::string GetFormat() const;
    std::string GetData(size_t size_get, size_t pos);
    std::string GetCurrentData(size_t size_get);
    std::streampos GetPos();
    void SetPos(size_t index);
    void NextPos(size_t next_pos);
    void Replace(size_t pos, const std::string &str, size_t size_replace);
    void PushBack(const std::string &str, size_t size_add);
    void PushBack(File &other, size_t size_buff = def_buffer_size);
    void ReplaceCurrentPos(const std::string &str, size_t size_replace);
    void Insert(size_t pos, const std::string &str, size_t size_insert, size_t size_buff = def_buffer_size);
    void DeletePos(size_t pos, size_t size_delete, size_t size_buff = def_buffer_size);
    void Delete();
    size_t GetSize() const;

    static File GetUniqueFile();

    ~File();
};

class Archive : public File {
  public:
    inline Archive(std::string name, std::string suff_name) : File(name, suff_name) {}
    inline Archive(std::string fully_name) : File(fully_name){}
    virtual void AddFile(File &file) = 0;
    virtual bool DeleteFile(const std::string &name) = 0;
    ~Archive() = default;
};

class Hamarc : public Archive {
    static const size_t countability_size = 1;
    static const size_t buff_size = (1 << (countability_size * 8)) - 1;
    static const size_t name_size = buff_size;
    static const size_t cnt_block_size = buff_size;
    static constexpr size_t chunk_size = buff_size + countability_size;

    size_t GetPosFinishName(std::string name, size_t startpos = 0);

    void NormalBlock(std::string &str) const;
    
    uint32_t GetCountabilityInfo(std::string data, size_t size_prefix) const;
    
    std::string DecodeBuff(const std::string &buff, size_t size_prefix);

    void EncodeBlock(std::string &str, size_t size_codec) const;

    File EncodeFile(File &file);

  public:
    Hamarc(std::string name);

    long long GetPosFile(std::string name);

    void AddFile(File &file);

    bool DeleteFile(const std::string &name);

    std::vector<std::pair<std::string, long long>> GetInfo();

    bool Extract(const std::string &name);

    void ExtractAll();

    void Merge(Hamarc &other);
};

} // namespace managefile

#endif