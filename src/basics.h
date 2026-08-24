#ifndef BASICS_H_
#define BASICS_H_

#define COUNTOF(arr)    (sizeof(arr) / sizeof(*(arr)))
#define LENOF(str)      (COUNTOF(str) - 1)
#define S(str)          (str), LENOF(str)

#define FOREACH(type, var, arr) for (type var = (arr); var < &(arr)[COUNTOF(arr)]; var++)

typedef unsigned char byte;
typedef signed char sbyte;

#endif // BASICS_H_
