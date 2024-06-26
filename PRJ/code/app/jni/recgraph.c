/*
 * recgraph.c -
 * This source file manages the ¡®recgraph¡¯ data structure for music recommendation
 * and communicates with the Linux kernel module through file operations.
 *
 * Author : 20211584 Junyeong Jang
 */

#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <android/log.h>
#define LOG_TAG "RECGRAPH"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

// #define DEBUG_LOGGING
#define SAVE_DATA_LOCAL_TMP

/* consts */
#define NUM_OF_NODES 10
#define NODES_IDX (NUM_OF_NODES + 1)
#define QUEUE_SIZE 10
#define INT_MAX (int)0x7FFFFFFE

/* dat file path */
#ifdef SAVE_DATA_LOCAL_TMP
#define RECGRAPH_DAT "/data/local/tmp/_recgraph_.dat"
#else
#define RECGRAPH_DAT "_recgraph_.dat"
#endif

/* device file params */
#define MAJOR_NUM 242
#define DEV_NAME "music_driver"
#define DEV_FILE_LOC "/dev/music_driver"

/* IOCTL number */
#define IOCTL_OPTION _IOW(MAJOR_NUM, 20211584, char *)

/* recgraph EDGE */
typedef struct _EDGE
{
    int node;
    int weight;
} EDGE;

/* global vars */
static int **graph;
static EDGE *maxEdge;
int module_fd = -1;

/* printGraph - Function to print recgraph (for debugging). */
static void printGraph()
{
    int i, j;
    char nodes[NODES_IDX + 1] = {0};
    LOGD("[GRAPH]");
    for (i = 0; i < NODES_IDX; i++)
    {
        for (j = 0; j < NODES_IDX; j++)
        {
            nodes[j] = graph[i][j] + '0';
        }
        LOGD("%s", nodes);
    }

    LOGD("[MAX_EDGE]");
    for (i = 0; i < NODES_IDX; i++)
        LOGD("(%d, %d) ", maxEdge[i].node, maxEdge[i].weight);
}

/* initializeGraph - JNI function to initialize recgraph. */
JNIEXPORT void JNICALL
Java_org_example_musicplayer_MusicService_initializeGraph(JNIEnv *env, jobject this)
{
#ifdef DEBUG_LOGGING
    LOGD("initializeGraph called.");
#endif
    int i, j;
#ifdef SAVE_DATA_LOCAL_TMP
    FILE *fp = fopen(RECGRAPH_DAT, "r");
#else
    /* Get the AppData path from the Java method. */
    jclass cls = (*env)->GetObjectClass(env, this);
    jmethodID mid = (*env)->GetMethodID(env, cls, "getAppDataFilepath", "(Ljava/lang/String;)Ljava/lang/String;");
    jstring filename = (*env)->NewStringUTF(env, RECGRAPH_DAT);
    jstring filepath = (jstring)(*env)->CallObjectMethod(env, this, mid, filename);
    const char *filepathStr = (*env)->GetStringUTFChars(env, filepath, NULL);
    FILE *fp = fopen(filepathStr, "r");
#endif
    if (fp != NULL)
    {
        /* Allocate memory for the graph and maxEdge. */
        graph = (int **)malloc(sizeof(int *) * NODES_IDX);
        maxEdge = (EDGE *)malloc(sizeof(EDGE) * NODES_IDX);
        /* Read and store recgraph information from _recgraph_.dat. */
        for (i = 0; i < NODES_IDX; i++)
        {
            graph[i] = (int *)malloc(sizeof(int) * NODES_IDX);
            for (j = 0; j < NODES_IDX; j++)
            {
                fscanf(fp, "%d", &graph[i][j]);
            }
            fscanf(fp, "%d%d", &maxEdge[i].node, &maxEdge[i].weight);
        }
        fclose(fp);
    }
    else
    {
        /* Allocate memory for the graph and maxEdge. */
        graph = (int **)malloc(sizeof(int *) * NODES_IDX);
        maxEdge = (EDGE *)malloc(sizeof(EDGE) * NODES_IDX);
        /* Initialize graph and maxEdge. */
        for (i = 0; i < NODES_IDX; i++)
        {
            graph[i] = (int *)malloc(sizeof(int) * NODES_IDX);
            for (j = 0; j < NODES_IDX; j++)
                graph[i][j] = 1;
            maxEdge[i].node = i % QUEUE_SIZE + 1;
            maxEdge[i].weight = 1;
        }
    }
    /* Open the device file. */
    module_fd = open(DEV_FILE_LOC, O_RDWR);
#ifdef DEBUG_LOGGING
    printGraph();
#endif
}

