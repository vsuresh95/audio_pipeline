#ifndef DATA_TYPE_H
#define DATA_TYPE_H

#if (USE_INT == 1)
typedef int audio_t;
#else
typedef float audio_t;
#endif

typedef int device_t;

#ifndef NO_DATA_INIT
#define MyMemset memset
#else
#define MyMemset
#endif

#endif // DATA_TYPE_H