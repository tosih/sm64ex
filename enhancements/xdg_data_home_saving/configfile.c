// configfile.c - handles loading and saving the configuration options
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

    // These five next libraries are required for saving in the correct folders.
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <SDL2/SDL.h>
#include <errno.h>

#include <stdlib.h> // hacky debug

#include "configfile.h"

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

enum ConfigOptionType {
    CONFIG_TYPE_BOOL,
    CONFIG_TYPE_UINT,
    CONFIG_TYPE_FLOAT,
};

struct ConfigOption {
    const char *name;
    enum ConfigOptionType type;
    union {
        bool *boolValue;
        unsigned int *uintValue;
        float *floatValue;
    };
};

/*
 *Config options and default values
 */
bool configFullscreen            = false;
// Keyboard mappings (scancode values)
unsigned int configKeyA          = 0x26;
unsigned int configKeyB          = 0x33;
unsigned int configKeyStart      = 0x39;
unsigned int configKeyR          = 0x36;
unsigned int configKeyZ          = 0x25;
unsigned int configKeyCUp        = 0x148;
unsigned int configKeyCDown      = 0x150;
unsigned int configKeyCLeft      = 0x14B;
unsigned int configKeyCRight     = 0x14D;
unsigned int configKeyStickUp    = 0x11;
unsigned int configKeyStickDown  = 0x1F;
unsigned int configKeyStickLeft  = 0x1E;
unsigned int configKeyStickRight = 0x20;


static const struct ConfigOption options[] = {
    {.name = "fullscreen",     .type = CONFIG_TYPE_BOOL, .boolValue = &configFullscreen},
    {.name = "key_a",          .type = CONFIG_TYPE_UINT, .uintValue = &configKeyA},
    {.name = "key_b",          .type = CONFIG_TYPE_UINT, .uintValue = &configKeyB},
    {.name = "key_start",      .type = CONFIG_TYPE_UINT, .uintValue = &configKeyStart},
    {.name = "key_r",          .type = CONFIG_TYPE_UINT, .uintValue = &configKeyR},
    {.name = "key_z",          .type = CONFIG_TYPE_UINT, .uintValue = &configKeyZ},
    {.name = "key_cup",        .type = CONFIG_TYPE_UINT, .uintValue = &configKeyCUp},
    {.name = "key_cdown",      .type = CONFIG_TYPE_UINT, .uintValue = &configKeyCDown},
    {.name = "key_cleft",      .type = CONFIG_TYPE_UINT, .uintValue = &configKeyCLeft},
    {.name = "key_cright",     .type = CONFIG_TYPE_UINT, .uintValue = &configKeyCRight},
    {.name = "key_stickup",    .type = CONFIG_TYPE_UINT, .uintValue = &configKeyStickUp},
    {.name = "key_stickdown",  .type = CONFIG_TYPE_UINT, .uintValue = &configKeyStickDown},
    {.name = "key_stickleft",  .type = CONFIG_TYPE_UINT, .uintValue = &configKeyStickLeft},
    {.name = "key_stickright", .type = CONFIG_TYPE_UINT, .uintValue = &configKeyStickRight},
};

// Reads an entire line from a file (excluding the newline character) and returns an allocated string
// Returns NULL if no lines could be read from the file
static char *read_file_line(FILE *file) {
    char *buffer;
    size_t bufferSize = 8;
    size_t offset = 0; // offset in buffer to write

    buffer = malloc(bufferSize);
    while (1) {
        // Read a line from the file
        if (fgets(buffer + offset, bufferSize - offset, file) == NULL) {
            free(buffer);
            return NULL; // Nothing could be read.
        }
        offset = strlen(buffer);
        assert(offset > 0);

        // If a newline was found, remove the trailing newline and exit
        if (buffer[offset - 1] == '\n') {
            buffer[offset - 1] = '\0';
            break;
        }

        if (feof(file)) // EOF was reached
            break;

        // If no newline or EOF was reached, then the whole line wasn't read.
        bufferSize *= 2; // Increase buffer size
        buffer = realloc(buffer, bufferSize);
        assert(buffer != NULL);
    }

    return buffer;
}

// Returns the position of the first non-whitespace character
static char *skip_whitespace(char *str) {
    while (isspace(*str))
        str++;
    return str;
}

// NULL-terminates the current whitespace-delimited word, and returns a pointer to the next word
static char *word_split(char *str) {
    // Precondition: str must not point to whitespace
    assert(!isspace(*str));

    // Find either the next whitespace char or end of string
    while (!isspace(*str) && *str != '\0')
        str++;
    if (*str == '\0') // End of string
        return str;

    // Terminate current word
    *(str++) = '\0';

    // Skip whitespace to next word
    return skip_whitespace(str);
}

// Splits a string into words, and stores the words into the 'tokens' array
// 'maxTokens' is the length of the 'tokens' array
// Returns the number of tokens parsed
static unsigned int tokenize_string(char *str, int maxTokens, char **tokens) {
    int count = 0;

    str = skip_whitespace(str);
    while (str[0] != '\0' && count < maxTokens) {
        tokens[count] = str;
        str = word_split(str);
        count++;
    }
    return count;
}

