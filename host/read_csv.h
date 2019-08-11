#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
// #include <stdio.h>
#include <errno.h>

int a2i(const char *s)
{
	int sign = 1;
	if (*s == '-')
		sign = -1;
	s++;
	int num = 0;
	while (*s)
	{
		num = ((*s) - '0') + num * 10;
		s++;
	}
	return num * sign;
}

struct csv_ro{
	int x, y, dirn, next_sig;
	bool track_ok;
};

struct data_ro{
	bool track_exists, track_ok;
	int dirn, next_sig;
};

struct data_ro* get_csv_arr(int mas_x, int mas_y){
	char cwd[1024];
   if (getcwd(cwd, sizeof(cwd)) != NULL){
       // fprintf(stdout, "Current working dir: %s\n", cwd);
   }
   else
       perror("getcwd() error");


	// printf("%d  %d\n",mas_x, mas_y);
	FILE *stream = fopen("1.csv", "r");
	if (stream == NULL){
		// printf("Null file\n");
	}
	else{
		// printf("not null file\n");
	}
	char line[1024];
	// struct csv_ro* resc = malloc(8 * sizeof(struct csv_ro));
	struct data_ro* resd = malloc(8 * sizeof(struct data_ro));
	memset(resd, 0, 8 * sizeof(struct data_ro));
	bool filled[8] ={false};
	for(int i = 0; i < 8; i++){
		filled[i] = false;
	}
	// printf("blaba\n");
	int cnt = 0;
	while (fgets(line, 1024, stream))
	{
		// printf("epoch: %d", cnt);
		cnt += 1;
		char tmp[1024];
		strcpy(tmp, line);
		struct data_ro temp_data;
		struct csv_ro temp_csv;
		int temp_int;
		sscanf(tmp, "%d,%d,%d,%d,%d", &temp_csv.x, &temp_csv.y, &temp_csv.dirn, &temp_int, &temp_csv.next_sig);
		// printf("x, y, dirn, temp_int, next_sig: %d,%d,%d,%d,%d\n", temp_csv.x, temp_csv.y, temp_csv.dirn, temp_int, temp_csv.next_sig);
		// free(tmp);
		if (temp_csv.x != mas_x || temp_csv.y != mas_y){
			continue;
		}
		temp_csv.track_ok = temp_int;

		temp_data.track_exists = true;
		temp_data.track_ok = temp_csv.track_ok;
		temp_data.dirn = temp_csv.dirn;
		temp_data.next_sig = temp_csv.next_sig;
		resd[temp_data.dirn] = temp_data;
		filled[temp_data.dirn] = true;
	}
	// printf("Out of while\n");
	for(int i = 0; i < 8; i++){
		if (filled[i]){
			continue;
		}
		struct data_ro temp_data;
		temp_data.track_exists = false;
		temp_data.dirn = i;
		temp_data.track_ok = false;
		temp_data.next_sig = 0;
		resd[i] = temp_data;
	}
	return resd;
}