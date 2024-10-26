#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000
#define KEYCODE_MASK 0x7FFF

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd)
{
  int keycode = inl(KBD_ADDR);

  if (keycode == AM_KEY_NONE)
  {
    kbd->keydown = false;
    kbd->keycode = AM_KEY_NONE;
  }
  else
  {
    kbd->keydown = (bool)((keycode & KEYDOWN_MASK) != 0);
    kbd->keycode = keycode & KEYCODE_MASK; // 提取有效的键码
  }
}