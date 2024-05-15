#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "bst.h"

// Local functions
static int place_bst_node( struct BST_Node *parent, struct BST_Node *node );
static struct BST_Node *make_bst_node( int key, void *data );


// Root's pointer is passed because root can get modified for the first node
int bst_add_node( struct BST_Node **root, int key, void *data )
{
	struct BST_Node *newnode = NULL;
	struct BST_Node *parent = NULL;
	struct BST_Node *retnode = NULL;
	int status = 0;

	newnode = make_bst_node( key, data);
	if( *root == NULL ){
		*root = newnode;
		status = BST_SUCCESS;
	}
	else{
		status = place_bst_node( *root, newnode );
	}
	return status;
}

struct BST_Node *bst_search( struct BST_Node *root, int key )
{
	struct BST_Node *retval = NULL;

	if( root == NULL ){
		return NULL;
	}
	else if( root->key == key )
		return root;
	else if( key < root->key )
		return bst_search( root->left_child, key );
	else if( key > root->key )
		return bst_search( root->right_child, key );
}
void bst_print( struct BST_Node *root )
{
	if( root == NULL )
		return;
	else{
		printf("%d ", root->key);
		bst_print( root->left_child );
		bst_print( root->right_child );
	}
}

void bst_free( struct BST_Node *root )
{
	if( root == NULL )
		return;
	else{
		bst_free( root->left_child );
		bst_free( root->right_child );
		free(root);
	}
}

void bst_destroy( struct BST_Node *root )
{
	if( root == NULL )
		return;
	else{
		bst_free( root->left_child );
		bst_free( root->right_child );
		free(root->data);
		free(root);
	}
}

static int place_bst_node( struct BST_Node *parent, struct BST_Node *node )
{
	int retstatus;

	if( parent == NULL ){
		return BST_NULL;
	}
	else if( node->key == parent->key ){
		return BST_DUP_KEY;
	}
	else if( node->key < parent->key ){
		if( parent->left_child == NULL ){
			parent->left_child = node;
			return BST_SUCCESS;
		}
		else{
			return place_bst_node( parent->left_child, node );
		}
	}
	else if( node->key > parent->key ){
		if( parent->right_child == NULL ){
			parent->right_child = node;
			return BST_SUCCESS;
		}
		else{
			return place_bst_node( parent->right_child, node );
		}
	}
}

static struct BST_Node *make_bst_node( int key, void *data )
{
	struct BST_Node *newnode;
	newnode = (struct BST_Node *) malloc(sizeof(struct BST_Node));
	newnode->key = key;
	newnode->data = data;
	newnode->left_child = NULL;
	newnode->right_child = NULL;
	return newnode;
}

int get_shm_id(char * name) {
	key_t key = ftok(name, 'a');
	printf("name: %s\n", name);
	int shmid = shmget(key, MAX_NUM*sizeof(struct BST_Node), 0666 | IPC_CREAT);
	return shmid;
}

struct BST_Node * init_shared_bst(char * repo_name) {	// creates / gets shared memory
	int shmid = get_shm_id(repo_name);
	printf("shmid: %d\n", shmid);
	perror("shmid");
	struct BST_Node * root = shmat(shmid, (void *) 0, 0);
	return root;
}

int shmExists(char * repo_name) {
	key_t key = ftok(repo_name, 'a');
	int shmid = shmget(key, MAX_NUM*sizeof(struct BST_Node), 0666 | IPC_EXCL | IPC_CREAT);
	return shmid; 
}

void load_shared_bst(struct BST_Node *root, struct BST_Node *shared_bst, int *offset) {
    // store in preorder
	if (root == NULL) {
        return;
    }

    int current_offset = (*offset)++;
    shared_bst[current_offset] = *root;

    if (root->left_child) {
        shared_bst[current_offset].left_child = &shared_bst[*offset];
        load_shared_bst(root->left_child, shared_bst, offset);
    } else {
        shared_bst[current_offset].left_child = NULL;
    }

    if (root->right_child) {
        shared_bst[current_offset].right_child = &shared_bst[*offset];
        load_shared_bst(root->right_child, shared_bst, offset);
    } else {
        shared_bst[current_offset].right_child = NULL;
    }
}



