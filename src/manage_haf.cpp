#include "headers/manager_files.hpp"
#include <algorithm>
#include <iostream>

// ---------------------------------------------------------------------------
// EncodingInfo
// ---------------------------------------------------------------------------

managefile::Hamarc::EncodingInfo::EncodingInfo(uint64_t count_bit, uint32_t copy_count) {
    size_t count_additional_bits = 0;
    while ((1ULL << count_additional_bits) <= count_bit + count_additional_bits) {
        ++count_additional_bits;
    }

    count_data_bits      = count_bit;
    count_data_bytes     = count_bit / 8 + (count_bit % 8 != 0 ? 1 : 0);
    count_hemming_bits   = count_additional_bits;
    count_additional_bytes = count_additional_bits / 8 + 1;
    if (count_data_bits % 8 + count_additional_bits % 8 > 8) {
        ++count_additional_bytes;
    }
    count_copy = static_cast<uint8_t>(copy_count);
}

managefile::Hamarc::EncodingInfo::EncodingInfo(std::span<char> arr) {
    uint64_t count_bit = 0;
    for (size_t i = 0; i < 7; ++i) {
        count_bit += static_cast<uint64_t>(static_cast<uint8_t>(arr[i])) << (48 - i * 8);
    }
    uint64_t copy_count = static_cast<uint8_t>(arr[7]);
    *this = EncodingInfo(count_bit, static_cast<uint32_t>(copy_count));
}

managefile::Hamarc::EncodingInfo::EncodingInfo()
    : count_data_bits(0), count_data_bytes(0),
      count_hemming_bits(0), count_additional_bytes(0), count_copy(0) {}

uint32_t managefile::Hamarc::EncodingInfo::ToInt() const {
    return static_cast<uint32_t>((count_data_bits << 8) + (count_copy & 0xFF));
}

bool managefile::Hamarc::EncodingInfo::operator==(const EncodingInfo& other) const {
    return count_data_bits      == other.count_data_bits
        && count_data_bytes     == other.count_data_bytes
        && count_additional_bytes == other.count_additional_bytes
        && count_hemming_bits   == other.count_hemming_bits
        && count_copy           == other.count_copy;
}

// ---------------------------------------------------------------------------
// ConfigEncodingFile
// ---------------------------------------------------------------------------

bool managefile::Hamarc::ConfigEncodingFile::operator==(const ConfigEncodingFile& other) const {
    return size_file           == other.size_file
        && size_name_file      == other.size_name_file
        && name_file           == other.name_file
        && encoding_data_file  == other.encoding_data_file;
}

managefile::Hamarc::ConfigEncodingFile::ConfigEncodingFile()
    : size_file(8, 1), size_name_file(8, 1), name_file(8, 1), encoding_data_file(8, 1) {}

managefile::Hamarc::ConfigEncodingFile::ConfigEncodingFile(EncodingInfo sf, EncodingInfo snf,
                                                           EncodingInfo nf, EncodingInfo edf)
    : size_file(sf), size_name_file(snf), name_file(nf), encoding_data_file(edf) {}

managefile::Hamarc::ConfigEncodingFile::ConfigEncodingFile(std::span<char> arr) {
    auto read8 = [&](size_t offset) {
        char buf[8];
        for (size_t i = 0; i < 8; ++i) {
            buf[i] = arr[offset + i];
        }
        return EncodingInfo(std::span<char>{buf, 8});
    };
    size_file          = read8(0);
    size_name_file     = read8(8);
    name_file          = read8(16);
    encoding_data_file = read8(24);
}

// ---------------------------------------------------------------------------
// Hat
// ---------------------------------------------------------------------------

static uint64_t SpanToUint64(std::span<char> s) {
    uint64_t v = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        v = (v << 8) | static_cast<uint8_t>(s[i]);
    }
    return v;
}

static uint32_t SpanToUint32(std::span<char> s) {
    uint32_t v = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        v = (v << 8) | static_cast<uint8_t>(s[i]);
    }
    return v;
}

managefile::Hamarc::Hat::Hat(std::span<char> sf, std::span<char> snf,
                             std::span<char> nf, std::span<char> edf) {
    size_file      = SpanToUint64(sf);
    size_name_file = SpanToUint32(snf);
    name_file      = std::string(nf.data(), nf.size());
    encoding_data_file = SpanToUint64(edf);
}

managefile::Hamarc::Hat::Hat()
    : size_file(0), size_name_file(0), encoding_data_file(0) {}

