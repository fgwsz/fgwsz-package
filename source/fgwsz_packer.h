#ifndef FGWSZ_PACKER_H
#define FGWSZ_PACKER_H

#include<string>    //::std::string
#include<filesystem>//::std::filesystem
#include<fstream>   //::std::ofstream ::std::ifstream
#include<vector>    //::std::vector

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
    void pack_file(
        ::std::filesystem::path const& file_path
        ,::std::filesystem::path const& base_dir_path
    );
    void pack_dir(::std::filesystem::path const& dir_path);
    ::std::ofstream package_;
    ::std::string package_path_string_;
    void pack_path(::std::filesystem::path const& path);
};

}//namespace fgwsz

#endif//FGWSZ_PACKER_H
