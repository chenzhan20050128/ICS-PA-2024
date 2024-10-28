/***************************************************************************************
 * Copyright (c) 2014-2024 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/
// hardware:
#include <common.h>     // 包含公共头文件，定义了一些全局使用的基本功能和类型
#include <device/map.h> // 包含设备映射相关的头文件
#include <SDL2/SDL.h>   // 包含SDL库，用于音频处理
#include <stdbool.h>
#include <stdint.h>

// 宏定义，用于获取两个数的最小值
#define min(x, y) ((x < y) ? x : y)

// 枚举定义音频设备寄存器，提供更具可读性的访问方式
enum
{
  reg_freq,      // 频率寄存器
  reg_channels,  // 通道寄存器
  reg_samples,   // 样本寄存器
  reg_sbuf_size, // 缓冲区大小寄存器
  reg_init,      // 初始化寄存器
  reg_count,     // 已使用计数寄存器
  nr_reg         // 寄存器数量
};

// 静态变量定义，用于存储音频缓冲区和基地址等信息
static uint8_t *sbuf = NULL;        // 声音缓冲区的指针
static uint32_t *audio_base = NULL; // 音频基地址，寄存器数组
static uint32_t sbuf_pos = 0;       // 缓冲区当前位置

// SDL音频规格
SDL_AudioSpec s = {};

// 标志变量，标识音频是否初始化
static bool is_init = false;

// SDL音频回调函数，负责从缓冲区读取数据，供SDL播放
void sdl_audio_callback(void *udata, uint8_t *stream, int len)
{
  memset(stream, 0, len);                    // 清空传入的音频流缓冲区
  uint32_t used_cnt = audio_base[reg_count]; // 获取当前已用音频数据的长度
  if (len > used_cnt)                        // 如果请求长度超过已用长度
    len = used_cnt;                          // 则只读取已用长度的数据

  uint32_t sbuf_size = audio_base[reg_sbuf_size] / sizeof(uint8_t); // 缓冲区大小
  if ((sbuf_pos + len) > sbuf_size)                                 // 如果当前读取位置超过缓冲区
  {
    // 分两部分混合读取：先读取至缓冲区尾部，再从头重新读取
    SDL_MixAudio(stream, sbuf + sbuf_pos, sbuf_size - sbuf_pos, SDL_MIX_MAXVOLUME);
    SDL_MixAudio(stream + (sbuf_size - sbuf_pos), sbuf, len - (sbuf_size - sbuf_pos), SDL_MIX_MAXVOLUME);
  }
  else
    SDL_MixAudio(stream, sbuf + sbuf_pos, len, SDL_MIX_MAXVOLUME); // 直接混合读取

  sbuf_pos = (sbuf_pos + len) % sbuf_size; // 更新缓冲区位置
  audio_base[reg_count] -= len;            // 减少剩余的已用音频数据计数
}

// 初始化音频设备
void init_sound()
{
  s.format = AUDIO_S16SYS;               // 设置音频格式为系统默认16位
  s.userdata = NULL;                     // 不传递用户数据
  s.freq = audio_base[reg_freq];         // 从寄存器获取频率
  s.channels = audio_base[reg_channels]; // 从寄存器获取通道数
  s.samples = audio_base[reg_samples];   // 从寄存器获取样本数
  s.callback = sdl_audio_callback;       // 设置回调函数

  int ret = SDL_InitSubSystem(SDL_INIT_AUDIO); // 初始化SDL音频子系统
  if (ret == 0)                                // 如果初始化成功
  {
    SDL_OpenAudio(&s, NULL); // 打开音频设备
    SDL_PauseAudio(0);       // 开始播放音频
  }
}

// 音频IO处理函数，根据偏移、长度和写入标志进行操作
static void audio_io_handler(uint32_t offset, int len, bool is_write)
{
  if (audio_base[reg_init] == 1) // 如果初始化寄存器标识为1，表示需要初始化
  {
    init_sound();             // 初始化音频
    is_init = true;           // 设置初始化标志
    audio_base[reg_init] = 0; // 重置初始化寄存器
  }
}

// 初始化音频设备和资源
void init_audio()
{
  uint32_t space_size = sizeof(uint32_t) * nr_reg; // 计算寄存器数组大小
  audio_base = (uint32_t *)new_space(space_size);  // 分配寄存器数组空间

#ifdef CONFIG_HAS_PORT_IO
  add_pio_map("audio", CONFIG_AUDIO_CTL_PORT, audio_base, space_size, audio_io_handler); // 添加端口IO映射
#else
  add_mmio_map("audio", CONFIG_AUDIO_CTL_MMIO, audio_base, space_size, audio_io_handler); // 添加内存映射IO
#endif

  sbuf = (uint8_t *)new_space(CONFIG_SB_SIZE);                            // 分配音频缓冲区空间
  add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, NULL); // 添加音频缓冲区内存映射
  audio_base[reg_sbuf_size] = CONFIG_SB_SIZE;                             // 设置缓冲区大小寄存器
}