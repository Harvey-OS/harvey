/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

long tab8859_1[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
    0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53,
    0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
    0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83,
    0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
    0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3,
    0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb,
    0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3,
    0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb,
    0xfc, 0xfd, 0xfe, 0xff,
};

long tab8859_2[256] = {
    0x00,   0x01,   0x02,   0x03,   0x04,   0x05,   0x06,   0x07,   0x08,
    0x09,   0x0a,   0x0b,   0x0c,   0x0d,   0x0e,   0x0f,   0x10,   0x11,
    0x12,   0x13,   0x14,   0x15,   0x16,   0x17,   0x18,   0x19,   0x1a,
    0x1b,   0x1c,   0x1d,   0x1e,   0x1f,   0x20,   0x21,   0x22,   0x23,
    0x24,   0x25,   0x26,   0x27,   0x28,   0x29,   0x2a,   0x2b,   0x2c,
    0x2d,   0x2e,   0x2f,   0x30,   0x31,   0x32,   0x33,   0x34,   0x35,
    0x36,   0x37,   0x38,   0x39,   0x3a,   0x3b,   0x3c,   0x3d,   0x3e,
    0x3f,   0x40,   0x41,   0x42,   0x43,   0x44,   0x45,   0x46,   0x47,
    0x48,   0x49,   0x4a,   0x4b,   0x4c,   0x4d,   0x4e,   0x4f,   0x50,
    0x51,   0x52,   0x53,   0x54,   0x55,   0x56,   0x57,   0x58,   0x59,
    0x5a,   0x5b,   0x5c,   0x5d,   0x5e,   0x5f,   0x60,   0x61,   0x62,
    0x63,   0x64,   0x65,   0x66,   0x67,   0x68,   0x69,   0x6a,   0x6b,
    0x6c,   0x6d,   0x6e,   0x6f,   0x70,   0x71,   0x72,   0x73,   0x74,
    0x75,   0x76,   0x77,   0x78,   0x79,   0x7a,   0x7b,   0x7c,   0x7d,
    0x7e,   0x7f,   0x80,   0x81,   0x82,   0x83,   0x84,   0x85,   0x86,
    0x87,   0x88,   0x89,   0x8a,   0x8b,   0x8c,   0x8d,   0x8e,   0x8f,
    0x90,   0x91,   0x92,   0x93,   0x94,   0x95,   0x96,   0x97,   0x98,
    0x99,   0x9a,   0x9b,   0x9c,   0x9d,   0x9e,   0x9f,   0x00a0, 0x0104,
    0x02d8, 0x0141, 0x00a4, 0x013d, 0x015a, 0x00a7, 0x00a8, 0x0160, 0x015e,
    0x0164, 0x0179, 0x00ad, 0x017d, 0x017b, 0x00b0, 0x0105, 0x02db, 0x0142,
    0x00b4, 0x013e, 0x015b, 0x02c7, 0x00b8, 0x0161, 0x015f, 0x0165, 0x017a,
    0x02dd, 0x017e, 0x017c, 0x0154, 0x00c1, 0x00c2, 0x0102, 0x00c4, 0x0139,
    0x0106, 0x00c7, 0x010c, 0x00c9, 0x0118, 0x00cb, 0x011a, 0x00cd, 0x00ce,
    0x010e, 0x0110, 0x0143, 0x0147, 0x00d3, 0x00d4, 0x0150, 0x00d6, 0x00d7,
    0x0158, 0x016e, 0x00da, 0x0170, 0x00dc, 0x00dd, 0x0162, 0x00df, 0x0155,
    0x00e1, 0x00e2, 0x0103, 0x00e4, 0x013a, 0x0107, 0x00e7, 0x010d, 0x00e9,
    0x0119, 0x00eb, 0x011b, 0x00ed, 0x00ee, 0x010f, 0x0111, 0x0144, 0x0148,
    0x00f3, 0x00f4, 0x0151, 0x00f6, 0x00f7, 0x0159, 0x016f, 0x00fa, 0x0171,
    0x00fc, 0x00fd, 0x0163, 0x02d9,
};

