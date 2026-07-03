#include "headers/manager_files.hpp"
#include <algorithm>
#include <iostream>

managefile::Hamarc::EncodingInfo::EncodingInfo(uint64_t count_bit, uint32_t count_copy) {
    size_t count_additional_bits = 0;

    while ((1 << count_additional_bits) <= count_bit + count_additional_bits) {
        ++count_additional_bits;
    }

    count_data_bits = count_bit;
    count_data_bytes = count_bit / 8 + (count_bit % 8 != 0 ? 1 : 0);
    count_hemming_bits = count_additional_bits;
    count_additional_bytes = count_additional_bits / 8 + 1;
    if (count_data_bits % 8 + count_additional_bits % 8 > 8) {
        ++count_additional_bytes;
    }
    this->count_copy = count_copy;
}

managefile::Hamarc::EncodingInfo::EncodingInfo(std::span<char> arr) {
    size_t count_bit = 0;
    size_t count_copy = 0;

    for (size_t index = 0; index < 7; index++) {
        count_bit += static_cast<uint64_t>(arr[index]) << (48 - index * 8);
    }
    count_copy = arr[7];

    *this = EncodingInfo(count_bit, count_copy);
}

managefile::Hamarc::EncodingInfo::EncodingInfo() {}

uint32_t managefile::Hamarc::EncodingInfo::ToInt() const { return (count_data_bits << 8) + (count_copy & 0b11111111); }

bool managefile::Hamarc::EncodingInfo::operator==(const EncodingInfo &other) const {
    return count_data_bits == other.count_data_bits && count_data_bytes == other.count_data_bytes &&
           count_additional_bytes == other.count_additional_bytes && count_hemming_bits == other.count_hemming_bits &&
           count_copy == other.count_copy;
}

bool managefile::Hamarc::ConfigEncodingFile::operator==(const ConfigEncodingFile &other) const {
    return size_file == other.size_file && size_name_file == other.size_name_file && name_file == other.name_file &&
           encoding_data_file == other.encoding_data_file;
}

managefile::Hamarc::ConfigEncodingFile::ConfigEncodingFile()
    : size_file(8, 1), size_name_file(8, 1), name_file(8, 1), encoding_data_file(8, 1) {}

managefile::Hamarc::ConfigEncodingFile::ConfigEncodingFile(EncodingInfo size_file, EncodingInfo size_name_file,
                                                           EncodingInfo name_file, EncodingInfo encoding_data_file)
    : size_file(size_file), size_name_file(size_name_file), name_file(name_file),
      encoding_data_file(encoding_data_file) {}

managefile::Hamarc::ConfigEncodingFile::ConfigEncodingFile(const std::span<char> &arr) {
    for (size_t start_index = 0; start_index < arr.size(); start_index += 8) {
        char current[8];
        for (size_t index = start_index; index < start_index + 8; index++) {
            current[index - start_index] = arr[index];
        }

        if (start_index == 0) {
            size_file = EncodingInfo(current);
        } else if (start_index == 8) {
            size_name_file = EncodingInfo(current);
        } else if (start_index == 16) {
            name_file = EncodingInfo(current);
        } else if (start_index == 24) {
            encoding_data_file = EncodingInfo(current);
        }
    }
}

managefile::Hamarc::Hat::Hat(std::span<char> size_file, std::span<char> size_name_file, std::span<char> name_file,
                             std::span<char> encoding_data_file) {
    this->size_file = 0;
    for (size_t index = 0; index < size_file.size(); index++) {
        size_t shift = ((size_file.size() - index - 1) * 8);
        uint64_t value = static_cast<uint8_t>(size_file[index]);
        value <<= shift;
        this->size_file += value;
    }

    this->size_name_file = 0;
    for (size_t index = 0; index < size_name_file.size(); index++) {
        size_t shift = ((size_name_file.size() - index - 1) * 8);
        uint64_t value = static_cast<uint8_t>(size_name_file[index]);
        value <<= shift;
        this->size_name_file += value;
    }

    for (size_t index = 0; index < name_file.size(); index++) {
        this->name_file += name_file[index];
    }

    this->encoding_data_file = 0;
    for (size_t index = 0; index < encoding_data_file.size(); index++) {
        size_t shift = ((encoding_data_file.size() - index - 1) * 8);
        uint64_t value = static_cast<uint8_t>(encoding_data_file[index]);
        value <<= shift;
        this->encoding_data_file += value;
    }
}