// ---------------------------------------------------------------------------
// Hamarc private helpers
// ---------------------------------------------------------------------------

std::vector<char> managefile::Hamarc::IntegralToVectorChar(long long num, size_t count_byte) const {
    std::vector<char> result(count_byte);
    for (size_t i = count_byte; i-- > 0;) {
        result[i] = static_cast<char>(num & 0xFF);
        num >>= 8;
    }
    return result;
}

std::vector<uint8_t> managefile::Hamarc::HemmingBytes(std::span<char> data,
                                                       const EncodingInfo& info) const {
    std::vector<uint8_t> hbytes(info.count_additional_bytes +
                                (info.count_data_bits % 8 != 0 ? 1 : 0), 0);

    auto xor_bit_into_hbytes = [&](size_t pos_1indexed) {
        for (size_t b = 0; b < 8 * hbytes.size(); ++b) {
            if (pos_1indexed & (1ULL << b)) {
                hbytes[b / 8] ^= static_cast<uint8_t>(1 << (b % 8));
            }
        }
    };

    size_t p = 0;
    // full bytes
    for (size_t ib = 0; ib + 1 < data.size(); ++ib) {
        for (size_t bit = 0; bit < 8; ++bit) {
            size_t pos = ib * 8 + bit + p + 1;
            while (pos == (1ULL << p)) { ++p; ++pos; }
            if (data[ib] & (1 << bit)) {
                xor_bit_into_hbytes(pos);
            }
        }
    }
    // last (possibly partial) byte
    size_t ost_bits = info.count_data_bits % 8;
    if (ost_bits == 0) ost_bits = 8;
    size_t last = data.size() - 1;
    for (size_t bit = 0; bit < ost_bits; ++bit) {
        size_t pos = last * 8 + bit + p + 1;
        while (pos == (1ULL << p)) { ++p; ++pos; }
        if (data[last] & (1 << bit)) {
            xor_bit_into_hbytes(pos);
        }
    }

    return hbytes;
}

std::vector<char> managefile::Hamarc::EncodeBlock(std::span<char> block,
                                                   const EncodingInfo& info) const {
    std::vector<char> result(block.begin(), block.end());

    std::vector<uint8_t> hbytes = HemmingBytes(result, info);
    size_t ih = 0; // index into hbytes bit stream

    // Pack Hamming bits after data bits in last data byte
    for (size_t bit = info.count_data_bits % 8;
         bit != 0 && bit < 8 && ih < info.count_hemming_bits; ++bit, ++ih) {
        uint8_t hbit = (hbytes[ih / 8] >> (ih % 8)) & 1;
        result.back() = static_cast<char>(
            static_cast<uint8_t>(result.back()) | (hbit << bit));
    }

    // Remaining Hamming bits in extra bytes
    while (ih < info.count_hemming_bits) {
        result.push_back(0);
        for (size_t bit = 0; bit < 8 && ih < info.count_hemming_bits; ++bit, ++ih) {
            uint8_t hbit = (hbytes[ih / 8] >> (ih % 8)) & 1;
            result.back() = static_cast<char>(
                static_cast<uint8_t>(result.back()) | (hbit << bit));
        }
    }

    // Replicate count_copy times
    std::vector<char> copy = result;
    result.clear();
    for (size_t c = 0; c < info.count_copy; ++c) {
        result.insert(result.end(), copy.begin(), copy.end());
    }
    return result;
}

std::vector<char> managefile::Hamarc::EncodeData(std::span<char> data,
                                                   const EncodingInfo& info) const {
    // Split data into blocks of count_data_bits bits
    struct Splitter {
        std::vector<std::vector<char>> blocks;
        std::vector<char> current;
        size_t cur_bit = 0;
        size_t block_bits;

        explicit Splitter(size_t bb) : block_bits(bb) {}

        void Feed(std::span<char> d) {
            for (char byte : d) {
                for (size_t b = 0; b < 8; ++b, ++cur_bit) {
                    if (cur_bit == block_bits) {
                        blocks.push_back(std::move(current));
                        current.clear();
                        cur_bit = 0;
                    }
                    if (cur_bit % 8 == 0) {
                        current.push_back(0);
                    }
                    if (byte & (1 << b)) {
                        current.back() = static_cast<char>(
                            static_cast<uint8_t>(current.back()) | (1 << (cur_bit % 8)));
                    }
                }
            }
        }

        std::vector<std::vector<char>> Finish(size_t count_data_bytes) {
            if (!current.empty()) {
                blocks.push_back(std::move(current));
            }
            while (blocks.back().size() < count_data_bytes) {
                blocks.back().push_back(0);
            }
            return std::move(blocks);
        }
    };

    Splitter splitter(info.count_data_bits);
    splitter.Feed(data);
    auto blocks = splitter.Finish(info.count_data_bytes);

    std::vector<char> result;
    for (auto& block : blocks) {
        auto encoded = EncodeBlock(block, info);
        result.insert(result.end(), encoded.begin(), encoded.end());
    }
    return result;
}