long tab8859_3[256] = {
    0x00,   0x01,   0x02,   0x03,   0x04,   0x05,   0x06,   0x07,   0x08,
    0x09,   0x0a,   0x0b,   0x0c,   0x0d,   0x0e,   0x0f,   0x10,   0x11,
    0x12,   0x13,   0x14,   0x15,   0x16,   0x17,   0x18,   0x19,   0x1a,
    0x1b,   0x1c,   0x1d,   0x1e,   0x1f,   0x20,   0x21,   0x22,   0x23,
    0x24,   0x25,   0x26,   0x27,   0x28,   0x29,   0x2a,   0x2b,   0x2c,
    0x2d,   0x2e,   0x2f,   0x30,   0x31,   0x32,   0x33,   0x34,   0x35,
    0x36,   0x37,   0x38,   0x39,   0x3a,   0x3b,   0x3c,   0x3d,   0x3e,
    0x3f,   0x40,   0x41,   0x42,   0x43,   0x44,   0x45,   0x46,   0x47,
    0x48,   0x49,   0x4a,   0x4b,   0x4c,   0x4d,   0x4e,   0x4f,   0x50,
    0x51,   0x52,   0x53,   0x54,   0x55,   0x56,   0x57,   0x58,   0x59,
    0x5a,   0x5b,   0x5c,   0x5d,   0x5e,   0x5f,   0x60,   0x61,   0x62,
    0x63,   0x64,   0x65,   0x66,   0x67,   0x68,   0x69,   0x6a,   0x6b,
    0x6c,   0x6d,   0x6e,   0x6f,   0x70,   0x71,   0x72,   0x73,   0x74,
    0x75,   0x76,   0x77,   0x78,   0x79,   0x7a,   0x7b,   0x7c,   0x7d,
    0x7e,   0x7f,   0x80,   0x81,   0x82,   0x83,   0x84,   0x85,   0x86,
    0x87,   0x88,   0x89,   0x8a,   0x8b,   0x8c,   0x8d,   0x8e,   0x8f,
    0x90,   0x91,   0x92,   0x93,   0x94,   0x95,   0x96,   0x97,   0x98,
    0x99,   0x9a,   0x9b,   0x9c,   0x9d,   0x9e,   0x9f,   0x00a0, 0x0126,
    0x02d8, 0x00a3, 0x00a4, -1,     0x0124, 0x00a7, 0x00a8, 0x0130, 0x015e,
    0x011e, 0x0134, 0x00ad, -1,     0x017b, 0x00b0, 0x0127, 0x00b2, 0x00b3,
    0x00b4, 0x00b5, 0x0125, 0x00b7, 0x00b8, 0x0131, 0x015f, 0x011f, 0x0135,
    0x00bd, -1,     0x017c, 0x00c0, 0x00c1, 0x00c2, -1,     0x00c4, 0x010a,
    0x0108, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce,
    0x00cf, -1,     0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x0120, 0x00d6, 0x00d7,
    0x011c, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x016c, 0x015c, 0x00df, 0x00e0,
    0x00e1, 0x00e2, -1,     0x00e4, 0x010b, 0x0109, 0x00e7, 0x00e8, 0x00e9,
    0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef, -1,     0x00f1, 0x00f2,
    0x00f3, 0x00f4, 0x0121, 0x00f6, 0x00f7, 0x011d, 0x00f9, 0x00fa, 0x00fb,
    0x00fc, 0x016d, 0x015d, 0x02d9,
};

