#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "bst.h"
#include "pds.h"

#define SIZE 1024


// checkin initialized values of pointers in shared memory


int main() {
    // initialising transactions.dat
    FILE * fp = fopen("transaction.dat", "wb");
    int n = 0;
    fwrite(&n,sizeof(int), 1,fp);
    fclose(fp);
    return 0;
}