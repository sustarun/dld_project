/* 
 * Copyright (C) 2012-2014 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <bit.h>
#include <stdbool.h>
#include <read_csv.h>
#include <write_csv.h>
#include <encryptor.h>
#include <decryptor.h>
#include <bit_hex.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <makestuff.h>
#include <libfpgalink.h>
#include <libbuffer.h>
#include <liberror.h>
#include <libdump.h>
#include <argtable2.h>
#include <readline/readline.h>
#include <readline/history.h>
#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif

bool sigIsRaised(void);
void sigRegisterHandler(void);
char str[3];


static const char *ptr;
static bool enableBenchmarking = false;


char key[33] = "11100000000000000000001100000000";
// char key[33] = "10101010101010101010101010101010";
char Ack1[33] = "10110111101101111011011110110111";
char Ack2[33] = "11010010110100101101001011010010";
char Ack3[33] = "11011000100100111010011100100101";
char reset_str[33] = "10101110101010101010101001101100";

void delay(int number_of_seconds)
{
    // Converting time into milli_seconds
    int milli_seconds = 1000 * number_of_seconds;
 
    // Stroing start time
    clock_t start_time = clock();
 
    // looping till required time is not acheived
    while (clock() < start_time + milli_seconds)
        ;
}


static bool isHexDigit(char ch) {
	return
		(ch >= '0' && ch <= '9') ||
		(ch >= 'a' && ch <= 'f') ||
		(ch >= 'A' && ch <= 'F');
}

static uint16 calcChecksum(const uint8 *data, size_t length) {
	uint16 cksum = 0x0000;
	while ( length-- ) {
		cksum = (uint16)(cksum + *data++);
	}
	return cksum;
}

static bool getHexNibble(char hexDigit, uint8 *nibble) {
	if ( hexDigit >= '0' && hexDigit <= '9' ) {
		*nibble = (uint8)(hexDigit - '0');
		return false;
	} else if ( hexDigit >= 'a' && hexDigit <= 'f' ) {
		*nibble = (uint8)(hexDigit - 'a' + 10);
		return false;
	} else if ( hexDigit >= 'A' && hexDigit <= 'F' ) {
		*nibble = (uint8)(hexDigit - 'A' + 10);
		return false;
	} else {
		return true;
	}
}

static int getHexByte(uint8 *byte) {
	uint8 upperNibble;
	uint8 lowerNibble;
	if ( !getHexNibble(ptr[0], &upperNibble) && !getHexNibble(ptr[1], &lowerNibble) ) {
		*byte = (uint8)((upperNibble << 4) | lowerNibble);
		byte += 2;
		return 0;
	} else {
		return 1;
	}
}

static const char *const errMessages[] = {
	NULL,
	NULL,
	"Unparseable hex number",
	"Channel out of range",
	"Conduit out of range",
	"Illegal character",
	"Unterminated string",
	"No memory",
	"Empty string",
	"Odd number of digits",
	"Cannot load file",
	"Cannot save file",
	"Bad arguments"
};

typedef enum {
	FLP_SUCCESS,
	FLP_LIBERR,
	FLP_BAD_HEX,
	FLP_CHAN_RANGE,
	FLP_CONDUIT_RANGE,
	FLP_ILL_CHAR,
	FLP_UNTERM_STRING,
	FLP_NO_MEMORY,
	FLP_EMPTY_STRING,
	FLP_ODD_DIGITS,
	FLP_CANNOT_LOAD,
	FLP_CANNOT_SAVE,
	FLP_ARGS
} ReturnCode;

static ReturnCode doRead(
	struct FLContext *handle, uint8 chan, uint32 length, FILE *destFile, uint16 *checksum,
	const char **error)
{
	ReturnCode retVal = FLP_SUCCESS;
	uint32 bytesWritten;
	FLStatus fStatus;
	uint32 chunkSize;
	const uint8 *recvData;
	uint32 actualLength;
	const uint8 *ptr;
	uint16 csVal = 0x0000;
	#define READ_MAX 65536

	// Read first chunk
	chunkSize = length >= READ_MAX ? READ_MAX : length;
	fStatus = flReadChannelAsyncSubmit(handle, chan, chunkSize, NULL, error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");
	length = length - chunkSize;

	while ( length ) {
		// Read chunk N
		chunkSize = length >= READ_MAX ? READ_MAX : length;
		fStatus = flReadChannelAsyncSubmit(handle, chan, chunkSize, NULL, error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");
		length = length - chunkSize;
		
		// Await chunk N-1
		fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");

		// Write chunk N-1 to file
		bytesWritten = (uint32)fwrite(recvData, 1, actualLength, destFile);
		CHECK_STATUS(bytesWritten != actualLength, FLP_CANNOT_SAVE, cleanup, "doRead()");

		// Checksum chunk N-1
		chunkSize = actualLength;
		ptr = recvData;
		while ( chunkSize-- ) {
			csVal = (uint16)(csVal + *ptr++);
		}
	}

	// Await last chunk
	fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");
	
	// Write last chunk to file
	bytesWritten = (uint32)fwrite(recvData, 1, actualLength, destFile);
	CHECK_STATUS(bytesWritten != actualLength, FLP_CANNOT_SAVE, cleanup, "doRead()");

	// Checksum last chunk
	chunkSize = actualLength;
	ptr = recvData;
	while ( chunkSize-- ) {
		csVal = (uint16)(csVal + *ptr++);
	}
	
	// Return checksum to caller
	*checksum = csVal;
cleanup:
	return retVal;
}

static ReturnCode doWrite(
	struct FLContext *handle, uint8 chan, FILE *srcFile, size_t *length, uint16 *checksum,
	const char **error)
{
	ReturnCode retVal = FLP_SUCCESS;
	size_t bytesRead, i;
	FLStatus fStatus;
	const uint8 *ptr;
	uint16 csVal = 0x0000;
	size_t lenVal = 0;
	#define WRITE_MAX (65536 - 5)
	uint8 buffer[WRITE_MAX];

	do {
		// Read Nth chunk
		bytesRead = fread(buffer, 1, WRITE_MAX, srcFile);
		if ( bytesRead ) {
			// Update running total
			lenVal = lenVal + bytesRead;

			// Submit Nth chunk
			fStatus = flWriteChannelAsync(handle, chan, bytesRead, buffer, error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doWrite()");

			// Checksum Nth chunk
			i = bytesRead;
			ptr = buffer;
			while ( i-- ) {
				csVal = (uint16)(csVal + *ptr++);
			}
		}
	} while ( bytesRead == WRITE_MAX );

	// Wait for writes to be received. This is optional, but it's only fair if we're benchmarking to
	// actually wait for the work to be completed.
	fStatus = flAwaitAsyncWrites(handle, error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doWrite()");

	// Return checksum & length to caller
	*checksum = csVal;
	*length = lenVal;
cleanup:
	return retVal;
}

static int parseLine(struct FLContext *handle, const char *line, const char **error) {
	ReturnCode retVal = FLP_SUCCESS, status;
	FLStatus fStatus;
	struct Buffer dataFromFPGA = {0,};
	BufferStatus bStatus;
	uint8 *data = NULL;
	char *fileName = NULL;
	FILE *file = NULL;
	double totalTime, speed;
	#ifdef WIN32
		LARGE_INTEGER tvStart, tvEnd, freq;
		DWORD_PTR mask = 1;
		SetThreadAffinityMask(GetCurrentThread(), mask);
		QueryPerformanceFrequency(&freq);
	#else
		struct timeval tvStart, tvEnd;
		long long startTime, endTime;
	#endif
	bStatus = bufInitialise(&dataFromFPGA, 1024, 0x00, error);
	CHECK_STATUS(bStatus, FLP_LIBERR, cleanup);
	ptr = line;
	do {
		while ( *ptr == ';' ) {
			ptr++;
		}
		switch ( *ptr ) {
		case 'r':{
			uint32 chan;
			uint32 length = 1;
			char *end;
			ptr++;
			
			// Get the channel to be read:
			errno = 0;
			chan = (uint32)strtoul(ptr, &end, 16);
			CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);

			// Ensure that it's 0-127
			CHECK_STATUS(chan > 127, FLP_CHAN_RANGE, cleanup);
			ptr = end;

			// Only three valid chars at this point:
			CHECK_STATUS(*ptr != '\0' && *ptr != ';' && *ptr != ' ', FLP_ILL_CHAR, cleanup);

			if ( *ptr == ' ' ) {
				ptr++;

				// Get the read count:
				errno = 0;
				length = (uint32)strtoul(ptr, &end, 16);
				CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);
				ptr = end;
				
				// Only three valid chars at this point:
				CHECK_STATUS(*ptr != '\0' && *ptr != ';' && *ptr != ' ', FLP_ILL_CHAR, cleanup);
				if ( *ptr == ' ' ) {
					const char *p;
					const char quoteChar = *++ptr;
					CHECK_STATUS(
						(quoteChar != '"' && quoteChar != '\''),
						FLP_ILL_CHAR, cleanup);
					
					// Get the file to write bytes to:
					ptr++;
					p = ptr;
					while ( *p != quoteChar && *p != '\0' ) {
						p++;
					}
					CHECK_STATUS(*p == '\0', FLP_UNTERM_STRING, cleanup);
					fileName = malloc((size_t)(p - ptr + 1));
					CHECK_STATUS(!fileName, FLP_NO_MEMORY, cleanup);
					CHECK_STATUS(p - ptr == 0, FLP_EMPTY_STRING, cleanup);
					strncpy(fileName, ptr, (size_t)(p - ptr));
					fileName[p - ptr] = '\0';
					ptr = p + 1;
				}
			}
			if ( fileName ) {
				uint16 checksum = 0x0000;

				// Open file for writing
				file = fopen(fileName, "wb");
				CHECK_STATUS(!file, FLP_CANNOT_SAVE, cleanup);
				free(fileName);
				fileName = NULL;

				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					status = doRead(handle, (uint8)chan, length, file, &checksum, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);
					status = doRead(handle, (uint8)chan, length, file, &checksum, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
				if ( enableBenchmarking ) {
					printf(
						"Read %d bytes (checksum 0x%04X) from channel %d at %f MiB/s\n",
						length, checksum, chan, speed);
				}
				CHECK_STATUS(status, status, cleanup);

				// Close the file
				fclose(file);
				file = NULL;
			} else {
				size_t oldLength = dataFromFPGA.length;
				bStatus = bufAppendConst(&dataFromFPGA, 0x00, length, error);
				CHECK_STATUS(bStatus, FLP_LIBERR, cleanup);
				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					fStatus = flReadChannel(handle, (uint8)chan, length, dataFromFPGA.data + oldLength, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);
					fStatus = flReadChannel(handle, (uint8)chan, length, dataFromFPGA.data + oldLength, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
				if ( enableBenchmarking ) {
					printf(
						"Read %d bytes (checksum 0x%04X) from channel %d at %f MiB/s\n",
						length, calcChecksum(dataFromFPGA.data + oldLength, length), chan, speed);
					sprintf(str, "%02X", *dataFromFPGA.data);
				}
				CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			}
			break;
		}
		case 'w':{
			unsigned long int chan;
			size_t length = 1, i;
			char *end, ch;
			const char *p;
			ptr++;
			
			// Get the channel to be written:
			errno = 0;
			chan = strtoul(ptr, &end, 16);
			CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);

			// Ensure that it's 0-127
			CHECK_STATUS(chan > 127, FLP_CHAN_RANGE, cleanup);
			ptr = end;

			// There must be a space now:
			CHECK_STATUS(*ptr != ' ', FLP_ILL_CHAR, cleanup);

			// Now either a quote or a hex digit
		   ch = *++ptr;
			if ( ch == '"' || ch == '\'' ) {
				uint16 checksum = 0x0000;

				// Get the file to read bytes from:
				ptr++;
				p = ptr;
				while ( *p != ch && *p != '\0' ) {
					p++;
				}
				CHECK_STATUS(*p == '\0', FLP_UNTERM_STRING, cleanup);
				fileName = malloc((size_t)(p - ptr + 1));
				CHECK_STATUS(!fileName, FLP_NO_MEMORY, cleanup);
				CHECK_STATUS(p - ptr == 0, FLP_EMPTY_STRING, cleanup);
				strncpy(fileName, ptr, (size_t)(p - ptr));
				fileName[p - ptr] = '\0';
				ptr = p + 1;  // skip over closing quote

				// Open file for reading
				file = fopen(fileName, "rb");
				CHECK_STATUS(!file, FLP_CANNOT_LOAD, cleanup);
				free(fileName);
				fileName = NULL;
				
				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					status = doWrite(handle, (uint8)chan, file, &length, &checksum, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);
					status = doWrite(handle, (uint8)chan, file, &length, &checksum, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
				if ( enableBenchmarking ) {
					// printf(
					// 	"Wrote "PFSZD" bytes (checksum 0x%04X) to channel %lu at %f MiB/s\n",
					// 	length, checksum, chan, speed);
				}
				CHECK_STATUS(status, status, cleanup);

				// Close the file
				fclose(file);
				file = NULL;
			} else if ( isHexDigit(ch) ) {
				// Read a sequence of hex bytes to write
				uint8 *dataPtr;
				p = ptr + 1;
				while ( isHexDigit(*p) ) {
					p++;
				}
				CHECK_STATUS((p - ptr) & 1, FLP_ODD_DIGITS, cleanup);
				length = (size_t)(p - ptr) / 2;
				data = malloc(length);
				dataPtr = data;
				for ( i = 0; i < length; i++ ) {
					getHexByte(dataPtr++);
					ptr += 2;
				}
				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					fStatus = flWriteChannel(handle, (uint8)chan, length, data, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);
					fStatus = flWriteChannel(handle, (uint8)chan, length, data, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
				if ( enableBenchmarking ) {
					printf(
						"Wrote "PFSZD" bytes (checksum 0x%04X) to channel %lu at %f MiB/s\n",
						length, calcChecksum(data, length), chan, speed);
				}
				CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
				free(data);
				data = NULL;
			} else {
				FAIL(FLP_ILL_CHAR, cleanup);
			}
			break;
		}
		case '+':{
			uint32 conduit;
			char *end;
			ptr++;

			// Get the conduit
			errno = 0;
			conduit = (uint32)strtoul(ptr, &end, 16);
			CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);

			// Ensure that it's 0-127
			CHECK_STATUS(conduit > 255, FLP_CONDUIT_RANGE, cleanup);
			ptr = end;

			// Only two valid chars at this point:
			CHECK_STATUS(*ptr != '\0' && *ptr != ';', FLP_ILL_CHAR, cleanup);

			fStatus = flSelectConduit(handle, (uint8)conduit, error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			break;
		}
		default:
			FAIL(FLP_ILL_CHAR, cleanup);
		}
	} while ( *ptr == ';' );
	CHECK_STATUS(*ptr != '\0', FLP_ILL_CHAR, cleanup);

	dump(0x00000000, dataFromFPGA.data, dataFromFPGA.length);

cleanup:
	bufDestroy(&dataFromFPGA);
	if ( file ) {
		fclose(file);
	}
	free(fileName);
	free(data);
	if ( retVal > FLP_LIBERR ) {
		const int column = (int)(ptr - line);
		int i;
		fprintf(stderr, "%s at column %d\n  %s\n  ", errMessages[retVal], column, line);
		for ( i = 0; i < column; i++ ) {
			fprintf(stderr, " ");
		}
		fprintf(stderr, "^\n");
	}
	return retVal;
}


static const char *nibbles[] = {
	"0000",  // '0'
	"0001",  // '1'
	"0010",  // '2'
	"0011",  // '3'
	"0100",  // '4'
	"0101",  // '5'
	"0110",  // '6'
	"0111",  // '7'
	"1000",  // '8'
	"1001",  // '9'

	"XXXX",  // ':'
	"XXXX",  // ';'
	"XXXX",  // '<'
	"XXXX",  // '='
	"XXXX",  // '>'
	"XXXX",  // '?'
	"XXXX",  // '@'

	"1010",  // 'A'
	"1011",  // 'B'
	"1100",  // 'C'
	"1101",  // 'D'
	"1110",  // 'E'
	"1111"   // 'F'
};

// char * read_fn(int ch_num){
// 	struct FLContext *handle = NULL;
// 	ReturnCode retVal = FLP_SUCCESS, pStatus;
// 	const char *line = NULL;
// 	char hexdata[9]="";
// 	for(int j=0; j<4; j++){
// 		char read_ch_num[4];
// 		sprintf(read_ch_num, "%d", ch_num);
// 		char line_temp[7]="r";
// 		strcat(line_temp, read_ch_num);
// 		strcat(line_temp, " 1");
// 		printf("line_temp is %s\n", line_temp);
// 		line = line_temp;
// 		printf("line read is %s\n", line);
// 		pStatus = parseLine(handle, line, &error);
// 		CHECK_STATUS(pStatus, pStatus, cleanup);
// 		printf("read_hex is %s\n", str);
// 		strcat(hexdata, str);
// 		printf("curr_hexdata is %s\n", hexdata);
// 	}
// 	printf("hexdata is %s\n", hexdata);
// 	printf("data in bits %s\n", hex_to_bit(hexdata));
// 	char enc_bits[33];
// 	sprintf(enc_bits, "%s", hex_to_bit(hexdata));
// 	printf("enc_bits is %s\n", enc_bits);
// 	char * dec_bits=malloc(33);
// 	memset(dec_bits, 0, 33);
// 	printf("function output is %s\n", decryptor(enc_bits, key));
// 	sprintf(dec_bits, "%s", decryptor(enc_bits, key));
// 	printf("dec_bits is %s\n", dec_bits);
// 	return dec_bits;
// }

// void write_fn(int ch_num, char * dec_data){
// 	struct FLContext *handle = NULL;
// 	ReturnCode retVal = FLP_SUCCESS, pStatus;
// 	const char *line = NULL;
// 	char enc_data[33];
// 	sprintf(enc_data, "%s", encryptor(dec_data, key));
// 	printf("first4data_enc is %s\n", enc_data);
// 	char hex_data_enc[9];
// 	sprintf(hex_data_enc, "%s", bit_to_hex(enc_data));
// 	char write_ch_num[4];
// 	sprintf(write_ch_num,"%d",ch_num);
// 	char line_temp[14]"w";
// 	strcat(line_temp, write_ch_num);
// 	strcat(line_temp, " ");
// 	strcat(line_temp, hex_data_enc);
// 	line = line_temp;
// 	pStatus = parseLine(handle, line, &error);
// 	CHECK_STATUS(pStatus, pStatus, cleanup);
// }

int main(int argc, char *argv[]) {
	ReturnCode retVal = FLP_SUCCESS, pStatus;
	struct arg_str *ivpOpt = arg_str0("i", "ivp", "<VID:PID>", "            vendor ID and product ID (e.g 04B4:8613)");
	struct arg_str *vpOpt = arg_str1("v", "vp", "<VID:PID[:DID]>", "       VID, PID and opt. dev ID (e.g 1D50:602B:0001)");
	struct arg_str *fwOpt = arg_str0("f", "fw", "<firmware.hex>", "        firmware to RAM-load (or use std fw)");
	struct arg_str *portOpt = arg_str0("d", "ports", "<bitCfg[,bitCfg]*>", " read/write digital ports (e.g B13+,C1-,B2?)");
	struct arg_str *queryOpt = arg_str0("q", "query", "<jtagBits>", "         query the JTAG chain");
	struct arg_str *progOpt = arg_str0("p", "program", "<config>", "         program a device");
	struct arg_uint *conOpt = arg_uint0("c", "conduit", "<conduit>", "        which comm conduit to choose (default 0x01)");
	struct arg_str *actOpt = arg_str0("a", "action", "<actionString>", "    a series of CommFPGA actions");
	struct arg_lit *shellOpt  = arg_lit0("s", "shell", "                    start up an interactive CommFPGA session");
	struct arg_lit *benOpt  = arg_lit0("b", "benchmark", "                enable benchmarking & checksumming");
	struct arg_lit *rstOpt  = arg_lit0("r", "reset", "                    reset the bulk endpoints");
	struct arg_str *dumpOpt = arg_str0("l", "dumploop", "<ch:file.bin>", "   write data from channel ch to file");
	struct arg_lit *helpOpt  = arg_lit0("h", "help", "                     print this help and exit");
	struct arg_str *eepromOpt  = arg_str0(NULL, "eeprom", "<std|fw.hex|fw.iic>", "   write firmware to FX2's EEPROM (!!)");
	struct arg_str *backupOpt  = arg_str0(NULL, "backup", "<kbitSize:fw.iic>", "     backup FX2's EEPROM (e.g 128:fw.iic)\n");
	struct arg_lit *readOpt  = arg_lit0("e", "read", "                    reading channel zero and writing channel one");
	struct arg_end *endOpt   = arg_end(20);
	void *argTable[] = {
		ivpOpt, vpOpt, fwOpt, portOpt, queryOpt, progOpt, conOpt, actOpt,
		shellOpt, benOpt, rstOpt, dumpOpt, helpOpt, eepromOpt, backupOpt, readOpt, endOpt
	};
	const char *progName = "flcli";
	int numErrors;
	struct FLContext *handle = NULL;
	FLStatus fStatus;
	const char *error = NULL;
	const char *ivp = NULL;
	const char *vp = NULL;
	bool isNeroCapable, isCommCapable;
	uint32 numDevices, scanChain[16], i;
	const char *line = NULL;
	uint8 conduit = 0x01;

	if ( arg_nullcheck(argTable) != 0 ) {
		fprintf(stderr, "%s: insufficient memory\n", progName);
		FAIL(1, cleanup);
	}

	numErrors = arg_parse(argc, argv, argTable);

	if ( helpOpt->count > 0 ) {
		printf("FPGALink Command-Line Interface Copyright (C) 2012-2014 Chris McClelland\n\nUsage: %s", progName);
		arg_print_syntax(stdout, argTable, "\n");
		printf("\nInteract with an FPGALink device.\n\n");
		arg_print_glossary(stdout, argTable,"  %-10s %s\n");
		FAIL(FLP_SUCCESS, cleanup);
	}

	if ( numErrors > 0 ) {
		arg_print_errors(stdout, endOpt, progName);
		fprintf(stderr, "Try '%s --help' for more information.\n", progName);
		FAIL(FLP_ARGS, cleanup);
	}

	fStatus = flInitialise(0, &error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);

	vp = vpOpt->sval[0];
	printf("Attempting to open connection to FPGALink device %s...\n", vp);
	fStatus = flOpen(vp, &handle, NULL);
	if ( fStatus ) {
		if ( ivpOpt->count ) {
			int count = 60;
			uint8 flag;
			ivp = ivpOpt->sval[0];
			printf("Loading firmware into %s...\n", ivp);
			if ( fwOpt->count ) {
				fStatus = flLoadCustomFirmware(ivp, fwOpt->sval[0], &error);
			} else {
				fStatus = flLoadStandardFirmware(ivp, vp, &error);
			}
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			
			printf("Awaiting renumeration");
			flSleep(1000);
			do {
				printf(".");
				fflush(stdout);
				fStatus = flIsDeviceAvailable(vp, &flag, &error);
				CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
				flSleep(250);
				count--;
			} while ( !flag && count );
			printf("\n");
			if ( !flag ) {
				fprintf(stderr, "FPGALink device did not renumerate properly as %s\n", vp);
				FAIL(FLP_LIBERR, cleanup);
			}

			printf("Attempting to open connection to FPGLink device %s again...\n", vp);
			fStatus = flOpen(vp, &handle, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		} else {
			fprintf(stderr, "Could not open FPGALink device at %s and no initial VID:PID was supplied\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	printf(
		"Connected to FPGALink device %s (firmwareID: 0x%04X, firmwareVersion: 0x%08X)\n",
		vp, flGetFirmwareID(handle), flGetFirmwareVersion(handle)
	);

	if ( eepromOpt->count ) {
		if ( !strcmp("std", eepromOpt->sval[0]) ) {
			printf("Writing the standard FPGALink firmware to the FX2's EEPROM...\n");
			fStatus = flFlashStandardFirmware(handle, vp, &error);
		} else {
			printf("Writing custom FPGALink firmware from %s to the FX2's EEPROM...\n", eepromOpt->sval[0]);
			fStatus = flFlashCustomFirmware(handle, eepromOpt->sval[0], &error);
		}
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
	}

	if ( backupOpt->count ) {
		const char *fileName;
		const uint32 kbitSize = strtoul(backupOpt->sval[0], (char**)&fileName, 0);
		if ( *fileName != ':' ) {
			fprintf(stderr, "%s: invalid argument to option --backup=<kbitSize:fw.iic>\n", progName);
			FAIL(FLP_ARGS, cleanup);
		}
		fileName++;
		printf("Saving a backup of %d kbit from the FX2's EEPROM to %s...\n", kbitSize, fileName);
		fStatus = flSaveFirmware(handle, kbitSize, fileName, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
	}

	if ( rstOpt->count ) {
		// Reset the bulk endpoints (only needed in some virtualised environments)
		fStatus = flResetToggle(handle, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
	}

	if ( conOpt->count ) {
		conduit = (uint8)conOpt->ival[0];
	}

	isNeroCapable = flIsNeroCapable(handle);
	isCommCapable = flIsCommCapable(handle, conduit);

	if ( portOpt->count ) {
		uint32 readState;
		char hex[9];
		const uint8 *p = (const uint8 *)hex;
		printf("Configuring ports...\n");
		fStatus = flMultiBitPortAccess(handle, portOpt->sval[0], &readState, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		sprintf(hex, "%08X", readState);
		printf("Readback:   28   24   20   16    12    8    4    0\n          %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s\n", nibbles[*p++ - '0']);
		flSleep(100);
	}

	if ( queryOpt->count ) {
		if ( isNeroCapable ) {
			fStatus = flSelectConduit(handle, 0x00, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = jtagScanChain(handle, queryOpt->sval[0], &numDevices, scanChain, 16, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			if ( numDevices ) {
				printf("The FPGALink device at %s scanned its JTAG chain, yielding:\n", vp);
				for ( i = 0; i < numDevices; i++ ) {
					printf("  0x%08X\n", scanChain[i]);
				}
			} else {
				printf("The FPGALink device at %s scanned its JTAG chain but did not find any attached devices\n", vp);
			}
		} else {
			fprintf(stderr, "JTAG chain scan requested but FPGALink device at %s does not support NeroProg\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	if ( progOpt->count ) {
		printf("Programming device...\n");
		if ( isNeroCapable ) {
			fStatus = flSelectConduit(handle, 0x00, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flProgram(handle, progOpt->sval[0], NULL, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		} else {
			fprintf(stderr, "Program operation requested but device at %s does not support NeroProg\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	if ( benOpt->count ) {
		enableBenchmarking = true;
	}
	
	if ( actOpt->count ) {
		printf("Executing CommFPGA actions on FPGALink device %s...\n", vp);
		if ( isCommCapable ) {
			uint8 isRunning;
			fStatus = flSelectConduit(handle, conduit, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flIsFPGARunning(handle, &isRunning, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			if ( isRunning ) {
				pStatus = parseLine(handle, actOpt->sval[0], &error);
				CHECK_STATUS(pStatus, pStatus, cleanup);
			} else {
				fprintf(stderr, "The FPGALink device at %s is not ready to talk - did you forget --program?\n", vp);
				FAIL(FLP_ARGS, cleanup);
			}
		} else {
			fprintf(stderr, "Action requested but device at %s does not support CommFPGA\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	if ( dumpOpt->count ) {
		const char *fileName;
		unsigned long chan = strtoul(dumpOpt->sval[0], (char**)&fileName, 10);
		FILE *file = NULL;
		const uint8 *recvData;
		uint32 actualLength;
		if ( *fileName != ':' ) {
			fprintf(stderr, "%s: invalid argument to option -l|--dumploop=<ch:file.bin>\n", progName);
			FAIL(FLP_ARGS, cleanup);
		}
		fileName++;
		printf("Copying from channel %lu to %s", chan, fileName);
		file = fopen(fileName, "wb");
		CHECK_STATUS(!file, FLP_CANNOT_SAVE, cleanup);
		sigRegisterHandler();
		fStatus = flSelectConduit(handle, conduit, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		fStatus = flReadChannelAsyncSubmit(handle, (uint8)chan, 22528, NULL, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		do {
			fStatus = flReadChannelAsyncSubmit(handle, (uint8)chan, 22528, NULL, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fwrite(recvData, 1, actualLength, file);
			printf(".");
		} while ( !sigIsRaised() );
		printf("\nCaught SIGINT, quitting...\n");
		fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		fwrite(recvData, 1, actualLength, file);
		fclose(file);
	}

	if ( shellOpt->count ) {
		printf("\nEntering CommFPGA command-line mode:\n");
		if ( isCommCapable ) {
		   uint8 isRunning;
			fStatus = flSelectConduit(handle, conduit, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flIsFPGARunning(handle, &isRunning, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			if ( isRunning ) {
				do {
					do {
						line = readline("> ");
					} while ( line && !line[0] );
					if ( line && line[0] && line[0] != 'q' ) {
						add_history(line);
						pStatus = parseLine(handle, line, &error);
						CHECK_STATUS(pStatus, pStatus, cleanup);
						free((void*)line);
					}
				} while ( line && line[0] != 'q' );
			} else {
				fprintf(stderr, "The FPGALink device at %s is not ready to talk - did you forget --xsvf?\n", vp);
				FAIL(FLP_ARGS, cleanup);
			}
		} else {
			fprintf(stderr, "Shell requested but device at %s does not support CommFPGA\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	if ( readOpt->count ) {
		printf("\nReading channel zero & writing channel one\n");
		if ( isCommCapable ) {
		   uint8 isRunning;
			fStatus = flSelectConduit(handle, conduit, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flIsFPGARunning(handle, &isRunning, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			if ( isRunning ) {
					int x=0, y=0, i, state=1, reset;
				do {
					reset = 0;
					if(state == 1){
						char dec_coord_bits[33];
						x = 0; y = 0;
						for(i=0; i<64; i++){
							printf("Current channel i is %d\n\n", i);
							//reading coordinates from channel 2*i
							char hexdata[9]="";
							for(int j=0; j<4; j++){
								char read_ch_num[4];
								sprintf(read_ch_num, "%x", 2*i);
								char line_temp[7]="r";
								strcat(line_temp, read_ch_num);
								strcat(line_temp, " 1");
								// printf("line_temp is %s\n", line_temp);
								line = line_temp;
								// printf("line read is %s\n", line);
								pStatus = parseLine(handle, line, &error);
								CHECK_STATUS(pStatus, pStatus, cleanup);
								// printf("read_hex is %s\n", str);
								strcat(hexdata, str);
								// printf("curr_hexdata is %s\n", hexdata);
							}
							printf("data read for coordinates is %s\n", hexdata);
							//converting hex to bits
							// printf("data in bits %s\n", hex_to_bit(hexdata));
							char enc_bits[33];
							sprintf(enc_bits, "%s", hex_to_bit(hexdata));
							// printf("enc_bits is %s\n", enc_bits);
							//decrypting the coordinates
							// char dec_bits[33];
							// printf("function output is %s\n", decryptor(enc_bits, key));
							sprintf(dec_coord_bits, "%s", decryptor(enc_bits, key));
							printf("decrypted bits of read coordinates is %s\n", dec_coord_bits);
							if(strncmp(dec_coord_bits, reset_str, 32) == 0){
								printf("Resetting Everything..............................................\n");
								reset = 1;
								break;
							}
							// printf("dec_bits is %s\n", dec_coord_bits);

							//re-encrypting the decrypte bits
							char enc_data[33];
							sprintf(enc_data, "%s", encryptor(dec_coord_bits, key));
							// printf("enc_data is %s\n", enc_data);
							//converting bits to hex
							char hex_data_enc[9];
							sprintf(hex_data_enc, "%s", bit_to_hex(enc_data));
							printf("written hex data of coordinates is %s\n", hex_data_enc);
							//writing encrypted hex to channel 2*i+1
							char write_ch_num[4];
							sprintf(write_ch_num,"%x",2*i+1);
							char line_temp[14]="w";
							strcat(line_temp, write_ch_num);
							strcat(line_temp, " ");
							strcat(line_temp, hex_data_enc);
							line = line_temp;
							pStatus = parseLine(handle, line, &error);
							CHECK_STATUS(pStatus, pStatus, cleanup);

							//reading Ack1 from channel 2*i
							char hexdata1[9]="";
							for(int j=0; j<4; j++){
								char read_ch_num[4];
								sprintf(read_ch_num, "%x", 2*i);
								char line_temp[7]="r";
								strcat(line_temp, read_ch_num);
								strcat(line_temp, " 1");
								// printf("line_temp is %s\n", line_temp);
								line = line_temp;
								// printf("line read is %s\n", line);
								pStatus = parseLine(handle, line, &error);
								CHECK_STATUS(pStatus, pStatus, cleanup);
								// printf("read_hex is %s\n", str);
								strcat(hexdata1, str);
								// printf("curr_hexdata1 is %s\n", hexdata1);
							}
							printf("hex of Ack1 read is %s\n", hexdata1);
							//converting hex to bits
							// printf("data in bits %s\n", hex_to_bit(hexdata1));
							char enc_bits1[33];
							sprintf(enc_bits1, "%s", hex_to_bit(hexdata1));
							// printf("enc_bits is %s\n", enc_bits1);
							//decrypting Ack1
							char dec_Ack1[33];
							// printf("function output is %s\n", decryptor(enc_bits1, key));
							sprintf(dec_Ack1, "%s", decryptor(enc_bits1, key));
							printf("decrypted bits of read Ack1 is %s\n", dec_Ack1);
							//checking if decrypte Ack1 is equal to actual Ack1
							if(strncmp(dec_Ack1,Ack1,32) == 0){
								printf("Ack1 matched\n");
								break;
							}
							if(strncmp(dec_Ack1, reset_str, 32) == 0){
								printf("Resetting Everything..............................................\n");
								reset = 1;
								break;
							}
							else{
								//waiting for 5 seconds
								delay(5000);
								char hexdata2[9]="";
								for(int j=0; j<4; j++){
									char read_ch_num[4];
									sprintf(read_ch_num, "%x", 2*i);
									char line_temp[7]="r";
									strcat(line_temp, read_ch_num);
									strcat(line_temp, " 1");
									// printf("line_temp is %s\n", line_temp);
									line = line_temp;
									// printf("line read is %s\n", line);
									pStatus = parseLine(handle, line, &error);
									CHECK_STATUS(pStatus, pStatus, cleanup);
									// printf("read_hex is %s\n", str);
									strcat(hexdata2, str);
									// printf("curr_hexdata2 is %s\n", hexdata2);
								}
								// printf("hexdata2 is %s\n", hexdata2);
								// printf("data in bits %s\n", hex_to_bit(hexdata2));
								char enc_bits2[33];
								sprintf(enc_bits2, "%s", hex_to_bit(hexdata2));
								// printf("enc_bits is %s\n", enc_bits2);
								char dec_Ack1_1[33];
								// printf("function output is %s\n", decryptor(enc_bits2, key));
								sprintf(dec_Ack1_1, "%s", decryptor(enc_bits2, key));
								// printf("dec_bits is %s\n", dec_Ack1_1);
								if(strncmp(dec_Ack1_1,Ack1,32) == 0){
									break;
								}
								if(strncmp(dec_Ack1_1, reset_str, 32) == 0){
								printf("Resetting Everything..............................................\n");
								reset = 1;
								break;
							}
							}
						}
						// printf("dec_bits is %s\n", dec_coord_bits);
						if(reset == 1){
							continue;
						}
						for(int k=0; k<4; k++){
							if(dec_coord_bits[k] == '1'){
								int j = 1 << (3-k);
								x += j;
							}
						}
						for(int k=4; k<8; k++){
							if(dec_coord_bits[k] == '1'){
								int j = 1 << (7-k);
								y += j;
							}
						}
						printf("%d\t%d\n", x,y);
						state = 2;
					}
					// }
				// }
					else if(state == 2){
						// write_fn(write_ch_num, Ack2);
						char enc_data[33];
						sprintf(enc_data, "%s", encryptor(Ack2, key));
						// printf("Ack2_enc is %s\n", enc_data);
						char hex_data_enc[9];
						sprintf(hex_data_enc, "%s", bit_to_hex(enc_data));
						printf("written hex of Ack2_enc is %s\n", hex_data_enc);
						char write_ch_num[4];
						sprintf(write_ch_num,"%x",2*i+1);
						char line_temp[14]="w";
						strcat(line_temp, write_ch_num);
						strcat(line_temp, " ");
						strcat(line_temp, hex_data_enc);
						line = line_temp;
						pStatus = parseLine(handle, line, &error);
						CHECK_STATUS(pStatus, pStatus, cleanup);

						struct data_ro* track_data = get_csv_arr(x, y);
						printf("File Read\n");
						// printf("I am here now\n" );
						char first4data[33]="";
						// memset(hex_data1, 0, 33);
						for(int i=0; i<4; i++){
							// printf("for entered\n");
							char one_byte[9];
							// printf("%s\n", bitmaker(track_data[i].track_exists, track_data[i].track_ok, track_data[i].dirn, track_data[i].next_sig));
							sprintf(one_byte, "%s", bitmaker(track_data[i].track_exists, track_data[i].track_ok, track_data[i].dirn, track_data[i].next_sig));
							// printf("one_byte is %s\n", one_byte);
							strcat(first4data, one_byte);
							// printf("first4data is %s\n", first4data);
						}
						// printf("for exited\n");
						// write_fn(write_ch_num, first4data);
						char enc_data1[33];
						sprintf(enc_data1, "%s", encryptor(first4data, key));
						// printf("first4data_enc is %s\n", enc_data1);
						char hex_data_enc1[9];
						sprintf(hex_data_enc1, "%s", bit_to_hex(enc_data1));
						printf("written hex data of first 4 bytes is %s\n", hex_data_enc1);
						// char write_ch_num[4];
						// sprintf(write_ch_num,"%x",2*i+1);
						char line_temp1[14]="w";
						strcat(line_temp1, write_ch_num);
						strcat(line_temp1, " ");
						strcat(line_temp1, hex_data_enc1);
						line = line_temp1;
						pStatus = parseLine(handle, line, &error);
						CHECK_STATUS(pStatus, pStatus, cleanup);
						// int start_time = ;
						// int cc=0;
						clock_t start_time = clock();
						while(true){
							// int end_time = ;
							// sprintf(dec_Ack1, "%s", read_fn(2*i));
							char hexdata1[9]="";
							for(int j=0; j<4; j++){
								char read_ch_num[4];
								sprintf(read_ch_num, "%x", 2*i);
								char line_temp[7]="r";
								strcat(line_temp, read_ch_num);
								strcat(line_temp, " 1");
								// printf("line_temp is %s\n", line_temp);
								line = line_temp;
								// printf("line read is %s\n", line);
								pStatus = parseLine(handle, line, &error);
								CHECK_STATUS(pStatus, pStatus, cleanup);
								// printf("read_hex is %s\n", str);
								strcat(hexdata1, str);
								// printf("curr_hexdata1 is %s\n", hexdata1);
							}
							printf("read hex of Ack1 is %s\n", hexdata1);
							// printf("data in bits %s\n", hex_to_bit(hexdata1));
							char enc_bits[33];
							sprintf(enc_bits, "%s", hex_to_bit(hexdata1));
							// printf("enc_bits is %s\n", enc_bits);
							// char dec_bits[33];
							char dec_Ack1[33];
							// memset(dec_bits, 0, 33);
							// printf("function output is %s\n", decryptor(enc_bits, key));
							sprintf(dec_Ack1, "%s", decryptor(enc_bits, key));
							printf("decrypted bits of read Ack1 is %s\n", dec_Ack1);
							// sprintf(dec_Ack1, "%s", read_fn(2*i));
							if(strncmp(dec_Ack1,Ack1,32) == 0){
								printf("Ack1 matched\n");
								break;
							}
							else if(strncmp(dec_Ack1, reset_str, 32) == 0){
								printf("Resetting Everything..............................................\n");
								reset =1;
								state = 1;
								break;
							}
							// if(end_time - start_time >= 256){
							// 	break;
							// }
							if(clock() >= start_time + 256000){
								state =1;
								break; 
							}
							// cc++;
						}
						if(state == 1){
							continue;
						}

						char last4data[33]="";
						// memset(hex_data1, 0, 33);
						for(int i=4; i<8; i++){
							// printf("for entered\n");
							char one_byte[9];
							// printf("%s\n", bitmaker(track_data[i].track_exists, track_data[i].track_ok, track_data[i].dirn, track_data[i].next_sig));
							sprintf(one_byte, "%s", bitmaker(track_data[i].track_exists, track_data[i].track_ok, track_data[i].dirn, track_data[i].next_sig));
							// printf("one_byte is %s\n", one_byte);
							strcat(last4data, one_byte);
							// printf("last4data is %s\n", last4data);
						}
						// printf("for exited\n");
						// write_fn(write_ch_num, last4data);
						char enc_data2[33];
						sprintf(enc_data2, "%s", encryptor(last4data, key));
						// printf("last4data_enc is %s\n", enc_data2);
						char hex_data_enc2[9];
						sprintf(hex_data_enc2, "%s", bit_to_hex(enc_data2));
						printf("written hex data of last 4 bytes is %s\n", hex_data_enc2);
						// char write_ch_num[4];
						// sprintf(write_ch_num,"%x",2*i+1);
						char line_temp2[14]="w";
						strcat(line_temp2, write_ch_num);
						strcat(line_temp2, " ");
						strcat(line_temp2, hex_data_enc2);
						line = line_temp2;
						pStatus = parseLine(handle, line, &error);
						CHECK_STATUS(pStatus, pStatus, cleanup);

						start_time = clock();
						while(true){
							// int end_time = ;
							// sprintf(dec_Ack1, "%s", read_fn(read_ch_num));
							// if(end_time - start_time >= 256){
							// 	break;
							// }
							char hexdata1[9]="";
							for(int j=0; j<4; j++){
								char read_ch_num[4];
								sprintf(read_ch_num, "%x", 2*i);
								char line_temp[7]="r";
								strcat(line_temp, read_ch_num);
								strcat(line_temp, " 1");
								// printf("line_temp is %s\n", line_temp);
								line = line_temp;
								// printf("line read is %s\n", line);
								pStatus = parseLine(handle, line, &error);
								CHECK_STATUS(pStatus, pStatus, cleanup);
								// printf("read_hex is %s\n", str);
								strcat(hexdata1, str);
								// printf("curr_hexdata1 is %s\n", hexdata1);
							}
							printf("read hex of Ack1 is %s\n", hexdata1);
							// printf("data in bits %s\n", hex_to_bit(hexdata1));
							char enc_bits[33];
							sprintf(enc_bits, "%s", hex_to_bit(hexdata1));
							// printf("enc_bits is %s\n", enc_bits);
							// char dec_bits[33];
							char dec_Ack1[33];
							// memset(dec_bits, 0, 33);
							// printf("function output is %s\n", decryptor(enc_bits, key));
							sprintf(dec_Ack1, "%s", decryptor(enc_bits, key));
							printf("decrypted bits of read Ack1 is %s\n", dec_Ack1);
							// sprintf(dec_Ack1, "%s", read_fn(2*i));
							if(strncmp(dec_Ack1,Ack1,32) == 0){
								printf("Ack1 matched\n");
								break;
							}
							else if(strncmp(dec_Ack1, reset_str, 32) == 0){
								printf("Resetting Everything..............................................\n");
								reset =1;
								state = 1;
								break;
							}
							if(clock() >= start_time + 256000){
								state =1;
								break; 
							}
							
						}
						if(state == 1){
							continue;
						}
						char enc_data3[33];
						sprintf(enc_data3, "%s", encryptor(Ack2, key));
						// printf("first4data_enc is %s\n", enc_data3);
						char hex_data_enc3[9];
						sprintf(hex_data_enc3, "%s", bit_to_hex(enc_data3));
						// char write_ch_num[4];
						// sprintf(write_ch_num,"%x",2*i+1);
						printf("written hex data of Ack2 is %s\n", hex_data_enc3);
						char line_temp3[14]="w";
						strcat(line_temp3, write_ch_num);
						strcat(line_temp3, " ");
						strcat(line_temp3, hex_data_enc3);
						line = line_temp3;
						pStatus = parseLine(handle, line, &error);
						CHECK_STATUS(pStatus, pStatus, cleanup);
						// write_fn(write_ch_num, Ack2);
						// delay(24000);
						start_time = clock();
						while(true){
							char hexdata1[9]="";
							for(int j=0; j<4; j++){
								char read_ch_num[4];
								sprintf(read_ch_num, "%x", 2*i);
								char line_temp[7]="r";
								strcat(line_temp, read_ch_num);
								strcat(line_temp, " 1");
								// printf("line_temp is %s\n", line_temp);
								line = line_temp;
								// printf("line read is %s\n", line);
								printf("Data needed \n");
								pStatus = parseLine(handle, line, &error);
								printf("Data got \n");
								CHECK_STATUS(pStatus, pStatus, cleanup);
								// printf("read_hex is %s\n", str);
								strcat(hexdata1, str);
								// printf("curr_hexdata1 is %s\n", hexdata1);
							}
							// printf("read hex of switch data is %s\n", hexdata1);
							// printf("data in bits %s\n", hex_to_bit(hexdata1));
							char enc_bits[33];
							sprintf(enc_bits, "%s", hex_to_bit(hexdata1));
							// printf("enc_bits is %s\n", enc_bits);
							// char dec_bits[33];
							char dec_data[33];
							// memset(dec_bits, 0, 33);
							// printf("function output is %s\n", decryptor(enc_bits, key));
							sprintf(dec_data, "%s", decryptor(enc_bits, key));
							printf("decrypted bits of read data is %s\n", dec_data);
							// sprintf(dec_Ack1, "%s", read_fn(2*i));
							char first_byte[9];
							for(int i=0; i<8; i++){
								first_byte[i]=dec_data[i];
							}
							first_byte[8]='\0';
							char last_byte[9];
							for(int i=0; i<8; i++){
								last_byte[i]=dec_data[i+24];
							}
							last_byte[8]='\0';
							char temp[9]="11111111";
							if(strncmp(dec_data,Ack3,32) == 0){
								printf("No Data Received \n");
								break;
							}
							else if(strncmp(dec_data, reset_str, 32) == 0){
								printf("Resetting Everything..............................................\n");
								reset =1;
								state = 1;
								break;
							}
							else if(strncmp(last_byte,temp,8) == 0) {
								printf("Data Recieved is %s\n", first_byte);
								write_csv(first_byte, "1.csv", x, y);
								printf("Data Written to csv file\n");
								break;
							}
							else{
								printf("Phantom data received. Dec data: %s\n", dec_data);
							}
							if(clock() >= start_time + 50000){
								printf("Time Exceeded for recieving data\n");
								break;
							}
							
						}

						state = 1;
						delay(2000);
					} 
				} while ( line && line[0] != 'q');
			} else {
				fprintf(stderr, "The FPGALink device at %s is not ready to talk - did you forget --xsvf?\n", vp);
				FAIL(FLP_ARGS, cleanup);
			}
		} else {
			fprintf(stderr, "Shell requested but device at %s does not support CommFPGA\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

cleanup:
	free((void*)line);
	flClose(handle);
	if ( error ) {
		fprintf(stderr, "%s\n", error);
		flFreeError(error);
	}
	return retVal;
}
