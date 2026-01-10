#ifndef FGWSZ_PACKER_H
#define FGWSZ_PACKER_H

#include<string>    //::std::string
#include<filesystem>//::std::filesystem
#include<fstream>   //::std::ofstream ::std::ifstream
#include<vector>    //::std::vector
#include<memory>    //::std::unique_ptr

#include"fgwsz_header.h"

namespace fgwsz{

class Packer{
public:
    //生命周期
    Packer(::std::filesystem::path const& package_path);
    ~Packer(void);
    //打包多个路径(目录/文件)到包
    void pack_paths(::std::vector<::std::filesystem::path> const& paths);
    //禁止拷贝
    Packer(Packer const&)noexcept=delete;
    Packer& operator=(Packer const&)noexcept=delete;
private:
    void pack_key(void);
    void pack_relative_path(
        ::std::filesystem::path const& file_path
        ,::std::filesystem::path const& base_dir_path
    );
    void pack_content_bytes(::std::filesystem::path const& file_path);
    void pack_header(
        ::std::filesystem::path const& file_path
        ,::std::filesystem::path const& base_dir_path
    );
    void pack_content(::std::filesystem::path const& file_path);
    void pack_file(
        ::std::filesystem::path const& file_path
        ,::std::filesystem::path const& base_dir_path
    );
    void pack_dir(::std::filesystem::path const& dir_path);
    void pack_path(::std::filesystem::path const& path);
    ::std::ofstream package_;
    ::std::string package_path_string_;
    ::fgwsz::Header header_;
    ::std::uint64_t content_bytes_;
    static constexpr ::std::uint64_t block_bytes_=1024*1024;//1MB
    ::std::unique_ptr<char[]> block_;
};

}//namespace fgwsz

#endif//FGWSZ_PACKER_H
