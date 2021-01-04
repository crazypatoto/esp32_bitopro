#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void removeNewLine(unsigned char *input)
{    
    for (size_t i = 0; i < strlen((char *)input); i++)
    {
        if (input[i] == '\n')
        {
            puts("\\n DETECTED!!");
            for (size_t j = i; j < strlen((char *)input); j++)
            {
                input[j] = input[j + 1];
            }               
        }
    }
}

int main()
{

    unsigned char test[] = "adcf7qwewq7";

    printf("%s", test);
    removeNewLine(test);    
    printf("%s", test);

    return 0;
}