long tab8859_4[256] = {
    0x00,   0x01,   0x02,   0x03,   0x04,   0x05,   0x06,   0x07,   0x08,
    0x09,   0x0a,   0x0b,   0x0c,   0x0d,   0x0e,   0x0f,   0x10,   0x11,
    0x12,   0x13,   0x14,   0x15,   0x16,   0x17,   0x18,   0x19,   0x1a,
    0x1b,   0x1c,   0x1d,   0x1e,   0x1f,   0x20,   0x21,   0x22,   0x23,
    0x24,   0x25,   0x26,   0x27,   0x28,   0x29,   0x2a,   0x2b,   0x2c,
    0x2d,   0x2e,   0x2f,   0x30,   0x31,   0x32,   0x33,   0x34,   0x35,
    0x36,   0x37,   0x38,   0x39,   0x3a,   0x3b,   0x3c,   0x3d,   0x3e,
    0x3f,   0x40,   0x41,   0x42,   0x43,   0x44,   0x45,   0x46,   0x47,
    0x48,   0x49,   0x4a,   0x4b,   0x4c,   0x4d,   0x4e,   0x4f,   0x50,
    0x51,   0x52,   0x53,   0x54,   0x55,   0x56,   0x57,   0x58,   0x59,
    0x5a,   0x5b,   0x5c,   0x5d,   0x5e,   0x5f,   0x60,   0x61,   0x62,
    0x63,   0x64,   0x65,   0x66,   0x67,   0x68,   0x69,   0x6a,   0x6b,
    0x6c,   0x6d,   0x6e,   0x6f,   0x70,   0x71,   0x72,   0x73,   0x74,
    0x75,   0x76,   0x77,   0x78,   0x79,   0x7a,   0x7b,   0x7c,   0x7d,
    0x7e,   0x7f,   0x80,   0x81,   0x82,   0x83,   0x84,   0x85,   0x86,
    0x87,   0x88,   0x89,   0x8a,   0x8b,   0x8c,   0x8d,   0x8e,   0x8f,
    0x90,   0x91,   0x92,   0x93,   0x94,   0x95,   0x96,   0x97,   0x98,
    0x99,   0x9a,   0x9b,   0x9c,   0x9d,   0x9e,   0x9f,   0x00a0, 0x0104,
    0x0138, 0x0156, 0x00a4, 0x0128, 0x013b, 0x00a7, 0x00a8, 0x0160, 0x0112,
    0x0122, 0x0166, 0x00ad, 0x017d, 0x00af, 0x00b0, 0x0105, 0x02db, 0x0157,
    0x00b4, 0x0129, 0x013c, 0x02c7, 0x00b8, 0x0161, 0x0113, 0x0123, 0x0167,
    0x014a, 0x017e, 0x014b, 0x0100, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5,
    0x00c6, 0x012e, 0x010c, 0x00c9, 0x0118, 0x00cb, 0x0116, 0x00cd, 0x00ce,
    0x012a, 0x0110, 0x0145, 0x014c, 0x0136, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
    0x00d8, 0x0172, 0x00da, 0x00db, 0x00dc, 0x0168, 0x016a, 0x00df, 0x0101,
    0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x012f, 0x010d, 0x00e9,
    0x0119, 0x00eb, 0x0117, 0x00ed, 0x00ee, 0x012b, 0x0111, 0x0146, 0x014d,
    0x0137, 0x00f4, 0x00f5, 0x00f6, 0x00f7, 0x00f8, 0x0173, 0x00fa, 0x00fb,
    0x00fc, 0x0169, 0x016b, 0x02d9,
};

long tab8859_5[256] = {
    0x00,   0x01,   0x02,   0x03,   0x04,   0x05,   0x06,   0x07,   0x08,
    0x09,   0x0a,   0x0b,   0x0c,   0x0d,   0x0e,   0x0f,   0x10,   0x11,
    0x12,   0x13,   0x14,   0x15,   0x16,   0x17,   0x18,   0x19,   0x1a,
    0x1b,   0x1c,   0x1d,   0x1e,   0x1f,   0x20,   0x21,   0x22,   0x23,
    0x24,   0x25,   0x26,   0x27,   0x28,   0x29,   0x2a,   0x2b,   0x2c,
    0x2d,   0x2e,   0x2f,   0x30,   0x31,   0x32,   0x33,   0x34,   0x35,
    0x36,   0x37,   0x38,   0x39,   0x3a,   0x3b,   0x3c,   0x3d,   0x3e,
    0x3f,   0x40,   0x41,   0x42,   0x43,   0x44,   0x45,   0x46,   0x47,
    0x48,   0x49,   0x4a,   0x4b,   0x4c,   0x4d,   0x4e,   0x4f,   0x50,
    0x51,   0x52,   0x53,   0x54,   0x55,   0x56,   0x57,   0x58,   0x59,
    0x5a,   0x5b,   0x5c,   0x5d,   0x5e,   0x5f,   0x60,   0x61,   0x62,
    0x63,   0x64,   0x65,   0x66,   0x67,   0x68,   0x69,   0x6a,   0x6b,
    0x6c,   0x6d,   0x6e,   0x6f,   0x70,   0x71,   0x72,   0x73,   0x74,
    0x75,   0x76,   0x77,   0x78,   0x79,   0x7a,   0x7b,   0x7c,   0x7d,
    0x7e,   0x7f,   0x80,   0x81,   0x82,   0x83,   0x84,   0x85,   0x86,
    0x87,   0x88,   0x89,   0x8a,   0x8b,   0x8c,   0x8d,   0x8e,   0x8f,
    0x90,   0x91,   0x92,   0x93,   0x94,   0x95,   0x96,   0x97,   0x98,
    0x99,   0x9a,   0x9b,   0x9c,   0x9d,   0x9e,   0x9f,   0x00a0, 0x0401,
    0x0402, 0x0403, 0x0404, 0x0405, 0x0406, 0x0407, 0x0408, 0x0409, 0x040a,
    0x040b, 0x040c, 0x00ad, 0x040e, 0x040f, 0x0410, 0x0411, 0x0412, 0x0413,
    0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041a, 0x041b, 0x041c,
    0x041d, 0x041e, 0x041f, 0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425,
    0x0426, 0x0427, 0x0428, 0x0429, 0x042a, 0x042b, 0x042c, 0x042d, 0x042e,
    0x042f, 0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
    0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, 0x043f, 0x0440,
    0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449,
    0x044a, 0x044b, 0x044c, 0x044d, 0x044e, 0x044f, 0x2116, 0x0451, 0x0452,
    0x0453, 0x0454, 0x0455, 0x0456, 0x0457, 0x0458, 0x0459, 0x045a, 0x045b,
    0x045c, 0x00a7, 0x045e, 0x045f,
};

