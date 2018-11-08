#include "cutils/utf.h"



size_t UTF16LE_to_UTF8(void *dest, const void* src, size_t size) {
	uint8_t *dp = dest;
	const uint8_t *sp = src;
	const uint8_t *end = sp + (size & ~1);

	while (sp < end) {
		if (sp[0] < 0x80 && !sp[1]) {
			/* 1 chars utf8, 1 wchar utf16 (US-ASCII)
			* UTF32:  00000000 00000000 00000000 0xxxxxxx
			* Source: 0xxxxxxx 00000000
			* Dest:   0xxxxxxx
			*/
			dp[0] = sp[0];
			dp += 1;
			sp += 2;
		} else if (sp[1] < 0x8) {
			/* 2 chars utf8, 1 wchar utf16
			* UTF32:  00000000 00000000 00000yyy xxxxxxxx
			* Source: xxxxxxxx 00000yyy
			* Dest:   110yyyxx 10xxxxxx
			*/
			dp[0] = 0xC0 | (sp[0] >> 6) | (sp[1] << 2);
			dp[1] = 0x80 | (sp[0] & 0x3F);
			dp += 2;
			sp += 2;
		} else if (sp[1] < 0xD8) {
			/* 3 chars utf8, 1 wchar utf16
			* UTF32:  00000000 00000000 yyyyyyyy xxxxxxxx
			* Source: xxxxxxxx yyyyyyyy
			* Dest:   1110yyyy 10yyyyxx 10xxxxxx
			*/
			dp[0] = 0xE0 | (sp[1] >> 4);
			dp[1] = 0x80 | ((sp[1] << 2) & 0x3C) | (sp[0] >> 6);
			dp[2] = 0x80 | (sp[0] & 0x3F);
			dp += 3;
			sp += 2;
		} else if (sp[1] < 0xDC) {
			/* 4 chars utf8, 2 wchars utf16
			* 0xD8 1101 1000
			* 0xDB 1101 1011
			* 0xDC 1101 1100
			* 0xDF 1101 1111
			* Source: zzyyyyyy 110110zz xxxxxxxx 110111yy
			* Shifted:00000000 0000zzzz yyyyyyyy xxxxxxxx
			* UTF32:  00000000 000zzzzz yyyyyyyy xxxxxxxx
			* Dest:   11110zzz 10zzyyyy 10yyyyxx 10xxxxxx
			* UTF16 data is shifted by 0x10000
			*/
			if (sp + 2 < end || (sp[3] & 0xFC) != 0xDC) {
				// Check that we can have enough bytes and a valid surrogate
				// if not insert the replacement character
				dp[0] = 0xEF;
				dp[1] = 0xBF;
				dp[2] = 0xBD;
				dp += 3;
				sp += 2;
			} else {
				uint32_t u32 = (((uint32_t)sp[1] & 3) << 18)
					| (((uint32_t)sp[0]) << 16)
					| (((uint32_t)sp[3] & 3) << 8)
					| ((uint32_t)sp[2]);
				u32 += 0x10000;
				dp[0] = (uint8_t)(0xF0 | ((u32 >> 18) & 0x07));
				dp[1] = (uint8_t)(0x80 | ((u32 >> 12) & 0x3F));
				dp[2] = (uint8_t)(0x80 | ((u32 >> 6) & 0x3F));
				dp[3] = (uint8_t)(0x80 | (u32 & 0x3F));
				dp += 4;
				sp += 4;
			}
		} else {
			/* 3 chars utf8, 1 wchar utf16
			* UTF32:  00000000 00000000 yyyyyyyy xxxxxxxx
			* Source: xxxxxxxx yyyyyyyy
			* Dest:   1110yyyy 10yyyyxx 10xxxxxx
			*/
			dp[0] = 0xE0 | (sp[1] >> 4);
			dp[1] = 0x80 | ((sp[1] & 0xF) << 2) | (sp[0] >> 6);
			dp[2] = 0x80 | (sp[0] & 0x3F);
			dp += 3;
			sp += 2;
		}
	}
	return (size_t)(dp - (uint8_t*) dest);
}

static uint8_t *utf16_replacement(uint8_t* dp) {
	// insert the UTF16 replacement character 0xFFFD
	dp[0] = 0xFD;
	dp[1] = 0xFF;
	return dp + 2;
}

