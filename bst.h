// not allowing multiple users coz of in-memmory ndx...

#ifndef BST_H
#define BST_H

#define MAX_NUM 1024 // maximum number of bst nodes

struct BST_Node{
	struct BST_Node *left_child;
	struct BST_Node *right_child;
	int key;
	void *data;
};

// Returns BST_Node of the node who key matches the search_key
// Returns NULL if search_key is not found in the BST
struct BST_Node *bst_search( struct BST_Node *root, int search_key );

// Adds the given <key, data> pair to the BST
// Returns BST_SUCCESS upon success, BST_DUP_KEY if duplicate key is being sent
// Note that the ADDRESS of the root (instead of root) should be sent as first paraemter
// For example:
// BST_Node *root;
// int ststus = bst_add_node (&root, somekey, somedata);

int bst_add_node( struct BST_Node **root, int key, void *data );

// Prints the keys of the BST
void bst_print( struct BST_Node *root );

// Releases the entire BST by recursively calling free()
void bst_free( struct BST_Node *root );

// Frees not only the nodes but also the data in the nodes
void bst_destroy( struct BST_Node *root );

void load_shared_bst( struct BST_Node *root, struct BST_Node * shared_bst , int * offset);

struct BST_Node * init_shared_bst(char *);

int get_shm_id(char *);

int shmExists(char *);

#define BST_SUCCESS 0
#define BST_DUP_KEY 1
#define BST_NULL 2

#endif
