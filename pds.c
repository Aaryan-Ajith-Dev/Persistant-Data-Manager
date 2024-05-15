// implemented multi version transaction control for record (.dat) file for a single user.

#include <stdio.h>
#include "pds.h"
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// Global structure to hold repository information
struct PDS_RepoInfo repo_handle;


// Function to create a new repository
int pds_create(char *repo_name){	// create data and index file "repo_name.ndx" for index file
	char name[30];
	strcpy(name, repo_name);
	// Create data file
	FILE *fp1 = fopen(strcat(name, ".dat"), "wb");
	if (fp1 == NULL)
		return PDS_FILE_ERROR;
	strcpy(name, repo_name);
    // Create index file
	FILE *fp2 = fopen(strcat(name, ".ndx"), "wb");
	if (fp2 == NULL)
		return PDS_FILE_ERROR;

    // Initialize record count to 0 in index file before reading
	int n = 0;
	if (fwrite(&n, sizeof(int), 1, fp2)!= 1)
		return PDS_FILE_ERROR;
	repo_handle.rec_count = n;
	fclose(fp1);
	fclose(fp2);
	return PDS_SUCCESS;
}

// Function to open an existing repository
int pds_open( char *repo_name, int rec_size ){	// 1st 4 bytes of index file is numebr of records || load the entire DS to the array
	char name[30];
	char prev_name[30];
	strcpy(name, repo_name);
	strcpy(prev_name, repo_handle.pds_name);
    // Check if the repository is already open
	if (repo_handle.repo_status == PDS_REPO_OPEN && strcmp(name, strcat(prev_name, ".dat")) == 0) {
		strcpy(repo_handle.pds_name, repo_name);
		return PDS_REPO_ALREADY_OPEN;
	}

	FILE *fp = fopen(strcat(name, ".dat"), "rb+");

	if (fp == NULL)
		return PDS_FILE_ERROR;

	repo_handle.pds_data_fp = fp;
    strcpy(repo_handle.pds_name, repo_name);
    repo_handle.rec_size = rec_size;
    repo_handle.repo_status = PDS_REPO_OPEN;

	if (pds_load_ndx())
		return PDS_FILE_ERROR;

	return PDS_SUCCESS;
}
// pds_load_ndx - Internal function
// Load the index entries into the BST ndx_root by calling bst_add_node repeatedly for each 
// index entry. Unlike array, for BST, you need read the index file one by one in a loop
int pds_load_ndx(){
	char name[30];
	strcpy(name, repo_handle.pds_name);
	FILE *fp = fopen(strcat(name, ".ndx"), "rb+");
	if (fp == NULL) return PDS_FILE_ERROR;
	fseek(fp,0,SEEK_SET);
	int n;
	if(fread(&n, sizeof(int), 1, fp)!=1)
		return PDS_FILE_ERROR;
	
	repo_handle.rec_count = n;
	for (int i = 0; i < n; i++){
		struct PDS_NdxInfo info;
		if (!fread(&info, sizeof(struct PDS_NdxInfo), 1, fp)) {
			fclose(fp);
			return PDS_FILE_ERROR;
		}
		struct Offset * offset = malloc(sizeof(struct Offset));
		offset->commited = info.offset;
		offset->uncommited = NO_OFFSET;
		bst_add_node( &repo_handle.ndx_root, info.key, offset);
	}

	fclose(fp);
	return PDS_SUCCESS;
}

