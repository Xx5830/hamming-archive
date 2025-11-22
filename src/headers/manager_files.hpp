#include <cstddef>
#include <fstream>
#include <optional>
#include <string>
#include <vector>
#include <filesystem>

#ifndef INCLUDE_MANAGER_FILES
#define INCLUDE_MANAGER_FILES
namespace managfile {

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
    std::string name;
    std::string suffix_name;
    std::fstream stream;

    File(std::string name, std::string suffix_name) {
        this->name = name;
        this->suffix_name = suffix_name;

        stream = std::fstream{name, std::ios::in | std::ios::out | std::ios::binary};
    };

  public:
    bool IsOpen() { return stream.is_open(); }
    bool IsEOF(){return stream.eof();}
    size_t GCount(){return stream.gcount();}
    std::string GetName() const { return name; }
    std::string GetFormat() const { return suffix_name; }
    std::string GetData(size_t size_buff, size_t pos) {
        if (stream.eof()) {
            stream.clear();
        }

        stream.seekg(pos, std::ios::beg);
        std::string current(size_buff, '0');
        stream.read(&current[0], size_buff);
        return current;
    };
    std::string GetCurrentData(size_t size_buff) {
        std::string current(size_buff, '0');
        stream.read(&current[0], size_buff);
        return current; 
    }
    size_t GetCurrentPos() const{
        return stream.tellg();
    }
    void SetPos(size_t index){
        stream.seekg(index, std::ios::beg);
    }
    void Replace(size_t pos, const std::string &str, size_t size_replace){
        if (stream.eof()) {
            stream.clear();
        }

        stream.seekg(pos, std::ios::beg);
        stream.write(&str[0], size_replace);
    }
    void PushBack(const std::string &str, size_t size_add){
        if (stream.eof()) {
            stream.clear();
        }
        
        stream.seekg(0, std::ios::end);
        stream.write(&str[0], size_add);
    }
    void PushBack(File other, size_t size_buff){
        std::string buff(size_buff, '0');
        other.SetPos(0);

        while (!other.IsEOF()){
            buff = other.GetCurrentData(size_buff);
            PushBack(buff, other.GCount());
        }
    }
    void ReplaceCurrentPos(const std::string &str, size_t size_replace){
        if (stream.eof()) {
            stream.clear();
        }

        stream.write(&str[0], size_replace);
    }
    void Insert(size_t pos, const std::string &str, size_t size_insert, size_t size_buff){
        if (stream.eof()) {
            stream.clear();
        }

        File current_file = GetUniqueFile();
        stream.seekg(pos, std::ios::beg);
        std::string buff(size_buff, '0');
        while (!stream.eof()){
            stream.read(&buff[0], size_buff);
            size_t count_symbol = stream.gcount();
            current_file.PushBack(buff, count_symbol);
        }

        
        Replace(pos, str, size_insert);
        

        current_file.DeleteFile();
    }
    
    void DeleteFile(){
        std::remove((name + "." + suffix_name).c_str());
    }

    static File GetUniqueFile(){
        std::string name = "current";
        std::string result_name = name + ".txt";

        size_t number = 1;
        while (std::filesystem::exists(result_name)){
            result_name = name + std::to_string(number) + ".txt";
        }

        File(name + std::to_string(number), "txt");
    } 
};

class Archive : public File {
  public:
    virtual bool Add(File &file) const = 0;
    virtual bool Delete(const std::string &name) const = 0;
};

class TextFile : public File {};

struct ManagerHaf {
    std::fstream archive;

    ManagerHaf(std::string &path);
    void AddFile(std::ifstream &in);
    void AddArchive(std::ifstream &in);
    std::vector<std::string> GetNameFiles();
    void Erase(std::string &name);
    void Extract(std::string &name);
};

} // namespace managfile

#endif