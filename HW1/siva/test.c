#include <stdio.h>
#include <string.h>

// 숫자 키와 해당하는 문자들을 매핑
char* keys[] = {" ", " ", "ABC", "DEF", "GHI", "JKL", "MNO", "PQRS", "TUV", "WXYZ"};

// 숫자 시퀀스를 영문자로 변환하는 함수
void convertSequenceToText(char* sequence) {
    int index = 0; // 현재 처리 중인 시퀀스의 인덱스
    int count = 0; // 같은 숫자가 연속으로 나타난 횟수
    char currentChar = sequence[0]; // 현재 처리 중인 숫자
    int mode = 0; // 모드 (0: 영어, 1: 숫자)
    
    while(sequence[index] != '\0') {
        if(sequence[index] == '1') { // '1'이면 모드 전환
            mode = !mode; // 모드 전환
            index++;
            continue;
        }

        if(mode == 1) { // 숫자 모드인 경우
            printf("%c", sequence[index]);
            index++;
            continue;
        }

        // 다음 숫자로 넘어갔거나, 문자열의 끝에 도달한 경우
        if(currentChar != sequence[index] || sequence[index] == '\0') {
            if(currentChar != '1') {
                // 해당 숫자에 대응되는 문자를 출력
                int keyIndex = currentChar - '0'; // ASCII 코드를 이용해 인덱스 계산
                printf("%c", keys[keyIndex][(count-1) % strlen(keys[keyIndex])]);
            }
            count = 0; // 카운트 초기화
        }

        // 같은 숫자가 연속으로 나타난 경우
        count++;
        currentChar = sequence[index];
        index++;
    }

    // 마지막 문자 처리
    if(currentChar != '1' && mode == 0) {
        int keyIndex = currentChar - '0';
        printf("%c", keys[keyIndex][(count-1) % strlen(keys[keyIndex])]);
    }

    printf("\n");
}

int main() {
    char sequence[] = "22225577199";
    convertSequenceToText(sequence); // 이제 AKQX를 출력해야 함
    return 0;
}
