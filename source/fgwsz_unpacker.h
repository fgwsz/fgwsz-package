#ifndef FGWSZ_UNPACKER_H
#define FGWSZ_UNPACKER_H

#include<cstdint>   //::std::uint8_t ::std::uint64_t

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
    void reset_package(void);
    void unpack_key(::std::uint8_t& key);
    void unpack_relative_path_bytes(
        ::std::uint8_t key
        ,::std::uint64_t& relative_path_bytes
    );
    void unpack_relative_path_string(
        ::std::uint8_t key
        ,::std::uint64_t relative_path_bytes
        ,::std::string& relative_path_string
    );
    void unpack_content_bytes(
        ::std::uint8_t key
        ,::std::uint64_t& content_bytes
    );
    void unpack_header(Header& header);
    void unpack_content(
        ::std::filesystem::path const& output_dir_path
        ,Unpacker::Header const& header
        ,char* block
        ,::std::uint64_t block_bytes
    );
    ::std::ifstream package_;
    ::std::string package_path_string_;
    ::std::uint64_t package_bytes_;
    ::std::uint64_t package_count_bytes_;
};

}//namespace fgwsz

#endif//FGWSZ_UNPACKER_H
