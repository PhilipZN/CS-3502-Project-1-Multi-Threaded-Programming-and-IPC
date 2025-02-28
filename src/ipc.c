#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>

int main(int argc, char* argv[]) {
    // Determine target directory for ls. If an argument is provided, use it; otherwise use current directory.
    const char *targetDir = (argc > 1 ? argv[1] : ".");
    printf("===== Project B: IPC (Pipe) Implementation =====\n");
    printf("Executing 'ls -l %s' and processing output...\n", targetDir);

    // Create pipe
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    // Fork child process
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /*** Child Process ***/
        // Child will write to pipe
        close(pipefd[0]);              // close unused read end
        dup2(pipefd[1], STDOUT_FILENO); // redirect stdout to pipe write end
        close(pipefd[1]);              // close original write descriptor

        // Execute "ls -l <targetDir>"
        execlp("ls", "ls", "-l", targetDir, (char*)NULL);
        // If exec fails, print error and exit
        perror("execlp ls");
        _exit(1);
    } 
    else {
        /*** Parent Process ***/
        close(pipefd[1]);  // close unused write end of pipe

        // Open the pipe read end as a FILE* stream for easier reading
        FILE *stream = fdopen(pipefd[0], "r");
        if (!stream) {
            perror("fdopen");
            exit(EXIT_FAILURE);
        }

        struct timeval start, end;
        gettimeofday(&start, NULL);

        // Variables for parsing and statistics
        char *line = NULL;
        size_t len = 0;
        ssize_t nread;
        long long total_bytes = 0;      // total bytes read from pipe
        int total_entries = 0;          // total items (files + directories)
        int file_count = 0;
        int dir_count = 0;
        long long total_file_size = 0;  // sum of sizes of files

        // Read and process each line from the pipe
        while ((nread = getline(&line, &len, stream)) != -1) {
            total_bytes += nread;
            // Remove trailing newline for easier parsing
            if (nread > 0 && line[nread - 1] == '\n') {
                line[nread - 1] = '\0';
            }
            if (strlen(line) == 0) {
                continue; // skip empty lines
            }
            if (strncmp(line, "total ", 6) == 0) {
                continue; // skip the "total N" line from ls -l output
            }

            // Parse the line: format "perm links owner group size date name"
            // We extract the permission string and file size.
            char perms[16];
            long long size = 0;
            perms[0] = '\0';
            int items = sscanf(line, "%15s %*s %*s %*s %lld", perms, &size);
            if (items < 2) {
                fprintf(stderr, "Warning: Unrecognized line format, skipping: %s\n", line);
                continue;
            }

            // Count entry and categorize by type
            total_entries++;
            if (perms[0] == 'd') {
                dir_count++;
                // (We ignore directory size in total_file_size)
            } else {
                file_count++;
                total_file_size += size;
            }
        }

        // Reading done; clean up
        free(line);
        fclose(stream);

        gettimeofday(&end, NULL);
        // Calculate elapsed time
        double elapsed_sec = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

        // Wait for child process to finish and get exit status
        int status;
        waitpid(pid, &status, 0);
        if (WIFSIGNALED(status)) {
            fprintf(stderr, "Error: Child process terminated by signal %d\n", WTERMSIG(status));
            if (WTERMSIG(status) == SIGPIPE) {
                fprintf(stderr, "Broken pipe: Child received SIGPIPE (no reader)\n");
            }
        } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Error: Child process exited with status %d (ls command failed?)\n", WEXITSTATUS(status));
        }

        // Output the processed results
        printf("Total entries: %d (%d files, %d directories)\n", total_entries, file_count, dir_count);
        printf("Total size of files: %lld bytes\n", total_file_size);
        if (elapsed_sec < 1e-6) elapsed_sec = 1e-6; // avoid division by zero
        double throughput = (total_bytes / 1048576.0) / elapsed_sec;
        printf("Data read: %lld bytes in %.4f seconds (%.2f MB/s)\n", total_bytes, elapsed_sec, throughput);
    }

    return 0;
}
