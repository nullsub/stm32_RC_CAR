/**
 ******************************************************************************
 *
 * @file       printf.h
 * @author     Stephen Caudle Copyright (C) 2010.
 * @brief      Main header.
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef TPRINTF_H
#define TPRINTF_H

/* writes bytes to the serial port*/
int data_out(char *data_to_write, unsigned int nr_of_bytes);

/**
 * @brief  Tiny embedded version of printf
 * @param  format The format string
 * @retval On success, the total number of characters written is returned.
 *         On failure, a negative number is returned.
 */
#ifdef USE_TERMINAL
int tprintf(const char *format, ...);
#endif

/**
 * @brief  Tiny embedded version of sprintf
 * @param  out The output character buffer
 * @param  format The format string
 * @retval On success, the total number of characters written is returned.
 *         On failure, a negative number is returned.
 */
#ifdef USE_TERMINAL
int tsprintf(char *out, const char *format, ...);
#endif

/**
 * @brief  Tiny embedded version of snprintf
 * @param  out The output character buffer
 * @param  count The total number of characters to output
 * @param  format The format string
 * @retval On success, the total number of characters written is returned.
 *         On failure, a negative number is returned.
 */
#ifdef USE_TERMINAL
int tsnprintf(char *out, unsigned int count, const char *format, ...);
#endif

#endif				/* TPRINTF_H */
