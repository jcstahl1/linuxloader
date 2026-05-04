#ifdef __linux__
#include <stdlib.h>
#include <unistd.h>

#include "log/log.h"
#include "mainShared.h"

uint32_t partialElfCrc = 0;

int main(int argc, char *argv[])
{
    char command[MAX_PATH_LENGTH];
    char originalDir[MAX_PATH_LENGTH];
    char gameELF[MAX_PATH_LENGTH];
    char libraryPath[MAX_PATH_LENGTH];

    parseArgs(argc, argv, command, originalDir, gameELF, libraryPath);

    log_info("Starting $ %s", command);

    int sysCmd = system(command);

    if (chdir(originalDir) != 0)
    {
        log_error("Could not return to the original directory.");
        return EXIT_FAILURE;
    }

    return sysCmd;
}
#endif