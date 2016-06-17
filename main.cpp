/****************************************************************************
*
*
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#define MAX_NO_AP 100
//#define IFDEBUG 0
#define K 3 // can be between 3 and 5
#define TEST_DATA 10
#define LOCAL_PI 3.1415926535897932385
struct ap_signal{
    int is_reachable; //0 or 1 : for checking if a particular ap is in range at a location
    char mac_id[25];
    int signal_strength; // default = -120
};

struct location{
    double longitude;
    double latitude;
    struct ap_signal ap[MAX_NO_AP]; // No. of ap = total no of different ap in the file ; hardcoded to 100
    int is_test_data;
};

int read_file_ver1(char *line , char *fname);
int read_file_ver2(char *line , char *fname, struct location *loc);
int populate_ds(char *line, struct location *loc,int i);
int test(struct location *loc,int i);
int get_index(char *str);
double localization_knn(struct location *loc, int index, int count , int test_data);
double ToRadians(double degrees);
double DirectDistance(double lat1, double lng1, double lat2, double lng2);
double find_median(double *error, int x);
double find_percentile(double *error, int x, int percent);
typedef struct location Loc;

int compare (const void * a, const void * b)
{
  double fa = *(const double*) a;
  double fb = *(const double*) b;
  return (fa > fb) - (fa < fb);
}
int main(){
    int count = 0;

//Step 1 - Check total number of different APs in the file

    char line[1024];
    struct location *loc = NULL;
    char fname[80]; //= "test.csv";
    printf("\n Please Enter the file name : ");
    scanf("%s",fname);
    printf("\n");
    int test_data = 0;
    int training_data = 0;
    time_t t;
    int i = 0;
    double *error = NULL;
    int x = 0;
    double median = 0;
    double percentile_67 = 0;
    double percentile_90 = 0;
    int random = 0;
    count = read_file_ver1(line , fname);
    printf("\nNumber of locations to test = %d",count-1);//value is 1 more than the actual number of lines

//Step 2 Imputation step:
//initialize all the data with signal_strength = -120
    loc = (struct location*)(malloc((count-1)*sizeof(struct location)));
    for(i = 0 ; i < count -1; i++ )
    {
        loc[i].is_test_data = 0;
        for(int j = 0; j < MAX_NO_AP ; j++)
        {
            loc[i].ap[j].is_reachable = 0;
            loc[i].ap[j].signal_strength = -120;
        }

    }
//populate all the data structure reading the values from the file
read_file_ver2(line , fname ,loc);


#ifdef IFDEBUG
test(loc,count);
#endif

//DS populated for all the lines in the file before this position

//test data = 10%
//training data = 90 %
test_data = (TEST_DATA)*(count - 1)/100;
training_data = count -1 -test_data;

//apply random function and select test data


   /* Intializes random number generator */
   srand((unsigned) time(&t));

   for( i = 0 ; i < test_data ; )
   {
#ifdef IFDEBUG
      printf("%d\t", rand() % (count -1));
#endif
        random = rand() % (count -1);
        if(loc[random].is_test_data == 0)//since the same location might be chosen again by random function
        {
            loc[random].is_test_data = 1;
            i++;
        }
   }

//Step 3 : Localization
//call knn for each test data
error = (double *)malloc(test_data*sizeof(double));
if(NULL == error)
{
    printf("Error Memory Unavailable!!");
    if(NULL != loc)
        free(loc);
    return -1;
}
printf("\n Total no. Of test_data = %d \n\nList of Test Data:\n-------------------",test_data);
   for( i = 0 ; i < count-1 ; i++ )
   {
        if(loc[i].is_test_data == 1)
        {
//#ifdef IFDEBUG
                printf("\n index = %d ",i+1);
//#endif
                error[x++] = localization_knn(loc, i, count-1, test_data);//call for each test data
        }

   }
#ifdef IFDEBUG
   printf("\nLocalization Errors in meters: \n");
   for(i = 0; i < x; i++)
   {
       printf("%f\t",error[i]);
   }
#endif // IFDEBUG
qsort(error,x,sizeof(double),compare);
printf("\n\nSorted Localization Errors in meters: \n\n");
   for(i = 0; i < x; i++)
   {
       printf("%f\n",error[i]);
   }
median = find_median(error,x);
printf("\n\nError Parameters:\n--------------------\n\n");
printf("\n Median = %f\t",median);
percentile_67 = find_percentile(error, x, 67);
percentile_90 = find_percentile(error, x, 90);
printf("\n percentile_67 = %f\t",percentile_67);
printf("\n percentile_90 = %f\t",percentile_90);
if(NULL != error)
    free(error);
