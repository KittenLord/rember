#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#include "planitems.c"
#include "interactive.c"
#include "atomic-hashmap.c"

#define SUB_INTERACTIVE "interactive"

#define CONFIG_DATAPATH "datapath"

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
bool ensureExists(char *path) {
    path = getPathWithHome(path); // hopefully shouldnt be needed
    size_t length = strlen(path);
    size_t index = 0;

    char *tpath = calloc(sizeof(char), length + 1);

    while(index < length) {
        do {
            tpath[index] = path[index];
            index++;
        } while(path[index] != '\0' && path[index] != '/');

        if(path[index] == '/') { // directory
            if(mkdir(tpath, 0700) == -1 && errno != 17) {} // return false;
        }
        else if(path[index] == '\0') { // file
            if(path[index - 1] == '/') return true; // path ends with / to specify directory
            FILE *file = fopen(tpath, "r");
            if(!file) file = fopen(tpath, "w");
            if(!file) {} // return false;
            if(file) fclose(file);
        }
        else { assert(false); }
    }
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

char *getPathVar(char *variable, char *fallback) {
    char *value = getenv(variable);
    if(!value || strlen(value) == 0) return getPathWithHome(fallback);
    return getPathWithHome(value);
}

int main(int argc, char **argv) {
    // so much memory gets leaked nooooo
    struct AtomicHashmap configHM = createHM();
    char *dataPath = getPathVar("XDG_DATA_HOME", "~/.local/share/rember/data.rember");
    setHM_LL(&configHM, "datapath", dataPath);
    // insertHM_LL(&config, "CONFIG_PATH", "");
    
    char *configPath = getPathVar("XDG_CONFIG_HOME", "~/.config/rember/config");
    bool exists = ensureExists(configPath);

    FILE *configFile = fopen(configPath, "r");

    // TODO: later provide ability to pass config via a string arg
    stream configStream = stream_file(configFile);

    parseConfig(&configStream, &configHM);

    char *subcommand = SUB_INTERACTIVE;

    for(int i = 1; i < argc; i++) {
        printf("%s\n", argv[i]);
    }

    // config is to never be changed after this, so we can skip copying this pointer to a local buffer since the pointer will be persistent
    dataPath = getHM(&configHM, CONFIG_DATAPATH, strlen(CONFIG_DATAPATH), NULL);
    ensureExists(dataPath);

    if(!strcmp(subcommand, SUB_INTERACTIVE)) {
        interactive(dataPath);
    }
}
