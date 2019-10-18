#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
#include <stdio.h>
using namespace std;

void merge(int arr[], int left, int middle, int right)
{
    int i, j, k;                        // initializations
    int size1 = middle - left + 1; 
    int size2 =  right - middle; 
  
    int Leftarray[size1], Rightarray[size2]; 
  
    for (i = 0; i < size1; i++)                 // copy array into 2 temporary arrays
        Leftarray[i] = arr[left + i]; 
    for (j = 0; j < size2; j++) 
        Rightarray[j] = arr[middle + 1 + j]; 
  
    i = 0;      // index of first array
    j = 0; // index of second array  
    k = left;  // index of merged array

    while (i < size1 && j < size2) 
    { 
        if (Leftarray[i] <= Rightarray[j])          // if left is less than right, store the left one first
        { 
            arr[k] = Leftarray[i]; 
            i++; 
        } 
        else
        { 
            arr[k] = Rightarray[j];                 // otherwise put the right first
            j++; 
        } 
        k++; 
    }
    while (i < size1) 
    { 
        arr[k] = Leftarray[i];                  // merge whatever is left
        i++; 
        k++; 
    } 
    while (j < size2) 
    { 
        arr[k] = Rightarray[j];                 // put everything remaining
        j++; 
        k++; 
    } 
}

void mergeSort(int arr[], int left, int right)
{
        if (left < right) 
        { 
        int middle = (left + right) / 2;      // middle index of initial array 
        mergeSort(arr, left, middle);           // sort left first
        mergeSort(arr, middle + 1, right);      // sort right
        merge(arr, left, middle, right);        //merge the two
        }    
}

int main(int argc, char* argv[])
{
    int  c;
    int numarray[50];
    int i = 0;
    if(!strcmp(argv[1], "--name"))
    {
        cout << "Andrew Hsu" << endl;
    }

    else
    {
        std::fstream myfile(argv[1]);       // file input numbers into int array
        if(myfile.is_open())
        {
            while(myfile >> c)
            {
                numarray[i++] = c;
            }
        }
        mergeSort(numarray, 0, i - 1);  // sort index with left index and right index
        // mergeSort(numarray, 0, numarray.size() - 1);  // sort index with left index and right index
        myfile.close();

        ofstream outfile("out.txt");
        if(outfile.is_open())
        {
            for(int j = 0; j < i; j++)
            {
                // cout << numarray[j] << endl;
                outfile << numarray[j] << endl;
            }
        }
        outfile.close();
    }
    return 0;
}