#include "dcp.h"

byte DCP_genCmndBCH(byte buff[], int count)
{ // genreate a BCH for this block of bytes
    byte i, j, bch, nBCHpoly, fBCHpoly;

    nBCHpoly = 0xB8;
    fBCHpoly = 0x7F;
    for (bch = 0, i = 0; i < count; i++){
        bch ^= buff[i]; // XOR w/byte xmtd
        for (j = 0; j < 8; j++){
            if ((bch & 1) == 1)
                bch = (bch >> 1) ^ nBCHpoly; // shI and XOR w/BCH poly
            else bch >>= 1;
        }
    }
    bch ^= fBCHpoly;
    return(bch);
}