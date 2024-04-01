#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <malloc.h>

static int quiet = 0;
static int samba = 0;

void usage(char* me) {
	printf("Usage: %s [-q] [-s] fromdir [destdir]\n", me);
}

void replace_in(char* str, char from, char to) {
	char *i = str;
	while((i = strchr(i, from)) != NULL)
		*i++ = to;

}

char* destname(char* name) {
	char *res;
	char *end;
	char *ext_start;
	if (samba == 0)
		return name;

	res = strdup(name);
	if (samba == 1) {
		replace_in(res, '<', '-');
		replace_in(res, '>', '-');
		replace_in(res, ':', '-');
		replace_in(res, '"', '-');
		replace_in(res, '\\', '-');
		replace_in(res, '|', '-');
		replace_in(res, '?', ' ');
		replace_in(res, '*', '-');
		ext_start = strrchr(res, '.');
		if (ext_start == NULL) {
			end = res + strlen(res);
			while (isspace(*--end));
			*(end+1) = '\0';
		} else {
			end = ext_start;
			while (isspace(*--end));
			strcpy(end+1, ext_start);
		}

	}
	return res;
}

char* join(char* path, char* name) {
	char *res = malloc(strlen(path) + 1 + strlen(name) + 1);
	strcpy(res, path);
	strcat(res, "/");
	strcat(res, name);
	return res;
}

int run(char *from, char *dest) {
	DIR *src;
	struct dirent *e;
	char *d_name;
	char *s_err;
	char *src_path;
	char *dst_path;

	src = opendir(from);
	if (!src) {
		if (asprintf(&s_err, "Unable to open source directory %s", from) == -1) {
			printf("Internal error (asprintf)\n");
		} else {
			perror(s_err);
		}
		free(s_err);
		return 1;
	}

	if (quiet == 0) {
		printf("%s\n", from);
	}

	while (e = readdir(src)) {
		if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
			continue;
		d_name = destname(e->d_name);
		src_path = join(from, e->d_name);
		dst_path = join(dest, d_name);

		if (e->d_type == DT_DIR) {
			if ( mkdir(dst_path, 0755) != 0) {
				if (asprintf(&s_err, "Unable to create directory %s", dst_path) == -1) {
					printf("Internal error (asprintf)\n");
				} else {
					perror(s_err);
				}
				free(s_err);
			}
			run(src_path, dst_path);
		} else if (e->d_type == DT_REG || e->d_type == DT_LNK) {
			if (symlink(src_path, dst_path) != 0) {
				if (asprintf(&s_err, "Unable to create symlink %s", dst_path) == -1) {
					printf("Internal error (asprintf)\n");
				} else {
					perror(s_err);
				}
				free(s_err);
			}
		}
		free(d_name);
		free(src_path);
		free(dst_path);
	}
	closedir(src);
	return 0;
}

int main(int argc, char *argv[]) {
	int opt;
	char *from;
	char *dest = NULL;
	char *me = argv[0];
	struct stat st;

	while((opt = getopt(argc, argv, ":qs")) != -1) {
		switch(opt) {
			case 'q':
				quiet = 1;
				break;
			case 's':
				samba = 1;
				break;
			default:
				usage(me);
				return 1;
		}
	}

	if (optind == argc) {
		usage(me);
		return 1;
	}
	from = argv[optind++];
	if (optind < argc) {
		dest = argv[optind];
	} else {
		dest = ".";
	}

	// From
	if (lstat(from, &st) == -1) {
		perror("Unable to access source directory");
		return 1;
	}
	if (!S_ISDIR(st.st_mode)) {
		printf("%s is not a directory !\n", from);
		return 1;
	}

	// To
	if (lstat(dest, &st) == -1) {
		perror("Unable to access destination directory");
		return 1;
	}
	if (!S_ISDIR(st.st_mode)) {
		printf("%s is not a directory !\n", dest);
		return 1;
	}
	if (access(dest, W_OK) == -1) {
		printf("%s is not writeable !\n", dest);
		return 1;
	}
	return run(from, dest);
}