// Function to add a record to the repository with a specified key
int put_rec_by_key( int key, void *rec ){
    if (repo_handle.repo_status != PDS_REPO_OPEN) return PDS_REPO_NOT_OPEN;
    FILE *fp = repo_handle.pds_data_fp;
    fseek(fp,0,SEEK_END);
	struct Offset * offset = malloc(sizeof(struct Offset));
	offset->commited = ftell(fp);
	offset->uncommited = NO_OFFSET;
	if (bst_add_node(&repo_handle.ndx_root, key, offset)){
		free(offset);
		return PDS_ADD_FAILED;
	}

	struct flock fl;
	fl.l_len = repo_handle.rec_size;
	fl.l_start = 0;
	fl.l_whence = SEEK_END;
	fl.l_type = F_WRLCK;
	fcntl(fileno(fp), F_SETLKW, &fl);

	int myKey = key;
	int version = 0;

	int writeCount = fwrite(&myKey, sizeof(int), 1, fp);
	if (writeCount != 1) return PDS_ADD_FAILED;
	writeCount = fwrite(&version, sizeof(int), 1, fp);
	if (writeCount != 1) return PDS_ADD_FAILED;
    writeCount = fwrite(rec, repo_handle.rec_size, 1, fp);
    if (writeCount != 1) return PDS_ADD_FAILED;

	fl.l_type = F_UNLCK;
	fcntl(fileno(fp), F_SETLKW, &fl);

	repo_handle.rec_count++;
	// printf("Rec count %d\n", repo_handle.rec_count);
    return PDS_SUCCESS;
}

// Function to update a record in the repository with a specified key
// Implementation similar to put_rec_by_key
int update_rec_by_key(int key, void *rec) {
    if (repo_handle.repo_status != PDS_REPO_OPEN) return PDS_REPO_NOT_OPEN;
    FILE *fp = repo_handle.pds_data_fp;
	struct Offset * offset;

	struct BST_Node *node = bst_search(repo_handle.ndx_root, key);
	if (node == NULL || node->data == NULL) return PDS_UPDATE_FAILED;

	offset = node->data;
	if (offset->commited == NO_OFFSET) return PDS_UPDATE_FAILED;
	struct flock fl;
	fl.l_len = repo_handle.rec_size;
	fl.l_start = offset->commited;
	fl.l_whence = SEEK_SET;
	fl.l_type = F_WRLCK;
	fcntl(fileno(fp), F_SETLKW, &fl);

	int myKey = key;
	int version = 0;
	fseek(fp, offset->commited, SEEK_SET);
	int writeCount = fwrite(&myKey, sizeof(int), 1, fp);
	if (writeCount != 1) return PDS_UPDATE_FAILED;
	writeCount = fwrite(&version, sizeof(int), 1, fp);
	if (writeCount != 1) return PDS_UPDATE_FAILED;
    writeCount = fwrite(rec, repo_handle.rec_size, 1, fp);
    if (writeCount != 1) return PDS_UPDATE_FAILED;

	fl.l_type = F_UNLCK;
	fcntl(fileno(fp), F_SETLKW, &fl);
    return PDS_SUCCESS;
}


// Function to update a record in the repository with a specified key uncommittedly
int update_rec_by_key_uncommited(int tid, int key, void *rec) {
// Implementation similar to update_rec_by_key
	if (repo_handle.repo_status != PDS_REPO_OPEN) return PDS_REPO_NOT_OPEN;
    FILE *fp = repo_handle.pds_data_fp;
	struct Offset * offset;

	struct BST_Node *node = bst_search(repo_handle.ndx_root, key);
	if (node == NULL || node->data == NULL) return PDS_UPDATE_FAILED;

	offset = node->data;
	if (offset->uncommited == NO_OFFSET) {	// no new version of dataitem
		struct flock fl;
		fl.l_len = repo_handle.rec_size;
		fl.l_start = 0;
		fl.l_whence = SEEK_END;
		fl.l_type = F_WRLCK;
		fcntl(fileno(fp), F_SETLKW, &fl);
		
		int myKey = key;
		int version = tid;
		fseek(fp, 0, SEEK_END);
		offset->uncommited = ftell(fp);
		
		int writeCount = fwrite(&myKey, sizeof(int), 1, fp);
		if (writeCount != 1) return PDS_ADD_FAILED;
		writeCount = fwrite(&version, sizeof(int), 1, fp);
		if (writeCount != 1) return PDS_ADD_FAILED;
		writeCount = fwrite(rec, repo_handle.rec_size, 1, fp);
		if (writeCount != 1) return PDS_ADD_FAILED;

		fl.l_type = F_UNLCK;
		fcntl(fileno(fp), F_SETLKW, &fl);

		repo_handle.rec_count++;
	}
	else {
		struct flock fl;
		fl.l_len = repo_handle.rec_size;
		fl.l_start = offset->commited;
		fl.l_whence = SEEK_SET;
		fl.l_type = F_WRLCK;
		fcntl(fileno(fp), F_SETLKW, &fl);

		int myKey = key;
		int version = tid;
		fseek(fp, offset->uncommited, SEEK_SET);
		int writeCount = fwrite(&myKey, sizeof(int), 1, fp);
		if (writeCount != 1) return PDS_UPDATE_FAILED;
		writeCount = fwrite(&version, sizeof(int), 1, fp);
		if (writeCount != 1) return PDS_UPDATE_FAILED;
		writeCount = fwrite(rec, repo_handle.rec_size, 1, fp);
		if (writeCount != 1) return PDS_UPDATE_FAILED;

		fl.l_type = F_UNLCK;
		fcntl(fileno(fp), F_SETLKW, &fl);
	}

    return PDS_SUCCESS;
}