if(NULL != loc)
    free(loc);
    return 0;
}
double find_percentile(double *error, int x, int percent)
{
    return( error[(int)ceil(percent*x/100)] );
}
double find_median(double *error, int x)
{
    if(x%2 == 0)
    {
        return((error[x/2]+error[(x/2) -1])/2);
    }
    else
    {
        return(error[x/2]);
    }

}
double localization_knn(struct location *loc, int index, int count, int test_data) // here count is same as the no of lines in file
{

    int i = 0;
    int j = 0;
    int k = 0;
    int min =0;
    int x = 0;
    int temp1 =0, temp2 =0;
    unsigned long long sum = 0;
    unsigned long long sqrt_sum = 0;
    unsigned long long sqr = 0;
    double est_latitude = 0, est_longitude = 0;
    double distance = 0;
#ifdef IFDEBUG
    printf("\n localization_knn entry ");
    printf("\ncount_data-test_data = %d\t",count - test_data);
    printf("test_data = %d",test_data);
#endif // IFDEBUG
    int **arr = NULL;
    arr = (int **)malloc((count - test_data)*sizeof(int*));
    if(arr == NULL){
        printf("\nError Memory not available");
        return -1;
    }
    for (i = 0 ; i < count - test_data; i++  )
    {
        *(arr+i) = (int *)malloc( 2* sizeof(int));
        if(*(arr+i) == NULL){
            free (arr);
            printf("\nError Memory not available");
            return -1;
        }
    }

    for( i = 0; i< count ;i++)
    {
        sum = 0;
        sqrt_sum = 0 ;
        sqr = 0;
#ifdef IFDEBUG
            printf("\n\n i = %d",i);
            printf("\t\n j = %d",j);
#endif // IFDEBUG


        if(0 == loc[i].is_test_data)
        {


            for(k = 0; k < MAX_NO_AP ; k++)
            {
#ifdef IFDEBUG
                printf("\n k= %d",k);
                printf("\t %d",loc[index].ap[k].signal_strength);
                printf("\t %d",loc[i].ap[k].signal_strength);
                printf("\t %d",(loc[index].ap[k].signal_strength -loc[i].ap[k].signal_strength));
#endif
                 //printf("\n k= %d",k);
                sqr = (loc[index].ap[k].signal_strength -loc[i].ap[k].signal_strength);
                sum = sum + sqr * sqr;
                //printf("\n %llu",sum);
            }
#ifdef IFDEBUG
            //printf("\ncount in localization_knn = %d",count);
            //printf("\ncount - test_data localization_knn = %d",count - test_data);
            //printf("\nj in  localization_knn = %d",count - test_data);
#endif
#ifdef IFDEBUG
            printf("\n %llu",sum);
#endif
            sqrt_sum = sqrt(sum);
#ifdef IFDEBUG
            printf("\n %llu",sqrt_sum);
#endif
            arr[j][0] = (int)sqrt_sum;
            arr[j][1] = i;
            j++;
        }
        else
        {
#ifdef IFDEBUG
            printf("\n%d is a test data",i);
#endif // IFDEBUG
        }
    }
#ifdef IFDEBUG
    printf("\n\n j = %d\n",j);
#endif
    for(i =0 ; i<j; i++)
       {
#ifdef IFDEBUG
        printf("\t %d",arr[i][0]);
        printf(" %d",arr[i][1]);
#endif
       }
    //qsort (arr, j, sizeof(int), compare);
    //for(i =0 ; i<j; i++)
      //  printf("\t %d",arr[i]);
//sort here

for( x = 0 ; x < K ; x++)
{
    min = x;
    for(i = x+1 ; i<j; i++)
       {
            if(arr[i][0] <= arr[min][0])
                min = i;
       }
    temp1 = arr[x][0];
    temp2 = arr[x][1];
    arr[x][0] = arr[min][0];
    arr[x][1] = arr[min][1];
    arr[min][0] = temp1;
    arr[min][1] = temp2;
}
#ifdef IFDEBUG
printf("\n After Sorting: \n");
#endif
    for(i =0 ; i<j; i++)
       {
#ifdef IFDEBUG
        printf("\t %d",arr[i][0]);
        printf(" %d",arr[i][1]);
#endif
       }
       est_latitude = 0;
       est_longitude = 0;

for( x = 0 ; x < K ; x++)
{
    est_latitude+=loc[arr[x][1]].latitude;
    est_longitude+=loc[arr[x][1]].longitude;
}
est_latitude/=K;
est_longitude/=K;
//loc[index].latitude;
//loc[index].longitude
distance = DirectDistance(loc[index].latitude, loc[index].longitude, est_latitude, est_longitude);
#ifdef IFDEBUG
printf("\nDistance = %f",distance);
#endif // IFDEBUG
    //return localization error
for (i = 0 ; i <count - test_data; i++  )
{
    if(NULL != *(arr+1))
        free(*(arr+i));
}
if(NULL != arr)
    free(arr);

    return distance;
}

int test(struct location *loc,int i) // for printing the ds values
{
    for(int x= 0 ; x<i-1; x++)
    {
        printf("\n %d",x);
        printf("\nlatitude = %f",loc[x].latitude);
        printf("\n longitude = %f",loc[x].longitude);
        for( int c= 0; c < MAX_NO_AP ;c++)
        {
            //if(loc[x].ap[c].is_reachable)
            {
                printf("\nmac = %s",loc[x].ap[c].mac_id);
                printf("\n is reachable = %d",loc[x].ap[c].is_reachable);
                printf("\n is signal_strength = %d",loc[x].ap[c].signal_strength);
            }

        }
    }
    return 0;
}