long tab8859_6[256] = {
    0x00,   0x01,   0x02,   0x03,   0x04,   0x05,   0x06,   0x07,   0x08,
    0x09,   0x0a,   0x0b,   0x0c,   0x0d,   0x0e,   0x0f,   0x10,   0x11,
    0x12,   0x13,   0x14,   0x15,   0x16,   0x17,   0x18,   0x19,   0x1a,
    0x1b,   0x1c,   0x1d,   0x1e,   0x1f,   0x20,   0x21,   0x22,   0x23,
    0x24,   0x25,   0x26,   0x27,   0x28,   0x29,   0x2a,   0x2b,   0x2c,
    0x2d,   0x2e,   0x2f,   0x30,   0x31,   0x32,   0x33,   0x34,   0x35,
    0x36,   0x37,   0x38,   0x39,   0x3a,   0x3b,   0x3c,   0x3d,   0x3e,
    0x3f,   0x40,   0x41,   0x42,   0x43,   0x44,   0x45,   0x46,   0x47,
    0x48,   0x49,   0x4a,   0x4b,   0x4c,   0x4d,   0x4e,   0x4f,   0x50,
    0x51,   0x52,   0x53,   0x54,   0x55,   0x56,   0x57,   0x58,   0x59,
    0x5a,   0x5b,   0x5c,   0x5d,   0x5e,   0x5f,   0x60,   0x61,   0x62,
    0x63,   0x64,   0x65,   0x66,   0x67,   0x68,   0x69,   0x6a,   0x6b,
    0x6c,   0x6d,   0x6e,   0x6f,   0x70,   0x71,   0x72,   0x73,   0x74,
    0x75,   0x76,   0x77,   0x78,   0x79,   0x7a,   0x7b,   0x7c,   0x7d,
    0x7e,   0x7f,   0x80,   0x81,   0x82,   0x83,   0x84,   0x85,   0x86,
    0x87,   0x88,   0x89,   0x8a,   0x8b,   0x8c,   0x8d,   0x8e,   0x8f,
    0x90,   0x91,   0x92,   0x93,   0x94,   0x95,   0x96,   0x97,   0x98,
    0x99,   0x9a,   0x9b,   0x9c,   0x9d,   0x9e,   0x9f,   0x00a0, -1,
    -1,     -1,     0x00a4, -1,     -1,     -1,     -1,     -1,     -1,
    -1,     0x060c, 0x00ad, -1,     -1,     -1,     -1,     -1,     -1,
    -1,     -1,     -1,     -1,     -1,     -1,     -1,     0x061b, -1,
    -1,     -1,     0x061f, -1,     0x0621, 0x0622, 0x0623, 0x0624, 0x0625,
    0x0626, 0x0627, 0x0628, 0x0629, 0x062a, 0x062b, 0x062c, 0x062d, 0x062e,
    0x062f, 0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637,
    0x0638, 0x0639, 0x063a, -1,     -1,     -1,     -1,     -1,     0x0640,
    0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647, 0x0648, 0x0649,
    0x064a, 0x064b, 0x064c, 0x064d, 0x064e, 0x064f, 0x0650, 0x0651, 0x0652,
    -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
    -1,     -1,     -1,     -1,
};