// Function to add a record to the repository with a specified key uncommittedly
int put_rec_by_key_uncommited( int tid, int key, void * rec ) {
    // Implementation similar to put_rec_by_key
	// printf("INSODE UNCOMMITED\n");
    if (repo_handle.repo_status != PDS_REPO_OPEN) return PDS_REPO_NOT_OPEN;
    FILE *fp = repo_handle.pds_data_fp;
    fseek(fp,0,SEEK_END);
	struct Offset * offset = malloc(sizeof(struct Offset));
	offset->commited = NO_OFFSET;
	offset->uncommited = ftell(fp);
	if (bst_add_node(&repo_handle.ndx_root, key, offset)){
		free(offset);
		return PDS_ADD_FAILED;
	}

	struct flock fl;
	fl.l_len = repo_handle.rec_size;
	fl.l_start = 0;
	fl.l_whence = SEEK_END;
	fl.l_type = F_WRLCK;
	fcntl(fileno(fp), F_SETLKW, &fl);

	int myKey = key;
	int version = tid;

	int writeCount = fwrite(&myKey, sizeof(int), 1, fp);
	if (writeCount != 1) return PDS_ADD_FAILED;
	writeCount = fwrite(&version, sizeof(int), 1, fp);
	if (writeCount != 1) return PDS_ADD_FAILED;
    writeCount = fwrite(rec, repo_handle.rec_size, 1, fp);
    if (writeCount != 1) return PDS_ADD_FAILED;

	fl.l_type = F_UNLCK;
	fcntl(fileno(fp), F_SETLKW, &fl);

	repo_handle.rec_count++;
    return PDS_SUCCESS;
}

// Function to retrieve a record from the repository by key
int get_rec_by_key( int key, void *rec ){

	if (repo_handle.repo_status != PDS_REPO_OPEN) return PDS_REPO_NOT_OPEN;

    FILE *fp = repo_handle.pds_data_fp;

	struct Offset *offset = NULL;
	int n = repo_handle.rec_count;

	struct BST_Node *node = bst_search(repo_handle.ndx_root, key);

	if (node == NULL) return PDS_REC_NOT_FOUND;
	offset = node->data;

	if (offset == NULL || offset->commited == NO_OFFSET) return PDS_REC_NOT_FOUND;
	fseek(fp, offset->commited, SEEK_SET);

	struct flock fl;
	fl.l_type = F_RDLCK;
	fl.l_len = repo_handle.rec_size;
	fl.l_start = offset->commited;
	fl.l_whence = SEEK_SET;

	int myKey, version;
	fcntl(fileno(fp), F_SETLKW, &fl);
	fread( &myKey, sizeof(int), 1, fp);
	fread( &version, sizeof(int), 1, fp);
	if (version != 0) return PDS_REC_NOT_FOUND;
	if (fread( rec ,repo_handle.rec_size, 1, fp) != 1) {
		rec = NULL;
		fl.l_type = F_UNLCK;
		fcntl(fileno(fp), F_SETLKW, &fl);
		return PDS_REC_NOT_FOUND;
	}

	fl.l_type = F_UNLCK;
	fcntl(fileno(fp), F_SETLKW, &fl);
	return PDS_SUCCESS;
}


