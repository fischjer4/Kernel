/*****************************************************
* FILENAME: sstf_iosched.c
* AUTHORS: Omeed Habibelahian & Jeremy Fischer
* COURSE: CS 444 - Operating Systems II (Fall 2017)
* DATE LAST MODIFIED: October 11, 2017
* SOURCES USED:

*****************************************************/
#include <stdio.h>
#include <stdlib.h>

void __merge_two(int arr[], int left[], int len_left, int right[], int len_right, int disk_head){
    int i, j = 0;

    //if the left direction is closer to the disk_head than right
    if(abs(left[len_left - 1] - disk_head) <= abs(right[0] - disk_head)){
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
    //right direction is closer
    else{
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
}

void merge_queue(int arr[], int length, int disk_head){
    int count, arr_count, right_count, i;
    //count everything to the left of the disk_head
    for(count = 0; count < length; count++){
		if(arr[count] > disk_head){
			break;
		}
    }
    int right_len = (length - count);
    int left[count], right[right_len];    

    for(arr_count = 0; arr_count < count; arr_count++){
        left[arr_count] = arr[arr_count];
	}
	for(right_count = 0; right_count < (right_len); right_count++){
        right[right_count] = arr[arr_count];
        arr_count++;
    }
    __merge_two(arr, left, count, right, (right_len), disk_head);
  

    printf("\n Left: ");
    for(i = 0; i < count; i++){
        printf("%d ", left[i]);
    }
    printf("\n Right: ");
    for(i = 0; i < right_len; i++){
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
    
    
    insertion_sort(arr, 9);
    printf("\n Whole: ");    
    for(i = 0; i < length; i++){
        printf("%d ", arr[i]);
    }

    merge_queue(arr, length, 6);
    printf("\n");
    return 0;
}