/* updateGraph - JNI function to update weights. */
JNIEXPORT void JNICALL
Java_org_example_musicplayer_MusicService_updateGraph(JNIEnv *env, jobject this,
                                                      jint prev, jint curr)
{
    /* Increases the edge weight by 1 between consecutively added music. */
    if (graph[prev][curr] < INT_MAX)
        graph[prev][curr]++;
    /* Update the maxEdge. */
    if (graph[prev][curr] > maxEdge[prev].weight)
    {
        maxEdge[prev].node = curr;
        maxEdge[prev].weight = graph[prev][curr];
    }
#ifdef DEBUG_LOGGING
    printGraph();
#endif
}

/* recommendMusic - JNI function to recommend music by recgraph. */
JNIEXPORT void JNICALL
Java_org_example_musicplayer_MusicService_recommendMusic(JNIEnv *env, jobject service)
{
    jclass cls = (*env)->GetObjectClass(env, service);

    /* Access the queue field. */
    jfieldID fid = (*env)->GetFieldID(env, cls, "queue", "Ljava/util/Queue;");
    jobject queue = (*env)->GetObjectField(env, service, fid); /* queue */

    /* Access the necessary class and method fields. */
    jclass queueCls = (*env)->GetObjectClass(env, queue);
    jclass integerCls = (*env)->FindClass(env, "java/lang/Integer");
    jmethodID sizeMethod = (*env)->GetMethodID(env, queueCls, "size", "()I");
    jmethodID addMethod = (*env)->GetMethodID(env, queueCls, "add", "(Ljava/lang/Object;)Z");
    jmethodID peekMethod = (*env)->GetMethodID(env, queueCls, "peek", "()Ljava/lang/Object;");
    jmethodID toArrayMethod = (*env)->GetMethodID(env, queueCls, "toArray", "()[Ljava/lang/Object;");
    jmethodID intValueMethod = (*env)->GetMethodID(env, integerCls, "intValue", "()I");

    /* Fill the queue with recommended music up to QUEUE_SIZE. */
    int size = (*env)->CallIntMethod(env, queue, sizeMethod);
    if (size == 0)
        return;
    while (size < QUEUE_SIZE)
    {
        jobjectArray q2array = (jobjectArray)(*env)->CallObjectMethod(env, queue, toArrayMethod);
        jobject lastElement = (*env)->GetObjectArrayElement(env, q2array, size - 1);
        int last = (*env)->CallIntMethod(env, lastElement, intValueMethod);
        /* Recommend the node with the largest weight that can be reached from last node. */
        int next = maxEdge[last].node;
        jobject nextElement = (*env)->NewObject(env, integerCls, (*env)->GetMethodID(env, integerCls, "<init>", "(I)V"), next);
        (*env)->CallBooleanMethod(env, queue, addMethod, nextElement);

        size = (*env)->CallIntMethod(env, queue, sizeMethod);
    }
}

