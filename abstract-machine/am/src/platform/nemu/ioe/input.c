#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd)
{
  // 读取键盘数据寄存器中的键码
  int keycode = inl(KBD_ADDR);

  // 判断是否为按下事件
  kbd->keydown = (keycode & KEYDOWN_MASK) != 0;

  // 获取实际键码（去掉按下掩码）
  kbd->keycode = keycode & ~KEYDOWN_MASK;
}
