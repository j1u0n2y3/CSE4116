#include "merge_process.h"

void merge_process()
{
    printf("BEGIN : merge process\n");

    int merge_q = shmget(MERGE_KEY, MERGE_MSG_SIZE, 0666 | IPC_CREAT);
    struct merge_msg *merge = (struct merge_msg *)shmat(merge_q, NULL, 0);
    merge->_BACK_=false;
    merge->_CALL_=false;

    while(1){
        int file_cnt = file_count("storage_files");
        if(merge->_BACK_){
            usleep(4000000);
            if(file_cnt == 3)
                merge_files("storage_files");
            break;
        }
        else if(merge->_CALL_){
            merge_files("storage_files");
        }
        else if(file_cnt ==3 ){
            merge_files("storage_files");
        }
        //usleep(100000);
    }

    printf("END : merge process\n");
}

int file_count(const char *dirPath) {
    DIR *dir;
    struct dirent *entry;
    int fileCount = 0;

    dir = opendir(dirPath);
    if (dir == NULL) {
        perror("opendir");
        return -1; // 에러 발생시 -1 반환
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue; // 숨김 파일 및 현재/상위 디렉토리 항목 건너뛰기
        }
        if (entry->d_type != DT_DIR) {
            fileCount++;
        }
    }

    closedir(dir);
    return fileCount;
}

void merge_files(const char *dirPath){
    motor_dd(MOTOR_ON);
    char file1Path[256], file2Path[256], file3Path[256], tmpFilePath[256];
    struct stat st;
    int file1Exists = 0, file2Exists = 0, file3Exists = 0;
    int fileCount = 0; // 파일 개수를 세기 위한 변수

    // 파일 경로 설정
    snprintf(file1Path, sizeof(file1Path), "%s/1.st", dirPath);
    snprintf(file2Path, sizeof(file2Path), "%s/2.st", dirPath);
    snprintf(file3Path, sizeof(file3Path), "%s/3.st", dirPath);
    snprintf(tmpFilePath, sizeof(tmpFilePath), "%s/tmp.st", dirPath);

    // 파일 존재 여부 확인
    file1Exists = (stat(file1Path, &st) == 0);
    file2Exists = (stat(file2Path, &st) == 0);
    file3Exists = (stat(file3Path, &st) == 0);

    // 파일 개수 세기
    fileCount += file1Exists ? 1 : 0;
    fileCount += file2Exists ? 1 : 0;
    fileCount += file3Exists ? 1 : 0;

    // 파일이 단 하나만 존재하는 경우 함수 종료
    if (fileCount == 1) {
        printf("단 하나의 파일만 존재합니다. 아무 작업도 수행하지 않고 종료합니다.\n");
        return;
    }

    // tmp.st 파일 생성
    FILE *tmpFile = fopen(tmpFilePath, "w");
    if (!tmpFile) {
        perror("fopen tmpFile");
        exit(EXIT_FAILURE);
    }
    
    mergeAndSaveRecords("storage_files/1.st", "storage_files/2.st", "storage_files/tmp.st");
    fclose(tmpFile);

    // 파일 처리 로직
    if (file1Exists && file2Exists && !file3Exists) {
        // 1.st와 2.st만 존재하는 경우
        remove(file1Path);
        remove(file2Path);
        rename(tmpFilePath, file1Path);
    } else if (file1Exists && file2Exists && file3Exists) {
        // 1.st, 2.st, 3.st 모두 존재하는 경우
        remove(file1Path);
        remove(file2Path);
        rename(file3Path, file1Path);
        rename(tmpFilePath, file2Path);
    } else {
        printf("지정된 조건에 맞는 파일 구성이 아닙니다.\n");
    }

    printf("파일 처리 완료.\n");
    usleep(2500000);
    motor_dd(MOTOR_OFF);
}

// 레코드 비교 함수 (key 기준 오름차순)
int compareRecords(const void *a, const void *b) {
    Record *recordA = (Record *)a;
    Record *recordB = (Record *)b;
    return recordA->key - recordB->key;
}

// 파일에서 레코드를 읽어오는 함수
void readRecordsFromFile(const char *filename, Record **records, int *count) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("파일 열기 실패");
        return;
    }

    int order, key;
    char value[5];
    while (fscanf(file, "%d %d %s", &order, &key, value) == 3) {
        (*records)[*count].order = order;
        (*records)[*count].key = key;
        strcpy((*records)[*count].value, value);
        (*count)++;
    }

    fclose(file);
}

// 레코드를 합치고 정렬하여 출력 파일에 저장하는 함수
void mergeAndSaveRecords(const char *inputFile1, const char *inputFile2, const char *outputFile) {
    Record *records = malloc(sizeof(Record) * 2000); // 예상 최대 레코드 수
    int count = 0;

    readRecordsFromFile(inputFile1, &records, &count);
    readRecordsFromFile(inputFile2, &records, &count);

    // key 기준으로 레코드 정렬
    qsort(records, count, sizeof(Record), compareRecords);

    // 중복 제거 및 파일 저장
    FILE *outFile = fopen(outputFile, "w");
    if (!outFile) {
        perror("출력 파일 열기 실패");
        free(records);
        return;
    }
    int i;
    int new_order=1;
    for (i = 0; i < count; i++) {
        if (i < count - 1 && records[i].key == records[i + 1].key) {
            continue; // 중복 key는 건너뛰기
        }
        fprintf(outFile, "%d %d %s\n", new_order++, records[i].key, records[i].value);
    }

    fclose(outFile);
    free(records);
}