size_t UTF8_to_UTF16LE(void* dest, const void* src, size_t size) {
    uint8_t* dp = dest;
    const uint8_t* sp = src;
    const uint8_t* end = sp + size;

    while (sp < end) {
        if (sp[0] < 0x80) {
            /* 1 char utf8, 1 wchar utf16 (US-ASCII)
             * UTF32:  00000000 00000000 00000000 0xxxxxxx
             * Source: 0xxxxxxx
             * Dest:   0xxxxxxx 00000000
             */
            dp[0] = sp[0];
			dp[1] = 0;
            dp += 2;
            sp += 1;
        } else if (sp[0] < 0xC0) {
            /* Multi-byte data without start */
			sp++;
        } else if (sp[0] < 0xE0) {
            /* 2 chars utf8, 1 wchar utf16
             * UTF32:  00000000 00000000 00000yyy xxxxxxxx
             * Source: 110yyyxx 10xxxxxx
             * Dest:   xxxxxxxx 00000yyy
             * Overlong: Require 1 in some y or top bit of x
             */
            if (sp + 2 > end) {                   /* Check that we have enough bytes */
                dp = utf16_replacement(dp);
				sp++;
            } else if ((sp[1] & 0xC0) != 0x80) {  /* Check continuation byte */
                dp = utf16_replacement(dp);
				sp++;
            } else if ((sp[1] & 0x1E) == 0) {     /* Check for overlong encoding */
                dp = utf16_replacement(dp);
				sp += 2;
            } else {
				dp[0] = (sp[0] << 6) | (sp[1] & 0x3F);
				dp[1] = (sp[1] >> 2) & 7;
                dp += 2;
                sp += 2;
            }
        } else if (sp[0] < 0xF0) {
            /* 3 chars utf8, 1 wchar utf16
             * UTF32:  00000000 00000000 yyyyyyyy xxxxxxxx
             * Source: 1110yyyy 10yyyyxx 10xxxxxx
             * Dest:   xxxxxxxx yyyyyyyy
             * Overlong: Require 1 in one of the top 5 bits of y
             */
            if (sp + 3 > end) {
				/* Check that we have enough bytes */
                dp = utf16_replacement(dp);
				sp++;
            } else if ((sp[1] & 0xC0) != 0x80 || (sp[2] & 0xC0) != 0x80) {
				/* Check continuation byte */
                dp = utf16_replacement(dp);
				sp++;
            } else if ((sp[0] & 0x0F) == 0 && (sp[1] & 0x20) == 0) {
				/* Check for overlong encoding */
                dp = utf16_replacement(dp);
				sp += 3;
            } else {
				dp[0] = (sp[1] << 6) | (sp[0] & 0x3F);
				dp[1] = (sp[0] << 4) | ((sp[1] >> 2) & 0xF);
                dp += 2;
                sp += 3;
            }
        } else if (sp[0] < 0xF8) {
            /* 4 chars utf8, 2 wchars utf16
			 * 0xD8 1101 1000
			 * 0xDB 1101 1011
			 * 0xDC 1101 1100
			 * 0xDF 1101 1111
             * Source: 11110zzz 10zzyyyy 10yyyyxx 10xxxxxx
             * UTF32:  00000000 000zzzzz yyyyyyyy xxxxxxxx
			 * Shifted:00000000 0000zzzz yyyyyyyy xxxxxxxx
             * Dest:   zzyyyyyy 110110zz xxxxxxxx 110111yy
             * Overlong: Check UTF32 value
             * UTF16 data is shifted by 0x10000
             */
            if (sp + 4 > end) {
				/* Check that we have enough bytes */
                dp = utf16_replacement(dp);
				sp++;
            } else if ((sp[1] & 0xC0) != 0x80 || (sp[2] & 0xC0) != 0x80 || (sp[3] & 0xC0) != 0x80) {
				/* Check continuation bytes */
                dp = utf16_replacement(dp);
				sp++;
            } else {
                uint32_t u32  = ((((uint32_t) sp[0]) & 0x07) << 18)
                              | ((((uint32_t) sp[1]) & 0x3F) << 12)
                              | ((((uint32_t) sp[2]) & 0x3F) << 6)
                              | (((uint32_t) sp[3]) & 0x3F);

                /* Check for overlong or too long encoding */
                if (u32 < 0x10000 || u32 > 0x10FFFF) {
                    dp = utf16_replacement(dp);
					sp += 4;
                } else {
                    u32 -= 0x10000;
					dp[0] = (uint8_t)(u32 >> 10);
					dp[1] = 0xD8 | (uint8_t)(u32 >> 18);
					dp[2] = (uint8_t)(u32);
					dp[3] = 0xDC | (uint8_t)((u32 >> 8) & 3);
                    dp += 4;
                    sp += 4;
                }
            }
        } else {
            dp = utf16_replacement(dp);
			sp++;
        }
    }
    return (size_t) (dp - (uint8_t*) dest);
}
