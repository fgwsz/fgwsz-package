#include"fgwsz_unpacker.h"

#include<cstdint>   //::std::uint8_t ::std::uint64_t

#include<string>    //::std::string
#include<filesystem>//::std::filesystem
#include<fstream>   //::std::ofstream ::std::ifstream
#include<vector>    //::std::vector
#include<memory>    //::std::unique_ptr

#include"fgwsz_endian.hpp"
#include"fgwsz_except.h"
#include"fgwsz_path.h"
#include"fgwsz_fstream.h"

namespace fgwsz{

Unpacker::Unpacker(::std::filesystem::path const& package_path){
    //检查包路径是否存在
    ::fgwsz::path_assert_exists(package_path);
    //检查包路径不为目录路径
    ::fgwsz::path_assert_is_not_directory(package_path);
    //初始化包文件路径字符串(用于抛出异常时的信息显示)
    this->package_path_string_=package_path.generic_string();
    //二进制方式打开包文件
    this->package_.open(package_path,::std::ios::binary);
    //包文件打开失败
    if(!(this->package_.is_open())){
        FGWSZ_THROW_WHAT(
            "failed to open package file: "+this->package_path_string_
        );
    }
    //包文件的大小
    this->package_bytes_=::std::filesystem::file_size(package_path);
}
Unpacker::~Unpacker(void){
    if(this->package_.is_open()){
        this->package_.close();
    }
}
void Unpacker::unpack_package(::std::filesystem::path const& output_dir_path){
    //========================================================================
    //输入参数检查阶段
    //========================================================================
    ::fgwsz::try_create_directories(output_dir_path);
    ::fgwsz::path_assert_is_directory(output_dir_path);

    //用于读取文件头信息的缓冲区
    ::std::vector<char> buffer={};
    //用于读取文件内容信息的内存块
    constexpr ::std::uint64_t block_bytes=1024*1024;//1MB
#ifndef _MSC_VER
    char block[block_bytes];
#else
    //MSVC中栈全部内存默认1MB,直接分配在栈上会导致栈溢出,改为分配在堆上
    auto block_body=::std::make_unique<char[]>(block_bytes);
    char* block=block_body.get();
#endif
    //用于记录已读取包内容字节数的计数器
    ::std::uint64_t package_count_bytes=0;
    //用于记录已读取/写入文件内容字节数的计数器
    ::std::uint64_t file_count_bytes=0;
    //用于实时记录每次读取的包内容字节数
    ::std::uint64_t read_bytes=0;
    //文件头信息变量
    ::std::uint8_t key=0;
    ::std::uint64_t relative_path_bytes=0;
    ::std::string relative_path_string={};
    ::std::uint64_t content_bytes=0;
    //字节序辅助结构体
    ::fgwsz::EndianHelper<::std::uint64_t> net;
    //文件路径
    ::std::filesystem::path file_path={};
    ::std::string file_path_string={};
    //文件输出流对象
    ::std::ofstream file={};
    while(package_count_bytes<(this->package_bytes_)){
        //====================================================================
        //文件头信息处理阶段
        //====================================================================
        //读取key
        buffer.resize(sizeof(key));
        read_bytes=::fgwsz::std_ifstream_read(
            this->package_
            ,buffer.data()
            ,sizeof(key)
            ,this->package_path_string_
        );
        if(sizeof(key)!=read_bytes){
            FGWSZ_THROW_WHAT(
                "failed to read key: "+this->package_path_string_
            );
        }
        package_count_bytes+=static_cast<::std::uint64_t>(read_bytes);
        key=static_cast<::std::uint8_t>(buffer[0]);
        //读取relative path bytes
        buffer.resize(sizeof(relative_path_bytes));
        read_bytes=::fgwsz::std_ifstream_read(
            this->package_
            ,buffer.data()
            ,sizeof(relative_path_bytes)
            ,this->package_path_string_
        );
        if(sizeof(relative_path_bytes)!=read_bytes){
            FGWSZ_THROW_WHAT(
                "failed to read relative path bytes: "
                +this->package_path_string_
            );
        }
        package_count_bytes+=static_cast<::std::uint64_t>(read_bytes);
        //解码relative path bytes的文件密钥xor混淆
        for(::std::uint8_t index=0;index<sizeof(::std::uint64_t);++index){
            net.byte_array[index]=
                static_cast<::std::uint8_t>(buffer[index])^key;
        }
        //将网络序转换为主机序,得到relative path bytes
        relative_path_bytes=::fgwsz::net_to_host<::std::uint64_t>(net.data);
        //读取relative path
        buffer.resize(relative_path_bytes);
        read_bytes=::fgwsz::std_ifstream_read(
            this->package_
            ,buffer.data()
            ,relative_path_bytes
            ,this->package_path_string_
        );
        if(relative_path_bytes!=read_bytes){
            FGWSZ_THROW_WHAT(
                "failed to read relative path: "+this->package_path_string_
            );
        }
        package_count_bytes+=static_cast<::std::uint64_t>(read_bytes);
        //解码relative path的文件密钥xor混淆
        relative_path_string.clear();
        relative_path_string.reserve(relative_path_bytes);
        for(char ch:buffer){
            relative_path_string.push_back(
                static_cast<char>(static_cast<::std::uint8_t>(ch)^key)
            );
        }
        ::fgwsz::path_assert_is_safe_relative_path(relative_path_string);
        //读取content bytes
        buffer.resize(sizeof(content_bytes));
        read_bytes=::fgwsz::std_ifstream_read(
            this->package_
            ,buffer.data()
            ,sizeof(content_bytes)
            ,this->package_path_string_
        );
        if(sizeof(content_bytes)!=read_bytes){
            FGWSZ_THROW_WHAT(
                "failed to read content bytes: "+this->package_path_string_
            );
        }
        package_count_bytes+=static_cast<::std::uint64_t>(read_bytes);
        //解码content bytes的文件密钥xor混淆
        for(::std::uint64_t index=0;index<sizeof(::std::uint64_t);++index){
            net.byte_array[index]=
                static_cast<::std::uint8_t>(buffer[index])^key;
        }
        //将网络序转换为主机序,得到content bytes
        content_bytes=::fgwsz::net_to_host<::std::uint64_t>(net.data);
        //====================================================================
        //文件内容信息处理阶段
        //====================================================================
        //创建文件父目录
        file_path=
            ::std::filesystem::absolute(output_dir_path/relative_path_string);
        ::fgwsz::try_create_directories(
            ::fgwsz::parent_path(file_path)
        );
        //文件输出流关联文件路径
        file.open(file_path,::std::ios::binary|::std::ios::trunc);
        file_path_string=file_path.generic_string();
        if(!file.is_open()){
            FGWSZ_THROW_WHAT("file isn't open: "+file_path_string);
        }
        //分块读取content
        file_count_bytes=0;
        while(file_count_bytes<content_bytes){
            read_bytes=::fgwsz::std_ifstream_read(
                this->package_
                ,&(block[0])
                ,((block_bytes>(content_bytes-file_count_bytes)
                    ?(content_bytes-file_count_bytes)
                    :block_bytes))
                ,this->package_path_string_
            );
            package_count_bytes+=read_bytes;
            //解码content的文件密钥xor混淆
            for(::std::uint64_t index=0;index<read_bytes;++index){
                block[index]=static_cast<char>(
                    static_cast<::std::uint8_t>(block[index])^key
                );
            }
            //将分块读取的content写入文件
            ::fgwsz::std_ofstream_write(
                file
                ,&(block[0])
                ,read_bytes
                ,file_path_string
            );
            file_count_bytes+=read_bytes;
        }
        if(file_count_bytes!=content_bytes){
            FGWSZ_THROW_WHAT("file write incomplete: "+file_path_string);
        }
        //关闭文件输出流与当前文件路径的关联
        file.close();
    }
    if(package_count_bytes!=this->package_bytes_){
        FGWSZ_THROW_WHAT(
            "package read incomplete: "+this->package_path_string_
        );
    }
}

}//namespace fgwsz
