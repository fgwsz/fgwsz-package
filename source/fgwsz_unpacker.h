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
    //禁止拷贝
    Unpacker(Unpacker const&)noexcept=delete;
    Unpacker& operator=(Unpacker const&)noexcept=delete;
private:
    ::std::ifstream package_;
    ::std::string package_path_string_;
    ::std::uint64_t package_bytes_;
};

}//namespace fgwsz

#endif//FGWSZ_UNPACKER_H