// Function to retrieve a record from the repository by key uncommittedly
int get_rec_by_key_uncommited( int tid, int key, void *rec ){

	if (repo_handle.repo_status != PDS_REPO_OPEN) return PDS_REPO_NOT_OPEN;

    FILE *fp = repo_handle.pds_data_fp;

	struct Offset *offset = NULL;
	int n = repo_handle.rec_count;

	struct BST_Node *node = bst_search(repo_handle.ndx_root, key);

	if (node == NULL) return PDS_REC_NOT_FOUND;
	offset = node->data;

	if (offset == NULL) return PDS_REC_NOT_FOUND;
	int off = offset->uncommited == NO_OFFSET ? offset->commited : offset->uncommited;
	fseek(fp, off, SEEK_SET);

	struct flock fl;
	fl.l_type = F_RDLCK;
	fl.l_len = repo_handle.rec_size;
	fl.l_start = off;
	fl.l_whence = SEEK_SET;

	int myKey, version;
	fcntl(fileno(fp), F_SETLKW, &fl);
	fread( &myKey, sizeof(int), 1, fp);
	fread( &version, sizeof(int), 1, fp);
	if (version != 0) return PDS_REC_NOT_FOUND;
	if (fread( rec ,repo_handle.rec_size, 1, fp) != 1) {
		rec = NULL;
		fl.l_type = F_UNLCK;
		fcntl(fileno(fp), F_SETLKW, &fl);
		return PDS_REC_NOT_FOUND;
	}

	fl.l_type = F_UNLCK;
	fcntl(fileno(fp), F_SETLKW, &fl);
	return PDS_SUCCESS;
}



// Function to write the current BST to the index file
int writeCurrentBST(struct BST_Node *root, FILE *fp) {
	if (!root) return PDS_SUCCESS;
	struct Offset *offset = root->data;
	struct PDS_NdxInfo s;
	s.key = root->key;
	s.offset = offset->commited;
	if (fwrite(&s, sizeof(struct PDS_NdxInfo), 1, fp) != 1) return PDS_FILE_ERROR;
	return writeCurrentBST(root->left_child, fp) || writeCurrentBST(root->right_child, fp);
}

// Function to close the repository
int pds_close()
{
    if (repo_handle.repo_status != PDS_REPO_OPEN)
        return PDS_NDX_SAVE_FAILED;
	
	char name[30];
	strcpy(name, repo_handle.pds_name);
	
	FILE *file = fopen(strcat(name,".ndx"),"wb");
	fseek(file,0,SEEK_SET);
	// printf("rec count: %d\n", repo_handle.rec_count);
	if (fwrite(&repo_handle.rec_count, sizeof(int), 1, file) != 1) {
		fclose(file);
    	fclose(repo_handle.pds_data_fp);
		repo_handle.repo_status = PDS_REPO_CLOSED;
		return PDS_NDX_SAVE_FAILED;
	}


	if (writeCurrentBST(repo_handle.ndx_root, file)){
		fclose(file);
    	fclose(repo_handle.pds_data_fp);
		return PDS_NDX_SAVE_FAILED;
	}
	
	fclose(file);
    fclose(repo_handle.pds_data_fp);
	// shmdt(repo_handle.ndx_root);
	// shmctl(get_shm_id(repo_handle.pds_name), IPC_RMID,NULL)
	bst_destroy(repo_handle.ndx_root);
    repo_handle.pds_data_fp = NULL;
    repo_handle.repo_status = PDS_REPO_CLOSED;
    return PDS_SUCCESS;

// Close the repo file
// Update file pointer and status in global struct
}

// Function to start a new transaction
int start_transaction() {
	if (repo_handle.repo_status != PDS_REPO_OPEN)
        return 1;
	FILE * fp = fopen("transaction.dat", "rb+");
	int tid;
	struct flock fl;
	fl.l_len = 0;
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fcntl(fileno(fp), F_SETLKW, &fl);

	fread(&tid, sizeof(int), 1 , fp);
	tid++;
	fseek(fp, 0, SEEK_SET);
	printf("Transaction Number: %d\n", tid);
	fwrite(&tid, sizeof(int), 1, fp);

	fl.l_type = F_UNLCK;
	fcntl(fileno(fp), F_SETLKW, &fl);
	fclose(fp);
	return tid;
}
// Function to commit a transaction

