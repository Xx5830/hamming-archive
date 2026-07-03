#pragma once

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>
#include <algorithm>

namespace managefile {

class File {
    std::string directories_path_;
    std::string name_;
    std::string format_;
    std::string file_path_;
    std::fstream stream_;
    bool delete_on_close_;

    void Open(bool create_if_not_exists, bool need_clear_on_open);
    File& operator=(const File& other) = delete;
    File(const File& other) = delete;

  public:
    static const size_t kDefaultBufferSize = 256;

    File();
    explicit File(const std::string& file_path, bool create_if_not_exists = true,
                  bool delete_on_close = false, bool need_clear_on_open = false);
    File(File&& other);
    File& operator=(File&& other);

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

    void MakeTemp();
    void UnMakeTemp();

    std::vector<char> Read(size_t read_size);
    std::vector<char> ReadPos(size_t pos, size_t read_size);

    void Write(std::span<const char> data);
    void Write(File& other, size_t buff_size = kDefaultBufferSize);
    void WritePos(size_t pos, std::span<const char> data);
    void WritePos(size_t pos, File& other, size_t buff_size = kDefaultBufferSize);

    void SetPos(size_t pos);
    void SetBegin();
    void SetEnd();
    void MovePos(ssize_t move_size);

    void PushBack(std::span<const char> data, size_t buff_size = kDefaultBufferSize);
    void PushBack(File& other, size_t buff_size = kDefaultBufferSize);
    void Insert(size_t pos, std::span<const char> data, size_t buff_size = kDefaultBufferSize);
    void Insert(size_t pos, File& other, size_t buff_size = kDefaultBufferSize);

    void PopBack(size_t delete_size);
    void Erase(size_t pos, size_t delete_size, size_t buff_size = kDefaultBufferSize);

    void Delete();

    static File CreateTempFile();

    ~File();
};

struct Archive {
    File file;

    virtual void Add(File& file) = 0;
    virtual void Delete(const std::string& name) = 0;
    virtual std::pair<File, bool> Get(const std::string& name) = 0;
    virtual size_t Length() const = 0;
    virtual ~Archive() = default;
};

class Hamarc : public Archive {
    /*
     * Структура архива:
     *
     * [конфиг × 5 копий (5×32 байта)]
     * [заголовок файла 1][данные файла 1]
     * [заголовок файла 2][данные файла 2]
     * ...
     *
     * Заголовок файла:
     * [размер файла (8 байт)][размер имени (4 байта)][имя файла (N байт)][encoding_info (8 байт)]
     *
     * Каждое поле закодировано кодом Хэмминга согласно ConfigEncodingFile.
     * [encoding_info] = [count_data_bit (7 байт)][count_copy (1 байт)]
     */

  public:
    struct EncodingInfo {
        uint64_t count_data_bits;
        uint64_t count_data_bytes;
        uint64_t count_hemming_bits;
        uint64_t count_additional_bytes;
        uint8_t count_copy;

        EncodingInfo(uint64_t count_bit, uint32_t count_copy);
        explicit EncodingInfo(std::span<char> arr);
        EncodingInfo();

        uint32_t ToInt() const;
        bool operator==(const EncodingInfo& other) const;
    };

    struct ConfigEncodingFile {
        EncodingInfo size_file;
        EncodingInfo size_name_file;
        EncodingInfo name_file;
        EncodingInfo encoding_data_file;

        bool operator==(const ConfigEncodingFile& other) const;

        ConfigEncodingFile();
        ConfigEncodingFile(EncodingInfo size_file, EncodingInfo size_name_file,
                           EncodingInfo name_file, EncodingInfo encoding_data_file);
        explicit ConfigEncodingFile(std::span<char> arr);
    };

    struct Hat {
        uint64_t size_file;
        uint32_t size_name_file;
        std::string name_file;
        uint64_t encoding_data_file;

        Hat(std::span<char> size_file, std::span<char> size_name_file,
            std::span<char> name_file, std::span<char> encoding_data_file);
        Hat();
    };

  private:
    ConfigEncodingFile config_;
    static constexpr size_t kCountCopyHatArchive = 5;
    size_t count_file_;
    size_t begin_file_pos_;

    std::vector<char> IntegralToVectorChar(long long num, size_t count_byte) const;
    std::vector<uint8_t> HemmingBytes(std::span<char> data, const EncodingInfo& encoding_info) const;

    std::vector<char> EncodeBlock(std::span<char> data, const EncodingInfo& encoding_info) const;
    std::vector<char> EncodeData(std::span<char> data, const EncodingInfo& encoding_info) const;
    std::vector<char> EncodeHat(Hat hat) const;
    File EncodeFile(File& file, const EncodingInfo& encoding_data,
                    size_t buff_size = File::kDefaultBufferSize) const;

    std::vector<char> DecodeBlock(std::span<char> block, const EncodingInfo& encoding_info) const;
    std::vector<char> DecodeData(std::span<char> data, const EncodingInfo& encoding_info);
    Hat DecodeHat(size_t pos);
    File DecodeFile(size_t pos, const Hat& hat, size_t buff_size = File::kDefaultBufferSize);

    size_t GetSizeFile(const Hat& hat) const;
    size_t GetPosFirstFile() const;

  public:
    Hamarc(const std::string& file_path, bool create_if_not_exist,
           const ConfigEncodingFile& config);

    ConfigEncodingFile GetConfig() const;

    std::pair<File, bool> Get(const std::string& name) override;
    ssize_t GetPos(const std::string& name);

    void Add(File& file) override;
    void Add(File& file, const EncodingInfo& encoding_info);

    void Delete(const std::string& name) override;

    size_t Length() const override;

    std::vector<std::pair<std::string, size_t>> Info();

    std::vector<File> GetAll();

    bool Merge(Hamarc& other);
};

} // namespace managefile