// Loads the config file specified by 'filename'
void configfile_load(const char *filename) {
    FILE *file;
    char *line;

    printf("\nThis is a testing build from github.com/sm64pc/sm64pc. Report bugs there.\n\n");    

    
    DIR* conf_dir = opendir(SDL_GetPrefPath("", "sm64pc"));     // Checks for XDG_DATA_HOME/sm64pc
    char * cur_dir = SDL_GetBasePath();                         // Saves the current working directory so we can return to it later.

    if (ENOENT == errno)  
    {                                                      // XDG_DATA_HOME/sm64pc does not exist, so try to read from the current working directory.
        printf("%s not found.\n", SDL_GetPrefPath("", "sm64pc"));
        file = fopen(filename, "r");
        if (file == NULL) {
            printf("Config file '%s' not found. Creating it.\n", filename);   // Config file also not found in cwd, so make a new one.
            closedir(conf_dir);
            configfile_save(filename);
            return;
        } else printf("Loading configuration from '%s%s'\n", SDL_GetBasePath(), filename); // We've found a file in cwd!
    } else 
    {                                           
                                                // The config folder exists, so try to read from it.
        
        chdir(SDL_GetPrefPath("", "sm64pc"));       // Goes to XDG_DATA_HOME/sm64pc.
        file = fopen(filename, "r");            
        if (file == NULL)                       // If there's not a file here
        {
            chdir(cur_dir);           // Go back to the previous cwd
            printf("Config file '%s' not found. Creating it.\n", filename);
            closedir(conf_dir);
            configfile_save(filename);
            return;
        } else 
            printf("Loading configuration from '%s%s'\n", SDL_GetPrefPath("", "sm64pc"), filename); // We've found a file in XDG_DATA_HOME/sm64pc!
    }

    closedir(conf_dir);
    
    // Go through each line in the file
    while ((line = read_file_line(file)) != NULL) {
        char *p = line;
        char *tokens[2];
        int numTokens;

        while (isspace(*p))
            p++;
        numTokens = tokenize_string(p, 2, tokens);
        if (numTokens != 0) {
            if (numTokens == 2) {
                const struct ConfigOption *option = NULL;

                for (unsigned int i = 0; i < ARRAY_LEN(options); i++) {
                    if (strcmp(tokens[0], options[i].name) == 0) {
                        option = &options[i];
                        break;
                    }
                }
                if (option == NULL)
                    printf("unknown option '%s'\n", tokens[0]);
                else {
                    switch (option->type) {
                        case CONFIG_TYPE_BOOL:
                            if (strcmp(tokens[1], "true") == 0)
                                *option->boolValue = true;
                            else if (strcmp(tokens[1], "false") == 0)
                                *option->boolValue = false;
                            break;
                        case CONFIG_TYPE_UINT:
                            sscanf(tokens[1], "%u", option->uintValue);
                            break;
                        case CONFIG_TYPE_FLOAT:
                            sscanf(tokens[1], "%f", option->floatValue);
                            break;
                        default:
                            assert(0); // bad type
                    }
                    printf("option: '%s', value: '%s'\n", tokens[0], tokens[1]);
                }
            } else
                puts("error: expected value");
        }
        free(line);
    }

    fclose(file);
    chdir(cur_dir);
}

// Writes the config file to 'filename'
void configfile_save(const char *filename) {
    
    FILE *file;

    printf("Saving configuration to '%s%s'\n",SDL_GetPrefPath("", "sm64pc"),  filename);
    
    char * cur_dir = SDL_GetBasePath();                         // Current working directory
    DIR* conf_dir = opendir(SDL_GetPrefPath("", "sm64pc"));     // Checks if there's already an 'sm64pc' folder
    if (ENOENT == errno)  // Directory does not exist
    { 
        mkdir(SDL_GetPrefPath("", "sm64pc"), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);    // If there isn't, we'll make one.
	    conf_dir = opendir(SDL_GetPrefPath("", "sm64pc"));
	    if (errno == ENOENT) 
	    {
		    printf("Error: Couldn't get config path.\n");                      // If it still doesn't exist, bail out
		    exit(ENOENT);
	    }
    }
    

    closedir(conf_dir);
    chdir(SDL_GetPrefPath("", "sm64pc"));                                           // Go to the pref folder
    file = fopen(filename, "w");                                                    // Write the config file
    chdir(cur_dir);                                                                 // Come back. 
    

    if (file == NULL) {
        // error
        return;
    }

    for (unsigned int i = 0; i < ARRAY_LEN(options); i++) {
        const struct ConfigOption *option = &options[i];

        switch (option->type) {
            case CONFIG_TYPE_BOOL:
                fprintf(file, "%s %s\n", option->name, *option->boolValue ? "true" : "false");
                break;
            case CONFIG_TYPE_UINT:
                fprintf(file, "%s %u\n", option->name, *option->uintValue);
                break;
            case CONFIG_TYPE_FLOAT:
                fprintf(file, "%s %f\n", option->name, *option->floatValue);
                break;
            default:
                assert(0); // unknown type
        }
    }

    fclose(file);
}