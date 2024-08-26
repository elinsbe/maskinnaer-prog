#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_SIZE 30

char no_space_string[MAX_SIZE];

void last_index(char *input)
{
    int string_length = 0;
    for (int i = 0; i < strlen(input); i++)
    {
        if (input[i] != ' ')
        {
            (no_space_string)[string_length] = tolower((input)[i]);
            string_length++;
        }
    }
}

int check_palindrome(char *string_checked)
{
    int length = strlen(string_checked);
    int pivot = length / 2;
    if (length % 2 != 0)
    {
        pivot = ((length - 1) / 2) - 1;
    }

    for (int i = 0; i < pivot; i++)
    {
        if (string_checked[i] != string_checked[length - i - 1])
        {
            return -1;
        }
    }
    return 0;
}

int main()
{
    char input[MAX_SIZE];
    printf("Write word for palindrome checker: \n");
    scanf("%10[0-9a-zA-Z ]", input);
    last_index(input);
    // NÅ HAR VI IKKE SPACES
    if (check_palindrome(no_space_string) == 0)
    {
        printf("PALINDROME! \n");
    }
    else
    {
        printf("NUHUN\n");
    }
}