managefile::Hamarc::Hat::Hat() {}

std::vector<char> managefile::Hamarc::IntegralToVectorCHar(long long num, size_t count_byte) const {
    std::vector<char> result;

    while (result.size() < count_byte) {
        result.push_back(static_cast<char>(num & 0b11111111));
        num >>= 8;
    }

    std::reverse(result.begin(), result.end());

    return result;
}

std::vector<uint8_t> managefile::Hamarc::HemmingBytes(const std::span<char> data,
                                                      const EncodingInfo &encoding_info) const {
    std::vector<uint8_t> EncodingBytes(encoding_info.count_additional_bytes +
                                       (encoding_info.count_data_bits % 8 != 0 ? 1 : 0));

    size_t p = 0;
    for (size_t index_byte = 0; index_byte < data.size() - 1; index_byte++) {
        for (size_t index_bit = 0; index_bit < 8; index_bit++) {
            size_t pos = index_byte * 8 + index_bit + p + 1;
            while (pos == (1 << p)) {
                ++p;
                ++pos;
            }

            if (data[index_byte] & (1 << index_bit)) {
                for (size_t index_bit2 = 0; index_bit2 < 8 * EncodingBytes.size(); index_bit2++) {
                    if (pos & (1 << index_bit2)) {
                        uint8_t index_byte2 = index_bit2 / 8;
                        uint8_t ost = index_bit2 - index_byte2 * 8;
                        EncodingBytes[index_byte2] ^= (1 << ost);
                    }
                }
            }
        }
    }
    size_t ost_bits = encoding_info.count_data_bits % 8;
    if (ost_bits == 0) {
        ost_bits = 8;
    }
    for (size_t index_bit = 0; index_bit < ost_bits; index_bit++) {
        size_t pos = (data.size() - 1) * 8 + index_bit + p + 1;
        while (pos == (1 << p)) {
            ++p;
            ++pos;
        }

        if (data[data.size() - 1] & (1 << index_bit)) {
            for (size_t index_bit2 = 0; index_bit2 < 8 * EncodingBytes.size(); index_bit2++) {
                if (pos & (1 << index_bit2)) {
                    uint8_t index_byte2 = index_bit2 / 8;
                    uint8_t ost = index_bit2 - index_byte2 * 8;
                    EncodingBytes[index_byte2] ^= (1 << ost);
                }
            }
        }
    }

    return EncodingBytes;
}

std::vector<char> managefile::Hamarc::EncodeBlock(const std::span<char> block,
                                                  const EncodingInfo &encoding_info) const {
    std::vector<char> result(block.size());

    for (size_t index = 0; index < block.size(); index++) {
        result[index] = block[index];
    }

    std::vector<uint8_t> hemming_bytes = HemmingBytes(result, encoding_info);
    size_t index_hemming_bit = 0;

    for (size_t index_bit = encoding_info.count_data_bits % 8;
         index_bit != 0 && index_bit < 8 && index_hemming_bit < encoding_info.count_hemming_bits;
         index_bit++, index_hemming_bit++) {
        uint8_t bit = hemming_bytes[0] & (1 << index_hemming_bit);
        uint8_t shift = index_bit - index_hemming_bit;
        result[result.size() - 1] |= bit << shift;
    }

    while (index_hemming_bit < encoding_info.count_hemming_bits) {
        result.push_back(0);
        for (size_t bit_result = 0; bit_result < 8 && index_hemming_bit < encoding_info.count_hemming_bits;
             bit_result++, index_hemming_bit++) {
            uint8_t bit = hemming_bytes[index_hemming_bit / 8] & (1 << (index_hemming_bit % 8));

            if (index_hemming_bit % 8 >= bit_result) {
                uint8_t shift = (index_hemming_bit % 8) - bit_result;
                result.back() |= bit >> shift;
            } else {
                uint8_t shift = bit_result - (index_hemming_bit % 8);
                result.back() |= bit << shift;
            }
        }
    }

    std::vector<char> current = result;
    result.clear();
    for (size_t index_copy = 0; index_copy < encoding_info.count_copy; index_copy++) {
        for (size_t index = 0; index < current.size(); index++) {
            result.push_back(current[index]);
        }
    }

    return result;
}