int commit_transaction(int tid) {
    if (repo_handle.repo_status != PDS_REPO_OPEN)
        return 1;

    FILE *fp = repo_handle.pds_data_fp;
    int key, version, status = 0;
    void *rec = malloc(repo_handle.rec_size);
    if (!rec) {
        perror("Error allocating memory for record");
        return 1;
    }

    struct flock fl;
    fl.l_len = 0;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;

    if (fcntl(fileno(fp), F_SETLKW, &fl) == -1) {
        perror("Error locking file");
        free(rec);
        return 1;
    }

    fseek(fp, 0, SEEK_SET);

    long position = 0;
    while (fread(&key, sizeof(int), 1, fp) == 1) {
        if (fread(&version, sizeof(int), 1, fp) != 1) break;
        if (fread(rec, repo_handle.rec_size, 1, fp) != 1) break;
		// printf("(key %d, version %d)\n", key, version);
        struct BST_Node *node = bst_search(repo_handle.ndx_root, key);
        if (node == NULL || node->data == NULL) {
            status = 1;
            break;
        }

        struct Offset *offset = (struct Offset *)node->data;
		// printf("--offset info: %d, %d\n", offset->commited, offset->uncommited);
        position = ftell(fp);
        if (offset->uncommited == -1 || ((offset->uncommited == (position - 2 * sizeof(int) - repo_handle.rec_size)) && (version == tid))) {
            if (offset->uncommited != -1) {
				offset->commited = offset->uncommited;
				offset->uncommited = -1;
			}
			version = 0;
        } else if (version == 0){
            version = -1;	// overriding the original
			repo_handle.rec_count--;
        }

        fseek(fp, position - repo_handle.rec_size - 2 * sizeof(int), SEEK_SET);
        fwrite(&key, sizeof(int), 1, fp);
        fwrite(&version, sizeof(int), 1, fp);
        fwrite(rec, repo_handle.rec_size, 1, fp);
        fseek(fp, position, SEEK_SET);
    }


    // Unlock the file
    fl.l_type = F_UNLCK;
    fcntl(fileno(fp), F_SETLKW, &fl);

    free(rec);
    return status;
}

// Function to rollback a transaction
int rollback_transaction(int tid) {

    if (repo_handle.repo_status != PDS_REPO_OPEN)
        return 1;

    FILE *fp = repo_handle.pds_data_fp;
    int key, version, status = 0;
    void *rec = malloc(repo_handle.rec_size);
    if (!rec) {
        perror("Error allocating memory for record");
        return 1;
    }

    struct flock fl;
    fl.l_len = 0;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;

    if (fcntl(fileno(fp), F_SETLKW, &fl) == -1) {
        perror("Error locking file");
        free(rec);
        return 1;
    }

    fseek(fp, 0, SEEK_SET);

    long position = 0;
    while (fread(&key, sizeof(int), 1, fp) == 1) {
        if (fread(&version, sizeof(int), 1, fp) != 1) break;
        if (fread(rec, repo_handle.rec_size, 1, fp) != 1) break;
		// printf("(key %d, version %d)\n", key, version);
		position = ftell(fp);
        struct BST_Node *node = bst_search(repo_handle.ndx_root, key);
        if (node == NULL || node->data == NULL) {
            status = 1;
            break;
        }

        struct Offset *offset = (struct Offset *)node->data;
		if (version == tid) {
			version = -1;
			repo_handle.rec_count--;
			offset->uncommited = -1;
		}

        fseek(fp, position - repo_handle.rec_size - 2 * sizeof(int), SEEK_SET);
        fwrite(&key, sizeof(int), 1, fp);
        fwrite(&version, sizeof(int), 1, fp);
        fwrite(rec, repo_handle.rec_size, 1, fp);
        fseek(fp, position, SEEK_SET);
    }

    // Unlock the file
    fl.l_type = F_UNLCK;
    fcntl(fileno(fp), F_SETLKW, &fl);

    free(rec);
    return status;
}
