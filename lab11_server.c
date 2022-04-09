// gcc -o msgqsrv lab11_server.c -lrt
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#define MSG_VAL_LEN  100
#define CLIENT_Q_NAME_LEN 100   // For the client queue message
#define MSG_TYPE_LEN 100        // For the server queue message

#define MIN_COURSES 1
#define MAX_COURSES 15
#define MIN_TEACHERS 1
#define MAX_TEACHERS 10

typedef struct
{

    char client_q[CLIENT_Q_NAME_LEN];
    char msg_val[MSG_VAL_LEN];

} client_msg_t;

typedef struct
{

    char msg_type[MSG_TYPE_LEN];
    char msg_val[MSG_VAL_LEN];

} server_msg_t;

struct mapping
{

    char *course_name;
    char *teacher_name;

} ;

struct teacher
{

    char *teacher_name;

};

int course_count = 0;
int teacher_count = 0;
struct mapping *pairs;
struct teacher *teachers;

int minCourses;
int maxCourses;
int minTeacher;
int maxTeacher;

static client_msg_t client_msg;

#define SERVER_QUEUE_NAME   "/server_msgq"
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE sizeof(client_msg_t)
#define MSG_BUFFER_SIZE (MAX_MSG_SIZE * MAX_MESSAGES)

void generateReport()
{
    printf("\n\n Generated Report\n\n");
    
    if(course_count !=0)
    {

        printf("\n Courses : Teachers\n\n");
        for(int i =0;i<maxCourses;i++)
        {
            if(!(strcmp(pairs[i].course_name,"NULL") == 0))
            {
                printf("   %s : %s\n",pairs[i].course_name,pairs[i].teacher_name);
            }
        }

    }
    
    printf("\n\n Teachers\n\n");
    
    if(teacher_count !=0)
    {

        for(int i =0;i<maxTeacher;i++)
        {
            if(!(strcmp(teachers[i].teacher_name,"NULL") == 0))
            {
                printf("   %s\n",teachers[i].teacher_name);
            }
        }
    }
}

void addTeacher(char *token)
{
    for(int i=0;i<maxTeacher;i++)
    {
        if(strcmp(teachers[i].teacher_name,"NULL") == 0)
        {
            strcpy(teachers[i].teacher_name,token);
            break;
        }
    }
}

void addCourse(char *token)
{
    for(int i=0;i<maxCourses;i++)
    {
        if(strcmp(pairs[i].course_name,"NULL") == 0)
        {
            strcpy(pairs[i].course_name,token);
            int random_int = rand()%teacher_count;
            int f=0;
            for(int j=0;j<maxTeacher;j++)
            {
                if(!(strcmp(teachers[j].teacher_name,"NULL") == 0))
                {
                    if(f == random_int)
                    {
                        strcpy(pairs[i].teacher_name,teachers[j].teacher_name);
                        break;
                    }
                    f++;
                }
            }
            break;
        }
    }
}

void deleteCourse(char *token)
{
    for(int i=0;i<maxCourses;i++)
    {
        if(strcmp(pairs[i].course_name,token) == 0)
        {
            strcpy(pairs[i].course_name,"NULL");
            strcpy(pairs[i].teacher_name,"NULL");
            break;
        }
    }
}

void deleteTeacher(char *token)
{
    for(int i=0;i<maxCourses;i++)
    {
        if(strcmp(pairs[i].teacher_name,token) == 0)
        {
            strcpy(pairs[i].course_name,"NULL");
            strcpy(pairs[i].teacher_name,"NULL");
        }
    }
    
    for(int i=0;i<maxCourses;i++)
    {
        if(strcmp(teachers[i].teacher_name,token) == 0)
        {
            strcpy(teachers[i].teacher_name,"NULL");
            break;
        }
    }
}

void update(client_msg_t out_msg, char * result)
{
    if(out_msg.msg_val[2] != ' ')
    {
        strcpy(result,"FAILED\n");
        return;
    }
    else
    {
        out_msg.msg_val[2] = ',';
    }

    int count = 0;
    for(int i = 0; i < strlen(out_msg.msg_val); i++)
    {
        if(out_msg.msg_val[i] == ' ')  
            out_msg.msg_val[i] = '_';
        if(out_msg.msg_val[i] == ',')  
            count++;
    }
    count--;
    if(out_msg.msg_val[0] == 'A' && out_msg.msg_val[1] == 'T')
    {
        if(count+teacher_count+1 > maxTeacher)
        {
            strcpy(result,"OVERFLOW\n");
            return;            
        }

        char * token = strtok(out_msg.msg_val, ",");// initial part needs to be ignored e.g. AT

        token = strtok(NULL, ",");
        while( token != NULL )
        {
            addTeacher(token);
            teacher_count++;
            token = strtok(NULL, ",");
        }

        strcpy(result,"DONE\n");
    }
    else if(out_msg.msg_val[0] == 'A' && out_msg.msg_val[1] == 'C')
    {
        if(count+course_count+1 > maxCourses)
        {
            strcpy(result,"OVERFLOW\n");
            return;
        }

        char * token = strtok(out_msg.msg_val, ",");// initial part needs to be ignored e.g. AT

        token = strtok(NULL, ",");
        while( token != NULL )
        {
            addCourse(token);
            course_count++;
            token = strtok(NULL, ",");
        }

        strcpy(result,"DONE\n");
    }
    else if(out_msg.msg_val[0] == 'D' && out_msg.msg_val[1] == 'T')
    {
        printf("%d %d",teacher_count,count);
        if(teacher_count - (count+1) < minTeacher)
        {
            strcpy(result,"UNDERFLOW\n");
            return;
        }

        char * token = strtok(out_msg.msg_val, ",");// initial part needs to be ignored e.g. AT

        token = strtok(NULL, ",");
        while( token != NULL )
        {
            deleteTeacher(token);
            teacher_count--;
            token = strtok(NULL, ",");
        }

        strcpy(result,"DONE\n");
    }
    else if(out_msg.msg_val[0] == 'D' && out_msg.msg_val[1] == 'C')
    {
        if(course_count - (count+1) < minCourses)
        {
            strcpy(result,"UNDERFLOW\n");
            return;
        }

        char * token = strtok(out_msg.msg_val, ",");// initial part needs to be ignored e.g. AT

        token = strtok(NULL, ",");
        while( token != NULL )
        {
            deleteCourse(token);
            course_count--;
            token = strtok(NULL, ",");
        }

        strcpy(result,"DONE\n");
    }
    else
    {
        strcpy(result,"FAILED\n");
    }
}

