#pragma once

#include <string>
#include <vector>
#include "manager_files.hpp"

namespace argparser {

/*
 * Парсер аргументов командной строки.
 *
 * Поддерживаемые флаги:
 *   -c, --create                          создать новый архив
 *   -f <файл>, --file=<файл>              имя файла архива
 *   -l, --list                            вывести список файлов в архиве
 *   -x, --extract                         извлечь файлы из архива
 *   -a, --append                          добавить файл в архив
 *   -d, --delete                          удалить файл из архива
 *   -A, --concatenate                     объединить два архива
 *
 * Флаги настройки кодирования (count_bits count_copies):
 *   --encoding-data <bits> <copies>       настройки кодирования данных файла
 *   --encoding-size-file <bits> <copies>  настройки кодирования размера файла
 *   --encoding-size-name <bits> <copies>  настройки кодирования размера имени
 *   --encoding-name <bits> <copies>       настройки кодирования имени файла
 *   --encoding-meta <bits> <copies>       настройки кодирования метаданных
 *
 * Краткие алиасы флагов кодирования:
 *   -e-d, -e-sf, -e-sn, -e-nf, -e-m
 *
 * Имена файлов передаются как свободные аргументы.
 * Имена архивов для --concatenate — свободные аргументы с расширением .haf
 */
struct Parser {
    bool create;
    bool list;
    bool extract;
    bool insert;
    bool erase;
    bool concatenate;

    managefile::Hamarc::EncodingInfo encoding_data;
    managefile::Hamarc::EncodingInfo encoding_size_file;
    managefile::Hamarc::EncodingInfo encoding_size_name_file;
    managefile::Hamarc::EncodingInfo encoding_name_file;
    managefile::Hamarc::EncodingInfo encoding_encoding_data_file;

    std::vector<std::string> files;
    std::vector<std::string> archives;
    std::string archive;

    Parser();

    void Parse(int argc, char** argv);

  private:
    static std::string ArgToString(char* arr);
    managefile::Hamarc::EncodingInfo ParseEncodingArg(int& index, int argc, char** argv);
};

} // namespace argparser
