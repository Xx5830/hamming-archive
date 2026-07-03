#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>
#include <algorithm>

#ifndef INCLUDE_MANAGER_FILES
#define INCLUDE_MANAGER_FILES
namespace managefile {

class File {
    std::string directories_path;
    std::string name;
    std::string format;
    std::string file_path;
    std::fstream stream;
    bool delete_on_close;

    void Open(bool create_if_not_exists, bool need_clear_on_open);
    File &operator=(const File &other) = delete;
    File(const File &other) = delete;
  public:
    static const size_t DEFAULT_BUFFER_SIZE = 256;
    File();
    explicit File(const std::string &file_path, bool create_if_not_exists = true, bool delete_on_close = false,
                  bool need_clear_on_open = false);
    File(File &&other);
    File &operator=(File &&other);

    void Close();

    bool IsOpen() const;
    bool IsEOF() const;
    void ClearEOF();

    std::string Name() const;
    std::string Directories() const;
    std::string Format() const;
    std::string FilePath() const;
    ssize_t Pos();
    size_t GCount() const;
    size_t Length() const;

    std::vector<char> Read(size_t read_size);
    std::vector<char> ReadPos(size_t pos, size_t read_size);

    void Write(const std::span<char> data);
    void Write(File &other, size_t buff_size = DEFAULT_BUFFER_SIZE);
    void WritePos(size_t pos, const std::span<char> data);
    void WritePos(size_t pos, File &other, size_t buff_size = DEFAULT_BUFFER_SIZE);

    void SetPos(size_t pos);
    void SetBegin();
    void SetEnd();
    void MovePos(ssize_t move_size);

    void PushBack(const std::span<char> data, size_t buff_size = DEFAULT_BUFFER_SIZE);
    void PushBack(File &other, size_t buff_size = DEFAULT_BUFFER_SIZE);
    void Insert(size_t pos, const std::span<char> data, size_t buff_size = DEFAULT_BUFFER_SIZE);
    void Insert(size_t pos, File &other, size_t buff_size = DEFAULT_BUFFER_SIZE);

    void PopBack(size_t delete_size);
    void Erase(size_t pos, size_t delete_size, size_t buff_size = DEFAULT_BUFFER_SIZE);

    void Delete();

    static File CreateTempFile();

    ~File();
};

struct Archive {
    File file;

    virtual void Add(File &file) = 0;
    virtual void Delete(const std::string &name) = 0;
    virtual std::pair<File, bool> Get(const std::string &name) = 0;
    virtual size_t Length() const = 0;
};

// archive with code hemming
class Hamarc : public Archive {
    /*

    archive:
    BEGIN
    [(size file) encoding info][(size name file) encoding info][(name file) encoding info][(encoding data) encoding info]
    [(size file) encoding info][(size name file) encoding info][(name file) encoding info][(encoding data) encoding info]
    [(size file) encoding info][(size name file) encoding info][(name file) encoding info][(encoding data) encoding info]
    [(size file) encoding info][(size name file) encoding info][(name file) encoding info][(encoding data) encoding info]
    [(size file) encoding info][(size name file) encoding info][(name file) encoding info][(encoding data) encoding info]

    file(1):
    [file size][size file name][file name][file (encoding info) data]
    [                     data                    ]
    ...
    file(n):
    [file size][size file name][file name][file (encoding info) data]
    [                     data                    ]
    END

    *[encoding info] = [count data bit][count copy]

    */

  public:
    struct EncodingInfo {
        uint64_t count_data_bits; // 0 -> not encoding code hemming
        uint64_t count_data_bytes;
        uint64_t count_hemming_bits;
        uint64_t count_additional_bytes;
        uint8_t count_copy;

        EncodingInfo(uint64_t count_bit, uint32_t count_copy);
        EncodingInfo(const std::span<char> arr);
        EncodingInfo();
        uint32_t ToInt() const;
        bool operator ==(const EncodingInfo &other) const;
    };
    struct ConfigEncodingFile {
        EncodingInfo size_file;
        EncodingInfo size_name_file;
        EncodingInfo name_file;
        EncodingInfo encoding_data_file;

        bool operator ==(const ConfigEncodingFile &other) const;
        ConfigEncodingFile();
        ConfigEncodingFile(EncodingInfo size_file, EncodingInfo size_name_file, EncodingInfo name_file, EncodingInfo encoding_data_file);
        ConfigEncodingFile(const std::span<char> &arr);
    };
    struct Hat{
      uint64_t size_file;
      uint32_t size_name_file;
      std::string name_file;
      uint64_t encoding_data_file;

      Hat(std::span <char> size_file, std::span <char> size_name_file, std::span<char> name_file, std::span<char> encoding_data_file);
      Hat();
    };

  private:
    ConfigEncodingFile config;
    size_t count_copy_hat_archive = 5;
    size_t count_file;
    size_t begin_file_pos;

    std::vector<char> IntegralToVectorCHar(long long num, size_t count_byte) const;
    std::vector<uint8_t> HemmingBytes(const std::span<char> data, const EncodingInfo &encoding_info) const;
    
    std::vector<char> EncodeBlock(const std::span<char> data, const EncodingInfo & encoding_info) const;
    std::vector<char> EncodeData(const std::span<char> data, const EncodingInfo & encoding_info) const;
    std::vector<char> EncodeHat(Hat hat) const;
    File EncodeFile(File &file, const EncodingInfo & encoding_data, size_t buff_size = File::DEFAULT_BUFFER_SIZE) const;

    std::vector<char> DecodeBlock(const std::span<char> block, const EncodingInfo & encoding_info) const;
    std::vector<char> DecodeData(std::span<char> data, const EncodingInfo & encoding_info);
    std::vector<char> DecodePosBlock(size_t pos, const EncodingInfo & encoding_info);
    Hat DecodeHat(const size_t pos);
    File DecodeFile(size_t pos, const Hat &hat, size_t buff_size = File::DEFAULT_BUFFER_SIZE);

    size_t GetSizeFile(const Hat &hat) const;
    size_t GetPosFirstFile();

  public:
    Hamarc(const std::string &file_path, bool create_if_not_exist, const ConfigEncodingFile &config);

    ConfigEncodingFile GetConfig();
    std::pair<File, bool> Get(const std::string &name);
    ssize_t GetPos(const std::string &name);
    
    void Add(File &file);
    void Add(File &file, const EncodingInfo &encoding_info);
    
    void Delete(const std::string &name);

    size_t Length() const;

    std::vector<std::pair<std::string, size_t>> Info();

    std::vector<File> GetAll();

    bool Merge(Hamarc &other);
};

} // namespace managefile

#endif