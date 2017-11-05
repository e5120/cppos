#include "../include/io.h"

void wait_KBC_sendready(void){
  while(1){
    if((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0){
      break;
    }
  }
  return;
}

void init_keyboard(void){
  wait_KBC_sendready();
  io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
  wait_KBC_sendready();
  io_out8(PORT_KEYDATA, KBC_MODE);
  return;
}

void enable_mouse(MOUSE_DEC* mdec){
  wait_KBC_sendready();
  io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
  wait_KBC_sendready();
  io_out8(PORT_KEYDATA, MOUSECMD_ENABLE);
  mdec->phase = 0;
  return;
}

int mouse_decode(MOUSE_DEC* mdec, unsigned char data){
  if(mdec->phase == 0){
    // 0xfaは要らないので捨てる
    if(data == 0xfa){
      mdec->phase  = 1;
    }
    return 0;
  }
  else if(mdec->phase == 1){
    // 正しいデータか判定
    if((data & 0xc8) == 0x08){
      mdec->buf[0] = data;
      mdec->phase = 2;
    }
    return 0;
  }
  else if(mdec->phase == 2){
    mdec->buf[1] = data;
    mdec->phase = 3;
    return 0;
  }
  else if(mdec->phase == 3){
    mdec->buf[2] = data;
    mdec->phase = 1;
    mdec->btn = mdec->buf[0] & 0x07;
    mdec->x = mdec->buf[1];
    mdec->y = mdec->buf[2];
    if((mdec->buf[0] & 0x10) != 0){
      mdec->x |= 0xffffff00;
    }
    if((mdec->buf[0] & 0x20) != 0){
      mdec->y |= 0xffffff00;
    }
    mdec->y = -mdec->y;
    return 1;
  }
  return -1;
}