std::vector<char> managefile::Hamarc::EncodeData(const std::span<char> data, const EncodingInfo &encoding_info) const {
    struct ReadMachine {
        std::vector<char> current;
        std::vector<std::vector<char>> blocks;
        size_t current_bit = 0;
        size_t bit_size_block;

        std::vector<std::vector<char>> GetResult() {
            if (current.size() != 0) {
                blocks.push_back(current);
            }

            return blocks;
        }
        void Read(std::span<char> data) {
            for (size_t index = 0; index < data.size(); index++) {
                for (size_t data_bit = 0; data_bit < 8; data_bit++, current_bit++) {
                    if (current_bit == bit_size_block) {
                        blocks.push_back(current);
                        current.clear();
                        current_bit = 0;
                    }
                    if (current_bit % 8 == 0) {
                        current.push_back(static_cast<char>(0));
                    }
                    uint8_t value = data[index] & (1 << data_bit);

                    if (value) {
                        current[current_bit / 8] += 1 << (current_bit % 8);
                    }
                }
            }
        }

        ReadMachine(size_t bit_size_block) { this->bit_size_block = bit_size_block; }
    };
    std::vector<char> result;

    ReadMachine read_machine(encoding_info.count_data_bits);

    read_machine.Read(data);
    std::vector<std::vector<char>> blocks = read_machine.GetResult();

    while (blocks.back().size() != encoding_info.count_data_bytes) {
        blocks.back().push_back(static_cast<char>(0));
    }

    for (size_t index = 0; index < blocks.size(); index++) {
        std::vector<char> encode_block = EncodeBlock(blocks[index], encoding_info);

        for (size_t index = 0; index < encode_block.size(); index++) {
            result.push_back(encode_block[index]);
        }
    }

    return result;
}

std::vector<char> managefile::Hamarc::EncodeHat(Hat hat) const {
    std::vector<char> size_file = IntegralToVectorCHar(hat.size_file, 8);
    std::vector<char> encoded_size_file = EncodeData(size_file, config.size_file);

    std::vector<char> size_name_file = IntegralToVectorCHar(hat.size_name_file, 4);
    std::vector<char> encoded_size_name_file = EncodeData(size_name_file, config.size_name_file);

    std::vector<char> name_file(hat.size_name_file);
    for (size_t index = 0; index < name_file.size(); index++) {
        name_file[index] = hat.name_file[index];
    }
    std::vector<char> encoded_name_file = EncodeData(name_file, config.name_file);

    std::vector<char> encoding_data_file = IntegralToVectorCHar(hat.encoding_data_file, 8);
    std::vector<char> encoded_encoding_data_file = EncodeData(encoding_data_file, config.encoding_data_file);

    std::vector<char> result;

    for (size_t index = 0; index < encoded_size_file.size(); index++) {
        result.push_back(encoded_size_file[index]);
    }
    for (size_t index = 0; index < encoded_size_name_file.size(); index++) {
        result.push_back(encoded_size_name_file[index]);
    }
    for (size_t index = 0; index < encoded_name_file.size(); index++) {
        result.push_back(encoded_name_file[index]);
    }
    for (size_t index = 0; index < encoded_encoding_data_file.size(); index++) {
        result.push_back(encoded_encoding_data_file[index]);
    }

    return result;
}

