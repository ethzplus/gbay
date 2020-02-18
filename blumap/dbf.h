#ifndef __DBF__
#define __DBF__

#define MAX_DBF_FILENAME 128
#define MAX_LINE 256
#define MAX_ENTRIES 64
#define MAX_STATE_NAME 64

#define DBF_ERR -1

typedef struct {
	char* state_name;
	unsigned int value;
} DBF_line;

typedef struct {
	char* filename;
	char* nodename;
	unsigned int entries_n;
	DBF_line entries[MAX_ENTRIES];
} DBF;

int DBF_Open(char* filename, DBF* dbf);
void DBF_Print(DBF dbf);
void DBF_Free(DBF dbf);
int DBF_getValueforStateName(DBF dbf, char* state_name);

#endif