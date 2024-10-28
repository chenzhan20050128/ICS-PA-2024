#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init()
{
  // int i;
  // int w = io_read(AM_GPU_CONFIG).width;  // TODO: get the correct width
  // int h = io_read(AM_GPU_CONFIG).height; // TODO: get the correct height
  // // printf("%d, %d", w, h);
  // uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  // for (i = 0; i < w * h; i++)
  //   fb[i] = i;
  // outl(SYNC_ADDR, 1);
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg)
{
  uint32_t info = inl(VGACTL_ADDR);
  uint16_t height = (uint16_t)(info & 0xFFFF);
  uint16_t width = (uint16_t)(info >> 16);
  *cfg = (AM_GPU_CONFIG_T){
      .present = true, .has_accel = false, .width = width, .height = height, .vmemsz = 0};
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl)
{
  // 获取屏幕宽度，用于计算帧缓冲区中的偏移量
  int screen_width = io_read(AM_GPU_CONFIG).width;

  // 定义帧缓冲区和像素数据指针
  uint32_t *framebuffer = (uint32_t *)(uintptr_t)FB_ADDR;
  uint32_t *pixel_data = (uint32_t *)(uintptr_t)ctl->pixels;

  // 将控件像素逐行复制到帧缓冲区中的指定位置
  for (int row = 0; row < ctl->h; ++row)
  {
    for (int col = 0; col < ctl->w; ++col)
    {
      int framebuffer_index = (ctl->y + row) * screen_width + (ctl->x + col);
      int pixel_data_index = row * ctl->w + col;

      // 将像素数据复制到帧缓冲区
      framebuffer[framebuffer_index] = pixel_data[pixel_data_index];
    }
  }
  if (ctl->sync)
  {
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status)
{
  status->ready = true;
}

void __am_gpu_memcpy(AM_GPU_MEMCPY_T *params)
{
  uint32_t *src = params->src, *dst = (uint32_t *)(FB_ADDR + params->dest);
  for (int i = 0; i < params->size >> 2; i++, src++, dst++)
  {
    *dst = *src;
  }
  // 处理剩余不足4字节的数据
  char *c_src = (char *)src, *c_dst = (char *)dst;
  for (int i = 0; i < (params->size & 3); i++)
  {
    c_dst[i] = c_src[i];
  }
}
