#include "icssh.h"
#include "helpers.h"
#include "dlinkedlist.h"
#include <readline/readline.h>
#include <signal.h>


volatile sig_atomic_t color = 31;
int last_exit_status = -1;
volatile sig_atomic_t condflag = 0;
volatile sig_atomic_t usr2flag = 0;


int main(int argc, char* argv[]) {
    int max_bgprocs = -1;
	int exec_result;
	int exit_status;
	pid_t pid;
	pid_t wait_result;
	char* line;
	dlist_t* bgJobList = CreateList(&compareBG, &printBG, &deleteBG);
	int in = dup(STDIN_FILENO);
	int out = dup(STDOUT_FILENO);
	int err = dup(STDERR_FILENO);
#ifdef GS  // This compilation flag is for grading purposes. DO NOT REMOVE
    rl_outstream = fopen("/dev/null", "w");
#endif

    // check command line arg
    if(argc > 1) {
        int check = atoi(argv[1]);
        if(check != 0)
            max_bgprocs = check;
        else {
            printf("Invalid command line argument value\n");
            exit(EXIT_FAILURE);
        }
    }

	// Setup segmentation fault handler
	if (signal(SIGSEGV, sigsegv_handler) == SIG_ERR) {
		perror("Failed to set signal handler");
		exit(EXIT_FAILURE);
	}

	if(signal(SIGCHLD, sigchld_handler) == SIG_ERR ){
		perror("Failed to set signal handler");
		exit(EXIT_FAILURE);
	}

	if(signal(SIGUSR2, sigusr2_handler) == SIG_ERR ){
		perror("Failed to set signal handler");
		exit(EXIT_FAILURE);
	}

    	// print the prompt & wait for the user to enter commands string
	while ((line = readline(SHELL_PROMPT)) != NULL) {
        	// MAGIC HAPPENS! Command string is parsed into a job struct
        	// Will print out error message if command string is invalid
		    job_info* job = validate_input(line);
        	if (job == NULL) { // Command was empty string or invalid
			free(line);
			continue;
		}

        	//Prints out the job linked list struture for debugging
        	#ifdef DEBUG   // If DEBUG flag removed in makefile, this will not longer print
            		debug_print_job(job);
        	#endif
			last_exit_status = WEXITSTATUS(exit_status);

		if(usr2flag){
			fprintf(stderr, "\x1B[0;%dmHi %s!\x1B[0m\n", color, getenv("USER"));
			if(color == 36){
				color = 31;
			}else{
				color++;
			}
			usr2flag = 0;
		}
		
		if(condflag){
			node_t* current = bgJobList->head;
			int indexToDelete = 0;
			while(current != NULL){
				indexToDelete++;
				bgentry_t* bgentry = (bgentry_t*)(current->data);
				pid_t bg_pid = bgentry->pid;
				current = current->next;
				if(waitpid(bg_pid, &exit_status, WNOHANG)  > 0){
					indexToDelete--;
					bgentry_t* temp = RemoveByIndex(bgJobList, indexToDelete);
					deleteBG(temp);
				}
			}
			condflag = 0;
			last_exit_status = WEXITSTATUS(exit_status);
			//reap all terminated background processes
		}

		if (job->in_file != NULL) {
			int fd_in = open(job->in_file, O_RDONLY);
			if (fd_in == -1) {
				fprintf(stderr, RD_ERR);
				free_job(job);
				free(line);
				continue;
			}
			dup2(fd_in, STDIN_FILENO);
			close(fd_in);
		}


		// Handle output redirection
		if (job->out_file != NULL) {
			int fd_out;
			if (job->append){
				fd_out = open(job->out_file, O_WRONLY | O_CREAT | O_APPEND);
			}else{
				fd_out = open(job->out_file, O_WRONLY | O_CREAT | O_TRUNC);
			}

			if (fd_out == -1) {
				fprintf(stderr, RD_ERR);
				free_job(job);
				free(line);
				continue;
			}
			dup2(fd_out, STDOUT_FILENO);
			close(fd_out);
		}

		// Handle error redirection
		proc_info* proc = job->procs;
		while (proc != NULL) {
			if(proc->err_file != NULL){
				int fd_err = open(proc->err_file, O_WRONLY | O_CREAT | O_TRUNC);
				if (fd_err == -1) {
					fprintf(stderr, RD_ERR);
					free_job(job);
					free(line);
					continue;
				}
				dup2(fd_err, STDERR_FILENO);
				close(fd_err);
			}
			proc = proc->next_proc;
		}
		
		// example built-in: exit
		if (strcmp(job->procs->cmd, "exit") == 0) {
			node_t* current = bgJobList->head;
			while(current != NULL){
				bgentry_t* bgentry = (bgentry_t*)(current->data);
				pid_t curPid = bgentry->pid;
				kill(curPid, SIGKILL);
				printf(BG_TERM, curPid, bgentry->job->line);
				current = current->next;
			}
			DeleteList(bgJobList);
			free(line);
			free_job(job);
			free(bgJobList);
            validate_input(NULL);   // calling validate_input with NULL will free the memory it has allocated
			return 0;
		}



		//cd command implementation
		if(strcmp(job->procs->cmd, "cd") == 0){
			char s[100];
			char* dir;
			if(job->procs->argc > 1){
				dir = job->procs->argv[1];
			}else{
				dir = getenv("HOME");;
			}
			if(chdir(dir)!=0){
				fprintf(stderr, DIR_ERR);
				free_job(job); 
				free(line);
				continue;
			}
			printf("%s\n", getcwd(s, 100));
			free_job(job);
			free(line);
			continue;
		}

		if(strcmp(job->procs->cmd, "estatus") == 0){
			if(last_exit_status == -1){
				printf("No child processes have been reaped yet.\n");
			}else{
				printf("%d\n", last_exit_status);
			}
			free_job(job);
			free(line);
			continue;
		}

		if(strcmp(job->procs->cmd, "ascii53") == 0){
			printf("                                             \n");
			printf("                                             \n");
			printf("JJJJJ.     .JJJJJ       ..+gNNg&,.     .JJJJ,\n");
			printf("MMMMM:     .MMMM#    .(MMMMMMMMMMMN,   dMMMM[\n");
			printf("MMMMM:     .MMMM#   .MMMMMMM HMMMMM@!  dMMMM[\n");
			printf("MMMMM:     .MMMM#  .MMMMM      .T      dMMMM[\n");
			printf("MMMMM:     .MMMM#  JMMMMF              dMMMM[\n");
			printf("MMMMM:     .MMMM#  dMMMM]              dMMMM[\n");
			printf("MMMMM]     .MMMM#  -MMMMN,      .,     dMMMM[\n");
			printf("(MMMMMN...gMMMMM/   UMMMMMN-..(MMMNx.  dMMMM[\n");
			printf(" ?MMMMMMMMMMMMMt     ?MMMMMMMMMMMMMD   dMMMM[\n");
			printf("   ?WMMMMMMMB=         (TMMMMMMMB=     dMMMM \n");
			printf("                                             \n");
			printf("                                             \n");
			free_job(job);
			free(line);
			continue;
		}

		if(strcmp(job->procs->cmd, "bglist") == 0){
			node_t* current = bgJobList->head;
			for(int i = 0; i < bgJobList->length; i++){
				print_bgentry(current->data);
				current = current->next;
			}
			free_job(job);
			free(line);
			continue;
		}

		if(strcmp(job->procs->cmd, "fg") == 0){
			if(job->procs->argc > 1){
				int cure_pid = atoi(job->procs->argv[1]);
				node_t* current = bgJobList->head;
				while(current != NULL){
					bgentry_t* bgentry = (bgentry_t*)current->data;
					if(bgentry->pid == cure_pid){
						waitpid(cure_pid, &exit_status, 0);
						printf("%s\n", bgentry->job->line);
						bgentry_t* temp = RemoveFromTail(bgJobList);
						deleteBG(temp);
						break;
					}
					current = current->next;
				}
				if(current == NULL){
					fprintf(stderr, PID_ERR);
				}
			}else{
				bgentry_t* bgentry = RemoveFromTail(bgJobList);
				waitpid(bgentry->pid, &exit_status, 0);
				printf("%s\n", bgentry->job->line);
				deleteBG(bgentry);
			}
			free(line);
			free_job(job);
			continue;
		}

		if(job->nproc == 1){
			// example of good error handling!
			// create the child proccess
			if ((pid = fork()) < 0) {
				perror("fork error");
				exit(EXIT_FAILURE);
			}
			if (pid == 0) {  //If zero, then it's the child process
				//get the first command in the job list to execute
				proc_info* proc = job->procs;
				exec_result = execvp(proc->cmd, proc->argv);
				if (exec_result < 0) {  //Error checking
					printf(EXEC_ERR, proc->cmd);
					
					// Cleaning up to make Valgrind happy 
					// (not technically necessary because resources will be released when reaped by parent)
					free_job(job);  
					free(line);
					validate_input(NULL);  // calling validate_input with NULL will free the memory it has allocated
					exit(EXIT_FAILURE);
				}
			} else {
				if(job->bg){
					if(max_bgprocs != -1 && max_bgprocs <= bgJobList->length){
						fprintf(stderr, BG_ERR);
						free_job(job);
						free(line);
						continue;
					}

					bgentry_t *newBGentry = (bgentry_t*)malloc(sizeof(bgentry_t));
					
					newBGentry->job = job;
					newBGentry->pid = pid;
					newBGentry->seconds = time(NULL);
					InsertInOrder(bgJobList, newBGentry);
					free(line);
					continue;
				}else{
					// As the parent, wait for the foreground job to finish
					wait_result = waitpid(pid, &exit_status, 0);
					if (wait_result < 0) {
						printf(WAIT_ERR);
						exit(EXIT_FAILURE);
					}
				}
			}
		}else if(job->nproc == 2){
			int p[2] = {0, 0};
			if(pipe(p) == -1){
				fprintf(stderr, PIPE_ERR);
				free_job(job);
				free(line);
				continue;
			}
			pid_t pid1 = fork();
			if (pid1 == 0){
				close(p[0]);
				dup2(p[1], STDOUT_FILENO);
				close(p[1]);
				exec_result = execvp(job->procs->cmd, job->procs->argv);
				if(exec_result < 0){
					printf(EXEC_ERR, job->procs->cmd);
					free_job(job);
					free(line);
					validate_input(NULL);
					exit(EXIT_FAILURE);
				}
			}else{
				pid_t pid2 = fork();
				if(pid2 == 0){
					dup2(p[0], STDIN_FILENO);
					close(p[0]);
					close(p[1]);
					exec_result = execvp(job->procs->next_proc->cmd, job->procs->next_proc->argv);
					if(exec_result < 0){
						printf(EXEC_ERR, job->procs->cmd);
						free_job(job);
						free(line);
						validate_input(NULL);
						exit(EXIT_FAILURE);
					}
				}else{
					close(p[0]);
					close(p[1]);
					waitpid(pid1, &exit_status, 0);
					waitpid(pid2, &exit_status, 0);
				}
			}
		}else if(job->nproc > 2){
			int p1[2] = {0, 0};
			int p2[2] = {0, 0};
			if(pipe(p1) == -1 || pipe(p2) == -1){
				fprintf(stderr, PIPE_ERR);
				free_job(job);
				free(line);
				continue;
			}
			pid_t pid1 = fork();
			if(pid1 == 0){
				close(p1[0]);
				dup2(p1[1], STDOUT_FILENO);
				close(p1[1]);
				close(p2[0]);
				close(p2[1]);
				exec_result = execvp(job->procs->cmd, job->procs->argv);
				if(exec_result < 0){
					printf(EXEC_ERR, job->procs->cmd);
					free_job(job);
					free(line);
					validate_input(NULL);
					exit(EXIT_FAILURE);
				}
			}else{
				pid_t pid2 = fork();
				if(pid2 == 0){
					dup2(p1[0], STDIN_FILENO);
					close(p1[0]);
					close(p1[1]);
					close(p2[0]);
					dup2(p2[1], STDOUT_FILENO);
					close(p2[1]);
					exec_result = execvp(job->procs->next_proc->cmd, job->procs->next_proc->argv);
					if(exec_result < 0){
						printf(EXEC_ERR, job->procs->next_proc->cmd);
						free_job(job);
						free(line);
						validate_input(NULL);
						exit(EXIT_FAILURE);
					}
				}else{
					pid_t pid3 = fork();
					if(pid3 == 0){
						close(p1[0]);
						close(p1[1]);
						dup2(p2[0], STDIN_FILENO);
						close(p2[0]);
						close(p2[1]);
						exec_result = execvp(job->procs->next_proc->next_proc->cmd, job->procs->next_proc->next_proc->argv);
						if(exec_result < 0){
							printf(EXEC_ERR, job->procs->next_proc->next_proc->cmd);
							free_job(job);
							free(line);
							validate_input(NULL);
							exit(EXIT_FAILURE);
						}
					}else{
						close(p1[0]);
						close(p1[1]);
						close(p2[0]);
						close(p2[1]);
						waitpid(pid1, &exit_status, 0);
						waitpid(pid2, &exit_status, 0);
						waitpid(pid3, &exit_status, 0);
					}
				}
			}
		}
		last_exit_status = WEXITSTATUS(exit_status);
		free_job(job);  // if a foreground job, we no longer need the data
		free(line);
		dup2(in, STDIN_FILENO);
		dup2(out, STDOUT_FILENO);
		dup2(err, STDERR_FILENO);
	}
	// calling validate_input with NULL will free the memory it has allocated
	validate_input(NULL);
	close(in);
	close(out);
	close(err);
#ifndef GS
	fclose(rl_outstream);
#endif
	return 0;
}
