#include<cstddef>   //::std::size_t
#include<cstdint>   //::std::uint8_t ::std::uint16_t ::std::uint64_t
#include<ios>       //::std::ios ::std::streamsize
#include<iostream>  //::std::cout ::std::cin ::std::cerr
#include<iterator>  //::std::make_move_iterator
#include<string>    //::std::string
#include<vector>    //::std::vector
#include<fstream>   //::std::ifstream ::std::ofstream
#include<exception> //::std::exception
#include<stdexcept> //::std::runtime_error
#include<filesystem>//::std::filesystem

//作者制定了归档一个或多个(文件/目录)的包标准:
//  包的二进制的文件结构如下:
//      [file item 1]...[file item N]
//  每个文件的二进制信息[file item]的文件结构是[A|B|C|D]:
//      A部分是[path_size(8字节)]
//      B部分是[relative_path]
//      C部分是[content_size(8字节)]
//      D部分是[content(binary)]

//文件结构体
struct File{
    ::std::filesystem::path relative_path;//归档之后的相对路径
    ::std::vector<char> content;//文件内容
    //生命周期
    File(
        ::std::filesystem::path const& file_relative_path
        ,::std::vector<char> const& file_content
    )noexcept
        :relative_path(file_relative_path)
        ,content(file_content)
    {}
    File(File const&)noexcept=default;
    File& operator=(File const&)noexcept=default;
    //移动语义
    File(File&&)noexcept=default;
    File& operator=(File&&)noexcept=default;
};
//打包模式
//通过文件路径构造文件结构体
::File read_file(
    ::std::filesystem::path const& file_path
    ,::std::filesystem::path const& base_path
){
    //检查输入参数有效性
    ::std::ifstream file(file_path,::std::ios::binary|::std::ios::ate);
    //文件打不开
    if(!file.is_open()){
        throw ::std::runtime_error("Failed to open file:"+file_path.string());
    }
    //基准路径不是目录
    if(!::std::filesystem::is_directory(base_path)){
        throw ::std::runtime_error(
            "Base path isn't directory:"+base_path.string()
        );
    }
    //根据基准路路径和文件路径得到归档之后的相对路径
    auto relative_path=::std::filesystem::proximate(file_path,base_path);
    //设置返回值初始值
    auto ret=::File{relative_path,::std::vector<char>{}};
    //读取文件内容
    ::std::size_t file_size=file.tellg();
    //文件内容为空,提前返回
    if(0==file_size){
        return ret;
    }
    file.seekg(0);
    //预先分配内存,减少内存分配次数
    ret.content.resize(file_size);
    file.read(
        ret.content.data()
        ,static_cast<::std::streamsize>(file_size)
    );
    //文件内容读取不完整
    if(file.gcount()!=file_size){
        throw ::std::runtime_error(
            "File read incomplete:"+file_path.string()
        );
    }
    return ret;
}
//打包模式
//使用目录路径来构造文件结构体序列
::std::vector<::File> read_dir(::std::filesystem::path const& dir_path){
    //检查输入路径是否为目录
    if(!::std::filesystem::is_directory(dir_path)){
        throw ::std::runtime_error(
            "Dir path isn't directory:"+dir_path.string()
        );
    }
    //设置返回值初始值
    ::std::vector<::File> ret={};
    //递归遍历所有的文件进行读取
    try{
        for(::std::filesystem::directory_entry const& dir_entry
            : ::std::filesystem::recursive_directory_iterator(dir_path)
        ){
            //对所有的文件进行读取
            if(!::std::filesystem::is_directory(dir_entry.path())){
                ret.emplace_back(::read_file(dir_entry.path(),dir_path));
            }
        }
    }catch(::std::filesystem::filesystem_error const& e){
        throw ::std::runtime_error(
            "Failed to read files from directory:"+dir_path.string()
        );
    }
    return ret;
}
//打包模式
//使用路径(文件路径/目录路径)来构造文件结构体序列
::std::vector<::File> read_path(::std::filesystem::path const& path){
    //检查输入路径类型
    if(::std::filesystem::is_directory(path)){//目录路径
        return ::read_dir(path);
    }else{//文件路径
        ::std::vector<::File> ret={};
        //为了避免出现如下情况,扩展path为绝对路径:
        //path:"file.txt"
        //  parent_path:"" 
        //  is_directory(parent_path) is false
        //path:"./file.txt"
        //  parent_path:"./"
        //  is_directory(parent_path) is true
        auto absolute_path=::std::filesystem::absolute(path);
        ret.emplace_back(
            ::read_file(absolute_path,absolute_path.parent_path())
        );
        return ret;
    }
}
//打包模式
//使用路径序列来构造文件结构体序列
::std::vector<::File> read_paths(
    ::std::vector<::std::filesystem::path> const& paths
){
    ::std::vector<::File> ret={};
    for(auto const& path:paths){
        ::std::vector<::File> files=::read_path(path);
        ret.insert(ret.end()
            ,::std::make_move_iterator(files.begin())
            ,::std::make_move_iterator(files.end())
        );
    }
    return ret;
}
//字节序辅助类
template<typename UnsignedType>
union EndianHelper{
    ::std::uint8_t byte_array[sizeof(UnsignedType)];
    UnsignedType number;
};
//判断主机字节序是否为小端序
bool is_little_endian(void){
    ::EndianHelper<::std::uint16_t> test;
    test.number=0x1234;
    //小端序:低位放在低地址
    //所以,低8位0x34放在低地址&(byte_array[0])
    return (test.byte_array[0]==0x34)&&(test.byte_array[1]==0x12);
}
//判断主机字节序是否为大端序
bool is_big_endian(void){
    return !::is_little_endian();
}
//将主机字节序64位无符号整数转换为同类型的网络字节序
::std::uint64_t host_to_net_uint64(::std::uint64_t host_number){
    if(::is_big_endian()){
        return host_number;
    }
    ::EndianHelper<::std::uint64_t> host;
    host.number=host_number;
    ::EndianHelper<::std::uint64_t> net;
    for(::std::uint8_t index=0;index<8;++index){
        net.byte_array[7-index]=host.byte_array[index];
    }
    return net.number;
}
//将网络字节序64位无符号整数转换为同类型的主机字节序
::std::uint64_t net_to_host_uint64(::std::uint64_t net_number){
    if(::is_big_endian()){
        return net_number;
    }
    ::EndianHelper<::std::uint64_t> net;
    net.number=net_number;
    ::EndianHelper<::std::uint64_t> host;
    for(::std::uint8_t index=0;index<8;++index){
        host.byte_array[7-index]=net.byte_array[index];
    }
    return host.number;
}
//打包模式
//将文件结构体序列打包为二进制字节数组
::std::vector<char> create_package(::std::vector<::File> const& files){
    //设置返回值初始值
    ::std::vector<char> ret={};
    //预遍历提前分配返回值内存容量
    ::std::uint64_t bytes=0;
    for(auto const& file:files){
        bytes+=16+file.relative_path.string().size()+file.content.size();
    }
    ret.reserve(bytes);
    //遍历写入返回值
    ::std::uint64_t relative_path_bytes=0;
    ::std::uint64_t content_bytes=0;
    for(auto const& file:files){
        auto relative_path_string=file.relative_path.string();
        //写入相对路径的字节数(这部分占用8字节)
        relative_path_bytes=static_cast<::std::uint64_t>(
            relative_path_string.size()
        );
        //首先把uint64_t从主机序转变为网络序
        //把网络序写入char[8]
        //从char[8]插入ret
        ::EndianHelper<::std::uint64_t> net;
        net.number=::host_to_net_uint64(relative_path_bytes);
        ret.insert(
            ret.end()
            ,reinterpret_cast<char const*>(&(net.byte_array[0]))
            ,reinterpret_cast<char const*>(&(net.byte_array[0]))+8
        );
        //写入相对路径
        ret.insert(
            ret.end()
            ,relative_path_string.begin()
            ,relative_path_string.begin()+relative_path_bytes
        );
        //写入文件内容的字节数(这部分占用8字节)
        content_bytes=static_cast<::std::uint64_t>(file.content.size());
        //首先把uint64_t从主机序转变为网络序
        //把网络序写入char[8]
        //从char[8]插入ret
        net.number=::host_to_net_uint64(content_bytes);
        ret.insert(
            ret.end()
            ,reinterpret_cast<char const*>(&(net.byte_array[0]))
            ,reinterpret_cast<char const*>(&(net.byte_array[0]))+8
        );
        //写入文件内容
        ret.insert(ret.end(),file.content.begin(),file.content.end());
    }
    return ret;
}
//解包模式
//将二进制字节数组解析为文件结构体序列
::std::vector<::File> extract_package(::std::vector<char> const& package){
    //检查一下package的大小
    if(package.size()<16){//小于一个文件内容的最小占用字节数
        throw ::std::runtime_error(
            "Failed to extract package:package too small"
        );
    }
    ::std::vector<::File> ret={};
    ::std::uint64_t relative_path_bytes=0;
    ::std::uint64_t content_bytes=0;
    ::EndianHelper<::std::uint64_t> net={};
    ::std::string relative_path_string={};
    ::std::vector<char> content={};
    ::std::uint64_t index=0;
    for(;index<package.size();){
        //解析相对路径占用的字节数
        if(index+8>=package.size()){
            throw ::std::runtime_error(
                "Failed to extract package:incomplete path length field"
            );
        }
        net.number=0;
        for(::std::uint8_t i=0;i<8;++i){
            net.byte_array[i]=static_cast<::std::uint8_t>(package[index]);
            ++index;
        }
        relative_path_bytes=::net_to_host_uint64(net.number);
        //解析相对路径
        if(index+relative_path_bytes>=package.size()){
            throw ::std::runtime_error(
                "Failed to extract package:incomplete path data"
            );
        }
        relative_path_string.clear();
        relative_path_string.reserve(relative_path_bytes);
        relative_path_string.append(&package[index],relative_path_bytes);
        index+=relative_path_bytes;
        //解析文件内容占用的字节数
        if(index+8>package.size()){
            throw ::std::runtime_error(
                "Failed to extract package:incomplete content length field"
            );
        }
        net.number=0;
        for(::std::uint8_t i=0;i<8;++i){
            net.byte_array[i]=static_cast<::std::uint8_t>(package[index]);
            ++index;
        }
        content_bytes=::net_to_host_uint64(net.number);
        //解析文件内容
        if(index+content_bytes>package.size()){
            throw ::std::runtime_error(
                "Failed to extract package:incomplete content data"
            );
        }
        content.assign(
            package.begin()+index
            ,package.begin()+index+content_bytes
        );
        ret.emplace_back(
            ::File{::std::filesystem::path{relative_path_string},content}
        );
        index+=content_bytes;
    }
    //检查解析字节数和包字节数的一致性
    if(index!=package.size()){
        throw ::std::runtime_error(
            "Failed to extract package:extra data at end of package"
        );
    }
    return ret;
}
//打包模式
//将二进制字节数组内容写入包文件
void write_package(
    ::std::filesystem::path const& package_path
    ,::std::vector<char> const& package
){
    //检查包路径是否为文件路径
    if(::std::filesystem::is_directory(package_path)){
        throw ::std::runtime_error(
            "Package path isn't file path:"+package_path.string()
        );
    }
    //二进制覆盖方式打开包输出文件路径
    ::std::ofstream package_file(
        package_path
        ,::std::ios::binary|::std::ios::trunc
    );
    //包输出文件路径打开失败
    if(!package_file.is_open()){
        throw ::std::runtime_error(
            "Failed to open package file:"+package_path.string()
        );
    }
    //包为空时
    if(package.empty()){
        return;
    }
    //写入包内容到包输出文件路径
    package_file.write(
        package.data()
        ,static_cast<::std::streamsize>(package.size())
    );
    //写入失败时
    if(!package_file.good()){
        throw ::std::runtime_error(
            "Package file write error:"+package_path.string()
        );
    }
}
//解包模式
//将包文件内容读取为二进制字节数组
::std::vector<char> read_package(::std::filesystem::path const& package_path){
    //检查包路径是否为文件路径
    if(::std::filesystem::is_directory(package_path)){
        throw ::std::runtime_error(
            "Package path isn't file path:"+package_path.string()
        );
    }
    //使用二进制尾部打开方式
    ::std::ifstream package_file(
        package_path
        ,::std::ios::binary|::std::ios::ate
    );
    //包文件路径打开失败
    if(!package_file.is_open()){
        throw ::std::runtime_error(
            "Failed to open package file:"+package_path.string()
        );
    }
    //得到包文件的字节数
    ::std::size_t file_size=package_file.tellg();
    //包为空时
    if(0==file_size){
        return {};
    }
    //读取包文件内容到内存中
    package_file.seekg(0);
    ::std::vector<char> ret={};
    ret.resize(file_size);
    package_file.read(
        &ret[0]
        ,static_cast<::std::streamsize>(file_size)
    );
    if(package_file.gcount()!=file_size){
        throw ::std::runtime_error(
            "Package file read incomplete:"+package_path.string()
        );
    }
    return ret;
}
//解包模式
//将文件结构体解包为文件
void extract_file(
    ::File const& file
    ,::std::filesystem::path const& output_dir_path=
        ::std::filesystem::current_path()
){
    //构建输出文件路径
    ::std::filesystem::path file_path=
        output_dir_path/file.relative_path.string();
    //检查输出文件路径的父路径是否存在,如果不存在就创建
    if(!::std::filesystem::exists(file_path.parent_path())){
        ::std::filesystem::create_directories(file_path.parent_path());
    }
    //检查文件路径是否为文件
    if(::std::filesystem::is_directory(file_path)){
        throw ::std::runtime_error(
            "File path is directory:"+file_path.string()
        );
    }
    //写入文件内容
    //二进制覆盖方式打开包输出文件路径
    ::std::ofstream ofs(
        file_path
        ,::std::ios::binary|::std::ios::trunc
    );
    //输出文件路径打开失败
    if(!ofs.is_open()){
        throw ::std::runtime_error(
            "Failed to open file:"+file_path.string()
        );
    }
    //写入包内容到包输出文件路径(包含包为空的情况)
    ofs.write(
        file.content.data()
        ,static_cast<::std::streamsize>(file.content.size())
    );
    //写入失败时
    if(!ofs.good()){
        throw ::std::runtime_error(
            "File write error:"+file_path.string()
        );
    }
}
//解包模式
//将文件结构体序列中的每个成员解包为对应的文件
void extract_files(
    ::std::vector<::File> const& files
    ,::std::filesystem::path output_dir_path=::std::filesystem::current_path()
){
    //检查输出目录路径是否存在,如果不存在就创建
    if(!::std::filesystem::exists(output_dir_path)){
        ::std::filesystem::create_directories(output_dir_path);
    }
    //检查输出目录路径是否为目录
    if(!::std::filesystem::is_directory(output_dir_path)){
        throw ::std::runtime_error(
            "Output dir path isn't directory path:"+output_dir_path.string()
        );
    }
    for(auto const& file:files){
        ::extract_file(file,output_dir_path);
    }
}
static bool const std_cout_init=[](void){
    //关闭与C语言的输入输出流同步
    ::std::ios_base::sync_with_stdio(false);
    //解除cin和cout的绑定
    ::std::cin.tie(nullptr);
    ::std::cout.tie(nullptr);
    return true;
}();
void help(void){
    ::std::cout<<
        "Usage(Pack  ): -a <input path> ... -o <output package path>\n"
        "Usage(Unpack): -x <input package path> <output directory path>\n"
        "Examples:\n"
        "  Pack a file and directory: -a README.md source -o 0.fgwsz\n"
        "  Unpack                   : -x 0.fgwsz output\n";
}
int main(int argc,char* argv[]){
    //输入参数太少
    if(argc<4){
        ::help();
        return 1;
    }
    try{
        //检查是不是解包模式
        if(argc==4){
            if("-x"!=::std::string{argv[1]}){
                ::help();
                return 1;
            }
            //解包模式
            //得到包文件路径
            ::std::filesystem::path package_path=argv[2];
            //得到输出目录路径
            ::std::filesystem::path output_directory_path=argv[3];
            //根据包文件路径得到二进制字节序列
            ::std::vector<char> byte_array=::read_package(package_path);
            //将二进制字节序列解析为文件结构体序列
            ::std::vector<::File> files=::extract_package(byte_array);
            //将文件结构体序列中的每个成员解包为对应的文件
            ::extract_files(files,output_directory_path);
        }else{//argc>4
            if("-a"!=::std::string{argv[1]}){
                ::help();
                return 1;
            }
            if("-o"!=::std::string{argv[argc-2]}){
                ::help();
                return 1;
            }
            //打包模式
            //得到输入路径序列
            ::std::vector<::std::filesystem::path> input_paths={};
            for(int index=2;index<argc-2;++index){
                //检查输入路径是否存在
                ::std::filesystem::path input_path=argv[index];
                if(!::std::filesystem::exists(input_path)){
                    throw ::std::runtime_error(
                        "Input path does not exist:"+input_path.string()
                    );
                }
                input_paths.emplace_back(input_path);
            }
            //得到包文件输出路径
            ::std::filesystem::path output_package_path=argv[argc-1];
            //使用输入路径序列来构造文件结构体序列
            ::std::vector<::File> files=::read_paths(input_paths);
            //将文件结构体序列转换为二进制字节序列
            ::std::vector<char> byte_array=::create_package(files);
            //将二进制字节序列写入包输出文件
            ::write_package(output_package_path,byte_array);
        }
    }catch(::std::exception const& e){
        ::std::cerr<<"Error:"<<e.what()<<'\n';
        return 1;
    }
    return 0;
}
