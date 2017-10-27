/*****************************************************
* FILENAME: sstf_iosched.c
* AUTHORS: Omeed Habibelahian & Jeremy Fischer
* COURSE: CS 444 - Operating Systems II (Fall 2017)
* DATE LAST MODIFIED: October 11, 2017
* SOURCES USED:

*****************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <linux/include/linux/elevator.h>

void __merge_two(int arr[], int left[], int len_left,
                int right[], int len_right, int disk_head,
                int direction){
    int i, j = 0, idle = 0;



    //Going in the Right Direction
    if(direction == 1){
        //have right direction jobs fill first
        for(i = 0; i < len_right; i++){
            arr[j] = right[i];
            j++;
        }
        //followed by left direction jobs
        for(i = len_left - 1; i >= 0; i--){
            arr[j] = left[i];
            j++;
        }
    }
    //Going in the Left Direction
    else if(direction == 0){
        //have left direction jobs fill first
        for(i = len_left - 1; i >= 0; i--){
            arr[j] = left[i];
            j++;
        }
        //followed by right direction jobs
        for(i = 0; i < len_right; i++){
            arr[j] = right[i];
            j++;
        }
    }
    //Idle doesn't run
}
void find_direction(int len_right, int left_count, int* direction){
    // if there is nothing to run, idle
    if(left_count == 0 && len_right == 0){
        (*direction) = -1;
    }
    //if moving right and there are no more tasks in direction
    else if(len_right == 0 && (*direction) == 1){
        (*direction) = 0;
    }
    //if moving left and there are no more tasks in direction
    else if(left_count == 0 && (*direction) == 0){
        (*direction) = 1;
    }
}

void merge_queue(int arr[], int length, int disk_head, int direction){
    int left_count, arr_count, right_count, i;
    //count everything to the left of the disk_head
    for(left_count = 0; left_count < length; left_count++){
		if(arr[left_count] > disk_head){
			break;
		}
    }
    int len_right = (length - left_count);
    int left[left_count], right[len_right];

    for(arr_count = 0; arr_count < left_count; arr_count++){
        left[arr_count] = arr[arr_count];
	}
	for(right_count = 0; right_count < (len_right); right_count++){
        right[right_count] = arr[arr_count];
        arr_count++;
    }
    find_direction(len_right, left_count, &direction);
    __merge_two(arr, left, left_count, right, len_right, disk_head, direction);


    printf("\n Left: ");
    for(i = 0; i < left_count; i++){
        printf("%d ", left[i]);
    }
    printf("\n Right: ");
    for(i = 0; i < len_right; i++){
        printf("%d ", right[i]);
    }

    printf("\n Merged Queue: ");
     for(i = 0; i < length; i++){
        printf("%d ", arr[i]);
    }
}

void insertion_sort(int arr[], int n){
   int i, key, j;
   for (i = 1; i < n; i++){
       key = arr[i];
       j = i-1;
       /* Move elements of arr[0..i-1], that are
          greater than key, to one position ahead
          of their current position */
       while (j >= 0 && arr[j] > key){
           arr[j+1] = arr[j];
           j = j - 1;
       }
       arr[j+1] = key;
   }
}

int main(int argc, char *argv[]){
    // int arr[] = {2,1,3,45,234,61,23,5,3};
    int arr[] = {1,4,5,9,12,13};
    int i, length = sizeof(arr) / sizeof(int);


    insertion_sort(arr, length);
    printf("\n Whole: ");
    for(i = 0; i < length; i++){
        printf("%d ", arr[i]);
    }

    merge_queue(arr, length, 6, 1);
    printf("\n");
    return 0;
}
