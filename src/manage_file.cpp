#include "headers/manager_files.hpp"
#include <sys/stat.h>

managefile::File::File() {}

void managefile::File::Open(bool create_if_not_exists, bool need_clear_on_open) {
    if (!std::filesystem::exists(FilePath()) && create_if_not_exists) {
        if (Directories().size() != 0) {
            std::filesystem::create_directories(Directories());
        }

        std::ofstream(FilePath(), std::ios::out).close();
    }

    if (need_clear_on_open) {
        stream = std::fstream{FilePath(), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc};
    } else {
        stream = std::fstream{FilePath(), std::ios::in | std::ios::out | std::ios::binary};
    }
}

managefile::File::File(const std::string &file_path, bool create_if_not_exists, bool delete_on_close,
                                bool need_clear_on_open) {
    ssize_t pos_slash = -1;
    ssize_t pos_point = -1;

    for (size_t index = 0; index < file_path.size(); index++) {
        if (file_path[index] == '/') {
            pos_slash = index;
        } else if (pos_point == -1 && file_path[index] == '.') {
            pos_point = index;
        }
    }

    this->file_path = file_path;

    if (pos_slash != -1) {
        directories_path = file_path.substr(0, pos_slash);
    }
    if (pos_point != -1) {
        format = file_path.substr(pos_point + 1, file_path.size() - pos_point - 1);
    }

    if (pos_point == -1) {
        pos_point = file_path.size();
    }

    name = file_path.substr(pos_slash + 1, pos_point - pos_slash - 1);

    this->delete_on_close = delete_on_close;
    Open(create_if_not_exists, need_clear_on_open);
}

managefile::File::File(File &&other) { *this = std::move(other); }

managefile::File &managefile::File::operator=(File &&other) {
    if (IsOpen()) {
        stream.close();
    }

    this->file_path = std::move(other.file_path);
    this->name = std::move(other.name);
    this->directories_path = std::move(other.directories_path);
    this->format = std::move(other.format);
    stream = std::move(other.stream);
    this->delete_on_close = other.delete_on_close;
    other.delete_on_close = false;

    return *this;
}

void managefile::File::Close() { stream.close(); }

bool managefile::File::IsOpen() const { return stream.is_open(); }

bool managefile::File::IsEOF() const { return stream.eof(); }

void managefile::File::ClearEOF() {stream.clear();}

std::string managefile::File::Name() const { return name; }

std::string managefile::File::Directories() const { return directories_path; }

std::string managefile::File::Format() const { return format; }

std::string managefile::File::FilePath() const { return file_path; }

ssize_t managefile::File::Pos() { return stream.tellg(); }

size_t managefile::File::GCount() const { return stream.gcount(); }

size_t managefile::File::Length() const { return std::filesystem::file_size(FilePath()); }

<<<<<<< HEAD
=======
void managefile::File::MakeTemp() {delete_on_close = true;}

void managefile::File::UnMakeTemp() {delete_on_close = false;}

>>>>>>> d6641a0 (synch)
std::vector<char> managefile::File::Read(size_t read_size) {
    std::vector <char> current(read_size);

    if (read_size > 0){
        stream.read(&current[0], read_size);
        current.resize(GCount());
    }
    return current;
}

std::vector<char> managefile::File::ReadPos(size_t pos, size_t read_size) {
    SetPos(pos);
    std::vector<char> current = Read(read_size);
    return current;
}

void managefile::File::Write(const std::span<char> data) {
    stream.write(&data[0], data.size());
}

void managefile::File::Write(File &other, size_t buff_size) {
    other.SetBegin();
    while (!other.IsEOF()){
        std::vector <char> buff = other.Read(buff_size);
        Write(buff);
    }
}

void managefile::File::WritePos(size_t pos, const std::span<char> data) {
    SetPos(pos);
    Write(data);
}

void managefile::File::WritePos(size_t pos, File &other, size_t buff_size) {
    SetPos(pos);
    Write(other, buff_size);
}

void managefile::File::SetPos(size_t index) {
    if (IsEOF()){
        ClearEOF();
    }

    stream.seekg(index, std::ios::beg);
}

void managefile::File::SetBegin() {
    if (IsEOF()){
        ClearEOF();
    }

    stream.seekg(0, std::ios::beg);
}

void managefile::File::SetEnd() {
    if (IsEOF()){
        ClearEOF();
    }

    stream.seekg(0, std::ios::end);
}

void managefile::File::MovePos(ssize_t move_size){
    if (IsEOF()){
        ClearEOF();
    }

    stream.seekg(move_size, std::ios::cur);
}

void managefile::File::PushBack(const std::span<char> data, size_t buff_size) {
    SetEnd();
    Write(data);
}

void managefile::File::PushBack(File &other, size_t buff_size) {
    std::vector<char> buff(buff_size);
    other.SetPos(0);

    while (!other.IsEOF()) {
        buff = other.Read(buff_size);
        PushBack(buff, other.GCount());
    }
}

void managefile::File::Insert(size_t pos, const std::span<char> data, size_t buff_size) {
    File temp_file = CreateTempFile();
    temp_file.SetBegin();
    SetPos(pos);

    while (!IsEOF()) {
        std::vector<char> buff = Read(buff_size);
        size_t count_symbol = GCount();
        temp_file.PushBack(buff, count_symbol);
    }

    WritePos(pos, data);
    Write(temp_file, buff_size);
}

void managefile::File::Insert(size_t pos, File &other, size_t buff_size) {
    File temp_file = CreateTempFile();
    temp_file.SetBegin();
    SetPos(pos);

    while (!IsEOF()) {
        std::vector <char> buff = Read(buff_size);
        temp_file.PushBack(buff, buff.size());
    }

    WritePos(pos, other, buff_size);
    Write(temp_file, buff_size);
}

void managefile::File::PopBack(size_t delete_size) {
    if (delete_size > Length()){
        delete_size = Length();
    }
    std::filesystem::resize_file(FilePath(), Length() - delete_size);
}

void managefile::File::Erase(size_t pos, size_t delete_size, size_t buff_size) {
    SetPos(0);
    File temp_file = CreateTempFile();
    
    while (!IsEOF() && Pos() < pos) {
        std::vector <char> buff;
        if (pos - Pos() > buff_size) {
            buff = Read(buff_size);
        } else {
            buff = Read(pos - Pos());
        }

        temp_file.PushBack(buff, buff.size());
    }

    MovePos(delete_size);

    while (!IsEOF()) {
        std::vector <char> buff = Read(buff_size);

        temp_file.PushBack(buff, buff.size());
    }

    temp_file.SetPos(0);
    Close();
    Open(0, 1);

    PushBack(temp_file, buff_size);
}

void managefile::File::Delete() {
    Close();
    std::remove((FilePath()).c_str());
}

managefile::File managefile::File::CreateTempFile() {
    std::string name = "current";
    std::string result_name = name + ".tmp";

    size_t number = 1;
    while (std::filesystem::exists(result_name)) {
        result_name = name + std::to_string(number) + ".tmp";
    }

    File result(name + std::to_string(number) + ".tmp", true, true);

    return result;
}

managefile::File::~File() {
    if (delete_on_close) {
        Delete();
    } else {
        Close();
    }
}