/* saveGraph - JNI function to save the graph. */
JNIEXPORT void JNICALL
Java_org_example_musicplayer_MusicService_saveGraph(JNIEnv *env, jobject this)
{
#ifdef DEBUG_LOGGING
    LOGD("saveGraph called.");
#endif
    int i, j;
#ifdef SAVE_DATA_LOCAL_TMP
    FILE *fp = fopen(RECGRAPH_DAT, "w");
#else
    /* Get the AppData path from the Java method. */
    jclass cls = (*env)->GetObjectClass(env, this);
    jmethodID mid = (*env)->GetMethodID(env, cls, "getAppDataFilepath", "(Ljava/lang/String;)Ljava/lang/String;");
    jstring filename = (*env)->NewStringUTF(env, RECGRAPH_DAT);
    jstring filepath = (jstring)(*env)->CallObjectMethod(env, this, mid, filename);
    const char *filepathStr = (*env)->GetStringUTFChars(env, filepath, NULL);
    FILE *fp = fopen(filepathStr, "w");
#endif
    /* Write recgraph information to _recgraph_.dat. */
    for (i = 0; i < NODES_IDX; i++)
    {
        for (j = 0; j < NODES_IDX; j++)
        {
            fprintf(fp, "%d ", graph[i][j]);
        }
        fprintf(fp, "\n%d %d\n", maxEdge[i].node, maxEdge[i].weight);
    }
    fclose(fp);

    /* Free the memory allocated for the graph and maxEdge. */
    free(maxEdge);
    for (i = 0; i < NODES_IDX; i++)
    {
        free(graph[i]);
    }
    free(graph);
    /* Close the device file. */
    close(module_fd);
}

/* resetInput - JNI function to get input from RESET switch. */
JNIEXPORT int JNICALL
Java_org_example_musicplayer_MusicService_resetInput(JNIEnv *env, jobject this)
{
    return read(module_fd, NULL, 0);
}

/* deviceOutput - JNI function to pass the necessary information to the output devices by IOCTL. */
JNIEXPORT void JNICALL
Java_org_example_musicplayer_MusicService_deviceOutput(JNIEnv *env, jobject service)
{
    int i;
    jclass cls = (*env)->GetObjectClass(env, service);

    /* Access the queue field. */
    jfieldID queueFid = (*env)->GetFieldID(env, cls, "queue", "Ljava/util/Queue;");
    jobject queue = (*env)->GetObjectField(env, service, queueFid); /* queue */

    /* Access the durationTime field. */
    jfieldID durationTimeFid = (*env)->GetFieldID(env, cls, "durationTime", "I");
    jint durationTime = (*env)->GetIntField(env, service, durationTimeFid); /* durationTime */

    /* Access the remainingTime field. */
    jfieldID remainingTimeFid = (*env)->GetFieldID(env, cls, "remainingTime", "I");
    jint remainingTime = (*env)->GetIntField(env, service, remainingTimeFid); /* remainingTime */

    /* Convert the four elements at the front of the queue to a string.
     * (e.g. |1|4|7|10|5|3|... -> "0 3 6 9 ")
     */
    jclass queueCls = (*env)->GetObjectClass(env, queue);
    jmethodID toArrayMethod = (*env)->GetMethodID(env, queueCls, "toArray", "()[Ljava/lang/Object;");
    jobjectArray array = (jobjectArray)(*env)->CallObjectMethod(env, queue, toArrayMethod);
    jsize len = (*env)->GetArrayLength(env, array);
    char queueStr[13] = {0};
    for (i = 0; i < 4 && i < len; i++)
    {
        jobject element = (*env)->GetObjectArrayElement(env, array, i);
        jclass integerCls = (*env)->GetObjectClass(env, element);
        jmethodID intValueMethod = (*env)->GetMethodID(env, integerCls, "intValue", "()I");
        int value = (*env)->CallIntMethod(env, element, intValueMethod);

        char str[3];
        sprintf(str, "%d ", value - 1);
        strcat(queueStr, str);
    }

    /* Prepare the IOCTL arguments.
     * (e.g. "0 3 6 9 |392|983")
     */
    char ioctlArgs[33] = {0};
    sprintf(ioctlArgs, "%s|%d|%d", queueStr, remainingTime, durationTime);

    /* Send the IOCTL command. */
    ioctl(module_fd, IOCTL_OPTION, ioctlArgs);
}

/* initDisplay - JNI function to initialize the output devices. */
JNIEXPORT void JNICALL
Java_org_example_musicplayer_MusicService_initDisplay(JNIEnv *env, jobject this)
{
    write(module_fd, NULL, 0);
}
