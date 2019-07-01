#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct mxfKey_t {
    uint8_t byte0;
    uint8_t byte1;
    uint8_t byte2;
    uint8_t byte3;
    uint8_t byte4;
    uint8_t byte5;
    uint8_t byte6;
    uint8_t byte7;
    uint8_t byte8;
    uint8_t byte9;
    uint8_t byte10;
    uint8_t byte11;
    uint8_t byte12;
    uint8_t byte13;
    uint8_t byte14;
    uint8_t byte15;
} mxfKey;

const mxfKey MXF_CC_KEY = { 0x06, 0x0E, 0x2B, 0x34,
                            0x01, 0x02, 0x01, 0x01,
                            0x0D, 0x01, 0x03, 0x01,
                            0x17, 0x01, 0x02, 0x01 };

const mxfKey MXF_HEADER_KEY = { 0x06, 0x0e, 0x2b, 0x34,
                                0x02, 0x05, 0x01, 0x01,
                                0x0d, 0x01, 0x02, 0x01,
                                0x01, 0x02, 0x04, 0x00 };

bool mxfKeyEqual(const mxfKey &k1, const mxfKey &k2)
{
    return !memcmp(&k1, &k2, sizeof(mxfKey));
}

int readMxfKey(mxfKey& k, FILE *fp)
{
    int count = (int) fread(&k, 1, sizeof(mxfKey), fp);

    if (count != sizeof(mxfKey))
        return 0;

    return count;
}

int readBERLength(uint64_t &retLength, FILE *fp)
{
    int bytesRead = 0;
    uint8_t b = 0;
    uint8_t lengthLen = 0;

    int size = sizeof(b);

    if(sizeof(b) != fread(&b, 1, sizeof(b), fp))
    {
        retLength = 0;
        return 0;
    }

    bytesRead++;

    if (b == 0x80) {
        // unknown length
        retLength = 0;
        lengthLen = 0;
    }
    else if ((b & 0x80) != 0x80) {
        // short form
        retLength = b;
        lengthLen = 1;
    }
    else {
        // long form
        int length = b & 0x7f;

        retLength = 0;

        for (int k = 0; k < length; k++) {
            if(sizeof(b) != fread(&b, 1, sizeof(b), fp))
            {
                retLength = 0;
                return 0;
            }

            bytesRead++;
            retLength = retLength << 8;
            retLength = retLength + b;
        }

        lengthLen = length + 1;
    }

    return bytesRead;
}

int main(int argc, char **argv)
{
    if(argc < 2)
        return 0;

    FILE *fp = fopen(argv[1], "r");

    if(NULL == fp)
        return 1;
    
    mxfKey key;
    uint64_t filePos = 0;

    // Look for MXF header first    
    filePos += readMxfKey(key, fp);

    if (!mxfKeyEqual(key, MXF_HEADER_KEY))
    {
        fprintf(stderr, "Invalid MXF File\n");
        fclose(fp);

        return 1;
    }

    // Skip the header key
    uint64_t klvLength = 0;
    filePos += readBERLength(klvLength, fp);
    filePos += klvLength;

    fseek(fp, filePos, 0);

    FILE *fpOut = fopen("c:\\cc_out.608", "wb");

    // Find CC keys, write out CC data to output file
    while(16 == readMxfKey(key, fp))
    {
        filePos += 16;
        filePos += readBERLength(klvLength, fp);
        filePos += klvLength;

        if (mxfKeyEqual(key, MXF_CC_KEY))
        {
            uint8_t *p = (uint8_t *) malloc(klvLength);
            fread(p, 1, klvLength, fp);
            fwrite(p, 1, klvLength, fpOut);
            free(p);
        }
        else
            fseek(fp, filePos, 0);
    };

    fclose(fpOut);
    fclose(fp);

    return 0;
}