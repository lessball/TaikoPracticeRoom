# TaikoPracticeRoom
---
#太鼓练习室 
windows phone 8的太鼓游戏，支持太鼓次郎的tja格式谱面。2013年为了自娱自乐写的。2016年想起来加了SD卡支持。  

TaikoGame.cpp 是tja解析运行的核心逻辑。  
TaikoSkin.h 是游戏皮肤的封装。  
谱面管理、选择、记录等外围逻辑都用c#实现。  

* 编译需要添加以下的库，需要自己修改编译选项编译成wp8版本：
    * boost（只用头文件不需要编译）
	* rapidxml
	* libogg
	* libvorbis
	* zlib
	* libpng
	* ijg.org