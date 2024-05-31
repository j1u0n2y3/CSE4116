#include "merge_process.h"

void merge_process()
{
    printf("BEGIN : merge process\n");

    int merge_q = shmget(MERGE_KEY, MERGE_MSG_SIZE, 0666 | IPC_CREAT);
    struct merge_msg *merge = (struct merge_msg *)shmat(merge_q, NULL, 0);
    merge->_BACK_ = false;
    merge->_CALL_ = false;
    merge->merge_end = false;

    while (1)
    {
        int file_cnt = file_count("storage_files");
        if (merge->_BACK_)
        {
            usleep(4000000);
            if (file_cnt == 3)
                merge_files("storage_files");
            break;
        }
        else if (merge->_CALL_)
        {
            merge_files("storage_files");
            merge->merge_end = true;
            merge->_CALL_ = false;
        }
        else if (file_cnt == 3)
        {
            merge_files("storage_files");
        }
        // usleep(100000);
    }

    printf("END : merge process\n");
}

int file_count(const char *dirPath)
{
    DIR *dir;
    struct dirent *entry;
    int fileCount = 0;

    dir = opendir(dirPath);
    if (dir == NULL)
    {
        perror("opendir");
        return -1; 
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
        {
            continue;
        }
        if (entry->d_type != DT_DIR)
        {
            fileCount++;
        }
    }

    closedir(dir);
    return fileCount;
}

void merge_files(const char *dirPath)
{
    motor_dd(MOTOR_ON);

    char file1Path[256], file2Path[256], file3Path[256], tmpFilePath[256];
    struct stat st;
    int file1Exists = 0, file2Exists = 0, file3Exists = 0;
    int fileCount = 0;

    snprintf(file1Path, sizeof(file1Path), "%s/1.st", dirPath);
    snprintf(file2Path, sizeof(file2Path), "%s/2.st", dirPath);
    snprintf(file3Path, sizeof(file3Path), "%s/3.st", dirPath);
    snprintf(tmpFilePath, sizeof(tmpFilePath), "%s/tmp.st", dirPath);

    file1Exists = (stat(file1Path, &st) == 0);
    file2Exists = (stat(file2Path, &st) == 0);
    file3Exists = (stat(file3Path, &st) == 0);

    fileCount += file1Exists ? 1 : 0;
    fileCount += file2Exists ? 1 : 0;
    fileCount += file3Exists ? 1 : 0;

    if (fileCount == 1)
        return;

    FILE *tmpFile = fopen(tmpFilePath, "w");
    if (!tmpFile)
    {
        perror("fopen tmpFile");
        exit(EXIT_FAILURE);
    }

    mergeAndSaveRecords("storage_files/1.st",
                        "storage_files/2.st",
                        "storage_files/tmp.st");
    fclose(tmpFile);

    if (file1Exists && file2Exists && !file3Exists)
    {
        remove(file1Path);
        remove(file2Path);
        rename(tmpFilePath, file1Path);
    }
    else if (file1Exists && file2Exists && file3Exists)
    {
        remove(file1Path);
        remove(file2Path);
        rename(file3Path, file2Path);
        rename(tmpFilePath, file1Path);
    }

    usleep(2000000);
    motor_dd(MOTOR_OFF);
}

int compareRecords(const void *a, const void *b)
{
    Record *recordA = (Record *)a;
    Record *recordB = (Record *)b;
    return recordA->key - recordB->key;
}

void readRecordsFromFile(const char *filename, Record **records, int *count)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        return;
    }

    int order, key;
    char value[5];
    while (fscanf(file, "%d %d %s", &order, &key, value) == 3)
    {
        (*records)[*count].order = order;
        (*records)[*count].key = key;
        strcpy((*records)[*count].value, value);
        (*count)++;
    }

    fclose(file);
}

void mergeAndSaveRecords(const char *inputFile1, const char *inputFile2, const char *outputFile)
{
    Record *records = malloc(sizeof(Record) * 2000);
    int count = 0;

    readRecordsFromFile(inputFile1, &records, &count);
    readRecordsFromFile(inputFile2, &records, &count);

    qsort(records, count, sizeof(Record), compareRecords);

    FILE *outFile = fopen(outputFile, "w");
    if (!outFile)
    {
        free(records);
        return;
    }
    int i;
    int new_order = 1;
    for (i = 0; i < count; i++)
    {
        if (i < count - 1 && records[i].key == records[i + 1].key)
            continue;
        fprintf(outFile, "%d %d %s\n", new_order++, records[i].key, records[i].value);
    }

    fclose(outFile);
    free(records);
}



