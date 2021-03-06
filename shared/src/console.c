#include "../include/console.h"

void console_task(LAYER *layer, unsigned int memtotal){
  TIMER *timer;
  TASK *task = task_now();
  MEM_MAN *memman = (MEM_MAN*)MEMMANAGER_ADDR;
  CONSOLE cons;
  int i, fifobuf[FIFO_MAX_BUF];
  int *fat = (int*)memory_manage_alloc_4k(memman, 4 * 2880);
  char cmdline[STR_MAX_BUF];

  cons.layer = layer;
  cons.cur_x = 8;
  cons.cur_y = 28;
  cons.cur_c = -1;
  *((int *)0x0fec) = (int)&cons;

  init_fifo32(&task->fifo, FIFO_MAX_BUF, fifobuf, task);
  timer = timer_alloc();
  init_timer(timer, &task->fifo, 1);
  settimer(timer, 50);

  file_readfat(fat, (unsigned char*)(ADDR_DISKIMG + 0x0002000));

  // プロンプト再表示
  cons_putchar(&cons, '>', 1);

  while(1){
    io_cli();
    if(fifo_status32(&task->fifo) == 0){
      task_sleep(task);
      io_sti();
    }
    else{
      i = get_fifo32(&task->fifo);
      io_sti();
      if(i <= 1){
        if(i != 0){
          init_timer(timer, &task->fifo, 0);
          if(cons.cur_c >= 0){
            cons.cur_c = COLOR_FFFFFF;
          }
        }
        else{
          init_timer(timer, &task->fifo, 1);
          if(cons.cur_c >= 0){
            cons.cur_c = COLOR_000000;
          }
        }
        settimer(timer, 50);
      }
      if(i == 2){
        cons.cur_c = COLOR_FFFFFF;
      }
      if(i == 3){
        draw_rectangle((char*)layer->buf, layer->bxsize, COLOR_000000, cons.cur_x, cons.cur_y, 8, 16);
        cons.cur_c = -1;
      }
      if(KEY_BOTTOM <= i && i < KEY_TOP){
        if(i == 8 + KEY_BOTTOM){    // backspace
          if(cons.cur_x > 16){
            cons_putchar(&cons, ' ', 0);
            cons.cur_x -= 8;
          }
        }
        else if(i == 10 + KEY_BOTTOM){    // enter
          cons_putchar(&cons, ' ', 0);
          cmdline[cons.cur_x / 8 - 2] = 0;
          cons_newline(&cons);
          cons_runcmd(cmdline, &cons, fat, memtotal);
          cons_putchar(&cons, '>', 1);
        }
        else{   // 一般文字
          if(cons.cur_x < 240){
            cmdline[cons.cur_x / 8 - 2] = i - KEY_BOTTOM;
            cons_putchar(&cons, i - 256, 1);
          }
        }
      }
      // カーソル再表示
      if(cons.cur_c >= 0){
        draw_rectangle((char*)layer->buf, layer->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, 8, 16);
      }
      layer_refresh(layer, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
    }
  }
}

void cons_newline(CONSOLE *cons){
  int x, y;
  if(cons->cur_y < 28 + 112){
    cons->cur_y += 16;
  }
  else{
    // スクロール
    for(y = 28; y < 28 + 112; ++y){
      for(x = 8; x < 8 + 240; ++x){
        cons->layer->buf[x + y * cons->layer->bxsize] = cons->layer->buf[x + (y + 16) * cons->layer->bxsize];
      }
    }
    for(y = 28 + 112; y < 28 + 128; ++y){
      for(x = 8; x < 8 + 240; ++x){
        cons->layer->buf[x + y * cons->layer->bxsize] = COLOR_000000;
      }
    }
    layer_refresh(cons->layer, 8, 28, 8 + 240, 28 + 128);
  }
  cons->cur_x = 8;
  return;
}

void cons_putchar(CONSOLE *cons, int chr, char move){
  char s[2];
  s[0] = chr;
  s[1] = '\0';
  if(s[0] == 0x09){   // tab
    while(1){
      put_string_layer(cons->layer, cons->cur_x, cons->cur_y, COLOR_FFFFFF, COLOR_000000, " ", 1);
      cons->cur_x += 8;
      if(cons->cur_x == 8 + 240){
        cons->cur_x = 8;
        cons_newline(cons);
      }
      if(((cons->cur_x - 8) & 0x1f) == 0){
        break;
      }
    }
  }
  else if(s[0] == 0x0a){    // 改行
    cons_newline(cons);
  }
  else if(s[0] == 0x0d){    // 復帰
    // 現状何もしない
  }
  else{
    put_string_layer(cons->layer, cons->cur_x, cons->cur_y, COLOR_FFFFFF, COLOR_000000, s, 1);
    if(move != 0){
      cons->cur_x += 8;
      if(cons->cur_x == 8 + 240){
        cons_newline(cons);
      }
    }
  }
  return;
}

void cons_runcmd(char *cmdline, CONSOLE *cons, int *fat, unsigned int memtotal){
  // コマンド実行
    if(lstrcmp(cmdline, "mem")){
      cmd_mem(cons, memtotal);
    }
    else if(lstrcmp(cmdline, "clear")){
      cmd_clear(cons);
    }
    else if(lstrcmp(cmdline, "ls")){
      cmd_ls(cons);
    }
    else if(lstrncmp(cmdline, "cat ", 4)){
      cmd_cat(cons, fat, cmdline);
    }
    else if(cmdline[0] != 0){
      if(cmd_app(cons, fat, cmdline) == 0){
        put_string_layer(cons->layer, 8, cons->cur_y, COLOR_FFFFFF, COLOR_000000, "Bad command.", 12);
        cons_newline(cons);
        cons_newline(cons);
      }
    }
    return;
}

void cmd_mem(CONSOLE *cons, unsigned int memtotal){
  MEM_MAN *memman = (MEM_MAN*)MEMMANAGER_ADDR;
  char s[STR_MAX_BUF];

  lsprintf(s, "total %dMB\n", memtotal / (1024 * 1024));
  cons_putstr(cons, s);
  lsprintf(s, "free %dKB\n\n", memory_manage_total(memman) / 1024);
  cons_putstr(cons, s);
  return;
}

void cmd_clear(CONSOLE*cons){
  int x, y;
  LAYER *layer = cons->layer;
  for(y = 28; y < 28 + 128; ++y){
    for(x = 8; x < 8 + 240; ++x){
      layer->buf[x + y * layer->bxsize] = COLOR_000000;
    }
  }
  layer_refresh(layer, 8, 28, 8 + 240, 28 + 128);
  cons->cur_y = 28;
}

void cmd_ls(CONSOLE *cons){
  FILE_INFO *finfo = (FILE_INFO*)(ADDR_DISKIMG + 0x002600);
  int i, j;
  char s[STR_MAX_BUF];

  for(i = 0; i < 224; ++i){
    if(finfo[i].name[0] == 0x00){
      break;
    }
    if(finfo[i].name[0] != 0xe5){
      if((finfo[i].type & 0x18) == 0){
        lsprintf(s, "filename.ext %d\n", finfo[i].size);
        for(j = 0; j < 8; ++j){
          s[j] = finfo[i].name[j];
        }
        s[9] = finfo[i].ext[0];
        s[10] = finfo[i].ext[1];
        s[11] = finfo[i].ext[2];
        cons_putstr(cons, s);
      }
    }
  }
  cons_newline(cons);
  return;
}

void cmd_cat(CONSOLE *cons, int *fat, char *cmdline){
  MEM_MAN *memman = (MEM_MAN*)MEMMANAGER_ADDR;
  FILE_INFO *finfo = file_search(cmdline + 4, (FILE_INFO*)(ADDR_DISKIMG + 0x002600), 224);
  char *p;
  int i;

  if(finfo != 0){
    // ファイル発見した場合
    p = (char*)memory_manage_alloc_4k(memman, finfo->size);
    file_loadfile(finfo->clustno, finfo->size, p, fat, (char*)(ADDR_DISKIMG + 0x003e00));
    for(i = 0; i < finfo->size; ++i){
      cons_putchar(cons, p[i], 1);
    }
    memory_manage_free_4k(memman, (int)p, finfo->size);
  }
  else{
    // ファイル発見できなかった場合
    cons_putstr(cons, "File not found.");
  }
  cons_newline(cons);
  return;
}

int cmd_app(CONSOLE *cons, int *fat, char *cmdline){
  MEM_MAN *memman = (MEM_MAN*)MEMMANAGER_ADDR;
  FILE_INFO *finfo = file_search("HLT.BIN", (FILE_INFO*)(ADDR_DISKIMG + 0x002600), 224);
  SEG_DESC *gdt = (SEG_DESC*)ADDR_GDT;
  TASK *task = task_now();
  char *p, *q, name[18];
  int i, segsize, datasize, esp, data;

  // コマンドラインからフィアル名を生成
  for(i = 0; i < 13; ++i){
      if(cmdline[i] <= ' '){
        break;
      }
      name[i] = cmdline[i];
  }
  name[i] = '\0';

  finfo = file_search(name, (FILE_INFO*)(ADDR_DISKIMG + 0x002600), 224);
  if(finfo == 0 && name[i-1] != '.'){
    // 拡張子付きでもう一度検索
    name[i] = '.';
    name[i + 1] = 'B';
    name[i + 2] = 'I';
    name[i + 3] = 'N';
    name[i + 4] = '\0';
    finfo = file_search(name, (FILE_INFO*)(ADDR_DISKIMG + 0x002600), 224);
  }
  if(finfo != 0){
    // ファイル発見
    p = (char*) memory_manage_alloc_4k(memman, finfo->size);
    q = (char*) memory_manage_alloc_4k(memman, 64 * 1024);
    *((int*)0xfe8) = (int)p;
    file_loadfile(finfo->clustno, finfo->size, p, fat, (char*)(ADDR_DISKIMG + 0x003e00));
    if(finfo->size >= 36 && lstrncmp(p + 4, "Hari", 4) && *p == 0x00){
      segsize = *((int*)(p + 0x0000));
      esp = *((int*)(p + 0x000c));
      datasize = *((int*)(p + 0x0010));
      data = *((int*)(p + 0x0014));
      q = (char*) memory_manage_alloc_4k(memman, segsize);
      *((int*)0xfe8) = (int)q;
      set_segmdesc(gdt + 1003, finfo->size - 1, (int)p, AR_CODE32_ER + 0x60);
      set_segmdesc(gdt + 1004, segsize - 1, (int)q, AR_DATA32_RW + 0x60);
      for(i = 0; i < datasize; ++i){
        q[esp + i] = p[data + i];
      }
      start_app(0x1b, 1003 * 8, esp, 1004 * 8, &(task->tss.esp0));
      memory_manage_free_4k(memman, (int)q, segsize);
    }
    else{
      cons_putstr(cons, "file format error.\n");
    }
    memory_manage_free_4k(memman, (int)p, finfo->size);
    cons_newline(cons);
    return 1;
  }
  return 0;
}

void cons_putstr(CONSOLE *cons, char *s){
  while(*s != '\0'){
    cons_putchar(cons, *s, 1);
    ++s;
  }
  return;
}

void cons_putnstr(CONSOLE *cons, char *s, int n){
  while(*s == '\0' && n > 0){
    cons_putchar(cons, *s, 1);
    --n;
    ++s;
  }
  return;
}

int os_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax){
  int ds_base = *((int*)0xfe8);
  CONSOLE *cons = (CONSOLE*) *((int*)0x0fec);
  TASK *task = task_now();
  LAYER_CTL *layerctl = (LAYER_CTL*)*((int*)0x0fe4);
  LAYER *layer;
  int *reg = &eax + 1;   // eaxの次の番地
  /*
  保存おためのPUSHADを強引に書き換える
  reg[0] : EDI,  reg[1] : ESI,  reg[2] : EBP,  reg[3] : ESP
  reg[4] : EBX,  reg[5] : EDX,  reg[6] : ECX,  reg[7] : EAX
  */

  // edxでapiの切り替え, ebxが文字列の番地, ecxが文字数
  if(edx == 1){
    // 1文字出力するAPI
    cons_putchar(cons, eax & 0xff, 1);
  }
  else if(edx == 2){
    // 文字列を出力するAPI
    cons_putstr(cons, (char*)ebx + ds_base);
  }
  else if(edx == 3){
    // n文字文字列出力するAPI
    cons_putnstr(cons, (char*)ebx + ds_base, ecx);
  }
  else if(edx == 4){
    return &(task->tss.esp0);
  }
  else if(edx == 5){
    // ウィンドウを表示するAPI
    /*
    edx = 5, ebx = ウィンドウのバッファ, esi = ウィンドウのx方向の大きさ
    edi = ウィンドウのy方向の大きさ, eax = 透明色, ecx = ウィンドウの名前
    eax = ウィンドウを操作するための番号(リフレッシュなどに使用)
    */
    layer = layer_alloc(layerctl);
    layer_setbuf(layer, (char*)ebx + ds_base, esi, edi, eax);
    make_window((char*)ebx + ds_base, esi, edi, (char*)ecx + ds_base, 0);
    layer_slide(layer, 100, 50);
    layer_updown(layer, 3);
    // eaxにアプリに返すレジスタの値を渡す
    reg[7] = (int)layer;
  }
  else if(edx == 6){
    // ウィンドウへの文字表示API
    /*
    edx = 6, ebx = ウィンドウの番号, esi = 表示位置のx座標
    edi = 表示位置のy座標, eax =　色番号, ecx = 文字列の長さ
    ebp = 文字列
    */
    layer = (LAYER*) ebx;
    put_string(layer->buf, layer->bxsize, esi, edi, eax, (char*)ebp + ds_base);
    layer_refresh(layer, esi, edi, esi + ecx * 8, edi + 16);
  }
  else if(edx == 7){
    // 四角形描画API
    /*
    edx = 7, ebx = ウィンドウの番号, eax = x, ecx = y
    esi = width, edi =　height, ebp = 色番号
    */
    layer = (LAYER*) ebx;
    draw_rectangle(layer->buf, layer->bxsize, ebp, eax, ecx, esi, edi);
    layer_refresh(layer->buf, eax, ecx, eax + esi, ecx + edi);
  }
  return 0;
}