long tab8859_7[256] = {
    0x00,   0x01,   0x02,   0x03,   0x04,   0x05,   0x06,   0x07,   0x08,
    0x09,   0x0a,   0x0b,   0x0c,   0x0d,   0x0e,   0x0f,   0x10,   0x11,
    0x12,   0x13,   0x14,   0x15,   0x16,   0x17,   0x18,   0x19,   0x1a,
    0x1b,   0x1c,   0x1d,   0x1e,   0x1f,   0x20,   0x21,   0x22,   0x23,
    0x24,   0x25,   0x26,   0x27,   0x28,   0x29,   0x2a,   0x2b,   0x2c,
    0x2d,   0x2e,   0x2f,   0x30,   0x31,   0x32,   0x33,   0x34,   0x35,
    0x36,   0x37,   0x38,   0x39,   0x3a,   0x3b,   0x3c,   0x3d,   0x3e,
    0x3f,   0x40,   0x41,   0x42,   0x43,   0x44,   0x45,   0x46,   0x47,
    0x48,   0x49,   0x4a,   0x4b,   0x4c,   0x4d,   0x4e,   0x4f,   0x50,
    0x51,   0x52,   0x53,   0x54,   0x55,   0x56,   0x57,   0x58,   0x59,
    0x5a,   0x5b,   0x5c,   0x5d,   0x5e,   0x5f,   0x60,   0x61,   0x62,
    0x63,   0x64,   0x65,   0x66,   0x67,   0x68,   0x69,   0x6a,   0x6b,
    0x6c,   0x6d,   0x6e,   0x6f,   0x70,   0x71,   0x72,   0x73,   0x74,
    0x75,   0x76,   0x77,   0x78,   0x79,   0x7a,   0x7b,   0x7c,   0x7d,
    0x7e,   0x7f,   0x80,   0x81,   0x82,   0x83,   0x84,   0x85,   0x86,
    0x87,   0x88,   0x89,   0x8a,   0x8b,   0x8c,   0x8d,   0x8e,   0x8f,
    0x90,   0x91,   0x92,   0x93,   0x94,   0x95,   0x96,   0x97,   0x98,
    0x99,   0x9a,   0x9b,   0x9c,   0x9d,   0x9e,   0x9f,   0x00a0, 0x2018,
    0x2019, 0x00a3, -1,     -1,     0x00a6, 0x00a7, 0x00a8, 0x00a9, -1,
    0x00ab, 0x00ac, 0x00ad, -1,     0x2015, 0x00b0, 0x00b1, 0x00b2, 0x00b3,
    0x0384, 0x0385, 0x0386, 0x00b7, 0x0388, 0x0389, 0x038a, 0x00bb, 0x038c,
    0x00bd, 0x038e, 0x038f, 0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395,
    0x0396, 0x0397, 0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e,
    0x039f, 0x03a0, 0x03a1, -1,     0x03a3, 0x03a4, 0x03a5, 0x03a6, 0x03a7,
    0x03a8, 0x03a9, 0x03aa, 0x03ab, 0x03ac, 0x03ad, 0x03ae, 0x03af, 0x03b0,
    0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7, 0x03b8, 0x03b9,
    0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf, 0x03c0, 0x03c1, 0x03c2,
    0x03c3, 0x03c4, 0x03c5, 0x03c6, 0x03c7, 0x03c8, 0x03c9, 0x03ca, 0x03cb,
    0x03cc, 0x03cd, 0x03ce, -1,
};

