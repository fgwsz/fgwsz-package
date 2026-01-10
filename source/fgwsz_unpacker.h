#ifndef FGWSZ_UNPACKER_H
#define FGWSZ_UNPACKER_H

#include<cstdint>   //::std::uint8_t ::std::uint64_t

#include<string>    //::std::string
#include<fstream>   //::std::ifstream
#include<filesystem>//::std::filesystem

#include"fgwsz_header.h"

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
    void reset_package(void);
    ::std::uint64_t package_read(void* ptr,::std::uint64_t bytes);
    void key_xor(void* ptr,::std::uint64_t bytes);
    void unpack_key(void);
    void unpack_relative_path_bytes(void);
    void unpack_relative_path_string(void);
    void unpack_content_bytes(void);
    void unpack_header(void);
    void unpack_content(
        ::std::filesystem::path const& output_dir_path
        ,char* block
        ,::std::uint64_t block_bytes
    );
    ::std::ifstream package_;
    ::std::string package_path_string_;
    ::std::uint64_t package_bytes_;
    ::std::uint64_t package_count_bytes_;
    ::fgwsz::Header header_;
};

}//namespace fgwsz

#endif//FGWSZ_UNPACKER_H
