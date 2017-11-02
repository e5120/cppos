// GDTの情報
typedef struct SEGMENT_DESCRIPTION{
  short limit_low;
  short base_low;
  char base_mid;
  char access_right;
  char limit_high;
  char base_high;
}SEG_DESC;

// IDTの情報
typedef struct GATE_DESCRIPTION{
  short offset_low;
  short selector;
  char dw_count;
  char access_right;
  short offset_high;
}GATE_DESC;

// プロトタイプ宣言
void init_gdtidt(void);
void set_segmdesc(SEG_DESC* sd, unsigned int limit, int base, int ar);
void set_gatedesc(GATE_DESC* gd, int offset, int selector, int ar);