std::vector<char> managefile::Hamarc::EncodeHat(Hat hat) const {
    auto enc = [&](const std::vector<char>& raw, const EncodingInfo& cfg) {
        return EncodeData(std::span<char>(const_cast<char*>(raw.data()), raw.size()), cfg);
    };

    auto sf  = enc(IntegralToVectorChar(hat.size_file, 8),      config_.size_file);
    auto snf = enc(IntegralToVectorChar(hat.size_name_file, 4),  config_.size_name_file);

    std::vector<char> name_vec(hat.name_file.begin(), hat.name_file.end());
    auto nf  = enc(name_vec, config_.name_file);

    auto edf = enc(IntegralToVectorChar(hat.encoding_data_file, 8), config_.encoding_data_file);

    std::vector<char> result;
    result.insert(result.end(), sf.begin(),  sf.end());
    result.insert(result.end(), snf.begin(), snf.end());
    result.insert(result.end(), nf.begin(),  nf.end());
    result.insert(result.end(), edf.begin(), edf.end());
    return result;
}

managefile::File managefile::Hamarc::EncodeFile(File& src_file, const EncodingInfo& enc_info,
                                                  size_t buff_size) const {
    File tmp = File::CreateTempFile();

    Hat hat;
    hat.size_file      = src_file.Length();
    hat.size_name_file = static_cast<uint32_t>(src_file.Name().size() + src_file.Format().size() + 1);
    hat.name_file      = src_file.Name() + "." + src_file.Format();
    hat.encoding_data_file = enc_info.ToInt();

    auto hat_data = EncodeHat(hat);
    tmp.Write(hat_data);

    // Align buffer to a multiple of 8 bits for clean block splitting
    auto gcd = [](size_t a, size_t b) -> size_t {
        while (b) { size_t t = b; b = a % b; a = t; }
        return a;
    };
    auto lcm = [&gcd](size_t a, size_t b) -> size_t {
        return a / gcd(a, b) * b;
    };
    buff_size = lcm(buff_size, 8);

    src_file.SetBegin();
    while (!src_file.IsEOF()) {
        auto chunk = src_file.Read(buff_size);
        if (chunk.empty()) break;
        auto encoded = EncodeData(chunk, enc_info);
        tmp.Write(encoded);
    }

    return tmp;
}

// ---------------------------------------------------------------------------
// Decode helpers
// ---------------------------------------------------------------------------

