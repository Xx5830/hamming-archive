#include "headers/manager_files.hpp"

managefile::File::File() : delete_on_close_(false) {}

void managefile::File::Open(bool create_if_not_exists, bool need_clear_on_open) {
    if (!std::filesystem::exists(FilePath()) && create_if_not_exists) {
        if (!Directories().empty()) {
            std::filesystem::create_directories(Directories());
        }
        std::ofstream(FilePath(), std::ios::out).close();
    }

    auto mode = std::ios::in | std::ios::out | std::ios::binary;
    if (need_clear_on_open) {
        mode |= std::ios::trunc;
    }
    stream_ = std::fstream{FilePath(), mode};
}

managefile::File::File(const std::string& file_path, bool create_if_not_exists, bool delete_on_close,
                       bool need_clear_on_open)
    : delete_on_close_(delete_on_close) {
    ssize_t pos_slash = -1;
    ssize_t pos_point = -1;

    for (size_t i = 0; i < file_path.size(); ++i) {
        if (file_path[i] == '/') {
            pos_slash = static_cast<ssize_t>(i);
        } else if (pos_point == -1 && file_path[i] == '.') {
            pos_point = static_cast<ssize_t>(i);
        }
    }

    file_path_ = file_path;

    if (pos_slash != -1) {
        directories_path_ = file_path.substr(0, pos_slash);
    }
    if (pos_point != -1) {
        format_ = file_path.substr(pos_point + 1);
    }

    ssize_t effective_point = (pos_point == -1) ? static_cast<ssize_t>(file_path.size()) : pos_point;

    name_ = file_path.substr(pos_slash + 1, effective_point - pos_slash - 1);

    Open(create_if_not_exists, need_clear_on_open);
}

managefile::File::File(File&& other) { *this = std::move(other); }

managefile::File& managefile::File::operator=(File&& other) {
    if (this == &other) {
        return *this;
    }
    if (IsOpen()) {
        stream_.close();
    }
    file_path_ = std::move(other.file_path_);
    name_ = std::move(other.name_);
    directories_path_ = std::move(other.directories_path_);
    format_ = std::move(other.format_);
    stream_ = std::move(other.stream_);
    delete_on_close_ = other.delete_on_close_;
    other.delete_on_close_ = false;
    return *this;
}

void managefile::File::Close() { stream_.close(); }

bool managefile::File::IsOpen() const { return stream_.is_open(); }

bool managefile::File::IsEOF() const { return stream_.eof(); }

void managefile::File::ClearEOF() { stream_.clear(); }

std::string managefile::File::Name() const { return name_; }
std::string managefile::File::Directories() const { return directories_path_; }
std::string managefile::File::Format() const { return format_; }
std::string managefile::File::FilePath() const { return file_path_; }

ssize_t managefile::File::Pos() { return static_cast<ssize_t>(stream_.tellg()); }

size_t managefile::File::GCount() const { return static_cast<size_t>(stream_.gcount()); }

size_t managefile::File::Length() const { return std::filesystem::file_size(FilePath()); }

void managefile::File::MakeTemp() { delete_on_close_ = true; }

void managefile::File::UnMakeTemp() { delete_on_close_ = false; }

std::vector<char> managefile::File::Read(size_t read_size) {
    std::vector<char> current(read_size);
    if (read_size > 0) {
        stream_.read(current.data(), static_cast<std::streamsize>(read_size));
        current.resize(GCount());
    }
    return current;
}

std::vector<char> managefile::File::ReadPos(size_t pos, size_t read_size) {
    SetPos(pos);
    return Read(read_size);
}

void managefile::File::Write(std::span<const char> data) {
    stream_.write(data.data(), static_cast<std::streamsize>(data.size()));
}

void managefile::File::Write(File& other, size_t buff_size) {
    other.SetBegin();
    while (!other.IsEOF()) {
        std::vector<char> buff = other.Read(buff_size);
        Write(buff);
    }
}

void managefile::File::WritePos(size_t pos, std::span<const char> data) {
    SetPos(pos);
    Write(data);
}

void managefile::File::WritePos(size_t pos, File& other, size_t buff_size) {
    SetPos(pos);
    Write(other, buff_size);
}

void managefile::File::SetPos(size_t index) {
    if (IsEOF()) {
        ClearEOF();
    }
    stream_.seekg(static_cast<std::streamoff>(index), std::ios::beg);
    stream_.seekp(static_cast<std::streamoff>(index), std::ios::beg);
}

void managefile::File::SetBegin() {
    if (IsEOF()) {
        ClearEOF();
    }
    stream_.seekg(0, std::ios::beg);
    stream_.seekp(0, std::ios::beg);
}

void managefile::File::SetEnd() {
    if (IsEOF()) {
        ClearEOF();
    }
    stream_.seekg(0, std::ios::end);
    stream_.seekp(0, std::ios::end);
}

void managefile::File::MovePos(ssize_t move_size) {
    if (IsEOF()) {
        ClearEOF();
    }
    stream_.seekg(move_size, std::ios::cur);
}

void managefile::File::PushBack(std::span<const char> data, size_t) {
    SetEnd();
    Write(data);
}

void managefile::File::PushBack(File& other, size_t buff_size) {
    other.SetPos(0);
    while (!other.IsEOF()) {
        std::vector<char> buff = other.Read(buff_size);
        if (!buff.empty()) {
            PushBack(buff);
        }
    }
}

void managefile::File::Insert(size_t pos, std::span<const char> data, size_t buff_size) {
    File temp_file = CreateTempFile();
    SetPos(pos);

    while (!IsEOF()) {
        std::vector<char> buff = Read(buff_size);
        temp_file.PushBack(buff);
    }

    WritePos(pos, data);
    Write(temp_file, buff_size);
}

void managefile::File::Insert(size_t pos, File& other, size_t buff_size) {
    File temp_file = CreateTempFile();
    SetPos(pos);

    while (!IsEOF()) {
        std::vector<char> buff = Read(buff_size);
        temp_file.PushBack(buff);
    }

    WritePos(pos, other, buff_size);
    Write(temp_file, buff_size);
}

void managefile::File::PopBack(size_t delete_size) {
    stream_.flush();
    size_t len = Length();
    if (delete_size > len) {
        delete_size = len;
    }
    std::filesystem::resize_file(FilePath(), len - delete_size);
}

void managefile::File::Erase(size_t pos, size_t delete_size, size_t buff_size) {
    File temp_file = CreateTempFile();

    SetPos(0);
    while (!IsEOF() && static_cast<size_t>(Pos()) < pos) {
        size_t to_read = std::min(buff_size, pos - static_cast<size_t>(Pos()));
        std::vector<char> buff = Read(to_read);
        temp_file.PushBack(buff);
    }

    MovePos(static_cast<ssize_t>(delete_size));

    while (!IsEOF()) {
        std::vector<char> buff = Read(buff_size);
        temp_file.PushBack(buff);
    }

    Close();
    Open(false, true);
    PushBack(temp_file, buff_size);
}

void managefile::File::Delete() {
    Close();
    std::filesystem::remove(FilePath());
}

managefile::File managefile::File::CreateTempFile() {
    const std::string base = "hamarc_tmp";
    std::string result_name;
    size_t number = 0;
    do {
        result_name = base + std::to_string(number++) + ".tmp";
    } while (std::filesystem::exists(result_name));

    return File(result_name, true, true);
}

managefile::File::~File() {
    if (delete_on_close_) {
        Delete();
    } else {
        Close();
    }
}