#include <stdio.h>

int main(int argc, char *argv[]) {
    FILE *fp = fopen("something.txt", "w");

    fputs("This is some text xxxx\n"
          "This is some more text", fp);
    fclose(fp);

    fp = fopen("something.txt", "r");

    char str[5];

    char *p1 = str;
    char *p2 = p1 + 2;
    fgets(str, 5, fp);
    puts(str);
    fclose(fp);

    return 0;

}