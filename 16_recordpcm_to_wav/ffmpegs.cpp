#include "ffmpegs.h"
#include <QFile>
#include <QDebug>
FFmpegs::FFmpegs()
{

}
void FFmpegs::pcmToWav(WAVHeader &wavHeader,const char *pcmFileName,const char *wavFileName){
      qDebug() << "FFmpegs::pcmToWav" << "开始转wav-------------";
      //文件
      QFile pcmFile(pcmFileName);
      //WriteOnly:只读模式。
      if(!pcmFile.open(QFile::ReadOnly)){
          qDebug() << "文件打开失败" << pcmFileName;
          return;
      }

      QFile wavFile(wavFileName);
      //WriteOnly:只写模式。如果文件不存在，就创建文件;如果文件存在，就删除文件内容
      if(!wavFile.open(QFile::WriteOnly)){
          qDebug() << "文件打开失败" << wavFileName;
          pcmFile.close();
          return;
      }
      wavFile.write((const char *)&wavHeader,sizeof(WAVHeader));

      char wavBuffer[1024];
      int size;
      while ((size = pcmFile.read(wavBuffer,sizeof (wavBuffer))) > 0) {
           wavFile.write(wavBuffer,size);
      }

      pcmFile.close();
      wavFile.close();
}