long tab8859_8[256] = {
    0x00,   0x01,   0x02,   0x03,   0x04,   0x05,   0x06,   0x07,   0x08,
    0x09,   0x0a,   0x0b,   0x0c,   0x0d,   0x0e,   0x0f,   0x10,   0x11,
    0x12,   0x13,   0x14,   0x15,   0x16,   0x17,   0x18,   0x19,   0x1a,
    0x1b,   0x1c,   0x1d,   0x1e,   0x1f,   0x20,   0x21,   0x22,   0x23,
    0x24,   0x25,   0x26,   0x27,   0x28,   0x29,   0x2a,   0x2b,   0x2c,
    0x2d,   0x2e,   0x2f,   0x30,   0x31,   0x32,   0x33,   0x34,   0x35,
    0x36,   0x37,   0x38,   0x39,   0x3a,   0x3b,   0x3c,   0x3d,   0x3e,
    0x3f,   0x40,   0x41,   0x42,   0x43,   0x44,   0x45,   0x46,   0x47,
    0x48,   0x49,   0x4a,   0x4b,   0x4c,   0x4d,   0x4e,   0x4f,   0x50,
    0x51,   0x52,   0x53,   0x54,   0x55,   0x56,   0x57,   0x58,   0x59,
    0x5a,   0x5b,   0x5c,   0x5d,   0x5e,   0x5f,   0x60,   0x61,   0x62,
    0x63,   0x64,   0x65,   0x66,   0x67,   0x68,   0x69,   0x6a,   0x6b,
    0x6c,   0x6d,   0x6e,   0x6f,   0x70,   0x71,   0x72,   0x73,   0x74,
    0x75,   0x76,   0x77,   0x78,   0x79,   0x7a,   0x7b,   0x7c,   0x7d,
    0x7e,   0x7f,   0x80,   0x81,   0x82,   0x83,   0x84,   0x85,   0x86,
    0x87,   0x88,   0x89,   0x8a,   0x8b,   0x8c,   0x8d,   0x8e,   0x8f,
    0x90,   0x91,   0x92,   0x93,   0x94,   0x95,   0x96,   0x97,   0x98,
    0x99,   0x9a,   0x9b,   0x9c,   0x9d,   0x9e,   0x9f,   0x00a0, -1,
    0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7, 0x00a8, 0x00a9, 0x00d7,
    0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x203e, 0x00b0, 0x00b1, 0x00b2, 0x00b3,
    0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00f7, 0x00bb, 0x00bc,
    0x00bd, 0x00be, -1,     -1,     -1,     -1,     -1,     -1,     -1,
    -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
    -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
    -1,     -1,     -1,     -1,     -1,     -1,     -1,     0x2017, 0x05d0,
    0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5, 0x05d6, 0x05d7, 0x05d8, 0x05d9,
    0x05da, 0x05db, 0x05dc, 0x05dd, 0x05de, 0x05df, 0x05e0, 0x05e1, 0x05e2,
    0x05e3, 0x05e4, 0x05e5, 0x05e6, 0x05e7, 0x05e8, 0x05e9, 0x05ea, -1,
    -1,     -1,     -1,     -1,
};

long tab8859_9[256] = {
    0x00,   0x01,   0x02,   0x03,   0x04,   0x05,   0x06,   0x07,   0x08,
    0x09,   0x0a,   0x0b,   0x0c,   0x0d,   0x0e,   0x0f,   0x10,   0x11,
    0x12,   0x13,   0x14,   0x15,   0x16,   0x17,   0x18,   0x19,   0x1a,
    0x1b,   0x1c,   0x1d,   0x1e,   0x1f,   0x20,   0x21,   0x22,   0x23,
    0x24,   0x25,   0x26,   0x27,   0x28,   0x29,   0x2a,   0x2b,   0x2c,
    0x2d,   0x2e,   0x2f,   0x30,   0x31,   0x32,   0x33,   0x34,   0x35,
    0x36,   0x37,   0x38,   0x39,   0x3a,   0x3b,   0x3c,   0x3d,   0x3e,
    0x3f,   0x40,   0x41,   0x42,   0x43,   0x44,   0x45,   0x46,   0x47,
    0x48,   0x49,   0x4a,   0x4b,   0x4c,   0x4d,   0x4e,   0x4f,   0x50,
    0x51,   0x52,   0x53,   0x54,   0x55,   0x56,   0x57,   0x58,   0x59,
    0x5a,   0x5b,   0x5c,   0x5d,   0x5e,   0x5f,   0x60,   0x61,   0x62,
    0x63,   0x64,   0x65,   0x66,   0x67,   0x68,   0x69,   0x6a,   0x6b,
    0x6c,   0x6d,   0x6e,   0x6f,   0x70,   0x71,   0x72,   0x73,   0x74,
    0x75,   0x76,   0x77,   0x78,   0x79,   0x7a,   0x7b,   0x7c,   0x7d,
    0x7e,   0x7f,   0x80,   0x81,   0x82,   0x83,   0x84,   0x85,   0x86,
    0x87,   0x88,   0x89,   0x8a,   0x8b,   0x8c,   0x8d,   0x8e,   0x8f,
    0x90,   0x91,   0x92,   0x93,   0x94,   0x95,   0x96,   0x97,   0x98,
    0x99,   0x9a,   0x9b,   0x9c,   0x9d,   0x9e,   0x9f,   0x00a0, 0x00a1,
    0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7, 0x00a8, 0x00a9, 0x00aa,
    0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af, 0x00b0, 0x00b1, 0x00b2, 0x00b3,
    0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc,
    0x00bd, 0x00be, 0x00bf, 0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5,
    0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce,
    0x00cf, 0x011e, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
    0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x0130, 0x015e, 0x00df, 0x00e0,
    0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7, 0x00e8, 0x00e9,
    0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef, 0x011f, 0x00f1, 0x00f2,
    0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb,
    0x00fc, 0x0131, 0x015f, 0x00ff,
};

