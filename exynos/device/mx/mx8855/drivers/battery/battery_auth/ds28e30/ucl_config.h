/*******************************************************************************
 * Copyright (C) 2015 Maxim Integrated Products, Inc., All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Maxim Integrated shall
 * not be used except as stated in the Maxim Integrated Products, Inc. Branding Policy.
 *
 * The mere transfer of this software does not imply any licenses
 * of trade secrets, proprietary technology, copyrights, patents,
 * trademarks, maskwork rights, or any other form of intellectual
 * property whatsoever. Maxim Integrated Products, Inc. retains all ownership rights.
 *******************************************************************************/

#ifndef _UCL_CONFIG_H_
#define _UCL_CONFIG_H_


#ifdef __MINGW32__

#  ifdef BUILD_SHARED_LIB
#    define __API__ __declspec(dllexport)
#  else
#    define __API__ __declspec(dllimport)
#  endif

#elif defined __GCC__

#  ifdef BUILD_SHARED_LIB
#    if __GNUC__ >= 4
#      define __API__ __attribute__ ((visibility ("default")))
#    else
#      define __API__
#    endif
#  else
#    define __API__
#  endif

#else
#define __API__
#endif

/* JIBE_LINUX_CRYPTO_HW */
#define JIBE_LINUX_CRYPTO_HW


/*#if defined (__jibe) && !defined (__linux)
#ifndef JIBE_COBRA
#define JIBE_COBRA
#endif
#endif*/

#if defined (__jibe) && defined (__linux) && defined(JIBE_LINUX_CRYPTO_HW)
#warning JIBE target will use the userland API to the kernel crypto drivers
#define JIBE_LINUX_HW
#endif

/** <b>UCL Stack default size</b>.
 * 8 Ko.
 * @ingroup UCL_CONFIG */
#define UCL_STACK_SIZE (8*1024)

/** <b>UCL RSA key max size</b>.
 * 512 bytes: 4096 bits.
 * @ingroup UCL_CONFIG
 */
//1024 is ok on mingw for rsa encrypt up to 3072
//but seems to be too large for jibe stack
#define UCL_RSA_KEY_MAXSIZE 512

/** <b>UCL RSA public exponent max size</b>.
 * 4 bytes: 32 bits.
 * @ingroup UCL_CONFIG */
#define UCL_RSA_PUBLIC_EXPONENT_MAXSIZE 4

/** <b>UCL ECC Precision</b>.
 * @ingroup UCL_CONFIG */
#define UCL_ECC_PRECISION 17

#endif /*_UCL_CONFIG_H_*/
