# -Wno-pointer-sign because of char* and uint8_t* distinction while trying to store strings in the hashmap (fix this whenever I implement a new hashmap)
gcc ./src/main.c -o ./out/rember -ggdb -Wall -Wextra -Wno-pointer-sign && ./out/rember $@
