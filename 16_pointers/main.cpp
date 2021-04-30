#include "mainwindow.h"

#include <QApplication>
#include <iostream>
#include <QDebug>
extern "C" {
//设备
 #include <libavdevice/avdevice.h>
}
int main(int argc, char *argv[])
{
    char *name[] = {"Follow me","BASIC","Great Wall","FORTRAN","Computer design"};
    char **p;
    int i;
    for(i = 0; i < 5; i++) {
        p = name + i;
        printf("%s\n", *p);
    }

    int b[] = {1, 3, 5, 7, 9};
    int *num[5] = {&b[0], &b[1], &b[2], &b[3],&b[4]};
    int* *n[5] = {&num[0],&num[1],&num[2],&num[3],&num[4]};
    int **q, j;
//    qDebug() << "q[0]" << q[0];
    for(j = 0; j < 5; j++) {
      //打印出来是num+j的内存地址，q = &num[0] *q = num[0]指向的是&b[0]的地址，加两颗星是**q = b[0]
        //**q = *num[0] = *&b[0] = b[0] = 1
      qDebug() << "num + j------" << num+j;
      q = &num[j];
      qDebug() << "q[j]------" << q[j];
      qDebug() << "q-------" << q;
      printf("%d ", **q);
    }
    int ***t,k;
    for(k=0;k<5;k++){
        //t = &n[0]===> *t = n[0] = &num[0] ===>  **t = num[0] = &b[0] ===> ***t = *&b[0] = b[0] = 1
        t = n+k;
        printf("% d",***t);
    }
    printf("\n");

    //C语言
    printf("printf-----------------------");
    //加换行不用刷新标准输出流
//    printf("printf-----------------------\n");
    //C++
    std::cout << "std::count-------\n";
    //加换行
//    std::cout << "std::count-------" << std::endl;

    //FFMpeg::日志级别 有大小和优先级，比level小的会打印输出 TRACE<DEBUG<INFO<WARNING<ERROR<FATAL
    av_log_set_level(AV_LOG_DEBUG);
    av_log(nullptr,AV_LOG_INFO,"av_log1---\n");
    av_log(nullptr,AV_LOG_DEBUG,"av_log2---\n");
    av_log(nullptr,AV_LOG_ERROR,"av_log3---\n");

    //QDebug
    qDebug() << "qDebug-------";

    //刷新标准输出流，printf,std::coutC语言C++的log能出来，不加换行要刷新标准输出流，加换行自己出来
    fflush(stdout);
    //av_log刷新错误输出
    fflush(stderr);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