std::vector<char> managefile::Hamarc::DecodeBlock(std::span<char> block,
                                                    const EncodingInfo& info) const {
    std::vector<char> data(info.count_data_bytes);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = block[i];
    }

    // Extract Hamming bits from packed position
    std::vector<uint8_t> hbytes_stored;
    size_t ih = 0;

    if (info.count_data_bits % 8 != 0) {
        // Zero out the hamming bits in the last data byte
        data.back() = 0;
        for (size_t bit = 0; bit < info.count_data_bits % 8; ++bit) {
            data.back() = static_cast<char>(
                static_cast<uint8_t>(data.back()) | (static_cast<uint8_t>(block[data.size()-1]) & (1 << bit)));
        }

        hbytes_stored.push_back(0);
        for (size_t bit = info.count_data_bits % 8;
             bit < 8 && ih < info.count_hemming_bits; ++bit, ++ih) {
            uint8_t hbit = (static_cast<uint8_t>(block[data.size()-1]) >> bit) & 1;
            hbytes_stored.back() = static_cast<uint8_t>(hbytes_stored.back()) | static_cast<uint8_t>(hbit << ih);
        }
    }

    for (size_t i = data.size(); i < block.size() && ih < info.count_hemming_bits; ++i) {
        for (size_t bit = 0; bit < 8 && ih < info.count_hemming_bits; ++bit, ++ih) {
            if (hbytes_stored.size() * 8 <= ih) {
                hbytes_stored.push_back(0);
            }
            uint8_t hbit = (static_cast<uint8_t>(block[i]) >> bit) & 1;
            hbytes_stored.back() = static_cast<uint8_t>(hbytes_stored.back()) | static_cast<uint8_t>(hbit << (ih % 8));
        }
    }

    std::vector<uint8_t> hbytes_computed = HemmingBytes(data, info);

    // Syndrome
    uint64_t syndrome = 0;
    uint32_t cnt_diff = 0;
    for (size_t ib = 0; ib < std::max(hbytes_stored.size(), hbytes_computed.size()); ++ib) {
        uint8_t s = (ib < hbytes_stored.size()   ? hbytes_stored[ib]   : 0)
                  ^ (ib < hbytes_computed.size() ? hbytes_computed[ib] : 0);
        for (size_t bit = 0; bit < 8; ++bit) {
            if (s & (1 << bit)) {
                ++cnt_diff;
                syndrome ^= 1ULL << (ib * 8 + bit);
            }
        }
    }

    // Single-bit error correction
    if (cnt_diff > 1 && syndrome > 0) {
        // syndrome is 1-indexed position in the Hamming code
        // Convert to data bit position
        size_t p = 0;
        size_t pos = syndrome;
        // count how many powers-of-2 are <= pos
        size_t data_pos = pos;
        for (size_t pw = 0; (1ULL << pw) <= pos; ++pw) {
            if (pos & (1ULL << pw)) {
                // it's a parity bit position, skip
            } else {
                --data_pos;
            }
        }
        // data_pos is now 1-indexed data bit
        --data_pos;
        size_t byte_idx = data_pos / 8;
        size_t bit_idx  = data_pos % 8;
        if (byte_idx < data.size()) {
            data[byte_idx] = static_cast<char>(
                static_cast<uint8_t>(data[byte_idx]) ^ static_cast<uint8_t>(1 << bit_idx));
        }
    }

    return data;
}

std::vector<char> managefile::Hamarc::DecodeData(std::span<char> data,
                                                   const EncodingInfo& info) {
    size_t size_copy  = info.count_data_bytes + info.count_additional_bytes;
    size_t size_block = size_copy * info.count_copy;
    if (size_block == 0) return {};

    size_t count_block = data.size() / size_block;
    std::vector<char> result;

    for (size_t ib = 0; ib < count_block; ++ib) {
        std::vector<std::vector<char>> copies(info.count_copy);
        for (size_t ic = 0; ic < info.count_copy; ++ic) {
            auto sub = data.subspan(ib * size_block + ic * size_copy, size_copy);
            copies[ic] = DecodeBlock(sub, info);
        }

        std::vector<char> chosen(info.count_data_bytes);
        if (copies.size() == 1) {
            chosen = copies[0];
        } else {
            // Majority vote per byte
            for (size_t i = 0; i < info.count_data_bytes; ++i) {
                std::vector<char> votes;
                votes.reserve(copies.size());
                for (auto& c : copies) votes.push_back(c[i]);
                std::sort(votes.begin(), votes.end());

                char best = votes[0];
                int  best_cnt = 1, cur_cnt = 1;
                for (size_t k = 1; k < votes.size(); ++k) {
                    if (votes[k] == votes[k-1]) {
                        ++cur_cnt;
                    } else {
                        cur_cnt = 1;
                    }
                    if (cur_cnt > best_cnt) {
                        best_cnt = cur_cnt;
                        best = votes[k];
                    }
                }
                chosen[i] = best;
            }
        }

        result.insert(result.end(), chosen.begin(), chosen.end());
    }

    return result;
}

managefile::Hamarc::Hat managefile::Hamarc::DecodeHat(size_t pos) {
    file.SetPos(pos);

    auto read_field = [&](size_t count_bytes, const EncodingInfo& cfg) {
        size_t encoded_bytes = count_bytes *
            (cfg.count_data_bytes + cfg.count_additional_bytes) * cfg.count_copy;
        auto raw = file.Read(encoded_bytes);
        return DecodeData(raw, cfg);
    };

    auto sf   = read_field(8, config_.size_file);
    auto snf_raw = read_field(4, config_.size_name_file);

    uint32_t name_len = SpanToUint32(snf_raw);

    // encoded name
    size_t enc_name_bytes = name_len *
        (config_.name_file.count_data_bytes + config_.name_file.count_additional_bytes) * config_.name_file.count_copy;
    auto enc_name = file.Read(enc_name_bytes);
    auto nf = DecodeData(enc_name, config_.name_file);

    auto edf = read_field(8, config_.encoding_data_file);

    return Hat(sf, snf_raw, nf, edf);
}

