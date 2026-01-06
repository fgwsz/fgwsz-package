#include"fgwsz_packer.h"

#include<cstdint>   //::std::uint8_t ::std::uint64_t

#include<string>    //::std::string
#include<filesystem>//::std::filesystem
#include<fstream>   //::std::ofstream ::std::ifstream
#include<vector>    //::std::vector
#include<memory>    //::std::unique_ptr

#include"fgwsz_endian.hpp"
#include"fgwsz_except.h"
#include"fgwsz_path.h"
#include"fgwsz_random.hpp"
#include"fgwsz_fstream.h"

namespace fgwsz{

Packer::Packer(::std::filesystem::path const& package_path){
    //检查包路径的父路径是否存在,若不存在则创建父路径
    ::fgwsz::try_create_directories(::fgwsz::parent_path(package_path));
    //检查包路径不为目录路径
    ::fgwsz::path_assert_is_not_directory(package_path);
    //初始化包文件路径字符串(用于抛出异常时的信息显示)
    this->package_path_string_=package_path.generic_string();
    //二进制覆盖方式打开包输出文件路径
    this->package_.open(package_path,::std::ios::binary|::std::ios::trunc);
    //包输出文件路径打开失败
    if(!(this->package_.is_open())){
        FGWSZ_THROW_WHAT(
            "failed to open package file: "+this->package_path_string_
        );
    }
}
Packer::~Packer(void){
    if(this->package_.is_open()){
        this->package_.close();
    }
}
void Packer::pack_file(
    ::std::filesystem::path const& file_path
    ,::std::filesystem::path const& base_dir_path
){
    //========================================================================
    //输入参数检查阶段
    //========================================================================
    ::fgwsz::path_assert_exists(file_path);
    ::fgwsz::path_assert_is_not_directory(file_path);
    ::fgwsz::path_assert_is_not_symlink(file_path);
    ::fgwsz::path_assert_exists(base_dir_path);
    ::fgwsz::path_assert_is_directory(base_dir_path);
    //========================================================================
    //文件头信息处理阶段
    //========================================================================
    //文件头信息包含4部分:
    //[key|relative path bytes|relative path|content bytes]
    //得到文件的密钥
#ifndef _MSC_VER
    auto key=::fgwsz::random<::std::uint8_t>(1,255);
#else
    //MSVC中没有实现特化类型为::std::uint8_t的随机数生成器
    //因此改为使用更大取值范围的无符号整数类型转到::std::uint8_t
    auto key=
        static_cast<::std::uint8_t>(::fgwsz::random<unsigned short>(1,255));
#endif
    //根据基准目录路径和文件路径得到归档之后的相对路径
    auto relative_path=::fgwsz::relative_path(file_path,base_dir_path);
    //检查文件的相对路径是否安全(不安全情况,存在溢出输出目录的风险)
    ::fgwsz::path_assert_is_safe_relative_path(relative_path);
    //得到相对路径和相对路径的所占用的字节数
    ::std::string relative_path_string=relative_path.generic_string();
    ::std::uint64_t relative_path_bytes=relative_path_string.size();
    //得到文件内容字节数
    ::std::uint64_t content_bytes=::std::filesystem::file_size(file_path);
    //构建文件头信息缓冲区
    ::std::vector<char> buffer={};
    buffer.resize(
        sizeof(key)
        +sizeof(relative_path_bytes)
        +relative_path_bytes
        +sizeof(content_bytes)
    );
    //将文件头信息写入缓冲区
    //写入key
    ::std::uint64_t buffer_index=0;
    buffer[0]=key;
    ++buffer_index;
    //写入relative path bytes
    ::fgwsz::EndianHelper<::std::uint64_t> net_endian;
    net_endian.data=
        ::fgwsz::host_to_net<::std::uint64_t>(relative_path_bytes);
    for(::std::uint8_t byte:net_endian.byte_array){
        buffer[buffer_index]=static_cast<char>(byte^key);//使用密钥进行xor混淆
        ++buffer_index;
    }
    //写入relative path
    for(char byte:relative_path_string){
        buffer[buffer_index]=static_cast<char>(
            static_cast<::std::uint8_t>(byte)^key
        );//使用密钥进行xor混淆
        ++buffer_index;
    }
    //写入content bytes
    net_endian.data=
        ::fgwsz::host_to_net<::std::uint64_t>(content_bytes);
    for(::std::uint8_t byte:net_endian.byte_array){
        buffer[buffer_index]=static_cast<char>(byte^key);//使用密钥进行xor混淆
        ++buffer_index;
    }
    //将缓冲区的文件头信息写入包
    ::fgwsz::std_ofstream_write(
        this->package_
        ,buffer.data()
        ,static_cast<::std::streamsize>(buffer.size())
        ,this->package_path_string_
    );
    //========================================================================
    //文件内容信息处理阶段
    //========================================================================
    //二进制方式打开文件
    ::std::ifstream file(file_path,::std::ios::binary);
    ::std::string file_path_string=file_path.generic_string();
    //文件打不开时
    if(!file.is_open()){
        FGWSZ_THROW_WHAT("failed to open file: "+file_path_string);
    }
    //设置内存块
    constexpr ::std::uint64_t block_bytes=1024*1024;//1MB
#ifndef _MSC_VER
    char block[block_bytes];
#else
    //MSVC中栈全部内存默认1MB,直接分配在栈上会导致栈溢出,改为分配在堆上
    auto block_body=::std::make_unique<char[]>(block_bytes);
    char* block=block_body.get();
#endif
    //分块读取文件内容
    ::std::uint64_t count_bytes=0;
    ::std::uint64_t read_bytes=0;
    while(count_bytes<content_bytes){
        //读取文件内容到块中
        read_bytes=static_cast<::std::uint64_t>(
            ::fgwsz::std_ifstream_read(
                file
                ,&(block[0])
                ,static_cast<::std::streamsize>(block_bytes)
                ,file_path_string
            )
        );
        //将内存块中的文件内容信息编码混淆
        for(::std::uint64_t index=0;index<read_bytes;++index){
            block[index]=static_cast<char>(
                key^static_cast<std::uint8_t>(block[index])
            );
        }
        //将内存块中的文件内容信息写入包
        ::fgwsz::std_ofstream_write(
            this->package_
            ,&(block[0])
            ,static_cast<::std::streamsize>(read_bytes)
            ,this->package_path_string_
        );
        //更新字节计数器
        count_bytes+=read_bytes;
    }
    //文件内容读取不完整
    if(count_bytes!=content_bytes){
        FGWSZ_THROW_WHAT("file read incomplete: "+file_path_string);
    }
    file.close();
}
void Packer::pack_dir(::std::filesystem::path const& dir_path){
    //检查路径是否存在
    ::fgwsz::path_assert_exists(dir_path);
    //检查路径是否指向目录
    ::fgwsz::path_assert_is_directory(dir_path);
    //递归遍历所有的子文件进行打包
    ::std::filesystem::file_status status={};
    for(::std::filesystem::directory_entry const& dir_entry
        : ::std::filesystem::recursive_directory_iterator(dir_path)
    ){
        //检查符号状态(不追踪符号链接)
        status=dir_entry.symlink_status();
        //跳过所有子目录
        if(::std::filesystem::is_directory(status)){
            continue;
        }
        //跳过所有符号链接
        if(::std::filesystem::is_symlink(status)){
            continue;
        }
        //只对文件进行打包
        this->pack_file(
            ::std::filesystem::absolute(dir_entry.path())
            ,::fgwsz::parent_path(dir_path)
        );//基准目录路径指向dir_path的父目录,是为了把dir_path本身也打包进去
    }
}
void Packer::pack_path(::std::filesystem::path const& path){
    //检查路径是否存在
    ::fgwsz::path_assert_exists(path);
    //检查路径类型
    if(::std::filesystem::is_directory(path)){//目录路径
        this->pack_dir(path);
    }else{//文件路径
        //为了避免出现如下情况,扩展path为绝对路径:
        //path:"file.txt"
        //  parent_path:"" 
        //  is_directory(parent_path) is false
        //path:"./file.txt"
        //  parent_path:"./"
        //  is_directory(parent_path) is true
        this->pack_file(path,::fgwsz::parent_path(path));
    }
}
void Packer::pack_paths(::std::vector<::std::filesystem::path> const& paths){
    for(auto const& path:paths){
        this->pack_path(path);
    }
}

}//namespace fgwsz
