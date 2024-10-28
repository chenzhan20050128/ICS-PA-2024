#include <am.h>   // 包含 AM 框架的头文件
#include <nemu.h> // 包含 NEMU 模拟器的头文件
#include <klib.h> // 包含内核库接口的头文件

// 定义音频设备寄存器的地址偏移
#define AUDIO_FREQ_ADDR (AUDIO_ADDR + 0x00)      // 音频频率寄存器
#define AUDIO_CHANNELS_ADDR (AUDIO_ADDR + 0x04)  // 音频通道数寄存器
#define AUDIO_SAMPLES_ADDR (AUDIO_ADDR + 0x08)   // 音频采样数寄存器
#define AUDIO_SBUF_SIZE_ADDR (AUDIO_ADDR + 0x0c) // 音频缓冲区大小寄存器
#define AUDIO_INIT_ADDR (AUDIO_ADDR + 0x10)      // 音频初始化寄存器
#define AUDIO_COUNT_ADDR (AUDIO_ADDR + 0x14)     // 已播放音频数据的计数寄存器

static uint32_t buf_pos = 0; // 静态变量，音频缓冲区的位置索引初始为0

// 音频初始化函数（当前未实现）
void __am_audio_init()
{
}

// 配置音频函数，设置音频设备的初始配置
void __am_audio_config(AM_AUDIO_CONFIG_T *cfg)
{
  cfg->present = false; // 假定音频设备未准备好
}

// 控制音频函数，配置音频的频率、通道数、采样数，启动音频设备
void __am_audio_ctrl(AM_AUDIO_CTRL_T *ctrl)
{
  outl(AUDIO_FREQ_ADDR, ctrl->freq);         // 设置音频频率
  outl(AUDIO_CHANNELS_ADDR, ctrl->channels); // 设置音频通道数
  outl(AUDIO_SAMPLES_ADDR, ctrl->samples);   // 设置采样数
  outl(AUDIO_INIT_ADDR, 1);                  // 初始化音频设备
}

// 音频状态查询函数，获取当前缓冲区的数据数目
void __am_audio_status(AM_AUDIO_STATUS_T *stat)
{
  stat->count = inl(AUDIO_COUNT_ADDR); // 读取已播放的音频数据计数
}

// 播放音频函数，向音频设备写入数据
void __am_audio_play(AM_AUDIO_PLAY_T *ctl)
{
  uint8_t *audio = (ctl->buf).start;                                    // 获取音频数据的起始地址
  uint32_t buf_size = inl(AUDIO_SBUF_SIZE_ADDR) / sizeof(uint8_t);      // 获取音频缓冲区大小
  uint32_t cnt = inl(AUDIO_COUNT_ADDR);                                 // 获取当前音频的计数
  uint32_t len = ((ctl->buf).end - (ctl->buf).start) / sizeof(uint8_t); // 计算音频数据长度

  printf("area end %x and begin %x\n", (ctl->buf).end, (ctl->buf).start); // 调试信息，输出区域的开始和结束地址

  while (len > buf_size - cnt) // 当缓冲区剩余空间小于数据长度时等待
  {
    ;
  }

  uint8_t *ab = (uint8_t *)(uintptr_t)AUDIO_SBUF_ADDR; // 获取音频缓冲区的指针
  for (int i = 0; i < len; ++i)                        // 将音频数据写入缓冲区
  {
    ab[buf_pos] = audio[i];             // 将音频数据复制到缓冲区
    buf_pos = (buf_pos + 1) % buf_size; // 更新缓冲区位置索引，循环缓冲
    // printf("cpu buf pos is %d\n",buf_pos); // 调试信息，输出当前缓冲区位置
  }

  outl(AUDIO_COUNT_ADDR, inl(AUDIO_COUNT_ADDR) + len); // 更新音频数据计数器
  // printf("used cnt is %d\n",inl(AUDIO_COUNT_ADDR)); // 调试信息，输出当前音频计数
}