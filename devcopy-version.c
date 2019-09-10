#include "devcopy.h"
#include "ktree.h"
#include "error.h"

int insert;
int newver;
int listver;
int verbose;


static void help(const char *prog)
{
    err_exit(EINVAL, "%s -v -i -n -l -f [hash-file] -c [change-file]",
             prog);
}

int main(int argc, char *argv[])
{
    int c;
    char *fchg, *fhash, *frollback;
    

    opterr = 0;

    while ((c = getopt(argc, argv, "inlf:c:v")) != EOF)
    {
        switch (c)
        {
            case 'i':
                /* Insert a child-version after current version */
                insert = 1;
                break;

            case 'n':
                newver = 1;
                break;

            case 'l':
                listver = 1;
                break;

            case 'f':
                fhash = optarg;
                break;

            case 'c':
                fchg = optarg;
                break;

            case 'v':
                verbose = 1;
                break;

            case '?':
                help(argv[0]);
        }
    }


}
