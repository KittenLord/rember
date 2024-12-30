#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#include "planitems.c"
#include "interactive.c"
#include "atomic-hashmap.c"

#define SUB_INTERACTIVE "interactive"

#define STREAM_STR 1
#define STREAM_FILE 2
typedef struct {
    char type;
    union {
        struct {
            FILE *file;
        };
        struct {
            char *ptr;
            size_t length;
            size_t index;
        };
    };
} stream;

#define stream_str(str, len) ((stream){ .type = STREAM_STR, .ptr = str, .length = len, .index = 0 })
#define stream_strlen(str) ((stream){ .type = STREAM_STR, .ptr = str, .length = strlen(str), .index = 0 })
#define stream_file(f) ((stream){ .type = STREAM_FILE, .file = f })

char readStream(stream *s) {
    if(s->type == STREAM_FILE) {
        if(!s->file) return EOF;
        char c;
        int r = fread(&c, sizeof(char), 1, s->file);
        if(r <= 0) return EOF;
        return c;
    }
    else if(s->type == STREAM_STR) {
        if(!s->ptr) return EOF;
        if(s->index >= s->length) return EOF;
        return s->ptr[s->index++];
    }
}

char *strcatnew(char *a, char *b, bool freeA) {
    size_t alen = strlen(a);
    size_t blen = strlen(b);
    char *result = calloc(sizeof(char), alen + blen + 1); // null terminator
    memcpy(result, a, alen);
    memcpy(result + alen, b, blen);
    if(freeA) free(a);
    return result;
}

char *getPathWithHome(char *path) {
    if(!path) return NULL;
    if(*path != '~') return path;
    
    return strcatnew(getenv("HOME"), path + 1, false);
}

// false on failure
bool ensureDirectoryExists(char *path) {
    return mkdir(path, 0700) != -1 || errno == 17; // errno 17 : Already exists
}

void parseConfig(stream *s, struct AtomicHashmap *hm) {
    char bufKey[2048] = {0}; // i really don't wanna bother doing a dynamic array here
    char bufVal[2048] = {0};
    char *ptr = bufKey;
    char c;
    while((c = readStream(s)) != EOF && c != '\n' && c != '=') {
        *ptr++ = c;
    }
    
    if(strlen(bufKey) == 0) return;

    if(c == EOF) {
        // parsed key, but there was no value
        fprintf(stderr, "ERROR: error when parsing config, kill yourself\n");
        return;
    }

    // if c != EOF, then '=' has been consumed, so we can parse the value
    ptr = bufVal;
    while((c = readStream(s)) != EOF && c != '\n' && c != ';') {
        *ptr++ = c;
    }

    setHM_LL(hm, bufKey, bufVal);

    if(c == EOF) return; // end of input
    parseConfig(s, hm); // '\n' or ';' have already been consumed, we can just recursively parse
}

FILE *getConfig() {
    char *configPath = getPathWithHome(getenv("XDG_CONFIG_HOME"));
    bool result = true;
    bool useDefaultPath = false;

    // if XDG_CONFIG_HOME is not set use default
    if(!configPath) {
        useDefaultPath = true;
        configPath = getenv("HOME");
        configPath = strcatnew(configPath, "/.config", false);
        result = ensureDirectoryExists(configPath);
        if(!result) return NULL;
    }

    configPath = strcatnew(configPath, "/rember", useDefaultPath);
    result = ensureDirectoryExists(configPath);

    if(!result) return NULL;

    configPath = strcatnew(configPath, "/config", true);
    FILE *configFile = fopen(configPath, "r");

    if(!configFile) {
        configFile = fopen(configPath, "w");
        if(!configFile) {}
        else { fclose(configFile); }
    }

    free(configPath);

    return configFile;
}

int main(int argc, char **argv) {
    struct AtomicHashmap configHM = createHM();
    FILE *configFile = getConfig();

    // TODO: later provide ability to pass config via a string arg
    stream configStream = stream_file(configFile);

    parseConfig(&configStream, &configHM);

    // insertHM_LL(&config, "CONFIG_PATH", "");

    // size_t length;
    // char *testKey = getHM(&configHM, "test", 4, &length);
    // if(!testKey) {
    //     printf("no such key\n");
    // }
    // else {
    //     char *t = calloc(sizeof(char), length + 1);
    //     memcpy(t, testKey, length);
    //     printf("%s\n", t);
    // }

    char *subcommand = SUB_INTERACTIVE;

    for(int i = 1; i < argc; i++) {
        printf("%s\n", argv[i]);
    }

    if(!strcmp(subcommand, SUB_INTERACTIVE)) {
        interactive();
    }
}
