# MiniCompiler
This is the final project for ZJU, Compiler Principle. 

## 运行方式

##### lab1

```powershell
flex lexical.l
bison -d syntax.y
g++ main.cpp syntax.cpp Tree.cpp -o output
output.exe testfile
```

##### lab2

1. 进入`src`文件夹，双击`scriptGen.bat`生成`exe`文件；
2. 运行`scriptRun.bat`，生成log文件，文件在`minicompiler/test/NJUTest/log2`中
3. 运行`scriptDel.bat`，删除冗余文件
4. （注意，测试文件在`minicompiler/test/NJUTest/Test2`中，可以自行补充测试）
