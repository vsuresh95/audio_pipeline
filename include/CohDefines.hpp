#ifndef COH_DEFINES_H
#define COH_DEFINES_H

///////////////////////////////////////////////////////////////
// Helper unions
///////////////////////////////////////////////////////////////
typedef union
{
  struct
  {
    unsigned int r_en   : 1;
    unsigned int r_op   : 1;
    unsigned int r_type : 2;
    unsigned int r_cid  : 4;
    unsigned int w_en   : 1;
    unsigned int w_op   : 1;
    unsigned int w_type : 2;
    unsigned int w_cid  : 4;
	unsigned int reserved: 16;
  };
  unsigned int spandex_reg;
} spandex_config_t;

typedef union
{
  struct
  {
    int32_t value_32_1;
    int32_t value_32_2;
  };
  int64_t value_64;
} spandex_token_t;

///////////////////////////////////////////////////////////////
// Choosing the read, write code and coherence register config
///////////////////////////////////////////////////////////////
#define QUAUX(X) #X
#define QU(X) QUAUX(X)

#define ESP_NON_COHERENT_DMA 3
#define ESP_LLC_COHERENT_DMA 2
#define ESP_COHERENT_DMA 1
#define ESP_BASELINE_MESI 0

#define SPX_OWNER_PREDICTION 3
#define SPX_WRITE_THROUGH_FWD 2
#define SPX_BASELINE_REQV 1
#define SPX_BASELINE_MESI 0

///////////////////////////////////////////////////////////////
// Choosing the read and write code
///////////////////////////////////////////////////////////////
#if (IS_ESP == 1)
#define READ_CODE 0x0002B30B
#define WRITE_CODE 0x0062B02B
#else
#if (COH_MODE == SPX_OWNER_PREDICTION)
#define READ_CODE 0x4002B30B
#define WRITE_CODE 0x2262B82B
#elif (COH_MODE == SPX_WRITE_THROUGH_FWD)
#define READ_CODE 0x4002B30B
#define WRITE_CODE 0x2062B02B
#elif (COH_MODE == SPX_BASELINE_REQV)
#define READ_CODE 0x2002B30B
#define WRITE_CODE 0x0062B02B
#else
#define READ_CODE 0x0002B30B
#define WRITE_CODE 0x0062B02B
#endif
#endif

#endif // COH_DEFINES_H