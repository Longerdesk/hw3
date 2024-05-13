// Your helper functions need to be here.
#include "helpers.h"
#include "dlinkedlist.h"
#include "icssh.h"

int compareBG(const void* data1, const void* data2){
    if(((bgentry_t*)data1)->seconds > ((bgentry_t*)data2)->seconds ){
        return -1;
    }else if(((bgentry_t*)data1)->seconds < ((bgentry_t*)data2)->seconds ){
        return 1;
    }else{
        return 0;
    }
}
void printBG(void* data1, void* data2){

}

void deleteBG(void* data){
    if(data != NULL){
        if(((bgentry_t*)data)->job != NULL){
            free_job(((bgentry_t*)data)->job);
        }
        free((bgentry_t*)data);
    }
}

void sigchld_handler(){
    int status;
    pid_t pid;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        condflag = 1;
        if(WIFEXITED(status)){
            //something
        }else if(WIFSIGNALED(status)){
            //something
        }
    }
}

void sigusr2_handler(){
    usr2flag = 1;
}