managefile::File managefile::Hamarc::EncodeFile(File &file, const EncodingInfo &encoding_data, size_t buff_size) const {
    File tmp_file = file.CreateTempFile();

    auto lambda_gcd = [](long long a, long long b) {
        while (b) {
            long long c = b;
            b = a % b;
            a = c;
        }
        return a;
    };
    auto lambda_lcm = [&lambda_gcd](long long a, long long b) { return a / lambda_gcd(a, b) * b; };

    Hat hat;
    hat.size_file = file.Length();
    hat.size_name_file = file.Name().size() + file.Format().size() + 1;
    hat.name_file = file.Name() + "." + file.Format();
    hat.encoding_data_file = encoding_data.ToInt();

    std::vector<char> hat_data = EncodeHat(hat);
    tmp_file.Write(hat_data);

    buff_size = lambda_lcm(buff_size, 8);
    size_t count_buff = file.Length() / buff_size;

    file.SetPos(0);
    for (size_t index = 0; index < count_buff; index++) {
        std::vector<char> buff;
        buff = file.Read(buff_size);

        std::vector<char> encoded_data = EncodeData(buff, encoding_data);
        tmp_file.Write(encoded_data);
    }
    if (file.Length() % buff_size != 0) {
        std::vector<char> buff;
        buff = file.Read(buff_size);

        std::vector<char> encoded_data = EncodeData(buff, encoding_data);
        tmp_file.Write(encoded_data);
    }

    return tmp_file;
}

std::vector<char> managefile::Hamarc::DecodeBlock(const std::span<char> block,
                                                  const EncodingInfo &encoding_info) const {
    std::vector<char> data(encoding_info.count_data_bytes);
    std::vector<uint8_t> hemming_bytes1;

    for (size_t index = 0; index < data.size(); index++) {
        data[index] = block[index];
    }
    size_t index_hemming_bit1 = 0;
    if (encoding_info.count_data_bits % 8 != 0) {
        data[data.size() - 1] = 0;
        for (size_t index_bit = 0; index_bit < encoding_info.count_data_bits % 8; index_bit++) {
            data[data.size() - 1] |= block[data.size() - 1] & (1 << index_bit);
        }

        hemming_bytes1.push_back(0);
        for (size_t index_bit = encoding_info.count_data_bits % 8;
             index_bit < 8 && index_hemming_bit1 < encoding_info.count_hemming_bits;
             index_bit++, index_hemming_bit1++) {
            size_t bit = block[data.size() - 1] & (1 << index_bit);
            size_t shift = index_bit - index_hemming_bit1;
            hemming_bytes1.back() |= bit >> shift;
        }
    }
    for (size_t index = data.size(); index < block.size(); index++) {
        for (size_t index_bit = 0; index_bit < 8 && index_hemming_bit1 < encoding_info.count_hemming_bits;
             index_bit++, index_hemming_bit1++) {
            size_t bit = block[index] & (1 << index_bit);

            if (hemming_bytes1.size() * 8 <= index_hemming_bit1) {
                hemming_bytes1.push_back(0);
            }

            if (index_hemming_bit1 % 8 > index_bit) {
                size_t shift = (index_hemming_bit1 % 8) - index_bit;
                hemming_bytes1.back() |= bit << shift;
            } else {
                size_t shift = index_bit - (index_hemming_bit1 % 8);
                hemming_bytes1.back() |= bit >> shift;
            }
        }
    }

    std::vector<uint8_t> hemming_bytes2 = HemmingBytes(data, encoding_info);

    uint64_t pos = 0;
    uint32_t cnt_diff = 0;
    for (size_t index_byte = 0; index_byte < hemming_bytes1.size(); index_byte++) {
        if (hemming_bytes1[index_byte] != hemming_bytes2[index_byte]) {
            uint8_t diff = hemming_bytes1[index_byte] ^ hemming_bytes2[index_byte];

            for (size_t index_bit = 0; index_bit < 8; index_bit++) {
                if (diff & (1 << index_bit)) {
                    ++cnt_diff;
                    pos ^= 1LL << (index_byte * 8 + index_bit);
                }
            }
        }
    }

    if (cnt_diff > 1) {
        size_t p = 0;
        while (pos >= (1LL << (p + 1))) {
            ++p;
        }
        pos -= p + 1;
        --pos;

        size_t num_byte = pos / 8;
        size_t ost = pos % 8;
        data[num_byte] ^= 1 << ost;
    }

    return data;
}

