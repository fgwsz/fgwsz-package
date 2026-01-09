#ifndef FGWSZ_UNPACKER_H
#define FGWSZ_UNPACKER_H

#include<cstdint>   //::std::uint64_t

#include<string>    //::std::string
#include<fstream>   //::std::ifstream
#include<filesystem>//::std::filesystem

namespace fgwsz{

class Unpacker{
public:
    Unpacker(::std::filesystem::path const& package_path);
    ~Unpacker(void);
    //解包到指定的输出目录下
    void unpack_package(::std::filesystem::path const& output_dir_path);
    //显示包内的文件信息
    void list_package(void);
    //禁止拷贝
    Unpacker(Unpacker const&)noexcept=delete;
    Unpacker& operator=(Unpacker const&)noexcept=delete;
private:
    struct Header{
        ::std::uint8_t key;
        ::std::uint64_t relative_path_bytes;
        ::std::string relative_path_string;
        ::std::uint64_t content_bytes;
    };
    ::std::uint8_t unpack_key(::std::uint64_t& package_count_bytes);
    ::std::uint64_t unpack_relative_path_bytes(
        ::std::uint64_t& package_count_bytes
        ,::std::uint8_t key
    );
    ::std::string unpack_relative_path_string(
        ::std::uint64_t& package_count_bytes
        ,::std::uint8_t key
        ,::std::uint64_t relative_path_bytes
    );
    ::std::uint64_t unpack_content_bytes(
        ::std::uint64_t& package_count_bytes
        ,::std::uint8_t key
    );
    void unpack_header(
        ::std::uint64_t& package_count_bytes
        ,Header& header
    );
    void unpack_content(
        ::std::uint64_t& package_count_bytes
        ,::std::uint8_t key
        ,char* block
        ,::std::uint64_t block_bytes
        ,::std::filesystem::path const& output_dir_path
        ,::std::string const& relative_path_string
        ,::std::uint64_t content_bytes
    );
    ::std::ifstream package_;
    ::std::string package_path_string_;
    ::std::uint64_t package_bytes_;
};

}//namespace fgwsz

#endif//FGWSZ_UNPACKER_H
