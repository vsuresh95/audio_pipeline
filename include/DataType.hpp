#ifndef DATA_TYPE_H
#define DATA_TYPE_H

#if (USE_INT == 1)
typedef int audio_t;
#else
typedef float audio_t;
#endif

typedef int device_t;

#if (DO_DATA_INIT == 1)
#define MyMemset memset
#else
#define MyMemset
#endif

#endif // DATA_TYPE_H