# fgwsz-package
```txt
作者制定了归档一个或多个(文件/目录)的包标准:
    包的二进制的文件结构如下:
        [file item 1]...[file item N]
    每个文件的二进制信息[file item]的文件结构是[A|B|C|D]:
        A部分是[relative path bytes(8字节)]
        B部分是[relative path]
        C部分是[content bytes(8字节)]
        D部分是[content(binary)]
```
使用`fgwsz-package`进行打包和解包  
```txt
Usage(Pack  ): -a <input path> ... -o <output package path>
Usage(Unpack): -x <input package path> <output directory path>
Examples:
  Pack a file and directory: -a README.md source -o 0.fgwsz
  Unpack                   : -x 0.fgwsz output
```
