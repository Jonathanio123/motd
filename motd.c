#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

//TODO: Revamp function
float pb(float total, float used, float size)
{
    float percent = used / total;
    float divider = 1 / size;

    printf("[");
    for (float i = 0; i < 1; i = i + divider)
    {
        if (i < percent)
        {
            printf("=");
        }
        else
        {
            printf("-");
        }
    }
    printf("]");
    fflush(stdout);

    return percent;
}

int main()
{

    char hostname[1024];
    gethostname(hostname, 1024);
    char hostCommand[1024] = "figlet ";

    system(strcat(hostCommand, hostname));

    FILE *fp;
    char os_str[1024];

    fp = fopen("/etc/os-release", "r");
    if (fp == NULL)
    {
        perror("Error opening file");
        return (-1);
    }
    fgets(os_str, sizeof(os_str), fp);
    fclose(fp);
    fp = NULL;

    printf("OS:         ");
    {
        char *token = strtok(os_str, "\"");
        token = strtok(NULL, "\"");
        printf("%s\n", token);
    }

    printf("Private IP: ");
    fflush(stdout);
    // TODO: Find a more elogant and consistant way to get ipv4 address.
    system("ip a | awk '/inet / && /global/ {split($2, arr, /\\//); print arr[1]}' | head -n 1");

    // Change YourHostname to your pcs hostname and Public ip to your domain name/public ip.
    // If not it wil get your public ip trough curl witch is quite a bit slower on first run.
    // It wil save it in /tmp/motd and read from it next time the script runs.
    if (strcmp(hostname, "YourHostname") == 0)
    {
        printf("Public IP:  YourPublicIp\n");
        fflush(stdout);
    }
    else
    {
        printf("Public IP:  ");
        fflush(stdout);
        char publicIPString[32];

        if (access("/tmp/motd", R_OK) == 0)
        {
            fp = fopen("/tmp/motd", "r");
            if (fp == NULL)
            {
                perror("Error opening file");
                return (-1);
            }

            fgets(publicIPString, sizeof(publicIPString), fp);
            fclose(fp);
            fp = NULL;

        } else {

        // Curl command increases running by 0.2s. Increasing total runtime from ~0.05 to ~0.205
        fp = popen("curl -m 2 --silent https://ipinfo.io/ip", "r");
        fgets(publicIPString, sizeof(publicIPString), fp);
        fclose(fp);

        fp = fopen("/tmp/motd", "w"); 
        if (fp == NULL)
        {
                perror("Error writing file motd to /tmp/motd");
                return (-1);
        }
        fputs(publicIPString, fp);
        fclose(fp);

        }

        printf("%s\n", publicIPString);

        
    }

    printf("Uptime:     ");
    fflush(stdout);
    system("uptime -p | awk '{print substr($0,4,length($0))}'");

    char mem_str[1024];
    float memTotal, memUsed, memFree;
    float swapTotal, swapUsed, swapFree;

    fp = fopen("/proc/meminfo", "r");
    if (fp == NULL)
    {
        perror("Error opening file");
        return (-1);
    }

    while (fgets(mem_str, sizeof(mem_str), fp))
    {
        char *token = strtok(mem_str, " ");

        if ((strcmp(token, "MemTotal:")) == 0)
        {
            token = strtok(NULL, " ");
            memTotal = atof(token);
            //printf("%.0f\n", memTotal);
        }
        else if ((strcmp(token, "MemAvailable:")) == 0)
        {
            token = strtok(NULL, " ");
            memFree = atof(token);
            //printf("%.0f\n", memFree);
        }
        else if ((strcmp(token, "SwapTotal:")) == 0)
        {
            token = strtok(NULL, " ");
            swapTotal = atof(token);
            //printf("%.0f\n", swapTotal);
        }
        else if ((strcmp(token, "SwapFree:")) == 0)
        {
            token = strtok(NULL, " ");
            swapFree = atof(token);
            //printf("%.0f\n", swapFree);

            break;
        }
    }
    fclose(fp);
    fp = NULL;
    memUsed = memTotal - memFree;
    swapUsed = swapTotal - swapFree;

    printf("Memory:     RAM - %.2fGB used, %.2fGB available\n            ",
           memUsed / pow(10, 6), memFree / pow(10, 6), memTotal / pow(10, 6));

    float permem = pb(memTotal, memUsed, 40);
    if (permem > 0.80)
    {
        printf("\n            SWAP - %.2fGB used, %.2fGB available\n            ",
               swapUsed / pow(10, 6), swapFree / pow(10, 6), swapTotal / pow(10, 6));
        pb(swapTotal, swapUsed, 40);
    }

    fp = popen("/bin/df -h", "r");
    if (fp != NULL)
    {
        char root_str[1035];
        float rootTotal, rootUsed, rootFree;

        while (fgets(root_str, sizeof(root_str), fp))
        {

            if (strstr(root_str, "/\n"))
            {
                char *token = strtok(root_str, " ");

                token = strtok(NULL, " ");
                for (int i = 0; i < 3; i++)
                {
                    if (i == 0)
                    {
                        rootTotal = atof(token);
                        //printf("\n%s, %i", token, i);
                    }
                    else if (i == 1)
                    {
                        rootUsed = atof(token);
                        //printf("\n%s, %i", token, i);
                    }
                    else
                    {
                        rootFree = atof(token);
                        //printf("\n%s, %i", token, i);
                    }
                    token = strtok(NULL, " ");
                }
                break;
            }
        }
        pclose(fp);
        fp = NULL;


        printf("\nDisk space: Root - %.1fGB used, %.1fGB free\n            ",
               rootUsed, rootFree);
        fflush(stdout);
        float rootPercent = pb(rootTotal, rootUsed, 40);

        if (rootPercent > 0.80)
        {
            printf("\n            Root partition nearing max capacity\n            %.2f%% used on root", rootPercent * pow(10, 2));
        }
    }

    /* Open the command for reading. */
    if (access("/bin/zpool", X_OK) == 0)
    {

        fp = popen("/bin/zpool list | awk 'FNR == 2 {print $2 $3 $4}'", "r");
        if (fp != NULL)
        {
            char storage_str[1035];
            float zfsTotal, zfsUsed, zfsFree;

            /* Read the output a line at a time - output it. */
            fgets(storage_str, sizeof(storage_str), fp);
            pclose(fp);
            fp = NULL;
            
            char *token = strtok(storage_str, "T");
            for (int i = 0; i < 3; i++)
            {
                if (i == 0)
                {
                    zfsTotal = atof(token);
                    // printf("\n%s, %i", token, i);
                }
                else if (i == 1)
                {
                    zfsUsed = atof(token);
                    // printf("\n%s, %i", token, i);
                }
                else
                {
                    zfsFree = atof(token);
                    // printf("\n%s, %i", token, i);
                }
                token = strtok(NULL, "T");
            }

            printf("\n            Storage - %.2fT used, %.2fT free\n            ",
                   zfsUsed, zfsFree);
            fflush(stdout);
            float zfsPercent = pb(zfsTotal, zfsUsed, 40);
            printf("\n");

            if (zfsPercent > 0.80)
            {
                printf("            Storage partition nearing max capacity\n            %.2f%% used on storage\n", zfsPercent * pow(10, 2));
            }
        }
    }

    // TODO: add services running and reports

    return 0;
}
