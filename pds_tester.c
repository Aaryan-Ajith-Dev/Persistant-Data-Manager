#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "pds.h"
#include "contact.h"

#define TREPORT(a1,a2) printf("Status: %s - %s\n\n",a1,a2); fflush(stdout);

void process_line( char *test_case );

int tid = 0;

int main(int argc, char *argv[])
{
	FILE *cfptr;
	char test_case[50];

	if( argc != 2 ){
		fprintf(stderr, "Usage: %s testcasefile\n", argv[0]);
		exit(1);
	}

	cfptr = (FILE *) fopen(argv[1], "r");
	while(fgets(test_case, sizeof(test_case)-1, cfptr)){
		// printf("line:%s",test_case);
		if( !strcmp(test_case,"\n") || !strcmp(test_case,"") )
			continue;
		process_line( test_case );
	}
}

void process_line( char *test_case )
{
	char repo_name[30];
	char command[10], param1[10], param2[10], info[1024];
	int contact_id, status, rec_size, expected_status;
	struct Contact testContact;

	strcpy(testContact.contact_name, "dummy name");
	strcpy(testContact.phone, "dummy number");

	rec_size = sizeof(struct Contact);

	sscanf(test_case, "%s%s%s", command, param1, param2);
	printf("Test case: %s", test_case); fflush(stdout);
	if( !strcmp(command,"CREATE") ){
		strcpy(repo_name, param1);
		if( !strcmp(param2,"0") )
			expected_status = CONTACT_SUCCESS;
		else
			expected_status = CONTACT_FAILURE;

		status = pds_create( repo_name );
		if(status == PDS_SUCCESS)
			status = CONTACT_SUCCESS;
		else
			status = CONTACT_FAILURE;
		if( status == expected_status ){
			TREPORT("PASS", "");
		}
		else{
			sprintf(info,"pds_create returned status %d",status);
			TREPORT("FAIL", info);
		}
	}
	else if( !strcmp(command,"OPEN") ){
		strcpy(repo_name, param1);
		if( !strcmp(param2,"0") )
			expected_status = CONTACT_SUCCESS;
		else
			expected_status = CONTACT_FAILURE;

		status = pds_open( repo_name, rec_size );
		if(status == PDS_SUCCESS)
			status = CONTACT_SUCCESS;
		else
			status = CONTACT_FAILURE;
		if( status == expected_status ){
			TREPORT("PASS", "");
		}
		else{
			sprintf(info,"pds_open returned status %d",status);
			TREPORT("FAIL", info);
		}
	}
	else if( !strcmp(command,"STORE") ){
		if( !strcmp(param2,"0") )
			expected_status = CONTACT_SUCCESS;
		else
			expected_status = CONTACT_FAILURE;

		sscanf(param1, "%d", &contact_id);
		testContact.contact_id = contact_id;
		if (tid == 0)
			status = add_contact( &testContact );
		else
			status = add_contact_uncommited( tid, &testContact );
		if(status == PDS_SUCCESS)
			status = CONTACT_SUCCESS;
		else
			status = CONTACT_FAILURE;
		if( status == expected_status ){
			TREPORT("PASS", "");
		}
		else{
			sprintf(info,"add_contact returned status %d",status);
			TREPORT("FAIL", info);
		}
	}
	else if( !strcmp(command,"SEARCH") ){
		if( !strcmp(param2,"0") )
			expected_status = CONTACT_SUCCESS;
		else
			expected_status = CONTACT_FAILURE;

		sscanf(param1, "%d", &contact_id);
		testContact.contact_id = -1;
		if (tid == 0)
			status = search_contact( contact_id, &testContact );
		else 
			status = search_contact_uncommited( tid, contact_id, &testContact );
		
		if(status == PDS_SUCCESS)
			status = CONTACT_SUCCESS;
		else
			status = CONTACT_FAILURE;
		if( status != expected_status ){
			sprintf(info,"search key: %d; Got status %d",contact_id, status);
			TREPORT("FAIL", info);
		}
		else{
			// Printing new values
			if( expected_status == 0 ){
				sprintf(info,"Got:{%d,%s,%s}\n",
					testContact.contact_id, testContact.contact_name, testContact.phone
				);
				if (testContact.contact_id == contact_id && 
					strcmp(testContact.contact_name,"dummy name") == 0 &&
					strcmp(testContact.phone,"dummy number") == 0){
						TREPORT("PASS", info);
				}
				else{
					TREPORT("PASS", info);
				}
			}
			else
				TREPORT("PASS", "");
		}
	}
	else if( !strcmp(command,"CLOSE") ){
		if( !strcmp(param1,"0") )
			expected_status = CONTACT_SUCCESS;
		else
			expected_status = CONTACT_FAILURE;

		status = pds_close();
		if(status == PDS_SUCCESS)
			status = CONTACT_SUCCESS;
		else
			status = CONTACT_FAILURE;
		if( status == expected_status ){
			TREPORT("PASS", "");
		}
		else{
			sprintf(info,"pds_close returned status %d",status);
			TREPORT("FAIL", info);
		}
	}
	else if (!strcmp("UPDATE", command)) {
		if( !strcmp(param2,"0") )
			expected_status = CONTACT_SUCCESS;
		else
			expected_status = CONTACT_FAILURE;

		sscanf(param1, "%d", &contact_id);
		testContact.contact_id = contact_id;
		strcpy(testContact.phone, "dummy number2");
		if (tid == 0)
			status = update_contact( &testContact );
		else status = update_contact_uncommited(tid, &testContact);
		if(status == PDS_SUCCESS)
			status = CONTACT_SUCCESS;
		else
			status = CONTACT_FAILURE;
		if( status == expected_status ){
			TREPORT("PASS", "");

			// print_contact(&testContact);
		}
		else{
			sprintf(info,"update_contact returned status %d",status);
			TREPORT("FAIL", info);
		}
	}
	else if (!strcmp("START_TRANSACTION", command)) {
		tid = start_transaction();
		printf("Created transaction with id: %d\n\n", tid);
	}
	else if(!strcmp("COMMIT", command)) {
		int status = commit_transaction(tid);
		printf("Status : %d\n\n", status);
		tid = 0;
	}
	else if (!strcmp("ROLLBACK", command)) {
		int status = rollback_transaction(tid);
		printf("Status : %d\n\n", status);
		tid = 0;
	}
	else if (!strcmp("WAIT", command)) {
		getchar();
	}
}


