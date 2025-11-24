#include "manager_files.hpp";

void managefile::File::Open(std::string name) {
    if (!std::filesystem::exists(fully_name)){
        std::ofstream cur = std::ofstream(fully_name, std::ios::out);
        cur.close();
    }
    stream = std::fstream{fully_name, std::ios::in | std::ios::out | std::ios::binary};
}

void managefile::File::OpenWithApp(std::string name) {
    if (!std::filesystem::exists(fully_name)){
        std::ofstream cur = std::ofstream(fully_name, std::ios::out);
        cur.close();
    }
    stream = std::fstream{fully_name, std::ios::in | std::ios::out | std::ios::binary | std::ios::app};
}

void managefile::File::TryOpen(std::string name) {
    stream = std::fstream{fully_name, std::ios::in | std::ios::out | std::ios::binary};
}

managefile::File::File(std::string name, std::string suffix_name, bool is_tmp) :File(name + "." + suffix_name,is_tmp) {}

managefile::File::File(std::string fully_name, bool is_tmp) {
    int32_t index_point = 0;
    int32_t index_finish_way = -1;
    for (uint32_t index = 0; index < fully_name.size(); index++){
        if (fully_name[index] == '.'){
            index_point = index;
            break;
        }
        else if (fully_name[index] == '/'){
            index_finish_way = index;
        }
    }

    std::string name, suffix_name;
    for (uint32_t index = index_finish_way + 1; index < index_point; index++){
        name += fully_name[index];
    }
    for (uint32_t index = index_point + 1; index < fully_name.size(); index++){
        suffix_name += fully_name[index];
    }

    this->name = name;
    this->suffix_name = suffix_name;
    this->fully_name = this->name + "." + this->suffix_name;
    this->is_tmp = is_tmp;

    Open(fully_name);
}

managefile::File::File(File &&other) { *this = std::move(other); }

managefile::File &managefile::File::operator=(File &&other) {
    if (IsOpen()) {
        stream.close();
    }
    if (other.IsOpen()) {
        other.stream.close();
    }

    this->name = other.name;
    this->suffix_name = other.suffix_name;
    this->fully_name = other.fully_name;
    Open(fully_name);
    this->is_tmp = other.is_tmp;
    other.is_tmp = 0;

    return *this;
}

void managefile::File::Close() {
    stream.close();
}

bool managefile::File::IsOpen() const { return stream.is_open(); }

bool managefile::File::IsEOF() const { return stream.eof(); }

size_t managefile::File::GCount() const  { return stream.gcount(); }

std::string managefile::File::GetName() const { return name; }

std::string managefile::File::GetFullyName() const {return fully_name;}

std::string managefile::File::GetLongName() const {
    std::string name = GetFullyName();
    uint32_t index_point = 0;
    for (uint32_t index = 0; index < name.size(); index++){
        if (name[index] == '.'){
            index_point = index;
            break;
        }
    }
    
    std::string long_name = name.substr(0, index_point);
    return long_name;
}

std::string managefile::File::GetFormat() const { return suffix_name; }

std::string managefile::File::GetData(size_t size_get, size_t pos) {
    if (stream.eof()) {
        stream.clear();
    }

    stream.seekg(pos, std::ios::beg);
    std::string current(size_get, '0');
    stream.read(&current[0], size_get);
    return current;
}

std::string managefile::File::GetCurrentData(size_t size_get) {
    std::string current(size_get, '0');
    stream.read(&current[0], size_get);
    return current;
}

std::streampos managefile::File::GetPos() { return stream.tellg(); }

void managefile::File::SetPos(size_t index)  {
    if (stream.eof()) {
        stream.clear();
    }
    stream.seekg(index, std::ios::beg);
}

void managefile::File::NextPos(size_t next_pos) { SetPos(GetPos() + next_pos); }

void managefile::File::Replace(size_t pos, const std::string &str, size_t size_replace) {
    if (stream.eof()) {
        stream.clear();
    }

    stream.seekg(pos, std::ios::beg);
    stream.write(&str[0], size_replace);
}

void managefile::File::PushBack(const std::string &str, size_t size_add) {
    if (stream.eof()) {
        stream.clear();
    }

    stream.seekg(0, std::ios::end);
    stream.write(&str[0], size_add);
}

void managefile::File::PushBack(File &other, size_t size_buff) {
    std::string buff(size_buff, '0');
    other.SetPos(0);

    while (!other.IsEOF()) {
        buff = other.GetCurrentData(size_buff);
        PushBack(buff, other.GCount());
    }
}

void managefile::File::ReplaceCurrentPos(const std::string &str, size_t size_replace) {
    if (stream.eof()) {
        stream.clear();
    }

    stream.write(&str[0], size_replace);
}

void managefile::File::Insert(size_t pos, const std::string &str, size_t size_insert, size_t size_buff) {
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

    current_file.Delete();
}

void managefile::File::DeletePos(size_t pos, size_t size_delete, size_t size_buff) {
    if (stream.eof()) {
        stream.clear();
    }

    SetPos(0);
    File current_file = GetUniqueFile();
    std::string buff(size_buff, '0');
    while (!stream.eof() && GetPos() < pos) {
        if (pos - GetPos() > size_buff){
            stream.read(&buff[0], size_buff);
        }
        else{
            stream.read(&buff[0], pos - GetPos());
        }

        size_t count_symbol = stream.gcount();
        current_file.PushBack(buff, count_symbol);
    }
    NextPos(size_delete);
    while (!stream.eof()){
        stream.read(&buff[0], size_buff);

        size_t count_symbol = stream.gcount();
        current_file.PushBack(buff, count_symbol);
    }

    current_file.SetPos(0);
    stream.close();
    stream.open(fully_name, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);

    PushBack(current_file, size_buff);

    current_file.Delete();
}

void managefile::File::Delete() { stream.close(); std::remove((fully_name).c_str()); }

size_t managefile::File::GetSize() const { return std::filesystem::file_size(fully_name); }

managefile::File managefile::File::GetUniqueFile() {
    std::string name = "current";
    std::string result_name = name + ".tmp";

    size_t number = 1;
    while (std::filesystem::exists(result_name)) {
        result_name = name + std::to_string(number) + ".tmp";
    }

    File result(name + std::to_string(number), "tmp");

    return result;
}

managefile::File::~File() {
    if (is_tmp) {
        Delete();
    }
    else{
        stream.close();
    }
}
