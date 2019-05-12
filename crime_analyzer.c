#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include <math.h>

// Data count is the number of entries our program should read.
//#define DATA_COUNT 1900000
#define DATA_COUNT 1000
#define BUFFER 255
#define PI 3.14159265359

float distanceMeasure(float deg_lat1, float deg_long1, float deg_lat2, float deg_long2);

// 0  - areadId, 1 - city, 2 - crime, 3 - lat, 4 - long
void storeData(int index, int arrayState, char* word, int* cityId, char** cities, char** crime, float* lats, float* longs) {
    switch (arrayState)
    {
        case 0: {
            cityId[index] = atoi(word);
            break;
        }
        case 1: {
            strcpy(cities[index], word);
            break;
        }
        case 2: {
            strcpy(crime[index], word);
            break;
        }
        case 3: { // lat-long will appear like this (34.0256, -118.3248)
        
            // printf("Lat of crime is %s\n", word);
            char first[BUFFER] = "";
            char second[BUFFER] = "";
            int cut = 0;
            for (int i = 1; i < strlen(word)-1; ++i) {
                if (word[i] == ','){
                    cut = i;
                    break;
                }
                // printf("word[i] is %c:\n", word[i]);
                strcat(first, &word[i]);
            }
            // strcat(first, '\0');
            for (int i = cut+2; i < strlen(word)-1; ++i){
                // printf("word[i] is %c:\n", word[i]);
                strcat(second, &word[i]);
            }
            // strcat(second/, '\0');
            // strcpy(word, word);
            lats[index] = atof(first);
            longs[index] = atof(second);
            break;
        }
        default: printf("No proper state was found\n");
            break;
        return;
    }
}

int main( int argc, char *argv[] )
{
    int rank, size, range, startEntry, lastEntry;
	int readCount, remainder, startEnd[2], start=0, end=-1;
    int commaCount = 0;
    char c;
    char word[BUFFER] = "";
    int* cityId = malloc(DATA_COUNT * sizeof(int));
    char** cities = malloc(DATA_COUNT * sizeof(char*));
    char** crime = malloc(DATA_COUNT * sizeof(char*));
    for (int i =0; i < DATA_COUNT; i++){
        cities[i] = malloc((BUFFER+1) * sizeof(char));
        crime[i] = malloc((BUFFER+1) * sizeof(char));
    }
    float* latitudes = malloc(DATA_COUNT * sizeof(float));
    float* longitudes = malloc(DATA_COUNT * sizeof(float));
    int avoidFirstLine = -1;
    int isQuote = -1; // some of the data include quotes with extra commas, we want to ignore those commas
    int linesRead = 0;
    int state = 0; // 0  - areadId, 1 - city, 2 - crime, 3 - lat, 4 - long
    int index = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank( MPI_COMM_WORLD, &rank);
    MPI_Comm_size( MPI_COMM_WORLD, &size);
    // MPI_File mpiFile;
    // MPI_Offset mpiOffset;


	/**katelyn testing**/
	int eighty = DATA_COUNT * 0.8;
	readCount = eighty / (size-2);
	remainder = eighty % (size-2);
	
	/***/
    // let rank 0 process our file
    if(rank == 0){
        FILE *file;
        file = fopen("Crime_Data_from_2010_to_Present.csv", "r");
        // we want  city name, city name id, long/lat, and the crime occuring
        // going to ignore first line of the file
        if (file) {
            while ((c = getc(file)) != EOF){
                if (avoidFirstLine != -1) {
                    if (c == '\n') {
                        isQuote = -1;
                        commaCount = 0;
                        if (strlen(word)!=0) {
                            // printf("word -> %s\n", word);
                            storeData(index, state, word, cityId, cities, crime, latitudes, longitudes);
                            strcpy(word, "");
                        }
                        index++;
                        linesRead++;
                        if (linesRead == DATA_COUNT) {
                            break;
                        }
                    }
                    else if (c == ',' && isQuote == -1){ //new comma was found save word
                        commaCount++;
                        if (strlen(word)!=0) {
                            // printf("word -> %s\n", word);
                            storeData(index, state, word, cityId, cities, crime, latitudes, longitudes);
                            strcpy(word, "");
                        }
                    }
                    else if (c == '\"'){
                        isQuote *= -1;
                    }
                    else if (commaCount == 4){ // city id
                        state = 0;
                        strcat(word, &c);
                    }
                    else if (commaCount == 5){ // city name
                        state = 1;
                        strcat(word, &c);
                    }
                    else if (commaCount == 8){ // crime name
                        state = 2;
                        strcat(word, &c);
                    }
                    else if (commaCount == 25){ // lat
                        state = 3;
                        strcat(word, &c);
                    }
                    else if (commaCount == 26){ // long
                        // state = 4;
                        strcat(word, &c);
                    }
                }
                else if (c == '\n'){ // this is how we skip the first line
                    avoidFirstLine = 0;
                }
            }
            fclose(file);
        }
/**Katelyn testing**/
		for( int r = 0; r < size; r++ ){
			int temp = readCount;

			if( r != 0 && r != (size-1 ) ){
				if( r <= remainder ){
					temp = readCount + 1;
					//printf( "Rank: %d, Count: %d\n", r, temp );
				}
				else{
					temp = readCount;
					//printf( "Rank: %d, Count: %d\n", r, temp );
				}

				if( r == 1 ){
					start = 0;
				}
				else {
					start = end + 1;
				}
				end += temp;
				startEnd[0] = start;
				startEnd[1] = end;
				//printf( "Rank: %d, temp: %d, start: %d, end: %d\n", r, temp, start, end );
				MPI_Send( &startEnd, 2, MPI_INT, r, 0, MPI_COMM_WORLD );			
			}
		}
/*****/

    }
	else if( rank != size-1 ){
		MPI_Recv( &startEnd, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
		printf( "Rank: %d, start: %d, end: %d\n", rank, startEnd[0], startEnd[1] );

	}
	else{
		printf( "I AM %d\n", size-1 );
	}

// This is just to check if the data matches, comment/remove this later
/*    for ( int i = 0 ; i < DATA_COUNT; i++ ) {
        printf("Entry number : %d\n", i);
        printf("City ids are : %d\n", cityId[i]);
        printf("City names are : %s\n", cities[i]);
        printf("The crimes are : %s\n", crime[i]);
        printf("The Lats are : %f\n", latitudes[i]);
        printf("The Longs are : %f\n\n", longitudes[i]);
    }
*/
    MPI_Finalize();
    return 0;
}

float distanceMeasure(float deg_lat1, float deg_long1, float deg_lat2, float deg_long2) {
	//convert to radians
	double lat1 = deg_lat1 * PI / 180;
	double long1 = deg_long1 * PI / 180;
	double lat2 = deg_lat2 * PI / 180;
	double long2 = deg_long2 * PI / 180;
	double lat_diff = lat2 - lat1;
	double long_diff = long2 - long1;
	double a = sin(lat_diff/2)*sin(lat_diff/2) + cos(lat1)*cos(lat2)*sin(long_diff/2)*sin(long_diff/2);
	if(sqrt(a) < 1)
		return asin(sqrt(a));
	else
		return asin(1);
}

