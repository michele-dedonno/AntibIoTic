#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <linux/limits.h>
#include <fcntl.h>

/*
 * This program replicates the scanning function of the sanitizer module of AntibIoTic and can be used to test its effectivenes.
 * It search for the pattern "tofind" in the binary of the process whose PID is given as input.
 * 
 * Usage: ./scan-file <PID> 
 * */


// Pattern to find in the file
static const char tofind[] = "\x0C\x43\x4C\x4B\x4F\x47\x22";

// local function
static int scan_file(char *);


/* program main */
int main(int argc, char**argv)
{

	if (argc != 2) 
	{
		printf("[%s]> Error: Invalid number of arguments. Usage: %s <PID>\n",argv[0],argv[0]);
		return 1;
	}
	
	char exe_path[64] = {0};
	strcpy(exe_path, "/proc/");
	strcat(exe_path, argv[1]);
	strcat(exe_path, "/exe");

	printf("[%s]> Scanning file %s...\n",argv[0],exe_path);
	if(scan_file(exe_path) != 0) 
	{
		printf("[%s]> Pattern '%s' found in process %s.\n",argv[0], tofind, argv[1]);
//          printf("[Test-Scan] Memory scan match for binary %s\n", exe_path);
//          BOOL success = TRUE;
//            if (kill(pid, 9) == -1) {
//                  printf("[sanitizer] Failed to kill %d, errno %d\n", pid, errno);
//	            success = FALSE;
//               }
	        int pid= atoi(argv[1]);
		if(kill(pid, 9) == -1)
			printf("[%s]> Failed to kill process.\n",argv[0]);
		else
			printf("[%s]> Proccess killed.\n",argv[0]);
	}
	return 0;
}

static int scan_file(char *path) 
{
  int matched_pattern = 0;

  // Open file to scan
  int fd = open(path, O_RDONLY);
  if (fd == -1) 
  {
      // Note that if the program is not running with enough privileges this
      // will return error 13 (permission denied) on many files
      printf("[scan_file] Failed to call open(), errno %d\n", errno);
  } else 
  {
      // Get file size and rewind file descriptor back to start of file
      int filesize = lseek(fd, 0, SEEK_END);
      if (lseek(fd, 0, SEEK_SET) == -1 || filesize == -1)
      {
        printf("[scan_file] Failed to call lseek(), errno %d\n", errno);
      } else 
      {
        	// Loop until a pattern is matched or the whole file has been scanned
		char buffer[4096];
		int bytes_read;
		while (matched_pattern == 0) 
		{
		  // Read part of file into memory
		  if ((bytes_read = read(fd, buffer, sizeof(buffer))) <= 0) 
		  {
		    if (bytes_read == -1)
		      printf("[sanitizer] Failed to call read(), errno %d\n", errno);
		    break;
		  }

		  // Match pattern against buffer
		  if (memmem(buffer, bytes_read, tofind, strlen(tofind)) != NULL) 
		  {
		      matched_pattern = 1;
		      break;
		  }
		 }
     	}
	close(fd);
  }
  return matched_pattern;
}