std::vector<char> managefile::Hamarc::DecodeData(std::span<char> data, const EncodingInfo &encoding_info) {
    size_t size_block =
        (encoding_info.count_data_bytes + encoding_info.count_additional_bytes) * encoding_info.count_copy;
    size_t count_block = data.size() / size_block;

    std::vector<char> result_data;

    for (size_t index_block = 0; index_block < count_block; index_block++) {
        size_t size_copy = encoding_info.count_data_bytes + encoding_info.count_additional_bytes;

        std::vector<std::vector<char>> copies(encoding_info.count_copy);
        for (size_t index_copy = 0; index_copy < encoding_info.count_copy; index_copy++) {
            std::span<char> encode_data = data.subspan(index_block * size_block + index_copy * size_copy, size_copy);

            copies[index_copy] = DecodeBlock(encode_data, encoding_info);
        }

        std::vector<char> current_data(encoding_info.count_data_bytes);

        if (copies.size() > 1) {
            for (size_t index = 0; index < current_data.size(); index++) {
                std::vector<char> mas_let;
                for (size_t index_copy = 0; index_copy < copies.size(); index_copy++) {
                    mas_let.push_back(copies[index_copy][index]);
                }

                std::sort(mas_let.begin(), mas_let.end());

                char result_let = mas_let[0];
                int result_count_let = 1;
                int current_count_let = 1;
                for (size_t index_let = 1; index_let < mas_let.size(); index_let++) {
                    if (mas_let[index_let] != mas_let[index_let - 1]) {
                        if (current_count_let > result_count_let) {
                            result_let = mas_let[index_let - 1];
                            result_count_let = current_count_let;
                        }

                        current_count_let = 1;
                    } else {
                        ++current_count_let;
                    }
                }

                if (current_count_let > result_count_let) {
                    result_let = mas_let[mas_let.size() - 1];
                    result_count_let = current_count_let;
                }

                current_data[index] = result_let;
            }
        } else {
            current_data = copies[0];
        }

        for (size_t index_current_data = 0; index_current_data < current_data.size(); index_current_data++) {
            result_data.push_back(current_data[index_current_data]);
        }
    }

    return result_data;
}

std::vector<char> managefile::Hamarc::DecodePosBlock(size_t pos, const EncodingInfo &encoding_info) {
    file.SetPos(pos);
    size_t size_block =
        (encoding_info.count_data_bytes + encoding_info.count_additional_bytes) * encoding_info.count_copy;
    std::vector<char> encoded_data = file.Read(size_block);

    std::vector<char> decoded_data = DecodeData(encoded_data, encoding_info);

    return encoded_data;
}

// improve const size block in hat
managefile::Hamarc::Hat managefile::Hamarc::DecodeHat(const size_t pos) {
    file.SetPos(pos);

    std::vector<char> size_file;
    std::vector<char> size_name_file;
    std::vector<char> name_file;
    std::vector<char> encoding_data_file;

    std::vector<char> encoded_size_file =
        file.Read(8 * (config.size_file.count_data_bytes + config.size_file.count_additional_bytes));
    size_file = DecodeData(encoded_size_file, config.size_file);

    std::vector<char> encoded_size_name_file =
        file.Read(4 * (config.size_name_file.count_data_bytes + config.size_name_file.count_additional_bytes));
    size_name_file = DecodeData(encoded_size_name_file, config.size_name_file);
    uint32_t usize_name = 0;
    usize_name += static_cast<uint32_t>(size_name_file[0]) << 24;
    usize_name += static_cast<uint32_t>(size_name_file[1]) << 16;
    usize_name += static_cast<uint32_t>(size_name_file[2]) << 8;
    usize_name += static_cast<uint32_t>(size_name_file[3]) << 0;

    std::vector<char> encoded_name_file =
        file.Read(usize_name * (config.name_file.count_data_bytes + config.name_file.count_additional_bytes));
    name_file = DecodeData(encoded_name_file, config.name_file);

    std::vector<char> encoded_encoding_data_file =
        file.Read(8 * (config.encoding_data_file.count_data_bytes + config.encoding_data_file.count_additional_bytes));
    encoding_data_file = DecodeData(encoded_encoding_data_file, config.encoding_data_file);

    Hat result_hat(size_file, size_name_file, name_file, encoding_data_file);

    return result_hat;
}