managefile::File managefile::Hamarc::DecodeFile(size_t pos, const Hat& hat, size_t buff_size) {
    File tmp = File::CreateTempFile();
    file.SetPos(pos);

    EncodingInfo enc(hat.encoding_data_file >> 8,
                     static_cast<uint32_t>((hat.encoding_data_file) & 0xFF));

    size_t size_block = (enc.count_data_bytes + enc.count_additional_bytes) * enc.count_copy;
    if (size_block == 0) return tmp;

    // Align buffer to block boundary
    buff_size = (buff_size / size_block) * size_block;
    if (buff_size == 0) buff_size = size_block;

    size_t encoded_size = GetSizeFile(hat);
    size_t bytes_read = 0;
    size_t total_data_bytes = 0; // track decoded bytes to trim at end

    while (bytes_read < encoded_size) {
        size_t to_read = std::min(buff_size, encoded_size - bytes_read);
        auto chunk = file.Read(to_read);
        bytes_read += chunk.size();

        auto decoded = DecodeData(chunk, enc);
        tmp.PushBack(decoded);
        total_data_bytes += decoded.size();
    }

    // Trim to original file size
    if (total_data_bytes > hat.size_file) {
        tmp.PopBack(total_data_bytes - hat.size_file);
    }

    tmp.SetBegin();
    return tmp;
}

size_t managefile::Hamarc::GetSizeFile(const Hat& hat) const {
    EncodingInfo enc(hat.encoding_data_file >> 8,
                     static_cast<uint32_t>((hat.encoding_data_file) & 0xFF));

    size_t size_block = (enc.count_data_bytes + enc.count_additional_bytes) * enc.count_copy;
    size_t count_block = (hat.size_file * 8) / enc.count_data_bits;
    if ((hat.size_file * 8) % enc.count_data_bits != 0) {
        ++count_block;
    }
    return size_block * count_block;
}

size_t managefile::Hamarc::GetPosFirstFile() const {
    return begin_file_pos_;
}

// ---------------------------------------------------------------------------
// Hamarc constructor
// ---------------------------------------------------------------------------