int main (int argc, char **argv)
{

    printf ("Edu_Server: Welcome !!!\n");

    minCourses = MIN_COURSES;
    maxCourses = MAX_COURSES;
    minTeacher = MIN_TEACHERS;
    maxTeacher = MAX_TEACHERS;

    pairs = (struct mapping *)malloc(MAX_COURSES * sizeof(struct mapping));
    for(int i=0;i<MAX_COURSES;i++)
    {
        pairs[i].course_name = (char *)malloc(100 * sizeof(char));
        strcpy(pairs[i].course_name,"NULL");
        pairs[i].teacher_name = (char *)malloc(100 * sizeof(char));
        strcpy(pairs[i].teacher_name,"NULL");
    }

    teachers = (struct teacher *)malloc(MAX_TEACHERS * sizeof(struct teacher));
    for(int i=0;i<MAX_TEACHERS;i++)
    {
        teachers[i].teacher_name = (char *)malloc(100 * sizeof(char));
        strcpy(teachers[i].teacher_name,"NULL");
    }

    int choice;
    printf("\nDo you want to change default values? (Yes -> 1; No -> -1): ");
    scanf("%d",&choice);

    if(choice == 1)
    {
        printf("Enter the minimum no. of courses: ");
        scanf("%d",&minCourses);
        if(!(minCourses >=MIN_COURSES && minCourses <=MAX_COURSES))
            minCourses = MIN_COURSES;

        printf("Enter the maximum no. of courses: ");
        scanf("%d",&maxCourses);
        if(!(maxCourses >=minCourses && maxCourses <=MAX_COURSES))
            maxCourses = MAX_COURSES;

        printf("Enter the minimum no. of teachers: ");
        scanf("%d",&minTeacher);
        if(!(minTeacher >=MIN_TEACHERS && minTeacher <=MAX_TEACHERS))
            minTeacher = MIN_TEACHERS;

        printf("Enter the maximum no. of teachers: ");
        scanf("%d",&maxTeacher);
        if(!(maxTeacher >=minTeacher && maxTeacher <=MAX_TEACHERS))
            maxTeacher = MAX_TEACHERS;
    }

    printf("\n\n Min Courses: %d",minCourses);
    printf("\n Max Courses: %d",maxCourses);
    printf("\n Min Teachers: %d",minTeacher);
    printf("\n Max Teachers: %d\n\n",maxTeacher);

    mqd_t qd_srv, qd_client;   // Server and Client Msg queue descriptors

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    if ((qd_srv = mq_open(SERVER_QUEUE_NAME, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS,&attr)) == -1)
    {
        perror ("Server MsgQ: mq_open (qd_srv)");
        exit (1);
    }

    client_msg_t in_msg;

    while (1)
    {
        // ssize_t mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio);
        if (mq_receive(qd_srv,(char *) &in_msg, MAX_MSG_SIZE, NULL) == -1)
        {
            perror ("Server msgq: mq_receive");
            exit (1);
        }

        printf("\nMessage Recieved from %s: %s\n", in_msg.client_q, in_msg.msg_val);

        char *result = (char *)malloc(100 * sizeof(char));
        update(in_msg, result);

		server_msg_t out_msg;
		sprintf(out_msg.msg_val, "%s", result);
		printf("%s", result);

		// Open the client queue using the client queue name received
        if ((qd_client = mq_open(in_msg.client_q, O_WRONLY)) == 1)
        {
            perror ("Server MsgQ: Not able to open the client queue");
            continue;
        }

        // Send back the value received + 10 to the client's queue
        // int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio);
        if (mq_send(qd_client, (char *) &out_msg, sizeof(out_msg), 0) == -1)
        {
            perror ("Server MsgQ: Not able to send message to the client queue");
            continue;
        }
        generateReport();
    } // end of while(1)
}  // end of main()