long tab8859_10[256] = /* from dkuug.dk:i18n/charmaps/ISO_8859-10:1993 */
    {
     0x00,   0x01,   0x02,   0x03,   0x04,   0x05,   0x06,   0x07,   0x08,
     0x09,   0x0a,   0x0b,   0x0c,   0x0d,   0x0e,   0x0f,   0x10,   0x11,
     0x12,   0x13,   0x14,   0x15,   0x16,   0x17,   0x18,   0x19,   0x1a,
     0x1b,   0x1c,   0x1d,   0x1e,   0x1f,   0x20,   0x21,   0x22,   0x23,
     0x24,   0x25,   0x26,   0x27,   0x28,   0x29,   0x2a,   0x2b,   0x2c,
     0x2d,   0x2e,   0x2f,   0x30,   0x31,   0x32,   0x33,   0x34,   0x35,
     0x36,   0x37,   0x38,   0x39,   0x3a,   0x3b,   0x3c,   0x3d,   0x3e,
     0x3f,   0x40,   0x41,   0x42,   0x43,   0x44,   0x45,   0x46,   0x47,
     0x48,   0x49,   0x4a,   0x4b,   0x4c,   0x4d,   0x4e,   0x4f,   0x50,
     0x51,   0x52,   0x53,   0x54,   0x55,   0x56,   0x57,   0x58,   0x59,
     0x5a,   0x5b,   0x5c,   0x5d,   0x5e,   0x5f,   0x60,   0x61,   0x62,
     0x63,   0x64,   0x65,   0x66,   0x67,   0x68,   0x69,   0x6a,   0x6b,
     0x6c,   0x6d,   0x6e,   0x6f,   0x70,   0x71,   0x72,   0x73,   0x74,
     0x75,   0x76,   0x77,   0x78,   0x79,   0x7a,   0x7b,   0x7c,   0x7d,
     0x7e,   0x7f,   0x80,   0x81,   0x82,   0x83,   0x84,   0x85,   0x86,
     0x87,   0x88,   0x89,   0x8a,   0x8b,   0x8c,   0x8d,   0x8e,   0x8f,
     0x90,   0x91,   0x92,   0x93,   0x94,   0x95,   0x96,   0x97,   0x98,
     0x99,   0x9a,   0x9b,   0x9c,   0x9d,   0x9e,   0x9f,   0x00a0, 0x0104,
     0x0112, 0x0122, 0x012a, 0x0128, 0x0136, 0x00a7, 0x013b, 0x0110, 0x0160,
     0x0166, 0x017d, 0x00ad, 0x016a, 0x014a, 0x00b0, 0x0105, 0x0113, 0x0123,
     0x012b, 0x0129, 0x0137, 0x00b7, 0x013c, 0x0110, 0x0161, 0x0167, 0x017e,
     0x2014, 0x016b, 0x014b, 0x0100, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5,
     0x00c6, 0x012e, 0x010c, 0x00c9, 0x0118, 0x00cb, 0x0116, 0x00cd, 0x00ce,
     0x00cf, 0x00d0, 0x0145, 0x014c, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x0168,
     0x00d8, 0x0172, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df, 0x0101,
     0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x012f, 0x010d, 0x00e9,
     0x0119, 0x00eb, 0x0117, 0x00ed, 0x00ee, 0x00ef, 0x00f0, 0x0146, 0x014d,
     0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x0169, 0x00f8, 0x0173, 0x00fa, 0x00fb,
     0x00fc, 0x00fd, 0x00fe, 0x0138,
};