int get_index(char *str)
{
    int index = 0;
    int len = strlen(str);
    int char_sum =0;
    if(len == 17)
    {
        for(int i = 0; i < len; i++ )
        {
            char_sum = char_sum + str[i];

        }
        index = char_sum % MAX_NO_AP;
    }
    return index;
}





// for initial read from the file to count the number of data
int read_file_ver1(char *line , char *fname)
{
    FILE *pFile;
    int count = 0;
    int count_AP = 0;
    pFile = fopen (fname,"r");
    if (pFile==NULL)
    {
        perror ("Error opening file");
        return 0;
    }
    while(!feof(pFile))
    {
        count ++;
        if(fgets (line , 1024 , pFile) != NULL );
#ifdef IFDEBUG
            puts(line);
            printf("check ");
#endif // IFDEBUG
        else
        {
        }

    }
    fclose (pFile);
    return count;
}

//2nd version of the function to populate ds for each line
int read_file_ver2(char *line , char *fname, struct location *loc)
{
    FILE *pFile = NULL;
    int count = 0;
    int count_AP = 0;
    char str[40];
    int c = 0;
    int i = 0; int j=0;
    pFile = fopen (fname,"r");
    if (pFile==NULL)
    {
        perror ("Error opening file");
        return 0;
    }
    while(!feof(pFile))
    {
        //printf("->");
        i = 0;
        c = 0;
        int flag =0;
        count ++;
        j=0;
        if(fgets (line , 1024 , pFile) != NULL )
        {
#ifdef IFDEBUG
            printf("->");
            puts (line);
            printf("\n%d\n",count-1);
#endif
            flag =1;
        }
        else
        {
            flag =0;
        }
        //call a function here to calculate the number of different APs
        while(c < 2 && flag == 1)
        {
            if(line[j] == ',')
            {
                str[i]='\0';
                i=0;j++;
                if(0 == c){
                    loc[count-1].latitude = atof(str);
#ifdef IFDEBUG
                    printf("latitude = %s \n",str);
                    printf("latitude in float= %f \n",loc[count-1].latitude );
#endif // IFDEBUG
                }
                else if(1 == c){
                    loc[count-1].longitude = atof(str);
#ifdef IFDEBUG
                    printf("longitude = %s \n",str);
                    printf("longitude in float = %f \n",loc[count-1].longitude );
#endif // IFDEBUG
                }
                c++;
                continue;
            }
            else{
                str[i] = line[j];
                i++;
                j++;
            }

        }
        if(flag)
        count_AP+=populate_ds(line,loc,count-1);
    }
    fclose (pFile);
#ifdef IFDEBUG
    printf("\nreturning from read_file_ver2\n");
#endif
    return count;
}
int populate_ds(char *line, struct location *loc , int l)
{
    int no_char = 0;
    char ch = ':';
    int is_ap = 0;
    int char_sum = 0;
    int index = 0;
    //do strtok etc here with hashing
    char *pch = NULL;
    pch = strtok (line,",");
    while (pch != NULL)
    {
        no_char = 0;
        is_ap = 0;
        char_sum = 0;
#ifdef IFDEBUG
        printf ("%s\n",pch);
#endif
        if(strlen(pch) == 17)
        {
            for(int i = 0; i < 17; i++ )
            {
                char_sum = char_sum + pch[i];
                if(pch[i] == ch)
                {
                    no_char++;
                }
            }
            if(no_char == 5) //the token is an ap
                is_ap = 1;
            if(is_ap)
            {
                index = char_sum % MAX_NO_AP;
                // fill the value of mac add
                loc[l].ap[index].is_reachable = 1;
                strcpy(loc[l].ap[index].mac_id,pch);

            }
        }
        else if(strlen(pch)<=4 && strlen(pch)>0)
        {
            //signal strength
            loc[l].ap[index].signal_strength = atoi(pch);
            //check for collision in hash table - TBD
        }
        pch = strtok (NULL, ",");
    }
#ifdef IFDEBUG
    printf("\nreturning from populate_ds\n");
#endif
    return 1;
}


double ToRadians(double degrees)
{
  double radians = degrees * LOCAL_PI / 180;
  return radians;
}

double DirectDistance(double lat1, double lng1, double lat2, double lng2)
{
  double earthRadius = 3958.75;
  double dLat = ToRadians(lat2-lat1);
  double dLng = ToRadians(lng2-lng1);
  double a = sin(dLat/2) * sin(dLat/2) +
             cos(ToRadians(lat1)) * cos(ToRadians(lat2)) *
             sin(dLng/2) * sin(dLng/2);
  double c = 2 * atan2(sqrt(a), sqrt(1-a));
  double dist = earthRadius * c;
  double meterConversion = 1609.00;
  return dist * meterConversion;
}
