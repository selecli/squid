#ifndef _SIZEDEF_H
#define _SIZEDEF_H 1

#define DATAMOD_ILP32  1


#ifdef DATAMOD_LP64 
	/** int 为32位，long = Pointer = 64位 */

	typedef unsigned char SZ_UBYTE1;
	typedef unsigned short SZ_UBYTE2;
	typedef signed char SZ_BYTE1;
	typedef signed short SZ_BYTE2;

	typedef float SZ_FLOAT;
	typedef double SZ_DOUBLE;
	typedef unsigned char SZ_UCHAR;
	typedef signed char SZ_CHAR;
	typedef unsigned short SZ_USHORT;
	typedef signed short SZ_SHORT;
	typedef signed short SZ_WCHAR;
	typedef unsigned short SZ_UWCHAR;

	typedef unsigned int SZ_UBYTE4;
	typedef signed int SZ_BYTE4;
	typedef unsigned long SZ_UBYTE8;
	typedef signed long SZ_BYTE8;

	typedef unsigned int SZ_UINT;
	typedef signed int SZ_INT;
	typedef unsigned long SZ_ULONG;
	typedef signed long SZ_LONG;

	typedef unsigned long SZ_UPTR;
	typedef signed long SZ_PTR;
	typedef unsigned long SZ_URESULT;
	typedef signed long SZ_RESULT;
	typedef unsigned long SZ_UT;
	typedef signed long SZ_T;

#elif DATAMOD_ILP64 	
	/*  int 是 64位, long 为 64位 */

	typedef unsigned char SZ_UBYTE1;
	typedef unsigned short SZ_UBYTE2;
	typedef signed char SZ_BYTE1;
	typedef signed short SZ_BYTE2;

	typedef float SZ_FLOAT;
	typedef double SZ_DOUBLE;
	typedef unsigned char SZ_UCHAR;
	typedef signed char SZ_CHAR;
	typedef unsigned short SZ_USHORT;
	typedef signed short SZ_SHORT;
	typedef signed short SZ_WCHAR;
	typedef unsigned short SZ_UWCHAR;

	typedef unsigned int SZ_UBYTE4;
	typedef signed int SZ_BYTE4;
	typedef unsigned long SZ_UBYTE8;
	typedef signed long SZ_BYTE8;

	typedef unsigned int SZ_UINT;
	typedef signed int SZ_INT;
	typedef unsigned long SZ_ULONG;
	typedef signed long SZ_LONG;

	typedef unsigned long SZ_UPTR;
	typedef signed long SZ_PTR;
	typedef unsigned long SZ_UT;
	typedef signed long SZ_T;

#elif DATAMOD_LLP64 
	/* int 为 32位，long 为 32位 */

	typedef unsigned char SZ_UBYTE1;
	typedef unsigned short SZ_UBYTE2;
	typedef signed char SZ_BYTE1;
	typedef signed short SZ_BYTE2;

	typedef float SZ_FLOAT;
	typedef double SZ_DOUBLE;
	typedef unsigned char SZ_UCHAR;
	typedef signed char SZ_CHAR;
	typedef unsigned short SZ_USHORT;
	typedef signed short SZ_SHORT;
	typedef signed short SZ_WCHAR;
	typedef unsigned short SZ_UWCHAR;

	typedef unsigned int SZ_UBYTE4;
	typedef signed int SZ_BYTE4;
	typedef unsigned long long SZ_UBYTE8; /* 可能实现中将其定义更大 */
	typedef signed long long SZ_BYTE8;

	typedef unsigned int SZ_UINT;
	typedef signed int SZ_INT;
	typedef unsigned long long SZ_ULONG;
	typedef signed long long SZ_LONG;
	
	typedef unsigned long long SZ_UPTR;
	typedef signed long long SZ_PTR;
	typedef unsigned long long SZ_UT;
	typedef signed long long SZ_T;

#elif DATAMOD_ILP32 
	/* int 为 32位, long 为32 位 */
	typedef unsigned char SZ_UBYTE1;
	typedef unsigned short SZ_UBYTE2;
	typedef signed char SZ_BYTE1;
	typedef signed short SZ_BYTE2;

	typedef float SZ_FLOAT;
	typedef double SZ_DOUBLE;
	typedef unsigned char SZ_UCHAR;
	typedef signed char SZ_CHAR;
	typedef unsigned short SZ_USHORT;
	typedef signed short SZ_SHORT;
	typedef signed short SZ_WCHAR;
	typedef unsigned short SZ_UWCHAR;


	typedef unsigned int SZ_UBYTE4;
	typedef signed int SZ_BYTE4;
	typedef unsigned long long  SZ_UBYTE8; /* 可能实现中将其定义更大 */
	typedef signed long long SZ_BYTE8;

	typedef unsigned int SZ_UINT;
	typedef signed int SZ_INT;
	typedef unsigned long long SZ_ULONG;
	typedef signed long long SZ_LONG;

	typedef unsigned int SZ_UPTR;
	typedef signed int SZ_PTR;
	typedef unsigned long SZ_UT;
	typedef signed long SZ_T;

#else 
	/* DATAMOD_LP32 int 为 16位 */
	typedef unsigned char SZ_UBYTE1;
	typedef unsigned short SZ_UBYTE2;
	typedef signed char SZ_BYTE1;
	typedef signed short SZ_BYTE2;

	typedef float SZ_FLOAT;
	typedef double SZ_DOUBLE;
	typedef unsigned char SZ_UCHAR;
	typedef signed char SZ_CHAR;
	typedef unsigned short SZ_USHORT;
	typedef signed short SZ_SHORT;
	typedef signed short SZ_WCHAR;
	typedef unsigned short SZ_UWCHAR;

	typedef unsigned long SZ_UBYTE4;
	typedef signed long SZ_BYTE4;
	typedef unsigned long long SZ_UBYTE8; /* 可能实现中将其定义更大 */
	typedef signed long long int SZ_BYTE8;

	typedef unsigned int SZ_UPTR;
	typedef signed int SZ_PTR;

	typedef unsigned long SZ_UINT;
	typedef signed long SZ_INT;
	typedef unsigned long long SZ_ULONG; /** 16位系统可能不会用到，也没有这个内建关键字 */
	typedef signed long long SZ_LONG;
	typedef unsigned int SZ_UT;
	typedef signed int SZ_T;
#endif

#endif
