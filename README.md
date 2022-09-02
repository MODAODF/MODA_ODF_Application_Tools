## MODA ODF Application Tools

MODA ODF Application Tools
本軟體是一套開放文件格式(ODF)應用工具組合，
用以協助單位人員產出符合基礎開放文件格式(ODF)之應用軟體套件。

## 編譯MODA ODF Application Tools for Windows

    1. 請先安裝Widnows 10 64 bit主機一台，並將相關基本的環境都建好(解壓縮、防毒、輸入法、瀏
        覽器等)
    
    2. 下載Cygwin，並且記得將安裝檔放在C:\中，建議路徑不要有非ASCII的字元存在，
        32 bit 和 64位元下載路徑不同：
        • 32 bit：http://cygwin.com/setup-x86.exe
        • 64 bit：http://cygwin.com/setup-x86_64.exe
        
    3. 手動安裝Cygwin，請cd到c:\目錄，接下來的指令如下：
        • 32 bit：
      setup-x86.exe -P autoconf -P automake -P bison -P cabextract -P doxygen -P flex -P gcc-g++ -P gettext-devel -P git -P gnupg -P gperf -P make -P mintty -P nasm -P openssh -P openssl -P patch -P perl -P python -P python3 -P pkg-config -P rsync -P unzip -P vim -P wget -P zip -P perl-Archive-Zip -P perl-Font-TTF -P perl-IO-String
        • 64 bit：
      setup-x86_64.exe -P autoconf -P automake -P bison -P cabextract -P doxygen -P flex -P gcc-g++ -P gettext-devel -P git -P gnupg -P gperf -P make -P mintty -P nasm -P openssh -P openssl -P patch -P perl -P python -P python3 -P pkg-config -P rsync -P unzip -P vim -P wget -P zip -P perl-Archive-Zip -P perl-Font-TTF -P perl-IO-String
          
	    • 畫面會跑出UI的安裝介面，請依指示按進行即可。
        
    4. 接下來請安裝Windows平台上的編譯工具，首先是JDK，要編譯不同平台的話，
        請同時安裝二個版本(32 bit和64 bit都要裝，JDK不會自動安裝二個平台)：
        • 下載連結：https://adoptopenjdk.net/
        
    5. 安裝Visual Studio 2019或以上版本 (LibreOffice 7.0 以上 requires VS2019)，
        請安裝Community版本，下載連結如下：
        • 主程式和語系檔：https://www.visualstudio.com/zh-tw/downloads
        • Visual Studio會自動安裝64 bit 及 32 bit的版本。
 
    6. 請使用LO官方提供的GNU make程式，指令如下：
        • 從Windows選單進入cygwin-terminal畫面中
        • mkdir -p /opt/lo/bin -> 這個可以自訂
        • cd /opt/lo/bin
        • wget https://dev-www.libreoffice.org/bin/cygwin/make-4.2.1-msvc.exe
        • mv make-4.2.1-msvc.exe make
        •  chmod a+x make
        
    7. 下載ant
        • 從Windows選單進入cygwin-terminal畫面中
        • mkdir -p /cygdrive/c/sources
        •  cd /cygdrive/c/sources
        •  wget https://archive.apache.org/dist/ant/binaries/apache-ant-1.9.5-bin.tar.bz2
        •  tar -xjvf apache-ant-1.9.5-bin.tar.bz2
    
    8. 設定 autogen.input 
        • 可以參考目錄內的 autogen.input.example32 或   
           autogen.input.example64