long tab8859_15[256] = /* from anyrhine@cs.helsinki.fi (Aki Nyrhinen) */
    {0x00,   0x01, 0x02, 0x03, 0x04,   0x05, 0x06,   0x07, 0x08,   0x09,
     0x0a,   0x0b, 0x0c, 0x0d, 0x0e,   0x0f, 0x10,   0x11, 0x12,   0x13,
     0x14,   0x15, 0x16, 0x17, 0x18,   0x19, 0x1a,   0x1b, 0x1c,   0x1d,
     0x1e,   0x1f, 0x20, 0x21, 0x22,   0x23, 0x24,   0x25, 0x26,   0x27,
     0x28,   0x29, 0x2a, 0x2b, 0x2c,   0x2d, 0x2e,   0x2f, 0x30,   0x31,
     0x32,   0x33, 0x34, 0x35, 0x36,   0x37, 0x38,   0x39, 0x3a,   0x3b,
     0x3c,   0x3d, 0x3e, 0x3f, 0x40,   0x41, 0x42,   0x43, 0x44,   0x45,
     0x46,   0x47, 0x48, 0x49, 0x4a,   0x4b, 0x4c,   0x4d, 0x4e,   0x4f,
     0x50,   0x51, 0x52, 0x53, 0x54,   0x55, 0x56,   0x57, 0x58,   0x59,
     0x5a,   0x5b, 0x5c, 0x5d, 0x5e,   0x5f, 0x60,   0x61, 0x62,   0x63,
     0x64,   0x65, 0x66, 0x67, 0x68,   0x69, 0x6a,   0x6b, 0x6c,   0x6d,
     0x6e,   0x6f, 0x70, 0x71, 0x72,   0x73, 0x74,   0x75, 0x76,   0x77,
     0x78,   0x79, 0x7a, 0x7b, 0x7c,   0x7d, 0x7e,   0x7f, 0x80,   0x81,
     0x82,   0x83, 0x84, 0x85, 0x86,   0x87, 0x88,   0x89, 0x8a,   0x8b,
     0x8c,   0x8d, 0x8e, 0x8f, 0x90,   0x91, 0x92,   0x93, 0x94,   0x95,
     0x96,   0x97, 0x98, 0x99, 0x9a,   0x9b, 0x9c,   0x9d, 0x9e,   0x9f,
     0xa0,   0xa1, 0xa2, 0xa3, 0x20ac, 0xa5, 0x0160, 0xa7, 0x0161, 0xa9,
     0xaa,   0xab, 0xac, 0xad, 0xae,   0xaf, 0xb0,   0xb1, 0xb2,   0xb3,
     0x017d, 0xb5, 0xb6, 0xb7, 0x017e, 0xb9, 0xba,   0xbb, 0x0152, 0x0153,
     0x0178, 0xbf, 0xc0, 0xc1, 0xc2,   0xc3, 0xc4,   0xc5, 0xc6,   0xc7,
     0xc8,   0xc9, 0xca, 0xcb, 0xcc,   0xcd, 0xce,   0xcf, 0xd0,   0xd1,
     0xd2,   0xd3, 0xd4, 0xd5, 0xd6,   0xd7, 0xd8,   0xd9, 0xda,   0xdb,
     0xdc,   0xdd, 0xde, 0xdf, 0xe0,   0xe1, 0xe2,   0xe3, 0xe4,   0xe5,
     0xe6,   0xe7, 0xe8, 0xe9, 0xea,   0xeb, 0xec,   0xed, 0xee,   0xef,
     0xf0,   0xf1, 0xf2, 0xf3, 0xf4,   0xf5, 0xf6,   0xf7, 0xf8,   0xf9,
     0xfa,   0xfb, 0xfc, 0xfd, 0xfe,   0xff};
