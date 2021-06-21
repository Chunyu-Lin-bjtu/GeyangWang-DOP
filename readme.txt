1. 替换CPP2DOP文件夹下源代码到项目360tools-master中，使用命令“360tools_conv.exe -i A_3840x1920_CPP.yuv -o A_3840x1920_CPP_DOP.yuv -w 3840 -h 1920 -l 3840 -m 1920 -n 1 -f 31”实现A测试序列由CPP格式转换为DOP格式。
使用命令“360tools_conv.exe -i A_3840x1920_DOP.yuv -o A_3840x1920_DOP_CPP.yuv -w 3840 -h 1920 -l 3840 -m 1920 -n 1 -f 0”可实现A测试序列从DOP格式到CPP格式的转换。
其中 -n 表示视频帧数，-f是转换模式，-w -h -l -m 分别为源视频与重建视频的宽和高，-i 是源视频名称，-o是重建视频名称。

2. 同样，替换ERP2DOP文件夹下源代码到项目360tools-master中，可以实现ERP和DOP格式间的互相转换，命令与第1条类似，其中ERP->DOP的转换模式是0，反之为31。

3. 不改变代码，直接运行 360tools-master 项目的 360tools_conv ，可以实现ERP和CPP格式间的互相转换，命令与第1条类似，其中ERP->CPP的转换模式是0，反之为31。

4. 不改变代码，直接运行 360tools-master 项目的 360tools_metric ，可以计算两个源视频的PSNR、CPP-PSNR、S-PSNR等。

5. 替换CPP2CDOP文件夹下源代码到项目360tools-master中，使用命令“360tools_conv.exe -i A_3840x1920_CPP.yuv -o A_2400x2400_CPP_CDOP.yuv -w 3840 -h 1920 -l 2400 -m 2400 -n 1 -f 31”实现A测试序列由CPP格式转换为CDOP格式。注意源视频与重建视频的分辨率关系。
使用命令“360tools_conv.exe -i A_2400x2400_CDOP.yuv -o A_3840x1920_CDOP_CPP.yuv -w 2400 -h 2400 -l 3840 -m 1920 -n 1 -f 0”可实现A测试序列从CDOP格式到CPP格式的转换。