managefile::File managefile::Hamarc::DecodeFile(size_t pos, const Hat &hat, size_t buff_size) {
    File tmp_file = File::CreateTempFile();
    file.SetPos(pos);

    EncodingInfo encoding_data(hat.encoding_data_file >> 8, (hat.encoding_data_file << 56) >> 56);

    size_t size_block =
        (encoding_data.count_data_bytes + encoding_data.count_additional_bytes) * encoding_data.count_copy;

    buff_size = buff_size / size_block * size_block;
    size_t size_file = GetSizeFile(hat);
    size_t count_reading = size_file / buff_size;

    for (size_t index_reading = 0; index_reading < count_reading; index_reading++) {
        std::vector<char> buff;
        buff = file.Read(buff_size);
        std::vector<char> decoded_buff = DecodeData(buff, encoding_data);

        tmp_file.PushBack(decoded_buff);
    }

    size_t ost_bytes = count_reading * buff_size;
    ost_bytes = size_file - ost_bytes;

    std::vector<char> buff;
    buff = file.Read(ost_bytes);
    std::vector<char> decoded_buff = DecodeData(buff, encoding_data);

    size_t ost_byte = (ost_bytes / size_block - 1) * encoding_data.count_data_bytes +
                      (hat.size_file % encoding_data.count_data_bytes);
    decoded_buff.resize(ost_byte);

    tmp_file.PushBack(decoded_buff);

    return std::move(tmp_file);
}

size_t managefile::Hamarc::GetSizeFile(const Hat &hat) const {
    EncodingInfo encoding_data(hat.encoding_data_file >> 8, (hat.encoding_data_file << 56) >> 56);

    size_t size_block =
        (encoding_data.count_data_bytes + encoding_data.count_additional_bytes) * encoding_data.count_copy;
    size_t count_block = (hat.size_file * 8) / encoding_data.count_data_bits;
    if ((hat.size_file * 8) % encoding_data.count_data_bits != 0) {
        ++count_block;
    }

    size_t size_file = size_block * count_block;

    return size_file;
}

size_t managefile::Hamarc::GetPosFirstFile() { return begin_file_pos; }

managefile::Hamarc::Hamarc(const std::string &file_path, bool create_if_not_exist, const ConfigEncodingFile &config) {
    if (std::filesystem::exists(file_path) && std::filesystem::file_size(file_path) > 0) {
        file = File(file_path, create_if_not_exist);

        std::vector<std::vector<char>> copies(count_copy_hat_archive);
        std::vector<char> config_archive(32);
        file.SetBegin();
        for (size_t index_copy = 0; index_copy < copies.size(); index_copy++) {
            copies[index_copy] = file.Read(32);
        }

        if (copies.size() > 1) {
            for (size_t index = 0; index < config_archive.size(); index++) {
                std::vector<char> mas_let;
                for (size_t index_copy = 0; index_copy < copies.size(); index_copy++) {
                    mas_let.push_back(copies[index_copy][index]);
                }

                std::sort(mas_let.begin(), mas_let.end());

                char result_let = mas_let[0];
                int result_count_let = 1;
                int current_count_let = 1;
                for (size_t index_let = 1; index_let < mas_let.size(); index_let++) {
                    if (mas_let[index_let] != mas_let[index_let - 1]) {
                        if (current_count_let > result_count_let) {
                            result_let = mas_let[index_let - 1];
                            result_count_let = current_count_let;
                        }

                        current_count_let = 1;
                    } else {
                        ++current_count_let;
                    }
                }

                if (current_count_let > result_count_let) {
                    result_let = mas_let[mas_let.size() - 1];
                    result_count_let = current_count_let;
                }

                config_archive[index] = result_let;
            }
        } else {
            config_archive = copies[0];
        }

        this->config = ConfigEncodingFile(config_archive);
        begin_file_pos = file.Pos();

        count_file = 0;
        size_t current_pos = begin_file_pos;
        while (current_pos < file.Length()) {
            Hat hat = DecodeHat(file.Pos());
            count_file++;
            size_t move_pos = GetSizeFile(hat);
            file.MovePos(move_pos);
            current_pos = file.Pos();
        }
    } else {
        file = File(file_path, create_if_not_exist);
        this->config = config;
        count_file = 0;

        std::vector<char> config_archive;

        config_archive = IntegralToVectorCHar(config.size_file.ToInt(), 8);
        std::vector<char> current = IntegralToVectorCHar(config.size_name_file.ToInt(), 8);
        for (size_t index = 0; index < current.size(); index++) {
            config_archive.push_back(current[index]);
        }
        current = IntegralToVectorCHar(config.name_file.ToInt(), 8);
        for (size_t index = 0; index < current.size(); index++) {
            config_archive.push_back(current[index]);
        }
        current = IntegralToVectorCHar(config.encoding_data_file.ToInt(), 8);
        for (size_t index = 0; index < current.size(); index++) {
            config_archive.push_back(current[index]);
        }

        for (size_t index_copy = 0; index_copy < count_copy_hat_archive; index_copy++) {
            file.PushBack(config_archive);
        }
        begin_file_pos = file.Pos();
    }
}

