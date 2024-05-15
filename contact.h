#ifndef CONTACT_H
#define CONTACT_H

#define CONTACT_SUCCESS 0
#define CONTACT_FAILURE 1
 
struct Contact{
	int contact_id;
	char contact_name[30];
	char phone[15];
};

extern struct PDS_RepoInfo *repoHandle;

// Add the given contact into the repository by calling put_rec_by_key
int add_contact( struct Contact *c );

// Display contact info in a single line as a CSV without any spaces
void print_contact( struct Contact *c );

// Use get_rec_by_key function to retrieve contact
int search_contact( int contact_id, struct Contact *c );

// Load all the contacts from a CSV file
int store_contacts( char *contact_data_file );

int update_contact(struct Contact * c);

int add_contact_uncommited( int tid, struct Contact *c );

int search_contact_uncommited( int tid, int contact_id, struct Contact *c );

int update_contact_uncommited(int, struct Contact *);
#endif
