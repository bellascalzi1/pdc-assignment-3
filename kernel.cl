__kernel void oddEvenSort(__global int *arr, const int n) {

    int evenIndex = get_global_id(0)*2;
    int oddIndex = evenIndex+1;

    // Odd Step
    if(arr[oddIndex + 1] < arr[oddIndex]) {
        // Swap
        int temp = arr[oddIndex + 1];
        arr[oddIndex + 1] = arr[oddIndex];
        arr[oddIndex] = temp;
    }

    barrier(CLK_GLOBAL_MEM_FENCE);

    // Even Step
    // If the step is even and we are working on the last item of the array, don't perform the
    // step since we would go out of the array bounds.
    if((evenIndex + 1) < n) {
        if(arr[evenIndex + 1] < arr[evenIndex]) {
            // Swap
            int temp = arr[evenIndex + 1];
            arr[evenIndex + 1] = arr[evenIndex];
            arr[evenIndex] = temp;
        }
    }
}