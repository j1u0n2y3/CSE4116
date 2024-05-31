#ifndef _MERGE_P_H_
#define _MERGE_P_H_

#include "main.h"
#include "device.h"

typedef struct
{
    int order;
    int key;
    char value[5];
} Record;

void merge_process();
void merge_files(const char *dirPath);
int file_count(const char *dirPath);
void mergeAndSaveRecords(const char *inputFile1, const char *inputFile2, const char *outputFile);
void readRecordsFromFile(const char *filename, Record **records, int *count);
int compareRecords(const void *a, const void *b);

#endif /* _MERGE_P_H_ */