managefile::Hamarc::ConfigEncodingFile managefile::Hamarc::GetConfig() { return config; }

std::pair<managefile::File, bool> managefile::Hamarc::Get(const std::string &name) {
    File tmp_file;

    file.SetPos(GetPosFirstFile());
    for (size_t index_file = 0; index_file < Length(); index_file++) {
        Hat hat = DecodeHat(file.Pos());
        if (hat.name_file == name) {
            tmp_file = DecodeFile(file.Pos(), hat);

            return {std::move(tmp_file), true};
        }

        size_t size_file = GetSizeFile(hat);

        file.MovePos(size_file);
    }

    return {std::move(tmp_file), false};
}

ssize_t managefile::Hamarc::GetPos(const std::string &name) {
    file.SetPos(GetPosFirstFile());
    for (size_t index_file = 0; index_file < Length(); index_file++) {
        size_t start_file_pos = file.Pos();
        Hat hat = DecodeHat(file.Pos());
        if (hat.name_file == name) {
            return start_file_pos;
        }

        size_t size_file = GetSizeFile(hat);

        file.MovePos(size_file);
    }

    return -1;
}

void managefile::Hamarc::Add(File &file) {
    EncodingInfo encoding_info(256, 1);
    Add(file, encoding_info);
}

void managefile::Hamarc::Add(File &file, const EncodingInfo &encoding_info) {
    File tmp_file = EncodeFile(file, encoding_info);
    this->file.PushBack(tmp_file);
    ++count_file;
}

void managefile::Hamarc::Delete(const std::string &name) {
    ssize_t start_pos = GetPos(name);

    if (start_pos != -1) {
        --count_file;
        file.SetPos(start_pos);

        Hat hat = DecodeHat(start_pos);
        size_t size_file = GetSizeFile(hat);

        size_t current_pos = file.Pos();
        size_t size_delete = current_pos - start_pos + size_file;
        file.Erase(start_pos, size_delete);
    }
}

size_t managefile::Hamarc::Length() const { return count_file; }

std::vector<std::pair<std::string, size_t>> managefile::Hamarc::Info() {
    file.SetPos(GetPosFirstFile());

    std::vector<std::pair<std::string, size_t>> result;
    for (size_t index_file = 0; index_file < Length(); index_file++) {
        Hat hat = DecodeHat(file.Pos());
        result.push_back({hat.name_file, hat.size_file});

        size_t size_file = GetSizeFile(hat);
        file.MovePos(size_file);
    }

    return result;
}

std::vector<managefile::File> managefile::Hamarc::GetAll() {
    file.SetPos(GetPosFirstFile());
    std::vector<File> result(Length());
    for (size_t index_file = 0; index_file < Length(); index_file++) {
        Hat hat = DecodeHat(file.Pos());
        size_t cu_pos = file.Pos();
        File file = std::move(DecodeFile(cu_pos, hat));
        File result_file(hat.name_file, true, false, true);
        result_file.PushBack(file);
        result[index_file] = std::move(result_file);
    }

    return result;
}

bool managefile::Hamarc::Merge(Hamarc &other) {
    if (config == other.config) {
        std::vector<File> files = other.GetAll();
        for (size_t index = 0; index < files.size(); index++) {
            file.PushBack(files[index]);
        }

        return true;
    }
    else{
        std::vector<File> files = other.GetAll();

        for (size_t index = 0; index > files.size(); index++){
            Add(files[index]);
        }
    }

    return false;
}