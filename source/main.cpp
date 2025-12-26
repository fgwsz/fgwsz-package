#include<cstddef>//::std::size_t
#include<ios>//::std::ios ::std::streamsize
#include<iostream>//::std::cout ::std::cin
#include<string>//::std::string
#include<fstream>//::std::ifstream ::std::ofstream
#include<stdexcept>// ::std::runtime_error

inline ::std::string read_file(::std::string const& file_path){
    ::std::ifstream file(file_path,::std::ios::binary|::std::ios::ate);
    if(!file.is_open()){
        throw ::std::runtime_error("Failed to open file:"+file_path);
    }
    ::std::size_t file_size=file.tellg();
    if(0==file_size){
        return {};
    }
    file.seekg(0);
    ::std::string content;
    content.resize(file_size);
    file.read(
        &content[0]
        ,static_cast<::std::streamsize>(file_size)
    );
    if(file.gcount()!=file_size){
        throw ::std::runtime_error("File read incomplete:"+file_path);
    }
    return content;
}
inline void write_file(
    ::std::string const& file_path
    ,::std::string& content
){
    ::std::ofstream file(file_path,::std::ios::binary|::std::ios::trunc);
    if(!file.is_open()){
        throw ::std::runtime_error("Failed to open file:"+file_path);
    }
    if(content.empty()){
        return;
    }
    file.write(
        content.data()
        ,static_cast<::std::streamsize>(content.size())
    );
    if(!file.good()){
        throw ::std::runtime_error("File read error:"+file_path);
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
inline void help(void){
    ::std::cout<<
        "Usage:--encrypt <input file path> <output dir path>\n"
        "Usage:-e <input file path> <output dir path>\n"
        "Usage:--decrypt <input file path> <output dir path>\n";
        "Usage:-d <input file path> <output dir path>\n";
}
//加密文件结构
//[分隔符][加密文件名][分隔符][密钥(分隔符模运算后的加密样式)][分隔符][加密内容]
//首先需要解决一个问题:各个部分之间的分隔符是什么?规定分隔符为x
//x是区间[0,255]中的一个随机数.
//加密的任何内容都不能是分隔符x
//解密的过程如下:
//由于分隔符号是一个字节:所以直接得到分隔符.
//然后根据加密分隔符号对内容进行拆分,得到加密文件名,密钥和加密内容
//然后根据根据密钥解密得到真实的文件名和真实的文件内容
//之后将真实的文件名,根据用户输入的输出路径和真实的文件名,进行写入
//得到真实的文件
//filename包含文件后缀名

//package:[filename][/][bytes(string type)][/][content]
//[encrypted package][key]
//key:[1,255]random
inline ::std::string encrypt_package(
    unsigned char key
    ,::std::string const& package
){
    ::std::string ret;
    ret.reserve(package.size());
    for(unsigned char byte:package){
        ret.push_back(static_cast<unsigned char>((byte^key)+key));
    }
    return ret;
}
inline ::std::string decrypt_package(
    unsigned char key
    ,::std::string const& package
){
    ::std::string ret;
    ret.reserve(package.size());
    for(unsigned char byte:package){
        ret.push_back(static_cast<unsigned char>(byte-key)^key);
    }
    return ret;
}
inline ::std::string encrypt(::std::string const& file_path){
    
}
struct File{
    ::std::string name;
    ::std::string content;
};
inline ::File decrypt(::std::string const& file_path){
    
}
int main(int argc,char* argv[]){
    if(argc!=4){
        ::help();
        return -1;
    }
    ::std::string option=argv[1];
    ::std::string input_file_path=argv[2];
    ::std::string output_dir_path=argv[3];
    if("--encrypt"==option||"-e"==option){
        ::write_file(argv[3],::encrypt(::read_file(argv[2])));
    }else if("--decrypt"==option||"-d"==option){
        ::write_file(argv[3],::decrypt(::read_file(argv[2])));
    }else{
        ::help();
        return -1;
    }
    return 0;
}