managefile::Hamarc::Hamarc(const std::string& file_path, bool create_if_not_exist,
                             const ConfigEncodingFile& config)
    : count_file_(0), begin_file_pos_(0) {
    if (std::filesystem::exists(file_path) && std::filesystem::file_size(file_path) > 0) {
        file = File(file_path, create_if_not_exist);

        // Read config with majority vote across kCountCopyHatArchive copies
        std::vector<std::vector<char>> copies(kCountCopyHatArchive);
        file.SetBegin();
        for (auto& c : copies) {
            c = file.Read(32);
        }

        std::vector<char> config_bytes(32);
        for (size_t i = 0; i < 32; ++i) {
            std::vector<char> votes;
            for (auto& c : copies) if (i < c.size()) votes.push_back(c[i]);
            std::sort(votes.begin(), votes.end());
            char best = votes[0]; int best_cnt = 1, cur = 1;
            for (size_t k = 1; k < votes.size(); ++k) {
                cur = (votes[k] == votes[k-1]) ? cur + 1 : 1;
                if (cur > best_cnt) { best_cnt = cur; best = votes[k]; }
            }
            config_bytes[i] = best;
        }

        config_ = ConfigEncodingFile(std::span<char>(config_bytes));
        begin_file_pos_ = static_cast<size_t>(file.Pos());

        // Count files
        size_t cur_pos = begin_file_pos_;
        size_t file_len = file.Length();
        while (cur_pos < file_len) {
            Hat hat = DecodeHat(cur_pos);
            ++count_file_;
            size_t sz = GetSizeFile(hat);
            file.MovePos(static_cast<ssize_t>(sz));
            cur_pos = static_cast<size_t>(file.Pos());
        }
    } else {
        file = File(file_path, create_if_not_exist);
        config_ = config;
        count_file_ = 0;

        // Write config × kCountCopyHatArchive
        auto append_int = [&](long long v, size_t n) {
            auto bytes = IntegralToVectorChar(v, n);
            return bytes;
        };

        std::vector<char> config_bytes;
        auto push = [&](const std::vector<char>& v) {
            config_bytes.insert(config_bytes.end(), v.begin(), v.end());
        };
        push(append_int(config.size_file.ToInt(),          8));
        push(append_int(config.size_name_file.ToInt(),     8));
        push(append_int(config.name_file.ToInt(),          8));
        push(append_int(config.encoding_data_file.ToInt(), 8));

        for (size_t i = 0; i < kCountCopyHatArchive; ++i) {
            file.PushBack(config_bytes);
        }
        begin_file_pos_ = static_cast<size_t>(file.Pos());
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

managefile::Hamarc::ConfigEncodingFile managefile::Hamarc::GetConfig() const {
    return config_;
}

std::pair<managefile::File, bool> managefile::Hamarc::Get(const std::string& name) {
    file.SetPos(GetPosFirstFile());
    for (size_t i = 0; i < count_file_; ++i) {
        size_t hat_pos = static_cast<size_t>(file.Pos());
        Hat hat = DecodeHat(hat_pos);
        if (hat.name_file == name) {
            File extracted = DecodeFile(static_cast<size_t>(file.Pos()), hat);
            return {std::move(extracted), true};
        }
        file.MovePos(static_cast<ssize_t>(GetSizeFile(hat)));
    }
    return {File{}, false};
}

ssize_t managefile::Hamarc::GetPos(const std::string& name) {
    file.SetPos(GetPosFirstFile());
    for (size_t i = 0; i < count_file_; ++i) {
        size_t hat_start = static_cast<size_t>(file.Pos());
        Hat hat = DecodeHat(hat_start);
        if (hat.name_file == name) {
            return static_cast<ssize_t>(hat_start);
        }
        file.MovePos(static_cast<ssize_t>(GetSizeFile(hat)));
    }
    return -1;
}

void managefile::Hamarc::Add(File& f) {
    EncodingInfo default_enc(256, 1);
    Add(f, default_enc);
}

void managefile::Hamarc::Add(File& f, const EncodingInfo& enc_info) {
    File tmp = EncodeFile(f, enc_info);
    file.PushBack(tmp);
    ++count_file_;
}

void managefile::Hamarc::Delete(const std::string& name) {
    ssize_t start = GetPos(name);
    if (start == -1) {
        std::cerr << "hamarc: file not found in archive: " << name << "\n";
        return;
    }

    size_t start_pos = static_cast<size_t>(start);
    Hat hat = DecodeHat(start_pos);
    size_t after_hat = static_cast<size_t>(file.Pos());
    size_t size_data = GetSizeFile(hat);
    size_t total_delete = (after_hat - start_pos) + size_data;

    file.Erase(start_pos, total_delete);
    --count_file_;
}

size_t managefile::Hamarc::Length() const {
    return count_file_;
}

std::vector<std::pair<std::string, size_t>> managefile::Hamarc::Info() {
    file.SetPos(GetPosFirstFile());
    std::vector<std::pair<std::string, size_t>> result;
    for (size_t i = 0; i < count_file_; ++i) {
        Hat hat = DecodeHat(static_cast<size_t>(file.Pos()));
        result.emplace_back(hat.name_file, hat.size_file);
        file.MovePos(static_cast<ssize_t>(GetSizeFile(hat)));
    }
    return result;
}

std::vector<managefile::File> managefile::Hamarc::GetAll() {
    file.SetPos(GetPosFirstFile());
    std::vector<File> result;
    result.reserve(count_file_);

    for (size_t i = 0; i < count_file_; ++i) {
        size_t hat_pos = static_cast<size_t>(file.Pos());
        Hat hat = DecodeHat(hat_pos);
        size_t data_pos = static_cast<size_t>(file.Pos());

        File decoded = DecodeFile(data_pos, hat);

        File out(hat.name_file, true, false, true);
        out.PushBack(decoded);
        out.SetBegin();
        result.push_back(std::move(out));

        file.SetPos(hat_pos);
        Hat hat2 = DecodeHat(hat_pos);
        file.MovePos(static_cast<ssize_t>(GetSizeFile(hat2)));
    }

    return result;
}

bool managefile::Hamarc::Merge(Hamarc& other) {
    if (!(GetConfig() == other.GetConfig())) {
        std::cerr << "hamarc: cannot merge archives with different encoding configs\n";
        return false;
    }
    auto files = other.GetAll();
    for (auto& f : files) {
        f.UnMakeTemp();
        Add(f);
    }
    return true;
}
