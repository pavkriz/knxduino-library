/*
 *  internal/functions.h - Library internal shared functions
 *
 *  Copyright (c) 2014 Stefan Taferner <stefan.taferner@gmx.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3 as
 *  published by the Free Software Foundation.
 */
#ifndef sblib_internal_functions_h
#define sblib_internal_functions_h


/*
 *  These functions are private library functions. Do not call from
 *  outside the library. They may be changed without warning.
 */


/*
 * Read userEeprom from Flash. (user_memory.cpp)
 */
void readUserEeprom();

// Write userEeprom to Flash. (user_memory.cpp)
void writeUserEeprom();

/*
 * Send the next communication object that is flagged to be sent.
 * Returns true if a group telegram has been sent.
 */
bool sendNextGroupTelegram();


#endif /*sblib_internal_functions_h*/
